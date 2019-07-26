







#include "xpcprivate.h"
#include "XPCWrapper.h"
#include "nsContentUtils.h"
#include "nsCycleCollectionNoteRootCallback.h"
#include "nsPrincipal.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Preferences.h"

#include "mozilla/dom/BindingUtils.h"

using namespace mozilla;
using namespace xpc;
using namespace JS;



XPCWrappedNativeScope* XPCWrappedNativeScope::gScopes = nullptr;
XPCWrappedNativeScope* XPCWrappedNativeScope::gDyingScopes = nullptr;


XPCWrappedNativeScope*
XPCWrappedNativeScope::GetNewOrUsed(JSContext *cx, JS::HandleObject aGlobal)
{
    XPCWrappedNativeScope* scope = GetObjectScope(aGlobal);
    if (!scope) {
        scope = new XPCWrappedNativeScope(cx, aGlobal);
    }
    return scope;
}

static bool
RemoteXULForbidsXBLScope(nsIPrincipal *aPrincipal, HandleObject aGlobal)
{
  
  if (!aPrincipal)
      return false;

  
  
  
  
  
  
  if (JS_GetClass(aGlobal) == &SafeJSContextGlobalClass)
      return false;

  
  
  MOZ_ASSERT(nsContentUtils::IsInitialized());
  if (nsContentUtils::IsSystemPrincipal(aPrincipal))
      return false;

  
  if (!nsContentUtils::AllowXULXBLForPrincipal(aPrincipal))
      return false;

  
  return !Preferences::GetBool("dom.use_xbl_scopes_for_remote_xul", false);
}

XPCWrappedNativeScope::XPCWrappedNativeScope(JSContext *cx,
                                             JS::HandleObject aGlobal)
      : mWrappedNativeMap(Native2WrappedNativeMap::newMap(XPC_NATIVE_MAP_SIZE)),
        mWrappedNativeProtoMap(ClassInfo2WrappedNativeProtoMap::newMap(XPC_NATIVE_PROTO_MAP_SIZE)),
        mComponents(nullptr),
        mNext(nullptr),
        mGlobalJSObject(aGlobal),
        mIsXBLScope(false)
{
    
    {
        MOZ_ASSERT(aGlobal);
        DebugOnly<const js::Class*> clasp = js::GetObjectClass(aGlobal);
        MOZ_ASSERT(clasp->flags & (JSCLASS_PRIVATE_IS_NSISUPPORTS |
                                   JSCLASS_HAS_PRIVATE) ||
                   mozilla::dom::IsDOMClass(clasp));
#ifdef DEBUG
        for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
            MOZ_ASSERT(aGlobal != cur->GetGlobalJSObjectPreserveColor(), "dup object");
#endif

        mNext = gScopes;
        gScopes = this;
    }

    MOZ_COUNT_CTOR(XPCWrappedNativeScope);

    
    CompartmentPrivate *priv = EnsureCompartmentPrivate(aGlobal);
    priv->scope = this;

    
    
    
    nsIPrincipal *principal = GetPrincipal();
    mAllowXBLScope = !RemoteXULForbidsXBLScope(principal, aGlobal);

    
    mUseXBLScope = mAllowXBLScope;
    if (mUseXBLScope) {
      const js::Class *clasp = js::GetObjectClass(mGlobalJSObject);
      mUseXBLScope = !strcmp(clasp->name, "Window") ||
                     !strcmp(clasp->name, "ChromeWindow") ||
                     !strcmp(clasp->name, "ModalContentWindow");
    }
    if (mUseXBLScope) {
      mUseXBLScope = principal && !nsContentUtils::IsSystemPrincipal(principal);
    }
}


bool
XPCWrappedNativeScope::IsDyingScope(XPCWrappedNativeScope *scope)
{
    for (XPCWrappedNativeScope *cur = gDyingScopes; cur; cur = cur->mNext) {
        if (scope == cur)
            return true;
    }
    return false;
}

