








#include "xpcprivate.h"
#include "nsCRT.h"
#include "XPCWrapper.h"
#include "nsWrapperCacheInlines.h"
#include "XPCLog.h"
#include "nsINode.h"
#include "XPCQuickStubs.h"
#include "jsproxy.h"
#include "AccessCheck.h"
#include "WrapperFactory.h"
#include "XrayWrapper.h"

#include "nsContentUtils.h"

#include "mozilla/StandardInteger.h"
#include "mozilla/Util.h"
#include "mozilla/Likely.h"
#include <algorithm>

using namespace xpc;
using namespace mozilla;
using namespace mozilla::dom;

bool
xpc_OkToHandOutWrapper(nsWrapperCache *cache)
{
    NS_ABORT_IF_FALSE(cache->GetWrapper(), "Must have wrapper");
    NS_ABORT_IF_FALSE(IS_WN_WRAPPER(cache->GetWrapper()),
                      "Must have XPCWrappedNative wrapper");
    return
        !static_cast<XPCWrappedNative*>(xpc_GetJSPrivate(cache->GetWrapper()))->
            NeedsSOW();
}



NS_IMETHODIMP
NS_CYCLE_COLLECTION_CLASSNAME(XPCWrappedNative)::UnlinkImpl(void *p)
{
    XPCWrappedNative *tmp = static_cast<XPCWrappedNative*>(p);
    tmp->ExpireWrapper();
    return NS_OK;
}

NS_IMETHODIMP
NS_CYCLE_COLLECTION_CLASSNAME(XPCWrappedNative)::TraverseImpl
   (NS_CYCLE_COLLECTION_CLASSNAME(XPCWrappedNative) *that, void *p,
    nsCycleCollectionTraversalCallback &cb)
{
    XPCWrappedNative *tmp = static_cast<XPCWrappedNative*>(p);
    if (!tmp->IsValid())
        return NS_OK;

    if (MOZ_UNLIKELY(cb.WantDebugInfo())) {
        char name[72];
        XPCNativeScriptableInfo* si = tmp->GetScriptableInfo();
        if (si)
            JS_snprintf(name, sizeof(name), "XPCWrappedNative (%s)",
                        si->GetJSClass()->name);
        else
            JS_snprintf(name, sizeof(name), "XPCWrappedNative");

        cb.DescribeRefCountedNode(tmp->mRefCnt.get(), name);
    } else {
        NS_IMPL_CYCLE_COLLECTION_DESCRIBE(XPCWrappedNative, tmp->mRefCnt.get())
    }

    if (tmp->mRefCnt.get() > 1) {

        
        
        
        
        
        
        
        
        

        JSObject *obj = tmp->GetFlatJSObjectPreserveColor();
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mFlatJSObject");
        cb.NoteJSChild(obj);
    }

    
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mIdentity");
    cb.NoteXPCOMChild(tmp->GetIdentityObject());

    tmp->NoteTearoffs(cb);

    return NS_OK;
}

void
XPCWrappedNative::NoteTearoffs(nsCycleCollectionTraversalCallback& cb)
{
    
    
    
    
    
    XPCWrappedNativeTearOffChunk* chunk;
    for (chunk = &mFirstChunk; chunk; chunk = chunk->mNextChunk) {
        XPCWrappedNativeTearOff* to = chunk->mTearOffs;
        for (int i = XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK-1; i >= 0; i--, to++) {
            JSObject* jso = to->GetJSObjectPreserveColor();
            if (!jso) {
                NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "tearoff's mNative");
                cb.NoteXPCOMChild(to->GetNative());
            }
        }
    }
}

#ifdef XPC_CHECK_CLASSINFO_CLAIMS
static void DEBUG_CheckClassInfoClaims(XPCWrappedNative* wrapper);
#else
#define DEBUG_CheckClassInfoClaims(wrapper) ((void)0)
#endif

#ifdef XPC_TRACK_WRAPPER_STATS
static int DEBUG_TotalWrappedNativeCount;
static int DEBUG_TotalLiveWrappedNativeCount;
static int DEBUG_TotalMaxWrappedNativeCount;
static int DEBUG_WrappedNativeWithProtoCount;
static int DEBUG_LiveWrappedNativeWithProtoCount;
static int DEBUG_MaxWrappedNativeWithProtoCount;
static int DEBUG_WrappedNativeNoProtoCount;
static int DEBUG_LiveWrappedNativeNoProtoCount;
static int DEBUG_MaxWrappedNativeNoProtoCount;
static int DEBUG_WrappedNativeTotalCalls;
static int DEBUG_WrappedNativeMethodCalls;
static int DEBUG_WrappedNativeGetterCalls;
static int DEBUG_WrappedNativeSetterCalls;
#define DEBUG_CHUNKS_TO_COUNT 4
static int DEBUG_WrappedNativeTearOffChunkCounts[DEBUG_CHUNKS_TO_COUNT+1];
static bool    DEBUG_DumpedWrapperStats;
#endif

#ifdef DEBUG
static void DEBUG_TrackNewWrapper(XPCWrappedNative* wrapper)
{
#ifdef XPC_CHECK_WRAPPERS_AT_SHUTDOWN
    if (wrapper->GetRuntime())
        wrapper->GetRuntime()->DEBUG_AddWrappedNative(wrapper);
    else
        NS_ERROR("failed to add wrapper");
#endif
#ifdef XPC_TRACK_WRAPPER_STATS
    DEBUG_TotalWrappedNativeCount++;
    DEBUG_TotalLiveWrappedNativeCount++;
    if (DEBUG_TotalMaxWrappedNativeCount < DEBUG_TotalLiveWrappedNativeCount)
        DEBUG_TotalMaxWrappedNativeCount = DEBUG_TotalLiveWrappedNativeCount;

    if (wrapper->HasProto()) {
        DEBUG_WrappedNativeWithProtoCount++;
        DEBUG_LiveWrappedNativeWithProtoCount++;
        if (DEBUG_MaxWrappedNativeWithProtoCount < DEBUG_LiveWrappedNativeWithProtoCount)
            DEBUG_MaxWrappedNativeWithProtoCount = DEBUG_LiveWrappedNativeWithProtoCount;
    } else {
        DEBUG_WrappedNativeNoProtoCount++;
        DEBUG_LiveWrappedNativeNoProtoCount++;
        if (DEBUG_MaxWrappedNativeNoProtoCount < DEBUG_LiveWrappedNativeNoProtoCount)
            DEBUG_MaxWrappedNativeNoProtoCount = DEBUG_LiveWrappedNativeNoProtoCount;
    }
#endif
}

static void DEBUG_TrackDeleteWrapper(XPCWrappedNative* wrapper)
{
#ifdef XPC_CHECK_WRAPPERS_AT_SHUTDOWN
    nsXPConnect::GetRuntimeInstance()->DEBUG_RemoveWrappedNative(wrapper);
#endif
#ifdef XPC_TRACK_WRAPPER_STATS
    DEBUG_TotalLiveWrappedNativeCount--;
    if (wrapper->HasProto())
        DEBUG_LiveWrappedNativeWithProtoCount--;
    else
        DEBUG_LiveWrappedNativeNoProtoCount--;

    int extraChunkCount = wrapper->DEBUG_CountOfTearoffChunks() - 1;
    if (extraChunkCount > DEBUG_CHUNKS_TO_COUNT)
        extraChunkCount = DEBUG_CHUNKS_TO_COUNT;
    DEBUG_WrappedNativeTearOffChunkCounts[extraChunkCount]++;
#endif
}
static void DEBUG_TrackWrapperCall(XPCWrappedNative* wrapper,
                                   XPCWrappedNative::CallMode mode)
{
#ifdef XPC_TRACK_WRAPPER_STATS
    DEBUG_WrappedNativeTotalCalls++;
    switch (mode) {
        case XPCWrappedNative::CALL_METHOD:
            DEBUG_WrappedNativeMethodCalls++;
            break;
        case XPCWrappedNative::CALL_GETTER:
            DEBUG_WrappedNativeGetterCalls++;
            break;
        case XPCWrappedNative::CALL_SETTER:
            DEBUG_WrappedNativeSetterCalls++;
            break;
        default:
            NS_ERROR("bad value");
    }
#endif
}

static void DEBUG_TrackShutdownWrapper(XPCWrappedNative* wrapper)
{
#ifdef XPC_TRACK_WRAPPER_STATS
    if (!DEBUG_DumpedWrapperStats) {
        DEBUG_DumpedWrapperStats = true;
        printf("%d WrappedNatives were constructed. "
               "(%d w/ protos, %d w/o)\n",
               DEBUG_TotalWrappedNativeCount,
               DEBUG_WrappedNativeWithProtoCount,
               DEBUG_WrappedNativeNoProtoCount);

        printf("%d WrappedNatives max alive at one time. "
               "(%d w/ protos, %d w/o)\n",
               DEBUG_TotalMaxWrappedNativeCount,
               DEBUG_MaxWrappedNativeWithProtoCount,
               DEBUG_MaxWrappedNativeNoProtoCount);

        printf("%d WrappedNatives alive now. "
               "(%d w/ protos, %d w/o)\n",
               DEBUG_TotalLiveWrappedNativeCount,
               DEBUG_LiveWrappedNativeWithProtoCount,
               DEBUG_LiveWrappedNativeNoProtoCount);

        printf("%d calls to WrappedNatives. "
               "(%d methods, %d getters, %d setters)\n",
               DEBUG_WrappedNativeTotalCalls,
               DEBUG_WrappedNativeMethodCalls,
               DEBUG_WrappedNativeGetterCalls,
               DEBUG_WrappedNativeSetterCalls);

        printf("(wrappers / tearoffs): (");
        int i;
        for (i = 0; i < DEBUG_CHUNKS_TO_COUNT; i++) {
            printf("%d / %d, ",
                   DEBUG_WrappedNativeTearOffChunkCounts[i],
                   (i+1) * XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK);
        }
        printf("%d / more)\n", DEBUG_WrappedNativeTearOffChunkCounts[i]);
    }
#endif
}
#else
#define DEBUG_TrackNewWrapper(wrapper) ((void)0)
#define DEBUG_TrackDeleteWrapper(wrapper) ((void)0)
#define DEBUG_TrackWrapperCall(wrapper, mode) ((void)0)
#define DEBUG_TrackShutdownWrapper(wrapper) ((void)0)
#endif


static nsresult
FinishCreate(XPCCallContext& ccx,
             XPCWrappedNativeScope* Scope,
             XPCNativeInterface* Interface,
             nsWrapperCache *cache,
             XPCWrappedNative* inWrapper,
             XPCWrappedNative** resultWrapper);











nsresult
XPCWrappedNative::WrapNewGlobal(XPCCallContext &ccx, xpcObjectHelper &nativeHelper,
                                nsIPrincipal *principal, bool initStandardClasses,
                                JS::ZoneSpecifier zoneSpec,
                                XPCWrappedNative **wrappedGlobal)
{
    nsISupports *identity = nativeHelper.GetCanonical();

    
    MOZ_ASSERT(nativeHelper.GetScriptableFlags() & nsIXPCScriptable::IS_GLOBAL_OBJECT);

    
    MOZ_ASSERT(!nativeHelper.GetWrapperCache() ||
               !nativeHelper.GetWrapperCache()->GetWrapperPreserveColor());

    
    XPCNativeScriptableCreateInfo sciProto;
    XPCNativeScriptableCreateInfo sciMaybe;
    const XPCNativeScriptableCreateInfo& sciWrapper =
        GatherScriptableCreateInfo(identity, nativeHelper.GetClassInfo(),
                                   sciProto, sciMaybe);

    
    
    AutoMarkingNativeScriptableInfoPtr
        si(ccx, XPCNativeScriptableInfo::Construct(ccx, &sciWrapper));
    MOZ_ASSERT(si.get());

    
    JSClass *clasp = si->GetJSClass();
    MOZ_ASSERT(clasp->flags & JSCLASS_IS_GLOBAL);

    
    JSObject *global = xpc::CreateGlobalObject(ccx, clasp, principal, zoneSpec);
    if (!global)
        return NS_ERROR_FAILURE;
    XPCWrappedNativeScope *scope = GetCompartmentPrivate(global)->scope;

    
    
    JSAutoCompartment ac(ccx, global);

    
    if (initStandardClasses && ! JS_InitStandardClasses(ccx, global))
        return NS_ERROR_FAILURE;

    
    XPCWrappedNativeProto *proto =
        XPCWrappedNativeProto::GetNewOrUsed(ccx,
                                            scope,
                                            nativeHelper.GetClassInfo(), &sciProto,
                                            UNKNOWN_OFFSETS,  false);
    if (!proto)
        return NS_ERROR_FAILURE;
    proto->CacheOffsets(identity);

    
    MOZ_ASSERT(proto->GetJSProtoObject());
    bool success = JS_SplicePrototype(ccx, global, proto->GetJSProtoObject());
    if (!success)
        return NS_ERROR_FAILURE;

    
    nsRefPtr<XPCWrappedNative> wrapper = new XPCWrappedNative(identity, proto);

    
    nativeHelper.forgetCanonical();

    
    
    
    

    
    
    
    
    
    
    
    XPCNativeScriptableInfo* siProto = proto->GetScriptableInfo();
    if (siProto && siProto->GetCallback() == sciWrapper.GetCallback()) {
        wrapper->mScriptableInfo = siProto;
        delete si;
    } else {
        wrapper->mScriptableInfo = si;
    }

    
    wrapper->mFlatJSObject = global;

    
    JS_SetPrivate(global, wrapper);

    
    
    
    
    AutoMarkingWrappedNativePtr wrapperMarker(ccx, wrapper);

    
    
    
    
    success = wrapper->FinishInit(ccx);
    MOZ_ASSERT(success);

    
    
    
    
    
    XPCNativeInterface* iface = XPCNativeInterface::GetNewOrUsed(ccx, &NS_GET_IID(nsISupports));
    MOZ_ASSERT(iface);
    nsresult status;
    success = wrapper->FindTearOff(ccx, iface, false, &status);
    if (!success)
        return status;

    
    
    
    return FinishCreate(ccx, scope, iface, nativeHelper.GetWrapperCache(),
                        wrapper, wrappedGlobal);
}


