








#include "xpcprivate.h"
#include "nsWrapperCacheInlines.h"
#include "XPCLog.h"
#include "jsprf.h"
#include "AccessCheck.h"
#include "WrapperFactory.h"
#include "XrayWrapper.h"

#include "nsContentUtils.h"
#include "nsCxPusher.h"

#include <stdint.h>
#include "mozilla/Likely.h"
#include "mozilla/dom/BindingUtils.h"
#include <algorithm>

using namespace xpc;
using namespace mozilla;
using namespace mozilla::dom;
using namespace JS;



NS_IMPL_CYCLE_COLLECTION_CLASS(XPCWrappedNative)





NS_IMETHODIMP_(void)
NS_CYCLE_COLLECTION_CLASSNAME(XPCWrappedNative)::Unlink(void *p)
{
    XPCWrappedNative *tmp = static_cast<XPCWrappedNative*>(p);
    tmp->ExpireWrapper();
}

NS_IMETHODIMP
NS_CYCLE_COLLECTION_CLASSNAME(XPCWrappedNative)::Traverse
   (void *p, nsCycleCollectionTraversalCallback &cb)
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


static nsresult
FinishCreate(XPCWrappedNativeScope* Scope,
             XPCNativeInterface* Interface,
             nsWrapperCache *cache,
             XPCWrappedNative* inWrapper,
             XPCWrappedNative** resultWrapper);











nsresult
XPCWrappedNative::WrapNewGlobal(xpcObjectHelper &nativeHelper,
                                nsIPrincipal *principal,
                                bool initStandardClasses,
                                bool fireOnNewGlobalHook,
                                JS::CompartmentOptions& aOptions,
                                XPCWrappedNative **wrappedGlobal)
{
    AutoJSContext cx;
    nsISupports *identity = nativeHelper.GetCanonical();

    
    MOZ_ASSERT(nativeHelper.GetScriptableFlags() & nsIXPCScriptable::IS_GLOBAL_OBJECT);

    
    MOZ_ASSERT(!nativeHelper.GetWrapperCache() ||
               !nativeHelper.GetWrapperCache()->GetWrapperPreserveColor());

    
    XPCNativeScriptableCreateInfo sciProto;
    XPCNativeScriptableCreateInfo sciMaybe;
    const XPCNativeScriptableCreateInfo& sciWrapper =
        GatherScriptableCreateInfo(identity, nativeHelper.GetClassInfo(),
                                   sciProto, sciMaybe);

    
    
    AutoMarkingNativeScriptableInfoPtr si(cx, XPCNativeScriptableInfo::Construct(&sciWrapper));
    MOZ_ASSERT(si.get());

    
    const JSClass *clasp = si->GetJSClass();
    MOZ_ASSERT(clasp->flags & JSCLASS_IS_GLOBAL);

    
    RootedObject global(cx, xpc::CreateGlobalObject(cx, clasp, principal, aOptions));
    if (!global)
        return NS_ERROR_FAILURE;
    XPCWrappedNativeScope *scope = GetCompartmentPrivate(global)->scope;

    
    
    JSAutoCompartment ac(cx, global);

    
    if (initStandardClasses && ! JS_InitStandardClasses(cx, global))
        return NS_ERROR_FAILURE;

    
    XPCWrappedNativeProto *proto =
        XPCWrappedNativeProto::GetNewOrUsed(scope,
                                            nativeHelper.GetClassInfo(), &sciProto,
                                             false);
    if (!proto)
        return NS_ERROR_FAILURE;

    
    MOZ_ASSERT(proto->GetJSProtoObject());
    bool success = JS_SplicePrototype(cx, global, proto->GetJSProtoObject());
    if (!success)
        return NS_ERROR_FAILURE;

    
    
    nsRefPtr<XPCWrappedNative> wrapper =
        new XPCWrappedNative(nativeHelper.forgetCanonical(), proto);

    
    
    
    

    
    
    
    
    
    
    
    XPCNativeScriptableInfo* siProto = proto->GetScriptableInfo();
    if (siProto && siProto->GetCallback() == sciWrapper.GetCallback()) {
        wrapper->mScriptableInfo = siProto;
        
        
        
        
        
        delete si;
        si = nullptr;
    } else {
        wrapper->mScriptableInfo = si;
    }

    
    wrapper->mFlatJSObject = global;
    wrapper->mFlatJSObject.setFlags(FLAT_JS_OBJECT_VALID);

    
    JS_SetPrivate(global, wrapper);

    
    
    
    
    AutoMarkingWrappedNativePtr wrapperMarker(cx, wrapper);

    
    
    
    
    success = wrapper->FinishInit();
    MOZ_ASSERT(success);

    
    
    
    
    
    XPCNativeInterface* iface = XPCNativeInterface::GetNewOrUsed(&NS_GET_IID(nsISupports));
    MOZ_ASSERT(iface);
    nsresult status;
    success = wrapper->FindTearOff(iface, false, &status);
    if (!success)
        return status;

    
    
    
    nsresult rv = FinishCreate(scope, iface, nativeHelper.GetWrapperCache(),
                               wrapper, wrappedGlobal);
    NS_ENSURE_SUCCESS(rv, rv);

    if (fireOnNewGlobalHook)
        JS_FireOnNewGlobalObject(cx, global);
    return NS_OK;
}