JSObject*
XPCWrappedNativeScope::GetComponentsJSObject()
{
    AutoJSContext cx;
    if (!mComponents) {
        nsIPrincipal *p = GetPrincipal();
        bool system = nsXPConnect::SecurityManager()->IsSystemPrincipal(p);
        mComponents = system ? new nsXPCComponents(this)
                             : new nsXPCComponentsBase(this);
    }

    RootedValue val(cx);
    xpcObjectHelper helper(mComponents);
    bool ok = XPCConvert::NativeInterface2JSObject(&val, nullptr, helper,
                                                   nullptr, nullptr, false,
                                                   nullptr);
    if (NS_WARN_IF(!ok))
        return nullptr;

    if (NS_WARN_IF(!val.isObject()))
        return nullptr;

    
    
    JS::RootedObject obj(cx, &val.toObject());
    if (NS_WARN_IF(!JS_WrapObject(cx, &obj)))
        return nullptr;
    return obj;
}

void
XPCWrappedNativeScope::ForcePrivilegedComponents()
{
    
    
    MOZ_RELEASE_ASSERT(Preferences::GetBool("security.turn_off_all_security_so_"
                                            "that_viruses_can_take_over_this_"
                                            "computer"));
    nsCOMPtr<nsIXPCComponents> c = do_QueryInterface(mComponents);
    if (!c)
        mComponents = new nsXPCComponents(this);
}

bool
XPCWrappedNativeScope::AttachComponentsObject(JSContext* aCx)
{
    RootedObject components(aCx, GetComponentsJSObject());
    if (!components)
        return false;

    RootedObject global(aCx, GetGlobalJSObject());
    MOZ_ASSERT(js::IsObjectInContextCompartment(global, aCx));

    RootedId id(aCx, XPCJSRuntime::Get()->GetStringID(XPCJSRuntime::IDX_COMPONENTS));
    return JS_DefinePropertyById(aCx, global, id, components,
                                 JSPROP_PERMANENT | JSPROP_READONLY);
}

JSObject*
XPCWrappedNativeScope::EnsureXBLScope(JSContext *cx)
{
    JS::RootedObject global(cx, GetGlobalJSObject());
    MOZ_ASSERT(js::IsObjectInContextCompartment(global, cx));
    MOZ_ASSERT(!mIsXBLScope);
    MOZ_ASSERT(strcmp(js::GetObjectClass(global)->name,
                      "nsXBLPrototypeScript compilation scope"));

    
    if (mXBLScope)
        return mXBLScope;

    
    if (!mUseXBLScope)
        return global;

    
    
    
    
    
    
    
    
    SandboxOptions options;
    options.wantXrays = true;
    options.wantComponents = true;
    options.proto = global;
    options.sameZoneAs = global;

    
    nsIPrincipal *principal = GetPrincipal();
    nsCOMPtr<nsIExpandedPrincipal> ep;
    MOZ_ASSERT(!(ep = do_QueryInterface(principal)));
    nsTArray< nsCOMPtr<nsIPrincipal> > principalAsArray(1);
    principalAsArray.AppendElement(principal);
    ep = new nsExpandedPrincipal(principalAsArray);

    
    RootedValue v(cx);
    nsresult rv = CreateSandboxObject(cx, &v, ep, options);
    NS_ENSURE_SUCCESS(rv, nullptr);
    mXBLScope = &v.toObject();

    
    EnsureCompartmentPrivate(js::UncheckedUnwrap(mXBLScope))->scope->mIsXBLScope = true;

    
    return mXBLScope;
}

bool
XPCWrappedNativeScope::AllowXBLScope()
{
    
    MOZ_ASSERT_IF(!mAllowXBLScope,
                  nsContentUtils::AllowXULXBLForPrincipal(GetPrincipal()));
    return mAllowXBLScope;
}