nsresult
XPCWrappedNative::GetNewOrUsed(XPCCallContext& ccx,
                               xpcObjectHelper& helper,
                               XPCWrappedNativeScope* Scope,
                               XPCNativeInterface* Interface,
                               XPCWrappedNative** resultWrapper)
{
    nsWrapperCache *cache = helper.GetWrapperCache();

    NS_ASSERTION(!cache || !cache->GetWrapperPreserveColor(),
                 "We assume the caller already checked if it could get the "
                 "wrapper from the cache.");

    nsresult rv;

    NS_ASSERTION(!Scope->GetRuntime()->GetThreadRunningGC(),
                 "XPCWrappedNative::GetNewOrUsed called during GC");

    nsISupports *identity = helper.GetCanonical();

    if (!identity) {
        NS_ERROR("This XPCOM object fails in QueryInterface to nsISupports!");
        return NS_ERROR_FAILURE;
    }

    XPCLock* mapLock = Scope->GetRuntime()->GetMapLock();

    nsRefPtr<XPCWrappedNative> wrapper;

    Native2WrappedNativeMap* map = Scope->GetWrappedNativeMap();
    if (!cache) {
        {   
            XPCAutoLock lock(mapLock);
            wrapper = map->Find(identity);
        }

        if (wrapper) {
            if (Interface &&
                !wrapper->FindTearOff(ccx, Interface, false, &rv)) {
                NS_ASSERTION(NS_FAILED(rv), "returning NS_OK on failure");
                return rv;
            }
            *resultWrapper = wrapper.forget().get();
            return NS_OK;
        }
    }
#ifdef DEBUG
    else if (!cache->GetWrapperPreserveColor())
    {   
        XPCAutoLock lock(mapLock);
        NS_ASSERTION(!map->Find(identity),
                     "There's a wrapper in the hashtable but it wasn't cached?");
    }
#endif

    
    
    
    
    
    
    
    

    
    
    
    bool iidIsClassInfo = Interface &&
                          Interface->GetIID()->Equals(NS_GET_IID(nsIClassInfo));
    uint32_t classInfoFlags;
    bool isClassInfoSingleton = helper.GetClassInfo() == helper.Object() &&
                                NS_SUCCEEDED(helper.GetClassInfo()
                                                   ->GetFlags(&classInfoFlags)) &&
                                (classInfoFlags & nsIClassInfo::SINGLETON_CLASSINFO);
    bool isClassInfo = iidIsClassInfo || isClassInfoSingleton;

    nsIClassInfo *info = helper.GetClassInfo();

    XPCNativeScriptableCreateInfo sciProto;
    XPCNativeScriptableCreateInfo sci;

    
    
    
    
    
    
    
    const XPCNativeScriptableCreateInfo& sciWrapper =
        isClassInfo ? sci :
        GatherScriptableCreateInfo(identity, info, sciProto, sci);

    JSObject* parent = Scope->GetGlobalJSObject();

    jsval newParentVal = JSVAL_NULL;
    XPCMarkableJSVal newParentVal_markable(&newParentVal);
    AutoMarkingJSVal newParentVal_automarker(ccx, &newParentVal_markable);
    JSBool needsSOW = false;
    JSBool needsCOW = false;

    mozilla::Maybe<JSAutoCompartment> ac;

    if (sciWrapper.GetFlags().WantPreCreate()) {
        
        js::AutoMaybeTouchDeadZones agc(parent);

        JSObject* plannedParent = parent;
        nsresult rv = sciWrapper.GetCallback()->PreCreate(identity, ccx,
                                                          parent, &parent);
        if (NS_FAILED(rv))
            return rv;

        if (rv == NS_SUCCESS_CHROME_ACCESS_ONLY)
            needsSOW = true;
        rv = NS_OK;

        NS_ASSERTION(!xpc::WrapperFactory::IsXrayWrapper(parent),
                     "Xray wrapper being used to parent XPCWrappedNative?");

        ac.construct(ccx, parent);

        if (parent != plannedParent) {
            XPCWrappedNativeScope* betterScope = GetObjectScope(parent);
            if (betterScope != Scope)
                return GetNewOrUsed(ccx, helper, betterScope, Interface, resultWrapper);

            newParentVal = OBJECT_TO_JSVAL(parent);
        }

        
        
        

        if (cache) {
            JSObject *cached = cache->GetWrapper();
            if (cached) {
                if (IS_SLIM_WRAPPER_OBJECT(cached)) {
                    if (NS_FAILED(XPCWrappedNative::Morph(ccx, cached,
                          Interface, cache, getter_AddRefs(wrapper))))
                        return NS_ERROR_FAILURE;
                } else {
                    wrapper = static_cast<XPCWrappedNative*>(xpc_GetJSPrivate(cached));
                }
            }
        } else {
            
            XPCAutoLock lock(mapLock);
            wrapper = map->Find(identity);
        }

        if (wrapper) {
            if (Interface && !wrapper->FindTearOff(ccx, Interface, false, &rv)) {
                NS_ASSERTION(NS_FAILED(rv), "returning NS_OK on failure");
                return rv;
            }
            *resultWrapper = wrapper.forget().get();
            return NS_OK;
        }
    } else {
        ac.construct(ccx, parent);

        nsISupports *Object = helper.Object();
        if (nsXPCWrappedJSClass::IsWrappedJS(Object)) {
            nsCOMPtr<nsIXPConnectWrappedJS> wrappedjs(do_QueryInterface(Object));
            JSObject *obj;
            wrappedjs->GetJSObject(&obj);
            if (xpc::AccessCheck::isChrome(js::GetObjectCompartment(obj)) &&
                !xpc::AccessCheck::isChrome(js::GetObjectCompartment(Scope->GetGlobalJSObject()))) {
                needsCOW = true;
            }
        }
    }

    AutoMarkingWrappedNativeProtoPtr proto(ccx);

    
    

    
    

    if (info && !isClassInfo) {
        proto = XPCWrappedNativeProto::GetNewOrUsed(ccx, Scope, info, &sciProto);
        if (!proto)
            return NS_ERROR_FAILURE;

        proto->CacheOffsets(identity);

        wrapper = new XPCWrappedNative(identity, proto);
        if (!wrapper)
            return NS_ERROR_FAILURE;
    } else {
        AutoMarkingNativeInterfacePtr iface(ccx, Interface);
        if (!iface)
            iface = XPCNativeInterface::GetISupports(ccx);

        AutoMarkingNativeSetPtr set(ccx);
        set = XPCNativeSet::GetNewOrUsed(ccx, nullptr, iface, 0);

        if (!set)
            return NS_ERROR_FAILURE;

        wrapper = new XPCWrappedNative(identity, Scope, set);
        if (!wrapper)
            return NS_ERROR_FAILURE;

        DEBUG_ReportShadowedMembers(set, wrapper, nullptr);
    }

    
    
    helper.forgetCanonical();

    NS_ASSERTION(!xpc::WrapperFactory::IsXrayWrapper(parent),
                 "Xray wrapper being used to parent XPCWrappedNative?");

    
    
    
    
    AutoMarkingWrappedNativePtr wrapperMarker(ccx, wrapper);

    if (!wrapper->Init(ccx, parent, &sciWrapper))
        return NS_ERROR_FAILURE;

    if (Interface && !wrapper->FindTearOff(ccx, Interface, false, &rv)) {
        NS_ASSERTION(NS_FAILED(rv), "returning NS_OK on failure");
        return rv;
    }

    if (needsSOW)
        wrapper->SetNeedsSOW();
    if (needsCOW)
        wrapper->SetNeedsCOW();

    return FinishCreate(ccx, Scope, Interface, cache, wrapper, resultWrapper);
}

static nsresult
FinishCreate(XPCCallContext& ccx,
             XPCWrappedNativeScope* Scope,
             XPCNativeInterface* Interface,
             nsWrapperCache *cache,
             XPCWrappedNative* inWrapper,
             XPCWrappedNative** resultWrapper)
{
    MOZ_ASSERT(inWrapper);

#if DEBUG_xpc_leaks
    {
        char* s = wrapper->ToString(ccx);
        NS_ASSERTION(wrapper->IsValid(), "eh?");
        printf("Created wrapped native %s, flat JSObject is %p\n",
               s, (void*)wrapper->GetFlatJSObjectNoMark());
        if (s)
            JS_smprintf_free(s);
    }
#endif

    XPCLock* mapLock = Scope->GetRuntime()->GetMapLock();
    Native2WrappedNativeMap* map = Scope->GetWrappedNativeMap();

    nsRefPtr<XPCWrappedNative> wrapper;
    {   

        
        
        
        
        XPCAutoLock lock(mapLock);
        wrapper = map->Add(inWrapper);
        if (!wrapper)
            return NS_ERROR_FAILURE;
    }

    if (wrapper == inWrapper) {
        JSObject *flat = wrapper->GetFlatJSObject();
        NS_ASSERTION(!cache || !cache->GetWrapperPreserveColor() ||
                     flat == cache->GetWrapperPreserveColor(),
                     "This object has a cached wrapper that's different from "
                     "the JSObject held by its native wrapper?");

        if (cache && !cache->GetWrapperPreserveColor())
            cache->SetWrapper(flat);

        
        
        XPCNativeScriptableInfo* si = wrapper->GetScriptableInfo();
        if (si && si->GetFlags().WantPostCreate()) {
            nsresult rv = si->GetCallback()->PostCreate(wrapper, ccx, flat);
            if (NS_FAILED(rv)) {
                
                
                
                
                
                
                
                
                NS_ERROR("PostCreate failed! This is known to cause "
                         "inconsistent state for some class types and may even "
                         "cause a crash in combination with a JS GC. Fix the "
                         "failing PostCreate ASAP!");

                {   
                    XPCAutoLock lock(mapLock);
                    map->Remove(wrapper);
                }

                
                

                if (cache)
                    cache->ClearWrapper();
                wrapper->Release();
                return rv;
            }
        }
    }

    DEBUG_CheckClassInfoClaims(wrapper);
    *resultWrapper = wrapper.forget().get();
    return NS_OK;
}


nsresult
XPCWrappedNative::Morph(XPCCallContext& ccx,
                        JSObject* existingJSObject,
                        XPCNativeInterface* Interface,
                        nsWrapperCache *cache,
                        XPCWrappedNative** resultWrapper)
{
    NS_ASSERTION(IS_SLIM_WRAPPER(existingJSObject),
                 "Trying to morph a JSObject that's not a slim wrapper?");

    nsISupports *identity =
        static_cast<nsISupports*>(xpc_GetJSPrivate(existingJSObject));
    XPCWrappedNativeProto *proto = GetSlimWrapperProto(existingJSObject);

#if DEBUG
    
    
#if 0
    if (proto->GetScriptableInfo()->GetFlags().WantPreCreate()) {
        JSObject* parent = JS_GetParent(existingJSObject);
        JSObject* plannedParent = parent;
        nsresult rv =
            proto->GetScriptableInfo()->GetCallback()->PreCreate(identity, ccx,
                                                                 parent,
                                                                 &parent);
        if (NS_FAILED(rv))
            return rv;

        NS_ASSERTION(parent == plannedParent,
                     "PreCreate returned a different parent");
    }
#endif
#endif

    nsRefPtr<XPCWrappedNative> wrapper = new XPCWrappedNative(dont_AddRef(identity), proto);
    if (!wrapper)
        return NS_ERROR_FAILURE;

    NS_ASSERTION(!xpc::WrapperFactory::IsXrayWrapper(js::GetObjectParent(existingJSObject)),
                 "Xray wrapper being used to parent XPCWrappedNative?");

    
    
    
    
    AutoMarkingWrappedNativePtr wrapperMarker(ccx, wrapper);

    JSAutoCompartment ac(ccx, existingJSObject);
    if (!wrapper->Init(ccx, existingJSObject))
        return NS_ERROR_FAILURE;

    nsresult rv;
    if (Interface && !wrapper->FindTearOff(ccx, Interface, false, &rv)) {
        NS_ASSERTION(NS_FAILED(rv), "returning NS_OK on failure");
        return rv;
    }

    return FinishCreate(ccx, wrapper->GetScope(), Interface, cache, wrapper, resultWrapper);
}


nsresult
XPCWrappedNative::GetUsedOnly(XPCCallContext& ccx,
                              nsISupports* Object,
                              XPCWrappedNativeScope* Scope,
                              XPCNativeInterface* Interface,
                              XPCWrappedNative** resultWrapper)
{
    NS_ASSERTION(Object, "XPCWrappedNative::GetUsedOnly was called with a null Object");

    XPCWrappedNative* wrapper;
    nsWrapperCache* cache = nullptr;
    CallQueryInterface(Object, &cache);
    if (cache) {
        JSObject *flat = cache->GetWrapper();
        if (flat && IS_SLIM_WRAPPER_OBJECT(flat) && !MorphSlimWrapper(ccx, flat))
           return NS_ERROR_FAILURE;

        wrapper = flat ?
                  static_cast<XPCWrappedNative*>(xpc_GetJSPrivate(flat)) :
                  nullptr;

        if (!wrapper) {
            *resultWrapper = nullptr;
            return NS_OK;
        }
        NS_ADDREF(wrapper);
    } else {
        nsCOMPtr<nsISupports> identity = do_QueryInterface(Object);

        if (!identity) {
            NS_ERROR("This XPCOM object fails in QueryInterface to nsISupports!");
            return NS_ERROR_FAILURE;
        }

        Native2WrappedNativeMap* map = Scope->GetWrappedNativeMap();

        {   
            XPCAutoLock lock(Scope->GetRuntime()->GetMapLock());
            wrapper = map->Find(identity);
            if (!wrapper) {
                *resultWrapper = nullptr;
                return NS_OK;
            }
            NS_ADDREF(wrapper);
        }
    }

    nsresult rv;
    if (Interface && !wrapper->FindTearOff(ccx, Interface, false, &rv)) {
        NS_RELEASE(wrapper);
        NS_ASSERTION(NS_FAILED(rv), "returning NS_OK on failure");
        return rv;
    }

    *resultWrapper = wrapper;
    return NS_OK;
}