nsresult
XPCWrappedNative::GetNewOrUsed(xpcObjectHelper& helper,
                               XPCWrappedNativeScope* Scope,
                               XPCNativeInterface* Interface,
                               XPCWrappedNative** resultWrapper)
{
    MOZ_ASSERT(Interface);
    AutoJSContext cx;
    nsWrapperCache *cache = helper.GetWrapperCache();

    MOZ_ASSERT(!cache || !cache->GetWrapperPreserveColor(),
               "We assume the caller already checked if it could get the "
               "wrapper from the cache.");

    nsresult rv;

    MOZ_ASSERT(!Scope->GetRuntime()->GCIsRunning(),
               "XPCWrappedNative::GetNewOrUsed called during GC");

    nsISupports *identity = helper.GetCanonical();

    if (!identity) {
        NS_ERROR("This XPCOM object fails in QueryInterface to nsISupports!");
        return NS_ERROR_FAILURE;
    }

    nsRefPtr<XPCWrappedNative> wrapper;

    Native2WrappedNativeMap* map = Scope->GetWrappedNativeMap();
    
    
    
    wrapper = map->Find(identity);

    if (wrapper) {
        if (!wrapper->FindTearOff(Interface, false, &rv)) {
            MOZ_ASSERT(NS_FAILED(rv), "returning NS_OK on failure");
            return rv;
        }
        *resultWrapper = wrapper.forget().get();
        return NS_OK;
    }

    
    
    
    
    
    
    
    

    
    
    
    bool iidIsClassInfo = Interface->GetIID()->Equals(NS_GET_IID(nsIClassInfo));
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

    RootedObject parent(cx, Scope->GetGlobalJSObject());

    RootedValue newParentVal(cx, NullValue());

    mozilla::Maybe<JSAutoCompartment> ac;

    if (sciWrapper.GetFlags().WantPreCreate()) {
        
        js::AutoMaybeTouchDeadZones agc(parent);

        RootedObject plannedParent(cx, parent);
        nsresult rv = sciWrapper.GetCallback()->PreCreate(identity, cx,
                                                          parent, parent.address());
        if (NS_FAILED(rv))
            return rv;
        rv = NS_OK;

        MOZ_ASSERT(!xpc::WrapperFactory::IsXrayWrapper(parent),
                   "Xray wrapper being used to parent XPCWrappedNative?");

        ac.construct(static_cast<JSContext*>(cx), parent);

        if (parent != plannedParent) {
            XPCWrappedNativeScope* betterScope = GetObjectScope(parent);
            if (betterScope != Scope)
                return GetNewOrUsed(helper, betterScope, Interface, resultWrapper);

            newParentVal = OBJECT_TO_JSVAL(parent);
        }

        
        
        

        if (cache) {
            RootedObject cached(cx, cache->GetWrapper());
            if (cached)
                wrapper = XPCWrappedNative::Get(cached);
        } else {
            wrapper = map->Find(identity);
        }

        if (wrapper) {
            if (wrapper->FindTearOff(Interface, false, &rv)) {
                MOZ_ASSERT(NS_FAILED(rv), "returning NS_OK on failure");
                return rv;
            }
            *resultWrapper = wrapper.forget().get();
            return NS_OK;
        }
    } else {
        ac.construct(static_cast<JSContext*>(cx), parent);
    }

    AutoMarkingWrappedNativeProtoPtr proto(cx);

    
    

    
    

    if (info && !isClassInfo) {
        proto = XPCWrappedNativeProto::GetNewOrUsed(Scope, info, &sciProto);
        if (!proto)
            return NS_ERROR_FAILURE;

        wrapper = new XPCWrappedNative(helper.forgetCanonical(), proto);
    } else {
        AutoMarkingNativeInterfacePtr iface(cx, Interface);
        if (!iface)
            iface = XPCNativeInterface::GetISupports();

        AutoMarkingNativeSetPtr set(cx);
        set = XPCNativeSet::GetNewOrUsed(nullptr, iface, 0);

        if (!set)
            return NS_ERROR_FAILURE;

        wrapper =
            new XPCWrappedNative(helper.forgetCanonical(), Scope, set);
    }

    MOZ_ASSERT(!xpc::WrapperFactory::IsXrayWrapper(parent),
               "Xray wrapper being used to parent XPCWrappedNative?");

    
    
    
    
    AutoMarkingWrappedNativePtr wrapperMarker(cx, wrapper);

    if (!wrapper->Init(parent, &sciWrapper))
        return NS_ERROR_FAILURE;

    if (!wrapper->FindTearOff(Interface, false, &rv)) {
        MOZ_ASSERT(NS_FAILED(rv), "returning NS_OK on failure");
        return rv;
    }

    return FinishCreate(Scope, Interface, cache, wrapper, resultWrapper);
}