namespace xpc {
JSObject *GetXBLScope(JSContext *cx, JSObject *contentScopeArg)
{
    JS::RootedObject contentScope(cx, contentScopeArg);
    JSAutoCompartment ac(cx, contentScope);
    JSObject *scope = EnsureCompartmentPrivate(contentScope)->scope->EnsureXBLScope(cx);
    NS_ENSURE_TRUE(scope, nullptr); 
    scope = js::UncheckedUnwrap(scope);
    JS::ExposeObjectToActiveJS(scope);
    return scope;
}

bool AllowXBLScope(JSCompartment *c)
{
  XPCWrappedNativeScope *scope = EnsureCompartmentPrivate(c)->scope;
  return scope && scope->AllowXBLScope();
}

bool UseXBLScope(JSCompartment *c)
{
  XPCWrappedNativeScope *scope = EnsureCompartmentPrivate(c)->scope;
  return scope && scope->UseXBLScope();
}

} 

XPCWrappedNativeScope::~XPCWrappedNativeScope()
{
    MOZ_COUNT_DTOR(XPCWrappedNativeScope);

    

    if (mWrappedNativeMap) {
        MOZ_ASSERT(0 == mWrappedNativeMap->Count(), "scope has non-empty map");
        delete mWrappedNativeMap;
    }

    if (mWrappedNativeProtoMap) {
        MOZ_ASSERT(0 == mWrappedNativeProtoMap->Count(), "scope has non-empty map");
        delete mWrappedNativeProtoMap;
    }

    
    
    if (mComponents)
        mComponents->mScope = nullptr;

    
    
    mComponents = nullptr;

    if (mXrayExpandos.initialized())
        mXrayExpandos.destroy();

    JSRuntime *rt = XPCJSRuntime::Get()->Runtime();
    mXBLScope.finalize(rt);
    mGlobalJSObject.finalize(rt);
}

static PLDHashOperator
WrappedNativeJSGCThingTracer(PLDHashTable *table, PLDHashEntryHdr *hdr,
                             uint32_t number, void *arg)
{
    XPCWrappedNative* wrapper = ((Native2WrappedNativeMap::Entry*)hdr)->value;
    if (wrapper->HasExternalReference() && !wrapper->IsWrapperExpired())
        wrapper->TraceSelf((JSTracer *)arg);

    return PL_DHASH_NEXT;
}


void
XPCWrappedNativeScope::TraceWrappedNativesInAllScopes(JSTracer* trc, XPCJSRuntime* rt)
{
    
    
    for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext) {
        cur->mWrappedNativeMap->Enumerate(WrappedNativeJSGCThingTracer, trc);
        if (cur->mDOMExpandoSet) {
            for (DOMExpandoSet::Enum e(*cur->mDOMExpandoSet); !e.empty(); e.popFront())
                JS_CallHashSetObjectTracer(trc, e, e.front(), "DOM expando object");
        }
    }
}

static PLDHashOperator
WrappedNativeSuspecter(PLDHashTable *table, PLDHashEntryHdr *hdr,
                       uint32_t number, void *arg)
{
    XPCWrappedNative* wrapper = ((Native2WrappedNativeMap::Entry*)hdr)->value;

    if (wrapper->HasExternalReference()) {
        nsCycleCollectionNoteRootCallback *cb =
            static_cast<nsCycleCollectionNoteRootCallback *>(arg);
        XPCJSRuntime::SuspectWrappedNative(wrapper, *cb);
    }

    return PL_DHASH_NEXT;
}

static void
SuspectDOMExpandos(JSObject *obj, nsCycleCollectionNoteRootCallback &cb)
{
    MOZ_ASSERT(dom::GetDOMClass(obj) && dom::GetDOMClass(obj)->mDOMObjectIsISupports);
    nsISupports* native = dom::UnwrapDOMObject<nsISupports>(obj);
    cb.NoteXPCOMRoot(native);
}


void
XPCWrappedNativeScope::SuspectAllWrappers(XPCJSRuntime* rt,
                                          nsCycleCollectionNoteRootCallback& cb)
{
    for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext) {
        cur->mWrappedNativeMap->Enumerate(WrappedNativeSuspecter, &cb);
        if (cur->mDOMExpandoSet) {
            for (DOMExpandoSet::Range r = cur->mDOMExpandoSet->all(); !r.empty(); r.popFront())
                SuspectDOMExpandos(r.front(), cb);
        }
    }
}