XPCWrappedNative::XPCWrappedNative(already_AddRefed<nsISupports> aIdentity,
                                   XPCWrappedNativeProto* aProto)
    : mMaybeProto(aProto),
      mSet(aProto->GetSet()),
      mFlatJSObject(INVALID_OBJECT), 
      mScriptableInfo(nullptr),
      mWrapperWord(0)
{
    mIdentity = aIdentity.get();

    NS_ASSERTION(mMaybeProto, "bad ctor param");
    NS_ASSERTION(mSet, "bad ctor param");

    DEBUG_TrackNewWrapper(this);
}


XPCWrappedNative::XPCWrappedNative(already_AddRefed<nsISupports> aIdentity,
                                   XPCWrappedNativeScope* aScope,
                                   XPCNativeSet* aSet)

    : mMaybeScope(TagScope(aScope)),
      mSet(aSet),
      mFlatJSObject(INVALID_OBJECT), 
      mScriptableInfo(nullptr),
      mWrapperWord(0)
{
    mIdentity = aIdentity.get();

    NS_ASSERTION(aScope, "bad ctor param");
    NS_ASSERTION(aSet, "bad ctor param");

    DEBUG_TrackNewWrapper(this);
}

XPCWrappedNative::~XPCWrappedNative()
{
    DEBUG_TrackDeleteWrapper(this);

    Destroy();
}

static const intptr_t WRAPPER_WORD_POISON = 0xa8a8a8a8;

void
XPCWrappedNative::Destroy()
{
    XPCWrappedNativeProto* proto = GetProto();

    if (mScriptableInfo &&
        (!HasProto() ||
         (proto && proto->GetScriptableInfo() != mScriptableInfo))) {
        delete mScriptableInfo;
        mScriptableInfo = nullptr;
    }

    XPCWrappedNativeScope *scope = GetScope();
    if (scope) {
        Native2WrappedNativeMap* map = scope->GetWrappedNativeMap();

        
        XPCAutoLock lock(GetRuntime()->GetMapLock());

        
        
        map->Remove(this);
    }

    if (mIdentity) {
        XPCJSRuntime* rt = GetRuntime();
        if (rt && rt->GetDoingFinalization()) {
            if (rt->DeferredRelease(mIdentity)) {
                mIdentity = nullptr;
            } else {
                NS_WARNING("Failed to append object for deferred release.");
                
                NS_RELEASE(mIdentity);
            }
        } else {
            NS_RELEASE(mIdentity);
        }
    }

    





    if (XPCJSRuntime *rt = GetRuntime()) {
        if (JS::IsIncrementalBarrierNeeded(rt->GetJSRuntime()))
            JS::IncrementalObjectBarrier(GetWrapperPreserveColor());
        mWrapperWord = WRAPPER_WORD_POISON;
    } else {
        MOZ_ASSERT(mWrapperWord == WRAPPER_WORD_POISON);
    }

    mMaybeScope = nullptr;
}

void
XPCWrappedNative::UpdateScriptableInfo(XPCNativeScriptableInfo *si)
{
    NS_ASSERTION(mScriptableInfo, "UpdateScriptableInfo expects an existing scriptable info");

    
    JSRuntime* rt = GetRuntime()->GetJSRuntime();
    if (JS::IsIncrementalBarrierNeeded(rt))
        mScriptableInfo->Mark();

    mScriptableInfo = si;
}

void
XPCWrappedNative::SetProto(XPCWrappedNativeProto* p)
{
    NS_ASSERTION(!IsWrapperExpired(), "bad ptr!");

    MOZ_ASSERT(HasProto());

    
    JSRuntime* rt = GetRuntime()->GetJSRuntime();
    GetProto()->WriteBarrierPre(rt);

    mMaybeProto = p;
}



void
XPCWrappedNative::GatherProtoScriptableCreateInfo(nsIClassInfo* classInfo,
                                                  XPCNativeScriptableCreateInfo& sciProto)
{
    NS_ASSERTION(classInfo, "bad param");
    NS_ASSERTION(!sciProto.GetCallback(), "bad param");

    nsXPCClassInfo *classInfoHelper = nullptr;
    CallQueryInterface(classInfo, &classInfoHelper);
    if (classInfoHelper) {
        nsCOMPtr<nsIXPCScriptable> helper =
          dont_AddRef(static_cast<nsIXPCScriptable*>(classInfoHelper));
        uint32_t flags = classInfoHelper->GetScriptableFlags();
        sciProto.SetCallback(helper.forget());
        sciProto.SetFlags(flags);
        sciProto.SetInterfacesBitmap(classInfoHelper->GetInterfacesBitmap());

        return;
    }

    nsCOMPtr<nsISupports> possibleHelper;
    nsresult rv = classInfo->GetHelperForLanguage(nsIProgrammingLanguage::JAVASCRIPT,
                                                  getter_AddRefs(possibleHelper));
    if (NS_SUCCEEDED(rv) && possibleHelper) {
        nsCOMPtr<nsIXPCScriptable> helper(do_QueryInterface(possibleHelper));
        if (helper) {
            uint32_t flags = helper->GetScriptableFlags();
            sciProto.SetCallback(helper.forget());
            sciProto.SetFlags(flags);
        }
    }
}


const XPCNativeScriptableCreateInfo&
XPCWrappedNative::GatherScriptableCreateInfo(nsISupports* obj,
                                             nsIClassInfo* classInfo,
                                             XPCNativeScriptableCreateInfo& sciProto,
                                             XPCNativeScriptableCreateInfo& sciWrapper)
{
    NS_ASSERTION(!sciWrapper.GetCallback(), "bad param");

    
    if (classInfo) {
        GatherProtoScriptableCreateInfo(classInfo, sciProto);

        if (sciProto.GetFlags().DontAskInstanceForScriptable())
            return sciProto;
    }

    
    nsCOMPtr<nsIXPCScriptable> helper(do_QueryInterface(obj));
    if (helper) {
        uint32_t flags = helper->GetScriptableFlags();
        sciWrapper.SetCallback(helper.forget());
        sciWrapper.SetFlags(flags);

        
        

        NS_ASSERTION(!(sciWrapper.GetFlags().WantPreCreate() &&
                       !sciProto.GetFlags().WantPreCreate()),
                     "Can't set WANT_PRECREATE on an instance scriptable "
                     "without also setting it on the class scriptable");

        NS_ASSERTION(!(sciWrapper.GetFlags().DontEnumStaticProps() &&
                       !sciProto.GetFlags().DontEnumStaticProps() &&
                       sciProto.GetCallback()),
                     "Can't set DONT_ENUM_STATIC_PROPS on an instance scriptable "
                     "without also setting it on the class scriptable (if present and shared)");

        NS_ASSERTION(!(sciWrapper.GetFlags().DontEnumQueryInterface() &&
                       !sciProto.GetFlags().DontEnumQueryInterface() &&
                       sciProto.GetCallback()),
                     "Can't set DONT_ENUM_QUERY_INTERFACE on an instance scriptable "
                     "without also setting it on the class scriptable (if present and shared)");

        NS_ASSERTION(!(sciWrapper.GetFlags().DontAskInstanceForScriptable() &&
                       !sciProto.GetFlags().DontAskInstanceForScriptable()),
                     "Can't set DONT_ASK_INSTANCE_FOR_SCRIPTABLE on an instance scriptable "
                     "without also setting it on the class scriptable");

        NS_ASSERTION(!(sciWrapper.GetFlags().ClassInfoInterfacesOnly() &&
                       !sciProto.GetFlags().ClassInfoInterfacesOnly() &&
                       sciProto.GetCallback()),
                     "Can't set CLASSINFO_INTERFACES_ONLY on an instance scriptable "
                     "without also setting it on the class scriptable (if present and shared)");

        NS_ASSERTION(!(sciWrapper.GetFlags().AllowPropModsDuringResolve() &&
                       !sciProto.GetFlags().AllowPropModsDuringResolve() &&
                       sciProto.GetCallback()),
                     "Can't set ALLOW_PROP_MODS_DURING_RESOLVE on an instance scriptable "
                     "without also setting it on the class scriptable (if present and shared)");

        NS_ASSERTION(!(sciWrapper.GetFlags().AllowPropModsToPrototype() &&
                       !sciProto.GetFlags().AllowPropModsToPrototype() &&
                       sciProto.GetCallback()),
                     "Can't set ALLOW_PROP_MODS_TO_PROTOTYPE on an instance scriptable "
                     "without also setting it on the class scriptable (if present and shared)");

        return sciWrapper;
    }

    return sciProto;
}

#ifdef DEBUG_slimwrappers
static uint32_t sMorphedSlimWrappers;
#endif

JSBool
XPCWrappedNative::Init(XPCCallContext& ccx, JSObject* parent,
                       const XPCNativeScriptableCreateInfo* sci)
{
    

    if (sci->GetCallback()) {
        if (HasProto()) {
            XPCNativeScriptableInfo* siProto = GetProto()->GetScriptableInfo();
            if (siProto && siProto->GetCallback() == sci->GetCallback())
                mScriptableInfo = siProto;
        }
        if (!mScriptableInfo) {
            mScriptableInfo =
                XPCNativeScriptableInfo::Construct(ccx, sci);

            if (!mScriptableInfo)
                return false;
        }
    }
    XPCNativeScriptableInfo* si = mScriptableInfo;

    

    JSClass* jsclazz = si ? si->GetJSClass() : Jsvalify(&XPC_WN_NoHelper_JSClass.base);

    
    MOZ_ASSERT_IF(si, !!si->GetFlags().IsGlobalObject() == !!(jsclazz->flags & JSCLASS_IS_GLOBAL));

    NS_ASSERTION(jsclazz &&
                 jsclazz->name &&
                 jsclazz->flags &&
                 jsclazz->addProperty &&
                 jsclazz->delProperty &&
                 jsclazz->getProperty &&
                 jsclazz->setProperty &&
                 jsclazz->enumerate &&
                 jsclazz->resolve &&
                 jsclazz->convert &&
                 jsclazz->finalize, "bad class");

    JSObject* protoJSObject = HasProto() ?
                                GetProto()->GetJSProtoObject() :
                                GetScope()->GetPrototypeNoHelper(ccx);

    if (!protoJSObject) {
        return false;
    }

    mFlatJSObject = JS_NewObject(ccx, jsclazz, protoJSObject, parent);
    if (!mFlatJSObject)
        return false;

    JS_SetPrivate(mFlatJSObject, this);

    return FinishInit(ccx);
}

JSBool
XPCWrappedNative::Init(XPCCallContext &ccx, JSObject *existingJSObject)
{
    
    JS_SetPrivate(existingJSObject, this);

    
    MorphMultiSlot(existingJSObject);

    mScriptableInfo = GetProto()->GetScriptableInfo();
    mFlatJSObject = existingJSObject;

    SLIM_LOG(("----- %i morphed slim wrapper (mFlatJSObject: %p, %p)\n",
              ++sMorphedSlimWrappers, mFlatJSObject,
              static_cast<nsISupports*>(xpc_GetJSPrivate(mFlatJSObject))));

    return FinishInit(ccx);
}

JSBool
XPCWrappedNative::FinishInit(XPCCallContext &ccx)
{
    
    
    
    JS_SetReservedSlot(mFlatJSObject, WRAPPER_MULTISLOT, JSVAL_NULL);

    
    
    
    NS_ASSERTION(1 == mRefCnt, "unexpected refcount value");
    NS_ADDREF(this);

    if (mScriptableInfo && mScriptableInfo->GetFlags().WantCreate() &&
        NS_FAILED(mScriptableInfo->GetCallback()->Create(this, ccx,
                                                         mFlatJSObject))) {
        return false;
    }

    
    JS_updateMallocCounter(ccx.GetJSContext(), 2 * sizeof(XPCWrappedNative));

    return true;
}


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(XPCWrappedNative)
  NS_INTERFACE_MAP_ENTRY(nsIXPConnectWrappedNative)
  NS_INTERFACE_MAP_ENTRY(nsIXPConnectJSObjectHolder)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPConnectWrappedNative)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(XPCWrappedNative)
NS_IMPL_THREADSAFE_RELEASE(XPCWrappedNative)






















































void
XPCWrappedNative::FlatJSObjectFinalized()
{
    if (!IsValid())
        return;

    
    
    
    

    XPCWrappedNativeTearOffChunk* chunk;
    for (chunk = &mFirstChunk; chunk; chunk = chunk->mNextChunk) {
        XPCWrappedNativeTearOff* to = chunk->mTearOffs;
        for (int i = XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK-1; i >= 0; i--, to++) {
            JSObject* jso = to->GetJSObjectPreserveColor();
            if (jso) {
                NS_ASSERTION(JS_IsAboutToBeFinalized(&jso), "bad!");
                JS_SetPrivate(jso, nullptr);
                to->JSObjectFinalized();
            }

            
            nsISupports* obj = to->GetNative();
            if (obj) {
#ifdef XP_WIN
                
                NS_ASSERTION(*(int*)obj != 0xdddddddd, "bad pointer!");
                NS_ASSERTION(*(int*)obj != 0,          "bad pointer!");
#endif
                XPCJSRuntime* rt = GetRuntime();
                if (rt) {
                    if (!rt->DeferredRelease(obj)) {
                        NS_WARNING("Failed to append object for deferred release.");
                        
                        obj->Release();
                    }
                } else {
                    obj->Release();
                }
                to->SetNative(nullptr);
            }

            to->SetInterface(nullptr);
        }
    }

    nsWrapperCache *cache = nullptr;
    CallQueryInterface(mIdentity, &cache);
    if (cache)
        cache->ClearWrapper();

    
    mFlatJSObject = nullptr;

    NS_ASSERTION(mIdentity, "bad pointer!");
#ifdef XP_WIN
    
    NS_ASSERTION(*(int*)mIdentity != 0xdddddddd, "bad pointer!");
    NS_ASSERTION(*(int*)mIdentity != 0,          "bad pointer!");
#endif

    if (IsWrapperExpired()) {
        Destroy();
    }

    
    

    Release();
}