static nsresult
FinishCreate(XPCWrappedNativeScope* Scope,
             XPCNativeInterface* Interface,
             nsWrapperCache *cache,
             XPCWrappedNative* inWrapper,
             XPCWrappedNative** resultWrapper)
{
    AutoJSContext cx;
    MOZ_ASSERT(inWrapper);

    Native2WrappedNativeMap* map = Scope->GetWrappedNativeMap();

    nsRefPtr<XPCWrappedNative> wrapper;
    
    
    
    
    wrapper = map->Add(inWrapper);
    if (!wrapper)
        return NS_ERROR_FAILURE;

    if (wrapper == inWrapper) {
        JSObject *flat = wrapper->GetFlatJSObject();
        MOZ_ASSERT(!cache || !cache->GetWrapperPreserveColor() ||
                   flat == cache->GetWrapperPreserveColor(),
                   "This object has a cached wrapper that's different from "
                   "the JSObject held by its native wrapper?");

        if (cache && !cache->GetWrapperPreserveColor())
            cache->SetWrapper(flat);

        
        
        XPCNativeScriptableInfo* si = wrapper->GetScriptableInfo();
        if (si && si->GetFlags().WantPostCreate()) {
            nsresult rv = si->GetCallback()->PostCreate(wrapper, cx, flat);
            if (NS_FAILED(rv)) {
                
                
                
                
                
                
                
                
                NS_ERROR("PostCreate failed! This is known to cause "
                         "inconsistent state for some class types and may even "
                         "cause a crash in combination with a JS GC. Fix the "
                         "failing PostCreate ASAP!");

                map->Remove(wrapper);

                
                

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
XPCWrappedNative::GetUsedOnly(nsISupports* Object,
                              XPCWrappedNativeScope* Scope,
                              XPCNativeInterface* Interface,
                              XPCWrappedNative** resultWrapper)
{
    AutoJSContext cx;
    MOZ_ASSERT(Object, "XPCWrappedNative::GetUsedOnly was called with a null Object");
    MOZ_ASSERT(Interface);

    nsRefPtr<XPCWrappedNative> wrapper;
    nsWrapperCache* cache = nullptr;
    CallQueryInterface(Object, &cache);
    if (cache) {
        RootedObject flat(cx, cache->GetWrapper());
        if (!flat) {
            *resultWrapper = nullptr;
            return NS_OK;
        }
        wrapper = XPCWrappedNative::Get(flat);
    } else {
        nsCOMPtr<nsISupports> identity = do_QueryInterface(Object);

        if (!identity) {
            NS_ERROR("This XPCOM object fails in QueryInterface to nsISupports!");
            return NS_ERROR_FAILURE;
        }

        Native2WrappedNativeMap* map = Scope->GetWrappedNativeMap();

        wrapper = map->Find(identity);
        if (!wrapper) {
            *resultWrapper = nullptr;
            return NS_OK;
        }
    }

    nsresult rv;
    if (!wrapper->FindTearOff(Interface, false, &rv)) {
        MOZ_ASSERT(NS_FAILED(rv), "returning NS_OK on failure");
        return rv;
    }

    wrapper.forget(resultWrapper);
    return NS_OK;
}


XPCWrappedNative::XPCWrappedNative(already_AddRefed<nsISupports> aIdentity,
                                   XPCWrappedNativeProto* aProto)
    : mMaybeProto(aProto),
      mSet(aProto->GetSet()),
      mScriptableInfo(nullptr)
{
    MOZ_ASSERT(NS_IsMainThread());

    mIdentity = aIdentity.get();
    mFlatJSObject.setFlags(FLAT_JS_OBJECT_VALID);

    MOZ_ASSERT(mMaybeProto, "bad ctor param");
    MOZ_ASSERT(mSet, "bad ctor param");
}


XPCWrappedNative::XPCWrappedNative(already_AddRefed<nsISupports> aIdentity,
                                   XPCWrappedNativeScope* aScope,
                                   XPCNativeSet* aSet)

    : mMaybeScope(TagScope(aScope)),
      mSet(aSet),
      mScriptableInfo(nullptr)
{
    MOZ_ASSERT(NS_IsMainThread());

    mIdentity = aIdentity.get();
    mFlatJSObject.setFlags(FLAT_JS_OBJECT_VALID);

    MOZ_ASSERT(aScope, "bad ctor param");
    MOZ_ASSERT(aSet, "bad ctor param");
}

XPCWrappedNative::~XPCWrappedNative()
{
    Destroy();
}

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

        
        
        map->Remove(this);
    }

    if (mIdentity) {
        XPCJSRuntime* rt = GetRuntime();
        if (rt && rt->GetDoingFinalization()) {
            nsContentUtils::DeferredFinalize(mIdentity);
            mIdentity = nullptr;
        } else {
            NS_RELEASE(mIdentity);
        }
    }

    mMaybeScope = nullptr;
}

void
XPCWrappedNative::UpdateScriptableInfo(XPCNativeScriptableInfo *si)
{
    MOZ_ASSERT(mScriptableInfo, "UpdateScriptableInfo expects an existing scriptable info");

    
    JSRuntime* rt = GetRuntime()->Runtime();
    if (IsIncrementalBarrierNeeded(rt))
        mScriptableInfo->Mark();

    mScriptableInfo = si;
}

void
XPCWrappedNative::SetProto(XPCWrappedNativeProto* p)
{
    MOZ_ASSERT(!IsWrapperExpired(), "bad ptr!");

    MOZ_ASSERT(HasProto());

    
    JSRuntime* rt = GetRuntime()->Runtime();
    GetProto()->WriteBarrierPre(rt);

    mMaybeProto = p;
}



void
XPCWrappedNative::GatherProtoScriptableCreateInfo(nsIClassInfo* classInfo,
                                                  XPCNativeScriptableCreateInfo& sciProto)
{
    MOZ_ASSERT(classInfo, "bad param");
    MOZ_ASSERT(!sciProto.GetCallback(), "bad param");

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
    MOZ_ASSERT(!sciWrapper.GetCallback(), "bad param");

    
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

        
        

        MOZ_ASSERT(!(sciWrapper.GetFlags().WantPreCreate() &&
                     !sciProto.GetFlags().WantPreCreate()),
                   "Can't set WANT_PRECREATE on an instance scriptable "
                   "without also setting it on the class scriptable");

        MOZ_ASSERT(!(sciWrapper.GetFlags().DontEnumStaticProps() &&
                     !sciProto.GetFlags().DontEnumStaticProps() &&
                     sciProto.GetCallback()),
                   "Can't set DONT_ENUM_STATIC_PROPS on an instance scriptable "
                   "without also setting it on the class scriptable (if present and shared)");

        MOZ_ASSERT(!(sciWrapper.GetFlags().DontEnumQueryInterface() &&
                     !sciProto.GetFlags().DontEnumQueryInterface() &&
                     sciProto.GetCallback()),
                   "Can't set DONT_ENUM_QUERY_INTERFACE on an instance scriptable "
                   "without also setting it on the class scriptable (if present and shared)");

        MOZ_ASSERT(!(sciWrapper.GetFlags().DontAskInstanceForScriptable() &&
                     !sciProto.GetFlags().DontAskInstanceForScriptable()),
                   "Can't set DONT_ASK_INSTANCE_FOR_SCRIPTABLE on an instance scriptable "
                   "without also setting it on the class scriptable");

        MOZ_ASSERT(!(sciWrapper.GetFlags().ClassInfoInterfacesOnly() &&
                     !sciProto.GetFlags().ClassInfoInterfacesOnly() &&
                     sciProto.GetCallback()),
                   "Can't set CLASSINFO_INTERFACES_ONLY on an instance scriptable "
                   "without also setting it on the class scriptable (if present and shared)");

        MOZ_ASSERT(!(sciWrapper.GetFlags().AllowPropModsDuringResolve() &&
                     !sciProto.GetFlags().AllowPropModsDuringResolve() &&
                     sciProto.GetCallback()),
                   "Can't set ALLOW_PROP_MODS_DURING_RESOLVE on an instance scriptable "
                   "without also setting it on the class scriptable (if present and shared)");

        MOZ_ASSERT(!(sciWrapper.GetFlags().AllowPropModsToPrototype() &&
                     !sciProto.GetFlags().AllowPropModsToPrototype() &&
                     sciProto.GetCallback()),
                   "Can't set ALLOW_PROP_MODS_TO_PROTOTYPE on an instance scriptable "
                   "without also setting it on the class scriptable (if present and shared)");

        return sciWrapper;
    }

    return sciProto;
}

bool
XPCWrappedNative::Init(HandleObject parent,
                       const XPCNativeScriptableCreateInfo* sci)
{
    AutoJSContext cx;
    

    if (sci->GetCallback()) {
        if (HasProto()) {
            XPCNativeScriptableInfo* siProto = GetProto()->GetScriptableInfo();
            if (siProto && siProto->GetCallback() == sci->GetCallback())
                mScriptableInfo = siProto;
        }
        if (!mScriptableInfo) {
            mScriptableInfo =
                XPCNativeScriptableInfo::Construct(sci);

            if (!mScriptableInfo)
                return false;
        }
    }
    XPCNativeScriptableInfo* si = mScriptableInfo;

    

    const JSClass* jsclazz = si ? si->GetJSClass() : Jsvalify(&XPC_WN_NoHelper_JSClass.base);

    
    MOZ_ASSERT_IF(si, !!si->GetFlags().IsGlobalObject() == !!(jsclazz->flags & JSCLASS_IS_GLOBAL));

    MOZ_ASSERT(jsclazz &&
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

    RootedObject protoJSObject(cx, HasProto() ?
                                   GetProto()->GetJSProtoObject() :
                                   JS_GetObjectPrototype(cx, parent));
    if (!protoJSObject) {
        return false;
    }

    mFlatJSObject = JS_NewObject(cx, jsclazz, protoJSObject, parent);
    if (!mFlatJSObject) {
        mFlatJSObject.unsetFlags(FLAT_JS_OBJECT_VALID);
        return false;
    }

    mFlatJSObject.setFlags(FLAT_JS_OBJECT_VALID);
    JS_SetPrivate(mFlatJSObject, this);

    return FinishInit();
}

bool
XPCWrappedNative::FinishInit()
{
    AutoJSContext cx;

    
    
    JS_SetReservedSlot(mFlatJSObject, WN_XRAYEXPANDOCHAIN_SLOT, JSVAL_NULL);

    
    
    
    MOZ_ASSERT(1 == mRefCnt, "unexpected refcount value");
    NS_ADDREF(this);

    if (mScriptableInfo && mScriptableInfo->GetFlags().WantCreate() &&
        NS_FAILED(mScriptableInfo->GetCallback()->Create(this, cx,
                                                         mFlatJSObject))) {
        return false;
    }

    
    JS_updateMallocCounter(cx, 2 * sizeof(XPCWrappedNative));

    return true;
}


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(XPCWrappedNative)
  NS_INTERFACE_MAP_ENTRY(nsIXPConnectWrappedNative)
  NS_INTERFACE_MAP_ENTRY(nsIXPConnectJSObjectHolder)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPConnectWrappedNative)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(XPCWrappedNative)




NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_LAST_RELEASE(XPCWrappedNative, Destroy())






















































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
                MOZ_ASSERT(JS_IsAboutToBeFinalizedUnbarriered(&jso));
                JS_SetPrivate(jso, nullptr);
                to->JSObjectFinalized();
            }

            
            nsISupports* obj = to->GetNative();
            if (obj) {
#ifdef XP_WIN
                
                MOZ_ASSERT(*(int*)obj != 0xdddddddd, "bad pointer!");
                MOZ_ASSERT(*(int*)obj != 0,          "bad pointer!");
#endif
                XPCJSRuntime* rt = GetRuntime();
                if (rt) {
                    nsContentUtils::DeferredFinalize(obj);
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
    mFlatJSObject.unsetFlags(FLAT_JS_OBJECT_VALID);

    MOZ_ASSERT(mIdentity, "bad pointer!");
#ifdef XP_WIN
    
    MOZ_ASSERT(*(int*)mIdentity != 0xdddddddd, "bad pointer!");
    MOZ_ASSERT(*(int*)mIdentity != 0,          "bad pointer!");
#endif

    if (IsWrapperExpired()) {
        Destroy();
    }

    
    

    Release();
}

void
XPCWrappedNative::SystemIsBeingShutDown()
{
    if (!IsValid())
        return;

    
    
    

    

    
    JS_SetPrivate(mFlatJSObject, nullptr);
    mFlatJSObject = nullptr;
    mFlatJSObject.unsetFlags(FLAT_JS_OBJECT_VALID);

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




class MOZ_STACK_CLASS AutoClonePrivateGuard {
public:
    AutoClonePrivateGuard(JSContext *cx, JSObject *aOld, JSObject *aNew)
        : mOldReflector(cx, aOld), mNewReflector(cx, aNew)
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
    RootedObject mOldReflector;
    RootedObject mNewReflector;
};


nsresult
XPCWrappedNative::ReparentWrapperIfFound(XPCWrappedNativeScope* aOldScope,
                                         XPCWrappedNativeScope* aNewScope,
                                         HandleObject aNewParent,
                                         nsISupports* aCOMObj)
{
    AutoJSContext cx;
    XPCNativeInterface* iface = XPCNativeInterface::GetISupports();

    if (!iface)
        return NS_ERROR_FAILURE;

    nsresult rv;

    nsRefPtr<XPCWrappedNative> wrapper;
    RootedObject flat(cx);
    nsWrapperCache* cache = nullptr;
    CallQueryInterface(aCOMObj, &cache);
    if (cache) {
        flat = cache->GetWrapper();
        if (flat) {
            wrapper = XPCWrappedNative::Get(flat);
            MOZ_ASSERT(wrapper->GetScope() == aOldScope,
                         "Incorrect scope passed");
        }
    } else {
        rv = XPCWrappedNative::GetUsedOnly(aCOMObj, aOldScope, iface,
                                           getter_AddRefs(wrapper));
        if (NS_FAILED(rv))
            return rv;

        if (wrapper)
            flat = wrapper->GetFlatJSObject();
    }

    if (!flat)
        return NS_OK;

    JSAutoCompartment ac(cx, aNewScope->GetGlobalJSObject());

    if (aOldScope != aNewScope) {
        
        AutoMarkingWrappedNativeProtoPtr oldProto(cx);
        AutoMarkingWrappedNativeProtoPtr newProto(cx);

        
        MOZ_ASSERT(js::GetObjectCompartment(aOldScope->GetGlobalJSObject()) !=
                   js::GetObjectCompartment(aNewScope->GetGlobalJSObject()));
        MOZ_ASSERT(aNewParent, "won't be able to find the new parent");

        if (wrapper->HasProto()) {
            oldProto = wrapper->GetProto();
            XPCNativeScriptableInfo *info = oldProto->GetScriptableInfo();
            XPCNativeScriptableCreateInfo ci(*info);
            newProto =
                XPCWrappedNativeProto::GetNewOrUsed(aNewScope,
                                                    oldProto->GetClassInfo(),
                                                    &ci);
            if (!newProto) {
                return NS_ERROR_FAILURE;
            }
        }

        
        
        
        
        

        RootedObject newobj(cx, JS_CloneObject(cx, flat,
                                               newProto->GetJSProtoObject(),
                                               aNewParent));
        if (!newobj)
            return NS_ERROR_FAILURE;

        
        
        
        
        
        
        
        RootedObject propertyHolder(cx);
        {
            AutoClonePrivateGuard cloneGuard(cx, flat, newobj);

            propertyHolder = JS_NewObjectWithGivenProto(cx, nullptr, JS::NullPtr(),
                                                        aNewParent);
            if (!propertyHolder)
                return NS_ERROR_OUT_OF_MEMORY;
            if (!JS_CopyPropertiesFrom(cx, propertyHolder, flat))
                return NS_ERROR_FAILURE;

            
            
            SetWNExpandoChain(newobj, nullptr);
            if (!XrayUtils::CloneExpandoChain(cx, newobj, flat))
                return NS_ERROR_FAILURE;

            
            
            
            
            
            
            JS_SetPrivate(flat, nullptr);
        }

        
        
        Native2WrappedNativeMap* oldMap = aOldScope->GetWrappedNativeMap();
        Native2WrappedNativeMap* newMap = aNewScope->GetWrappedNativeMap();

        oldMap->Remove(wrapper);

        if (wrapper->HasProto())
            wrapper->SetProto(newProto);

        
        
        

        if (wrapper->mScriptableInfo &&
            wrapper->mScriptableInfo == oldProto->GetScriptableInfo()) {
            
            
            
            

            MOZ_ASSERT(oldProto->GetScriptableInfo()->GetScriptableShared() ==
                       newProto->GetScriptableInfo()->GetScriptableShared(),
                       "Changing proto is also changing JSObject Classname or "
                       "helper's nsIXPScriptable flags. This is not allowed!");

            wrapper->UpdateScriptableInfo(newProto->GetScriptableInfo());
        }

        
        if (newMap->Find(wrapper->GetIdentityObject()))
            MOZ_CRASH();

        if (!newMap->Add(wrapper))
            MOZ_CRASH();

        flat = xpc::TransplantObject(cx, flat, newobj);
        if (!flat)
            MOZ_CRASH();

        MOZ_ASSERT(flat);
        wrapper->mFlatJSObject = flat;
        wrapper->mFlatJSObject.setFlags(FLAT_JS_OBJECT_VALID);

        if (cache) {
            bool preserving = cache->PreservingWrapper();
            cache->SetPreservingWrapper(false);
            cache->SetWrapper(flat);
            cache->SetPreservingWrapper(preserving);
        }
        if (!JS_CopyPropertiesFrom(cx, flat, propertyHolder))
            MOZ_CRASH();

        
        XPCNativeScriptableInfo* si = wrapper->GetScriptableInfo();
        if (si->GetFlags().WantPostCreate())
            (void) si->GetCallback()->PostTransplant(wrapper, cx, flat);
    }

    

    if (aNewParent) {
        if (!JS_SetParent(cx, flat, aNewParent))
            MOZ_CRASH();
    }

    return NS_OK;
}

















static nsresult
RescueOrphans(HandleObject obj)
{
    AutoJSContext cx;
    
    
    
    

    
    
    
    
    
    
    nsresult rv;
    RootedObject parentObj(cx, js::GetObjectParent(obj));
    if (!parentObj)
        return NS_OK; 
    parentObj = js::UncheckedUnwrap(parentObj,  false);

    
    js::AutoMaybeTouchDeadZones agc(parentObj);

    
    rv = RescueOrphans(parentObj);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (!js::IsCrossCompartmentWrapper(parentObj))
        return NS_OK;

    
    if (IS_WN_REFLECTOR(obj)) {
        RootedObject realParent(cx, js::UncheckedUnwrap(parentObj));
        XPCWrappedNative *wn =
            static_cast<XPCWrappedNative*>(js::GetObjectPrivate(obj));
        return wn->ReparentWrapperIfFound(GetObjectScope(parentObj),
                                          GetObjectScope(realParent),
                                          realParent, wn->GetIdentityObject());
    }

    return ReparentWrapper(cx, obj);
}





nsresult
XPCWrappedNative::RescueOrphans()
{
    AutoJSContext cx;
    RootedObject flatJSObject(cx, mFlatJSObject);
    return ::RescueOrphans(flatJSObject);
}

bool
XPCWrappedNative::ExtendSet(XPCNativeInterface* aInterface)
{
    AutoJSContext cx;

    if (!mSet->HasInterface(aInterface)) {
        AutoMarkingNativeSetPtr newSet(cx);
        newSet = XPCNativeSet::GetNewOrUsed(mSet, aInterface,
                                            mSet->GetInterfaceCount());
        if (!newSet)
            return false;

        mSet = newSet;
    }
    return true;
}

XPCWrappedNativeTearOff*
XPCWrappedNative::LocateTearOff(XPCNativeInterface* aInterface)
{
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
XPCWrappedNative::FindTearOff(XPCNativeInterface* aInterface,
                              bool needJSObject ,
                              nsresult* pError )
{
    AutoJSContext cx;
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
                    AutoMarkingWrappedNativeTearOffPtr tearoff(cx, to);
                    bool ok = InitTearOffJSObject(to);
                    
                    
                    
                    
                    to->Unmark();
                    if (!ok) {
                        to = nullptr;
                        rv = NS_ERROR_OUT_OF_MEMORY;
                    }
                }
                if (pError)
                    *pError = rv;
                return to;
            }
            if (!firstAvailable && to->IsAvailable())
                firstAvailable = to;
        }
    }

    to = firstAvailable;

    if (!to) {
        auto newChunk = new XPCWrappedNativeTearOffChunk();
        lastChunk->mNextChunk = newChunk;
        to = newChunk->mTearOffs;
    }

    {
        
        AutoMarkingWrappedNativeTearOffPtr tearoff(cx, to);
        rv = InitTearOff(to, aInterface, needJSObject);
        
        
        
        to->Unmark();
        if (NS_FAILED(rv))
            to = nullptr;
    }

    if (pError)
        *pError = rv;
    return to;
}

