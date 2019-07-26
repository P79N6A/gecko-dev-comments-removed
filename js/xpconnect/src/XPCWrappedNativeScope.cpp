






#include "xpcprivate.h"
#include "XPCWrapper.h"
#include "jsproxy.h"

#include "mozilla/dom/BindingUtils.h"

using namespace mozilla;



#ifdef XPC_TRACK_SCOPE_STATS
static int DEBUG_TotalScopeCount;
static int DEBUG_TotalLiveScopeCount;
static int DEBUG_TotalMaxScopeCount;
static int DEBUG_TotalScopeTraversalCount;
static bool    DEBUG_DumpedStats;
#endif

#ifdef DEBUG
static void DEBUG_TrackNewScope(XPCWrappedNativeScope* scope)
{
#ifdef XPC_TRACK_SCOPE_STATS
    DEBUG_TotalScopeCount++;
    DEBUG_TotalLiveScopeCount++;
    if (DEBUG_TotalMaxScopeCount < DEBUG_TotalLiveScopeCount)
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
    if (!DEBUG_DumpedStats) {
        DEBUG_DumpedStats = true;
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



XPCWrappedNativeScope* XPCWrappedNativeScope::gScopes = nullptr;
XPCWrappedNativeScope* XPCWrappedNativeScope::gDyingScopes = nullptr;


XPCWrappedNativeScope*
XPCWrappedNativeScope::GetNewOrUsed(XPCCallContext& ccx, JSObject* aGlobal, nsISupports* aNative)
{

    XPCWrappedNativeScope* scope = FindInJSObjectScope(ccx, aGlobal, true);
    if (!scope)
        scope = new XPCWrappedNativeScope(ccx, aGlobal, aNative);
    else {
        
        
        
        
        
        scope->SetGlobal(ccx, aGlobal, aNative);
    }
    if (js::GetObjectClass(aGlobal)->flags & JSCLASS_XPCONNECT_GLOBAL)
        JS_SetReservedSlot(aGlobal,
                           JSCLASS_GLOBAL_SLOT_COUNT,
                           PRIVATE_TO_JSVAL(scope));
    return scope;
}

XPCWrappedNativeScope::XPCWrappedNativeScope(XPCCallContext& ccx,
                                             JSObject* aGlobal,
                                             nsISupports* aNative)
    :   mRuntime(ccx.GetRuntime()),
        mWrappedNativeMap(Native2WrappedNativeMap::newMap(XPC_NATIVE_MAP_SIZE)),
        mWrappedNativeProtoMap(ClassInfo2WrappedNativeProtoMap::newMap(XPC_NATIVE_PROTO_MAP_SIZE)),
        mMainThreadWrappedNativeProtoMap(ClassInfo2WrappedNativeProtoMap::newMap(XPC_NATIVE_PROTO_MAP_SIZE)),
        mComponents(nullptr),
        mNext(nullptr),
        mGlobalJSObject(nullptr),
        mPrototypeJSObject(nullptr),
        mPrototypeNoHelper(nullptr),
        mScriptObjectPrincipal(nullptr),
        mNewDOMBindingsEnabled(ccx.GetRuntime()->NewDOMBindingsEnabled()),
        mExperimentalBindingsEnabled(ccx.GetRuntime()->ExperimentalBindingsEnabled())
{
    
    {   
        XPCAutoLock lock(mRuntime->GetMapLock());

#ifdef DEBUG
        for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
            MOZ_ASSERT(aGlobal != cur->GetGlobalJSObjectPreserveColor(), "dup object");
#endif

        mNext = gScopes;
        gScopes = this;

        
        mContext = XPCContext::GetXPCContext(ccx.GetJSContext());
        mContext->AddScope(this);
    }

    if (aGlobal)
        SetGlobal(ccx, aGlobal, aNative);

    DEBUG_TrackNewScope(this);
    MOZ_COUNT_CTOR(XPCWrappedNativeScope);
}


JSBool
XPCWrappedNativeScope::IsDyingScope(XPCWrappedNativeScope *scope)
{
    for (XPCWrappedNativeScope *cur = gDyingScopes; cur; cur = cur->mNext) {
        if (scope == cur)
            return true;
    }
    return false;
}

void
XPCWrappedNativeScope::SetComponents(nsXPCComponents* aComponents)
{
    mComponents = aComponents;
}

nsXPCComponents*
XPCWrappedNativeScope::GetComponents()
{
    return mComponents;
}










js::Class XPC_WN_NoHelper_Proto_JSClass = {
    "XPC_WN_NoHelper_Proto_JSClass",
    WRAPPER_SLOTS,                  

    
    JS_PropertyStub,                
    JS_PropertyStub,                
    JS_PropertyStub,                
    JS_StrictPropertyStub,          
    JS_EnumerateStub,               
    JS_ResolveStub,                 
    JS_ConvertStub,                 
    nullptr,                         

    
    nullptr,                         
    nullptr,                         
    nullptr,                         
    nullptr,                         
    nullptr,                         

    JS_NULL_CLASS_EXT,
    XPC_WN_NoCall_ObjectOps
};


void
XPCWrappedNativeScope::SetGlobal(XPCCallContext& ccx, JSObject* aGlobal,
                                 nsISupports* aNative)
{
    
    

    mGlobalJSObject = aGlobal;
    mScriptObjectPrincipal = nullptr;

    
    
    nsISupports *native;
    if (aNative) {
        native = aNative;
    } else {
        const JSClass *jsClass = js::GetObjectJSClass(aGlobal);
        nsISupports *priv;
        if (!(~jsClass->flags & (JSCLASS_HAS_PRIVATE |
                                 JSCLASS_PRIVATE_IS_NSISUPPORTS))) {
            
            
            priv = static_cast<nsISupports*>(xpc_GetJSPrivate(aGlobal));
        } else if (dom::IsDOMClass(jsClass) &&
                   dom::DOMJSClass::FromJSClass(jsClass)->mDOMObjectIsISupports) {
            priv = dom::UnwrapDOMObject<nsISupports>(aGlobal);
        } else {
            priv = nullptr;
        }
        nsCOMPtr<nsIXPConnectWrappedNative> wn = do_QueryInterface(priv);
        if (wn)
            native = static_cast<XPCWrappedNative*>(wn.get())->GetIdentityObject();
        else
            native = priv;
    }

    
    nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(native);
    mScriptObjectPrincipal = sop;

    
    JSObject *objectPrototype =
        JS_GetObjectPrototype(ccx.GetJSContext(), aGlobal);
    if (objectPrototype)
        mPrototypeJSObject = objectPrototype;
    else
        NS_ERROR("Can't get globalObject.Object.prototype");

    
    
    mPrototypeNoHelper = nullptr;
}

XPCWrappedNativeScope::~XPCWrappedNativeScope()
{
    MOZ_COUNT_DTOR(XPCWrappedNativeScope);
    DEBUG_TrackDeleteScope(this);

    

    if (mWrappedNativeMap) {
        NS_ASSERTION(0 == mWrappedNativeMap->Count(), "scope has non-empty map");
        delete mWrappedNativeMap;
    }

    if (mWrappedNativeProtoMap) {
        NS_ASSERTION(0 == mWrappedNativeProtoMap->Count(), "scope has non-empty map");
        delete mWrappedNativeProtoMap;
    }

    if (mMainThreadWrappedNativeProtoMap) {
        NS_ASSERTION(0 == mMainThreadWrappedNativeProtoMap->Count(), "scope has non-empty map");
        delete mMainThreadWrappedNativeProtoMap;
    }

    if (mContext)
        mContext->RemoveScope(this);

    
    
    if (mComponents)
        mComponents->mScope = nullptr;

    
    
    mComponents = nullptr;

    JSRuntime *rt = mRuntime->GetJSRuntime();
    mGlobalJSObject.finalize(rt);
    mPrototypeJSObject.finalize(rt);
}

JSObject *
XPCWrappedNativeScope::GetPrototypeNoHelper(XPCCallContext& ccx)
{
    
    
    
    if (!mPrototypeNoHelper) {
        mPrototypeNoHelper =
            xpc_NewSystemInheritingJSObject(ccx,
                                            js::Jsvalify(&XPC_WN_NoHelper_Proto_JSClass),
                                            mPrototypeJSObject,
                                            false, mGlobalJSObject);

        NS_ASSERTION(mPrototypeNoHelper,
                     "Failed to create prototype for wrappers w/o a helper");
    } else {
        xpc_UnmarkGrayObject(mPrototypeNoHelper);
    }

    return mPrototypeNoHelper;
}

static JSDHashOperator
WrappedNativeJSGCThingTracer(JSDHashTable *table, JSDHashEntryHdr *hdr,
                             uint32_t number, void *arg)
{
    XPCWrappedNative* wrapper = ((Native2WrappedNativeMap::Entry*)hdr)->value;
    if (wrapper->HasExternalReference() && !wrapper->IsWrapperExpired())
        wrapper->TraceSelf((JSTracer *)arg);

    return JS_DHASH_NEXT;
}


void
XPCWrappedNativeScope::TraceWrappedNativesInAllScopes(JSTracer* trc, XPCJSRuntime* rt)
{
    
    
    XPCAutoLock lock(rt->GetMapLock());

    
    for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext) {
        cur->mWrappedNativeMap->Enumerate(WrappedNativeJSGCThingTracer, trc);
    }
}

static JSDHashOperator
WrappedNativeSuspecter(JSDHashTable *table, JSDHashEntryHdr *hdr,
                       uint32_t number, void *arg)
{
    XPCWrappedNative* wrapper = ((Native2WrappedNativeMap::Entry*)hdr)->value;

    if (wrapper->HasExternalReference()) {
        nsCycleCollectionTraversalCallback *cb =
            static_cast<nsCycleCollectionTraversalCallback *>(arg);
        XPCJSRuntime::SuspectWrappedNative(wrapper, *cb);
    }

    return JS_DHASH_NEXT;
}


void
XPCWrappedNativeScope::SuspectAllWrappers(XPCJSRuntime* rt,
                                          nsCycleCollectionTraversalCallback& cb)
{
    XPCAutoLock lock(rt->GetMapLock());

    for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext) {
        cur->mWrappedNativeMap->Enumerate(WrappedNativeSuspecter, &cb);
    }
}


