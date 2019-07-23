









































#include "xpcprivate.h"
#ifdef DEBUG
#include "XPCNativeWrapper.h"
#endif



#ifdef XPC_TRACK_SCOPE_STATS
static int DEBUG_TotalScopeCount;
static int DEBUG_TotalLiveScopeCount;
static int DEBUG_TotalMaxScopeCount;
static int DEBUG_TotalScopeTraversalCount;
static PRBool  DEBUG_DumpedStats;
#endif

#ifdef DEBUG
static void DEBUG_TrackNewScope(XPCWrappedNativeScope* scope)
{
#ifdef XPC_TRACK_SCOPE_STATS
    DEBUG_TotalScopeCount++;
    DEBUG_TotalLiveScopeCount++;
    if(DEBUG_TotalMaxScopeCount < DEBUG_TotalLiveScopeCount)
        DEBUG_TotalMaxScopeCount = DEBUG_TotalLiveScopeCount;
#endif
}

static void DEBUG_TrackDeleteScope(XPCWrappedNativeScope* scope)
{
#ifdef XPC_TRACK_SCOPE_STATS
    DEBUG_TotalLiveScopeCount--;
#endif
}

static void DEBUG_TrackScopeTraversal()
{
#ifdef XPC_TRACK_SCOPE_STATS
    DEBUG_TotalScopeTraversalCount++;
#endif
}

static void DEBUG_TrackScopeShutdown()
{
#ifdef XPC_TRACK_SCOPE_STATS
    if(!DEBUG_DumpedStats)
    {
        DEBUG_DumpedStats = PR_TRUE;
        printf("%d XPCWrappedNativeScope(s) were constructed.\n",
               DEBUG_TotalScopeCount);

        printf("%d XPCWrappedNativeScopes(s) max alive at one time.\n",
               DEBUG_TotalMaxScopeCount);

        printf("%d XPCWrappedNativeScope(s) alive now.\n" ,
               DEBUG_TotalLiveScopeCount);

        printf("%d traversals of Scope list.\n",
               DEBUG_TotalScopeTraversalCount);
    }
#endif
}
#else
#define DEBUG_TrackNewScope(scope) ((void)0)
#define DEBUG_TrackDeleteScope(scope) ((void)0)
#define DEBUG_TrackScopeTraversal() ((void)0)
#define DEBUG_TrackScopeShutdown() ((void)0)
#endif



XPCWrappedNativeScope* XPCWrappedNativeScope::gScopes = nsnull;
XPCWrappedNativeScope* XPCWrappedNativeScope::gDyingScopes = nsnull;


XPCWrappedNativeScope*
XPCWrappedNativeScope::GetNewOrUsed(XPCCallContext& ccx, JSObject* aGlobal)
{

    XPCWrappedNativeScope* scope = FindInJSObjectScope(ccx, aGlobal, JS_TRUE);
    if(!scope)
        scope = new XPCWrappedNativeScope(ccx, aGlobal);
    else
    {
        
        
        
        
        
        
        scope->SetGlobal(ccx, aGlobal);
    }
    return scope;
}

XPCWrappedNativeScope::XPCWrappedNativeScope(XPCCallContext& ccx,
                                             JSObject* aGlobal)
    :   mRuntime(ccx.GetRuntime()),
        mWrappedNativeMap(Native2WrappedNativeMap::newMap(XPC_NATIVE_MAP_SIZE)),
        mWrappedNativeProtoMap(ClassInfo2WrappedNativeProtoMap::newMap(XPC_NATIVE_PROTO_MAP_SIZE)),
        mMainThreadWrappedNativeProtoMap(ClassInfo2WrappedNativeProtoMap::newMap(XPC_NATIVE_PROTO_MAP_SIZE)),
        mWrapperMap(WrappedNative2WrapperMap::newMap(XPC_WRAPPER_MAP_SIZE)),
        mComponents(nsnull),
        mNext(nsnull),
        mGlobalJSObject(nsnull),
        mPrototypeJSObject(nsnull),
        mPrototypeJSFunction(nsnull),
        mPrototypeNoHelper(nsnull)
#ifndef XPCONNECT_STANDALONE
        , mScriptObjectPrincipal(nsnull)