nsresult
XPCWrappedNative::InitTearOff(XPCWrappedNativeTearOff* aTearOff,
                              XPCNativeInterface* aInterface,
                              bool needJSObject)
{
    AutoJSContext cx;

    

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
        RootedObject jso(cx, wrappedJS->GetJSObject());
        if (jso == mFlatJSObject) {
            
            
            
            
            
            
            
            
            
            
            

            NS_RELEASE(obj);
            aTearOff->SetInterface(nullptr);
            return NS_OK;
        }

        
        
        
        
        
        
        
        
        
        

        if (iid->Equals(NS_GET_IID(nsIPropertyBag)) && jso) {
            nsRefPtr<nsXPCWrappedJSClass> clasp = nsXPCWrappedJSClass::GetNewOrUsed(cx, *iid);
            if (clasp) {
                RootedObject answer(cx, clasp->CallQueryInterfaceOnJSObject(cx, jso, *iid));

                if (!answer) {
                    NS_RELEASE(obj);
                    aTearOff->SetInterface(nullptr);
                    return NS_ERROR_NO_INTERFACE;
                }
            }
        }
    }

    nsIXPCSecurityManager* sm = nsXPConnect::XPConnect()->GetDefaultSecurityManager();
    if (sm && NS_FAILED(sm->
                        CanCreateWrapper(cx, *iid, identity,
                                         GetClassInfo()))) {
        
        NS_RELEASE(obj);
        aTearOff->SetInterface(nullptr);
        return NS_ERROR_XPC_SECURITY_MANAGER_VETO;
    }

    
    
    
    

    if (!mSet->HasInterface(aInterface) && !ExtendSet(aInterface)) {
        NS_RELEASE(obj);
        aTearOff->SetInterface(nullptr);
        return NS_ERROR_NO_INTERFACE;
    }

    aTearOff->SetInterface(aInterface);
    aTearOff->SetNative(obj);
    if (needJSObject && !InitTearOffJSObject(aTearOff))
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