void
XPCWrappedNativeScope::StartFinalizationPhaseOfGC(JSFreeOp *fop, XPCJSRuntime* rt)
{
    
    
    XPCAutoLock lock(rt->GetMapLock());

    
    
    
    NS_ASSERTION(gDyingScopes == nullptr,
                 "JSGC_MARK_END without JSGC_FINALIZE_END");

    XPCWrappedNativeScope* prev = nullptr;
    XPCWrappedNativeScope* cur = gScopes;

    while (cur) {
        XPCWrappedNativeScope* next = cur->mNext;

        if (cur->mGlobalJSObject &&
            JS_IsAboutToBeFinalized(cur->mGlobalJSObject)) {
            cur->mGlobalJSObject.finalize(fop->runtime());
            cur->mScriptObjectPrincipal = nullptr;
            if (cur->GetCachedDOMPrototypes().IsInitialized())
                 cur->GetCachedDOMPrototypes().Clear();
            
            if (prev)
                prev->mNext = next;
            else
                gScopes = next;
            cur->mNext = gDyingScopes;
            gDyingScopes = cur;
            cur = nullptr;
        } else {
            if (cur->mPrototypeJSObject &&
                JS_IsAboutToBeFinalized(cur->mPrototypeJSObject)) {
                cur->mPrototypeJSObject.finalize(fop->runtime());
            }
            if (cur->mPrototypeNoHelper &&
                JS_IsAboutToBeFinalized(cur->mPrototypeNoHelper)) {
                cur->mPrototypeNoHelper = nullptr;
            }
        }
        if (cur)
            prev = cur;
        cur = next;
    }
}


