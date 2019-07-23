










































#include "xpcprivate.h"
#include "nsCRT.h"
#include "XPCNativeWrapper.h"
#include "XPCWrapper.h"
#include "nsWrapperCache.h"
#include "xpclog.h"



NS_IMPL_CYCLE_COLLECTION_CLASS(XPCWrappedNative)

NS_IMETHODIMP
NS_CYCLE_COLLECTION_CLASSNAME(XPCWrappedNative)::RootAndUnlinkJSObjects(void *p)
{
    XPCWrappedNative *tmp = static_cast<XPCWrappedNative*>(p);
    tmp->ExpireWrapper();
    return NS_OK;
}


NS_IMETHODIMP
NS_CYCLE_COLLECTION_CLASSNAME(XPCWrappedNative)::Traverse(void *p,
                                                          nsCycleCollectionTraversalCallback &cb)
{
    XPCWrappedNative *tmp = static_cast<XPCWrappedNative*>(p);
    if(!tmp->IsValid())
        return NS_OK;

    if (NS_UNLIKELY(cb.WantDebugInfo())) {
        char name[72];
        XPCNativeScriptableInfo* si = tmp->GetScriptableInfo();
        if(si)
            JS_snprintf(name, sizeof(name), "XPCWrappedNative (%s)",
                        si->GetJSClass()->name);
        else
            JS_snprintf(name, sizeof(name), "XPCWrappedNative");

        cb.DescribeNode(RefCounted, tmp->mRefCnt.get(),
                        sizeof(XPCWrappedNative), name);
    } else {
        cb.DescribeNode(RefCounted, tmp->mRefCnt.get(),
                        sizeof(XPCWrappedNative), "XPCWrappedNative");
    }

    if(tmp->mRefCnt.get() > 1) {

        
        
        
        
        
        
        
        
        

        JSObject *obj = nsnull;
        nsresult rv = tmp->GetJSObject(&obj);
        if(NS_SUCCEEDED(rv))
            cb.NoteScriptChild(nsIProgrammingLanguage::JAVASCRIPT, obj);
    }

    
    cb.NoteXPCOMChild(tmp->GetIdentityObject());

    tmp->NoteTearoffs(cb);

    return NS_OK;
}