bool
XPCWrappedNative::InitTearOffJSObject(XPCWrappedNativeTearOff* to)
{
    AutoJSContext cx;

    RootedObject parent(cx, mFlatJSObject);
    RootedObject proto(cx, JS_GetObjectPrototype(cx, parent));
    JSObject* obj = JS_NewObject(cx, Jsvalify(&XPC_WN_Tearoff_JSClass),
                                 proto, parent);
    if (!obj)
        return false;

    JS_SetPrivate(obj, to);
    to->SetJSObject(obj);
    return true;
}



static bool Throw(nsresult errNum, XPCCallContext& ccx)
{
    XPCThrower::Throw(errNum, ccx);
    return false;
}



class MOZ_STACK_CLASS CallMethodHelper
{
    XPCCallContext& mCallContext;
    
    
    
    nsresult mInvokeResult;
    nsIInterfaceInfo* const mIFaceInfo;
    const nsXPTMethodInfo* mMethodInfo;
    nsISupports* const mCallee;
    const uint16_t mVTableIndex;
    HandleId mIdxValueId;

    nsAutoTArray<nsXPTCVariant, 8> mDispatchParams;
    uint8_t mJSContextIndex; 
    uint8_t mOptArgcIndex; 

    jsval* const mArgv;
    const uint32_t mArgc;