void
XPCWrappedNative::SystemIsBeingShutDown()
{
#ifdef DEBUG_xpc_hacker
    {
        printf("Removing root for still-live XPCWrappedNative %p wrapping:\n",
               static_cast<void*>(this));
        for (uint16_t i = 0, i_end = mSet->GetInterfaceCount(); i < i_end; ++i) {
            nsXPIDLCString name;
            mSet->GetInterfaceAt(i)->GetInterfaceInfo()
                ->GetName(getter_Copies(name));
            printf("  %s\n", name.get());
        }
    }
#endif
    DEBUG_TrackShutdownWrapper(this);

    if (!IsValid())
        return;

    
    
    

    

    
    JS_SetPrivate(mFlatJSObject, nullptr);
    mFlatJSObject = nullptr; 

    XPCWrappedNativeProto* proto = GetProto();

    if (HasProto())
        proto->SystemIsBeingShutDown();

    if (mScriptableInfo &&
        (!HasProto() ||
         (proto && proto->GetScriptableInfo() != mScriptableInfo))) {
        delete mScriptableInfo;
    }

    

    XPCWrappedNativeTearOffChunk* chunk;
    for (chunk = &mFirstChunk; chunk; chunk = chunk->mNextChunk) {
        XPCWrappedNativeTearOff* to = chunk->mTearOffs;
        for (int i = XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK-1; i >= 0; i--, to++) {
            if (JSObject *jso = to->GetJSObjectPreserveColor()) {
                JS_SetPrivate(jso, nullptr);
                to->SetJSObject(nullptr);
            }
            
            
            to->SetNative(nullptr);
            to->SetInterface(nullptr);
        }
    }

    if (mFirstChunk.mNextChunk) {
        delete mFirstChunk.mNextChunk;
        mFirstChunk.mNextChunk = nullptr;
    }
}




class AutoClonePrivateGuard NS_STACK_CLASS {
public:
    AutoClonePrivateGuard(JSObject *aOld, JSObject *aNew)
        : mOldReflector(aOld), mNewReflector(aNew)
    {
        MOZ_ASSERT(JS_GetPrivate(aOld) == JS_GetPrivate(aNew));
    }

    ~AutoClonePrivateGuard()
    {
        if (JS_GetPrivate(mOldReflector)) {
            JS_SetPrivate(mNewReflector, nullptr);
        }
    }

private:
    JSObject* mOldReflector;
    JSObject* mNewReflector;
};


nsresult
XPCWrappedNative::ReparentWrapperIfFound(XPCCallContext& ccx,
                                         XPCWrappedNativeScope* aOldScope,
                                         XPCWrappedNativeScope* aNewScope,
                                         JSObject* aNewParent,
                                         nsISupports* aCOMObj)
{
    XPCNativeInterface* iface =
        XPCNativeInterface::GetISupports(ccx);

    if (!iface)
        return NS_ERROR_FAILURE;

    nsresult rv;

    nsRefPtr<XPCWrappedNative> wrapper;
    JSObject *flat = nullptr;
    nsWrapperCache* cache = nullptr;
    CallQueryInterface(aCOMObj, &cache);
    if (cache) {
        flat = cache->GetWrapper();
        if (flat && !IS_SLIM_WRAPPER_OBJECT(flat)) {
            wrapper = static_cast<XPCWrappedNative*>(xpc_GetJSPrivate(flat));
            NS_ASSERTION(wrapper->GetScope() == aOldScope,
                         "Incorrect scope passed");
        }
    } else {
        rv = XPCWrappedNative::GetUsedOnly(ccx, aCOMObj, aOldScope, iface,
                                           getter_AddRefs(wrapper));
        if (NS_FAILED(rv))
            return rv;

        if (wrapper)
            flat = wrapper->GetFlatJSObject();
    }

    if (!flat)
        return NS_OK;

    
    
    
    if (wrapper &&
        wrapper->GetProto() &&
        !wrapper->GetProto()->ClassIsMainThreadOnly()) {
        return NS_ERROR_FAILURE;
    }

    JSAutoCompartment ac(ccx, aNewScope->GetGlobalJSObject());

    if (aOldScope != aNewScope) {
        
        AutoMarkingWrappedNativeProtoPtr oldProto(ccx);
        AutoMarkingWrappedNativeProtoPtr newProto(ccx);

        
        MOZ_ASSERT(js::GetObjectCompartment(aOldScope->GetGlobalJSObject()) !=
                   js::GetObjectCompartment(aNewScope->GetGlobalJSObject()));
        NS_ASSERTION(aNewParent, "won't be able to find the new parent");
        NS_ASSERTION(wrapper, "can't transplant slim wrappers");

        if (!wrapper)
            oldProto = GetSlimWrapperProto(flat);
        else if (wrapper->HasProto())
            oldProto = wrapper->GetProto();

        if (oldProto) {
            XPCNativeScriptableInfo *info = oldProto->GetScriptableInfo();
            XPCNativeScriptableCreateInfo ci(*info);
            newProto =
                XPCWrappedNativeProto::GetNewOrUsed(ccx, aNewScope,
                                                    oldProto->GetClassInfo(),
                                                    &ci, oldProto->GetOffsetsMasked());
            if (!newProto) {
                return NS_ERROR_FAILURE;
            }
        }

        if (wrapper) {

            
            
            
            
            

            JSObject *newobj = JS_CloneObject(ccx, flat,
                                              newProto->GetJSProtoObject(),
                                              aNewParent);
            if (!newobj)
                return NS_ERROR_FAILURE;

            
            
            
            
            
            
            
            JSObject *propertyHolder;
            {
                AutoClonePrivateGuard cloneGuard(flat, newobj);

                propertyHolder = JS_NewObjectWithGivenProto(ccx, NULL, NULL, aNewParent);
                if (!propertyHolder)
                    return NS_ERROR_OUT_OF_MEMORY;
                if (!JS_CopyPropertiesFrom(ccx, propertyHolder, flat))
                    return NS_ERROR_FAILURE;

                
                
                SetWNExpandoChain(newobj, nullptr);
                if (!XrayUtils::CloneExpandoChain(ccx, newobj, flat))
                    return NS_ERROR_FAILURE;

                
                
                
                
                
                
                JS_SetPrivate(flat, nullptr);
            }

            
            
            
            {
                JSAutoCompartment innerAC(ccx, aOldScope->GetGlobalJSObject());
                if (!wrapper->GetSameCompartmentSecurityWrapper(ccx))
                    return NS_ERROR_FAILURE;
            }

            
            
            {   
                Native2WrappedNativeMap* oldMap = aOldScope->GetWrappedNativeMap();
                Native2WrappedNativeMap* newMap = aNewScope->GetWrappedNativeMap();
                XPCAutoLock lock(aOldScope->GetRuntime()->GetMapLock());

                oldMap->Remove(wrapper);

                if (wrapper->HasProto())
                    wrapper->SetProto(newProto);

                
                
                

                if (wrapper->mScriptableInfo &&
                    wrapper->mScriptableInfo == oldProto->GetScriptableInfo()) {
                    
                    
                    
                    

                    NS_ASSERTION(oldProto->GetScriptableInfo()->GetScriptableShared() ==
                                 newProto->GetScriptableInfo()->GetScriptableShared(),
                                 "Changing proto is also changing JSObject Classname or "
                                 "helper's nsIXPScriptable flags. This is not allowed!");

                    wrapper->UpdateScriptableInfo(newProto->GetScriptableInfo());
                }

                
                if (newMap->Find(wrapper->GetIdentityObject()))
                    MOZ_CRASH();

                if (!newMap->Add(wrapper))
                    MOZ_CRASH();
            }

            JSObject *ww = wrapper->GetWrapper();
            if (ww) {
                JSObject *newwrapper;
                MOZ_ASSERT(wrapper->NeedsSOW(), "weird wrapper wrapper");
                newwrapper = xpc::WrapperFactory::WrapSOWObject(ccx, newobj);
                if (!newwrapper)
                    MOZ_CRASH();

                
                ww = xpc::TransplantObjectWithWrapper(ccx, flat, ww, newobj,
                                                      newwrapper);
                if (!ww)
                    MOZ_CRASH();

                flat = newobj;
                wrapper->SetWrapper(ww);
            } else {
                flat = xpc::TransplantObject(ccx, flat, newobj);
                if (!flat)
                    MOZ_CRASH();
            }

            wrapper->mFlatJSObject = flat;
            if (cache) {
                bool preserving = cache->PreservingWrapper();
                cache->SetPreservingWrapper(false);
                cache->SetWrapper(flat);
                cache->SetPreservingWrapper(preserving);
            }
            if (!JS_CopyPropertiesFrom(ccx, flat, propertyHolder))
                MOZ_CRASH();
        } else {
            SetSlimWrapperProto(flat, newProto.get());
            if (!JS_SetPrototype(ccx, flat, newProto->GetJSProtoObject()))
                MOZ_CRASH(); 
        }

        
        XPCNativeScriptableInfo* si = wrapper->GetScriptableInfo();
        if (si->GetFlags().WantPostCreate())
            (void) si->GetCallback()->PostTransplant(wrapper, ccx, flat);
    }

    

    if (aNewParent) {
        if (!JS_SetParent(ccx, flat, aNewParent))
            MOZ_CRASH();

        JSObject *nw;
        if (wrapper &&
            (nw = wrapper->GetWrapper()) &&
            !JS_SetParent(ccx, nw, JS_GetGlobalForObject(ccx, aNewParent))) {
            MOZ_CRASH();
        }
    }

    return NS_OK;
}

















static nsresult
RescueOrphans(XPCCallContext& ccx, JSObject* obj)
{
    
    
    
    

    
    
    
    
    
    
    nsresult rv;
    JSObject *parentObj = js::GetObjectParent(obj);
    if (!parentObj)
        return NS_OK; 
    parentObj = js::UnwrapObject(parentObj,  false);

    
    js::AutoMaybeTouchDeadZones agc(parentObj);

    bool isWN = IS_WRAPPER_CLASS(js::GetObjectClass(obj));

    
    
    
    
    
    
    if (MOZ_UNLIKELY(JS_IsDeadWrapper(parentObj))) {
        if (isWN) {
            XPCWrappedNative *wn =
                static_cast<XPCWrappedNative*>(js::GetObjectPrivate(obj));
            rv = wn->GetScriptableInfo()->GetCallback()->PreCreate(wn->GetIdentityObject(), ccx,
                                                           wn->GetScope()->GetGlobalJSObject(),
                                                           &parentObj);
            NS_ENSURE_SUCCESS(rv, rv);
        } else {
            MOZ_ASSERT(IsDOMObject(obj));
            const DOMClass* domClass = GetDOMClass(obj);
            parentObj = domClass->mGetParent(ccx, obj);
        }
    }

    
    if (IS_SLIM_WRAPPER(parentObj)) {
        bool ok = MorphSlimWrapper(ccx, parentObj);
        NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);
    }

    
    rv = RescueOrphans(ccx, parentObj);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (!js::IsCrossCompartmentWrapper(parentObj))
        return NS_OK;

    
    if (isWN) {
        JSObject *realParent = js::UnwrapObject(parentObj);
        XPCWrappedNative *wn =
            static_cast<XPCWrappedNative*>(js::GetObjectPrivate(obj));
        return wn->ReparentWrapperIfFound(ccx, GetObjectScope(parentObj),
                                          GetObjectScope(realParent),
                                          realParent, wn->GetIdentityObject());
    }

    return ReparentWrapper(ccx, obj);
}





nsresult
XPCWrappedNative::RescueOrphans(XPCCallContext& ccx)
{
    return ::RescueOrphans(ccx, mFlatJSObject);
}

JSBool
XPCWrappedNative::ExtendSet(XPCCallContext& ccx, XPCNativeInterface* aInterface)
{
    

    if (!mSet->HasInterface(aInterface)) {
        AutoMarkingNativeSetPtr newSet(ccx);
        newSet = XPCNativeSet::GetNewOrUsed(ccx, mSet, aInterface,
                                            mSet->GetInterfaceCount());
        if (!newSet)
            return false;

        mSet = newSet;

        DEBUG_ReportShadowedMembers(newSet, this, GetProto());
    }
    return true;
}

XPCWrappedNativeTearOff*
XPCWrappedNative::LocateTearOff(XPCCallContext& ccx,
                                XPCNativeInterface* aInterface)
{
    XPCAutoLock al(GetLock()); 

    for (XPCWrappedNativeTearOffChunk* chunk = &mFirstChunk;
         chunk != nullptr;
         chunk = chunk->mNextChunk) {
        XPCWrappedNativeTearOff* tearOff = chunk->mTearOffs;
        XPCWrappedNativeTearOff* const end = tearOff +
            XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK;
        for (tearOff = chunk->mTearOffs;
             tearOff < end;
             tearOff++) {
            if (tearOff->GetInterface() == aInterface) {
                return tearOff;
            }
        }
    }
    return nullptr;
}

XPCWrappedNativeTearOff*
XPCWrappedNative::FindTearOff(XPCCallContext& ccx,
                              XPCNativeInterface* aInterface,
                              JSBool needJSObject ,
                              nsresult* pError )
{
    XPCAutoLock al(GetLock()); 

    nsresult rv = NS_OK;
    XPCWrappedNativeTearOff* to;
    XPCWrappedNativeTearOff* firstAvailable = nullptr;

    XPCWrappedNativeTearOffChunk* lastChunk;
    XPCWrappedNativeTearOffChunk* chunk;
    for (lastChunk = chunk = &mFirstChunk;
         chunk;
         lastChunk = chunk, chunk = chunk->mNextChunk) {
        to = chunk->mTearOffs;
        XPCWrappedNativeTearOff* const end = chunk->mTearOffs +
            XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK;
        for (to = chunk->mTearOffs;
             to < end;
             to++) {
            if (to->GetInterface() == aInterface) {
                if (needJSObject && !to->GetJSObjectPreserveColor()) {
                    AutoMarkingWrappedNativeTearOffPtr tearoff(ccx, to);
                    JSBool ok = InitTearOffJSObject(ccx, to);
                    
                    
                    
                    
                    to->Unmark();
                    if (!ok) {
                        to = nullptr;
                        rv = NS_ERROR_OUT_OF_MEMORY;
                    }
                }
                goto return_result;
            }
            if (!firstAvailable && to->IsAvailable())
                firstAvailable = to;
        }
    }

    to = firstAvailable;

    if (!to) {
        XPCWrappedNativeTearOffChunk* newChunk =
            new XPCWrappedNativeTearOffChunk();
        if (!newChunk) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            goto return_result;
        }
        lastChunk->mNextChunk = newChunk;
        to = newChunk->mTearOffs;
    }

    {
        
        AutoMarkingWrappedNativeTearOffPtr tearoff(ccx, to);
        rv = InitTearOff(ccx, to, aInterface, needJSObject);
        
        
        
        to->Unmark();
        if (NS_FAILED(rv))
            to = nullptr;
    }

