






#include "xpcprivate.h"
#include "XPCWrapper.h"
#include "jsproxy.h"
#include "nsContentUtils.h"
#include "nsPrincipal.h"

#include "mozilla/dom/BindingUtils.h"

using namespace mozilla;
using namespace xpc;



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
XPCWrappedNativeScope::GetNewOrUsed(JSContext *cx, JSObject* aGlobal)
{
    XPCWrappedNativeScope* scope = GetObjectScope(aGlobal);
    if (!scope) {
        scope = new XPCWrappedNativeScope(cx, aGlobal);
    } else {
        
        
        
        
        scope->SetGlobal(cx, aGlobal);
    }
    return scope;
}

XPCWrappedNativeScope::XPCWrappedNativeScope(JSContext *cx,
                                             JSObject* aGlobal)
      : mWrappedNativeMap(Native2WrappedNativeMap::newMap(XPC_NATIVE_MAP_SIZE)),
        mWrappedNativeProtoMap(ClassInfo2WrappedNativeProtoMap::newMap(XPC_NATIVE_PROTO_MAP_SIZE)),
        mMainThreadWrappedNativeProtoMap(ClassInfo2WrappedNativeProtoMap::newMap(XPC_NATIVE_PROTO_MAP_SIZE)),
        mComponents(nullptr),
        mNext(nullptr),
        mGlobalJSObject(nullptr),
        mPrototypeNoHelper(nullptr),
        mExperimentalBindingsEnabled(XPCJSRuntime::Get()->ExperimentalBindingsEnabled()),
        mIsXBLScope(false)
{
    
    {   
        XPCAutoLock lock(XPCJSRuntime::Get()->GetMapLock());

#ifdef DEBUG
        for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
            MOZ_ASSERT(aGlobal != cur->GetGlobalJSObjectPreserveColor(), "dup object");
#endif

        mNext = gScopes;
        gScopes = this;

        
        mContext = XPCContext::GetXPCContext(cx);
        mContext->AddScope(this);
    }

    if (aGlobal)
        SetGlobal(cx, aGlobal);

    DEBUG_TrackNewScope(this);
    MOZ_COUNT_CTOR(XPCWrappedNativeScope);

    
    CompartmentPrivate *priv = EnsureCompartmentPrivate(aGlobal);
    priv->scope = this;
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

JSObject*
XPCWrappedNativeScope::GetComponentsJSObject(XPCCallContext& ccx)
{
    if (!mComponents)
        mComponents = new nsXPCComponents(this);

    AutoMarkingNativeInterfacePtr iface(ccx);
    iface = XPCNativeInterface::GetNewOrUsed(ccx, &NS_GET_IID(nsIXPCComponents));
    if (!iface)
        return nullptr;

    nsCOMPtr<nsIXPCComponents> cholder(mComponents);
    xpcObjectHelper helper(cholder);
    nsCOMPtr<XPCWrappedNative> wrapper;
    XPCWrappedNative::GetNewOrUsed(ccx, helper, this, iface, getter_AddRefs(wrapper));
    if (!wrapper)
        return nullptr;

    
    
    JSObject *obj = wrapper->GetFlatJSObject();
    if (!JS_WrapObject(ccx, &obj))
        return nullptr;
    return obj;
}

JSObject*
XPCWrappedNativeScope::EnsureXBLScope(JSContext *cx)
{
    JSObject *global = GetGlobalJSObject();
    MOZ_ASSERT(js::IsObjectInContextCompartment(global, cx));
    MOZ_ASSERT(!mIsXBLScope);
    MOZ_ASSERT(strcmp(js::GetObjectClass(global)->name,
                      "nsXBLPrototypeScript compilation scope"));

    
    if (mXBLScope)
        return mXBLScope;

    
    MOZ_ASSERT(!strcmp(js::GetObjectClass(mGlobalJSObject)->name, "Window") ||
               !strcmp(js::GetObjectClass(mGlobalJSObject)->name, "ChromeWindow") ||
               !strcmp(js::GetObjectClass(mGlobalJSObject)->name, "ModalContentWindow"));

    
    
    nsIPrincipal *principal = GetPrincipal();
    if (!principal)
        return nullptr;
    if (nsContentUtils::IsSystemPrincipal(principal))
        return global;

    
    if (!XPCJSRuntime::Get()->XBLScopesEnabled())
        return global;

    
    
    
    
    
    
    
    
    SandboxOptions options;
    options.wantXrays = true;
    options.wantComponents = true;
    options.wantXHRConstructor = false;
    options.proto = global;
    options.sameZoneAs = global;

    
    nsCOMPtr<nsIExpandedPrincipal> ep;
    MOZ_ASSERT(!(ep = do_QueryInterface(principal)));
    nsTArray< nsCOMPtr<nsIPrincipal> > principalAsArray(1);
    principalAsArray.AppendElement(principal);
    ep = new nsExpandedPrincipal(principalAsArray);

    
    JSAutoRequest ar(cx);
    JS::Value v = JS::UndefinedValue();
    nsresult rv = xpc_CreateSandboxObject(cx, &v, ep, options);
    NS_ENSURE_SUCCESS(rv, nullptr);
    mXBLScope = &v.toObject();

    
    EnsureCompartmentPrivate(js::UnwrapObject(mXBLScope))->scope->mIsXBLScope = true;

    
    return mXBLScope;
}

namespace xpc {
JSObject *GetXBLScope(JSContext *cx, JSObject *contentScope)
{
    JSAutoCompartment ac(cx, contentScope);
    JSObject *scope = EnsureCompartmentPrivate(contentScope)->scope->EnsureXBLScope(cx);
    scope = js::UnwrapObject(scope);
    xpc_UnmarkGrayObject(scope);
    return scope;
}
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
XPCWrappedNativeScope::SetGlobal(JSContext *cx, JSObject* aGlobal)
{
    
    
    mGlobalJSObject = aGlobal;

    
    
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

    JSRuntime *rt = XPCJSRuntime::Get()->GetJSRuntime();
    mXBLScope.finalize(rt);
    mGlobalJSObject.finalize(rt);
}

JSObject *
XPCWrappedNativeScope::GetPrototypeNoHelper(XPCCallContext& ccx)
{
    
    
    
    if (!mPrototypeNoHelper) {
        mPrototypeNoHelper = JS_NewObject(ccx, js::Jsvalify(&XPC_WN_NoHelper_Proto_JSClass),
                                          JS_GetObjectPrototype(ccx, mGlobalJSObject),
                                          mGlobalJSObject);

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

static PLDHashOperator
TraceDOMExpandos(nsPtrHashKey<JSObject> *expando, void *aClosure)
{
    JS_CALL_OBJECT_TRACER(static_cast<JSTracer *>(aClosure), expando->GetKey(),
                          "DOM expando object");
    return PL_DHASH_NEXT;
}


void
XPCWrappedNativeScope::TraceWrappedNativesInAllScopes(JSTracer* trc, XPCJSRuntime* rt)
{
    
    
    XPCAutoLock lock(rt->GetMapLock());

    
    
    for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext) {
        cur->mWrappedNativeMap->Enumerate(WrappedNativeJSGCThingTracer, trc);
        if (cur->mDOMExpandoMap)
            cur->mDOMExpandoMap->EnumerateEntries(TraceDOMExpandos, trc);
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

static PLDHashOperator
SuspectDOMExpandos(nsPtrHashKey<JSObject> *key, void *arg)
{
    nsCycleCollectionTraversalCallback *cb =
      static_cast<nsCycleCollectionTraversalCallback *>(arg);
    JSObject* obj = key->GetKey();
    const dom::DOMClass* clasp = dom::GetDOMClass(obj);
    MOZ_ASSERT(clasp && clasp->mDOMObjectIsISupports);
    nsISupports* native = dom::UnwrapDOMObject<nsISupports>(obj);
    cb->NoteXPCOMRoot(native);
    return PL_DHASH_NEXT;
}


void
XPCWrappedNativeScope::SuspectAllWrappers(XPCJSRuntime* rt,
                                          nsCycleCollectionTraversalCallback& cb)
{
    XPCAutoLock lock(rt->GetMapLock());

    for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext) {
        cur->mWrappedNativeMap->Enumerate(WrappedNativeSuspecter, &cb);
        if (cur->mDOMExpandoMap)
            cur->mDOMExpandoMap->EnumerateEntries(SuspectDOMExpandos, &cb);
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
        
        if (cur->mWaiverWrapperMap)
            cur->mWaiverWrapperMap->Sweep();

        XPCWrappedNativeScope* next = cur->mNext;

        if (cur->mGlobalJSObject &&
            JS_IsAboutToBeFinalized(cur->mGlobalJSObject)) {
            cur->mGlobalJSObject.finalize(fop->runtime());
            
            if (prev)
                prev->mNext = next;
            else
                gScopes = next;
            cur->mNext = gDyingScopes;
            gDyingScopes = cur;
            cur = nullptr;
        } else {
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
    XPCAutoLock al(XPCJSRuntime::Get()->GetMapLock());

    mWrappedNativeProtoMap->Enumerate(WNProtoRemover,
                                      GetRuntime()->GetDetachedWrappedNativeProtoMap());
    mMainThreadWrappedNativeProtoMap->Enumerate(WNProtoRemover,
                                                GetRuntime()->GetDetachedWrappedNativeProtoMap());
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
        XPC_LOG_ALWAYS(("mNext @ %x", mNext));
        XPC_LOG_ALWAYS(("mComponents @ %x", mComponents.get()));
        XPC_LOG_ALWAYS(("mGlobalJSObject @ %x", mGlobalJSObject.get()));
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