void
XPCWrappedNativeScope::StartFinalizationPhaseOfGC(JSFreeOp *fop, XPCJSRuntime* rt)
{
    
    
    
    MOZ_ASSERT(!gDyingScopes, "JSGC_MARK_END without JSGC_FINALIZE_END");

    XPCWrappedNativeScope* prev = nullptr;
    XPCWrappedNativeScope* cur = gScopes;

    while (cur) {
        
        if (cur->mWaiverWrapperMap)
            cur->mWaiverWrapperMap->Sweep();

        XPCWrappedNativeScope* next = cur->mNext;

        if (cur->mGlobalJSObject && cur->mGlobalJSObject.isAboutToBeFinalized()) {
            cur->mGlobalJSObject.finalize(fop->runtime());
            
            if (prev)
                prev->mNext = next;
            else
                gScopes = next;
            cur->mNext = gDyingScopes;
            gDyingScopes = cur;
            cur = nullptr;
        }
        if (cur)
            prev = cur;
        cur = next;
    }
}


void
XPCWrappedNativeScope::FinishedFinalizationPhaseOfGC()
{
    KillDyingScopes();
}

static PLDHashOperator
WrappedNativeMarker(PLDHashTable *table, PLDHashEntryHdr *hdr,
                    uint32_t number_t, void *arg)
{
    ((Native2WrappedNativeMap::Entry*)hdr)->value->Mark();
    return PL_DHASH_NEXT;
}



static PLDHashOperator
WrappedNativeProtoMarker(PLDHashTable *table, PLDHashEntryHdr *hdr,
                         uint32_t number, void *arg)
{
    ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value->Mark();
    return PL_DHASH_NEXT;
}


void
XPCWrappedNativeScope::MarkAllWrappedNativesAndProtos()
{
    for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext) {
        cur->mWrappedNativeMap->Enumerate(WrappedNativeMarker, nullptr);
        cur->mWrappedNativeProtoMap->Enumerate(WrappedNativeProtoMarker, nullptr);
    }
}

#ifdef DEBUG
static PLDHashOperator
ASSERT_WrappedNativeSetNotMarked(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                 uint32_t number, void *arg)
{
    ((Native2WrappedNativeMap::Entry*)hdr)->value->ASSERT_SetsNotMarked();
    return PL_DHASH_NEXT;
}

static PLDHashOperator
ASSERT_WrappedNativeProtoSetNotMarked(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                      uint32_t number, void *arg)
{
    ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value->ASSERT_SetNotMarked();
    return PL_DHASH_NEXT;
}


void
XPCWrappedNativeScope::ASSERT_NoInterfaceSetsAreMarked()
{
    for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext) {
        cur->mWrappedNativeMap->Enumerate(ASSERT_WrappedNativeSetNotMarked, nullptr);
        cur->mWrappedNativeProtoMap->Enumerate(ASSERT_WrappedNativeProtoSetNotMarked, nullptr);
    }
}
#endif

static PLDHashOperator
WrappedNativeTearoffSweeper(PLDHashTable *table, PLDHashEntryHdr *hdr,
                            uint32_t number, void *arg)
{
    ((Native2WrappedNativeMap::Entry*)hdr)->value->SweepTearOffs();
    return PL_DHASH_NEXT;
}


void
XPCWrappedNativeScope::SweepAllWrappedNativeTearOffs()
{
    for (XPCWrappedNativeScope* cur = gScopes; cur; cur = cur->mNext)
        cur->mWrappedNativeMap->Enumerate(WrappedNativeTearoffSweeper, nullptr);
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

static PLDHashOperator
WrappedNativeShutdownEnumerator(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                uint32_t number, void *arg)
{
    ShutdownData* data = (ShutdownData*) arg;
    XPCWrappedNative* wrapper = ((Native2WrappedNativeMap::Entry*)hdr)->value;

    if (wrapper->IsValid()) {
        wrapper->SystemIsBeingShutDown();
        data->wrapperCount++;
    }
    return PL_DHASH_REMOVE;
}

static PLDHashOperator
WrappedNativeProtoShutdownEnumerator(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                     uint32_t number, void *arg)
{
    ShutdownData* data = (ShutdownData*) arg;
    ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value->
        SystemIsBeingShutDown();
    data->protoCount++;
    return PL_DHASH_REMOVE;
}


void
XPCWrappedNativeScope::SystemIsBeingShutDown()
{
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
        cur->mWrappedNativeMap->
                Enumerate(WrappedNativeShutdownEnumerator,  &data);
    }

    
    KillDyingScopes();
}