return_result:

    if (pError)
        *pError = rv;
    return to;
}

nsresult
XPCWrappedNative::InitTearOff(XPCCallContext& ccx,
                              XPCWrappedNativeTearOff* aTearOff,
                              XPCNativeInterface* aInterface,
                              JSBool needJSObject)
{
    

    

    const nsIID* iid = aInterface->GetIID();
    nsISupports* identity = GetIdentityObject();
    nsISupports* obj;

    
    
    if (mScriptableInfo &&
        mScriptableInfo->GetFlags().ClassInfoInterfacesOnly() &&
        !mSet->HasInterface(aInterface) &&
        !mSet->HasInterfaceWithAncestor(aInterface)) {
        return NS_ERROR_NO_INTERFACE;
    }

    
    

    aTearOff->SetReserved();

    {   
        XPCAutoUnlock unlock(GetLock());

        if (NS_FAILED(identity->QueryInterface(*iid, (void**)&obj)) || !obj) {
            aTearOff->SetInterface(nullptr);
            return NS_ERROR_NO_INTERFACE;
        }

        
        if (iid->Equals(NS_GET_IID(nsIClassInfo))) {
            nsCOMPtr<nsISupports> alternate_identity(do_QueryInterface(obj));
            if (alternate_identity.get() != identity) {
                NS_RELEASE(obj);
                aTearOff->SetInterface(nullptr);
                return NS_ERROR_NO_INTERFACE;
            }
        }

        
        
        
        
        
        
        
        
        
        
        
        

        
        

        nsCOMPtr<nsIXPConnectWrappedJS> wrappedJS(do_QueryInterface(obj));
        if (wrappedJS) {
            JSObject* jso = nullptr;
            if (NS_SUCCEEDED(wrappedJS->GetJSObject(&jso)) &&
                jso == mFlatJSObject) {
                
                
                
                
                
                
                
                
                
                
                

#ifdef DEBUG_xpc_hacker
                {
                    
                    
                    
                    
                    
                    
                    
                    if (HasProto()) {
                        JSObject* proto  = nullptr;
                        JSObject* our_proto = GetProto()->GetJSProtoObject();

                        proto = jso->getProto();

                        NS_ASSERTION(proto && proto != our_proto,
                                     "!!! xpconnect/xbl check - wrapper has no special proto");

                        bool found_our_proto = false;
                        while (proto && !found_our_proto) {
                            proto = proto->getProto();

                            found_our_proto = proto == our_proto;
                        }

                        NS_ASSERTION(found_our_proto,
                                     "!!! xpconnect/xbl check - wrapper has extra proto");
                    } else {
                        NS_WARNING("!!! xpconnect/xbl check - wrapper has no proto");
                    }
                }
#endif
                NS_RELEASE(obj);
                aTearOff->SetInterface(nullptr);
                return NS_OK;
            }

            
            
            
            
            
            
            
            
            
            

            nsXPCWrappedJSClass* clazz;
            if (iid->Equals(NS_GET_IID(nsIPropertyBag)) && jso &&
                NS_SUCCEEDED(nsXPCWrappedJSClass::GetNewOrUsed(ccx,*iid,&clazz))&&
                clazz) {
                JSObject* answer =
                    clazz->CallQueryInterfaceOnJSObject(ccx, jso, *iid);
                NS_RELEASE(clazz);
                if (!answer) {
                    NS_RELEASE(obj);
                    aTearOff->SetInterface(nullptr);
                    return NS_ERROR_NO_INTERFACE;
                }
            }
        }

        nsIXPCSecurityManager* sm;
           sm = ccx.GetXPCContext()->GetAppropriateSecurityManager(nsIXPCSecurityManager::HOOK_CREATE_WRAPPER);
        if (sm && NS_FAILED(sm->
                            CanCreateWrapper(ccx, *iid, identity,
                                             GetClassInfo(), GetSecurityInfoAddr()))) {
            
            NS_RELEASE(obj);
            aTearOff->SetInterface(nullptr);
            return NS_ERROR_XPC_SECURITY_MANAGER_VETO;
        }
    }
    

    
    
    
    

    if (!mSet->HasInterface(aInterface) && !ExtendSet(ccx, aInterface)) {
        NS_RELEASE(obj);
        aTearOff->SetInterface(nullptr);
        return NS_ERROR_NO_INTERFACE;
    }

    aTearOff->SetInterface(aInterface);
    aTearOff->SetNative(obj);
    if (needJSObject && !InitTearOffJSObject(ccx, aTearOff))
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

JSBool
XPCWrappedNative::InitTearOffJSObject(XPCCallContext& ccx,
                                      XPCWrappedNativeTearOff* to)
{
    

    JSObject* obj = JS_NewObject(ccx, Jsvalify(&XPC_WN_Tearoff_JSClass),
                                 JS_GetObjectPrototype(ccx, mFlatJSObject),
                                 mFlatJSObject);
    if (!obj)
        return false;

    JS_SetPrivate(obj, to);
    to->SetJSObject(obj);
    return true;
}

JSObject*
XPCWrappedNative::GetSameCompartmentSecurityWrapper(JSContext *cx)
{
    
    JSObject *flat = GetFlatJSObject();
    JSObject *wrapper = GetWrapper();

    
    if (wrapper)
        return wrapper;

    
    JSCompartment *cxCompartment = js::GetContextCompartment(cx);
    MOZ_ASSERT(cxCompartment == js::GetObjectCompartment(flat));
    if (xpc::AccessCheck::isChrome(cxCompartment)) {
        MOZ_ASSERT(wrapper == NULL);
        return flat;
    }

    
    
    
    if (NeedsSOW()) {
        wrapper = xpc::WrapperFactory::WrapSOWObject(cx, flat);
        if (!wrapper)
            return NULL;
    } else if (xpc::WrapperFactory::IsComponentsObject(flat)) {
        wrapper = xpc::WrapperFactory::WrapComponentsObject(cx, flat);
        if (!wrapper)
            return NULL;
    }

    
    if (wrapper) {
        SetWrapper(wrapper);
        return wrapper;
    }

    
    return flat;
}



static JSBool Throw(nsresult errNum, XPCCallContext& ccx)
{
    XPCThrower::Throw(errNum, ccx);
    return false;
}



class CallMethodHelper
{
    XPCCallContext& mCallContext;
    nsIInterfaceInfo* const mIFaceInfo;
    const nsXPTMethodInfo* mMethodInfo;
    nsISupports* const mCallee;
    const uint16_t mVTableIndex;
    const jsid mIdxValueId;

    nsAutoTArray<nsXPTCVariant, 8> mDispatchParams;
    uint8_t mJSContextIndex; 
    uint8_t mOptArgcIndex; 

    jsval* const mArgv;
    const uint32_t mArgc;

    JS_ALWAYS_INLINE JSBool
    GetArraySizeFromParam(uint8_t paramIndex, uint32_t* result) const;

    JS_ALWAYS_INLINE JSBool
    GetInterfaceTypeFromParam(uint8_t paramIndex,
                              const nsXPTType& datum_type,
                              nsID* result) const;

    JS_ALWAYS_INLINE JSBool
    GetOutParamSource(uint8_t paramIndex, jsval* srcp) const;

    JS_ALWAYS_INLINE JSBool
    GatherAndConvertResults();

    JS_ALWAYS_INLINE JSBool
    QueryInterfaceFastPath() const;

    nsXPTCVariant*
    GetDispatchParam(uint8_t paramIndex)
    {
        if (paramIndex >= mJSContextIndex)
            paramIndex += 1;
        if (paramIndex >= mOptArgcIndex)
            paramIndex += 1;
        return &mDispatchParams[paramIndex];
    }
    const nsXPTCVariant*
    GetDispatchParam(uint8_t paramIndex) const
    {
        return const_cast<CallMethodHelper*>(this)->GetDispatchParam(paramIndex);
    }

    JS_ALWAYS_INLINE JSBool InitializeDispatchParams();

    JS_ALWAYS_INLINE JSBool ConvertIndependentParams(JSBool* foundDependentParam);
    JS_ALWAYS_INLINE JSBool ConvertIndependentParam(uint8_t i);
    JS_ALWAYS_INLINE JSBool ConvertDependentParams();
    JS_ALWAYS_INLINE JSBool ConvertDependentParam(uint8_t i);

    JS_ALWAYS_INLINE void CleanupParam(nsXPTCMiniVariant& param, nsXPTType& type);

    JS_ALWAYS_INLINE JSBool HandleDipperParam(nsXPTCVariant* dp,
                                              const nsXPTParamInfo& paramInfo);

    JS_ALWAYS_INLINE nsresult Invoke();

public:

    CallMethodHelper(XPCCallContext& ccx)
        : mCallContext(ccx)
        , mIFaceInfo(ccx.GetInterface()->GetInterfaceInfo())
        , mMethodInfo(nullptr)
        , mCallee(ccx.GetTearOff()->GetNative())
        , mVTableIndex(ccx.GetMethodIndex())
        , mIdxValueId(ccx.GetRuntime()->GetStringID(XPCJSRuntime::IDX_VALUE))
        , mJSContextIndex(UINT8_MAX)
        , mOptArgcIndex(UINT8_MAX)
        , mArgv(ccx.GetArgv())
        , mArgc(ccx.GetArgc())

    {
        
        mIFaceInfo->GetMethodInfo(mVTableIndex, &mMethodInfo);
    }

    ~CallMethodHelper();

    JS_ALWAYS_INLINE JSBool Call();

};


NS_SUPPRESS_STACK_CHECK JSBool
XPCWrappedNative::CallMethod(XPCCallContext& ccx,
                             CallMode mode )
{
    XPCContext* xpcc = ccx.GetXPCContext();
    NS_ASSERTION(xpcc->CallerTypeIsJavaScript(),
                 "Native caller for XPCWrappedNative::CallMethod?");

    nsresult rv = ccx.CanCallNow();
    if (NS_FAILED(rv)) {
        return Throw(rv, ccx);
    }

    DEBUG_TrackWrapperCall(ccx.GetWrapper(), mode);

    

    uint32_t secFlag;
    uint32_t secAction;

    switch (mode) {
        case CALL_METHOD:
            secFlag   = nsIXPCSecurityManager::HOOK_CALL_METHOD;
            secAction = nsIXPCSecurityManager::ACCESS_CALL_METHOD;
            break;
        case CALL_GETTER:
            secFlag   = nsIXPCSecurityManager::HOOK_GET_PROPERTY;
            secAction = nsIXPCSecurityManager::ACCESS_GET_PROPERTY;
            break;
        case CALL_SETTER:
            secFlag   = nsIXPCSecurityManager::HOOK_SET_PROPERTY;
            secAction = nsIXPCSecurityManager::ACCESS_SET_PROPERTY;
            break;
        default:
            NS_ERROR("bad value");
            return false;
    }

    nsIXPCSecurityManager* sm =
        xpcc->GetAppropriateSecurityManager(secFlag);
    if (sm && NS_FAILED(sm->CanAccess(secAction, &ccx, ccx,
                                      ccx.GetFlattenedJSObject(),
                                      ccx.GetWrapper()->GetIdentityObject(),
                                      ccx.GetWrapper()->GetClassInfo(),
                                      ccx.GetMember()->GetName(),
                                      ccx.GetWrapper()->GetSecurityInfoAddr()))) {
        
        return false;
    }

    return CallMethodHelper(ccx).Call();
}

JSBool
CallMethodHelper::Call()
{
    mCallContext.SetRetVal(JSVAL_VOID);

    XPCJSRuntime::Get()->SetPendingException(nullptr);
    mCallContext.GetXPCContext()->SetLastResult(NS_ERROR_UNEXPECTED);

    if (mVTableIndex == 0) {
        return QueryInterfaceFastPath();
    }

    if (!mMethodInfo) {
        Throw(NS_ERROR_XPC_CANT_GET_METHOD_INFO, mCallContext);
        return false;
    }

    if (!InitializeDispatchParams())
        return false;

    
    
    
    
    JSBool foundDependentParam = false;
    if (!ConvertIndependentParams(&foundDependentParam))
        return false;

    if (foundDependentParam && !ConvertDependentParams())
        return false;

    nsresult invokeResult = Invoke();

    mCallContext.GetXPCContext()->SetLastResult(invokeResult);

    if (JS_IsExceptionPending(mCallContext)) {
        return false;
    }

    if (NS_FAILED(invokeResult)) {
        ThrowBadResult(invokeResult, mCallContext);
        return false;
    }

    return GatherAndConvertResults();
}

CallMethodHelper::~CallMethodHelper()
{
    uint8_t paramCount = mMethodInfo->GetParamCount();
    if (mDispatchParams.Length()) {
        for (uint8_t i = 0; i < paramCount; i++) {
            nsXPTCVariant* dp = GetDispatchParam(i);
            const nsXPTParamInfo& paramInfo = mMethodInfo->GetParam(i);

            if (paramInfo.GetType().IsArray()) {
                void* p = dp->val.p;
                if (!p)
                    continue;

                
                if (dp->DoesValNeedCleanup()) {
                    
                    uint32_t array_count = 0;
                    nsXPTType datum_type;
                    if (!GetArraySizeFromParam(i, &array_count) ||
                        !NS_SUCCEEDED(mIFaceInfo->GetTypeForParam(mVTableIndex,
                                                                  &paramInfo,
                                                                  1, &datum_type))) {
                        
                        
                        NS_ERROR("failed to get array information, we'll leak here");
                        continue;
                    }

                    
                    
                    for (uint32_t k = 0; k < array_count; k++) {
                        nsXPTCMiniVariant v;
                        v.val.p = static_cast<void**>(p)[k];
                        CleanupParam(v, datum_type);
                    }
                }

                
                nsMemory::Free(p);
            } else {
                
                if (dp->DoesValNeedCleanup())
                    CleanupParam(*dp, dp->type);
            }
        }
    }

}

JSBool
CallMethodHelper::GetArraySizeFromParam(uint8_t paramIndex,
                                        uint32_t* result) const
{
    nsresult rv;
    const nsXPTParamInfo& paramInfo = mMethodInfo->GetParam(paramIndex);

    

    rv = mIFaceInfo->GetSizeIsArgNumberForParam(mVTableIndex, &paramInfo, 0, &paramIndex);
    if (NS_FAILED(rv))
        return Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, mCallContext);

    *result = GetDispatchParam(paramIndex)->val.u32;

    return true;
}