void
XPCWrappedNative::NoteTearoffs(nsCycleCollectionTraversalCallback& cb)
{
    
    
    
    
    
    XPCWrappedNativeTearOffChunk* chunk;
    for(chunk = &mFirstChunk; chunk; chunk = chunk->mNextChunk)
    {
        XPCWrappedNativeTearOff* to = chunk->mTearOffs;
        for(int i = XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK-1; i >= 0; i--, to++)
        {
            JSObject* jso = to->GetJSObject();
            if(!jso)
            {
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
static PRBool  DEBUG_DumpedWrapperStats;
#endif

#ifdef DEBUG
static void DEBUG_TrackNewWrapper(XPCWrappedNative* wrapper)
{
#ifdef XPC_CHECK_WRAPPERS_AT_SHUTDOWN
    if(wrapper->GetRuntime())
        wrapper->GetRuntime()->DEBUG_AddWrappedNative(wrapper);
    else
        NS_ERROR("failed to add wrapper");
#endif
#ifdef XPC_TRACK_WRAPPER_STATS
    DEBUG_TotalWrappedNativeCount++;
    DEBUG_TotalLiveWrappedNativeCount++;
    if(DEBUG_TotalMaxWrappedNativeCount < DEBUG_TotalLiveWrappedNativeCount)
        DEBUG_TotalMaxWrappedNativeCount = DEBUG_TotalLiveWrappedNativeCount;

    if(wrapper->HasProto())
    {
        DEBUG_WrappedNativeWithProtoCount++;
        DEBUG_LiveWrappedNativeWithProtoCount++;
        if(DEBUG_MaxWrappedNativeWithProtoCount < DEBUG_LiveWrappedNativeWithProtoCount)
            DEBUG_MaxWrappedNativeWithProtoCount = DEBUG_LiveWrappedNativeWithProtoCount;
    }
    else
    {
        DEBUG_WrappedNativeNoProtoCount++;
        DEBUG_LiveWrappedNativeNoProtoCount++;
        if(DEBUG_MaxWrappedNativeNoProtoCount < DEBUG_LiveWrappedNativeNoProtoCount)
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
    if(wrapper->HasProto())
        DEBUG_LiveWrappedNativeWithProtoCount--;
    else
        DEBUG_LiveWrappedNativeNoProtoCount--;

    int extraChunkCount = wrapper->DEBUG_CountOfTearoffChunks() - 1;
    if(extraChunkCount > DEBUG_CHUNKS_TO_COUNT)
        extraChunkCount = DEBUG_CHUNKS_TO_COUNT;
    DEBUG_WrappedNativeTearOffChunkCounts[extraChunkCount]++;
#endif
}
static void DEBUG_TrackWrapperCall(XPCWrappedNative* wrapper,
                                   XPCWrappedNative::CallMode mode)
{
#ifdef XPC_TRACK_WRAPPER_STATS
    DEBUG_WrappedNativeTotalCalls++;
    switch(mode)
    {
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
    if(!DEBUG_DumpedWrapperStats)
    {
        DEBUG_DumpedWrapperStats = PR_TRUE;
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
        for(i = 0; i < DEBUG_CHUNKS_TO_COUNT; i++)
        {
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
             XPCWrappedNative* wrapper,
             XPCWrappedNative** resultWrapper);


nsresult
XPCWrappedNative::GetNewOrUsed(XPCCallContext& ccx,
                               nsISupports* Object,
                               XPCWrappedNativeScope* Scope,
                               XPCNativeInterface* Interface,
                               nsWrapperCache *cache,
                               JSBool isGlobal,
                               XPCWrappedNative** resultWrapper)
{
    NS_ASSERTION(!cache || !cache->GetWrapper(),
                 "We assume the caller already checked if it could get the "
                 "wrapper from the cache.");

    nsresult rv;

#ifdef DEBUG
    NS_ASSERTION(!Scope->GetRuntime()->GetThreadRunningGC(), 
                 "XPCWrappedNative::GetNewOrUsed called during GC");
    {
        nsWrapperCache *cache2 = nsnull;
        CallQueryInterface(Object, &cache2);
        NS_ASSERTION(!cache == !cache2, "Caller should pass in the cache!");
    }
#endif

    nsCOMPtr<nsISupports> identity;
#ifdef XPC_IDISPATCH_SUPPORT
    
    
    
    
    
    PRBool isIDispatch = Interface &&
                         Interface->GetIID()->Equals(NSID_IDISPATCH);
    if(isIDispatch)
        identity = Object;
    else
#endif
        identity = do_QueryInterface(Object);

    if(!identity)
    {
        NS_ERROR("This XPCOM object fails in QueryInterface to nsISupports!");
        return NS_ERROR_FAILURE;
    }

    XPCLock* mapLock = Scope->GetRuntime()->GetMapLock();
    
    
    
    
    
    AutoMarkingWrappedNativePtr wrapper(ccx);

    Native2WrappedNativeMap* map = Scope->GetWrappedNativeMap();
    if(!cache)
    {
        {   
            XPCAutoLock lock(mapLock);
            wrapper = map->Find(identity);
            if(wrapper)
                wrapper->AddRef();
        }

        if(wrapper)
        {
            if(Interface &&
               !wrapper->FindTearOff(ccx, Interface, JS_FALSE, &rv))
            {
                NS_RELEASE(wrapper);
                NS_ASSERTION(NS_FAILED(rv), "returning NS_OK on failure");
                return rv;
            }
            DEBUG_CheckWrapperThreadSafety(wrapper);
            *resultWrapper = wrapper;
            return NS_OK;
        }
    }
#ifdef DEBUG
    else if(!cache->GetWrapper())
    {   
        XPCAutoLock lock(mapLock);
        NS_ASSERTION(!map->Find(identity),
                     "There's a wrapper in the hashtable but it wasn't cached?");
    }
#endif

    
    
    
    
    
    
    
    

    
    
    
    JSBool isClassInfo = Interface &&
                         Interface->GetIID()->Equals(NS_GET_IID(nsIClassInfo));

    nsCOMPtr<nsIClassInfo> info;

    if(!isClassInfo)
        info = do_QueryInterface(identity);

#ifdef XPC_IDISPATCH_SUPPORT
    
    
    if(isIDispatch && !info)
    {
        info = dont_AddRef(static_cast<nsIClassInfo*>
                                      (XPCIDispatchClassInfo::GetSingleton()));
    }
#endif

    XPCNativeScriptableCreateInfo sciProto;
    XPCNativeScriptableCreateInfo sciWrapper;

    
    
    
    
    
    
    
    if(!isClassInfo &&
       NS_FAILED(GatherScriptableCreateInfo(identity, info.get(),
                                            &sciProto, &sciWrapper)))
        return NS_ERROR_FAILURE;

    JSObject* parent = Scope->GetGlobalJSObject();

    jsval newParentVal = JSVAL_NULL;
    XPCMarkableJSVal newParentVal_markable(&newParentVal);
    AutoMarkingJSVal newParentVal_automarker(ccx, &newParentVal_markable);
    JSBool chromeOnly = JS_FALSE;
    JSBool crossDoubleWrapped = JS_FALSE;

    if(sciWrapper.GetFlags().WantPreCreate())
    {
        JSObject* plannedParent = parent;
        nsresult rv = sciWrapper.GetCallback()->PreCreate(identity, ccx,
                                                          parent, &parent);
        if(NS_FAILED(rv))
            return rv;

        chromeOnly = (rv == NS_SUCCESS_CHROME_ACCESS_ONLY);
        rv = NS_OK;

        NS_ASSERTION(!XPCNativeWrapper::IsNativeWrapper(parent),
                     "Parent should never be an XPCNativeWrapper here");

        if(parent != plannedParent)
        {
            XPCWrappedNativeScope* betterScope =
                XPCWrappedNativeScope::FindInJSObjectScope(ccx, parent);
            if(betterScope != Scope)
                return GetNewOrUsed(ccx, identity, betterScope, Interface,
                                    cache, isGlobal, resultWrapper);

            newParentVal = OBJECT_TO_JSVAL(parent);
        }

        
        
        

        if(cache)
        {
            JSObject *cached = cache->GetWrapper();
            if(cached)
            {
                if(IS_SLIM_WRAPPER_OBJECT(cached))
                {
                    nsRefPtr<XPCWrappedNative> morphed;
                    if(!XPCWrappedNative::Morph(ccx, cached, Interface, cache,
                                                getter_AddRefs(morphed)))
                        return NS_ERROR_FAILURE;

                    wrapper = morphed.forget().get();
                } else {
                    wrapper =
                        static_cast<XPCWrappedNative*>(xpc_GetJSPrivate(cached));
                    if(wrapper)
                        wrapper->AddRef();
                }
            }
        }
        else
        {   
            XPCAutoLock lock(mapLock);
            wrapper = map->Find(identity);
            if(wrapper)
                wrapper->AddRef();
        }

        if(wrapper)
        {
            if(Interface && !wrapper->FindTearOff(ccx, Interface, JS_FALSE, &rv))
            {
                NS_RELEASE(wrapper);
                NS_ASSERTION(NS_FAILED(rv), "returning NS_OK on failure");
                return rv;
            }
            DEBUG_CheckWrapperThreadSafety(wrapper);
            *resultWrapper = wrapper;
            return NS_OK;
        }
    }
    else
    {
        if(nsXPCWrappedJSClass::IsWrappedJS(Object))
        {
            nsCOMPtr<nsIXPConnectWrappedJS> wrappedjs(do_QueryInterface(Object));
            JSObject *obj;
            wrappedjs->GetJSObject(&obj);
            if((obj->isSystem() ||
                JS_GetGlobalForObject(ccx, obj)->isSystem()) &&
               !Scope->GetGlobalJSObject()->isSystem())
            {
                crossDoubleWrapped = JS_TRUE;
            }
        }
    }

    AutoMarkingWrappedNativeProtoPtr proto(ccx);

    
    

    
    

    if(info && !isClassInfo)
    {
        proto = XPCWrappedNativeProto::GetNewOrUsed(ccx, Scope, info, &sciProto,
                                                    JS_FALSE, isGlobal);
        if(!proto)
            return NS_ERROR_FAILURE;

        proto->CacheOffsets(identity);

        wrapper = new XPCWrappedNative(identity.get(), proto);
        if(!wrapper)
            return NS_ERROR_FAILURE;
    }
    else
    {
        AutoMarkingNativeInterfacePtr iface(ccx, Interface);
        if(!iface)
            iface = XPCNativeInterface::GetISupports(ccx);

        AutoMarkingNativeSetPtr set(ccx);
        set = XPCNativeSet::GetNewOrUsed(ccx, nsnull, iface, 0);

        if(!set)
            return NS_ERROR_FAILURE;

        wrapper = new XPCWrappedNative(identity.get(), Scope, set);
        if(!wrapper)
            return NS_ERROR_FAILURE;

        DEBUG_ReportShadowedMembers(set, wrapper, nsnull);
    }

    
    
    
    identity.forget();

    NS_ADDREF(wrapper);

    NS_ASSERTION(!XPCNativeWrapper::IsNativeWrapper(parent),
                 "XPCNativeWrapper being used to parent XPCWrappedNative?");

    if(!wrapper->Init(ccx, parent, isGlobal, &sciWrapper))
    {
        NS_RELEASE(wrapper);
        return NS_ERROR_FAILURE;
    }

    if(Interface && !wrapper->FindTearOff(ccx, Interface, JS_FALSE, &rv))
    {
        
        wrapper->Release();
        NS_ASSERTION(NS_FAILED(rv), "returning NS_OK on failure");
        return rv;
    }

    if(chromeOnly)
        wrapper->SetNeedsChromeWrapper();
    if(crossDoubleWrapped)
        wrapper->SetIsDoubleWrapper();

    return FinishCreate(ccx, Scope, Interface, cache, wrapper, resultWrapper);
}

static nsresult
FinishCreate(XPCCallContext& ccx,
             XPCWrappedNativeScope* Scope,
             XPCNativeInterface* Interface,
             nsWrapperCache *cache,
             XPCWrappedNative* wrapper,
             XPCWrappedNative** resultWrapper)
{
#if DEBUG_xpc_leaks
    {
        char* s = wrapper->ToString(ccx);
        NS_ASSERTION(wrapper->GetFlatJSObject(), "eh?");
        printf("Created wrapped native %s, flat JSObject is %p\n",
               s, (void*)wrapper->GetFlatJSObject());
        if(s)
            JS_smprintf_free(s);
    }
#endif

    XPCLock* mapLock = Scope->GetRuntime()->GetMapLock();
    Native2WrappedNativeMap* map = Scope->GetWrappedNativeMap();

    
    XPCWrappedNative* wrapperToKill = nsnull;

    {   
        XPCAutoLock lock(mapLock);

        
        
        XPCWrappedNative* wrapper2 = map->Add(wrapper);
        if(!wrapper2)
        {
            NS_ERROR("failed to add our wrapper!");
            wrapperToKill = wrapper;
            wrapper = nsnull;
        }
        else if(wrapper2 != wrapper)
        {
            NS_ADDREF(wrapper2);
            wrapperToKill = wrapper;
            wrapper = wrapper2;
        }
    }

    if(wrapperToKill)
    {
        
        wrapperToKill->Release();
    }
    else if(wrapper)
    {
        JSObject *flat = wrapper->GetFlatJSObject();
        NS_ASSERTION(!cache || !cache->GetWrapper() ||
                     flat == cache->GetWrapper(),
                     "This object has a cached wrapper that's different from "
                     "the JSObject held by its native wrapper?");

        if(cache && !cache->GetWrapper())
            cache->SetWrapper(flat);

        
        
        XPCNativeScriptableInfo* si = wrapper->GetScriptableInfo();
        if(si && si->GetFlags().WantPostCreate())
        {
            nsresult rv = si->GetCallback()->
                     PostCreate(wrapper, ccx, wrapper->GetFlatJSObject());
            if(NS_FAILED(rv))
            {
                
                
                
                
                
                
                
                
                NS_ERROR("PostCreate failed! This is known to cause "
                         "inconsistent state for some class types and may even "
                         "cause a crash in combination with a JS GC. Fix the "
                         "failing PostCreate ASAP!");

                {   
                    XPCAutoLock lock(mapLock);
                    map->Remove(wrapper);
                }

                
                

                if(cache)
                    cache->ClearWrapper();
                wrapper->Release();
                return rv;
            }
        }
    }

    if(!wrapper)
        return NS_ERROR_FAILURE;

    DEBUG_CheckClassInfoClaims(wrapper);
    *resultWrapper = wrapper;
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

    
    
    
    
    AutoMarkingWrappedNativePtr wrapper(ccx);

#if DEBUG
    
    
#if 0
    if(proto->GetScriptableInfo()->GetFlags().WantPreCreate())
    {
        JSObject* parent = JS_GetParent(ccx, existingJSObject);
        JSObject* plannedParent = parent;
        nsresult rv =
            proto->GetScriptableInfo()->GetCallback()->PreCreate(identity, ccx,
                                                                 parent,
                                                                 &parent);
        if(NS_FAILED(rv))
            return rv;

        NS_ASSERTION(parent == plannedParent,
                     "PreCreate returned a different parent");
    }
#endif
#endif

    wrapper = new XPCWrappedNative(dont_AddRef(identity), proto);
    if(!wrapper)
        return NS_ERROR_FAILURE;

    NS_ADDREF(wrapper);

    NS_ASSERTION(!XPCNativeWrapper::IsNativeWrapper(STOBJ_GET_PARENT(
                                                             existingJSObject)),
                 "XPCNativeWrapper being used to parent XPCWrappedNative?");
    
    if(!wrapper->Init(ccx, existingJSObject))
    {
        NS_RELEASE(wrapper);
        return NS_ERROR_FAILURE;
    }

    nsresult rv;
    if(Interface && !wrapper->FindTearOff(ccx, Interface, JS_FALSE, &rv))
    {
        
        wrapper->Release();
        NS_ASSERTION(NS_FAILED(rv), "returning NS_OK on failure");
        return rv;
    }

    return FinishCreate(ccx, wrapper->GetScope(), Interface, cache, wrapper,
                        resultWrapper);
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
    nsWrapperCache* cache = nsnull;
    CallQueryInterface(Object, &cache);
    if(cache)
    {
        JSObject *flat = cache->GetWrapper();
        if(flat && IS_SLIM_WRAPPER_OBJECT(flat) && !MorphSlimWrapper(ccx, flat))
           return NS_ERROR_FAILURE;

        wrapper = flat ?
                  static_cast<XPCWrappedNative*>(xpc_GetJSPrivate(flat)) :
                  nsnull;

        if(!wrapper)
        {
            *resultWrapper = nsnull;
            return NS_OK;
        }
        NS_ADDREF(wrapper);
    }
    else
    {
        nsCOMPtr<nsISupports> identity;
#ifdef XPC_IDISPATCH_SUPPORT
        
        if(Interface->GetIID()->Equals(NSID_IDISPATCH))
            identity = Object;
        else
#endif
            identity = do_QueryInterface(Object);

        if(!identity)
        {
            NS_ERROR("This XPCOM object fails in QueryInterface to nsISupports!");
            return NS_ERROR_FAILURE;
        }

        Native2WrappedNativeMap* map = Scope->GetWrappedNativeMap();

        {   
            XPCAutoLock lock(Scope->GetRuntime()->GetMapLock());
            wrapper = map->Find(identity);
            if(!wrapper)
            {
                *resultWrapper = nsnull;
                return NS_OK;
            }
            NS_ADDREF(wrapper);
        }
    }

    nsresult rv;
    if(Interface && !wrapper->FindTearOff(ccx, Interface, JS_FALSE, &rv))
    {
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
      mFlatJSObject((JSObject*)JSVAL_ONE), 
      mScriptableInfo(nsnull),
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
      mFlatJSObject((JSObject*)JSVAL_ONE), 
      mScriptableInfo(nsnull),
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

    XPCWrappedNativeProto* proto = GetProto();

    if(mScriptableInfo &&
       (!HasProto() ||
        (proto && proto->GetScriptableInfo() != mScriptableInfo)))
    {
        delete mScriptableInfo;
    }

    XPCWrappedNativeScope *scope = GetScope();
    if(scope)
    {
        Native2WrappedNativeMap* map = scope->GetWrappedNativeMap();

        
        XPCAutoLock lock(GetRuntime()->GetMapLock());

        
        
        map->Remove(this);
    }

    if(mIdentity)
    {
        XPCJSRuntime* rt = GetRuntime();
        if(rt && rt->GetDoingFinalization())
        {
            if(!rt->DeferredRelease(mIdentity))
            {
                NS_WARNING("Failed to append object for deferred release.");
                
                NS_RELEASE(mIdentity);
            }
        }
        else
        {
            NS_RELEASE(mIdentity);
        }
    }
}



nsresult 
XPCWrappedNative::GatherProtoScriptableCreateInfo(
                        nsIClassInfo* classInfo,
                        XPCNativeScriptableCreateInfo* sciProto)
{
    NS_ASSERTION(classInfo, "bad param");
    NS_ASSERTION(sciProto && !sciProto->GetCallback(), "bad param");

    nsCOMPtr<nsISupports> possibleHelper;
    nsresult rv = classInfo->GetHelperForLanguage(
                                    nsIProgrammingLanguage::JAVASCRIPT,
                                    getter_AddRefs(possibleHelper));
    if(NS_SUCCEEDED(rv) && possibleHelper)
    {
        nsCOMPtr<nsIXPCScriptable> helper(do_QueryInterface(possibleHelper));
        if(helper)
        {
            JSUint32 flags;
            rv = helper->GetScriptableFlags(&flags);
            if(NS_FAILED(rv))
                flags = 0;

            sciProto->SetCallback(helper.forget());
            sciProto->SetFlags(flags);
        }
    }
    return NS_OK;
}


nsresult
XPCWrappedNative::GatherScriptableCreateInfo(
                        nsISupports* obj,
                        nsIClassInfo* classInfo,
                        XPCNativeScriptableCreateInfo* sciProto,
                        XPCNativeScriptableCreateInfo* sciWrapper)
{
    NS_ASSERTION(sciProto   && !sciProto->GetCallback(), "bad param");
    NS_ASSERTION(sciWrapper && !sciWrapper->GetCallback(), "bad param");

    
    if(classInfo)
    {
        GatherProtoScriptableCreateInfo(classInfo, sciProto);

        sciWrapper->SetCallback(sciProto->GetCallback());
        sciWrapper->SetFlags(sciProto->GetFlags());

        if(sciProto->GetFlags().DontAskInstanceForScriptable())
            return NS_OK;
    }

    
    nsCOMPtr<nsIXPCScriptable> helper(do_QueryInterface(obj));
    if(helper)
    {
        JSUint32 flags;
        nsresult rv = helper->GetScriptableFlags(&flags);
        if(NS_FAILED(rv))
            flags = 0;

        sciWrapper->SetCallback(helper.forget());
        sciWrapper->SetFlags(flags);

        
        

        NS_ASSERTION(!(sciWrapper->GetFlags().WantPreCreate() &&
                        !sciProto->GetFlags().WantPreCreate()),
                     "Can't set WANT_PRECREATE on an instance scriptable "
                     "without also setting it on the class scriptable");

        NS_ASSERTION(!(sciWrapper->GetFlags().DontEnumStaticProps() &&
                        !sciProto->GetFlags().DontEnumStaticProps() &&
                        sciProto->GetCallback() &&
                        !sciProto->GetFlags().DontSharePrototype()),
                     "Can't set DONT_ENUM_STATIC_PROPS on an instance scriptable "
                     "without also setting it on the class scriptable (if present and shared)");

        NS_ASSERTION(!(sciWrapper->GetFlags().DontEnumQueryInterface() &&
                        !sciProto->GetFlags().DontEnumQueryInterface() &&
                        sciProto->GetCallback() &&
                        !sciProto->GetFlags().DontSharePrototype()),
                     "Can't set DONT_ENUM_QUERY_INTERFACE on an instance scriptable "
                     "without also setting it on the class scriptable (if present and shared)");

        NS_ASSERTION(!(sciWrapper->GetFlags().DontAskInstanceForScriptable() &&
                        !sciProto->GetFlags().DontAskInstanceForScriptable()),
                     "Can't set DONT_ASK_INSTANCE_FOR_SCRIPTABLE on an instance scriptable "
                     "without also setting it on the class scriptable");

        NS_ASSERTION(!(sciWrapper->GetFlags().ClassInfoInterfacesOnly() &&
                        !sciProto->GetFlags().ClassInfoInterfacesOnly() &&
                        sciProto->GetCallback() &&
                        !sciProto->GetFlags().DontSharePrototype()),
                     "Can't set CLASSINFO_INTERFACES_ONLY on an instance scriptable "
                     "without also setting it on the class scriptable (if present and shared)");

        NS_ASSERTION(!(sciWrapper->GetFlags().AllowPropModsDuringResolve() &&
                        !sciProto->GetFlags().AllowPropModsDuringResolve() &&
                        sciProto->GetCallback() &&
                        !sciProto->GetFlags().DontSharePrototype()),
                     "Can't set ALLOW_PROP_MODS_DURING_RESOLVE on an instance scriptable "
                     "without also setting it on the class scriptable (if present and shared)");

        NS_ASSERTION(!(sciWrapper->GetFlags().AllowPropModsToPrototype() &&
                        !sciProto->GetFlags().AllowPropModsToPrototype() &&
                        sciProto->GetCallback() &&
                        !sciProto->GetFlags().DontSharePrototype()),
                     "Can't set ALLOW_PROP_MODS_TO_PROTOTYPE on an instance scriptable "
                     "without also setting it on the class scriptable (if present and shared)");

        NS_ASSERTION(!(sciWrapper->GetFlags().DontSharePrototype() &&
                        !sciProto->GetFlags().DontSharePrototype() &&
                        sciProto->GetCallback()),
                     "Can't set DONT_SHARE_PROTOTYPE on an instance scriptable "
                     "without also setting it on the class scriptable (if present and shared)");
    }

    return NS_OK;
}

void
XPCWrappedNative::TraceOtherWrapper(JSTracer* trc)
{
    
    
    
    JSObject *otherWrapper = GetScope()->GetWrapperMap()->Find(mFlatJSObject);
    if(otherWrapper)
    {
        JS_CALL_OBJECT_TRACER(trc, otherWrapper,
                              "XPCWrappedNative::mOtherWrapper");
    }
}

#ifdef DEBUG_slimwrappers
static PRUint32 sMorphedSlimWrappers;
#endif

JSBool
XPCWrappedNative::Init(XPCCallContext& ccx,
                       JSObject* parent, JSBool isGlobal,
                       const XPCNativeScriptableCreateInfo* sci)
{
    

    if(sci->GetCallback())
    {
        if(HasProto())
        {
            XPCNativeScriptableInfo* siProto = GetProto()->GetScriptableInfo();
            if(siProto && siProto->GetCallback() == sci->GetCallback())
                mScriptableInfo = siProto;
        }
        if(!mScriptableInfo)
        {
            mScriptableInfo =
                XPCNativeScriptableInfo::Construct(ccx, isGlobal, sci);

            if(!mScriptableInfo)
                return JS_FALSE;

            
            
            
            
            if(HasProto() && !HasSharedProto())
                GetProto()->SetScriptableInfo(mScriptableInfo);
        }
    }
    XPCNativeScriptableInfo* si = mScriptableInfo;

    

    JSClass* jsclazz = si ? si->GetJSClass() : &XPC_WN_NoHelper_JSClass.base;

    if(isGlobal)
    {
        
        
        
        if(!(jsclazz->flags & JSCLASS_IS_GLOBAL))
            jsclazz->flags |= JSCLASS_GLOBAL_FLAGS;
    }
    else
        NS_ASSERTION(!(jsclazz->flags & JSCLASS_IS_GLOBAL),
                     "Non-global object has the wrong flags");

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
        return JS_FALSE;
    }

    mFlatJSObject = xpc_NewSystemInheritingJSObject(ccx, jsclazz, protoJSObject,
                                                    parent);
    if(!mFlatJSObject)
        return JS_FALSE;

    return FinishInit(ccx);
}

JSBool
XPCWrappedNative::Init(XPCCallContext &ccx, JSObject *existingJSObject)
{
    
    if(!JS_SetReservedSlot(ccx, existingJSObject, 0, JSVAL_VOID))
        return JS_FALSE;

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
    
    
    
    
    if(!JS_SetPrivate(ccx, mFlatJSObject, this))
    {
        mFlatJSObject = nsnull;
        return JS_FALSE;
    }

    
    
    
    NS_ASSERTION(1 == mRefCnt, "unexpected refcount value");
    NS_ADDREF(this);

    if(mScriptableInfo && mScriptableInfo->GetFlags().WantCreate() &&
       NS_FAILED(mScriptableInfo->GetCallback()->Create(this, ccx,
                                                        mFlatJSObject)))
    {
        return JS_FALSE;
    }

#ifdef XPC_CHECK_WRAPPER_THREADSAFETY
    mThread = do_GetCurrentThread();

    if(HasProto() && GetProto()->ClassIsMainThreadOnly() && !NS_IsMainThread())
        DEBUG_ReportWrapperThreadSafetyError(ccx,
            "MainThread only wrapper created on the wrong thread", this);
#endif

    return JS_TRUE;
}


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(XPCWrappedNative)
  NS_INTERFACE_MAP_ENTRY(nsIXPConnectWrappedNative)
  NS_INTERFACE_MAP_ENTRY(nsIXPConnectJSObjectHolder)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXPConnectWrappedNative)
NS_INTERFACE_MAP_END_THREADSAFE

NS_IMPL_THREADSAFE_ADDREF(XPCWrappedNative)
NS_IMPL_THREADSAFE_RELEASE(XPCWrappedNative)





















































void
XPCWrappedNative::FlatJSObjectFinalized(JSContext *cx)
{
    if(!IsValid())
        return;

    
    
    
    

    XPCWrappedNativeTearOffChunk* chunk;
    for(chunk = &mFirstChunk; chunk; chunk = chunk->mNextChunk)
    {
        XPCWrappedNativeTearOff* to = chunk->mTearOffs;
        for(int i = XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK-1; i >= 0; i--, to++)
        {
            JSObject* jso = to->GetJSObject();
            if(jso)
            {
                NS_ASSERTION(JS_IsAboutToBeFinalized(cx, jso), "bad!");
                JS_SetPrivate(cx, jso, nsnull);
                to->JSObjectFinalized();
            }

            
            nsISupports* obj = to->GetNative();
            if(obj)
            {
#ifdef XP_WIN
                
                NS_ASSERTION(*(int*)obj != 0xdddddddd, "bad pointer!");
                NS_ASSERTION(*(int*)obj != 0,          "bad pointer!");
#endif
                XPCJSRuntime* rt = GetRuntime();
                if(rt)
                {
                    if(!rt->DeferredRelease(obj))
                    {
                        NS_WARNING("Failed to append object for deferred release.");
                        
                        obj->Release();
                    }
                }
                else
                {
                    obj->Release();
                }
                to->SetNative(nsnull);
            }

            to->SetInterface(nsnull);
        }
    }

    GetScope()->GetWrapperMap()->Remove(mFlatJSObject);

    if(IsWrapperExpired())
    {
        GetScope()->GetWrappedNativeMap()->Remove(this);

        XPCWrappedNativeProto* proto = GetProto();

        if(mScriptableInfo &&
           (!HasProto() ||
            (proto && proto->GetScriptableInfo() != mScriptableInfo)))
        {
            delete mScriptableInfo;
            mScriptableInfo = nsnull;
        }

        mMaybeScope = nsnull;
    }

    nsWrapperCache *cache = nsnull;
    CallQueryInterface(mIdentity, &cache);
    if(cache)
        cache->ClearWrapper();

    
    mFlatJSObject = nsnull;

    NS_ASSERTION(mIdentity, "bad pointer!");
#ifdef XP_WIN
    
    NS_ASSERTION(*(int*)mIdentity != 0xdddddddd, "bad pointer!");
    NS_ASSERTION(*(int*)mIdentity != 0,          "bad pointer!");
#endif

    
    

    Release();
}

void
XPCWrappedNative::SystemIsBeingShutDown(JSContext* cx)
{
#ifdef DEBUG_xpc_hacker
    {
        printf("Removing root for still-live XPCWrappedNative %p wrapping:\n",
               static_cast<void*>(this));
        for(PRUint16 i = 0, i_end = mSet->GetInterfaceCount(); i < i_end; ++i)
        {
            nsXPIDLCString name;
            mSet->GetInterfaceAt(i)->GetInterfaceInfo()
                ->GetName(getter_Copies(name));
            printf("  %s\n", name.get());
        }
    }
#endif
    DEBUG_TrackShutdownWrapper(this);

    if(!IsValid())
        return;

    
    
    

    

    
    JS_SetPrivate(cx, mFlatJSObject, nsnull);
    mFlatJSObject = nsnull; 

    XPCWrappedNativeProto* proto = GetProto();

    if(HasProto())
        proto->SystemIsBeingShutDown(cx);

    if(mScriptableInfo &&
       (!HasProto() ||
        (proto && proto->GetScriptableInfo() != mScriptableInfo)))
    {
        delete mScriptableInfo;
    }

    

    XPCWrappedNativeTearOffChunk* chunk;
    for(chunk = &mFirstChunk; chunk; chunk = chunk->mNextChunk)
    {
        XPCWrappedNativeTearOff* to = chunk->mTearOffs;
        for(int i = XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK-1; i >= 0; i--, to++)
        {
            if(to->GetJSObject())
            {
                JS_SetPrivate(cx, to->GetJSObject(), nsnull);
#ifdef XPC_IDISPATCH_SUPPORT
                if(to->IsIDispatch())
                    delete to->GetIDispatchInfo();
#endif
                to->SetJSObject(nsnull);
            }
            
            
            to->SetNative(nsnull);
            to->SetInterface(nsnull);
        }
    }

    if(mFirstChunk.mNextChunk)
    {
        delete mFirstChunk.mNextChunk;
        mFirstChunk.mNextChunk = nsnull;
    }
}




nsresult
XPCWrappedNative::ReparentWrapperIfFound(XPCCallContext& ccx,
                                         XPCWrappedNativeScope* aOldScope,
                                         XPCWrappedNativeScope* aNewScope,
                                         JSObject* aNewParent,
                                         nsISupports* aCOMObj,
                                         XPCWrappedNative** aWrapper)
{
    XPCNativeInterface* iface =
        XPCNativeInterface::GetISupports(ccx);

    if(!iface)
        return NS_ERROR_FAILURE;

    nsresult rv;

    nsRefPtr<XPCWrappedNative> wrapper;
    JSObject *flat;
    nsWrapperCache* cache = nsnull;
    CallQueryInterface(aCOMObj, &cache);
    if(cache)
    {
        flat = cache->GetWrapper();
        if(flat && !IS_SLIM_WRAPPER_OBJECT(flat))
            wrapper = static_cast<XPCWrappedNative*>(xpc_GetJSPrivate(flat));
        
    }
    else
    {
        rv = XPCWrappedNative::GetUsedOnly(ccx, aCOMObj, aOldScope, iface,
                                           getter_AddRefs(wrapper));
        if(NS_FAILED(rv))
            return rv;

        flat = wrapper->GetFlatJSObject();
    }

    if(!flat)
    {
        *aWrapper = nsnull;
        return NS_OK;
    }

    
    
    
    if (!XPCPerThreadData::IsMainThread(ccx) ||
        (wrapper &&
         wrapper->GetProto() &&
         !wrapper->GetProto()->ClassIsMainThreadOnly())) {
        return NS_ERROR_FAILURE;
    }

    if(aOldScope != aNewScope)
    {
        

        
        if(wrapper)
        {
            nsXPConnect* xpc = nsXPConnect::GetXPConnect();
            xpc->UpdateXOWs(ccx, wrapper, nsIXPConnect::XPC_XOW_CLEARSCOPE);
        }

        AutoMarkingWrappedNativeProtoPtr oldProto(ccx);
        AutoMarkingWrappedNativeProtoPtr newProto(ccx);

        if(!wrapper)
            oldProto = GetSlimWrapperProto(flat);
        else if(wrapper->HasProto())
            oldProto = wrapper->GetProto();

        if(oldProto)
        {
            XPCNativeScriptableInfo *info = oldProto->GetScriptableInfo();
            XPCNativeScriptableCreateInfo ci(*info);
            newProto =
                XPCWrappedNativeProto::GetNewOrUsed(ccx, aNewScope,
                                                    oldProto->GetClassInfo(),
                                                    &ci,
                                                    !oldProto->IsShared(),
                                                    (info->GetJSClass()->flags & JSCLASS_IS_GLOBAL),
                                                    oldProto->GetOffsetsMasked());
            if(!newProto)
            {
                return NS_ERROR_FAILURE;
            }
        }

        if(wrapper)
        {
            if(!XPC_XOW_WrapperMoved(ccx, wrapper, aNewScope))
            {
                return NS_ERROR_FAILURE;
            }

            Native2WrappedNativeMap* oldMap = aOldScope->GetWrappedNativeMap();
            Native2WrappedNativeMap* newMap = aNewScope->GetWrappedNativeMap();

            {   
                XPCAutoLock lock(aOldScope->GetRuntime()->GetMapLock());

                oldMap->Remove(wrapper);

                if(wrapper->HasProto())
                    wrapper->SetProto(newProto);

                
                
                

                if(wrapper->mScriptableInfo &&
                   wrapper->mScriptableInfo == oldProto->GetScriptableInfo())
                {
                    
                    
                    
                    

                    NS_ASSERTION(
                       oldProto->GetScriptableInfo()->GetScriptableShared() ==
                       newProto->GetScriptableInfo()->GetScriptableShared(),
                       "Changing proto is also changing JSObject Classname or "
                       "helper's nsIXPScriptable flags. This is not allowed!");

                    wrapper->mScriptableInfo = newProto->GetScriptableInfo();
                }

                NS_ASSERTION(!newMap->Find(wrapper->GetIdentityObject()),
                             "wrapper already in new scope!");

                (void) newMap->Add(wrapper);
            }

            
            

            if(wrapper->HasProto() &&
               STOBJ_GET_PROTO(flat) == oldProto->GetJSProtoObject())
            {
                if(!JS_SetPrototype(ccx, flat, newProto->GetJSProtoObject()))
                {
                    
                    NS_ERROR("JS_SetPrototype failed");
                    return NS_ERROR_FAILURE;
                }
            }
            else
            {
                NS_WARNING("Moving XPConnect wrappedNative to new scope, "
                           "but can't fixup __proto__");
            }
        }
        else
        {
            if(!JS_SetReservedSlot(ccx, flat, 0,
                                   PRIVATE_TO_JSVAL(newProto.get())) ||
               !JS_SetPrototype(ccx, flat, newProto->GetJSProtoObject()))
            {
                
                JS_SetReservedSlot(ccx, flat, 0, JSVAL_NULL);
                NS_ERROR("JS_SetPrototype failed");
                return NS_ERROR_FAILURE;
            }
        }
    }

    

    if(!JS_SetParent(ccx, flat, aNewParent))
    {
        return NS_ERROR_FAILURE;
    }

    *aWrapper = nsnull;
    wrapper.swap(*aWrapper);

    return NS_OK;
}

#define IS_TEAROFF_CLASS(clazz)                                               \
          ((clazz) == &XPC_WN_Tearoff_JSClass)


XPCWrappedNative*
XPCWrappedNative::GetWrappedNativeOfJSObject(JSContext* cx,
                                             JSObject* obj,
                                             JSObject* funobj,
                                             JSObject** pobj2,
                                             XPCWrappedNativeTearOff** pTearOff)
{
    NS_PRECONDITION(obj, "bad param");

    JSObject* cur;

    XPCWrappedNativeProto* proto = nsnull;
    nsIClassInfo* protoClassInfo = nsnull;

    
    

    if(funobj)
    {
        JSObject* funObjParent = STOBJ_GET_PARENT(funobj);
        NS_ASSERTION(funObjParent, "funobj has no parent");

        JSClass* funObjParentClass = STOBJ_GET_CLASS(funObjParent);

        if(IS_PROTO_CLASS(funObjParentClass))
        {
            NS_ASSERTION(STOBJ_GET_PARENT(funObjParent), "funobj's parent (proto) is global");
            proto = (XPCWrappedNativeProto*) xpc_GetJSPrivate(funObjParent);
            if(proto)
                protoClassInfo = proto->GetClassInfo();
        }
        else if(IS_WRAPPER_CLASS(funObjParentClass))
        {
            cur = funObjParent;
            goto return_wrapper;
        }
        else if(IS_TEAROFF_CLASS(funObjParentClass))
        {
            NS_ASSERTION(STOBJ_GET_PARENT(funObjParent), "funobj's parent (tearoff) is global");
            cur = funObjParent;
            goto return_tearoff;
        }
        else
        {
            NS_ERROR("function object has parent of unknown class!");
            return nsnull;
        }
    }

    for(cur = obj; cur; cur = STOBJ_GET_PROTO(cur))
    {
        
        JSClass* clazz;
        clazz = STOBJ_GET_CLASS(cur);

        if(IS_WRAPPER_CLASS(clazz))
        {
return_wrapper:
            JSBool isWN = IS_WN_WRAPPER_OBJECT(cur);
            XPCWrappedNative* wrapper =
                isWN ? (XPCWrappedNative*) xpc_GetJSPrivate(cur) : nsnull;
            if(proto)
            {
                XPCWrappedNativeProto* wrapper_proto =
                    isWN ? wrapper->GetProto() : GetSlimWrapperProto(cur);
                XPCWrappedNativeScope* wrapper_scope =
                    wrapper_proto ? wrapper_proto->GetScope() :
                                    wrapper->GetScope();
                if(proto != wrapper_proto &&
                   (proto->GetScope() != wrapper_scope ||
                    !protoClassInfo || !wrapper_proto ||
                    protoClassInfo != wrapper_proto->GetClassInfo()))
                    continue;
            }
            if(pobj2)
                *pobj2 = isWN ? nsnull : cur;
            return wrapper;
        }

        if(IS_TEAROFF_CLASS(clazz))
        {
return_tearoff:
            XPCWrappedNative* wrapper =
                (XPCWrappedNative*) xpc_GetJSPrivate(STOBJ_GET_PARENT(cur));
            if(proto && proto != wrapper->GetProto() &&
               (proto->GetScope() != wrapper->GetScope() ||
                !protoClassInfo || !wrapper->GetProto() ||
                protoClassInfo != wrapper->GetProto()->GetClassInfo()))
                continue;
            if(pobj2)
                *pobj2 = nsnull;
            XPCWrappedNativeTearOff* to =
                (XPCWrappedNativeTearOff*) xpc_GetJSPrivate(cur);
            if(!to)
                return nsnull;
            if(pTearOff)
                *pTearOff = to;
            return wrapper;
        }

        
        JSObject *unsafeObj;
        if((unsafeObj = XPCWrapper::Unwrap(cx, cur)))
            return GetWrappedNativeOfJSObject(cx, unsafeObj, funobj, pobj2,
                                              pTearOff);
    }

    
    

    JSClass *clazz = STOBJ_GET_CLASS(obj);

    if((clazz->flags & JSCLASS_IS_EXTENDED) &&
        ((JSExtendedClass*)clazz)->outerObject)
    {
        JSObject *outer = ((JSExtendedClass*)clazz)->outerObject(cx, obj);

        
        JSObject *unsafeObj;
        clazz = STOBJ_GET_CLASS(outer);
        if(clazz == &sXPC_XOW_JSClass.base &&
           (unsafeObj = XPCWrapper::UnwrapXOW(cx, outer)))
        {
            outer = unsafeObj;
        }

        if(outer && outer != obj)
            return GetWrappedNativeOfJSObject(cx, outer, funobj, pobj2,
                                              pTearOff);
    }

    if(pobj2)
        *pobj2 = nsnull;
    return nsnull;
}

JSBool
XPCWrappedNative::ExtendSet(XPCCallContext& ccx, XPCNativeInterface* aInterface)
{
    

    if(!mSet->HasInterface(aInterface))
    {
        AutoMarkingNativeSetPtr newSet(ccx);
        newSet = XPCNativeSet::GetNewOrUsed(ccx, mSet, aInterface,
                                            mSet->GetInterfaceCount());
        if(!newSet)
            return JS_FALSE;

        mSet = newSet;

        DEBUG_ReportShadowedMembers(newSet, this, GetProto());
    }
    return JS_TRUE;
}

XPCWrappedNativeTearOff*
XPCWrappedNative::LocateTearOff(XPCCallContext& ccx,
                              XPCNativeInterface* aInterface)
{
    XPCAutoLock al(GetLock()); 

    for(
        XPCWrappedNativeTearOffChunk* chunk = &mFirstChunk;
        chunk != nsnull;
        chunk = chunk->mNextChunk)
    {
        XPCWrappedNativeTearOff* tearOff = chunk->mTearOffs;
        XPCWrappedNativeTearOff* const end = tearOff + 
            XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK;
        for(
            tearOff = chunk->mTearOffs;
            tearOff < end; 
            tearOff++)
        {
            if(tearOff->GetInterface() == aInterface)
            {
                return tearOff;
            }
        }
    }
    return nsnull;
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
    XPCWrappedNativeTearOff* firstAvailable = nsnull;

    XPCWrappedNativeTearOffChunk* lastChunk;
    XPCWrappedNativeTearOffChunk* chunk;
    for(lastChunk = chunk = &mFirstChunk;
        chunk;
        lastChunk = chunk, chunk = chunk->mNextChunk)
    {
        to = chunk->mTearOffs;
        XPCWrappedNativeTearOff* const end = chunk->mTearOffs + 
            XPC_WRAPPED_NATIVE_TEAROFFS_PER_CHUNK;
        for(
            to = chunk->mTearOffs;
            to < end; 
            to++)
        {
            if(to->GetInterface() == aInterface)
            {
                if(needJSObject && !to->GetJSObject())
                {
                    AutoMarkingWrappedNativeTearOffPtr tearoff(ccx, to);
                    rv = InitTearOffJSObject(ccx, to);
                    
                    
                    
                    
                    to->Unmark();
                    if(NS_FAILED(rv))
                        to = nsnull;
                }
                goto return_result;
            }
            if(!firstAvailable && to->IsAvailable())
                firstAvailable = to;
        }
    }

    to = firstAvailable;

    if(!to)
    {
        XPCWrappedNativeTearOffChunk* newChunk =
            new XPCWrappedNativeTearOffChunk();
        if(!newChunk)
        {
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
        if(NS_FAILED(rv))
            to = nsnull;
    }

return_result:

    if(pError)
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

    
    
    if(mScriptableInfo &&
       mScriptableInfo->GetFlags().ClassInfoInterfacesOnly() &&
       !mSet->HasInterface(aInterface) &&
       !mSet->HasInterfaceWithAncestor(aInterface))
    {
        return NS_ERROR_NO_INTERFACE;
    }

    
    

    aTearOff->SetReserved();

    {   
        XPCAutoUnlock unlock(GetLock());

        if(NS_FAILED(identity->QueryInterface(*iid, (void**)&obj)) || !obj)
        {
            aTearOff->SetInterface(nsnull);
            return NS_ERROR_NO_INTERFACE;
        }

        
        if(iid->Equals(NS_GET_IID(nsIClassInfo)))
        {
            nsCOMPtr<nsISupports> alternate_identity(do_QueryInterface(obj));
            if(alternate_identity.get() != identity)
            {
                NS_RELEASE(obj);
                aTearOff->SetInterface(nsnull);
                return NS_ERROR_NO_INTERFACE;
            }
        }

        
        
        
        
        
        
        
        
        
        
        
        

        
        

        nsCOMPtr<nsIXPConnectWrappedJS> wrappedJS(do_QueryInterface(obj));
        if(wrappedJS)
        {
            JSObject* jso = nsnull;
            if(NS_SUCCEEDED(wrappedJS->GetJSObject(&jso)) &&
               jso == GetFlatJSObject())
            {
                
                
                
                
                
                
                
                
                
                
                
                
#ifdef DEBUG_xpc_hacker
                {
                    
                    
                    
                    
                    
                    
                    
                    if(HasProto())
                    {
                        JSObject* proto  = nsnull;
                        JSObject* our_proto = GetProto()->GetJSProtoObject();

                        proto = STOBJ_GET_PROTO(jso);

                        NS_ASSERTION(proto && proto != our_proto,
                            "!!! xpconnect/xbl check - wrapper has no special proto");

                        PRBool found_our_proto = PR_FALSE;
                        while(proto && !found_our_proto) {
                            proto = STOBJ_GET_PROTO(proto);

                            found_our_proto = proto == our_proto;
                        }

                        NS_ASSERTION(found_our_proto,
                            "!!! xpconnect/xbl check - wrapper has extra proto");
                    }
                    else
                    {
                        NS_WARNING("!!! xpconnect/xbl check - wrapper has no proto");
                    }
                }
#endif
                NS_RELEASE(obj);
                aTearOff->SetInterface(nsnull);
                return NS_OK;
            }
            
            
            
            
            
            
            
            
            
            
            

            nsXPCWrappedJSClass* clazz;
            if(iid->Equals(NS_GET_IID(nsIPropertyBag)) && jso &&
               NS_SUCCEEDED(nsXPCWrappedJSClass::GetNewOrUsed(ccx,*iid,&clazz))&&
               clazz)
            {
                JSObject* answer =
                    clazz->CallQueryInterfaceOnJSObject(ccx, jso, *iid);
                NS_RELEASE(clazz);
                if(!answer)
                {
                    NS_RELEASE(obj);
                    aTearOff->SetInterface(nsnull);
                    return NS_ERROR_NO_INTERFACE;
                }
            }
        }

        nsIXPCSecurityManager* sm;
           sm = ccx.GetXPCContext()->GetAppropriateSecurityManager(
                                nsIXPCSecurityManager::HOOK_CREATE_WRAPPER);
        if(sm && NS_FAILED(sm->
                    CanCreateWrapper(ccx, *iid, identity,
                                     GetClassInfo(), GetSecurityInfoAddr())))
        {
            
            NS_RELEASE(obj);
            aTearOff->SetInterface(nsnull);
            return NS_ERROR_XPC_SECURITY_MANAGER_VETO;
        }
    }
    

    
    
    
    

    if(!mSet->HasInterface(aInterface) && !ExtendSet(ccx, aInterface))
    {
        NS_RELEASE(obj);
        aTearOff->SetInterface(nsnull);
        return NS_ERROR_NO_INTERFACE;
    }

    aTearOff->SetInterface(aInterface);
    aTearOff->SetNative(obj);
#ifdef XPC_IDISPATCH_SUPPORT
    
    if(iid->Equals(NSID_IDISPATCH))
    {
        aTearOff->SetIDispatch(ccx);
    }  
#endif
    if(needJSObject && !InitTearOffJSObject(ccx, aTearOff))
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

JSBool
XPCWrappedNative::InitTearOffJSObject(XPCCallContext& ccx,
                                      XPCWrappedNativeTearOff* to)
{
    

    JSObject* obj =
        xpc_NewSystemInheritingJSObject(ccx, &XPC_WN_Tearoff_JSClass,
                                        GetScope()->GetPrototypeJSObject(),
                                        mFlatJSObject);

    if(!obj || !JS_SetPrivate(ccx, obj, to))
        return JS_FALSE;

    to->SetJSObject(obj);
    return JS_TRUE;
}



static JSBool Throw(uintN errNum, XPCCallContext& ccx)
{
    XPCThrower::Throw(errNum, ccx);
    return JS_FALSE;
}

enum SizeMode {GET_SIZE, GET_LENGTH};



static JSBool
GetArraySizeFromParam(XPCCallContext& ccx,
                      nsIInterfaceInfo* ifaceInfo,
                      const nsXPTMethodInfo* methodInfo,
                      const nsXPTParamInfo& paramInfo,
                      uint16 vtblIndex,
                      uint8 paramIndex,
                      SizeMode mode,
                      nsXPTCVariant* dispatchParams,
                      JSUint32* result)
{
    uint8 argnum;
    nsresult rv;

    

    if(mode == GET_SIZE)
        rv = ifaceInfo->GetSizeIsArgNumberForParam(vtblIndex, &paramInfo, 0, &argnum);
    else
        rv = ifaceInfo->GetLengthIsArgNumberForParam(vtblIndex, &paramInfo, 0, &argnum);
    if(NS_FAILED(rv))
        return Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, ccx);

    const nsXPTParamInfo& arg_param = methodInfo->GetParam(argnum);
    const nsXPTType& arg_type = arg_param.GetType();

    
    if(arg_type.IsPointer() || arg_type.TagPart() != nsXPTType::T_U32)
        return Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, ccx);

    *result = dispatchParams[argnum].val.u32;

    return JS_TRUE;
}


static JSBool
GetInterfaceTypeFromParam(XPCCallContext& ccx,
                          nsIInterfaceInfo* ifaceInfo,
                          const nsXPTMethodInfo* methodInfo,
                          const nsXPTParamInfo& paramInfo,
                          uint16 vtblIndex,
                          uint8 paramIndex,
                          const nsXPTType& datum_type,
                          nsXPTCVariant* dispatchParams,
                          nsID* result)
{
    uint8 argnum;
    nsresult rv;
    uint8 type_tag = datum_type.TagPart();

    

    if(type_tag == nsXPTType::T_INTERFACE)
    {
        rv = ifaceInfo->GetIIDForParamNoAlloc(vtblIndex, &paramInfo, result);
        if(NS_FAILED(rv))
            return ThrowBadParam(NS_ERROR_XPC_CANT_GET_PARAM_IFACE_INFO, paramIndex, ccx);
    }
    else if(type_tag == nsXPTType::T_INTERFACE_IS)
    {
        rv = ifaceInfo->GetInterfaceIsArgNumberForParam(vtblIndex, &paramInfo, &argnum);
        if(NS_FAILED(rv))
            return Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, ccx);

        const nsXPTParamInfo& arg_param = methodInfo->GetParam(argnum);
        const nsXPTType& arg_type = arg_param.GetType();
        
        
        if(!arg_type.IsPointer() || arg_type.TagPart() != nsXPTType::T_IID)
            return ThrowBadParam(NS_ERROR_XPC_CANT_GET_PARAM_IFACE_INFO, paramIndex, ccx);

        nsID* p = (nsID*) dispatchParams[argnum].val.p;
        if(!p)
            return ThrowBadParam(NS_ERROR_XPC_CANT_GET_PARAM_IFACE_INFO, paramIndex, ccx);
        *result = *p;
    }
    return JS_TRUE;
}




NS_SUPPRESS_STACK_CHECK JSBool
XPCWrappedNative::CallMethod(XPCCallContext& ccx,
                             CallMode mode )
{
    NS_ASSERTION(ccx.GetXPCContext()->CallerTypeIsJavaScript(),
                 "Native caller for XPCWrappedNative::CallMethod?");
    
    nsresult rv = ccx.CanCallNow();
    if(NS_FAILED(rv))
    {
        
        
        NS_ASSERTION(rv == NS_ERROR_XPC_SECURITY_MANAGER_VETO, 
                     "hmm? CanCallNow failed in XPCWrappedNative::CallMethod. "
                     "We are finding out about this late!");
        return Throw(rv, ccx);
    }

    DEBUG_TrackWrapperCall(ccx.GetWrapper(), mode);

    

#define PARAM_BUFFER_COUNT     8

    nsXPTCVariant paramBuffer[PARAM_BUFFER_COUNT];

    
    
    
    
    
    char autoString[sizeof(nsAutoString)];
    PRBool autoStringUsed = PR_FALSE;

    JSBool retval = JS_FALSE;

    nsXPTCVariant* dispatchParams = nsnull;
    uint8 i;
    const nsXPTMethodInfo* methodInfo;
    uint8 requiredArgs;
    uint8 paramCount;
    jsval src;
    nsresult invokeResult;
    nsID param_iid;
    uintN err;
    nsIXPCSecurityManager* sm;
    JSBool foundDependentParam;

    XPCJSRuntime* rt = ccx.GetRuntime();
    XPCContext* xpcc = ccx.GetXPCContext();
    nsISupports* callee = ccx.GetTearOff()->GetNative();
    XPCPerThreadData* tls = ccx.GetThreadData();
    uint16 vtblIndex = ccx.GetMethodIndex();
    nsIInterfaceInfo* ifaceInfo = ccx.GetInterface()->GetInterfaceInfo();
    jsval name = ccx.GetMember()->GetName();
    jsval* argv = ccx.GetArgv();
    PRUint32 argc = ccx.GetArgc();

    ccx.SetRetVal(JSVAL_VOID);

    tls->SetException(nsnull);
    xpcc->SetLastResult(NS_ERROR_UNEXPECTED);

    

    PRUint32 secFlag;
    PRUint32 secAction;

    switch(mode)
    {
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
            return JS_FALSE;
    }

    sm = xpcc->GetAppropriateSecurityManager(secFlag);
    if(sm && NS_FAILED(sm->CanAccess(secAction, &ccx, ccx,
                                     ccx.GetFlattenedJSObject(),
                                     ccx.GetWrapper()->GetIdentityObject(),
                                     ccx.GetWrapper()->GetClassInfo(), name,
                                     ccx.GetWrapper()->GetSecurityInfoAddr())))
    {
        
        return JS_FALSE;
    }

    
    
    if(vtblIndex == 0)
    {
        if(argc < 1)
        {
            Throw(NS_ERROR_XPC_NOT_ENOUGH_ARGS, ccx);
            return JS_FALSE;
        }
        const nsID* iid;
        JSObject* obj;
        if(!JSVAL_IS_OBJECT(argv[0]) ||
           (!(obj = JSVAL_TO_OBJECT(argv[0]))) ||
           (!(iid = xpc_JSObjectToID(ccx, obj))))
        {
            ThrowBadParam(NS_ERROR_XPC_BAD_CONVERT_JS, 0, ccx);
            return JS_FALSE;
        }

        nsISupports* qiresult = nsnull;
        {
            AutoJSSuspendNonMainThreadRequest req(ccx.GetJSContext());
            invokeResult = callee->QueryInterface(*iid, (void**) &qiresult);
        }

        xpcc->SetLastResult(invokeResult);

        if(NS_FAILED(invokeResult))
        {
            ThrowBadResult(invokeResult, ccx);
            return JS_FALSE;
        }

        jsval v = JSVAL_NULL;
        retval = XPCConvert::NativeData2JS(ccx, &v, &qiresult, 
                                           nsXPTType::T_INTERFACE_IS | XPT_TDP_POINTER,
                                           iid, ccx.GetCurrentJSObject(), &err);
        NS_IF_RELEASE(qiresult);

        if(!retval)
        {
            ThrowBadParam(err, 0, ccx);
            return JS_FALSE;
        }

        ccx.SetRetVal(v);
        return JS_TRUE;
    }

    if(NS_FAILED(ifaceInfo->GetMethodInfo(vtblIndex, &methodInfo)))
    {
        Throw(NS_ERROR_XPC_CANT_GET_METHOD_INFO, ccx);
        goto done;
    }

    
    paramCount = methodInfo->GetParamCount();
    requiredArgs = paramCount;
    if(paramCount && methodInfo->GetParam(paramCount-1).IsRetval())
        requiredArgs--;
    if(argc < requiredArgs)
    {
        
        while(requiredArgs && methodInfo->GetParam(requiredArgs-1).IsOptional())
          requiredArgs--;

        if(argc < requiredArgs) {
            Throw(NS_ERROR_XPC_NOT_ENOUGH_ARGS, ccx);
            goto done;
        }
    }

    
    if(paramCount > PARAM_BUFFER_COUNT)
    {
        if(!(dispatchParams = new nsXPTCVariant[paramCount]))
        {
            JS_ReportOutOfMemory(ccx);
            goto done;
        }
    }
    else
        dispatchParams = paramBuffer;

    
    for(i = 0; i < paramCount; i++)
    {
        nsXPTCVariant* dp = &dispatchParams[i];
        dp->ClearFlags();
        dp->val.p = nsnull;
    }

    
    
    
    
    foundDependentParam = JS_FALSE;
    for(i = 0; i < paramCount; i++)
    {
        JSBool useAllocator = JS_FALSE;
        const nsXPTParamInfo& paramInfo = methodInfo->GetParam(i);
        const nsXPTType& type = paramInfo.GetType();
        uint8 type_tag = type.TagPart();

        if(type.IsDependent())
        {
            foundDependentParam = JS_TRUE;
            continue;
        }

        nsXPTCVariant* dp = &dispatchParams[i];
        dp->type = type;

        if(type_tag == nsXPTType::T_INTERFACE)
        {
            dp->SetValIsInterface();
        }

        
        

        if((paramInfo.IsOut() || paramInfo.IsDipper()) &&
           !paramInfo.IsRetval()) {
          NS_ASSERTION(i < argc || paramInfo.IsOptional(),
                       "Expected either enough arguments or an optional argument");
          jsval arg = i < argc ? argv[i] : JSVAL_NULL;
          if(JSVAL_IS_PRIMITIVE(arg) ||
             !JS_GetPropertyById(ccx, JSVAL_TO_OBJECT(arg),
                                 rt->GetStringID(XPCJSRuntime::IDX_VALUE),
                                 &src))
          {
              ThrowBadParam(NS_ERROR_XPC_NEED_OUT_OBJECT, i, ccx);
              goto done;
          }
        }

        if(paramInfo.IsOut())
        {
            dp->SetPtrIsData();
            dp->ptr = &dp->val;

            if(type.IsPointer() &&
               type_tag != nsXPTType::T_INTERFACE &&
               !paramInfo.IsShared())
            {
                useAllocator = JS_TRUE;
                dp->SetValIsAllocated();
            }

            if(!paramInfo.IsIn())
                continue;
        }
        else
        {
            if(type.IsPointer())
            {
                switch(type_tag)
                {
                case nsXPTType::T_IID:
                    dp->SetValIsAllocated();
                    useAllocator = JS_TRUE;
                    break;

                case nsXPTType::T_ASTRING:
                    

                case nsXPTType::T_DOMSTRING:
                    if(paramInfo.IsDipper())
                    {
                        
                        
                        

                        if(!autoStringUsed)
                        {
                            
                            
                            
                            
                            nsAutoString *s = (nsAutoString*)&autoString;
                            new (s) nsAutoString();
                            autoStringUsed = PR_TRUE;

                            
                            
                            dp->val.p = s;
                            continue;
                        }

                        dp->SetValIsDOMString();
                        if(!(dp->val.p = new nsAutoString()))
                        {
                            JS_ReportOutOfMemory(ccx);
                            goto done;
                        }
                        continue;
                    }
                    

                    
                    
                    
                    dp->SetValIsDOMString();
                    useAllocator = JS_TRUE;
                    break;

                case nsXPTType::T_UTF8STRING:                    
                    
                case nsXPTType::T_CSTRING:                    
                    dp->SetValIsCString();
                    if(paramInfo.IsDipper())
                    {
                        
                        if(!(dp->val.p = new nsCString()))
                        {
                            JS_ReportOutOfMemory(ccx);
                            goto done;
                        }
                        continue;
                    }
                    
                    
                    useAllocator = JS_TRUE;
                    break;
                }
            }

            
            
            
            NS_ASSERTION(i < argc || paramInfo.IsOptional(),
                         "Expected either enough arguments or an optional argument");
            src = i < argc ? argv[i] : JSVAL_NULL;
        }

        if(type_tag == nsXPTType::T_INTERFACE &&
           NS_FAILED(ifaceInfo->GetIIDForParamNoAlloc(vtblIndex, &paramInfo,
                                               &param_iid)))
        {
            ThrowBadParam(NS_ERROR_XPC_CANT_GET_PARAM_IFACE_INFO, i, ccx);
            goto done;
        }

        if(!XPCConvert::JSData2Native(ccx, &dp->val, src, type,
                                      useAllocator, &param_iid, &err))
        {
            ThrowBadParam(err, i, ccx);
            goto done;
        }
    }

    
    if(foundDependentParam)
    {
        for(i = 0; i < paramCount; i++)
        {
            const nsXPTParamInfo& paramInfo = methodInfo->GetParam(i);
            const nsXPTType& type = paramInfo.GetType();

            if(!type.IsDependent())
                continue;

            nsXPTType datum_type;
            JSUint32 array_count;
            JSUint32 array_capacity;
            JSBool useAllocator = JS_FALSE;
            PRBool isArray = type.IsArray();

            PRBool isSizedString = isArray ?
                    JS_FALSE :
                    type.TagPart() == nsXPTType::T_PSTRING_SIZE_IS ||
                    type.TagPart() == nsXPTType::T_PWSTRING_SIZE_IS;

            nsXPTCVariant* dp = &dispatchParams[i];
            dp->type = type;

            if(isArray)
            {
                dp->SetValIsArray();

                if(NS_FAILED(ifaceInfo->GetTypeForParam(vtblIndex, &paramInfo, 1,
                                                    &datum_type)))
                {
                    Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, ccx);
                    goto done;
                }
            }
            else
                datum_type = type;

            if(datum_type.IsInterfacePointer())
            {
                dp->SetValIsInterface();
            }

            
            

            if(paramInfo.IsOut())
            {
                dp->SetPtrIsData();
                dp->ptr = &dp->val;

                if(!paramInfo.IsRetval()) {
                  NS_ASSERTION(i < argc || paramInfo.IsOptional(),
                               "Expected either enough arguments or an optional argument");
                  jsval arg = i < argc ? argv[i] : JSVAL_NULL;
                  if(JSVAL_IS_PRIMITIVE(arg) ||
                     !JS_GetPropertyById(ccx, JSVAL_TO_OBJECT(arg),
                         rt->GetStringID(XPCJSRuntime::IDX_VALUE), &src))
                  {
                      ThrowBadParam(NS_ERROR_XPC_NEED_OUT_OBJECT, i, ccx);
                      goto done;
                  }
                }

                if(datum_type.IsPointer() &&
                   !datum_type.IsInterfacePointer() &&
                   (isArray || !paramInfo.IsShared()))
                {
                    useAllocator = JS_TRUE;
                    dp->SetValIsAllocated();
                }

                if(!paramInfo.IsIn())
                    continue;
            }
            else
            {
                NS_ASSERTION(i < argc || paramInfo.IsOptional(),
                             "Expected either enough arguments or an optional argument");
                src = i < argc ? argv[i] : JSVAL_NULL;

                if(datum_type.IsPointer() &&
                   datum_type.TagPart() == nsXPTType::T_IID)
                {
                    useAllocator = JS_TRUE;
                    dp->SetValIsAllocated();
                }
            }

            if(datum_type.IsInterfacePointer() &&
               !GetInterfaceTypeFromParam(ccx, ifaceInfo, methodInfo, paramInfo,
                                          vtblIndex, i, datum_type,
                                          dispatchParams, &param_iid))
                goto done;

            if(isArray || isSizedString)
            {
                if(!GetArraySizeFromParam(ccx, ifaceInfo, methodInfo, paramInfo,
                                          vtblIndex, i, GET_SIZE,
                                          dispatchParams, &array_capacity)||
                   !GetArraySizeFromParam(ccx, ifaceInfo, methodInfo, paramInfo,
                                          vtblIndex, i, GET_LENGTH,
                                          dispatchParams, &array_count))
                    goto done;

                if(isArray)
                {
                    if(array_count &&
                       !XPCConvert::JSArray2Native(ccx, (void**)&dp->val, src,
                                                   array_count, array_capacity,
                                                   datum_type,
                                                   useAllocator,
                                                   &param_iid, &err))
                    {
                        
                        ThrowBadParam(err, i, ccx);
                        goto done;
                    }
                }
                else 
                {
                    if(!XPCConvert::JSStringWithSize2Native(ccx,
                                                   (void*)&dp->val,
                                                   src,
                                                   array_count, array_capacity,
                                                   datum_type, useAllocator,
                                                   &err))
                    {
                        ThrowBadParam(err, i, ccx);
                        goto done;
                    }
                }
            }
            else
            {
                if(!XPCConvert::JSData2Native(ccx, &dp->val, src, type,
                                              useAllocator, &param_iid,
                                              &err))
                {
                    ThrowBadParam(err, i, ccx);
                    goto done;
                }
            }
        }
    }


    
    {
        AutoJSSuspendNonMainThreadRequest req(ccx.GetJSContext());
        invokeResult = NS_InvokeByIndex(callee, vtblIndex, paramCount,
                                        dispatchParams);
    }

    xpcc->SetLastResult(invokeResult);

    if(NS_FAILED(invokeResult))
    {
        ThrowBadResult(invokeResult, ccx);
        goto done;
    }
    else if(JS_IsExceptionPending(ccx))
    {
        goto done;
    }

    
    for(i = 0; i < paramCount; i++)
    {
        const nsXPTParamInfo& paramInfo = methodInfo->GetParam(i);
        if(!paramInfo.IsOut() && !paramInfo.IsDipper())
            continue;

        const nsXPTType& type = paramInfo.GetType();
        nsXPTCVariant* dp = &dispatchParams[i];
        jsval v = JSVAL_NULL;
        AUTO_MARK_JSVAL(ccx, &v);
        JSUint32 array_count;
        nsXPTType datum_type;
        PRBool isArray = type.IsArray();
        PRBool isSizedString = isArray ?
                JS_FALSE :
                type.TagPart() == nsXPTType::T_PSTRING_SIZE_IS ||
                type.TagPart() == nsXPTType::T_PWSTRING_SIZE_IS;

        if(isArray)
        {
            if(NS_FAILED(ifaceInfo->GetTypeForParam(vtblIndex, &paramInfo, 1,
                                                    &datum_type)))
            {
                Throw(NS_ERROR_XPC_CANT_GET_ARRAY_INFO, ccx);
                goto done;
            }
        }
        else
            datum_type = type;

        if(isArray || isSizedString)
        {
            if(!GetArraySizeFromParam(ccx, ifaceInfo, methodInfo, paramInfo,
                                      vtblIndex, i, GET_LENGTH, dispatchParams,
                                      &array_count))
                goto done;
        }

        if(datum_type.IsInterfacePointer() &&
           !GetInterfaceTypeFromParam(ccx, ifaceInfo, methodInfo, paramInfo,
                                      vtblIndex, i, datum_type, dispatchParams,
                                      &param_iid))
            goto done;

        if(isArray)
        {
            XPCLazyCallContext lccx(ccx);
            if(!XPCConvert::NativeArray2JS(lccx, &v, (const void**)&dp->val,
                                           datum_type, &param_iid,
                                           array_count, ccx.GetCurrentJSObject(),
                                           &err))
            {
                
                ThrowBadParam(err, i, ccx);
                goto done;
            }
        }
        else if(isSizedString)
        {
            if(!XPCConvert::NativeStringWithSize2JS(ccx, &v,
                                           (const void*)&dp->val,
                                           datum_type,
                                           array_count, &err))
            {
                ThrowBadParam(err, i, ccx);
                goto done;
            }
        }
        else
        {
            if(!XPCConvert::NativeData2JS(ccx, &v, &dp->val, datum_type,
                                          &param_iid,
                                          ccx.GetCurrentJSObject(), &err))
            {
                ThrowBadParam(err, i, ccx);
                goto done;
            }
        }

        if(paramInfo.IsRetval())
        {
            if(!ccx.GetReturnValueWasSet())
                ccx.SetRetVal(v);
        }
        else if(i < argc)
        {
            
            NS_ASSERTION(JSVAL_IS_OBJECT(argv[i]), "out var is not object");
            if(!JS_SetPropertyById(ccx, JSVAL_TO_OBJECT(argv[i]),
                        rt->GetStringID(XPCJSRuntime::IDX_VALUE), &v))
            {
                ThrowBadParam(NS_ERROR_XPC_CANT_SET_OUT_VAL, i, ccx);
                goto done;
            }
        }
        else
        {
            NS_ASSERTION(paramInfo.IsOptional(),
                         "Expected either enough arguments or an optional argument");
        }
    }

    retval = JS_TRUE;
done:
    
    
    if(dispatchParams)
    {
        for(i = 0; i < paramCount; i++)
        {
            nsXPTCVariant* dp = &dispatchParams[i];
            void* p = dp->val.p;
            if(!p)
                continue;

            if(dp->IsValArray())
            {
                
                if(dp->IsValAllocated() || dp->IsValInterface())
                {
                    
                    JSUint32 array_count;

                    const nsXPTParamInfo& paramInfo = methodInfo->GetParam(i);
                    if(!GetArraySizeFromParam(ccx, ifaceInfo, methodInfo,
                                              paramInfo, vtblIndex,
                                              i, GET_LENGTH, dispatchParams,
                                              &array_count))
                    {
                        NS_ERROR("failed to get array length, we'll leak here");
                        continue;
                    }
                    if(dp->IsValAllocated())
                    {
                        void** a = (void**)p;
                        for(JSUint32 k = 0; k < array_count; k++)
                        {
                            void* o = a[k];
                            if(o) nsMemory::Free(o);
                        }
                    }
                    else 
                    {
                        nsISupports** a = (nsISupports**)p;
                        for(JSUint32 k = 0; k < array_count; k++)
                        {
                            nsISupports* o = a[k];
                            NS_IF_RELEASE(o);
                        }
                    }
                }
                
                nsMemory::Free(p);
            }
            else if(dp->IsValAllocated())
                nsMemory::Free(p);
            else if(dp->IsValInterface())
                ((nsISupports*)p)->Release();
            else if(dp->IsValDOMString())
                ccx.DeleteString((nsAString*)p);
            else if(dp->IsValUTF8String())
                delete (nsCString*) p;
            else if(dp->IsValCString())
                delete (nsCString*) p;
        }   
    }

    if (autoStringUsed) {
        

        nsAutoString *s = (nsAutoString*)&autoString;

        s->~nsAutoString();
    }

    if(dispatchParams && dispatchParams != paramBuffer)
        delete [] dispatchParams;

    return retval;
}





NS_IMETHODIMP XPCWrappedNative::GetJSObject(JSObject * *aJSObject)
{
    *aJSObject = mFlatJSObject;
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

#ifndef XPCONNECT_STANDALONE
nsIPrincipal*
XPCWrappedNative::GetObjectPrincipal() const
{
    nsIPrincipal* principal = GetScope()->GetPrincipal();
#ifdef DEBUG
    nsCOMPtr<nsIScriptObjectPrincipal> objPrin(do_QueryInterface(mIdentity));
    NS_ASSERTION(!objPrin || objPrin->GetPrincipal() == principal,
                 "Principal mismatch.  Expect bad things to happen");
#endif
    return principal;
}
#endif


NS_IMETHODIMP XPCWrappedNative::GetXPConnect(nsIXPConnect * *aXPConnect)
{
    if(IsValid())
    {
        nsIXPConnect* temp = GetRuntime()->GetXPConnect();
        NS_IF_ADDREF(temp);
        *aXPConnect = temp;
    }
    else
        *aXPConnect = nsnull;
    return NS_OK;
}


NS_IMETHODIMP XPCWrappedNative::FindInterfaceWithMember(jsval name, nsIInterfaceInfo * *_retval)
{
    XPCNativeInterface* iface;
    XPCNativeMember*  member;

    if(GetSet()->FindMember(name, &member, &iface) && iface)
    {
        nsIInterfaceInfo* temp = iface->GetInterfaceInfo();
        NS_IF_ADDREF(temp);
        *_retval = temp;
    }
    else
        *_retval = nsnull;
    return NS_OK;
}


NS_IMETHODIMP XPCWrappedNative::FindInterfaceWithName(jsval name, nsIInterfaceInfo * *_retval)
{
    XPCNativeInterface* iface = GetSet()->FindNamedInterface(name);
    if(iface)
    {
        nsIInterfaceInfo* temp = iface->GetInterfaceInfo();
        NS_IF_ADDREF(temp);
        *_retval = temp;
    }
    else
        *_retval = nsnull;
    return NS_OK;
}

inline nsresult UnexpectedFailure(nsresult rv)
{
    NS_ERROR("This is not supposed to fail!");
    return rv;
}


NS_IMETHODIMP XPCWrappedNative::RefreshPrototype()
{
    XPCCallContext ccx(NATIVE_CALLER);
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    if(!HasProto())
        return NS_OK;

    if(!GetFlatJSObject())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    AutoMarkingWrappedNativeProtoPtr oldProto(ccx);
    AutoMarkingWrappedNativeProtoPtr newProto(ccx);
    
    oldProto = GetProto();

    XPCNativeScriptableInfo *info = oldProto->GetScriptableInfo();
    XPCNativeScriptableCreateInfo ci(*info);
    newProto = XPCWrappedNativeProto::GetNewOrUsed(ccx, oldProto->GetScope(),
                                                   oldProto->GetClassInfo(),
                                                   &ci,
                                                   !oldProto->IsShared(),
                                                   (info->GetJSClass()->flags & JSCLASS_IS_GLOBAL),
                                                   oldProto->GetOffsetsMasked());
    if(!newProto)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    

    if(newProto.get() == oldProto.get())
        return NS_OK;

    if(!JS_SetPrototype(ccx, GetFlatJSObject(), newProto->GetJSProtoObject()))
        return UnexpectedFailure(NS_ERROR_FAILURE);

    SetProto(newProto);

    if(mScriptableInfo == oldProto->GetScriptableInfo())
        mScriptableInfo = newProto->GetScriptableInfo();

    return NS_OK;
}

NS_IMETHODIMP XPCWrappedNative::GetSecurityInfoAddress(void*** securityInfoAddrPtr)
{
    NS_ENSURE_ARG_POINTER(securityInfoAddrPtr);
    *securityInfoAddrPtr = GetSecurityInfoAddr();
    return NS_OK;
}


NS_IMETHODIMP XPCWrappedNative::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth-- ;
    XPC_LOG_ALWAYS(("XPCWrappedNative @ %x with mRefCnt = %d", this, mRefCnt.get()));
    XPC_LOG_INDENT();

        if(HasProto())
        {
            XPCWrappedNativeProto* proto = GetProto();
            if(depth && proto)
                proto->DebugDump(depth);
            else
                XPC_LOG_ALWAYS(("mMaybeProto @ %x", proto));
        }
        else
            XPC_LOG_ALWAYS(("Scope @ %x", GetScope()));

        if(depth && mSet)
            mSet->DebugDump(depth);
        else
            XPC_LOG_ALWAYS(("mSet @ %x", mSet));

        XPC_LOG_ALWAYS(("mFlatJSObject of %x", mFlatJSObject));
        XPC_LOG_ALWAYS(("mIdentity of %x", mIdentity));
        XPC_LOG_ALWAYS(("mScriptableInfo @ %x", mScriptableInfo));

        if(depth && mScriptableInfo)
        {
            XPC_LOG_INDENT();
            XPC_LOG_ALWAYS(("mScriptable @ %x", mScriptableInfo->GetCallback()));
            XPC_LOG_ALWAYS(("mFlags of %x", (PRUint32)mScriptableInfo->GetFlags()));
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

    char* sz = nsnull;
    char* name = nsnull;

    XPCNativeScriptableInfo* si = GetScriptableInfo();
    if(si)
        name = JS_smprintf("%s", si->GetJSClass()->name);
    if(to)
    {
        const char* fmt = name ? " (%s)" : "%s";
        name = JS_sprintf_append(name, fmt,
                                 to->GetInterface()->GetNameString());
    }
    else if(!name)
    {
        XPCNativeSet* set = GetSet();
        XPCNativeInterface** array = set->GetInterfaceArray();
        PRUint16 count = set->GetInterfaceCount();

        if(count == 1)
            name = JS_sprintf_append(name, "%s", array[0]->GetNameString());
        else if(count == 2 &&
                array[0] == XPCNativeInterface::GetISupports(ccx))
        {
            name = JS_sprintf_append(name, "%s", array[1]->GetNameString());
        }
        else
        {
            for(PRUint16 i = 0; i < count; i++)
            {
                const char* fmt = (i == 0) ?
                                    "(%s" : (i == count-1) ?
                                        ", %s)" : ", %s";
                name = JS_sprintf_append(name, fmt,
                                         array[i]->GetNameString());
            }
        }
    }

    if(!name)
    {
        return nsnull;
    }
    const char* fmt = "[xpconnect wrapped %s" FMT_ADDR FMT_STR(" (native")
        FMT_ADDR FMT_STR(")") "]";
    if(si)
    {
        fmt = "[object %s" FMT_ADDR FMT_STR(" (native") FMT_ADDR FMT_STR(")") "]";
    }
    sz = JS_smprintf(fmt, name PARAM_ADDR(this) PARAM_ADDR(mIdentity));

    JS_smprintf_free(name);


    return sz;

#undef FMT_ADDR
#undef PARAM_ADDR
}



#ifdef XPC_DETECT_LEADING_UPPERCASE_ACCESS_ERRORS

void
XPCWrappedNative::HandlePossibleNameCaseError(JSContext* cx,
                                              XPCNativeSet* set,
                                              XPCNativeInterface* iface,
                                              jsval name)
{
    XPCCallContext ccx(JS_CALLER, cx);
    HandlePossibleNameCaseError(ccx, set, iface, name);
}


void
XPCWrappedNative::HandlePossibleNameCaseError(XPCCallContext& ccx,
                                              XPCNativeSet* set,
                                              XPCNativeInterface* iface,
                                              jsval name)
{
    if(!ccx.IsValid())
        return;

    JSString* oldJSStr;
    JSString* newJSStr;
    PRUnichar* oldStr;
    PRUnichar* newStr;
    XPCNativeMember* member;
    XPCNativeInterface* localIface;

    
    if(JSVAL_IS_STRING(name) &&
       nsnull != (oldJSStr = JSVAL_TO_STRING(name)) &&
       nsnull != (oldStr = (PRUnichar*) JS_GetStringChars(oldJSStr)) &&
       oldStr[0] != 0 &&
       oldStr[0] >> 8 == 0 &&
       nsCRT::IsUpper((char)oldStr[0]) &&
       nsnull != (newStr = nsCRT::strdup(oldStr)))
    {
        newStr[0] = (PRUnichar) nsCRT::ToLower((char)newStr[0]);
        newJSStr = JS_NewUCStringCopyZ(ccx, (const jschar*)newStr);
        nsCRT::free(newStr);
        if(newJSStr && (set ?
             set->FindMember(STRING_TO_JSVAL(newJSStr), &member, &localIface) :
                        NS_PTR_TO_INT32(iface->FindMember(STRING_TO_JSVAL(newJSStr)))))
        {
            
            const char* ifaceName = set ?
                    localIface->GetNameString() :
                    iface->GetNameString();
            const char* goodName = JS_GetStringBytes(newJSStr);
            const char* badName = JS_GetStringBytes(oldJSStr);
            char* locationStr = nsnull;

            nsIException* e = nsnull;
            nsXPCException::NewException("", NS_OK, nsnull, nsnull, &e);

            if(e)
            {
                nsresult rv;
                nsCOMPtr<nsIStackFrame> loc = nsnull;
                rv = e->GetLocation(getter_AddRefs(loc));
                if(NS_SUCCEEDED(rv) && loc) {
                    loc->ToString(&locationStr); 
                }
            }

            if(locationStr && ifaceName && goodName && badName )
            {
                printf("**************************************************\n"
                       "ERROR: JS code at [%s]\n"
                       "tried to access nonexistent property called\n"
                       "\'%s\' on interface of type \'%s\'.\n"
                       "That interface does however have a property called\n"
                       "\'%s\'. Did you mean to access that lowercase property?\n"
                       "Please fix the JS code as appropriate.\n"
                       "**************************************************\n",
                        locationStr, badName, ifaceName, goodName);
            }
            if(locationStr)
                nsMemory::Free(locationStr);
        }
    }
}
#endif

#ifdef XPC_CHECK_CLASSINFO_CLAIMS
static void DEBUG_CheckClassInfoClaims(XPCWrappedNative* wrapper)
{
    if(!wrapper || !wrapper->GetClassInfo())
        return;

    nsISupports* obj = wrapper->GetIdentityObject();
    XPCNativeSet* set = wrapper->GetSet();
    PRUint16 count = set->GetInterfaceCount();
    for(PRUint16 i = 0; i < count; i++)
    {
        nsIClassInfo* clsInfo = wrapper->GetClassInfo();
        XPCNativeInterface* iface = set->GetInterfaceAt(i);
        nsIInterfaceInfo* info = iface->GetInterfaceInfo();
        const nsIID* iid;
        nsISupports* ptr;

        info->GetIIDShared(&iid);
        nsresult rv = obj->QueryInterface(*iid, (void**)&ptr);
        if(NS_SUCCEEDED(rv))
        {
            NS_RELEASE(ptr);
            continue;
        }
        if(rv == NS_ERROR_OUT_OF_MEMORY)
            continue;

        

        char* className = nsnull;
        char* contractID = nsnull;
        const char* interfaceName;

        info->GetNameShared(&interfaceName);
        clsInfo->GetContractID(&contractID);
        if(wrapper->GetScriptableInfo())
        {
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

        if(className)
            nsMemory::Free(className);
        if(contractID)
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
    if(header)
        printf("%s\n", header);

    printf("   XPCNativeSet @ 0x%p for the class:\n", (void*)set);

    char* className = nsnull;
    char* contractID = nsnull;

    nsIClassInfo* clsInfo = proto ? proto->GetClassInfo() : nsnull;
    if(clsInfo)
        clsInfo->GetContractID(&contractID);

    XPCNativeScriptableInfo* si = wrapper ?
            wrapper->GetScriptableInfo() :
            proto->GetScriptableInfo();
    if(si)
        si->GetCallback()->GetClassName(&className);

    printf("   classname: %s \n"
           "   contractid: %s \n",
           className ? className : "<unknown>",
           contractID ? contractID : "<unknown>");

    if(className)
        nsMemory::Free(className);
    if(contractID)
        nsMemory::Free(contractID);

    printf("   claims to implement interfaces:\n");

    PRUint16 count = set->GetInterfaceCount();
    for(PRUint16 i = 0; i < count; i++)
    {
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
    if(JSVAL_IS_STRING(memberName))
        printf("%s::%s", JS_GetStringBytes(JSVAL_TO_STRING(ifaceName)),
                         JS_GetStringBytes(JSVAL_TO_STRING(memberName)));
    else
        printf("%s", JS_GetStringBytes(JSVAL_TO_STRING(ifaceName)));
}

static void ShowHeader(JSBool* printedHeader,
                       const char* header,
                       XPCNativeSet* set,
                       XPCWrappedNative* wrapper,
                       XPCWrappedNativeProto* proto)
{
    if(!*printedHeader)
    {
        DEBUG_PrintShadowObjectInfo(header, set, wrapper, proto);
        *printedHeader = JS_TRUE;
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
    printf(" ! %s appears twice in the nsIClassInfo interface set!\n",
           JS_GetStringBytes(JSVAL_TO_STRING(ifaceName)));
}

static JSBool InterfacesAreRelated(XPCNativeInterface* iface1,
                                   XPCNativeInterface* iface2)
{
    nsIInterfaceInfo* info1 = iface1->GetInterfaceInfo();
    nsIInterfaceInfo* info2 = iface2->GetInterfaceInfo();

    NS_ASSERTION(info1 != info2, "should not have different iface!");

    PRBool match;

    return
        (NS_SUCCEEDED(info1->HasAncestor(iface2->GetIID(), &match)) && match) ||
        (NS_SUCCEEDED(info2->HasAncestor(iface1->GetIID(), &match)) && match);
}

static JSBool MembersAreTheSame(XPCNativeInterface* iface1,
                                PRUint16 memberIndex1,
                                XPCNativeInterface* iface2,
                                PRUint16 memberIndex2)
{
    nsIInterfaceInfo* info1 = iface1->GetInterfaceInfo();
    nsIInterfaceInfo* info2 = iface2->GetInterfaceInfo();

    XPCNativeMember* member1 = iface1->GetMemberAt(memberIndex1);
    XPCNativeMember* member2 = iface2->GetMemberAt(memberIndex2);

    PRUint16 index1 = member1->GetIndex();
    PRUint16 index2 = member2->GetIndex();

    

    if(member1->IsConstant())
    {
        if(!member2->IsConstant())
            return JS_FALSE;

        const nsXPTConstant* constant1;
        const nsXPTConstant* constant2;

        return NS_SUCCEEDED(info1->GetConstant(index1, &constant1)) &&
               NS_SUCCEEDED(info2->GetConstant(index2, &constant2)) &&
               constant1->GetType() == constant2->GetType() &&
               constant1->GetValue() == constant2->GetValue();
    }

    
    

    if(member1->IsMethod() != member2->IsMethod() ||
       member1->IsWritableAttribute() != member2->IsWritableAttribute() ||
       member1->IsReadOnlyAttribute() != member2->IsReadOnlyAttribute())
    {
        return JS_FALSE;
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
    

    if(!(proto || wrapper) || !set || set->GetInterfaceCount() < 2)
        return;

    NS_ASSERTION(proto || wrapper, "bad param!");
    XPCJSRuntime* rt = proto ? proto->GetRuntime() : wrapper->GetRuntime();

    
    static int nextSeenSet = 0;
    static const int MAX_SEEN_SETS = 128;
    static XPCNativeSet* SeenSets[MAX_SEEN_SETS];
    for(int seen = 0; seen < MAX_SEEN_SETS; seen++)
        if(set == SeenSets[seen])
            return;
    SeenSets[nextSeenSet] = set;

#ifdef off_DEBUG_jband
    static int seenCount = 0;
    printf("--- adding SeenSets[%d] = 0x%p\n", nextSeenSet, set);
    DEBUG_PrintShadowObjectInfo(nsnull, set, wrapper, proto);
#endif
    int localNext = nextSeenSet+1;
    nextSeenSet = localNext < MAX_SEEN_SETS ? localNext : 0;

    XPCNativeScriptableInfo* si = wrapper ?
            wrapper->GetScriptableInfo() :
            proto->GetScriptableInfo();

    
    if(si)
    {
        
        static const char* skipClasses[] = {
            "Window",
            "HTMLDocument",
            "HTMLCollection",
            "Event",
            "ChromeWindow",
            nsnull
        };

        static PRBool warned = JS_FALSE;
        if(!warned)
        {
            printf("!!! XPConnect won't warn about Shadowed Members of...\n  ");
            for(const char** name = skipClasses; *name; name++)
                printf("%s %s", name == skipClasses ? "" : ",", *name);
             printf("\n");
            warned = JS_TRUE;
        }

        PRBool quit = JS_FALSE;
        char* className = nsnull;
        si->GetCallback()->GetClassName(&className);
        if(className)
        {
            for(const char** name = skipClasses; *name; name++)
            {
                if(!strcmp(*name, className))
                {
                    quit = JS_TRUE;
                    break;
                }
            }
            nsMemory::Free(className);
        }
        if(quit)
            return;
    }

    const char header[] =
        "!!!Object wrapped by XPConnect has members whose names shadow each other!!!";

    JSBool printedHeader = JS_FALSE;

    jsval QIName = rt->GetStringJSVal(XPCJSRuntime::IDX_QUERY_INTERFACE);

    PRUint16 ifaceCount = set->GetInterfaceCount();
    PRUint16 i, j, k, m;

    

    for(i = 0; i < ifaceCount; i++)
    {
        XPCNativeInterface* ifaceOuter = set->GetInterfaceAt(i);
        for(k = i+1; k < ifaceCount; k++)
        {
            XPCNativeInterface* ifaceInner = set->GetInterfaceAt(k);
            if(ifaceInner == ifaceOuter)
            {
                ShowHeader(&printedHeader, header, set, wrapper, proto);
                ShowDuplicateInterface(ifaceOuter->GetName());
            }
        }
    }

    

    for(i = 0; i < ifaceCount; i++)
    {
        XPCNativeInterface* ifaceOuter = set->GetInterfaceAt(i);
        jsval ifaceOuterName = ifaceOuter->GetName();

        PRUint16 memberCountOuter = ifaceOuter->GetMemberCount();
        for(j = 0; j < memberCountOuter; j++)
        {
            XPCNativeMember* memberOuter = ifaceOuter->GetMemberAt(j);
            jsval memberOuterName = memberOuter->GetName();

            if(memberOuterName == QIName)
                continue;

            for(k = i+1; k < ifaceCount; k++)
            {
                XPCNativeInterface* ifaceInner = set->GetInterfaceAt(k);
                jsval ifaceInnerName = ifaceInner->GetName();

                
                if(ifaceInner == ifaceOuter)
                    continue;

                
                
                if(InterfacesAreRelated(ifaceInner, ifaceOuter))
                    continue;

                if(ifaceInnerName == memberOuterName)
                {
                    ShowHeader(&printedHeader, header, set, wrapper, proto);
                    ShowOneShadow(ifaceInnerName, JSVAL_NULL,
                                  ifaceOuterName, memberOuterName);
                }

                PRUint16 memberCountInner = ifaceInner->GetMemberCount();

                for(m = 0; m < memberCountInner; m++)
                {
                    XPCNativeMember* memberInner = ifaceInner->GetMemberAt(m);
                    jsval memberInnerName = memberInner->GetName();

                    if(memberInnerName == QIName)
                        continue;

                    if(memberOuterName == memberInnerName &&
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

#ifdef XPC_CHECK_WRAPPER_THREADSAFETY
void DEBUG_ReportWrapperThreadSafetyError(XPCCallContext& ccx,
                                          const char* msg,
                                          const XPCWrappedNative* wrapper)
{
    XPCPerThreadData* tls = ccx.GetThreadData();
    if(1 != tls->IncrementWrappedNativeThreadsafetyReportDepth())
        return;

    printf("---------------------------------------------------------------\n");
    printf("!!!!! XPConnect wrapper thread use error...\n");

    char* wrapperDump = wrapper->ToString(ccx);
    if(wrapperDump)
    {
        printf("  %s\n  wrapper: %s\n", msg, wrapperDump);
        JS_smprintf_free(wrapperDump);
    }
    else
        printf("  %s\n  wrapper @ 0x%p\n", msg, (void *)wrapper);

    printf("  JS call stack...\n");
    xpc_DumpJSStack(ccx, JS_TRUE, JS_TRUE, JS_TRUE);
    printf("---------------------------------------------------------------\n");
    
    tls->ClearWrappedNativeThreadsafetyReportDepth();
}

void DEBUG_CheckWrapperThreadSafety(const XPCWrappedNative* wrapper)
{
    XPCWrappedNativeProto* proto = wrapper->GetProto();
    if(proto && proto->ClassIsThreadSafe())
        return;

    PRBool val;
    if(proto && proto->ClassIsMainThreadOnly())
    {
        if(!NS_IsMainThread())
        {
            XPCCallContext ccx(NATIVE_CALLER);
            DEBUG_ReportWrapperThreadSafetyError(ccx,
                "Main Thread Only wrapper accessed on another thread", wrapper);
        }
    }
    else if(NS_SUCCEEDED(wrapper->mThread->IsOnCurrentThread(&val)) && !val)
    {
        XPCCallContext ccx(NATIVE_CALLER);
        DEBUG_ReportWrapperThreadSafetyError(ccx,
            "XPConnect WrappedNative is being accessed on multiple threads but "
            "the underlying native xpcom object does not have a "
            "nsIClassInfo with the 'THREADSAFE' flag set", wrapper);
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
    RemoveFromRootSet(nsXPConnect::GetRuntimeInstance()->GetJSRuntime());
}

void
XPCJSObjectHolder::TraceJS(JSTracer *trc)
{
    JS_SET_TRACING_DETAILS(trc, PrintTraceName, this, 0);
    JS_CallTracer(trc, mJSObj, JSTRACE_OBJECT);
}

#ifdef DEBUG

void
XPCJSObjectHolder::PrintTraceName(JSTracer* trc, char *buf, size_t bufsize)
{
    JS_snprintf(buf, bufsize, "XPCJSObjectHolder[0x%p].mJSObj",
                trc->debugPrintArg);
}
#endif


XPCJSObjectHolder*
XPCJSObjectHolder::newHolder(XPCCallContext& ccx, JSObject* obj)
{
    if(!obj)
    {
        NS_ERROR("bad param");
        return nsnull;
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
    nsWrapperCache *cache = nsnull;
    CallQueryInterface(object, &cache);
    nsRefPtr<XPCWrappedNative> wn;
    nsresult rv = XPCWrappedNative::Morph(ccx, obj, nsnull, cache,
                                          getter_AddRefs(wn));
    return NS_SUCCEEDED(rv);
}

#ifdef DEBUG_slimwrappers
static PRUint32 sSlimWrappers;
#endif

JSBool
ConstructSlimWrapper(XPCCallContext &ccx, nsISupports *p, nsWrapperCache *cache,
                     XPCWrappedNativeScope* xpcScope, jsval *rval)
{
    nsCOMPtr<nsISupports> identityObj = do_QueryInterface(p);

    nsCOMPtr<nsIClassInfo> classInfo = do_QueryInterface(p);
    if(!classInfo)
        return JS_FALSE;

    
    
    XPCNativeScriptableCreateInfo sciProto;
    nsresult rv = XPCWrappedNative::GatherProtoScriptableCreateInfo(classInfo,
                                                                    &sciProto);
    NS_ENSURE_SUCCESS(rv, JS_FALSE);

    JSObject* parent = xpcScope->GetGlobalJSObject();
    if(!sciProto.GetFlags().WantPreCreate())
    {
        SLIM_LOG_NOT_CREATED(ccx, identityObj,
                             "scriptable helper has no PreCreate hook");

        return JS_FALSE;
    }

    JSObject* plannedParent = parent;
    rv = sciProto.GetCallback()->PreCreate(identityObj, ccx, parent,
                                           &parent);
    if(rv != NS_SUCCESS_ALLOW_SLIM_WRAPPERS)
    {
        SLIM_LOG_NOT_CREATED(ccx, identityObj, "PreCreate hook refused");

        return JS_FALSE;
    }

    if(parent != plannedParent)
    {
        XPCWrappedNativeScope *newXpcScope =
            XPCWrappedNativeScope::FindInJSObjectScope(ccx, parent);
        if(newXpcScope != xpcScope)
        {
            SLIM_LOG_NOT_CREATED(ccx, identityObj, "crossing origins");

            return JS_FALSE;
        }
    }

    
    
    JSObject* wrapper = cache->GetWrapper();
    if(wrapper)
    {
        *rval = OBJECT_TO_JSVAL(wrapper);

        return JS_TRUE;
    }

    AutoMarkingWrappedNativeProtoPtr xpcproto(ccx);
    JSBool isGlobal = JS_FALSE;
    xpcproto = XPCWrappedNativeProto::GetNewOrUsed(ccx, xpcScope, classInfo,
                                                   &sciProto, JS_FALSE,
                                                   isGlobal);
    if(!xpcproto)
        return JS_FALSE;

    xpcproto->CacheOffsets(identityObj);

    XPCNativeScriptableInfo* si = xpcproto->GetScriptableInfo();
    JSClass* jsclazz = si->GetSlimJSClass();
    if(!jsclazz)
        return JS_FALSE;

    wrapper = xpc_NewSystemInheritingJSObject(ccx, jsclazz,
                                              xpcproto->GetJSProtoObject(),
                                              parent);
    if(!wrapper ||
       !JS_SetPrivate(ccx, wrapper, identityObj) ||
       !JS_SetReservedSlot(ccx, wrapper, 0, PRIVATE_TO_JSVAL(xpcproto.get())))
        return JS_FALSE;

    
    identityObj.forget();

    cache->SetWrapper(wrapper);

    SLIM_LOG(("+++++ %i created slim wrapper (%p, %p, %p)\n", ++sSlimWrappers,
              wrapper, p, xpcScope));

    *rval = OBJECT_TO_JSVAL(wrapper);

    return JS_TRUE;
}