void
XPCWrappedNativeScope::FinishedFinalizationPhaseOfGC()
{
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();

    
    
    
    XPCAutoLock lock(rt->GetMapLock());
    KillDyingScopes();
}

static JSDHashOperator
WrappedNativeMarker(JSDHashTable *table, JSDHashEntryHdr *hdr,
                    uint32_t number_t, void *arg)
{
    ((Native2WrappedNativeMap::Entry*)hdr)->value->Mark();
    return JS_DHASH_NEXT;
}



static JSDHashOperator
WrappedNativeProtoMarker(JSDHashTable *table, JSDHashEntryHdr *hdr,
                         uint32_t number, void *arg)
{
    ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value->Mark();
    return JS_DHASH_NEXT;
}


void
XPCWrappedNativeScope::MarkAllWrappedNativesAndProtos()
{
    for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext) {
        cur->mWrappedNativeMap->Enumerate(WrappedNativeMarker, nullptr);
        cur->mWrappedNativeProtoMap->Enumerate(WrappedNativeProtoMarker, nullptr);
        cur->mMainThreadWrappedNativeProtoMap->Enumerate(WrappedNativeProtoMarker, nullptr);
    }

    DEBUG_TrackScopeTraversal();
}

#ifdef DEBUG
static JSDHashOperator
ASSERT_WrappedNativeSetNotMarked(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                 uint32_t number, void *arg)
{
    ((Native2WrappedNativeMap::Entry*)hdr)->value->ASSERT_SetsNotMarked();
    return JS_DHASH_NEXT;
}