JSBool
CallMethodHelper::GetInterfaceTypeFromParam(uint8_t paramIndex,
                                            const nsXPTType& datum_type,
                                            nsID* result) const
{
    nsresult rv;
    const nsXPTParamInfo& paramInfo = mMethodInfo->GetParam(paramIndex);
    uint8_t tag = datum_type.TagPart();

    

    if (tag == nsXPTType::T_INTERFACE) {
        rv = mIFaceInfo->GetIIDForParamNoAlloc(mVTableIndex, &paramInfo, result);
        if (NS_FAILED(rv))
            return ThrowBadParam(NS_ERROR_XPC_CANT_GET_PARAM_IFACE_INFO,
                                 paramIndex, mCallContext);
    } else if (tag == nsXPTType::T_INTERFACE_IS) {
        rv = mIFaceInfo->GetInterfaceIsArgNumberForParam(mVTableIndex, &paramInfo,
                                                         &paramIndex);
        if (NS_FAILED(rv))
            return Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, mCallContext);

        nsID* p = (nsID*) GetDispatchParam(paramIndex)->val.p;
        if (!p)
            return ThrowBadParam(NS_ERROR_XPC_CANT_GET_PARAM_IFACE_INFO,
                                 paramIndex, mCallContext);
        *result = *p;
    }
    return true;
}

JSBool
CallMethodHelper::GetOutParamSource(uint8_t paramIndex, jsval* srcp) const
{
    const nsXPTParamInfo& paramInfo = mMethodInfo->GetParam(paramIndex);

    if ((paramInfo.IsOut() || paramInfo.IsDipper()) &&
        !paramInfo.IsRetval()) {
        NS_ASSERTION(paramIndex < mArgc || paramInfo.IsOptional(),
                     "Expected either enough arguments or an optional argument");
        jsval arg = paramIndex < mArgc ? mArgv[paramIndex] : JSVAL_NULL;
        if (paramIndex < mArgc &&
            (JSVAL_IS_PRIMITIVE(arg) ||
             !JS_GetPropertyById(mCallContext,
                                 JSVAL_TO_OBJECT(arg),
                                 mIdxValueId,
                                 srcp))) {
            
            
            
            ThrowBadParam(NS_ERROR_XPC_NEED_OUT_OBJECT, paramIndex,
                          mCallContext);
            return false;
        }
    }

    return true;
}

JSBool
CallMethodHelper::GatherAndConvertResults()
{
    
    uint8_t paramCount = mMethodInfo->GetParamCount();
    for (uint8_t i = 0; i < paramCount; i++) {
        const nsXPTParamInfo& paramInfo = mMethodInfo->GetParam(i);
        if (!paramInfo.IsOut() && !paramInfo.IsDipper())
            continue;

        const nsXPTType& type = paramInfo.GetType();
        nsXPTCVariant* dp = GetDispatchParam(i);
        jsval v = JSVAL_NULL;
        AUTO_MARK_JSVAL(mCallContext, &v);
        uint32_t array_count = 0;
        nsXPTType datum_type;
        bool isArray = type.IsArray();
        bool isSizedString = isArray ?
                false :
                type.TagPart() == nsXPTType::T_PSTRING_SIZE_IS ||
                type.TagPart() == nsXPTType::T_PWSTRING_SIZE_IS;

        if (isArray) {
            if (NS_FAILED(mIFaceInfo->GetTypeForParam(mVTableIndex, &paramInfo, 1,
                                                      &datum_type))) {
                Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, mCallContext);
                return false;
            }
        } else
            datum_type = type;

        if (isArray || isSizedString) {
            if (!GetArraySizeFromParam(i, &array_count))
                return false;
        }

        nsID param_iid;
        if (datum_type.IsInterfacePointer() &&
            !GetInterfaceTypeFromParam(i, datum_type, &param_iid))
            return false;

        nsresult err;
        if (isArray) {
            XPCLazyCallContext lccx(mCallContext);
            if (!XPCConvert::NativeArray2JS(lccx, &v, (const void**)&dp->val,
                                            datum_type, &param_iid,
                                            array_count, &err)) {
                
                ThrowBadParam(err, i, mCallContext);
                return false;
            }
        } else if (isSizedString) {
            if (!XPCConvert::NativeStringWithSize2JS(mCallContext, &v,
                                                     (const void*)&dp->val,
                                                     datum_type,
                                                     array_count, &err)) {
                ThrowBadParam(err, i, mCallContext);
                return false;
            }
        } else {
            if (!XPCConvert::NativeData2JS(mCallContext, &v, &dp->val, datum_type,
                                           &param_iid, &err)) {
                ThrowBadParam(err, i, mCallContext);
                return false;
            }
        }

        if (paramInfo.IsRetval()) {
            mCallContext.SetRetVal(v);
        } else if (i < mArgc) {
            
            NS_ASSERTION(mArgv[i].isObject(), "out var is not object");
            if (!JS_SetPropertyById(mCallContext,
                                    &mArgv[i].toObject(),
                                    mIdxValueId, &v)) {
                ThrowBadParam(NS_ERROR_XPC_CANT_SET_OUT_VAL, i, mCallContext);
                return false;
            }
        } else {
            NS_ASSERTION(paramInfo.IsOptional(),
                         "Expected either enough arguments or an optional argument");
        }
    }

    return true;
}

JSBool
CallMethodHelper::QueryInterfaceFastPath() const
{
    NS_ASSERTION(mVTableIndex == 0,
                 "Using the QI fast-path for a method other than QueryInterface");

    if (mArgc < 1) {
        Throw(NS_ERROR_XPC_NOT_ENOUGH_ARGS, mCallContext);
        return false;
    }
    
    if (!mArgv[0].isObject()) {
        ThrowBadParam(NS_ERROR_XPC_BAD_CONVERT_JS, 0, mCallContext);
        return false;
    }

    const nsID* iid = xpc_JSObjectToID(mCallContext, &mArgv[0].toObject());
    if (!iid) {
        ThrowBadParam(NS_ERROR_XPC_BAD_CONVERT_JS, 0, mCallContext);
        return false;
    }

    nsresult invokeResult;
    nsISupports* qiresult = nullptr;
    invokeResult = mCallee->QueryInterface(*iid, (void**) &qiresult);

    mCallContext.GetXPCContext()->SetLastResult(invokeResult);

    if (NS_FAILED(invokeResult)) {
        ThrowBadResult(invokeResult, mCallContext);
        return false;
    }

    jsval v = JSVAL_NULL;
    nsresult err;
    JSBool success =
        XPCConvert::NativeData2JS(mCallContext, &v, &qiresult,
                                  nsXPTType::T_INTERFACE_IS,
                                  iid, &err);
    NS_IF_RELEASE(qiresult);

    if (!success) {
        ThrowBadParam(err, 0, mCallContext);
        return false;
    }

    mCallContext.SetRetVal(v);
    return true;
}

JSBool
CallMethodHelper::InitializeDispatchParams()
{
    const uint8_t wantsOptArgc = mMethodInfo->WantsOptArgc() ? 1 : 0;
    const uint8_t wantsJSContext = mMethodInfo->WantsContext() ? 1 : 0;
    const uint8_t paramCount = mMethodInfo->GetParamCount();
    uint8_t requiredArgs = paramCount;
    uint8_t hasRetval = 0;

    
    if (paramCount && mMethodInfo->GetParam(paramCount-1).IsRetval()) {
        hasRetval = 1;
        requiredArgs--;
    }

    if (mArgc < requiredArgs || wantsOptArgc) {
        if (wantsOptArgc)
            mOptArgcIndex = requiredArgs;

        
        while (requiredArgs && mMethodInfo->GetParam(requiredArgs-1).IsOptional())
            requiredArgs--;

        if (mArgc < requiredArgs) {
            Throw(NS_ERROR_XPC_NOT_ENOUGH_ARGS, mCallContext);
            return false;
        }
    }

    if (wantsJSContext) {
        if (wantsOptArgc)
            
            mJSContextIndex = mOptArgcIndex++;
        else if (mMethodInfo->IsSetter() || mMethodInfo->IsGetter())
            
            mJSContextIndex = 0;
        else
            mJSContextIndex = paramCount - hasRetval;
    }

    
    for (uint8_t i = 0; i < paramCount + wantsJSContext + wantsOptArgc; i++) {
        nsXPTCVariant* dp = mDispatchParams.AppendElement();
        dp->ClearFlags();
        dp->val.p = nullptr;
    }

    
    if (wantsJSContext) {
        nsXPTCVariant* dp = &mDispatchParams[mJSContextIndex];
        dp->type = nsXPTType::T_VOID;
        dp->val.p = mCallContext;
    }

    
    if (wantsOptArgc) {
        nsXPTCVariant* dp = &mDispatchParams[mOptArgcIndex];
        dp->type = nsXPTType::T_U8;
        dp->val.u8 = std::min<uint32_t>(mArgc, paramCount) - requiredArgs;
    }

    return true;
}

JSBool
CallMethodHelper::ConvertIndependentParams(JSBool* foundDependentParam)
{
    const uint8_t paramCount = mMethodInfo->GetParamCount();
    for (uint8_t i = 0; i < paramCount; i++) {
        const nsXPTParamInfo& paramInfo = mMethodInfo->GetParam(i);

        if (paramInfo.GetType().IsDependent())
            *foundDependentParam = true;
        else if (!ConvertIndependentParam(i))
            return false;

    }

    return true;
}

JSBool
CallMethodHelper::ConvertIndependentParam(uint8_t i)
{
    const nsXPTParamInfo& paramInfo = mMethodInfo->GetParam(i);
    const nsXPTType& type = paramInfo.GetType();
    uint8_t type_tag = type.TagPart();
    nsXPTCVariant* dp = GetDispatchParam(i);
    dp->type = type;
    NS_ABORT_IF_FALSE(!paramInfo.IsShared(), "[shared] implies [noscript]!");

    
    if (paramInfo.IsDipper())
        return HandleDipperParam(dp, paramInfo);

    
    if (paramInfo.IsIndirect())
        dp->SetIndirect();

    
    
    if (type_tag == nsXPTType::T_JSVAL) {
        
        dp->val.j = JSVAL_VOID;
        if (!JS_AddValueRoot(mCallContext, &dp->val.j))
            return false;
    }

    
    if (!type.IsArithmetic())
        dp->SetValNeedsCleanup();

    
    
    
    
    
    jsval src;
    if (!GetOutParamSource(i, &src))
        return false;

    
    
    if (!paramInfo.IsIn())
        return true;

    
    
    
    
    if (!paramInfo.IsOut()) {
        
        NS_ASSERTION(i < mArgc || paramInfo.IsOptional(),
                     "Expected either enough arguments or an optional argument");
        if (i < mArgc)
            src = mArgv[i];
        else if (type_tag == nsXPTType::T_JSVAL)
            src = JSVAL_VOID;
        else
            src = JSVAL_NULL;
    }

    nsID param_iid;
    if (type_tag == nsXPTType::T_INTERFACE &&
        NS_FAILED(mIFaceInfo->GetIIDForParamNoAlloc(mVTableIndex, &paramInfo,
                                                    &param_iid))) {
        ThrowBadParam(NS_ERROR_XPC_CANT_GET_PARAM_IFACE_INFO, i, mCallContext);
        return false;
    }

    nsresult err;
    if (!XPCConvert::JSData2Native(mCallContext, &dp->val, src, type,
                                   true, &param_iid, &err)) {
        ThrowBadParam(err, i, mCallContext);
        return false;
    }

    return true;
}

JSBool
CallMethodHelper::ConvertDependentParams()
{
    const uint8_t paramCount = mMethodInfo->GetParamCount();
    for (uint8_t i = 0; i < paramCount; i++) {
        const nsXPTParamInfo& paramInfo = mMethodInfo->GetParam(i);

        if (!paramInfo.GetType().IsDependent())
            continue;
        if (!ConvertDependentParam(i))
            return false;
    }

    return true;
}

JSBool
CallMethodHelper::ConvertDependentParam(uint8_t i)
{
    const nsXPTParamInfo& paramInfo = mMethodInfo->GetParam(i);
    const nsXPTType& type = paramInfo.GetType();
    nsXPTType datum_type;
    uint32_t array_count = 0;
    bool isArray = type.IsArray();

    bool isSizedString = isArray ?
        false :
        type.TagPart() == nsXPTType::T_PSTRING_SIZE_IS ||
        type.TagPart() == nsXPTType::T_PWSTRING_SIZE_IS;

    nsXPTCVariant* dp = GetDispatchParam(i);
    dp->type = type;

    if (isArray) {
        if (NS_FAILED(mIFaceInfo->GetTypeForParam(mVTableIndex, &paramInfo, 1,
                                                  &datum_type))) {
            Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, mCallContext);
            return false;
        }
        NS_ABORT_IF_FALSE(datum_type.TagPart() != nsXPTType::T_JSVAL,
                          "Arrays of JSVals not currently supported - "
                          "see bug 693337.");
    } else {
        datum_type = type;
    }

    
    if (paramInfo.IsIndirect())
        dp->SetIndirect();

    
    
    
    
    
    if (!datum_type.IsArithmetic())
        dp->SetValNeedsCleanup();

    
    
    
    
    
    jsval src;
    if (!GetOutParamSource(i, &src))
        return false;

    
    
    if (!paramInfo.IsIn())
        return true;

    
    
    
    
    if (!paramInfo.IsOut()) {
        
        NS_ASSERTION(i < mArgc || paramInfo.IsOptional(),
                     "Expected either enough arguments or an optional argument");
        src = i < mArgc ? mArgv[i] : JSVAL_NULL;
    }

    nsID param_iid;
    if (datum_type.IsInterfacePointer() &&
        !GetInterfaceTypeFromParam(i, datum_type, &param_iid))
        return false;

    nsresult err;

    if (isArray || isSizedString) {
        if (!GetArraySizeFromParam(i, &array_count))
            return false;

        if (isArray) {
            if (array_count &&
                !XPCConvert::JSArray2Native(mCallContext, (void**)&dp->val, src,
                                            array_count, datum_type, &param_iid,
                                            &err)) {
                
                ThrowBadParam(err, i, mCallContext);
                return false;
            }
        } else 
        {
            if (!XPCConvert::JSStringWithSize2Native(mCallContext,
                                                     (void*)&dp->val,
                                                     src, array_count,
                                                     datum_type, &err)) {
                ThrowBadParam(err, i, mCallContext);
                return false;
            }
        }
    } else {
        if (!XPCConvert::JSData2Native(mCallContext, &dp->val, src, type,
                                       true, &param_iid, &err)) {
            ThrowBadParam(err, i, mCallContext);
            return false;
        }
    }

    return true;
}