static PLDHashOperator
WNProtoRemover(PLDHashTable *table, PLDHashEntryHdr *hdr,
               uint32_t number, void *arg)
{
    XPCWrappedNativeProtoMap* detachedMap = (XPCWrappedNativeProtoMap*)arg;

    XPCWrappedNativeProto* proto = (XPCWrappedNativeProto*)
        ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value;

    detachedMap->Add(proto);

    return PL_DHASH_REMOVE;
}

void
XPCWrappedNativeScope::RemoveWrappedNativeProtos()
{
    mWrappedNativeProtoMap->Enumerate(WNProtoRemover,
                                      GetRuntime()->GetDetachedWrappedNativeProtoMap());
}

JSObject *
XPCWrappedNativeScope::GetExpandoChain(JSObject *target)
{
    MOZ_ASSERT(GetObjectScope(target) == this);
    if (!mXrayExpandos.initialized())
        return nullptr;
    return mXrayExpandos.lookup(target);
}

bool
XPCWrappedNativeScope::SetExpandoChain(JSContext *cx, HandleObject target,
                                       HandleObject chain)
{
    MOZ_ASSERT(GetObjectScope(target) == this);
    MOZ_ASSERT(js::IsObjectInContextCompartment(target, cx));
    MOZ_ASSERT_IF(chain, GetObjectScope(chain) == this);
    if (!mXrayExpandos.initialized() && !mXrayExpandos.init(cx))
        return false;
    return mXrayExpandos.put(cx, target, chain);
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
static PLDHashOperator
WrappedNativeMapDumpEnumerator(PLDHashTable *table, PLDHashEntryHdr *hdr,
                               uint32_t number, void *arg)
{
    ((Native2WrappedNativeMap::Entry*)hdr)->value->DebugDump(*(int16_t*)arg);
    return PL_DHASH_NEXT;
}
static PLDHashOperator
WrappedNativeProtoMapDumpEnumerator(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                    uint32_t number, void *arg)
{
    ((ClassInfo2WrappedNativeProtoMap::Entry*)hdr)->value->DebugDump(*(int16_t*)arg);
    return PL_DHASH_NEXT;
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
    XPC_LOG_OUTDENT();
#endif
}

void
XPCWrappedNativeScope::AddSizeOfAllScopesIncludingThis(ScopeSizeInfo* scopeSizeInfo)
{
    for (XPCWrappedNativeScope *cur = gScopes; cur; cur = cur->mNext)
        cur->AddSizeOfIncludingThis(scopeSizeInfo);
}

void
XPCWrappedNativeScope::AddSizeOfIncludingThis(ScopeSizeInfo* scopeSizeInfo)
{
    scopeSizeInfo->mScopeAndMapSize += scopeSizeInfo->mMallocSizeOf(this);
    scopeSizeInfo->mScopeAndMapSize +=
        mWrappedNativeMap->SizeOfIncludingThis(scopeSizeInfo->mMallocSizeOf);
    scopeSizeInfo->mScopeAndMapSize +=
        mWrappedNativeProtoMap->SizeOfIncludingThis(scopeSizeInfo->mMallocSizeOf);

    if (dom::HasProtoAndIfaceCache(mGlobalJSObject)) {
        dom::ProtoAndIfaceCache* cache = dom::GetProtoAndIfaceCache(mGlobalJSObject);
        scopeSizeInfo->mProtoAndIfaceCacheSize +=
            cache->SizeOfIncludingThis(scopeSizeInfo->mMallocSizeOf);
    }

    
    
    
}