static JSDHashOperator
ASSERT_WrappedNativeProtoSetNotMarked(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                      uint32_t number, void *arg)
{
    ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value->ASSERT_SetNotMarked();
    return JS_DHASH_NEXT;
}


void
XPCWrappedNativeScope::ASSERT_NoInterfaceSetsAreMarked()
{
    for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext) {
        cur->mWrappedNativeMap->Enumerate(ASSERT_WrappedNativeSetNotMarked, nullptr);
        cur->mWrappedNativeProtoMap->Enumerate(ASSERT_WrappedNativeProtoSetNotMarked, nullptr);
        cur->mMainThreadWrappedNativeProtoMap->Enumerate(ASSERT_WrappedNativeProtoSetNotMarked, nullptr);
    }
}
#endif

static JSDHashOperator
WrappedNativeTearoffSweeper(JSDHashTable *table, JSDHashEntryHdr *hdr,
                            uint32_t number, void *arg)
{
    ((Native2WrappedNativeMap::Entry*)hdr)->value->SweepTearOffs();
    return JS_DHASH_NEXT;
}


void
XPCWrappedNativeScope::SweepAllWrappedNativeTearOffs()
{
    for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
        cur->mWrappedNativeMap->Enumerate(WrappedNativeTearoffSweeper, nullptr);

    DEBUG_TrackScopeTraversal();
}


void
XPCWrappedNativeScope::KillDyingScopes()
{
    
    XPCWrappedNativeScope* cur = gDyingScopes;
    while (cur) {
        XPCWrappedNativeScope* next = cur->mNext;
        delete cur;
        cur = next;
    }
    gDyingScopes = nullptr;
}

struct ShutdownData
{
    ShutdownData()
        : wrapperCount(0),
          protoCount(0) {}
    int wrapperCount;
    int protoCount;
};

static JSDHashOperator
WrappedNativeShutdownEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                uint32_t number, void *arg)
{
    ShutdownData* data = (ShutdownData*) arg;
    XPCWrappedNative* wrapper = ((Native2WrappedNativeMap::Entry*)hdr)->value;

    if (wrapper->IsValid()) {
        wrapper->SystemIsBeingShutDown();
        data->wrapperCount++;
    }
    return JS_DHASH_REMOVE;
}

static JSDHashOperator
WrappedNativeProtoShutdownEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                     uint32_t number, void *arg)
{
    ShutdownData* data = (ShutdownData*) arg;
    ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value->
        SystemIsBeingShutDown();
    data->protoCount++;
    return JS_DHASH_REMOVE;
}


void
XPCWrappedNativeScope::SystemIsBeingShutDown()
{
    DEBUG_TrackScopeTraversal();
    DEBUG_TrackScopeShutdown();

    int liveScopeCount = 0;

    ShutdownData data;

    XPCWrappedNativeScope* cur;

    

    cur = gScopes;
    while (cur) {
        XPCWrappedNativeScope* next = cur->mNext;
        cur->mNext = gDyingScopes;
        gDyingScopes = cur;
        cur = next;
        liveScopeCount++;
    }
    gScopes = nullptr;

    
    
    

    for (cur = gDyingScopes; cur; cur = cur->mNext) {
        
        if (cur->mComponents)
            cur->mComponents->SystemIsBeingShutDown();

        
        
        cur->mWrappedNativeProtoMap->
                Enumerate(WrappedNativeProtoShutdownEnumerator,  &data);
        cur->mMainThreadWrappedNativeProtoMap->
                Enumerate(WrappedNativeProtoShutdownEnumerator,  &data);
        cur->mWrappedNativeMap->
                Enumerate(WrappedNativeShutdownEnumerator,  &data);

        
        
        
        
        JSObject *global = cur->mGlobalJSObject;
        if (global &&
            js::GetObjectClass(global)->flags & JSCLASS_XPCONNECT_GLOBAL)
        {
            JS_SetReservedSlot(global, JSCLASS_GLOBAL_SLOT_COUNT,
                               PRIVATE_TO_JSVAL(nullptr));
        }
    }

    
    KillDyingScopes();

#ifdef XPC_DUMP_AT_SHUTDOWN
    if (data.wrapperCount)
        printf("deleting nsXPConnect  with %d live XPCWrappedNatives\n",
               data.wrapperCount);
    if (data.protoCount)
        printf("deleting nsXPConnect  with %d live XPCWrappedNativeProtos\n",
               data.protoCount);
    if (liveScopeCount)
        printf("deleting nsXPConnect  with %d live XPCWrappedNativeScopes\n",
               liveScopeCount);
#endif
}