void
CallMethodHelper::CleanupParam(nsXPTCMiniVariant& param, nsXPTType& type)
{
    
    NS_ABORT_IF_FALSE(type.TagPart() != nsXPTType::T_ARRAY, "Can't handle arrays.");

    
    
    if (type.TagPart() != nsXPTType::T_JSVAL && param.val.p == nullptr)
        return;

    switch (type.TagPart()) {
        case nsXPTType::T_JSVAL:
            JS_RemoveValueRoot(mCallContext, (jsval*)&param.val);
            break;
        case nsXPTType::T_INTERFACE:
        case nsXPTType::T_INTERFACE_IS:
            ((nsISupports*)param.val.p)->Release();
            break;
        case nsXPTType::T_ASTRING:
        case nsXPTType::T_DOMSTRING:
            nsXPConnect::GetRuntimeInstance()->DeleteString((nsAString*)param.val.p);
            break;
        case nsXPTType::T_UTF8STRING:
        case nsXPTType::T_CSTRING:
            delete (nsCString*) param.val.p;
            break;
        default:
            NS_ABORT_IF_FALSE(!type.IsArithmetic(),
                              "Cleanup requested on unexpected type.");
            nsMemory::Free(param.val.p);
            break;
    }
}






















JSBool
CallMethodHelper::HandleDipperParam(nsXPTCVariant* dp,
                                    const nsXPTParamInfo& paramInfo)
{
    
    uint8_t type_tag = paramInfo.GetType().TagPart();

    
    NS_ABORT_IF_FALSE(!paramInfo.IsOut(), "Dipper has unexpected flags.");

    
    
    NS_ABORT_IF_FALSE(type_tag == nsXPTType::T_ASTRING ||
                      type_tag == nsXPTType::T_DOMSTRING ||
                      type_tag == nsXPTType::T_UTF8STRING ||
                      type_tag == nsXPTType::T_CSTRING,
                      "Unexpected dipper type!");

    
    
    if (type_tag == nsXPTType::T_ASTRING || type_tag == nsXPTType::T_DOMSTRING)
        dp->val.p = new nsAutoString();
    else
        dp->val.p = new nsCString();

    
    if (!dp->val.p) {
        JS_ReportOutOfMemory(mCallContext);
        return false;
    }

    
    dp->SetValNeedsCleanup();

    return true;
}

nsresult
CallMethodHelper::Invoke()
{
    uint32_t argc = mDispatchParams.Length();
    nsXPTCVariant* argv = mDispatchParams.Elements();

    return NS_InvokeByIndex(mCallee, mVTableIndex, argc, argv);
}





NS_IMETHODIMP XPCWrappedNative::GetJSObject(JSObject * *aJSObject)
{
    *aJSObject = GetFlatJSObject();
    return NS_OK;
}


NS_IMETHODIMP XPCWrappedNative::GetNative(nsISupports * *aNative)
{
    
    
    *aNative = mIdentity;
    NS_ADDREF(*aNative);
    return NS_OK;
}


NS_IMETHODIMP XPCWrappedNative::GetJSObjectPrototype(JSObject * *aJSObjectPrototype)
{
    *aJSObjectPrototype = HasProto() ?
                GetProto()->GetJSProtoObject() : GetFlatJSObject();
    return NS_OK;
}

nsIPrincipal*
XPCWrappedNative::GetObjectPrincipal() const
{
    nsIPrincipal* principal = GetScope()->GetPrincipal();
#ifdef DEBUG
    
    
    
    nsCOMPtr<nsIScriptObjectPrincipal> objPrin(do_QueryInterface(mIdentity));
    if (objPrin) {
        bool equal;
        if (!principal)
            equal = !objPrin->GetPrincipal();
        else
            principal->Equals(objPrin->GetPrincipal(), &equal);
        NS_ASSERTION(equal, "Principal mismatch.  Expect bad things to happen");
    }
#endif
    return principal;
}


NS_IMETHODIMP XPCWrappedNative::GetXPConnect(nsIXPConnect * *aXPConnect)
{
    if (IsValid()) {
        nsIXPConnect* temp = GetRuntime()->GetXPConnect();
        NS_IF_ADDREF(temp);
        *aXPConnect = temp;
    } else
        *aXPConnect = nullptr;
    return NS_OK;
}


NS_IMETHODIMP XPCWrappedNative::FindInterfaceWithMember(jsid name, nsIInterfaceInfo * *_retval)
{
    XPCNativeInterface* iface;
    XPCNativeMember*  member;

    if (GetSet()->FindMember(name, &member, &iface) && iface) {
        nsIInterfaceInfo* temp = iface->GetInterfaceInfo();
        NS_IF_ADDREF(temp);
        *_retval = temp;
    } else
        *_retval = nullptr;
    return NS_OK;
}


NS_IMETHODIMP XPCWrappedNative::FindInterfaceWithName(jsid name, nsIInterfaceInfo * *_retval)
{
    XPCNativeInterface* iface = GetSet()->FindNamedInterface(name);
    if (iface) {
        nsIInterfaceInfo* temp = iface->GetInterfaceInfo();
        NS_IF_ADDREF(temp);
        *_retval = temp;
    } else
        *_retval = nullptr;
    return NS_OK;
}


NS_IMETHODIMP_(bool)
XPCWrappedNative::HasNativeMember(jsid name)
{
    XPCNativeMember *member = nullptr;
    uint16_t ignored;
    return GetSet()->FindMember(name, &member, &ignored) && !!member;
}

inline nsresult UnexpectedFailure(nsresult rv)
{
    NS_ERROR("This is not supposed to fail!");
    return rv;
}