    MOZ_ALWAYS_INLINE bool
    GetArraySizeFromParam(uint8_t paramIndex, uint32_t* result) const;

    MOZ_ALWAYS_INLINE bool
    GetInterfaceTypeFromParam(uint8_t paramIndex,
                              const nsXPTType& datum_type,
                              nsID* result) const;

    MOZ_ALWAYS_INLINE bool
    GetOutParamSource(uint8_t paramIndex, MutableHandleValue srcp) const;

    MOZ_ALWAYS_INLINE bool
    GatherAndConvertResults();

    MOZ_ALWAYS_INLINE bool
    QueryInterfaceFastPath();

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

    MOZ_ALWAYS_INLINE bool InitializeDispatchParams();

    MOZ_ALWAYS_INLINE bool ConvertIndependentParams(bool* foundDependentParam);
    MOZ_ALWAYS_INLINE bool ConvertIndependentParam(uint8_t i);
    MOZ_ALWAYS_INLINE bool ConvertDependentParams();
    MOZ_ALWAYS_INLINE bool ConvertDependentParam(uint8_t i);

    MOZ_ALWAYS_INLINE void CleanupParam(nsXPTCMiniVariant& param, nsXPTType& type);

    MOZ_ALWAYS_INLINE bool HandleDipperParam(nsXPTCVariant* dp,
                                             const nsXPTParamInfo& paramInfo);

    MOZ_ALWAYS_INLINE nsresult Invoke();

public:

    CallMethodHelper(XPCCallContext& ccx)
        : mCallContext(ccx)
        , mInvokeResult(NS_ERROR_UNEXPECTED)
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

    MOZ_ALWAYS_INLINE bool Call();

};