static
XPCWrappedNativeScope*
GetScopeOfObject(JSObject* obj)
{
    nsISupports* supports;
    js::Class* clazz = js::GetObjectClass(obj);
    JSBool isWrapper = IS_WRAPPER_CLASS(clazz);

    if (isWrapper && IS_SLIM_WRAPPER_OBJECT(obj))
        return GetSlimWrapperProto(obj)->GetScope();

    if (!isWrapper || !(supports = (nsISupports*) xpc_GetJSPrivate(obj)))
        return nullptr;

#ifdef DEBUG
    {
        nsCOMPtr<nsIXPConnectWrappedNative> iface = do_QueryInterface(supports);

        NS_ASSERTION(iface, "Uh, how'd this happen?");
    }
#endif

    
    return ((XPCWrappedNative*)supports)->GetScope();
}


XPCWrappedNativeScope*
XPCWrappedNativeScope::FindInJSObjectScope(JSContext* cx, JSObject* obj,
                                           JSBool OKIfNotInitialized,
                                           XPCJSRuntime* runtime)
{
    XPCWrappedNativeScope* scope;

    if (!obj)
        return nullptr;

    
    

    scope = GetScopeOfObject(obj);
    if (scope)
        return scope;

    

    JSAutoEnterCompartment ac;
    ac.enterAndIgnoreErrors(cx, obj);

    obj = JS_GetGlobalForObject(cx, obj);

    if (js::GetObjectClass(obj)->flags & JSCLASS_XPCONNECT_GLOBAL) {
        scope = XPCWrappedNativeScope::GetNativeScope(obj);
        if (scope)
            return scope;
    }

    if (!runtime) {
        runtime = nsXPConnect::GetRuntimeInstance();
        NS_ASSERTION(runtime, "This should never be null!");
    }

    
    
    XPCWrappedNativeScope* found = nullptr;
    {   
        XPCAutoLock lock(runtime->GetMapLock());

        DEBUG_TrackScopeTraversal();

        for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext) {
            if (obj == cur->GetGlobalJSObjectPreserveColor()) {
                found = cur;
                break;
            }
        }
    }

    if (found) {
        
        return found;
    }

    
    
    
    NS_ASSERTION(OKIfNotInitialized, "No scope has this global object!");
    return nullptr;
}



static JSDHashOperator
WNProtoSecPolicyClearer(JSDHashTable *table, JSDHashEntryHdr *hdr,
                        uint32_t number, void *arg)
{
    XPCWrappedNativeProto* proto =
        ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value;
    *(proto->GetSecurityInfoAddr()) = nullptr;
    return JS_DHASH_NEXT;
}


nsresult
XPCWrappedNativeScope::ClearAllWrappedNativeSecurityPolicies(XPCCallContext& ccx)
{
    
    XPCAutoLock lock(ccx.GetRuntime()->GetMapLock());

    for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext) {
        cur->mWrappedNativeProtoMap->Enumerate(WNProtoSecPolicyClearer, nullptr);
        cur->mMainThreadWrappedNativeProtoMap->Enumerate(WNProtoSecPolicyClearer, nullptr);
    }

    DEBUG_TrackScopeTraversal();

    return NS_OK;
}

static JSDHashOperator
WNProtoRemover(JSDHashTable *table, JSDHashEntryHdr *hdr,
               uint32_t number, void *arg)
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

static PLDHashOperator
TraceDOMPrototype(const char* aKey, JSObject* aData, void* aClosure)
{
    JSTracer *trc = static_cast<JSTracer*>(aClosure);
    JS_CALL_OBJECT_TRACER(trc, aData, "DOM prototype");
    return PL_DHASH_NEXT;
}

void
XPCWrappedNativeScope::TraceDOMPrototypes(JSTracer *trc)
{
    if (mCachedDOMPrototypes.IsInitialized()) {
        mCachedDOMPrototypes.EnumerateRead(TraceDOMPrototype, trc);
    }
}