NS_IMETHODIMP XPCWrappedNative::FinishInitForWrappedGlobal()
{
    
    MOZ_ASSERT(mScriptableInfo);
    MOZ_ASSERT(mScriptableInfo->GetFlags().IsGlobalObject());
    MOZ_ASSERT(HasProto());

    
    XPCCallContext ccx(NATIVE_CALLER);
    if (!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    
    bool success = GetProto()->CallPostCreatePrototype(ccx);
    if (!success)
        return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP XPCWrappedNative::GetSecurityInfoAddress(void*** securityInfoAddrPtr)
{
    NS_ENSURE_ARG_POINTER(securityInfoAddrPtr);
    *securityInfoAddrPtr = GetSecurityInfoAddr();
    return NS_OK;
}


NS_IMETHODIMP XPCWrappedNative::DebugDump(int16_t depth)
{
#ifdef DEBUG
    depth-- ;
    XPC_LOG_ALWAYS(("XPCWrappedNative @ %x with mRefCnt = %d", this, mRefCnt.get()));
    XPC_LOG_INDENT();

        if (HasProto()) {
            XPCWrappedNativeProto* proto = GetProto();
            if (depth && proto)
                proto->DebugDump(depth);
            else
                XPC_LOG_ALWAYS(("mMaybeProto @ %x", proto));
        } else
            XPC_LOG_ALWAYS(("Scope @ %x", GetScope()));

        if (depth && mSet)
            mSet->DebugDump(depth);
        else
            XPC_LOG_ALWAYS(("mSet @ %x", mSet));

        XPC_LOG_ALWAYS(("mFlatJSObject of %x", mFlatJSObject));
        XPC_LOG_ALWAYS(("mIdentity of %x", mIdentity));
        XPC_LOG_ALWAYS(("mScriptableInfo @ %x", mScriptableInfo));

        if (depth && mScriptableInfo) {
            XPC_LOG_INDENT();
            XPC_LOG_ALWAYS(("mScriptable @ %x", mScriptableInfo->GetCallback()));
            XPC_LOG_ALWAYS(("mFlags of %x", (uint32_t)mScriptableInfo->GetFlags()));
            XPC_LOG_ALWAYS(("mJSClass @ %x", mScriptableInfo->GetJSClass()));
            XPC_LOG_OUTDENT();
        }
    XPC_LOG_OUTDENT();
#endif
    return NS_OK;
}



char*
XPCWrappedNative::ToString(XPCCallContext& ccx,
                           XPCWrappedNativeTearOff* to  ) const
{
#ifdef DEBUG
#  define FMT_ADDR " @ 0x%p"
#  define FMT_STR(str) str
#  define PARAM_ADDR(w) , w
#else
#  define FMT_ADDR ""
#  define FMT_STR(str)
#  define PARAM_ADDR(w)
#endif

    char* sz = nullptr;
    char* name = nullptr;

    XPCNativeScriptableInfo* si = GetScriptableInfo();
    if (si)
        name = JS_smprintf("%s", si->GetJSClass()->name);
    if (to) {
        const char* fmt = name ? " (%s)" : "%s";
        name = JS_sprintf_append(name, fmt,
                                 to->GetInterface()->GetNameString());
    } else if (!name) {
        XPCNativeSet* set = GetSet();
        XPCNativeInterface** array = set->GetInterfaceArray();
        uint16_t count = set->GetInterfaceCount();

        if (count == 1)
            name = JS_sprintf_append(name, "%s", array[0]->GetNameString());
        else if (count == 2 &&
                 array[0] == XPCNativeInterface::GetISupports(ccx)) {
            name = JS_sprintf_append(name, "%s", array[1]->GetNameString());
        } else {
            for (uint16_t i = 0; i < count; i++) {
                const char* fmt = (i == 0) ?
                                    "(%s" : (i == count-1) ?
                                        ", %s)" : ", %s";
                name = JS_sprintf_append(name, fmt,
                                         array[i]->GetNameString());
            }
        }
    }

    if (!name) {
        return nullptr;
    }
    const char* fmt = "[xpconnect wrapped %s" FMT_ADDR FMT_STR(" (native")
        FMT_ADDR FMT_STR(")") "]";
    if (si) {
        fmt = "[object %s" FMT_ADDR FMT_STR(" (native") FMT_ADDR FMT_STR(")") "]";
    }
    sz = JS_smprintf(fmt, name PARAM_ADDR(this) PARAM_ADDR(mIdentity));

    JS_smprintf_free(name);


    return sz;

#undef FMT_ADDR
#undef PARAM_ADDR
}



#ifdef XPC_CHECK_CLASSINFO_CLAIMS
static void DEBUG_CheckClassInfoClaims(XPCWrappedNative* wrapper)
{
    if (!wrapper || !wrapper->GetClassInfo())
        return;

    nsISupports* obj = wrapper->GetIdentityObject();
    XPCNativeSet* set = wrapper->GetSet();
    uint16_t count = set->GetInterfaceCount();
    for (uint16_t i = 0; i < count; i++) {
        nsIClassInfo* clsInfo = wrapper->GetClassInfo();
        XPCNativeInterface* iface = set->GetInterfaceAt(i);
        nsIInterfaceInfo* info = iface->GetInterfaceInfo();
        const nsIID* iid;
        nsISupports* ptr;

        info->GetIIDShared(&iid);
        nsresult rv = obj->QueryInterface(*iid, (void**)&ptr);
        if (NS_SUCCEEDED(rv)) {
            NS_RELEASE(ptr);
            continue;
        }
        if (rv == NS_ERROR_OUT_OF_MEMORY)
            continue;

        

        char* className = nullptr;
        char* contractID = nullptr;
        const char* interfaceName;

        info->GetNameShared(&interfaceName);
        clsInfo->GetContractID(&contractID);
        if (wrapper->GetScriptableInfo()) {
            wrapper->GetScriptableInfo()->GetCallback()->
                GetClassName(&className);
        }


        printf("\n!!! Object's nsIClassInfo lies about its interfaces!!!\n"
               "   classname: %s \n"
               "   contractid: %s \n"
               "   unimplemented interface name: %s\n\n",
               className ? className : "<unknown>",
               contractID ? contractID : "<unknown>",
               interfaceName);

#ifdef XPC_ASSERT_CLASSINFO_CLAIMS
        NS_ERROR("Fix this QueryInterface or nsIClassInfo");
#endif

        if (className)
            nsMemory::Free(className);
        if (contractID)
            nsMemory::Free(contractID);
    }
}
#endif

#ifdef XPC_REPORT_SHADOWED_WRAPPED_NATIVE_MEMBERS
static void DEBUG_PrintShadowObjectInfo(const char* header,
                                        XPCNativeSet* set,
                                        XPCWrappedNative* wrapper,
                                        XPCWrappedNativeProto* proto)

{
    if (header)
        printf("%s\n", header);

    printf("   XPCNativeSet @ 0x%p for the class:\n", (void*)set);

    char* className = nullptr;
    char* contractID = nullptr;

    nsIClassInfo* clsInfo = proto ? proto->GetClassInfo() : nullptr;
    if (clsInfo)
        clsInfo->GetContractID(&contractID);

    XPCNativeScriptableInfo* si = wrapper ?
            wrapper->GetScriptableInfo() :
            proto->GetScriptableInfo();
    if (si)
        si->GetCallback()->GetClassName(&className);

    printf("   classname: %s \n"
           "   contractid: %s \n",
           className ? className : "<unknown>",
           contractID ? contractID : "<unknown>");

    if (className)
        nsMemory::Free(className);
    if (contractID)
        nsMemory::Free(contractID);

    printf("   claims to implement interfaces:\n");

    uint16_t count = set->GetInterfaceCount();
    for (uint16_t i = 0; i < count; i++) {
        XPCNativeInterface* iface = set->GetInterfaceAt(i);
        nsIInterfaceInfo* info = iface->GetInterfaceInfo();
        const char* interfaceName;
        info->GetNameShared(&interfaceName);
        printf("      %s\n", interfaceName);
    }
}

static void ReportSingleMember(jsval ifaceName,
                               jsval memberName)
{
    JS_FileEscapedString(stdout, ifaceName, 0);
    if (JSVAL_IS_STRING(memberName)) {
        fputs("::", stdout);
        JS_FileEscapedString(stdout, memberName, 0);
    }
}

static void ShowHeader(JSBool* printedHeader,
                       const char* header,
                       XPCNativeSet* set,
                       XPCWrappedNative* wrapper,
                       XPCWrappedNativeProto* proto)
{
    if (!*printedHeader) {
        DEBUG_PrintShadowObjectInfo(header, set, wrapper, proto);
        *printedHeader = true;
    }

}

static void ShowOneShadow(jsval ifaceName1,
                          jsval memberName1,
                          jsval ifaceName2,
                          jsval memberName2)
{
    ReportSingleMember(ifaceName1, memberName1);
    printf(" shadows ");
    ReportSingleMember(ifaceName2, memberName2);
    printf("\n");
}

static void ShowDuplicateInterface(jsval ifaceName)
{
    fputs(" ! ", stdout);
    JS_FileEscapedString(stdout, ifaceName, 0);
    fputs(" appears twice in the nsIClassInfo interface set!\n", stdout);
}

static JSBool InterfacesAreRelated(XPCNativeInterface* iface1,
                                   XPCNativeInterface* iface2)
{
    nsIInterfaceInfo* info1 = iface1->GetInterfaceInfo();
    nsIInterfaceInfo* info2 = iface2->GetInterfaceInfo();

    NS_ASSERTION(info1 != info2, "should not have different iface!");

    bool match;

    return
        (NS_SUCCEEDED(info1->HasAncestor(iface2->GetIID(), &match)) && match) ||
        (NS_SUCCEEDED(info2->HasAncestor(iface1->GetIID(), &match)) && match);
}

static JSBool MembersAreTheSame(XPCNativeInterface* iface1,
                                uint16_t memberIndex1,
                                XPCNativeInterface* iface2,
                                uint16_t memberIndex2)
{
    nsIInterfaceInfo* info1 = iface1->GetInterfaceInfo();
    nsIInterfaceInfo* info2 = iface2->GetInterfaceInfo();

    XPCNativeMember* member1 = iface1->GetMemberAt(memberIndex1);
    XPCNativeMember* member2 = iface2->GetMemberAt(memberIndex2);

    uint16_t index1 = member1->GetIndex();
    uint16_t index2 = member2->GetIndex();

    

    if (member1->IsConstant()) {
        if (!member2->IsConstant())
            return false;

        const nsXPTConstant* constant1;
        const nsXPTConstant* constant2;

        return NS_SUCCEEDED(info1->GetConstant(index1, &constant1)) &&
               NS_SUCCEEDED(info2->GetConstant(index2, &constant2)) &&
               constant1->GetType() == constant2->GetType() &&
               constant1->GetValue() == constant2->GetValue();
    }

    
    

    if (member1->IsMethod() != member2->IsMethod() ||
        member1->IsWritableAttribute() != member2->IsWritableAttribute() ||
        member1->IsReadOnlyAttribute() != member2->IsReadOnlyAttribute()) {
        return false;
    }

    const nsXPTMethodInfo* mi1;
    const nsXPTMethodInfo* mi2;

    return NS_SUCCEEDED(info1->GetMethodInfo(index1, &mi1)) &&
           NS_SUCCEEDED(info2->GetMethodInfo(index2, &mi2)) &&
           mi1 == mi2;
}

void DEBUG_ReportShadowedMembers(XPCNativeSet* set,
                                 XPCWrappedNative* wrapper,
                                 XPCWrappedNativeProto* proto)
{
    

    if (!(proto || wrapper) || !set || set->GetInterfaceCount() < 2)
        return;

    NS_ASSERTION(proto || wrapper, "bad param!");
    XPCJSRuntime* rt = proto ? proto->GetRuntime() : wrapper->GetRuntime();

    
    static int nextSeenSet = 0;
    static const int MAX_SEEN_SETS = 128;
    static XPCNativeSet* SeenSets[MAX_SEEN_SETS];
    for (int seen = 0; seen < MAX_SEEN_SETS; seen++)
        if (set == SeenSets[seen])
            return;
    SeenSets[nextSeenSet] = set;

#ifdef off_DEBUG_jband
    static int seenCount = 0;
    printf("--- adding SeenSets[%d] = 0x%p\n", nextSeenSet, set);
    DEBUG_PrintShadowObjectInfo(nullptr, set, wrapper, proto);
#endif
    int localNext = nextSeenSet+1;
    nextSeenSet = localNext < MAX_SEEN_SETS ? localNext : 0;

    XPCNativeScriptableInfo* si = wrapper ?
            wrapper->GetScriptableInfo() :
            proto->GetScriptableInfo();

    
    if (si) {
        
        static const char* skipClasses[] = {
            "Window",
            "HTMLDocument",
            "HTMLCollection",
            "Event",
            "ChromeWindow",
            nullptr
        };

        static bool warned = false;
        if (!warned) {
            printf("!!! XPConnect won't warn about Shadowed Members of...\n  ");
            for (const char** name = skipClasses; *name; name++)
                printf("%s %s", name == skipClasses ? "" : ",", *name);
             printf("\n");
            warned = true;
        }

        bool quit = false;
        char* className = nullptr;
        si->GetCallback()->GetClassName(&className);
        if (className) {
            for (const char** name = skipClasses; *name; name++) {
                if (!strcmp(*name, className)) {
                    quit = true;
                    break;
                }
            }
            nsMemory::Free(className);
        }
        if (quit)
            return;
    }

    const char header[] =
        "!!!Object wrapped by XPConnect has members whose names shadow each other!!!";

    JSBool printedHeader = false;

    jsval QIName = rt->GetStringJSVal(XPCJSRuntime::IDX_QUERY_INTERFACE);

    uint16_t ifaceCount = set->GetInterfaceCount();
    uint16_t i, j, k, m;

    

    for (i = 0; i < ifaceCount; i++) {
        XPCNativeInterface* ifaceOuter = set->GetInterfaceAt(i);
        for (k = i+1; k < ifaceCount; k++) {
            XPCNativeInterface* ifaceInner = set->GetInterfaceAt(k);
            if (ifaceInner == ifaceOuter) {
                ShowHeader(&printedHeader, header, set, wrapper, proto);
                ShowDuplicateInterface(ifaceOuter->GetName());
            }
        }
    }

    

    for (i = 0; i < ifaceCount; i++) {
        XPCNativeInterface* ifaceOuter = set->GetInterfaceAt(i);
        jsval ifaceOuterName = ifaceOuter->GetName();

        uint16_t memberCountOuter = ifaceOuter->GetMemberCount();
        for (j = 0; j < memberCountOuter; j++) {
            XPCNativeMember* memberOuter = ifaceOuter->GetMemberAt(j);
            jsval memberOuterName = memberOuter->GetName();

            if (memberOuterName == QIName)
                continue;

            for (k = i+1; k < ifaceCount; k++) {
                XPCNativeInterface* ifaceInner = set->GetInterfaceAt(k);
                jsval ifaceInnerName = ifaceInner->GetName();

                
                if (ifaceInner == ifaceOuter)
                    continue;

                
                
                if (InterfacesAreRelated(ifaceInner, ifaceOuter))
                    continue;

                if (ifaceInnerName == memberOuterName) {
                    ShowHeader(&printedHeader, header, set, wrapper, proto);
                    ShowOneShadow(ifaceInnerName, JSVAL_NULL,
                                  ifaceOuterName, memberOuterName);
                }

                uint16_t memberCountInner = ifaceInner->GetMemberCount();

                for (m = 0; m < memberCountInner; m++) {
                    XPCNativeMember* memberInner = ifaceInner->GetMemberAt(m);
                    jsval memberInnerName = memberInner->GetName();

                    if (memberInnerName == QIName)
                        continue;

                    if (memberOuterName == memberInnerName &&
                        !MembersAreTheSame(ifaceOuter, j, ifaceInner, m))

                    {
                        ShowHeader(&printedHeader, header, set, wrapper, proto);
                        ShowOneShadow(ifaceOuterName, memberOuterName,
                                      ifaceInnerName, memberInnerName);
                    }
                }
            }
        }
    }
}
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(XPCJSObjectHolder, nsIXPConnectJSObjectHolder)

NS_IMETHODIMP
XPCJSObjectHolder::GetJSObject(JSObject** aJSObj)
{
    NS_PRECONDITION(aJSObj, "bad param");
    NS_PRECONDITION(mJSObj, "bad object state");
    *aJSObj = mJSObj;
    return NS_OK;
}

XPCJSObjectHolder::XPCJSObjectHolder(XPCCallContext& ccx, JSObject* obj)
    : mJSObj(obj)
{
    ccx.GetRuntime()->AddObjectHolderRoot(this);
}

XPCJSObjectHolder::~XPCJSObjectHolder()
{
    RemoveFromRootSet(nsXPConnect::GetRuntimeInstance()->GetMapLock());
}

void
XPCJSObjectHolder::TraceJS(JSTracer *trc)
{
    JS_SET_TRACING_DETAILS(trc, GetTraceName, this, 0);
    JS_CallObjectTracer(trc, mJSObj, "XPCJSObjectHolder::mJSObj");
}


void
XPCJSObjectHolder::GetTraceName(JSTracer* trc, char *buf, size_t bufsize)
{
    JS_snprintf(buf, bufsize, "XPCJSObjectHolder[0x%p].mJSObj",
                trc->debugPrintArg);
}


XPCJSObjectHolder*
XPCJSObjectHolder::newHolder(XPCCallContext& ccx, JSObject* obj)
{
    if (!obj) {
        NS_ERROR("bad param");
        return nullptr;
    }
    return new XPCJSObjectHolder(ccx, obj);
}

JSBool
MorphSlimWrapper(JSContext *cx, JSObject *obj)
{
    SLIM_LOG(("***** morphing from MorphSlimToWrapper (%p, %p)\n",
              obj, static_cast<nsISupports*>(xpc_GetJSPrivate(obj))));

    XPCCallContext ccx(JS_CALLER, cx);

    nsISupports* object = static_cast<nsISupports*>(xpc_GetJSPrivate(obj));
    nsWrapperCache *cache = nullptr;
    CallQueryInterface(object, &cache);
    nsRefPtr<XPCWrappedNative> wn;
    nsresult rv = XPCWrappedNative::Morph(ccx, obj, nullptr, cache,
                                          getter_AddRefs(wn));
    return NS_SUCCEEDED(rv);
}

#ifdef DEBUG_slimwrappers
static uint32_t sSlimWrappers;
#endif

JSBool
ConstructSlimWrapper(XPCCallContext &ccx,
                     xpcObjectHelper &aHelper,
                     XPCWrappedNativeScope* xpcScope, jsval *rval)
{
    nsISupports *identityObj = aHelper.GetCanonical();
    nsXPCClassInfo *classInfoHelper = aHelper.GetXPCClassInfo();

    if (!classInfoHelper) {
        SLIM_LOG_NOT_CREATED(ccx, identityObj, "No classinfo helper");
        return false;
    }

    XPCNativeScriptableFlags flags(classInfoHelper->GetScriptableFlags());

    NS_ASSERTION(flags.DontAskInstanceForScriptable(),
                 "Not supported for cached wrappers!");

    JSObject* parent = xpcScope->GetGlobalJSObject();
    if (!flags.WantPreCreate()) {
        SLIM_LOG_NOT_CREATED(ccx, identityObj,
                             "scriptable helper has no PreCreate hook");

        return false;
    }

    
    js::AutoMaybeTouchDeadZones agc(parent);

    JSObject* plannedParent = parent;
    nsresult rv = classInfoHelper->PreCreate(identityObj, ccx, parent, &parent);
    if (rv != NS_SUCCESS_ALLOW_SLIM_WRAPPERS) {
        SLIM_LOG_NOT_CREATED(ccx, identityObj, "PreCreate hook refused");

        return false;
    }

    if (!js::IsObjectInContextCompartment(parent, ccx.GetJSContext())) {
        SLIM_LOG_NOT_CREATED(ccx, identityObj, "wrong compartment");

        return false;
    }

    JSAutoCompartment ac(ccx, parent);

    if (parent != plannedParent) {
        XPCWrappedNativeScope *newXpcScope = GetObjectScope(parent);
        if (newXpcScope != xpcScope) {
            SLIM_LOG_NOT_CREATED(ccx, identityObj, "crossing origins");

            return false;
        }
    }

    
    
    nsWrapperCache *cache = aHelper.GetWrapperCache();
    JSObject* wrapper = cache->GetWrapper();
    if (wrapper) {
        *rval = OBJECT_TO_JSVAL(wrapper);

        return true;
    }

    uint32_t interfacesBitmap = classInfoHelper->GetInterfacesBitmap();
    XPCNativeScriptableCreateInfo
        sciProto(aHelper.forgetXPCClassInfo(), flags, interfacesBitmap);

    AutoMarkingWrappedNativeProtoPtr xpcproto(ccx);
    xpcproto = XPCWrappedNativeProto::GetNewOrUsed(ccx, xpcScope,
                                                   classInfoHelper, &sciProto);
    if (!xpcproto)
        return false;

    xpcproto->CacheOffsets(identityObj);

    XPCNativeScriptableInfo* si = xpcproto->GetScriptableInfo();
    JSClass* jsclazz = si->GetSlimJSClass();
    if (!jsclazz)
        return false;

    wrapper = JS_NewObject(ccx, jsclazz, xpcproto->GetJSProtoObject(), parent);
    if (!wrapper)
        return false;

    JS_SetPrivate(wrapper, identityObj);
    SetSlimWrapperProto(wrapper, xpcproto.get());

    
    aHelper.forgetCanonical();

    cache->SetWrapper(wrapper);

    SLIM_LOG(("+++++ %i created slim wrapper (%p, %p, %p)\n", ++sSlimWrappers,
              wrapper, p, xpcScope));

    *rval = OBJECT_TO_JSVAL(wrapper);

    return true;
}