bool
XPCWrappedNative::CallMethod(XPCCallContext& ccx,
                             CallMode mode )
{
    MOZ_ASSERT(ccx.GetXPCContext()->CallerTypeIsJavaScript(),
               "Native caller for XPCWrappedNative::CallMethod?");

    nsresult rv = ccx.CanCallNow();
    if (NS_FAILED(rv)) {
        return Throw(rv, ccx);
    }

    return CallMethodHelper(ccx).Call();
}

bool
CallMethodHelper::Call()
{
    mCallContext.SetRetVal(JSVAL_VOID);

    XPCJSRuntime::Get()->SetPendingException(nullptr);

    if (mVTableIndex == 0) {
        return QueryInterfaceFastPath();
    }

    if (!mMethodInfo) {
        Throw(NS_ERROR_XPC_CANT_GET_METHOD_INFO, mCallContext);
        return false;
    }

    if (!InitializeDispatchParams())
        return false;

    
    
    
    
    bool foundDependentParam = false;
    if (!ConvertIndependentParams(&foundDependentParam))
        return false;

    if (foundDependentParam && !ConvertDependentParams())
        return false;

    mInvokeResult = Invoke();

    if (JS_IsExceptionPending(mCallContext)) {
        return false;
    }

    if (NS_FAILED(mInvokeResult)) {
        ThrowBadResult(mInvokeResult, mCallContext);
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

    mCallContext.GetXPCContext()->SetLastResult(mInvokeResult);
}

bool
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

bool
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

bool
CallMethodHelper::GetOutParamSource(uint8_t paramIndex, MutableHandleValue srcp) const
{
    const nsXPTParamInfo& paramInfo = mMethodInfo->GetParam(paramIndex);

    if ((paramInfo.IsOut() || paramInfo.IsDipper()) &&
        !paramInfo.IsRetval()) {
        MOZ_ASSERT(paramIndex < mArgc || paramInfo.IsOptional(),
                   "Expected either enough arguments or an optional argument");
        jsval arg = paramIndex < mArgc ? mArgv[paramIndex] : JSVAL_NULL;
        if (paramIndex < mArgc) {
            RootedObject obj(mCallContext);
            if (!arg.isPrimitive())
                obj = &arg.toObject();
            if (!obj || !JS_GetPropertyById(mCallContext, obj, mIdxValueId, srcp)) {
                
                
                
                ThrowBadParam(NS_ERROR_XPC_NEED_OUT_OBJECT, paramIndex,
                              mCallContext);
                return false;
            }
        }
    }

    return true;
}

bool
CallMethodHelper::GatherAndConvertResults()
{
    
    uint8_t paramCount = mMethodInfo->GetParamCount();
    for (uint8_t i = 0; i < paramCount; i++) {
        const nsXPTParamInfo& paramInfo = mMethodInfo->GetParam(i);
        if (!paramInfo.IsOut() && !paramInfo.IsDipper())
            continue;

        const nsXPTType& type = paramInfo.GetType();
        nsXPTCVariant* dp = GetDispatchParam(i);
        RootedValue v(mCallContext, NullValue());
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
            if (!XPCConvert::NativeArray2JS(&v, (const void**)&dp->val,
                                            datum_type, &param_iid,
                                            array_count, &err)) {
                
                ThrowBadParam(err, i, mCallContext);
                return false;
            }
        } else if (isSizedString) {
            if (!XPCConvert::NativeStringWithSize2JS(&v,
                                                     (const void*)&dp->val,
                                                     datum_type,
                                                     array_count, &err)) {
                ThrowBadParam(err, i, mCallContext);
                return false;
            }
        } else {
            if (!XPCConvert::NativeData2JS(&v, &dp->val, datum_type,
                                           &param_iid, &err)) {
                ThrowBadParam(err, i, mCallContext);
                return false;
            }
        }

        if (paramInfo.IsRetval()) {
            mCallContext.SetRetVal(v);
        } else if (i < mArgc) {
            
            MOZ_ASSERT(mArgv[i].isObject(), "out var is not object");
            RootedObject obj(mCallContext, &mArgv[i].toObject());
            if (!JS_SetPropertyById(mCallContext, obj, mIdxValueId, v)) {
                ThrowBadParam(NS_ERROR_XPC_CANT_SET_OUT_VAL, i, mCallContext);
                return false;
            }
        } else {
            MOZ_ASSERT(paramInfo.IsOptional(),
                       "Expected either enough arguments or an optional argument");
        }
    }

    return true;
}