void
XPCWrappedNativeScope::DebugDumpAllScopes(int16_t depth)
{
#ifdef DEBUG
    depth-- ;

    
    int count = 0;
    XPCWrappedNativeScope* cur;
    for (cur = gScopes; cur; cur = cur->mNext)
        count++ ;

    XPC_LOG_ALWAYS(("chain of %d XPCWrappedNativeScope(s)", count));
    XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("gDyingScopes @ %x", gDyingScopes));
        if (depth)
            for (cur = gScopes; cur; cur = cur->mNext)
                cur->DebugDump(depth);
    XPC_LOG_OUTDENT();
#endif
}

#ifdef DEBUG
static JSDHashOperator
WrappedNativeMapDumpEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                               uint32_t number, void *arg)
{
    ((Native2WrappedNativeMap::Entry*)hdr)->value->DebugDump(*(int16_t*)arg);
    return JS_DHASH_NEXT;
}
static JSDHashOperator
WrappedNativeProtoMapDumpEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                    uint32_t number, void *arg)
{
    ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value->DebugDump(*(int16_t*)arg);
    return JS_DHASH_NEXT;
}
#endif

void
XPCWrappedNativeScope::DebugDump(int16_t depth)
{
#ifdef DEBUG
    depth-- ;
    XPC_LOG_ALWAYS(("XPCWrappedNativeScope @ %x", this));
    XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("mRuntime @ %x", mRuntime));
        XPC_LOG_ALWAYS(("mNext @ %x", mNext));
        XPC_LOG_ALWAYS(("mComponents @ %x", mComponents.get()));
        XPC_LOG_ALWAYS(("mGlobalJSObject @ %x", mGlobalJSObject.get()));
        XPC_LOG_ALWAYS(("mPrototypeJSObject @ %x", mPrototypeJSObject.get()));
        XPC_LOG_ALWAYS(("mPrototypeNoHelper @ %x", mPrototypeNoHelper));

        XPC_LOG_ALWAYS(("mWrappedNativeMap @ %x with %d wrappers(s)",         \
                        mWrappedNativeMap,                                    \
                        mWrappedNativeMap ? mWrappedNativeMap->Count() : 0));
        
        if (depth && mWrappedNativeMap && mWrappedNativeMap->Count()) {
            XPC_LOG_INDENT();
            mWrappedNativeMap->Enumerate(WrappedNativeMapDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }

        XPC_LOG_ALWAYS(("mWrappedNativeProtoMap @ %x with %d protos(s)",      \
                        mWrappedNativeProtoMap,                               \
                        mWrappedNativeProtoMap ? mWrappedNativeProtoMap->Count() : 0));
        
        if (depth && mWrappedNativeProtoMap && mWrappedNativeProtoMap->Count()) {
            XPC_LOG_INDENT();
            mWrappedNativeProtoMap->Enumerate(WrappedNativeProtoMapDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }

        XPC_LOG_ALWAYS(("mMainThreadWrappedNativeProtoMap @ %x with %d protos(s)", \
                        mMainThreadWrappedNativeProtoMap,                     \
                        mMainThreadWrappedNativeProtoMap ? mMainThreadWrappedNativeProtoMap->Count() : 0));
        
        if (depth && mMainThreadWrappedNativeProtoMap && mMainThreadWrappedNativeProtoMap->Count()) {
            XPC_LOG_INDENT();
            mMainThreadWrappedNativeProtoMap->Enumerate(WrappedNativeProtoMapDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }
    XPC_LOG_OUTDENT();
#endif
}

size_t
XPCWrappedNativeScope::SizeOfAllScopesIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    XPCJSRuntime *rt = nsXPConnect::GetRuntimeInstance();
    XPCAutoLock lock(rt->GetMapLock());

    size_t n = 0;
    for (XPCWrappedNativeScope *cur = gScopes; cur; cur = cur->mNext) {
        n += cur->SizeOfIncludingThis(mallocSizeOf);
    }
    return n;
}

size_t
XPCWrappedNativeScope::SizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    n += mWrappedNativeMap->SizeOfIncludingThis(mallocSizeOf);
    n += mWrappedNativeProtoMap->SizeOfIncludingThis(mallocSizeOf);
    n += mMainThreadWrappedNativeProtoMap->SizeOfIncludingThis(mallocSizeOf);

    
    
    

    return n;
}