#endif
{
    
    {   
        XPCAutoLock lock(mRuntime->GetMapLock());

#ifdef DEBUG
        for(XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
            NS_ASSERTION(aGlobal != cur->GetGlobalJSObject(), "dup object");
#endif

        mNext = gScopes;
        gScopes = this;

        
        mContext = XPCContext::GetXPCContext(ccx.GetJSContext());
        mContext->AddScope(this);
    }

    if(aGlobal)
        SetGlobal(ccx, aGlobal);

    DEBUG_TrackNewScope(this);
    MOZ_COUNT_CTOR(XPCWrappedNativeScope);
}


JSBool
XPCWrappedNativeScope::IsDyingScope(XPCWrappedNativeScope *scope)
{
    for(XPCWrappedNativeScope *cur = gDyingScopes; cur; cur = cur->mNext)
    {
        if(scope == cur)
            return JS_TRUE;
    }
    return JS_FALSE;
}

void
XPCWrappedNativeScope::SetComponents(nsXPCComponents* aComponents)
{
    NS_IF_ADDREF(aComponents);
    NS_IF_RELEASE(mComponents);
    mComponents = aComponents;
}










JSClass XPC_WN_NoHelper_Proto_JSClass = {
    "XPC_WN_NoHelper_Proto_JSClass",
    WRAPPER_SLOTS,                  

    
    JS_PropertyStub,                
    JS_PropertyStub,                
    JS_PropertyStub,                
    JS_PropertyStub,                
    JS_EnumerateStub,               
    JS_ResolveStub,                 
    JS_ConvertStub,                 
    nsnull,                         

    
    XPC_WN_Proto_GetObjectOps,      
    nsnull,                         
    nsnull,                         
    nsnull,                         
    nsnull,                         
    nsnull,                         
    nsnull,                         
    nsnull                          
};


void
XPCWrappedNativeScope::SetGlobal(XPCCallContext& ccx, JSObject* aGlobal)
{
    
    

    mGlobalJSObject = aGlobal;
#ifndef XPCONNECT_STANDALONE
    mScriptObjectPrincipal = nsnull;
    

    const JSClass* jsClass = STOBJ_GET_CLASS(aGlobal);
    if(!(~jsClass->flags & (JSCLASS_HAS_PRIVATE |
                            JSCLASS_PRIVATE_IS_NSISUPPORTS)))
    {
        
        
        nsISupports* priv = (nsISupports*)xpc_GetJSPrivate(aGlobal);
        nsCOMPtr<nsIXPConnectWrappedNative> native =
            do_QueryInterface(priv);
        nsCOMPtr<nsIScriptObjectPrincipal> sop;
        if(native)
        {
            sop = do_QueryWrappedNative(native);
        }
        if(!sop)
        {
            sop = do_QueryInterface(priv);
        }
        mScriptObjectPrincipal = sop;
    }
#endif

    
    {
        AutoJSErrorAndExceptionEater eater(ccx); 

        jsval val;
        jsid idObj = mRuntime->GetStringID(XPCJSRuntime::IDX_OBJECT);
        jsid idFun = mRuntime->GetStringID(XPCJSRuntime::IDX_FUNCTION);
        jsid idProto = mRuntime->GetStringID(XPCJSRuntime::IDX_PROTOTYPE);

        if(JS_GetPropertyById(ccx, aGlobal, idObj, &val) &&
           !JSVAL_IS_PRIMITIVE(val) &&
           JS_GetPropertyById(ccx, JSVAL_TO_OBJECT(val), idProto, &val) &&
           !JSVAL_IS_PRIMITIVE(val))
        {
            mPrototypeJSObject = JSVAL_TO_OBJECT(val);
        }
        else
        {
            NS_ERROR("Can't get globalObject.Object.prototype");
        }

        if(JS_GetPropertyById(ccx, aGlobal, idFun, &val) &&
           !JSVAL_IS_PRIMITIVE(val) &&
           JS_GetPropertyById(ccx, JSVAL_TO_OBJECT(val), idProto, &val) &&
           !JSVAL_IS_PRIMITIVE(val))
        {
            mPrototypeJSFunction = JSVAL_TO_OBJECT(val);
        }
        else
        {
            NS_ERROR("Can't get globalObject.Function.prototype");
        }
    }

    
    
    mPrototypeNoHelper = nsnull;
}

XPCWrappedNativeScope::~XPCWrappedNativeScope()
{
    MOZ_COUNT_DTOR(XPCWrappedNativeScope);
    DEBUG_TrackDeleteScope(this);

    

    if(mWrappedNativeMap)
    {
        NS_ASSERTION(0 == mWrappedNativeMap->Count(), "scope has non-empty map");
        delete mWrappedNativeMap;
    }

    if(mWrappedNativeProtoMap)
    {
        NS_ASSERTION(0 == mWrappedNativeProtoMap->Count(), "scope has non-empty map");
        delete mWrappedNativeProtoMap;
    }

    if(mMainThreadWrappedNativeProtoMap)
    {
        NS_ASSERTION(0 == mMainThreadWrappedNativeProtoMap->Count(), "scope has non-empty map");
        delete mMainThreadWrappedNativeProtoMap;
    }

    if(mWrapperMap)
    {
        NS_ASSERTION(0 == mWrapperMap->Count(), "scope has non-empty map");
        delete mWrapperMap;
    }

    if(mContext)
        mContext->RemoveScope(this);

    
    
    NS_IF_RELEASE(mComponents);
}

JSObject *
XPCWrappedNativeScope::GetPrototypeNoHelper(XPCCallContext& ccx)
{
    
    
    
    if(!mPrototypeNoHelper)
    {
        mPrototypeNoHelper =
            xpc_NewSystemInheritingJSObject(ccx, &XPC_WN_NoHelper_Proto_JSClass,
                                            mPrototypeJSObject,
                                            mGlobalJSObject);

        NS_ASSERTION(mPrototypeNoHelper,
                     "Failed to create prototype for wrappers w/o a helper");
    }

    return mPrototypeNoHelper;
}

static JSDHashOperator
WrappedNativeJSGCThingTracer(JSDHashTable *table, JSDHashEntryHdr *hdr,
                             uint32 number, void *arg)
{
    XPCWrappedNative* wrapper = ((Native2WrappedNativeMap::Entry*)hdr)->value;
    if(wrapper->HasExternalReference() && !wrapper->IsWrapperExpired())
    {
        JSTracer* trc = (JSTracer *)arg;
        JS_CALL_OBJECT_TRACER(trc, wrapper->GetFlatJSObject(),
                              "XPCWrappedNative::mFlatJSObject");
    }

    return JS_DHASH_NEXT;
}


void
XPCWrappedNativeScope::TraceJS(JSTracer* trc, XPCJSRuntime* rt)
{
    
    
    XPCAutoLock lock(rt->GetMapLock());

    
    for(XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
    {
        cur->mWrappedNativeMap->Enumerate(WrappedNativeJSGCThingTracer, trc);
    }
}

struct SuspectClosure
{
    SuspectClosure(JSContext *aCx, nsCycleCollectionTraversalCallback& aCb)
        : cx(aCx), cb(aCb)
    {
    }

    JSContext* cx;
    nsCycleCollectionTraversalCallback& cb;
};

static JSDHashOperator
WrappedNativeSuspecter(JSDHashTable *table, JSDHashEntryHdr *hdr,
                       uint32 number, void *arg)
{
    SuspectClosure* closure = static_cast<SuspectClosure*>(arg);
    XPCWrappedNative* wrapper = ((Native2WrappedNativeMap::Entry*)hdr)->value;
    if(wrapper->IsValid())
    {
        NS_ASSERTION(NS_IsMainThread(), 
                     "Suspecting wrapped natives from non-main thread");

        
        
        if(!(closure->cb.WantAllTraces()) && 
           !JS_IsAboutToBeFinalized(closure->cx, wrapper->GetFlatJSObject()))
            return JS_DHASH_NEXT;

        closure->cb.NoteRoot(nsIProgrammingLanguage::JAVASCRIPT,
                             wrapper->GetFlatJSObject(),
                             nsXPConnect::GetXPConnect());
    }

    return JS_DHASH_NEXT;
}


void
XPCWrappedNativeScope::SuspectAllWrappers(XPCJSRuntime* rt, JSContext* cx,
                                          nsCycleCollectionTraversalCallback& cb)
{
    XPCAutoLock lock(rt->GetMapLock());

    SuspectClosure closure(cx, cb);
    for(XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
    {
        cur->mWrappedNativeMap->Enumerate(WrappedNativeSuspecter, &closure);
    }
}


void
XPCWrappedNativeScope::FinishedMarkPhaseOfGC(JSContext* cx, XPCJSRuntime* rt)
{
    
    
    XPCAutoLock lock(rt->GetMapLock());

    
    
    
    NS_ASSERTION(gDyingScopes == nsnull,
                 "JSGC_MARK_END without JSGC_FINALIZE_END");

    XPCWrappedNativeScope* prev = nsnull;
    XPCWrappedNativeScope* cur = gScopes;

    while(cur)
    {
        XPCWrappedNativeScope* next = cur->mNext;
        if(cur->mGlobalJSObject &&
           JS_IsAboutToBeFinalized(cx, cur->mGlobalJSObject))
        {
            cur->mGlobalJSObject = nsnull;
#ifndef XPCONNECT_STANDALONE
            cur->mScriptObjectPrincipal = nsnull;
#endif
            
            if(prev)
                prev->mNext = next;
            else
                gScopes = next;
            cur->mNext = gDyingScopes;
            gDyingScopes = cur;
            cur = nsnull;
        }
        else
        {
            if(cur->mPrototypeJSObject &&
               JS_IsAboutToBeFinalized(cx, cur->mPrototypeJSObject))
            {
                cur->mPrototypeJSObject = nsnull;
            }
            if(cur->mPrototypeJSFunction &&
               JS_IsAboutToBeFinalized(cx, cur->mPrototypeJSFunction))
            {
                cur->mPrototypeJSFunction = nsnull;
            }
            if(cur->mPrototypeNoHelper &&
               JS_IsAboutToBeFinalized(cx, cur->mPrototypeNoHelper))
            {
                cur->mPrototypeNoHelper = nsnull;
            }
        }
        if(cur)
            prev = cur;
        cur = next;
    }
}


void
XPCWrappedNativeScope::FinishedFinalizationPhaseOfGC(JSContext* cx)
{
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();

    
    
    
    XPCAutoLock lock(rt->GetMapLock());
    KillDyingScopes();
}

static JSDHashOperator
WrappedNativeMarker(JSDHashTable *table, JSDHashEntryHdr *hdr,
                    uint32 number, void *arg)
{
    ((Native2WrappedNativeMap::Entry*)hdr)->value->Mark();
    return JS_DHASH_NEXT;
}



static JSDHashOperator
WrappedNativeProtoMarker(JSDHashTable *table, JSDHashEntryHdr *hdr,
                         uint32 number, void *arg)
{
    ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value->Mark();
    return JS_DHASH_NEXT;
}


void
XPCWrappedNativeScope::MarkAllWrappedNativesAndProtos()
{
    for(XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
    {
        cur->mWrappedNativeMap->Enumerate(WrappedNativeMarker, nsnull);
        cur->mWrappedNativeProtoMap->Enumerate(WrappedNativeProtoMarker, nsnull);
        cur->mMainThreadWrappedNativeProtoMap->Enumerate(WrappedNativeProtoMarker, nsnull);
    }

    DEBUG_TrackScopeTraversal();
}

#ifdef DEBUG
static JSDHashOperator
ASSERT_WrappedNativeSetNotMarked(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                 uint32 number, void *arg)
{
    ((Native2WrappedNativeMap::Entry*)hdr)->value->ASSERT_SetsNotMarked();
    return JS_DHASH_NEXT;
}

static JSDHashOperator
ASSERT_WrappedNativeProtoSetNotMarked(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                      uint32 number, void *arg)
{
    ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value->ASSERT_SetNotMarked();
    return JS_DHASH_NEXT;
}


void
XPCWrappedNativeScope::ASSERT_NoInterfaceSetsAreMarked()
{
    for(XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
    {
        cur->mWrappedNativeMap->Enumerate(
            ASSERT_WrappedNativeSetNotMarked, nsnull);
        cur->mWrappedNativeProtoMap->Enumerate(
            ASSERT_WrappedNativeProtoSetNotMarked, nsnull);
        cur->mMainThreadWrappedNativeProtoMap->Enumerate(
            ASSERT_WrappedNativeProtoSetNotMarked, nsnull);
    }
}
#endif

static JSDHashOperator
WrappedNativeTearoffSweeper(JSDHashTable *table, JSDHashEntryHdr *hdr,
                            uint32 number, void *arg)
{
    ((Native2WrappedNativeMap::Entry*)hdr)->value->SweepTearOffs();
    return JS_DHASH_NEXT;
}


void
XPCWrappedNativeScope::SweepAllWrappedNativeTearOffs()
{
    for(XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
        cur->mWrappedNativeMap->Enumerate(WrappedNativeTearoffSweeper, nsnull);

    DEBUG_TrackScopeTraversal();
}


void
XPCWrappedNativeScope::KillDyingScopes()
{
    
    XPCWrappedNativeScope* cur = gDyingScopes;
    while(cur)
    {
        XPCWrappedNativeScope* next = cur->mNext;
        delete cur;
        cur = next;
    }
    gDyingScopes = nsnull;
}

struct ShutdownData
{
    ShutdownData(JSContext* acx)
        : cx(acx), wrapperCount(0),
          sharedProtoCount(0), nonSharedProtoCount(0) {}
    JSContext* cx;
    int wrapperCount;
    int sharedProtoCount;
    int nonSharedProtoCount;
};

static JSDHashOperator
WrappedNativeShutdownEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                uint32 number, void *arg)
{
    ShutdownData* data = (ShutdownData*) arg;
    XPCWrappedNative* wrapper = ((Native2WrappedNativeMap::Entry*)hdr)->value;

    if(wrapper->IsValid())
    {
        if(wrapper->HasProto() && !wrapper->HasSharedProto())
            data->nonSharedProtoCount++;
        wrapper->SystemIsBeingShutDown(data->cx);
        data->wrapperCount++;
    }
    return JS_DHASH_REMOVE;
}

static JSDHashOperator
WrappedNativeProtoShutdownEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                     uint32 number, void *arg)
{
    ShutdownData* data = (ShutdownData*) arg;
    ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value->
        SystemIsBeingShutDown(data->cx);
    data->sharedProtoCount++;
    return JS_DHASH_REMOVE;
}


void
XPCWrappedNativeScope::SystemIsBeingShutDown(JSContext* cx)
{
    DEBUG_TrackScopeTraversal();
    DEBUG_TrackScopeShutdown();

    int liveScopeCount = 0;

    ShutdownData data(cx);

    XPCWrappedNativeScope* cur;

    

    cur = gScopes;
    while(cur)
    {
        XPCWrappedNativeScope* next = cur->mNext;
        cur->mNext = gDyingScopes;
        gDyingScopes = cur;
        cur = next;
        liveScopeCount++;
    }
    gScopes = nsnull;

    

    for(cur = gDyingScopes; cur; cur = cur->mNext)
    {
        
        if(cur->mComponents)
            cur->mComponents->SystemIsBeingShutDown();

        
        
        cur->mWrappedNativeProtoMap->
                Enumerate(WrappedNativeProtoShutdownEnumerator,  &data);
        cur->mMainThreadWrappedNativeProtoMap->
                Enumerate(WrappedNativeProtoShutdownEnumerator,  &data);
        cur->mWrappedNativeMap->
                Enumerate(WrappedNativeShutdownEnumerator,  &data);
    }

    
    KillDyingScopes();

#ifdef XPC_DUMP_AT_SHUTDOWN
    if(data.wrapperCount)
        printf("deleting nsXPConnect  with %d live XPCWrappedNatives\n",
               data.wrapperCount);
    if(data.sharedProtoCount + data.nonSharedProtoCount)
        printf("deleting nsXPConnect  with %d live XPCWrappedNativeProtos (%d shared)\n",
               data.sharedProtoCount + data.nonSharedProtoCount,
               data.sharedProtoCount);
    if(liveScopeCount)
        printf("deleting nsXPConnect  with %d live XPCWrappedNativeScopes\n",
               liveScopeCount);
#endif
}




static
XPCWrappedNativeScope*
GetScopeOfObject(JSObject* obj)
{
    nsISupports* supports;
    JSClass* clazz = STOBJ_GET_CLASS(obj);
    JSBool isWrapper = IS_WRAPPER_CLASS(clazz);

    if(isWrapper && IS_SLIM_WRAPPER_OBJECT(obj))
        return GetSlimWrapperProto(obj)->GetScope();

    if(!isWrapper || !(supports = (nsISupports*) xpc_GetJSPrivate(obj)))
    {
#ifdef DEBUG
        {
            if(!(~clazz->flags & (JSCLASS_HAS_PRIVATE |
                                  JSCLASS_PRIVATE_IS_NSISUPPORTS)) &&
               (supports = (nsISupports*) xpc_GetJSPrivate(obj)) &&
               !XPCNativeWrapper::IsNativeWrapperClass(clazz))
            {
                nsCOMPtr<nsIXPConnectWrappedNative> iface =
                    do_QueryInterface(supports);

                NS_ASSERTION(!iface, "Uh, how'd this happen?");
            }
        }
#endif

        return nsnull;
    }

#ifdef DEBUG
    {
        nsCOMPtr<nsIXPConnectWrappedNative> iface = do_QueryInterface(supports);

        NS_ASSERTION(iface, "Uh, how'd this happen?");
    }
#endif

    
    return ((XPCWrappedNative*)supports)->GetScope();
}


#ifdef DEBUG
void DEBUG_CheckForComponentsInScope(JSContext* cx, JSObject* obj,
                                     JSBool OKIfNotInitialized,
                                     XPCJSRuntime* runtime)
{
    if(OKIfNotInitialized)
        return;

    if(!(JS_GetOptions(cx) & JSOPTION_PRIVATE_IS_NSISUPPORTS))
        return;

    const char* name = runtime->GetStringName(XPCJSRuntime::IDX_COMPONENTS);
    jsval prop;
    if(JS_LookupProperty(cx, obj, name, &prop) && !JSVAL_IS_PRIMITIVE(prop))
        return;

    
    
    
    
    
    
#ifdef I_FOOLISHLY_WANT_TO_IGNORE_THIS_LIKE_THE_OTHER_CRAP_WE_PRINTF
    NS_WARNING("XPConnect is being called on a scope without a 'Components' property!");
#else
    NS_ERROR("XPConnect is being called on a scope without a 'Components' property!");
#endif
}
#else
#define DEBUG_CheckForComponentsInScope(ccx, obj, OKIfNotInitialized, runtime) \
    ((void)0)
#endif


XPCWrappedNativeScope*
XPCWrappedNativeScope::FindInJSObjectScope(JSContext* cx, JSObject* obj,
                                           JSBool OKIfNotInitialized,
                                           XPCJSRuntime* runtime)
{
    XPCWrappedNativeScope* scope;

    if(!obj)
        return nsnull;

    
    

    scope = GetScopeOfObject(obj);
    if(scope)
        return scope;

    

    obj = JS_GetGlobalForObject(cx, obj);

    if(!runtime)
    {
        runtime = nsXPConnect::GetRuntimeInstance();
        NS_ASSERTION(runtime, "This should never be null!");
    }

    
    
    XPCWrappedNativeScope* found = nsnull;
    {   
        XPCAutoLock lock(runtime->GetMapLock());

        DEBUG_TrackScopeTraversal();

        for(XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
        {
            if(obj == cur->GetGlobalJSObject())
            {
                found = cur;
                break;
            }
        }
    }

    if(found) {
        
        DEBUG_CheckForComponentsInScope(cx, obj, OKIfNotInitialized, runtime);
        return found;
    }

    
    
    
    NS_ASSERTION(OKIfNotInitialized, "No scope has this global object!");
    return nsnull;
}



static JSDHashOperator
WNProtoSecPolicyClearer(JSDHashTable *table, JSDHashEntryHdr *hdr,
                        uint32 number, void *arg)
{
    XPCWrappedNativeProto* proto =
        ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value;
    *(proto->GetSecurityInfoAddr()) = nsnull;
    return JS_DHASH_NEXT;
}

static JSDHashOperator
WNSecPolicyClearer(JSDHashTable *table, JSDHashEntryHdr *hdr,
                    uint32 number, void *arg)
{
    XPCWrappedNative* wrapper = ((Native2WrappedNativeMap::Entry*)hdr)->value;
    if(wrapper->HasProto() && !wrapper->HasSharedProto())
        *(wrapper->GetProto()->GetSecurityInfoAddr()) = nsnull;
    return JS_DHASH_NEXT;
}


nsresult
XPCWrappedNativeScope::ClearAllWrappedNativeSecurityPolicies(XPCCallContext& ccx)
{
    
    XPCAutoLock lock(ccx.GetRuntime()->GetMapLock());

    for(XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
    {
        cur->mWrappedNativeProtoMap->Enumerate(WNProtoSecPolicyClearer, nsnull);
        cur->mMainThreadWrappedNativeProtoMap->Enumerate(WNProtoSecPolicyClearer, nsnull);
        cur->mWrappedNativeMap->Enumerate(WNSecPolicyClearer, nsnull);
    }

    DEBUG_TrackScopeTraversal();

    return NS_OK;
}

static JSDHashOperator
WNProtoRemover(JSDHashTable *table, JSDHashEntryHdr *hdr,
               uint32 number, void *arg)
{
    XPCWrappedNativeProtoMap* detachedMap = (XPCWrappedNativeProtoMap*)arg;
    
    XPCWrappedNativeProto* proto = (XPCWrappedNativeProto*)
        ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value;

    detachedMap->Add(proto);

    return JS_DHASH_REMOVE;
}

void
XPCWrappedNativeScope::RemoveWrappedNativeProtos()
{
    XPCAutoLock al(mRuntime->GetMapLock());
    
    mWrappedNativeProtoMap->Enumerate(WNProtoRemover, 
        GetRuntime()->GetDetachedWrappedNativeProtoMap());
    mMainThreadWrappedNativeProtoMap->Enumerate(WNProtoRemover, 
        GetRuntime()->GetDetachedWrappedNativeProtoMap());
}




void
XPCWrappedNativeScope::DebugDumpAllScopes(PRInt16 depth)
{
#ifdef DEBUG
    depth-- ;

    
    int count = 0;
    XPCWrappedNativeScope* cur;
    for(cur = gScopes; cur; cur = cur->mNext)
        count++ ;

    XPC_LOG_ALWAYS(("chain of %d XPCWrappedNativeScope(s)", count));
    XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("gDyingScopes @ %x", gDyingScopes));
        if(depth)
            for(cur = gScopes; cur; cur = cur->mNext)
                cur->DebugDump(depth);
    XPC_LOG_OUTDENT();
#endif
}

#ifdef DEBUG
static JSDHashOperator
WrappedNativeMapDumpEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                               uint32 number, void *arg)
{
    ((Native2WrappedNativeMap::Entry*)hdr)->value->DebugDump(*(PRInt16*)arg);
    return JS_DHASH_NEXT;
}
static JSDHashOperator
WrappedNativeProtoMapDumpEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                    uint32 number, void *arg)
{
    ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value->DebugDump(*(PRInt16*)arg);
    return JS_DHASH_NEXT;
}
#endif

void
XPCWrappedNativeScope::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth-- ;
    XPC_LOG_ALWAYS(("XPCWrappedNativeScope @ %x", this));
    XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("mRuntime @ %x", mRuntime));
        XPC_LOG_ALWAYS(("mNext @ %x", mNext));
        XPC_LOG_ALWAYS(("mComponents @ %x", mComponents));
        XPC_LOG_ALWAYS(("mGlobalJSObject @ %x", mGlobalJSObject));
        XPC_LOG_ALWAYS(("mPrototypeJSObject @ %x", mPrototypeJSObject));
        XPC_LOG_ALWAYS(("mPrototypeJSFunction @ %x", mPrototypeJSFunction));
        XPC_LOG_ALWAYS(("mPrototypeNoHelper @ %x", mPrototypeNoHelper));

        XPC_LOG_ALWAYS(("mWrappedNativeMap @ %x with %d wrappers(s)", \
                         mWrappedNativeMap, \
                         mWrappedNativeMap ? mWrappedNativeMap->Count() : 0));
        
        if(depth && mWrappedNativeMap && mWrappedNativeMap->Count())
        {
            XPC_LOG_INDENT();
            mWrappedNativeMap->Enumerate(WrappedNativeMapDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }

        XPC_LOG_ALWAYS(("mWrappedNativeProtoMap @ %x with %d protos(s)", \
                         mWrappedNativeProtoMap, \
                         mWrappedNativeProtoMap ? mWrappedNativeProtoMap->Count() : 0));
        
        if(depth && mWrappedNativeProtoMap && mWrappedNativeProtoMap->Count())
        {
            XPC_LOG_INDENT();
            mWrappedNativeProtoMap->Enumerate(WrappedNativeProtoMapDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }

        XPC_LOG_ALWAYS(("mMainThreadWrappedNativeProtoMap @ %x with %d protos(s)", \
                         mMainThreadWrappedNativeProtoMap, \
                         mMainThreadWrappedNativeProtoMap ? mMainThreadWrappedNativeProtoMap->Count() : 0));
        
        if(depth && mMainThreadWrappedNativeProtoMap && mMainThreadWrappedNativeProtoMap->Count())
        {
            XPC_LOG_INDENT();
            mMainThreadWrappedNativeProtoMap->Enumerate(WrappedNativeProtoMapDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }
    XPC_LOG_OUTDENT();
#endif
}