bool
CallMethodHelper::QueryInterfaceFastPath()
{
    MOZ_ASSERT(mVTableIndex == 0,
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

    nsISupports* qiresult = nullptr;
    mInvokeResult = mCallee->QueryInterface(*iid, (void**) &qiresult);

    if (NS_FAILED(mInvokeResult)) {
        ThrowBadResult(mInvokeResult, mCallContext);
        return false;
    }

    RootedValue v(mCallContext, NullValue());
    nsresult err;
    bool success =
        XPCConvert::NativeData2JS(&v, &qiresult,
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

bool
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

bool
CallMethodHelper::ConvertIndependentParams(bool* foundDependentParam)
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

bool
CallMethodHelper::ConvertIndependentParam(uint8_t i)
{
    const nsXPTParamInfo& paramInfo = mMethodInfo->GetParam(i);
    const nsXPTType& type = paramInfo.GetType();
    uint8_t type_tag = type.TagPart();
    nsXPTCVariant* dp = GetDispatchParam(i);
    dp->type = type;
    MOZ_ASSERT(!paramInfo.IsShared(), "[shared] implies [noscript]!");

    
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

    
    
    
    
    
    RootedValue src(mCallContext);
    if (!GetOutParamSource(i, &src))
        return false;

    
    
    if (!paramInfo.IsIn())
        return true;

    
    
    
    
    if (!paramInfo.IsOut()) {
        
        MOZ_ASSERT(i < mArgc || paramInfo.IsOptional(),
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
    if (!XPCConvert::JSData2Native(&dp->val, src, type, true, &param_iid, &err)) {
        ThrowBadParam(err, i, mCallContext);
        return false;
    }

    return true;
}

bool
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

bool
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
        MOZ_ASSERT(datum_type.TagPart() != nsXPTType::T_JSVAL,
                   "Arrays of JSVals not currently supported - see bug 693337.");
    } else {
        datum_type = type;
    }

    
    if (paramInfo.IsIndirect())
        dp->SetIndirect();

    
    
    
    
    
    if (!datum_type.IsArithmetic())
        dp->SetValNeedsCleanup();

    
    
    
    
    
    RootedValue src(mCallContext);
    if (!GetOutParamSource(i, &src))
        return false;

    
    
    if (!paramInfo.IsIn())
        return true;

    
    
    
    
    if (!paramInfo.IsOut()) {
        
        MOZ_ASSERT(i < mArgc || paramInfo.IsOptional(),
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
                !XPCConvert::JSArray2Native((void**)&dp->val, src,
                                            array_count, datum_type, &param_iid,
                                            &err)) {
                
                ThrowBadParam(err, i, mCallContext);
                return false;
            }
        } else 
        {
            if (!XPCConvert::JSStringWithSize2Native((void*)&dp->val,
                                                     src, array_count,
                                                     datum_type, &err)) {
                ThrowBadParam(err, i, mCallContext);
                return false;
            }
        }
    } else {
        if (!XPCConvert::JSData2Native(&dp->val, src, type, true,
                                       &param_iid, &err)) {
            ThrowBadParam(err, i, mCallContext);
            return false;
        }
    }

    return true;
}





void
CallMethodHelper::CleanupParam(nsXPTCMiniVariant& param, nsXPTType& type)
{
    
    MOZ_ASSERT(type.TagPart() != nsXPTType::T_ARRAY, "Can't handle arrays.");

    
    
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
            MOZ_ASSERT(!type.IsArithmetic(), "Cleanup requested on unexpected type.");
            nsMemory::Free(param.val.p);
            break;
    }
}






















bool
CallMethodHelper::HandleDipperParam(nsXPTCVariant* dp,
                                    const nsXPTParamInfo& paramInfo)
{
    
    uint8_t type_tag = paramInfo.GetType().TagPart();

    
    MOZ_ASSERT(!paramInfo.IsOut(), "Dipper has unexpected flags.");

    
    
    MOZ_ASSERT(type_tag == nsXPTType::T_ASTRING ||
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





JSObject*
XPCWrappedNative::GetJSObject()
{
    return GetFlatJSObject();
}


NS_IMETHODIMP XPCWrappedNative::GetNative(nsISupports * *aNative)
{
    
    
    nsCOMPtr<nsISupports> rval = mIdentity;
    rval.forget(aNative);
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
        MOZ_ASSERT(equal, "Principal mismatch.  Expect bad things to happen");
    }
#endif
    return principal;
}


NS_IMETHODIMP XPCWrappedNative::FindInterfaceWithMember(HandleId name,
                                                        nsIInterfaceInfo * *_retval)
{
    XPCNativeInterface* iface;
    XPCNativeMember*  member;

    if (GetSet()->FindMember(name, &member, &iface) && iface) {
        nsCOMPtr<nsIInterfaceInfo> temp = iface->GetInterfaceInfo();
        temp.forget(_retval);
    } else
        *_retval = nullptr;
    return NS_OK;
}


NS_IMETHODIMP XPCWrappedNative::FindInterfaceWithName(HandleId name,
                                                      nsIInterfaceInfo * *_retval)
{
    XPCNativeInterface* iface = GetSet()->FindNamedInterface(name);
    if (iface) {
        nsCOMPtr<nsIInterfaceInfo> temp = iface->GetInterfaceInfo();
        temp.forget(_retval);
    } else
        *_retval = nullptr;
    return NS_OK;
}


NS_IMETHODIMP_(bool)
XPCWrappedNative::HasNativeMember(HandleId name)
{
    XPCNativeMember *member = nullptr;
    uint16_t ignored;
    return GetSet()->FindMember(name, &member, &ignored) && !!member;
}


NS_IMETHODIMP XPCWrappedNative::FinishInitForWrappedGlobal()
{
    
    MOZ_ASSERT(mScriptableInfo);
    MOZ_ASSERT(mScriptableInfo->GetFlags().IsGlobalObject());
    MOZ_ASSERT(HasProto());

    
    bool success = GetProto()->CallPostCreatePrototype();
    if (!success)
        return NS_ERROR_FAILURE;

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

        XPC_LOG_ALWAYS(("mFlatJSObject of %x", mFlatJSObject.getPtr()));
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
XPCWrappedNative::ToString(XPCWrappedNativeTearOff* to  ) const
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
                 array[0] == XPCNativeInterface::GetISupports()) {
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

        if (className)
            nsMemory::Free(className);
        if (contractID)
            nsMemory::Free(contractID);
    }
}
#endif

NS_IMPL_ISUPPORTS1(XPCJSObjectHolder, nsIXPConnectJSObjectHolder)

JSObject*
XPCJSObjectHolder::GetJSObject()
{
    NS_PRECONDITION(mJSObj, "bad object state");
    return mJSObj;
}

XPCJSObjectHolder::XPCJSObjectHolder(JSObject* obj)
    : mJSObj(obj)
{
    XPCJSRuntime::Get()->AddObjectHolderRoot(this);
}

XPCJSObjectHolder::~XPCJSObjectHolder()
{
    RemoveFromRootSet();
}

void
XPCJSObjectHolder::TraceJS(JSTracer *trc)
{
    JS_SET_TRACING_DETAILS(trc, GetTraceName, this, 0);
    JS_CallHeapObjectTracer(trc, &mJSObj, "XPCJSObjectHolder::mJSObj");
}


void
XPCJSObjectHolder::GetTraceName(JSTracer* trc, char *buf, size_t bufsize)
{
    JS_snprintf(buf, bufsize, "XPCJSObjectHolder[0x%p].mJSObj",
                trc->debugPrintArg);
}


XPCJSObjectHolder*
XPCJSObjectHolder::newHolder(JSObject* obj)
{
    if (!obj) {
        NS_ERROR("bad param");
        return nullptr;
    }
    return new XPCJSObjectHolder(obj);
}
