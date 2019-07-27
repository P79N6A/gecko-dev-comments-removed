







#include "mozilla/MemoryReporting.h"
#include "mozilla/UniquePtr.h"

#include "xpcprivate.h"
#include "xpcpublic.h"
#include "XPCWrapper.h"
#include "XPCJSMemoryReporter.h"
#include "WrapperFactory.h"
#include "mozJSComponentLoader.h"

#include "nsIMemoryInfoDumper.h"
#include "nsIMemoryReporter.h"
#include "nsIObserverService.h"
#include "nsIDebug2.h"
#include "nsIDocShell.h"
#include "amIAddonManager.h"
#include "nsPIDOMWindow.h"
#include "nsPrintfCString.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"
#include "mozilla/Services.h"

#include "nsContentUtils.h"
#include "nsCCUncollectableMarker.h"
#include "nsCycleCollectionNoteRootCallback.h"
#include "nsCycleCollector.h"
#include "nsScriptLoader.h"
#include "jsfriendapi.h"
#include "jsprf.h"
#include "js/MemoryMetrics.h"
#include "mozilla/dom/GeneratedAtomList.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/WindowBinding.h"
#include "mozilla/Atomics.h"
#include "mozilla/Attributes.h"
#include "mozilla/ProcessHangMonitor.h"
#include "AccessCheck.h"
#include "nsGlobalWindow.h"
#include "nsAboutProtocolUtils.h"

#include "GeckoProfiler.h"
#include "nsIXULRuntime.h"
#include "nsJSPrincipals.h"

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

using namespace mozilla;
using namespace xpc;
using namespace JS;
using mozilla::dom::PerThreadAtomCache;



const char* const XPCJSRuntime::mStrings[] = {
    "constructor",          
    "toString",             
    "toSource",             
    "lastResult",           
    "returnCode",           
    "value",                
    "QueryInterface",       
    "Components",           
    "wrappedJSObject",      
    "Object",               
    "Function",             
    "prototype",            
    "createInstance",       
    "item",                 
    "__proto__",            
    "__iterator__",         
    "__exposedProps__",     
    "eval",                 
    "controllers",          
    "realFrameElement",     
    "length",               
    "name",                 
    "undefined",            
    "",                     
    "fileName",             
    "lineNumber",           
    "columnNumber",         
    "stack",                
    "message",              
    "lastIndex"             
};



static mozilla::Atomic<bool> sDiscardSystemSource(false);

bool
xpc::ShouldDiscardSystemSource() { return sDiscardSystemSource; }

#ifdef DEBUG
static mozilla::Atomic<bool> sExtraWarningsForSystemJS(false);
bool xpc::ExtraWarningsForSystemJS() { return sExtraWarningsForSystemJS; }
#else
bool xpc::ExtraWarningsForSystemJS() { return false; }
#endif

static void * const UNMARK_ONLY = nullptr;
static void * const UNMARK_AND_SWEEP = (void*)1;

static PLDHashOperator
NativeInterfaceSweeper(PLDHashTable* table, PLDHashEntryHdr* hdr,
                       uint32_t number, void* arg)
{
    XPCNativeInterface* iface = ((IID2NativeInterfaceMap::Entry*)hdr)->value;
    if (iface->IsMarked()) {
        iface->Unmark();
        return PL_DHASH_NEXT;
    }

    if (arg == UNMARK_ONLY)
        return PL_DHASH_NEXT;

    XPCNativeInterface::DestroyInstance(iface);
    return PL_DHASH_REMOVE;
}






static PLDHashOperator
NativeUnMarkedSetRemover(PLDHashTable* table, PLDHashEntryHdr* hdr,
                         uint32_t number, void* arg)
{
    XPCNativeSet* set = ((ClassInfo2NativeSetMap::Entry*)hdr)->value;
    if (set->IsMarked())
        return PL_DHASH_NEXT;
    return PL_DHASH_REMOVE;
}

static PLDHashOperator
NativeSetSweeper(PLDHashTable* table, PLDHashEntryHdr* hdr,
                 uint32_t number, void* arg)
{
    XPCNativeSet* set = ((NativeSetMap::Entry*)hdr)->key_value;
    if (set->IsMarked()) {
        set->Unmark();
        return PL_DHASH_NEXT;
    }

    if (arg == UNMARK_ONLY)
        return PL_DHASH_NEXT;

    XPCNativeSet::DestroyInstance(set);
    return PL_DHASH_REMOVE;
}

static PLDHashOperator
JSClassSweeper(PLDHashTable* table, PLDHashEntryHdr* hdr,
               uint32_t number, void* arg)
{
    XPCNativeScriptableShared* shared =
        ((XPCNativeScriptableSharedMap::Entry*) hdr)->key;
    if (shared->IsMarked()) {
        shared->Unmark();
        return PL_DHASH_NEXT;
    }

    if (arg == UNMARK_ONLY)
        return PL_DHASH_NEXT;

    delete shared;
    return PL_DHASH_REMOVE;
}

static PLDHashOperator
DyingProtoKiller(PLDHashTable* table, PLDHashEntryHdr* hdr,
                 uint32_t number, void* arg)
{
    XPCWrappedNativeProto* proto =
        (XPCWrappedNativeProto*)((PLDHashEntryStub*)hdr)->key;
    delete proto;
    return PL_DHASH_REMOVE;
}

static PLDHashOperator
DetachedWrappedNativeProtoMarker(PLDHashTable* table, PLDHashEntryHdr* hdr,
                                 uint32_t number, void* arg)
{
    XPCWrappedNativeProto* proto =
        (XPCWrappedNativeProto*)((PLDHashEntryStub*)hdr)->key;

    proto->Mark();
    return PL_DHASH_NEXT;
}

bool
XPCJSRuntime::CustomContextCallback(JSContext* cx, unsigned operation)
{
    if (operation == JSCONTEXT_NEW) {
        if (!OnJSContextNew(cx)) {
            return false;
        }
    } else if (operation == JSCONTEXT_DESTROY) {
        delete XPCContext::GetXPCContext(cx);
    }

    nsTArray<xpcContextCallback> callbacks(extraContextCallbacks);
    for (uint32_t i = 0; i < callbacks.Length(); ++i) {
        if (!callbacks[i](cx, operation)) {
            return false;
        }
    }

    return true;
}

class AsyncFreeSnowWhite : public nsRunnable
{
public:
  NS_IMETHOD Run()
  {
      TimeStamp start = TimeStamp::Now();
      bool hadSnowWhiteObjects = nsCycleCollector_doDeferredDeletion();
      Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR_ASYNC_SNOW_WHITE_FREEING,
                            uint32_t((TimeStamp::Now() - start).ToMilliseconds()));
      if (hadSnowWhiteObjects && !mContinuation) {
          mContinuation = true;
          if (NS_FAILED(NS_DispatchToCurrentThread(this))) {
              mActive = false;
          }
      } else {
          mActive = false;
      }
      return NS_OK;
  }

  void Dispatch(bool aContinuation = false)
  {
      if (mContinuation) {
          mContinuation = aContinuation;
      }
      if (!mActive && NS_SUCCEEDED(NS_DispatchToCurrentThread(this))) {
          mActive = true;
      }
  }

  AsyncFreeSnowWhite() : mContinuation(false), mActive(false) {}

public:
  bool mContinuation;
  bool mActive;
};

namespace xpc {

CompartmentPrivate::~CompartmentPrivate()
{
    MOZ_COUNT_DTOR(xpc::CompartmentPrivate);
}

static bool
TryParseLocationURICandidate(const nsACString& uristr,
                             CompartmentPrivate::LocationHint aLocationHint,
                             nsIURI** aURI)
{
    static NS_NAMED_LITERAL_CSTRING(kGRE, "resource://gre/");
    static NS_NAMED_LITERAL_CSTRING(kToolkit, "chrome://global/");
    static NS_NAMED_LITERAL_CSTRING(kBrowser, "chrome://browser/");

    if (aLocationHint == CompartmentPrivate::LocationHintAddon) {
        
        if (StringBeginsWith(uristr, kGRE) ||
            StringBeginsWith(uristr, kToolkit) ||
            StringBeginsWith(uristr, kBrowser))
            return false;
    }

    nsCOMPtr<nsIURI> uri;
    if (NS_FAILED(NS_NewURI(getter_AddRefs(uri), uristr)))
        return false;

    nsAutoCString scheme;
    if (NS_FAILED(uri->GetScheme(scheme)))
        return false;

    
    
    
    if (scheme.EqualsLiteral("data") || scheme.EqualsLiteral("blob"))
        return false;

    uri.forget(aURI);
    return true;
}

bool CompartmentPrivate::TryParseLocationURI(CompartmentPrivate::LocationHint aLocationHint,
                                             nsIURI** aURI)
{
    if (!aURI)
        return false;

    
    if (location.IsEmpty())
        return false;

    
    
    
    
    
    
    
    
    
    
    

    static const nsDependentCString from("(from: ");
    static const nsDependentCString arrow(" -> ");
    static const size_t fromLength = from.Length();
    static const size_t arrowLength = arrow.Length();

    
    int32_t idx = location.Find(from);
    if (idx < 0)
        return TryParseLocationURICandidate(location, aLocationHint, aURI);


    
    
    if (TryParseLocationURICandidate(Substring(location, 0, idx), aLocationHint,
                                     aURI))
        return true;

    
    
    

    
    int32_t ridx = location.RFind(NS_LITERAL_CSTRING(":"));
    nsAutoCString chain(Substring(location, idx + fromLength,
                                  ridx - idx - fromLength));

    
    
    for (;;) {
        idx = chain.RFind(arrow);
        if (idx < 0) {
            
            return TryParseLocationURICandidate(chain, aLocationHint, aURI);
        }

        
        if (TryParseLocationURICandidate(Substring(chain, idx + arrowLength),
                                         aLocationHint, aURI))
            return true;

        
        
        chain = Substring(chain, 0, idx);
    }

    MOZ_CRASH("Chain parser loop does not terminate");
}

static bool
PrincipalImmuneToScriptPolicy(nsIPrincipal* aPrincipal)
{
    
    if (nsXPConnect::SecurityManager()->IsSystemPrincipal(aPrincipal))
        return true;

    
    nsCOMPtr<nsIExpandedPrincipal> ep = do_QueryInterface(aPrincipal);
    if (ep)
        return true;

    
    
    nsCOMPtr<nsIURI> principalURI;
    aPrincipal->GetURI(getter_AddRefs(principalURI));
    MOZ_ASSERT(principalURI);
    bool isAbout;
    nsresult rv = principalURI->SchemeIs("about", &isAbout);
    if (NS_SUCCEEDED(rv) && isAbout) {
        nsCOMPtr<nsIAboutModule> module;
        rv = NS_GetAboutModule(principalURI, getter_AddRefs(module));
        if (NS_SUCCEEDED(rv)) {
            uint32_t flags;
            rv = module->GetURIFlags(principalURI, &flags);
            if (NS_SUCCEEDED(rv) &&
                (flags & nsIAboutModule::ALLOW_SCRIPT)) {
                return true;
            }
        }
    }

    return false;
}

Scriptability::Scriptability(JSCompartment* c) : mScriptBlocks(0)
                                               , mDocShellAllowsScript(true)
                                               , mScriptBlockedByPolicy(false)
{
    nsIPrincipal* prin = nsJSPrincipals::get(JS_GetCompartmentPrincipals(c));
    mImmuneToScriptPolicy = PrincipalImmuneToScriptPolicy(prin);

    
    
    if (!mImmuneToScriptPolicy) {
        nsCOMPtr<nsIURI> codebase;
        nsresult rv = prin->GetURI(getter_AddRefs(codebase));
        bool policyAllows;
        if (NS_SUCCEEDED(rv) && codebase &&
            NS_SUCCEEDED(nsXPConnect::SecurityManager()->PolicyAllowsScript(codebase, &policyAllows)))
        {
            mScriptBlockedByPolicy = !policyAllows;
        } else {
            
            mScriptBlockedByPolicy = true;
        }
    }
}

bool
Scriptability::Allowed()
{
    return mDocShellAllowsScript && !mScriptBlockedByPolicy &&
           mScriptBlocks == 0;
}

bool
Scriptability::IsImmuneToScriptPolicy()
{
    return mImmuneToScriptPolicy;
}

void
Scriptability::Block()
{
    ++mScriptBlocks;
}

void
Scriptability::Unblock()
{
    MOZ_ASSERT(mScriptBlocks > 0);
    --mScriptBlocks;
}

void
Scriptability::SetDocShellAllowsScript(bool aAllowed)
{
    mDocShellAllowsScript = aAllowed || mImmuneToScriptPolicy;
}


Scriptability&
Scriptability::Get(JSObject* aScope)
{
    return CompartmentPrivate::Get(aScope)->scriptability;
}

bool
IsContentXBLScope(JSCompartment* compartment)
{
    
    CompartmentPrivate* priv = CompartmentPrivate::Get(compartment);
    if (!priv || !priv->scope)
        return false;
    return priv->scope->IsContentXBLScope();
}

bool
IsInContentXBLScope(JSObject* obj)
{
    return IsContentXBLScope(js::GetObjectCompartment(obj));
}

bool
IsInAddonScope(JSObject* obj)
{
    return ObjectScope(obj)->IsAddonScope();
}

bool
IsUniversalXPConnectEnabled(JSCompartment* compartment)
{
    CompartmentPrivate* priv = CompartmentPrivate::Get(compartment);
    if (!priv)
        return false;
    return priv->universalXPConnectEnabled;
}

bool
IsUniversalXPConnectEnabled(JSContext* cx)
{
    JSCompartment* compartment = js::GetContextCompartment(cx);
    if (!compartment)
        return false;
    return IsUniversalXPConnectEnabled(compartment);
}

bool
EnableUniversalXPConnect(JSContext* cx)
{
    JSCompartment* compartment = js::GetContextCompartment(cx);
    if (!compartment)
        return true;
    
    
    if (AccessCheck::isChrome(compartment))
        return true;
    CompartmentPrivate* priv = CompartmentPrivate::Get(compartment);
    if (!priv)
        return true;
    if (priv->universalXPConnectEnabled)
        return true;
    priv->universalXPConnectEnabled = true;

    
    
    bool ok = js::RecomputeWrappers(cx, js::SingleCompartment(compartment),
                                    js::AllCompartments());
    NS_ENSURE_TRUE(ok, false);

    
    
    
    XPCWrappedNativeScope* scope = priv->scope;
    if (!scope)
        return true;
    scope->ForcePrivilegedComponents();
    return scope->AttachComponentsObject(cx);
}

JSObject*
UnprivilegedJunkScope()
{
    return XPCJSRuntime::Get()->UnprivilegedJunkScope();
}

JSObject*
PrivilegedJunkScope()
{
    return XPCJSRuntime::Get()->PrivilegedJunkScope();
}

JSObject*
CompilationScope()
{
    return XPCJSRuntime::Get()->CompilationScope();
}

nsGlobalWindow*
WindowOrNull(JSObject* aObj)
{
    MOZ_ASSERT(aObj);
    MOZ_ASSERT(!js::IsWrapper(aObj));

    nsGlobalWindow* win = nullptr;
    UNWRAP_OBJECT(Window, aObj, win);
    return win;
}

nsGlobalWindow*
WindowGlobalOrNull(JSObject* aObj)
{
    MOZ_ASSERT(aObj);
    JSObject* glob = js::GetGlobalForObjectCrossCompartment(aObj);

    return WindowOrNull(glob);
}

nsGlobalWindow*
AddonWindowOrNull(JSObject* aObj)
{
    if (!IsInAddonScope(aObj))
        return nullptr;

    JSObject* global = js::GetGlobalForObjectCrossCompartment(aObj);
    JSObject* proto = js::GetPrototypeNoProxy(global);

    
    
    
    MOZ_RELEASE_ASSERT(js::IsCrossCompartmentWrapper(proto) ||
                       xpc::IsSandboxPrototypeProxy(proto));
    JSObject* mainGlobal = js::UncheckedUnwrap(proto,  false);
    MOZ_RELEASE_ASSERT(JS_IsGlobalObject(mainGlobal));

    return WindowOrNull(mainGlobal);
}

nsGlobalWindow*
CurrentWindowOrNull(JSContext* cx)
{
    JSObject* glob = JS::CurrentGlobalOrNull(cx);
    return glob ? WindowOrNull(glob) : nullptr;
}

}

static void
CompartmentDestroyedCallback(JSFreeOp* fop, JSCompartment* compartment)
{
    
    

    
    
    nsAutoPtr<CompartmentPrivate> priv(CompartmentPrivate::Get(compartment));
    JS_SetCompartmentPrivate(compartment, nullptr);
}

void XPCJSRuntime::TraceNativeBlackRoots(JSTracer* trc)
{
    
    
    if (!nsXPConnect::XPConnect()->IsShuttingDown()) {
        
        if (AutoMarkingPtr* roots = Get()->mAutoRoots)
            roots->TraceJSAll(trc);
    }

    
    
    XPCRootSetElem* e;
    for (e = mObjectHolderRoots; e; e = e->GetNextRoot())
        static_cast<XPCJSObjectHolder*>(e)->TraceJS(trc);

    dom::TraceBlackJS(trc, JS_GetGCParameter(Runtime(), JSGC_NUMBER),
                      nsXPConnect::XPConnect()->IsShuttingDown());
}

void XPCJSRuntime::TraceAdditionalNativeGrayRoots(JSTracer* trc)
{
    XPCWrappedNativeScope::TraceWrappedNativesInAllScopes(trc, this);

    for (XPCRootSetElem* e = mVariantRoots; e ; e = e->GetNextRoot())
        static_cast<XPCTraceableVariant*>(e)->TraceJS(trc);

    for (XPCRootSetElem* e = mWrappedJSRoots; e ; e = e->GetNextRoot())
        static_cast<nsXPCWrappedJS*>(e)->TraceJS(trc);
}


void
XPCJSRuntime::SuspectWrappedNative(XPCWrappedNative* wrapper,
                                   nsCycleCollectionNoteRootCallback& cb)
{
    if (!wrapper->IsValid() || wrapper->IsWrapperExpired())
        return;

    MOZ_ASSERT(NS_IsMainThread(),
               "Suspecting wrapped natives from non-main thread");

    
    
    JSObject* obj = wrapper->GetFlatJSObjectPreserveColor();
    if (JS::ObjectIsMarkedGray(obj) || cb.WantAllTraces())
        cb.NoteJSRoot(obj);
}

void
XPCJSRuntime::TraverseAdditionalNativeRoots(nsCycleCollectionNoteRootCallback& cb)
{
    XPCWrappedNativeScope::SuspectAllWrappers(this, cb);

    for (XPCRootSetElem* e = mVariantRoots; e ; e = e->GetNextRoot()) {
        XPCTraceableVariant* v = static_cast<XPCTraceableVariant*>(e);
        if (nsCCUncollectableMarker::InGeneration(cb,
                                                  v->CCGeneration())) {
           JS::Value val = v->GetJSValPreserveColor();
           if (val.isObject() && !JS::ObjectIsMarkedGray(&val.toObject()))
               continue;
        }
        cb.NoteXPCOMRoot(v);
    }

    for (XPCRootSetElem* e = mWrappedJSRoots; e ; e = e->GetNextRoot()) {
        cb.NoteXPCOMRoot(ToSupports(static_cast<nsXPCWrappedJS*>(e)));
    }
}

void
XPCJSRuntime::UnmarkSkippableJSHolders()
{
    CycleCollectedJSRuntime::UnmarkSkippableJSHolders();
}

void
XPCJSRuntime::PrepareForForgetSkippable()
{
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        obs->NotifyObservers(nullptr, "cycle-collector-forget-skippable", nullptr);
    }
}

void
XPCJSRuntime::BeginCycleCollectionCallback()
{
    nsJSContext::BeginCycleCollectionCallback();

    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        obs->NotifyObservers(nullptr, "cycle-collector-begin", nullptr);
    }
}

void
XPCJSRuntime::EndCycleCollectionCallback(CycleCollectorResults& aResults)
{
    nsJSContext::EndCycleCollectionCallback(aResults);

    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        obs->NotifyObservers(nullptr, "cycle-collector-end", nullptr);
    }
}

void
XPCJSRuntime::DispatchDeferredDeletion(bool aContinuation)
{
    mAsyncSnowWhiteFreer->Dispatch(aContinuation);
}

void
xpc_UnmarkSkippableJSHolders()
{
    if (nsXPConnect::XPConnect()->GetRuntime()) {
        nsXPConnect::XPConnect()->GetRuntime()->UnmarkSkippableJSHolders();
    }
}

template<class T> static void
DoDeferredRelease(nsTArray<T>& array)
{
    while (1) {
        uint32_t count = array.Length();
        if (!count) {
            array.Compact();
            break;
        }
        T wrapper = array[count-1];
        array.RemoveElementAt(count-1);
        NS_RELEASE(wrapper);
    }
}

 void
XPCJSRuntime::GCSliceCallback(JSRuntime* rt,
                              JS::GCProgress progress,
                              const JS::GCDescription& desc)
{
    XPCJSRuntime* self = nsXPConnect::GetRuntimeInstance();
    if (!self)
        return;

#ifdef MOZ_CRASHREPORTER
    CrashReporter::SetGarbageCollecting(progress == JS::GC_CYCLE_BEGIN ||
                                        progress == JS::GC_SLICE_BEGIN);
#endif

    if (self->mPrevGCSliceCallback)
        (*self->mPrevGCSliceCallback)(rt, progress, desc);
}

void
XPCJSRuntime::CustomGCCallback(JSGCStatus status)
{
    nsTArray<xpcGCCallback> callbacks(extraGCCallbacks);
    for (uint32_t i = 0; i < callbacks.Length(); ++i)
        callbacks[i](status);
}

 void
XPCJSRuntime::FinalizeCallback(JSFreeOp* fop,
                               JSFinalizeStatus status,
                               bool isCompartmentGC,
                               void* data)
{
    XPCJSRuntime* self = nsXPConnect::GetRuntimeInstance();
    if (!self)
        return;

    switch (status) {
        case JSFINALIZE_GROUP_START:
        {
            MOZ_ASSERT(!self->mDoingFinalization, "bad state");

            MOZ_ASSERT(!self->mGCIsRunning, "bad state");
            self->mGCIsRunning = true;

            self->mDoingFinalization = true;
            break;
        }
        case JSFINALIZE_GROUP_END:
        {
            MOZ_ASSERT(self->mDoingFinalization, "bad state");
            self->mDoingFinalization = false;

            
            
            DoDeferredRelease(self->mWrappedJSToReleaseArray);

            
            XPCWrappedNativeScope::KillDyingScopes();

            MOZ_ASSERT(self->mGCIsRunning, "bad state");
            self->mGCIsRunning = false;

            break;
        }
        case JSFINALIZE_COLLECTION_END:
        {
            MOZ_ASSERT(!self->mGCIsRunning, "bad state");
            self->mGCIsRunning = true;

            
            

            
            XPCWrappedNativeScope::MarkAllWrappedNativesAndProtos();

            self->mDetachedWrappedNativeProtoMap->
                Enumerate(DetachedWrappedNativeProtoMarker, nullptr);

            
            
            
            
            
            

            
            
            if (!nsXPConnect::XPConnect()->IsShuttingDown()) {

                
                if (AutoMarkingPtr* roots = Get()->mAutoRoots)
                    roots->MarkAfterJSFinalizeAll();

                XPCCallContext* ccxp = XPCJSRuntime::Get()->GetCallContext();
                while (ccxp) {
                    
                    
                    
                    
                    if (ccxp->CanGetSet()) {
                        XPCNativeSet* set = ccxp->GetSet();
                        if (set)
                            set->Mark();
                    }
                    if (ccxp->CanGetInterface()) {
                        XPCNativeInterface* iface = ccxp->GetInterface();
                        if (iface)
                            iface->Mark();
                    }
                    ccxp = ccxp->GetPrevCallContext();
                }
            }

            
            
            
            
            
            
            
            
            
            
            
            void* sweepArg = isCompartmentGC ? UNMARK_ONLY : UNMARK_AND_SWEEP;

            
            
            
            if (!nsXPConnect::XPConnect()->IsShuttingDown()) {
                self->mNativeScriptableSharedMap->
                    Enumerate(JSClassSweeper, sweepArg);
            }

            if (!isCompartmentGC) {
                self->mClassInfo2NativeSetMap->
                    Enumerate(NativeUnMarkedSetRemover, nullptr);
            }

            self->mNativeSetMap->
                Enumerate(NativeSetSweeper, sweepArg);

            self->mIID2NativeInterfaceMap->
                Enumerate(NativeInterfaceSweeper, sweepArg);

#ifdef DEBUG
            XPCWrappedNativeScope::ASSERT_NoInterfaceSetsAreMarked();
#endif

            
            
            
            
            
            
            
            
            
            
            

            
            
            if (!nsXPConnect::XPConnect()->IsShuttingDown()) {
                

                XPCCallContext* ccxp = XPCJSRuntime::Get()->GetCallContext();
                while (ccxp) {
                    
                    
                    
                    
                    if (ccxp->CanGetTearOff()) {
                        XPCWrappedNativeTearOff* to =
                            ccxp->GetTearOff();
                        if (to)
                            to->Mark();
                    }
                    ccxp = ccxp->GetPrevCallContext();
                }

                
                XPCWrappedNativeScope::SweepAllWrappedNativeTearOffs();
            }

            
            
            
            
            
            
            
            
            
            
            
            
            

            self->mDyingWrappedNativeProtoMap->
                Enumerate(DyingProtoKiller, nullptr);

            MOZ_ASSERT(self->mGCIsRunning, "bad state");
            self->mGCIsRunning = false;

            break;
        }
    }
}

 void
XPCJSRuntime::WeakPointerCallback(JSRuntime* rt, void* data)
{
    
    

    XPCJSRuntime* self = static_cast<XPCJSRuntime*>(data);

    self->mWrappedJSMap->UpdateWeakPointersAfterGC(self);

    XPCWrappedNativeScope::UpdateWeakPointersAfterGC(self);
}

static void WatchdogMain(void* arg);
class Watchdog;
class WatchdogManager;
class AutoLockWatchdog {
    Watchdog* const mWatchdog;
  public:
    explicit AutoLockWatchdog(Watchdog* aWatchdog);
    ~AutoLockWatchdog();
};

class Watchdog
{
  public:
    explicit Watchdog(WatchdogManager* aManager)
      : mManager(aManager)
      , mLock(nullptr)
      , mWakeup(nullptr)
      , mThread(nullptr)
      , mHibernating(false)
      , mInitialized(false)
      , mShuttingDown(false)
      , mMinScriptRunTimeSeconds(1)
    {}
    ~Watchdog() { MOZ_ASSERT(!Initialized()); }

    WatchdogManager* Manager() { return mManager; }
    bool Initialized() { return mInitialized; }
    bool ShuttingDown() { return mShuttingDown; }
    PRLock* GetLock() { return mLock; }
    bool Hibernating() { return mHibernating; }
    void WakeUp()
    {
        MOZ_ASSERT(Initialized());
        MOZ_ASSERT(Hibernating());
        mHibernating = false;
        PR_NotifyCondVar(mWakeup);
    }

    
    
    

    void Init()
    {
        MOZ_ASSERT(NS_IsMainThread());
        mLock = PR_NewLock();
        if (!mLock)
            NS_RUNTIMEABORT("PR_NewLock failed.");
        mWakeup = PR_NewCondVar(mLock);
        if (!mWakeup)
            NS_RUNTIMEABORT("PR_NewCondVar failed.");

        {
            AutoLockWatchdog lock(this);

            mThread = PR_CreateThread(PR_USER_THREAD, WatchdogMain, this,
                                      PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                                      PR_UNJOINABLE_THREAD, 0);
            if (!mThread)
                NS_RUNTIMEABORT("PR_CreateThread failed!");

            
            
            
            mInitialized = true;
        }
    }

    void Shutdown()
    {
        MOZ_ASSERT(NS_IsMainThread());
        MOZ_ASSERT(Initialized());
        {   
            AutoLockWatchdog lock(this);

            
            mShuttingDown = true;

            
            PR_NotifyCondVar(mWakeup);
            PR_WaitCondVar(mWakeup, PR_INTERVAL_NO_TIMEOUT);
            MOZ_ASSERT(!mShuttingDown);
        }

        
        mThread = nullptr;
        PR_DestroyCondVar(mWakeup);
        mWakeup = nullptr;
        PR_DestroyLock(mLock);
        mLock = nullptr;

        
        mInitialized = false;
    }

    void SetMinScriptRunTimeSeconds(int32_t seconds)
    {
        
        
        MOZ_ASSERT(seconds > 0);
        mMinScriptRunTimeSeconds = seconds;
    }

    
    
    

    void Hibernate()
    {
        MOZ_ASSERT(!NS_IsMainThread());
        mHibernating = true;
        Sleep(PR_INTERVAL_NO_TIMEOUT);
    }
    void Sleep(PRIntervalTime timeout)
    {
        MOZ_ASSERT(!NS_IsMainThread());
        MOZ_ALWAYS_TRUE(PR_WaitCondVar(mWakeup, timeout) == PR_SUCCESS);
    }
    void Finished()
    {
        MOZ_ASSERT(!NS_IsMainThread());
        mShuttingDown = false;
        PR_NotifyCondVar(mWakeup);
    }

    int32_t MinScriptRunTimeSeconds()
    {
        return mMinScriptRunTimeSeconds;
    }

  private:
    WatchdogManager* mManager;

    PRLock* mLock;
    PRCondVar* mWakeup;
    PRThread* mThread;
    bool mHibernating;
    bool mInitialized;
    bool mShuttingDown;
    mozilla::Atomic<int32_t> mMinScriptRunTimeSeconds;
};

#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif

#define PREF_MAX_SCRIPT_RUN_TIME_CHILD "dom.max_child_script_run_time"
#define PREF_MAX_SCRIPT_RUN_TIME_CONTENT "dom.max_script_run_time"
#define PREF_MAX_SCRIPT_RUN_TIME_CHROME "dom.max_chrome_script_run_time"

class WatchdogManager : public nsIObserver
{
  public:

    NS_DECL_ISUPPORTS
    explicit WatchdogManager(XPCJSRuntime* aRuntime) : mRuntime(aRuntime)
                                                     , mRuntimeState(RUNTIME_INACTIVE)
    {
        
        PodArrayZero(mTimestamps);
        mTimestamps[TimestampRuntimeStateChange] = PR_Now();

        
        RefreshWatchdog();

        
        mozilla::Preferences::AddStrongObserver(this, "dom.use_watchdog");
        mozilla::Preferences::AddStrongObserver(this, PREF_MAX_SCRIPT_RUN_TIME_CONTENT);
        mozilla::Preferences::AddStrongObserver(this, PREF_MAX_SCRIPT_RUN_TIME_CHROME);
        mozilla::Preferences::AddStrongObserver(this, PREF_MAX_SCRIPT_RUN_TIME_CHILD);
    }

  protected:

    virtual ~WatchdogManager()
    {
        
        
        
        MOZ_ASSERT(!mWatchdog);
        mozilla::Preferences::RemoveObserver(this, "dom.use_watchdog");
        mozilla::Preferences::RemoveObserver(this, PREF_MAX_SCRIPT_RUN_TIME_CONTENT);
        mozilla::Preferences::RemoveObserver(this, PREF_MAX_SCRIPT_RUN_TIME_CHROME);
        mozilla::Preferences::RemoveObserver(this, PREF_MAX_SCRIPT_RUN_TIME_CHILD);
    }

  public:

    NS_IMETHOD Observe(nsISupports* aSubject, const char* aTopic,
                       const char16_t* aData) override
    {
        RefreshWatchdog();
        return NS_OK;
    }

    
    
    
    void
    RecordRuntimeActivity(bool active)
    {
        
        MOZ_ASSERT(NS_IsMainThread());
        Maybe<AutoLockWatchdog> lock;
        if (mWatchdog)
            lock.emplace(mWatchdog);

        
        mTimestamps[TimestampRuntimeStateChange] = PR_Now();
        mRuntimeState = active ? RUNTIME_ACTIVE : RUNTIME_INACTIVE;

        
        
        if (active && mWatchdog && mWatchdog->Hibernating())
            mWatchdog->WakeUp();
    }
    bool IsRuntimeActive() { return mRuntimeState == RUNTIME_ACTIVE; }
    PRTime TimeSinceLastRuntimeStateChange()
    {
        return PR_Now() - GetTimestamp(TimestampRuntimeStateChange);
    }

    
    
    void RecordTimestamp(WatchdogTimestampCategory aCategory)
    {
        
        Maybe<AutoLockWatchdog> maybeLock;
        if (NS_IsMainThread() && mWatchdog)
            maybeLock.emplace(mWatchdog);
        mTimestamps[aCategory] = PR_Now();
    }
    PRTime GetTimestamp(WatchdogTimestampCategory aCategory)
    {
        
        Maybe<AutoLockWatchdog> maybeLock;
        if (NS_IsMainThread() && mWatchdog)
            maybeLock.emplace(mWatchdog);
        return mTimestamps[aCategory];
    }

    XPCJSRuntime* Runtime() { return mRuntime; }
    Watchdog* GetWatchdog() { return mWatchdog; }

    void RefreshWatchdog()
    {
        bool wantWatchdog = Preferences::GetBool("dom.use_watchdog", true);
        if (wantWatchdog != !!mWatchdog) {
            if (wantWatchdog)
                StartWatchdog();
            else
                StopWatchdog();
        }

        if (mWatchdog) {
            int32_t contentTime = Preferences::GetInt(PREF_MAX_SCRIPT_RUN_TIME_CONTENT, 10);
            if (contentTime <= 0)
                contentTime = INT32_MAX;
            int32_t chromeTime = Preferences::GetInt(PREF_MAX_SCRIPT_RUN_TIME_CHROME, 20);
            if (chromeTime <= 0)
                chromeTime = INT32_MAX;
            int32_t childTime = Preferences::GetInt(PREF_MAX_SCRIPT_RUN_TIME_CHILD, 3);
            if (childTime <= 0)
                childTime = INT32_MAX;
            mWatchdog->SetMinScriptRunTimeSeconds(std::min(std::min(contentTime, chromeTime), childTime));
        }
    }

    void StartWatchdog()
    {
        MOZ_ASSERT(!mWatchdog);
        mWatchdog = new Watchdog(this);
        mWatchdog->Init();
    }

    void StopWatchdog()
    {
        MOZ_ASSERT(mWatchdog);
        mWatchdog->Shutdown();
        mWatchdog = nullptr;
    }

  private:
    XPCJSRuntime* mRuntime;
    nsAutoPtr<Watchdog> mWatchdog;

    enum { RUNTIME_ACTIVE, RUNTIME_INACTIVE } mRuntimeState;
    PRTime mTimestamps[TimestampCount];
};

NS_IMPL_ISUPPORTS(WatchdogManager, nsIObserver)

AutoLockWatchdog::AutoLockWatchdog(Watchdog* aWatchdog) : mWatchdog(aWatchdog)
{
    PR_Lock(mWatchdog->GetLock());
}

AutoLockWatchdog::~AutoLockWatchdog()
{
    PR_Unlock(mWatchdog->GetLock());
}

static void
WatchdogMain(void* arg)
{
    PR_SetCurrentThreadName("JS Watchdog");

#ifdef MOZ_NUWA_PROCESS
    if (IsNuwaProcess()) {
        NuwaMarkCurrentThread(nullptr, nullptr);
        NuwaFreezeCurrentThread();
    }
#endif

    Watchdog* self = static_cast<Watchdog*>(arg);
    WatchdogManager* manager = self->Manager();

    
    AutoLockWatchdog lock(self);

    MOZ_ASSERT(self->Initialized());
    MOZ_ASSERT(!self->ShuttingDown());
    while (!self->ShuttingDown()) {
        
        if (manager->IsRuntimeActive() ||
            manager->TimeSinceLastRuntimeStateChange() <= PRTime(2*PR_USEC_PER_SEC))
        {
            self->Sleep(PR_TicksPerSecond());
        } else {
            manager->RecordTimestamp(TimestampWatchdogHibernateStart);
            self->Hibernate();
            manager->RecordTimestamp(TimestampWatchdogHibernateStop);
        }

        
        manager->RecordTimestamp(TimestampWatchdogWakeup);

        
        
        

        
        
        
        
        
        
        
        
        
        
        PRTime usecs = self->MinScriptRunTimeSeconds() * PR_USEC_PER_SEC / 2;
        if (manager->IsRuntimeActive() &&
            manager->TimeSinceLastRuntimeStateChange() >= usecs)
        {
            bool debuggerAttached = false;
            nsCOMPtr<nsIDebug2> dbg = do_GetService("@mozilla.org/xpcom/debug;1");
            if (dbg)
                dbg->GetIsDebuggerAttached(&debuggerAttached);
            if (!debuggerAttached)
                JS_RequestInterruptCallback(manager->Runtime()->Runtime());
        }
    }

    
    self->Finished();
}

PRTime
XPCJSRuntime::GetWatchdogTimestamp(WatchdogTimestampCategory aCategory)
{
    return mWatchdogManager->GetTimestamp(aCategory);
}

void
xpc::SimulateActivityCallback(bool aActive)
{
    XPCJSRuntime::ActivityCallback(XPCJSRuntime::Get(), aActive);
}


JSContext*
XPCJSRuntime::DefaultJSContextCallback(JSRuntime* rt)
{
    MOZ_ASSERT(rt == Get()->Runtime());
    return Get()->GetJSContextStack()->GetSafeJSContext();
}


void
XPCJSRuntime::ActivityCallback(void* arg, bool active)
{
    if (!active) {
        ProcessHangMonitor::ClearHang();
    }

    XPCJSRuntime* self = static_cast<XPCJSRuntime*>(arg);
    self->mWatchdogManager->RecordRuntimeActivity(active);
}






void
XPCJSRuntime::CTypesActivityCallback(JSContext* cx, js::CTypesActivityType type)
{
  if (type == js::CTYPES_CALLBACK_BEGIN) {
    if (!xpc::PushJSContextNoScriptContext(cx))
      MOZ_CRASH();
  } else if (type == js::CTYPES_CALLBACK_END) {
    xpc::PopJSContextNoScriptContext();
  }
}


bool
XPCJSRuntime::InterruptCallback(JSContext* cx)
{
    XPCJSRuntime* self = XPCJSRuntime::Get();

    
    
    
    if (self->mSlowScriptCheckpoint.IsNull()) {
        self->mSlowScriptCheckpoint = TimeStamp::NowLoRes();
        self->mSlowScriptSecondHalf = false;
        return true;
    }

    
    
    if (!nsContentUtils::IsInitialized())
        return true;

    bool contentProcess = XRE_GetProcessType() == GeckoProcessType_Content;

    
    
    
    TimeDuration duration = TimeStamp::NowLoRes() - self->mSlowScriptCheckpoint;
    bool chrome = nsContentUtils::IsCallerChrome();
    const char* prefName = contentProcess ? PREF_MAX_SCRIPT_RUN_TIME_CHILD
                                 : chrome ? PREF_MAX_SCRIPT_RUN_TIME_CHROME
                                          : PREF_MAX_SCRIPT_RUN_TIME_CONTENT;
    int32_t limit = Preferences::GetInt(prefName, chrome ? 20 : 10);

    
    if (limit == 0 || duration.ToSeconds() < limit / 2.0)
        return true;

    
    
    
    if (!self->mSlowScriptSecondHalf) {
        self->mSlowScriptCheckpoint = TimeStamp::NowLoRes();
        self->mSlowScriptSecondHalf = true;
        return true;
    }

    
    
    

    
    
    RootedObject global(cx, JS::CurrentGlobalOrNull(cx));
    nsRefPtr<nsGlobalWindow> win = WindowOrNull(global);
    if (!win && IsSandbox(global)) {
        
        
        
        JS::Rooted<JSObject*> proto(cx);
        if (!JS_GetPrototype(cx, global, &proto))
            return false;
        if (proto && IsSandboxPrototypeProxy(proto) &&
            (proto = js::CheckedUnwrap(proto,  false)))
        {
            win = WindowGlobalOrNull(proto);
        }
    }

    if (!win) {
        NS_WARNING("No active window");
        return true;
    }

    MOZ_ASSERT(!win->IsDying());

    if (win->GetIsPrerendered()) {
        
        
        mozilla::dom::HandlePrerenderingViolation(win);
        return false;
    }

    
    nsGlobalWindow::SlowScriptResponse response = win->ShowSlowScriptDialog();
    if (response == nsGlobalWindow::KillSlowScript)
        return false;

    
    
    if (response != nsGlobalWindow::ContinueSlowScriptAndKeepNotifying)
        self->mSlowScriptCheckpoint = TimeStamp::NowLoRes();

    if (response == nsGlobalWindow::AlwaysContinueSlowScript)
        Preferences::SetInt(prefName, 0);

    return true;
}

void
XPCJSRuntime::CustomOutOfMemoryCallback()
{
    if (!Preferences::GetBool("memory.dump_reports_on_oom")) {
        return;
    }

    nsCOMPtr<nsIMemoryInfoDumper> dumper =
        do_GetService("@mozilla.org/memory-info-dumper;1");
    if (!dumper) {
        return;
    }

    
    dumper->DumpMemoryInfoToTempDir(NS_LITERAL_STRING("due-to-JS-OOM"),
                                     false,
                                     false);
}

void
XPCJSRuntime::CustomLargeAllocationFailureCallback()
{
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os) {
        os->NotifyObservers(nullptr, "memory-pressure", MOZ_UTF16("heap-minimize"));
    }
}

size_t
XPCJSRuntime::SizeOfIncludingThis(MallocSizeOf mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    n += mWrappedJSMap->SizeOfIncludingThis(mallocSizeOf);
    n += mIID2NativeInterfaceMap->SizeOfIncludingThis(mallocSizeOf);
    n += mClassInfo2NativeSetMap->ShallowSizeOfIncludingThis(mallocSizeOf);
    n += mNativeSetMap->SizeOfIncludingThis(mallocSizeOf);

    n += CycleCollectedJSRuntime::SizeOfExcludingThis(mallocSizeOf);

    
    
    

    return n;
}



static PLDHashOperator
DetachedWrappedNativeProtoShutdownMarker(PLDHashTable* table, PLDHashEntryHdr* hdr,
                                         uint32_t number, void* arg)
{
    XPCWrappedNativeProto* proto =
        (XPCWrappedNativeProto*)((PLDHashEntryStub*)hdr)->key;

    proto->SystemIsBeingShutDown();
    return PL_DHASH_NEXT;
}

void XPCJSRuntime::DestroyJSContextStack()
{
    delete mJSContextStack;
    mJSContextStack = nullptr;
}

void XPCJSRuntime::SystemIsBeingShutDown()
{
    if (mDetachedWrappedNativeProtoMap)
        mDetachedWrappedNativeProtoMap->
            Enumerate(DetachedWrappedNativeProtoShutdownMarker, nullptr);
}

#define JS_OPTIONS_DOT_STR "javascript.options."

static void
ReloadPrefsCallback(const char* pref, void* data)
{
    XPCJSRuntime* runtime = reinterpret_cast<XPCJSRuntime*>(data);
    JSRuntime* rt = runtime->Runtime();

    bool safeMode = false;
    nsCOMPtr<nsIXULRuntime> xr = do_GetService("@mozilla.org/xre/runtime;1");
    if (xr) {
        xr->GetInSafeMode(&safeMode);
    }

    bool useBaseline = Preferences::GetBool(JS_OPTIONS_DOT_STR "baselinejit") && !safeMode;
    bool useIon = Preferences::GetBool(JS_OPTIONS_DOT_STR "ion") && !safeMode;
    bool useAsmJS = Preferences::GetBool(JS_OPTIONS_DOT_STR "asmjs") && !safeMode;
    bool useNativeRegExp = Preferences::GetBool(JS_OPTIONS_DOT_STR "native_regexp") && !safeMode;

    bool parallelParsing = Preferences::GetBool(JS_OPTIONS_DOT_STR "parallel_parsing");
    bool offthreadIonCompilation = Preferences::GetBool(JS_OPTIONS_DOT_STR
                                                       "ion.offthread_compilation");
    bool useBaselineEager = Preferences::GetBool(JS_OPTIONS_DOT_STR
                                                 "baselinejit.unsafe_eager_compilation");
    bool useIonEager = Preferences::GetBool(JS_OPTIONS_DOT_STR "ion.unsafe_eager_compilation");

    sDiscardSystemSource = Preferences::GetBool(JS_OPTIONS_DOT_STR "discardSystemSource");

    bool werror = Preferences::GetBool(JS_OPTIONS_DOT_STR "werror");

    bool extraWarnings = Preferences::GetBool(JS_OPTIONS_DOT_STR "strict");
#ifdef DEBUG
    sExtraWarningsForSystemJS = Preferences::GetBool(JS_OPTIONS_DOT_STR "strict.debug");
#endif

    JS::RuntimeOptionsRef(rt).setBaseline(useBaseline)
                             .setIon(useIon)
                             .setAsmJS(useAsmJS)
                             .setNativeRegExp(useNativeRegExp)
                             .setWerror(werror)
                             .setExtraWarnings(extraWarnings);

    JS_SetParallelParsingEnabled(rt, parallelParsing);
    JS_SetOffthreadIonCompilationEnabled(rt, offthreadIonCompilation);
    JS_SetGlobalJitCompilerOption(rt, JSJITCOMPILER_BASELINE_WARMUP_TRIGGER,
                                  useBaselineEager ? 0 : -1);
    JS_SetGlobalJitCompilerOption(rt, JSJITCOMPILER_ION_WARMUP_TRIGGER,
                                  useIonEager ? 0 : -1);
}

XPCJSRuntime::~XPCJSRuntime()
{
    
    
    
    
    js::SetActivityCallback(Runtime(), nullptr, nullptr);
    JS_RemoveFinalizeCallback(Runtime(), FinalizeCallback);
    JS_RemoveWeakPointerCallback(Runtime(), WeakPointerCallback);

    
    
    SetPendingException(nullptr);

    JS::SetGCSliceCallback(Runtime(), mPrevGCSliceCallback);

    xpc_DelocalizeRuntime(Runtime());

    if (mWatchdogManager->GetWatchdog())
        mWatchdogManager->StopWatchdog();

    if (mCallContext)
        mCallContext->SystemIsBeingShutDown();

    auto rtPrivate = static_cast<PerThreadAtomCache*>(JS_GetRuntimePrivate(Runtime()));
    delete rtPrivate;
    JS_SetRuntimePrivate(Runtime(), nullptr);

    
    if (mWrappedJSMap) {
        mWrappedJSMap->ShutdownMarker();
        delete mWrappedJSMap;
        mWrappedJSMap = nullptr;
    }

    if (mWrappedJSClassMap) {
        delete mWrappedJSClassMap;
        mWrappedJSClassMap = nullptr;
    }

    if (mIID2NativeInterfaceMap) {
        delete mIID2NativeInterfaceMap;
        mIID2NativeInterfaceMap = nullptr;
    }

    if (mClassInfo2NativeSetMap) {
        delete mClassInfo2NativeSetMap;
        mClassInfo2NativeSetMap = nullptr;
    }

    if (mNativeSetMap) {
        delete mNativeSetMap;
        mNativeSetMap = nullptr;
    }

    if (mThisTranslatorMap) {
        delete mThisTranslatorMap;
        mThisTranslatorMap = nullptr;
    }

    if (mNativeScriptableSharedMap) {
        delete mNativeScriptableSharedMap;
        mNativeScriptableSharedMap = nullptr;
    }

    if (mDyingWrappedNativeProtoMap) {
        delete mDyingWrappedNativeProtoMap;
        mDyingWrappedNativeProtoMap = nullptr;
    }

    if (mDetachedWrappedNativeProtoMap) {
        delete mDetachedWrappedNativeProtoMap;
        mDetachedWrappedNativeProtoMap = nullptr;
    }

#ifdef MOZ_ENABLE_PROFILER_SPS
    
    if (PseudoStack* stack = mozilla_get_pseudo_stack())
        stack->sampleRuntime(nullptr);
#endif

    Preferences::UnregisterCallback(ReloadPrefsCallback, JS_OPTIONS_DOT_STR, this);
}



static void
GetCompartmentName(JSCompartment* c, nsCString& name, int* anonymizeID,
                   bool replaceSlashes)
{
    if (js::IsAtomsCompartment(c)) {
        name.AssignLiteral("atoms");
    } else if (*anonymizeID && !js::IsSystemCompartment(c)) {
        name.AppendPrintf("<anonymized-%d>", *anonymizeID);
        *anonymizeID += 1;
    } else if (JSPrincipals* principals = JS_GetCompartmentPrincipals(c)) {
        nsJSPrincipals::get(principals)->GetScriptLocation(name);

        
        
        
        
        CompartmentPrivate* compartmentPrivate = CompartmentPrivate::Get(c);
        if (compartmentPrivate) {
            const nsACString& location = compartmentPrivate->GetLocation();
            if (!location.IsEmpty() && !location.Equals(name)) {
                name.AppendLiteral(", ");
                name.Append(location);
            }
        }

        if (*anonymizeID) {
            
            
            static const char* filePrefix = "file://";
            int filePos = name.Find(filePrefix);
            if (filePos >= 0) {
                int pathPos = filePos + strlen(filePrefix);
                int lastSlashPos = -1;
                for (int i = pathPos; i < int(name.Length()); i++) {
                    if (name[i] == '/' || name[i] == '\\') {
                        lastSlashPos = i;
                    }
                }
                if (lastSlashPos != -1) {
                    name.ReplaceASCII(pathPos, lastSlashPos - pathPos,
                                      "<anonymized>");
                } else {
                    
                    
                    name.Truncate(pathPos);
                    name += "<anonymized?!>";
                }
            }

            
            
            
            
            static const char* ownedByPrefix =
                "inProcessTabChildGlobal?ownedBy=";
            int ownedByPos = name.Find(ownedByPrefix);
            if (ownedByPos >= 0) {
                const char* chrome = "chrome:";
                int ownerPos = ownedByPos + strlen(ownedByPrefix);
                const nsDependentCSubstring& ownerFirstPart =
                    Substring(name, ownerPos, strlen(chrome));
                if (!ownerFirstPart.EqualsASCII(chrome)) {
                    name.Truncate(ownerPos);
                    name += "<anonymized>";
                }
            }
        }

        
        
        
        if (replaceSlashes)
            name.ReplaceChar('/', '\\');
    } else {
        name.AssignLiteral("null-principal");
    }
}

static int64_t
JSMainRuntimeGCHeapDistinguishedAmount()
{
    JSRuntime* rt = nsXPConnect::GetRuntimeInstance()->Runtime();
    return int64_t(JS_GetGCParameter(rt, JSGC_TOTAL_CHUNKS)) *
           js::gc::ChunkSize;
}

static int64_t
JSMainRuntimeTemporaryPeakDistinguishedAmount()
{
    JSRuntime* rt = nsXPConnect::GetRuntimeInstance()->Runtime();
    return JS::PeakSizeOfTemporary(rt);
}

static int64_t
JSMainRuntimeCompartmentsSystemDistinguishedAmount()
{
    JSRuntime* rt = nsXPConnect::GetRuntimeInstance()->Runtime();
    return JS::SystemCompartmentCount(rt);
}

static int64_t
JSMainRuntimeCompartmentsUserDistinguishedAmount()
{
    JSRuntime* rt = nsXPConnect::GetRuntimeInstance()->Runtime();
    return JS::UserCompartmentCount(rt);
}

class JSMainRuntimeTemporaryPeakReporter final : public nsIMemoryReporter
{
    ~JSMainRuntimeTemporaryPeakReporter() {}

  public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD CollectReports(nsIHandleReportCallback* aHandleReport,
                              nsISupports* aData, bool aAnonymize) override
    {
        return MOZ_COLLECT_REPORT("js-main-runtime-temporary-peak",
            KIND_OTHER, UNITS_BYTES,
            JSMainRuntimeTemporaryPeakDistinguishedAmount(),
            "Peak transient data size in the main JSRuntime (the current size "
            "of which is reported as "
            "'explicit/js-non-window/runtime/temporary').");
    }
};

NS_IMPL_ISUPPORTS(JSMainRuntimeTemporaryPeakReporter, nsIMemoryReporter)






#define SUNDRIES_THRESHOLD js::MemoryReportingSundriesThreshold()

#define REPORT(_path, _kind, _units, _amount, _desc)                          \
    do {                                                                      \
        nsresult rv;                                                          \
        rv = cb->Callback(EmptyCString(), _path,                              \
                          nsIMemoryReporter::_kind,                           \
                          nsIMemoryReporter::_units,                          \
                          _amount,                                            \
                          NS_LITERAL_CSTRING(_desc),                          \
                          closure);                                           \
        NS_ENSURE_SUCCESS(rv, rv);                                            \
    } while (0)

#define REPORT_BYTES(_path, _kind, _amount, _desc)                            \
    REPORT(_path, _kind, UNITS_BYTES, _amount, _desc);

#define REPORT_GC_BYTES(_path, _amount, _desc)                                \
    do {                                                                      \
        size_t amount = _amount;  /* evaluate _amount only once */            \
        nsresult rv;                                                          \
        rv = cb->Callback(EmptyCString(), _path,                              \
                          nsIMemoryReporter::KIND_NONHEAP,                    \
                          nsIMemoryReporter::UNITS_BYTES, amount,             \
                          NS_LITERAL_CSTRING(_desc), closure);                \
        NS_ENSURE_SUCCESS(rv, rv);                                            \
        gcTotal += amount;                                                    \
    } while (0)


#define ZCREPORT_BYTES(_path, _amount, _desc)                                 \
    do {                                                                      \
        /* Assign _descLiteral plus "" into a char* to prove that it's */     \
        /* actually a literal. */                                             \
        size_t amount = _amount;  /* evaluate _amount only once */            \
        if (amount >= SUNDRIES_THRESHOLD) {                                   \
            nsresult rv;                                                      \
            rv = cb->Callback(EmptyCString(), _path,                          \
                              nsIMemoryReporter::KIND_HEAP,                   \
                              nsIMemoryReporter::UNITS_BYTES, amount,         \
                              NS_LITERAL_CSTRING(_desc), closure);            \
            NS_ENSURE_SUCCESS(rv, rv);                                        \
        } else {                                                              \
            sundriesMallocHeap += amount;                                     \
        }                                                                     \
    } while (0)


#define ZCREPORT_GC_BYTES(_path, _amount, _desc)                              \
    do {                                                                      \
        size_t amount = _amount;  /* evaluate _amount only once */            \
        if (amount >= SUNDRIES_THRESHOLD) {                                   \
            nsresult rv;                                                      \
            rv = cb->Callback(EmptyCString(), _path,                          \
                              nsIMemoryReporter::KIND_NONHEAP,                \
                              nsIMemoryReporter::UNITS_BYTES, amount,         \
                              NS_LITERAL_CSTRING(_desc), closure);            \
            NS_ENSURE_SUCCESS(rv, rv);                                        \
            gcTotal += amount;                                                \
        } else {                                                              \
            sundriesGCHeap += amount;                                         \
        }                                                                     \
    } while (0)


#define RREPORT_BYTES(_path, _kind, _amount, _desc)                           \
    do {                                                                      \
        size_t amount = _amount;  /* evaluate _amount only once */            \
        nsresult rv;                                                          \
        rv = cb->Callback(EmptyCString(), _path,                              \
                          nsIMemoryReporter::_kind,                           \
                          nsIMemoryReporter::UNITS_BYTES, amount,             \
                          NS_LITERAL_CSTRING(_desc), closure);                \
        NS_ENSURE_SUCCESS(rv, rv);                                            \
        rtTotal += amount;                                                    \
    } while (0)


#define MREPORT_BYTES(_path, _kind, _amount, _desc)                           \
    do {                                                                      \
        size_t amount = _amount;  /* evaluate _amount only once */            \
        nsresult rv;                                                          \
        rv = cb->Callback(EmptyCString(), _path,                              \
                          nsIMemoryReporter::_kind,                           \
                          nsIMemoryReporter::UNITS_BYTES, amount,             \
                          NS_LITERAL_CSTRING(_desc), closure);                \
        NS_ENSURE_SUCCESS(rv, rv);                                            \
        gcThingTotal += amount;                                               \
    } while (0)

MOZ_DEFINE_MALLOC_SIZE_OF(JSMallocSizeOf)

namespace xpc {

static nsresult
ReportZoneStats(const JS::ZoneStats& zStats,
                const xpc::ZoneStatsExtras& extras,
                nsIMemoryReporterCallback* cb,
                nsISupports* closure,
                bool anonymize,
                size_t* gcTotalOut = nullptr)
{
    const nsAutoCString& pathPrefix = extras.pathPrefix;
    size_t gcTotal = 0, sundriesGCHeap = 0, sundriesMallocHeap = 0;

    MOZ_ASSERT(!gcTotalOut == zStats.isTotals);

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("symbols/gc-heap"),
        zStats.symbolsGCHeap,
        "Symbols.");

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap-arena-admin"),
        zStats.gcHeapArenaAdmin,
        "Bookkeeping information and alignment padding within GC arenas.");

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("unused-gc-things"),
        zStats.unusedGCThings.totalSize(),
        "Unused GC thing cells within non-empty arenas.");

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("lazy-scripts/gc-heap"),
        zStats.lazyScriptsGCHeap,
        "Scripts that haven't executed yet.");

    ZCREPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("lazy-scripts/malloc-heap"),
        zStats.lazyScriptsMallocHeap,
        "Lazy script tables containing free variables or inner functions.");

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("jit-codes-gc-heap"),
        zStats.jitCodesGCHeap,
        "References to executable code pools used by the JITs.");

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("object-groups/gc-heap"),
        zStats.objectGroupsGCHeap,
        "Classification and type inference information about objects.");

    ZCREPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("object-groups/malloc-heap"),
        zStats.objectGroupsMallocHeap,
        "Object group addenda.");

    ZCREPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("type-pool"),
        zStats.typePool,
        "Type sets and related data.");

    ZCREPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("baseline/optimized-stubs"),
        zStats.baselineStubsOptimized,
        "The Baseline JIT's optimized IC stubs (excluding code).");

    size_t stringsNotableAboutMemoryGCHeap = 0;
    size_t stringsNotableAboutMemoryMallocHeap = 0;

    #define MAYBE_INLINE \
        "The characters may be inline or on the malloc heap."
    #define MAYBE_OVERALLOCATED \
        "Sometimes over-allocated to simplify string concatenation."

    for (size_t i = 0; i < zStats.notableStrings.length(); i++) {
        const JS::NotableStringInfo& info = zStats.notableStrings[i];

        MOZ_ASSERT(!zStats.isTotals);

        
        
        
        MOZ_ASSERT(!anonymize);

        nsDependentCString notableString(info.buffer);

        
        
        
        
        
        
        
#       define STRING_LENGTH "string(length="
        if (FindInReadable(NS_LITERAL_CSTRING(STRING_LENGTH), notableString)) {
            stringsNotableAboutMemoryGCHeap += info.gcHeapLatin1;
            stringsNotableAboutMemoryGCHeap += info.gcHeapTwoByte;
            stringsNotableAboutMemoryMallocHeap += info.mallocHeapLatin1;
            stringsNotableAboutMemoryMallocHeap += info.mallocHeapTwoByte;
            continue;
        }

        
        
        
        nsCString escapedString(notableString);
        escapedString.ReplaceSubstring("/", "\\");

        bool truncated = notableString.Length() < info.length;

        nsCString path = pathPrefix +
            nsPrintfCString("strings/" STRING_LENGTH "%d, copies=%d, \"%s\"%s)/",
                            info.length, info.numCopies, escapedString.get(),
                            truncated ? " (truncated)" : "");

        if (info.gcHeapLatin1 > 0) {
            REPORT_GC_BYTES(path + NS_LITERAL_CSTRING("gc-heap/latin1"),
                info.gcHeapLatin1,
                "Latin1 strings. " MAYBE_INLINE);
        }

        if (info.gcHeapTwoByte > 0) {
            REPORT_GC_BYTES(path + NS_LITERAL_CSTRING("gc-heap/two-byte"),
                info.gcHeapTwoByte,
                "TwoByte strings. " MAYBE_INLINE);
        }

        if (info.mallocHeapLatin1 > 0) {
            REPORT_BYTES(path + NS_LITERAL_CSTRING("malloc-heap/latin1"),
                KIND_HEAP, info.mallocHeapLatin1,
                "Non-inline Latin1 string characters. " MAYBE_OVERALLOCATED);
        }

        if (info.mallocHeapTwoByte > 0) {
            REPORT_BYTES(path + NS_LITERAL_CSTRING("malloc-heap/two-byte"),
                KIND_HEAP, info.mallocHeapTwoByte,
                "Non-inline TwoByte string characters. " MAYBE_OVERALLOCATED);
        }
    }

    nsCString nonNotablePath = pathPrefix;
    nonNotablePath += (zStats.isTotals || anonymize)
                    ? NS_LITERAL_CSTRING("strings/")
                    : NS_LITERAL_CSTRING("strings/string(<non-notable strings>)/");

    if (zStats.stringInfo.gcHeapLatin1 > 0) {
        REPORT_GC_BYTES(nonNotablePath + NS_LITERAL_CSTRING("gc-heap/latin1"),
            zStats.stringInfo.gcHeapLatin1,
            "Latin1 strings. " MAYBE_INLINE);
    }

    if (zStats.stringInfo.gcHeapTwoByte > 0) {
        REPORT_GC_BYTES(nonNotablePath + NS_LITERAL_CSTRING("gc-heap/two-byte"),
            zStats.stringInfo.gcHeapTwoByte,
            "TwoByte strings. " MAYBE_INLINE);
    }

    if (zStats.stringInfo.mallocHeapLatin1 > 0) {
        REPORT_BYTES(nonNotablePath + NS_LITERAL_CSTRING("malloc-heap/latin1"),
            KIND_HEAP, zStats.stringInfo.mallocHeapLatin1,
            "Non-inline Latin1 string characters. " MAYBE_OVERALLOCATED);
    }

    if (zStats.stringInfo.mallocHeapTwoByte > 0) {
        REPORT_BYTES(nonNotablePath + NS_LITERAL_CSTRING("malloc-heap/two-byte"),
            KIND_HEAP, zStats.stringInfo.mallocHeapTwoByte,
            "Non-inline TwoByte string characters. " MAYBE_OVERALLOCATED);
    }

    if (stringsNotableAboutMemoryGCHeap > 0) {
        MOZ_ASSERT(!zStats.isTotals);
        REPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("strings/string(<about-memory>)/gc-heap"),
            stringsNotableAboutMemoryGCHeap,
            "Strings that contain the characters '" STRING_LENGTH "', which "
            "are probably from about:memory itself." MAYBE_INLINE
            " We filter them out rather than display them, because displaying "
            "them would create even more such strings every time about:memory "
            "is refreshed.");
    }

    if (stringsNotableAboutMemoryMallocHeap > 0) {
        MOZ_ASSERT(!zStats.isTotals);
        REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("strings/string(<about-memory>)/malloc-heap"),
            KIND_HEAP, stringsNotableAboutMemoryMallocHeap,
            "Non-inline string characters of strings that contain the "
            "characters '" STRING_LENGTH "', which are probably from "
            "about:memory itself. " MAYBE_OVERALLOCATED
            " We filter them out rather than display them, because displaying "
            "them would create even more such strings every time about:memory "
            "is refreshed.");
    }

    if (sundriesGCHeap > 0) {
        
        REPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("sundries/gc-heap"),
            sundriesGCHeap,
            "The sum of all 'gc-heap' measurements that are too small to be "
            "worth showing individually.");
    }

    if (sundriesMallocHeap > 0) {
        
        REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("sundries/malloc-heap"),
            KIND_HEAP, sundriesMallocHeap,
            "The sum of all 'malloc-heap' measurements that are too small to "
            "be worth showing individually.");
    }

    if (gcTotalOut)
        *gcTotalOut += gcTotal;

    return NS_OK;

#   undef STRING_LENGTH
}

static nsresult
ReportClassStats(const ClassInfo& classInfo, const nsACString& path,
                 nsIHandleReportCallback* cb, nsISupports* closure,
                 size_t& gcTotal)
{
    
    

    if (classInfo.objectsGCHeap > 0) {
        REPORT_GC_BYTES(path + NS_LITERAL_CSTRING("objects/gc-heap"),
            classInfo.objectsGCHeap,
            "Objects, including fixed slots.");
    }

    if (classInfo.objectsMallocHeapSlots > 0) {
        REPORT_BYTES(path + NS_LITERAL_CSTRING("objects/malloc-heap/slots"),
            KIND_HEAP, classInfo.objectsMallocHeapSlots,
            "Non-fixed object slots.");
    }

    if (classInfo.objectsMallocHeapElementsNonAsmJS > 0) {
        REPORT_BYTES(path + NS_LITERAL_CSTRING("objects/malloc-heap/elements/non-asm.js"),
            KIND_HEAP, classInfo.objectsMallocHeapElementsNonAsmJS,
            "Non-asm.js indexed elements.");
    }

    
    
    
    
    
    size_t mallocHeapElementsAsmJS = classInfo.objectsMallocHeapElementsAsmJS;
    size_t nonHeapElementsAsmJS    = classInfo.objectsNonHeapElementsAsmJS;
    MOZ_ASSERT(mallocHeapElementsAsmJS == 0 || nonHeapElementsAsmJS == 0);
    if (mallocHeapElementsAsmJS > 0) {
        REPORT_BYTES(path + NS_LITERAL_CSTRING("objects/malloc-heap/elements/asm.js"),
            KIND_HEAP, mallocHeapElementsAsmJS,
            "asm.js array buffer elements on the malloc heap.");
    }
    if (nonHeapElementsAsmJS > 0) {
        REPORT_BYTES(path + NS_LITERAL_CSTRING("objects/non-heap/elements/asm.js"),
            KIND_NONHEAP, nonHeapElementsAsmJS,
            "asm.js array buffer elements outside both the malloc heap and "
            "the GC heap.");
    }

    if (classInfo.objectsNonHeapElementsMapped > 0) {
        REPORT_BYTES(path + NS_LITERAL_CSTRING("objects/non-heap/elements/mapped"),
            KIND_NONHEAP, classInfo.objectsNonHeapElementsMapped,
            "Memory-mapped array buffer elements.");
    }

    if (classInfo.objectsNonHeapCodeAsmJS > 0) {
        REPORT_BYTES(path + NS_LITERAL_CSTRING("objects/non-heap/code/asm.js"),
            KIND_NONHEAP, classInfo.objectsNonHeapCodeAsmJS,
            "AOT-compiled asm.js code.");
    }

    if (classInfo.objectsMallocHeapMisc > 0) {
        REPORT_BYTES(path + NS_LITERAL_CSTRING("objects/malloc-heap/misc"),
            KIND_HEAP, classInfo.objectsMallocHeapMisc,
            "Miscellaneous object data.");
    }

    if (classInfo.shapesGCHeapTree > 0) {
        REPORT_GC_BYTES(path + NS_LITERAL_CSTRING("shapes/gc-heap/tree"),
            classInfo.shapesGCHeapTree,
        "Shapes in a property tree.");
    }

    if (classInfo.shapesGCHeapDict > 0) {
        REPORT_GC_BYTES(path + NS_LITERAL_CSTRING("shapes/gc-heap/dict"),
            classInfo.shapesGCHeapDict,
        "Shapes in dictionary mode.");
    }

    if (classInfo.shapesGCHeapBase > 0) {
        REPORT_GC_BYTES(path + NS_LITERAL_CSTRING("shapes/gc-heap/base"),
            classInfo.shapesGCHeapBase,
            "Base shapes, which collate data common to many shapes.");
    }

    if (classInfo.shapesMallocHeapTreeTables > 0) {
        REPORT_BYTES(path + NS_LITERAL_CSTRING("shapes/malloc-heap/tree-tables"),
            KIND_HEAP, classInfo.shapesMallocHeapTreeTables,
            "Property tables of shapes in a property tree.");
    }

    if (classInfo.shapesMallocHeapDictTables > 0) {
        REPORT_BYTES(path + NS_LITERAL_CSTRING("shapes/malloc-heap/dict-tables"),
            KIND_HEAP, classInfo.shapesMallocHeapDictTables,
            "Property tables of shapes in dictionary mode.");
    }

    if (classInfo.shapesMallocHeapTreeKids > 0) {
        REPORT_BYTES(path + NS_LITERAL_CSTRING("shapes/malloc-heap/tree-kids"),
            KIND_HEAP, classInfo.shapesMallocHeapTreeKids,
            "Kid hashes of shapes in a property tree.");
    }

    return NS_OK;
}

static nsresult
ReportCompartmentStats(const JS::CompartmentStats& cStats,
                       const xpc::CompartmentStatsExtras& extras,
                       amIAddonManager* addonManager,
                       nsIMemoryReporterCallback* cb,
                       nsISupports* closure, size_t* gcTotalOut = nullptr)
{
    static const nsDependentCString addonPrefix("explicit/add-ons/");

    size_t gcTotal = 0, sundriesGCHeap = 0, sundriesMallocHeap = 0;
    nsAutoCString cJSPathPrefix = extras.jsPathPrefix;
    nsAutoCString cDOMPathPrefix = extras.domPathPrefix;
    nsresult rv;

    MOZ_ASSERT(!gcTotalOut == cStats.isTotals);

    
    
    if (extras.location && addonManager &&
        cJSPathPrefix.Find(addonPrefix, false, 0, 0) != 0) {
        nsAutoCString addonId;
        bool ok;
        if (NS_SUCCEEDED(addonManager->MapURIToAddonID(extras.location,
                                                        addonId, &ok))
            && ok) {
            
            
            static const size_t explicitLength = strlen("explicit/");
            addonId.Insert(NS_LITERAL_CSTRING("add-ons/"), 0);
            addonId += "/";
            cJSPathPrefix.Insert(addonId, explicitLength);
            cDOMPathPrefix.Insert(addonId, explicitLength);
        }
    }

    nsCString nonNotablePath = cJSPathPrefix;
    nonNotablePath += cStats.isTotals
                    ? NS_LITERAL_CSTRING("classes/")
                    : NS_LITERAL_CSTRING("classes/class(<non-notable classes>)/");

    rv = ReportClassStats(cStats.classInfo, nonNotablePath, cb, closure,
                          gcTotal);
    NS_ENSURE_SUCCESS(rv, rv);

    for (size_t i = 0; i < cStats.notableClasses.length(); i++) {
        MOZ_ASSERT(!cStats.isTotals);
        const JS::NotableClassInfo& classInfo = cStats.notableClasses[i];

        nsCString classPath = cJSPathPrefix +
            nsPrintfCString("classes/class(%s)/", classInfo.className_);

        rv = ReportClassStats(classInfo, classPath, cb, closure, gcTotal);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    
    ZCREPORT_BYTES(cDOMPathPrefix + NS_LITERAL_CSTRING("orphan-nodes"),
        cStats.objectsPrivate,
        "Orphan DOM nodes, i.e. those that are only reachable from JavaScript "
        "objects.");

    ZCREPORT_GC_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("scripts/gc-heap"),
        cStats.scriptsGCHeap,
        "JSScript instances. There is one per user-defined function in a "
        "script, and one for the top-level code in a script.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("scripts/malloc-heap/data"),
        cStats.scriptsMallocHeapData,
        "Various variable-length tables in JSScripts.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("baseline/data"),
        cStats.baselineData,
        "The Baseline JIT's compilation data (BaselineScripts).");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("baseline/fallback-stubs"),
        cStats.baselineStubsFallback,
        "The Baseline JIT's fallback IC stubs (excluding code).");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("ion-data"),
        cStats.ionData,
        "The IonMonkey JIT's compilation data (IonScripts).");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("type-inference/type-scripts"),
        cStats.typeInferenceTypeScripts,
        "Type sets associated with scripts.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("type-inference/allocation-site-tables"),
        cStats.typeInferenceAllocationSiteTables,
        "Tables of type objects associated with allocation sites.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("type-inference/array-type-tables"),
        cStats.typeInferenceArrayTypeTables,
        "Tables of type objects associated with array literals.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("type-inference/object-type-tables"),
        cStats.typeInferenceObjectTypeTables,
        "Tables of type objects associated with object literals.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("compartment-object"),
        cStats.compartmentObject,
        "The JSCompartment object itself.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("compartment-tables"),
        cStats.compartmentTables,
        "Compartment-wide tables storing shape and type object information.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("inner-views"),
        cStats.innerViewsTable,
        "The table for array buffer inner views.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("lazy-array-buffers"),
        cStats.lazyArrayBuffersTable,
        "The table for typed object lazy array buffers.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("object-metadata"),
        cStats.objectMetadataTable,
        "The table used by debugging tools for tracking object metadata");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("cross-compartment-wrapper-table"),
        cStats.crossCompartmentWrappersTable,
        "The cross-compartment wrapper table.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("regexp-compartment"),
        cStats.regexpCompartment,
        "The regexp compartment and regexp data.");

    if (sundriesGCHeap > 0) {
        
        REPORT_GC_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("sundries/gc-heap"),
            sundriesGCHeap,
            "The sum of all 'gc-heap' measurements that are too small to be "
            "worth showing individually.");
    }

    if (sundriesMallocHeap > 0) {
        
        REPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("sundries/malloc-heap"),
            KIND_HEAP, sundriesMallocHeap,
            "The sum of all 'malloc-heap' measurements that are too small to "
            "be worth showing individually.");
    }

    if (gcTotalOut)
        *gcTotalOut += gcTotal;

    return NS_OK;
}

static nsresult
ReportScriptSourceStats(const ScriptSourceInfo& scriptSourceInfo,
                        const nsACString& path,
                        nsIHandleReportCallback* cb, nsISupports* closure,
                        size_t& rtTotal)
{
    if (scriptSourceInfo.compressed > 0) {
        RREPORT_BYTES(path + NS_LITERAL_CSTRING("compressed"),
            KIND_HEAP, scriptSourceInfo.compressed,
            "Compressed JavaScript source code.");
    }

    if (scriptSourceInfo.uncompressed > 0) {
        RREPORT_BYTES(path + NS_LITERAL_CSTRING("uncompressed"),
            KIND_HEAP, scriptSourceInfo.uncompressed,
            "Uncompressed JavaScript source code.");
    }

    if (scriptSourceInfo.misc > 0) {
        RREPORT_BYTES(path + NS_LITERAL_CSTRING("misc"),
            KIND_HEAP, scriptSourceInfo.misc,
            "Miscellaneous data relating to JavaScript source code.");
    }

    return NS_OK;
}

static nsresult
ReportJSRuntimeExplicitTreeStats(const JS::RuntimeStats& rtStats,
                                 const nsACString& rtPath,
                                 amIAddonManager* addonManager,
                                 nsIMemoryReporterCallback* cb,
                                 nsISupports* closure,
                                 bool anonymize,
                                 size_t* rtTotalOut)
{
    nsresult rv;

    size_t gcTotal = 0;

    for (size_t i = 0; i < rtStats.zoneStatsVector.length(); i++) {
        const JS::ZoneStats& zStats = rtStats.zoneStatsVector[i];
        const xpc::ZoneStatsExtras* extras =
          static_cast<const xpc::ZoneStatsExtras*>(zStats.extra);
        rv = ReportZoneStats(zStats, *extras, cb, closure, anonymize, &gcTotal);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    for (size_t i = 0; i < rtStats.compartmentStatsVector.length(); i++) {
        const JS::CompartmentStats& cStats = rtStats.compartmentStatsVector[i];
        const xpc::CompartmentStatsExtras* extras =
            static_cast<const xpc::CompartmentStatsExtras*>(cStats.extra);

        rv = ReportCompartmentStats(cStats, *extras, addonManager, cb, closure,
                                    &gcTotal);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    

    size_t rtTotal = 0;

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/runtime-object"),
        KIND_HEAP, rtStats.runtime.object,
        "The JSRuntime object.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/atoms-table"),
        KIND_HEAP, rtStats.runtime.atomsTable,
        "The atoms table.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/contexts"),
        KIND_HEAP, rtStats.runtime.contexts,
        "JSContext objects and structures that belong to them.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/dtoa"),
        KIND_HEAP, rtStats.runtime.dtoa,
        "The DtoaState object, which is used for converting strings to "
        "numbers and vice versa.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/temporary"),
        KIND_HEAP, rtStats.runtime.temporary,
        "Transient data (mostly parse nodes) held by the JSRuntime during "
        "compilation.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/interpreter-stack"),
        KIND_HEAP, rtStats.runtime.interpreterStack,
        "JS interpreter frames.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/math-cache"),
        KIND_HEAP, rtStats.runtime.mathCache,
        "The math cache.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/uncompressed-source-cache"),
        KIND_HEAP, rtStats.runtime.uncompressedSourceCache,
        "The uncompressed source code cache.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/compressed-source-sets"),
        KIND_HEAP, rtStats.runtime.compressedSourceSet,
        "The table indexing compressed source code in the runtime.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/script-data"),
        KIND_HEAP, rtStats.runtime.scriptData,
        "The table holding script data shared in the runtime.");

    nsCString nonNotablePath =
        rtPath + nsPrintfCString("runtime/script-sources/source(scripts=%d, <non-notable files>)/",
                                 rtStats.runtime.scriptSourceInfo.numScripts);

    rv = ReportScriptSourceStats(rtStats.runtime.scriptSourceInfo,
                                 nonNotablePath, cb, closure, rtTotal);
    NS_ENSURE_SUCCESS(rv, rv);

    for (size_t i = 0; i < rtStats.runtime.notableScriptSources.length(); i++) {
        const JS::NotableScriptSourceInfo& scriptSourceInfo =
            rtStats.runtime.notableScriptSources[i];

        
        
        
        
        
        nsCString escapedFilename;
        if (anonymize) {
            escapedFilename.AppendPrintf("<anonymized-source-%d>", int(i));
        } else {
            nsDependentCString filename(scriptSourceInfo.filename_);
            escapedFilename.Append(filename);
            escapedFilename.ReplaceSubstring("/", "\\");
        }

        nsCString notablePath = rtPath +
            nsPrintfCString("runtime/script-sources/source(scripts=%d, %s)/",
                            scriptSourceInfo.numScripts, escapedFilename.get());

        rv = ReportScriptSourceStats(scriptSourceInfo, notablePath,
                                     cb, closure, rtTotal);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/code/ion"),
        KIND_NONHEAP, rtStats.runtime.code.ion,
        "Code generated by the IonMonkey JIT.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/code/baseline"),
        KIND_NONHEAP, rtStats.runtime.code.baseline,
        "Code generated by the Baseline JIT.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/code/regexp"),
        KIND_NONHEAP, rtStats.runtime.code.regexp,
        "Code generated by the regexp JIT.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/code/other"),
        KIND_NONHEAP, rtStats.runtime.code.other,
        "Code generated by the JITs for wrappers and trampolines.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/code/unused"),
        KIND_NONHEAP, rtStats.runtime.code.unused,
        "Memory allocated by one of the JITs to hold code, but which is "
        "currently unused.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/gc/marker"),
        KIND_HEAP, rtStats.runtime.gc.marker,
        "The GC mark stack and gray roots.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/gc/nursery-committed"),
        KIND_NONHEAP, rtStats.runtime.gc.nurseryCommitted,
        "Memory being used by the GC's nursery.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/gc/nursery-huge-slots"),
        KIND_NONHEAP, rtStats.runtime.gc.nurseryHugeSlots,
        "Out-of-line slots and elements belonging to objects in the "
        "nursery.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/gc/store-buffer/vals"),
        KIND_HEAP, rtStats.runtime.gc.storeBufferVals,
        "Values in the store buffer.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/gc/store-buffer/cells"),
        KIND_HEAP, rtStats.runtime.gc.storeBufferCells,
        "Cells in the store buffer.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/gc/store-buffer/slots"),
        KIND_HEAP, rtStats.runtime.gc.storeBufferSlots,
        "Slots in the store buffer.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/gc/store-buffer/whole-cells"),
        KIND_HEAP, rtStats.runtime.gc.storeBufferWholeCells,
        "Whole cells in the store buffer.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/gc/store-buffer/reloc-vals"),
        KIND_HEAP, rtStats.runtime.gc.storeBufferRelocVals,
        "Relocatable values in the store buffer.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/gc/store-buffer/reloc-cells"),
        KIND_HEAP, rtStats.runtime.gc.storeBufferRelocCells,
        "Relocatable cells in the store buffer.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/gc/store-buffer/generics"),
        KIND_HEAP, rtStats.runtime.gc.storeBufferGenerics,
        "Generic things in the store buffer.");

    if (rtTotalOut)
        *rtTotalOut = rtTotal;

    

    
    
    nsCString rtPath2(rtPath);
    rtPath2.Replace(0, strlen("explicit"), NS_LITERAL_CSTRING("decommitted"));
    REPORT_GC_BYTES(rtPath2 + NS_LITERAL_CSTRING("gc-heap/decommitted-arenas"),
        rtStats.gcHeapDecommittedArenas,
        "GC arenas in non-empty chunks that is decommitted, i.e. it takes up "
        "address space but no physical memory or swap space.");

    REPORT_BYTES(rtPath2 + NS_LITERAL_CSTRING("runtime/gc/nursery-decommitted"),
        KIND_NONHEAP, rtStats.runtime.gc.nurseryDecommitted,
        "Memory allocated to the GC's nursery this is decommitted, i.e. it takes up "
        "address space but no physical memory or swap space.");

    REPORT_GC_BYTES(rtPath + NS_LITERAL_CSTRING("gc-heap/unused-chunks"),
        rtStats.gcHeapUnusedChunks,
        "Empty GC chunks which will soon be released unless claimed for new "
        "allocations.");

    REPORT_GC_BYTES(rtPath + NS_LITERAL_CSTRING("gc-heap/unused-arenas"),
        rtStats.gcHeapUnusedArenas,
        "Empty GC arenas within non-empty chunks.");

    REPORT_GC_BYTES(rtPath + NS_LITERAL_CSTRING("gc-heap/chunk-admin"),
        rtStats.gcHeapChunkAdmin,
        "Bookkeeping information within GC chunks.");

    
    
    MOZ_ASSERT(gcTotal == rtStats.gcHeapChunkTotal);

    return NS_OK;
}

nsresult
ReportJSRuntimeExplicitTreeStats(const JS::RuntimeStats& rtStats,
                                 const nsACString& rtPath,
                                 nsIMemoryReporterCallback* cb,
                                 nsISupports* closure,
                                 bool anonymize,
                                 size_t* rtTotalOut)
{
    nsCOMPtr<amIAddonManager> am;
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
        
        am = do_GetService("@mozilla.org/addons/integration;1");
    }
    return ReportJSRuntimeExplicitTreeStats(rtStats, rtPath, am.get(),
                                            cb, closure, anonymize, rtTotalOut);
}


} 

class JSMainRuntimeCompartmentsReporter final : public nsIMemoryReporter
{

    ~JSMainRuntimeCompartmentsReporter() {}

  public:
    NS_DECL_ISUPPORTS

    struct Data {
        int anonymizeID;
        js::Vector<nsCString, 0, js::SystemAllocPolicy> paths;
    };

    static void CompartmentCallback(JSRuntime* rt, void* vdata, JSCompartment* c) {
        
        Data* data = static_cast<Data*>(vdata);
        nsCString path;
        GetCompartmentName(c, path, &data->anonymizeID,  true);
        path.Insert(js::IsSystemCompartment(c)
                    ? NS_LITERAL_CSTRING("js-main-runtime-compartments/system/")
                    : NS_LITERAL_CSTRING("js-main-runtime-compartments/user/"),
                    0);
        data->paths.append(path);
    }

    NS_IMETHOD CollectReports(nsIMemoryReporterCallback* cb,
                              nsISupports* closure, bool anonymize) override
    {
        
        
        

        Data data;
        data.anonymizeID = anonymize ? 1 : 0;
        JS_IterateCompartments(nsXPConnect::GetRuntimeInstance()->Runtime(),
                               &data, CompartmentCallback);

        for (size_t i = 0; i < data.paths.length(); i++)
            REPORT(nsCString(data.paths[i]), KIND_OTHER, UNITS_COUNT, 1,
                "A live compartment in the main JSRuntime.");

        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS(JSMainRuntimeCompartmentsReporter, nsIMemoryReporter)

MOZ_DEFINE_MALLOC_SIZE_OF(OrphanMallocSizeOf)

namespace xpc {

static size_t
SizeOfTreeIncludingThis(nsINode* tree)
{
    size_t n = tree->SizeOfIncludingThis(OrphanMallocSizeOf);
    for (nsIContent* child = tree->GetFirstChild(); child; child = child->GetNextNode(tree))
        n += child->SizeOfIncludingThis(OrphanMallocSizeOf);

    return n;
}

class OrphanReporter : public JS::ObjectPrivateVisitor
{
  public:
    explicit OrphanReporter(GetISupportsFun aGetISupports)
      : JS::ObjectPrivateVisitor(aGetISupports)
    {
    }

    virtual size_t sizeOfIncludingThis(nsISupports* aSupports) override {
        size_t n = 0;
        nsCOMPtr<nsINode> node = do_QueryInterface(aSupports);
        
        
        
        if (node && !node->IsInDoc() &&
            !(node->IsElement() && node->AsElement()->IsInNamespace(kNameSpaceID_XBL)))
        {
            
            
            
            nsCOMPtr<nsINode> orphanTree = node->SubtreeRoot();
            if (!mAlreadyMeasuredOrphanTrees.Contains(orphanTree)) {
                n += SizeOfTreeIncludingThis(orphanTree);
                mAlreadyMeasuredOrphanTrees.PutEntry(orphanTree);
            }
        }
        return n;
    }

  private:
    nsTHashtable <nsISupportsHashKey> mAlreadyMeasuredOrphanTrees;
};

#ifdef DEBUG
static bool
StartsWithExplicit(nsACString& s)
{
    return StringBeginsWith(s, NS_LITERAL_CSTRING("explicit/"));
}
#endif

class XPCJSRuntimeStats : public JS::RuntimeStats
{
    WindowPaths* mWindowPaths;
    WindowPaths* mTopWindowPaths;
    bool mGetLocations;
    int mAnonymizeID;

  public:
    XPCJSRuntimeStats(WindowPaths* windowPaths, WindowPaths* topWindowPaths,
                      bool getLocations, bool anonymize)
      : JS::RuntimeStats(JSMallocSizeOf),
        mWindowPaths(windowPaths),
        mTopWindowPaths(topWindowPaths),
        mGetLocations(getLocations),
        mAnonymizeID(anonymize ? 1 : 0)
    {}

    ~XPCJSRuntimeStats() {
        for (size_t i = 0; i != compartmentStatsVector.length(); ++i)
            delete static_cast<xpc::CompartmentStatsExtras*>(compartmentStatsVector[i].extra);


        for (size_t i = 0; i != zoneStatsVector.length(); ++i)
            delete static_cast<xpc::ZoneStatsExtras*>(zoneStatsVector[i].extra);
    }

    virtual void initExtraZoneStats(JS::Zone* zone, JS::ZoneStats* zStats) override {
        
        nsXPConnect* xpc = nsXPConnect::XPConnect();
        AutoSafeJSContext cx;
        JSCompartment* comp = js::GetAnyCompartmentInZone(zone);
        xpc::ZoneStatsExtras* extras = new xpc::ZoneStatsExtras;
        extras->pathPrefix.AssignLiteral("explicit/js-non-window/zones/");
        RootedObject global(cx, JS_GetGlobalForCompartmentOrNull(cx, comp));
        if (global) {
            
            
            JSAutoCompartment ac(cx, global);
            nsISupports* native = xpc->GetNativeOfWrapper(cx, global);
            if (nsCOMPtr<nsPIDOMWindow> piwindow = do_QueryInterface(native)) {
                
                
                if (mTopWindowPaths->Get(piwindow->WindowID(),
                                         &extras->pathPrefix))
                    extras->pathPrefix.AppendLiteral("/js-");
            }
        }

        extras->pathPrefix += nsPrintfCString("zone(0x%p)/", (void*)zone);

        MOZ_ASSERT(StartsWithExplicit(extras->pathPrefix));

        zStats->extra = extras;
    }

    virtual void initExtraCompartmentStats(JSCompartment* c,
                                           JS::CompartmentStats* cstats) override
    {
        xpc::CompartmentStatsExtras* extras = new xpc::CompartmentStatsExtras;
        nsCString cName;
        GetCompartmentName(c, cName, &mAnonymizeID,  true);
        if (mGetLocations) {
            CompartmentPrivate* cp = CompartmentPrivate::Get(c);
            if (cp)
              cp->GetLocationURI(CompartmentPrivate::LocationHintAddon,
                                 getter_AddRefs(extras->location));
            
            
            
        }

        
        nsXPConnect* xpc = nsXPConnect::XPConnect();
        AutoSafeJSContext cx;
        bool needZone = true;
        RootedObject global(cx, JS_GetGlobalForCompartmentOrNull(cx, c));
        if (global) {
            
            
            JSAutoCompartment ac(cx, global);
            nsISupports* native = xpc->GetNativeOfWrapper(cx, global);
            if (nsCOMPtr<nsPIDOMWindow> piwindow = do_QueryInterface(native)) {
                
                
                if (mWindowPaths->Get(piwindow->WindowID(),
                                      &extras->jsPathPrefix)) {
                    extras->domPathPrefix.Assign(extras->jsPathPrefix);
                    extras->domPathPrefix.AppendLiteral("/dom/");
                    extras->jsPathPrefix.AppendLiteral("/js-");
                    needZone = false;
                } else {
                    extras->jsPathPrefix.AssignLiteral("explicit/js-non-window/zones/");
                    extras->domPathPrefix.AssignLiteral("explicit/dom/unknown-window-global?!/");
                }
            } else {
                extras->jsPathPrefix.AssignLiteral("explicit/js-non-window/zones/");
                extras->domPathPrefix.AssignLiteral("explicit/dom/non-window-global?!/");
            }
        } else {
            extras->jsPathPrefix.AssignLiteral("explicit/js-non-window/zones/");
            extras->domPathPrefix.AssignLiteral("explicit/dom/no-global?!/");
        }

        if (needZone)
            extras->jsPathPrefix += nsPrintfCString("zone(0x%p)/", (void*)js::GetCompartmentZone(c));

        extras->jsPathPrefix += NS_LITERAL_CSTRING("compartment(") + cName + NS_LITERAL_CSTRING(")/");

        
        
        
        
        
        
        
        
        
        
        

        MOZ_ASSERT(StartsWithExplicit(extras->jsPathPrefix));
        MOZ_ASSERT(StartsWithExplicit(extras->domPathPrefix));

        cstats->extra = extras;
    }
};

nsresult
JSReporter::CollectReports(WindowPaths* windowPaths,
                           WindowPaths* topWindowPaths,
                           nsIMemoryReporterCallback* cb,
                           nsISupports* closure,
                           bool anonymize)
{
    XPCJSRuntime* xpcrt = nsXPConnect::GetRuntimeInstance();

    
    
    
    
    

    nsCOMPtr<amIAddonManager> addonManager;
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
        
        addonManager = do_GetService("@mozilla.org/addons/integration;1");
    }
    bool getLocations = !!addonManager;
    XPCJSRuntimeStats rtStats(windowPaths, topWindowPaths, getLocations,
                              anonymize);
    OrphanReporter orphanReporter(XPCConvert::GetISupportsFromJSObject);
    if (!JS::CollectRuntimeStats(xpcrt->Runtime(), &rtStats, &orphanReporter,
                                 anonymize))
    {
        return NS_ERROR_FAILURE;
    }

    size_t xpcJSRuntimeSize = xpcrt->SizeOfIncludingThis(JSMallocSizeOf);

    size_t wrappedJSSize = xpcrt->GetWrappedJSMap()->SizeOfWrappedJS(JSMallocSizeOf);

    XPCWrappedNativeScope::ScopeSizeInfo sizeInfo(JSMallocSizeOf);
    XPCWrappedNativeScope::AddSizeOfAllScopesIncludingThis(&sizeInfo);

    mozJSComponentLoader* loader = mozJSComponentLoader::Get();
    size_t jsComponentLoaderSize = loader ? loader->SizeOfIncludingThis(JSMallocSizeOf) : 0;

    
    

    nsresult rv;
    size_t rtTotal = 0;
    rv = xpc::ReportJSRuntimeExplicitTreeStats(rtStats,
                                               NS_LITERAL_CSTRING("explicit/js-non-window/"),
                                               addonManager, cb, closure,
                                               anonymize, &rtTotal);
    NS_ENSURE_SUCCESS(rv, rv);

    
    xpc::CompartmentStatsExtras cExtrasTotal;
    cExtrasTotal.jsPathPrefix.AssignLiteral("js-main-runtime/compartments/");
    cExtrasTotal.domPathPrefix.AssignLiteral("window-objects/dom/");
    rv = ReportCompartmentStats(rtStats.cTotals, cExtrasTotal, addonManager,
                                cb, closure);
    NS_ENSURE_SUCCESS(rv, rv);

    xpc::ZoneStatsExtras zExtrasTotal;
    zExtrasTotal.pathPrefix.AssignLiteral("js-main-runtime/zones/");
    rv = ReportZoneStats(rtStats.zTotals, zExtrasTotal, cb, closure, anonymize);
    NS_ENSURE_SUCCESS(rv, rv);

    
    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime/runtime"),
        KIND_OTHER, rtTotal,
        "The sum of all measurements under 'explicit/js-non-window/runtime/'.");

    

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime/gc-heap/unused-chunks"),
        KIND_OTHER, rtStats.gcHeapUnusedChunks,
        "The same as 'explicit/js-non-window/gc-heap/unused-chunks'.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime/gc-heap/unused-arenas"),
        KIND_OTHER, rtStats.gcHeapUnusedArenas,
        "The same as 'explicit/js-non-window/gc-heap/unused-arenas'.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime/gc-heap/chunk-admin"),
        KIND_OTHER, rtStats.gcHeapChunkAdmin,
        "The same as 'explicit/js-non-window/gc-heap/chunk-admin'.");

    

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/unused/chunks"),
        KIND_OTHER, rtStats.gcHeapUnusedChunks,
        "The same as 'explicit/js-non-window/gc-heap/unused-chunks'.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/unused/arenas"),
        KIND_OTHER, rtStats.gcHeapUnusedArenas,
        "The same as 'explicit/js-non-window/gc-heap/unused-arenas'.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/unused/gc-things/objects"),
        KIND_OTHER, rtStats.zTotals.unusedGCThings.object,
        "Unused object cells within non-empty arenas.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/unused/gc-things/strings"),
        KIND_OTHER, rtStats.zTotals.unusedGCThings.string,
        "Unused string cells within non-empty arenas.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/unused/gc-things/symbols"),
        KIND_OTHER, rtStats.zTotals.unusedGCThings.symbol,
        "Unused symbol cells within non-empty arenas.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/unused/gc-things/shapes"),
        KIND_OTHER, rtStats.zTotals.unusedGCThings.shape,
        "Unused shape cells within non-empty arenas.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/unused/gc-things/base-shapes"),
        KIND_OTHER, rtStats.zTotals.unusedGCThings.baseShape,
        "Unused base shape cells within non-empty arenas.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/unused/gc-things/object-groups"),
        KIND_OTHER, rtStats.zTotals.unusedGCThings.objectGroup,
        "Unused object group cells within non-empty arenas.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/unused/gc-things/scripts"),
        KIND_OTHER, rtStats.zTotals.unusedGCThings.script,
        "Unused script cells within non-empty arenas.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/unused/gc-things/lazy-scripts"),
        KIND_OTHER, rtStats.zTotals.unusedGCThings.lazyScript,
        "Unused lazy script cells within non-empty arenas.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/unused/gc-things/jitcode"),
        KIND_OTHER, rtStats.zTotals.unusedGCThings.jitcode,
        "Unused jitcode cells within non-empty arenas.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/used/chunk-admin"),
        KIND_OTHER, rtStats.gcHeapChunkAdmin,
        "The same as 'explicit/js-non-window/gc-heap/chunk-admin'.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/used/arena-admin"),
        KIND_OTHER, rtStats.zTotals.gcHeapArenaAdmin,
        "The same as 'js-main-runtime/zones/gc-heap-arena-admin'.");

    size_t gcThingTotal = 0;

    MREPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/used/gc-things/objects"),
        KIND_OTHER, rtStats.cTotals.classInfo.objectsGCHeap,
        "Used object cells.");

    MREPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/used/gc-things/strings"),
        KIND_OTHER, rtStats.zTotals.stringInfo.sizeOfLiveGCThings(),
        "Used string cells.");

    MREPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/used/gc-things/symbols"),
        KIND_OTHER, rtStats.zTotals.symbolsGCHeap,
        "Used symbol cells.");

    MREPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/used/gc-things/shapes"),
        KIND_OTHER,
        rtStats.cTotals.classInfo.shapesGCHeapTree + rtStats.cTotals.classInfo.shapesGCHeapDict,
        "Used shape cells.");

    MREPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/used/gc-things/base-shapes"),
        KIND_OTHER, rtStats.cTotals.classInfo.shapesGCHeapBase,
        "Used base shape cells.");

    MREPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/used/gc-things/object-groups"),
        KIND_OTHER, rtStats.zTotals.objectGroupsGCHeap,
        "Used object group cells.");

    MREPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/used/gc-things/scripts"),
        KIND_OTHER, rtStats.cTotals.scriptsGCHeap,
        "Used script cells.");

    MREPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/used/gc-things/lazy-scripts"),
        KIND_OTHER, rtStats.zTotals.lazyScriptsGCHeap,
        "Used lazy script cells.");

    MREPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/used/gc-things/jitcode"),
        KIND_OTHER, rtStats.zTotals.jitCodesGCHeap,
        "Used jitcode cells.");

    MOZ_ASSERT(gcThingTotal == rtStats.gcHeapGCThings);

    

    REPORT_BYTES(NS_LITERAL_CSTRING("explicit/xpconnect/runtime"),
        KIND_HEAP, xpcJSRuntimeSize,
        "The XPConnect runtime.");

    REPORT_BYTES(NS_LITERAL_CSTRING("explicit/xpconnect/wrappedjs"),
        KIND_HEAP, wrappedJSSize,
        "Wrappers used to implement XPIDL interfaces with JS.");

    REPORT_BYTES(NS_LITERAL_CSTRING("explicit/xpconnect/scopes"),
        KIND_HEAP, sizeInfo.mScopeAndMapSize,
        "XPConnect scopes.");

    REPORT_BYTES(NS_LITERAL_CSTRING("explicit/xpconnect/proto-iface-cache"),
        KIND_HEAP, sizeInfo.mProtoAndIfaceCacheSize,
        "Prototype and interface binding caches.");

    REPORT_BYTES(NS_LITERAL_CSTRING("explicit/xpconnect/js-component-loader"),
        KIND_HEAP, jsComponentLoaderSize,
        "XPConnect's JS component loader.");

    return NS_OK;
}

static nsresult
JSSizeOfTab(JSObject* objArg, size_t* jsObjectsSize, size_t* jsStringsSize,
            size_t* jsPrivateSize, size_t* jsOtherSize)
{
    JSRuntime* rt = nsXPConnect::GetRuntimeInstance()->Runtime();
    JS::RootedObject obj(rt, objArg);

    TabSizes sizes;
    OrphanReporter orphanReporter(XPCConvert::GetISupportsFromJSObject);
    NS_ENSURE_TRUE(JS::AddSizeOfTab(rt, obj, moz_malloc_size_of,
                                    &orphanReporter, &sizes),
                   NS_ERROR_OUT_OF_MEMORY);

    *jsObjectsSize = sizes.objects;
    *jsStringsSize = sizes.strings;
    *jsPrivateSize = sizes.private_;
    *jsOtherSize   = sizes.other;
    return NS_OK;
}

} 

static void
AccumulateTelemetryCallback(int id, uint32_t sample, const char* key)
{
    switch (id) {
      case JS_TELEMETRY_GC_REASON:
        Telemetry::Accumulate(Telemetry::GC_REASON_2, sample);
        break;
      case JS_TELEMETRY_GC_IS_COMPARTMENTAL:
        Telemetry::Accumulate(Telemetry::GC_IS_COMPARTMENTAL, sample);
        break;
      case JS_TELEMETRY_GC_MS:
        Telemetry::Accumulate(Telemetry::GC_MS, sample);
        break;
      case JS_TELEMETRY_GC_MAX_PAUSE_MS:
        Telemetry::Accumulate(Telemetry::GC_MAX_PAUSE_MS, sample);
        break;
      case JS_TELEMETRY_GC_MARK_MS:
        Telemetry::Accumulate(Telemetry::GC_MARK_MS, sample);
        break;
      case JS_TELEMETRY_GC_SWEEP_MS:
        Telemetry::Accumulate(Telemetry::GC_SWEEP_MS, sample);
        break;
      case JS_TELEMETRY_GC_MARK_ROOTS_MS:
        Telemetry::Accumulate(Telemetry::GC_MARK_ROOTS_MS, sample);
        break;
      case JS_TELEMETRY_GC_MARK_GRAY_MS:
        Telemetry::Accumulate(Telemetry::GC_MARK_GRAY_MS, sample);
        break;
      case JS_TELEMETRY_GC_SLICE_MS:
        Telemetry::Accumulate(Telemetry::GC_SLICE_MS, sample);
        break;
      case JS_TELEMETRY_GC_MMU_50:
        Telemetry::Accumulate(Telemetry::GC_MMU_50, sample);
        break;
      case JS_TELEMETRY_GC_RESET:
        Telemetry::Accumulate(Telemetry::GC_RESET, sample);
        break;
      case JS_TELEMETRY_GC_INCREMENTAL_DISABLED:
        Telemetry::Accumulate(Telemetry::GC_INCREMENTAL_DISABLED, sample);
        break;
      case JS_TELEMETRY_GC_NON_INCREMENTAL:
        Telemetry::Accumulate(Telemetry::GC_NON_INCREMENTAL, sample);
        break;
      case JS_TELEMETRY_GC_SCC_SWEEP_TOTAL_MS:
        Telemetry::Accumulate(Telemetry::GC_SCC_SWEEP_TOTAL_MS, sample);
        break;
      case JS_TELEMETRY_GC_SCC_SWEEP_MAX_PAUSE_MS:
        Telemetry::Accumulate(Telemetry::GC_SCC_SWEEP_MAX_PAUSE_MS, sample);
        break;
      case JS_TELEMETRY_DEPRECATED_LANGUAGE_EXTENSIONS_IN_CONTENT:
        Telemetry::Accumulate(Telemetry::JS_DEPRECATED_LANGUAGE_EXTENSIONS_IN_CONTENT, sample);
        break;
      case JS_TELEMETRY_ADDON_EXCEPTIONS:
        Telemetry::Accumulate(Telemetry::JS_TELEMETRY_ADDON_EXCEPTIONS, nsDependentCString(key), sample);
        break;
      default:
        MOZ_ASSERT_UNREACHABLE("Unexpected JS_TELEMETRY id");
    }
}

static void
CompartmentNameCallback(JSRuntime* rt, JSCompartment* comp,
                        char* buf, size_t bufsize)
{
    nsCString name;
    
    
    int anonymizeID = 0;
    GetCompartmentName(comp, name, &anonymizeID,  false);
    if (name.Length() >= bufsize)
        name.Truncate(bufsize - 1);
    memcpy(buf, name.get(), name.Length() + 1);
}

static bool
PreserveWrapper(JSContext* cx, JSObject* obj)
{
    MOZ_ASSERT(cx);
    MOZ_ASSERT(obj);
    MOZ_ASSERT(IS_WN_REFLECTOR(obj) || mozilla::dom::IsDOMObject(obj));

    return mozilla::dom::IsDOMObject(obj) && mozilla::dom::TryPreserveWrapper(obj);
}

static nsresult
ReadSourceFromFilename(JSContext* cx, const char* filename, char16_t** src, size_t* len)
{
    nsresult rv;

    
    
    const char* arrow;
    while ((arrow = strstr(filename, " -> ")))
        filename = arrow + strlen(" -> ");

    
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), filename);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIChannel> scriptChannel;
    rv = NS_NewChannel(getter_AddRefs(scriptChannel),
                       uri,
                       nsContentUtils::GetSystemPrincipal(),
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_OTHER);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIURI> actualUri;
    rv = scriptChannel->GetURI(getter_AddRefs(actualUri));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCString scheme;
    rv = actualUri->GetScheme(scheme);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!scheme.EqualsLiteral("file") && !scheme.EqualsLiteral("jar"))
        return NS_OK;

    nsCOMPtr<nsIInputStream> scriptStream;
    rv = scriptChannel->Open(getter_AddRefs(scriptStream));
    NS_ENSURE_SUCCESS(rv, rv);

    uint64_t rawLen;
    rv = scriptStream->Available(&rawLen);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!rawLen)
        return NS_ERROR_FAILURE;

    
    
    
    if (rawLen > UINT32_MAX)
        return NS_ERROR_FILE_TOO_BIG;

    
    nsAutoArrayPtr<unsigned char> buf(new unsigned char[rawLen]);
    if (!buf)
        return NS_ERROR_OUT_OF_MEMORY;

    unsigned char* ptr = buf;
    unsigned char* end = ptr + rawLen;
    while (ptr < end) {
        uint32_t bytesRead;
        rv = scriptStream->Read(reinterpret_cast<char*>(ptr), end - ptr, &bytesRead);
        if (NS_FAILED(rv))
            return rv;
        MOZ_ASSERT(bytesRead > 0, "stream promised more bytes before EOF");
        ptr += bytesRead;
    }

    rv = nsScriptLoader::ConvertToUTF16(scriptChannel, buf, rawLen, EmptyString(),
                                        nullptr, *src, *len);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!*src)
        return NS_ERROR_FAILURE;

    
    
    
    JS_updateMallocCounter(cx, *len);

    return NS_OK;
}




class XPCJSSourceHook: public js::SourceHook {
    bool load(JSContext* cx, const char* filename, char16_t** src, size_t* length) {
        *src = nullptr;
        *length = 0;

        if (!nsContentUtils::IsCallerChrome())
            return true;

        if (!filename)
            return true;

        nsresult rv = ReadSourceFromFilename(cx, filename, src, length);
        if (NS_FAILED(rv)) {
            xpc::Throw(cx, rv);
            return false;
        }

        return true;
    }
};

static const JSWrapObjectCallbacks WrapObjectCallbacks = {
    xpc::WrapperFactory::Rewrap,
    xpc::WrapperFactory::PrepareForWrapping
};

XPCJSRuntime::XPCJSRuntime(nsXPConnect* aXPConnect)
   : CycleCollectedJSRuntime(nullptr, JS::DefaultHeapMaxBytes, JS::DefaultNurseryBytes),
   mJSContextStack(new XPCJSContextStack(this)),
   mCallContext(nullptr),
   mAutoRoots(nullptr),
   mResolveName(JSID_VOID),
   mResolvingWrapper(nullptr),
   mWrappedJSMap(JSObject2WrappedJSMap::newMap(XPC_JS_MAP_LENGTH)),
   mWrappedJSClassMap(IID2WrappedJSClassMap::newMap(XPC_JS_CLASS_MAP_LENGTH)),
   mIID2NativeInterfaceMap(IID2NativeInterfaceMap::newMap(XPC_NATIVE_INTERFACE_MAP_LENGTH)),
   mClassInfo2NativeSetMap(ClassInfo2NativeSetMap::newMap(XPC_NATIVE_SET_MAP_LENGTH)),
   mNativeSetMap(NativeSetMap::newMap(XPC_NATIVE_SET_MAP_LENGTH)),
   mThisTranslatorMap(IID2ThisTranslatorMap::newMap(XPC_THIS_TRANSLATOR_MAP_LENGTH)),
   mNativeScriptableSharedMap(XPCNativeScriptableSharedMap::newMap(XPC_NATIVE_JSCLASS_MAP_LENGTH)),
   mDyingWrappedNativeProtoMap(XPCWrappedNativeProtoMap::newMap(XPC_DYING_NATIVE_PROTO_MAP_LENGTH)),
   mDetachedWrappedNativeProtoMap(XPCWrappedNativeProtoMap::newMap(XPC_DETACHED_NATIVE_PROTO_MAP_LENGTH)),
   mGCIsRunning(false),
   mWrappedJSToReleaseArray(),
   mNativesToReleaseArray(),
   mDoingFinalization(false),
   mVariantRoots(nullptr),
   mWrappedJSRoots(nullptr),
   mObjectHolderRoots(nullptr),
   mWatchdogManager(new WatchdogManager(this)),
   mUnprivilegedJunkScope(this->Runtime(), nullptr),
   mPrivilegedJunkScope(this->Runtime(), nullptr),
   mCompilationScope(this->Runtime(), nullptr),
   mAsyncSnowWhiteFreer(new AsyncFreeSnowWhite()),
   mSlowScriptSecondHalf(false)
{
    
    mStrIDs[0] = JSID_VOID;

    MOZ_ASSERT(Runtime());
    JSRuntime* runtime = Runtime();

    auto rtPrivate = new PerThreadAtomCache();
    memset(rtPrivate, 0, sizeof(PerThreadAtomCache));
    JS_SetRuntimePrivate(runtime, rtPrivate);

    
    
    
    
    
    
    JS_SetGCParameter(runtime, JSGC_MAX_BYTES, 0xffffffff);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    const size_t kSystemCodeBuffer = 10 * 1024;

    
    
    
    const size_t kDefaultStackQuota = 128 * sizeof(size_t) * 1024;

    
    
    

#if defined(XP_MACOSX) || defined(DARWIN)
    
    
    const size_t kStackQuota = 7 * 1024 * 1024;
    const size_t kTrustedScriptBuffer = 180 * 1024;
#elif defined(MOZ_ASAN)
    
    
    
    
    
    
    const size_t kStackQuota =  2 * kDefaultStackQuota;
    const size_t kTrustedScriptBuffer = 270 * 1024;
#elif defined(XP_WIN)
    
    
    const size_t kStackQuota = 900 * 1024;
    const size_t kTrustedScriptBuffer = (sizeof(size_t) == 8) ? 140 * 1024
                                                              : 80 * 1024;
    
    
#elif defined(DEBUG)
    
    
    
    const size_t kStackQuota = 2 * kDefaultStackQuota;
    const size_t kTrustedScriptBuffer = sizeof(size_t) * 12800;
#else
    const size_t kStackQuota = kDefaultStackQuota;
    const size_t kTrustedScriptBuffer = sizeof(size_t) * 12800;
#endif

    
    
    (void) kDefaultStackQuota;

    JS_SetNativeStackQuota(runtime,
                           kStackQuota,
                           kStackQuota - kSystemCodeBuffer,
                           kStackQuota - kSystemCodeBuffer - kTrustedScriptBuffer);

    JS_SetErrorReporter(runtime, xpc::SystemErrorReporter);
    JS_SetDestroyCompartmentCallback(runtime, CompartmentDestroyedCallback);
    JS_SetCompartmentNameCallback(runtime, CompartmentNameCallback);
    mPrevGCSliceCallback = JS::SetGCSliceCallback(runtime, GCSliceCallback);
    JS_AddFinalizeCallback(runtime, FinalizeCallback, nullptr);
    JS_AddWeakPointerCallback(runtime, WeakPointerCallback, this);
    JS_SetWrapObjectCallbacks(runtime, &WrapObjectCallbacks);
    js::SetPreserveWrapperCallback(runtime, PreserveWrapper);
#ifdef MOZ_ENABLE_PROFILER_SPS
    if (PseudoStack* stack = mozilla_get_pseudo_stack())
        stack->sampleRuntime(runtime);
#endif
    JS_SetAccumulateTelemetryCallback(runtime, AccumulateTelemetryCallback);
    js::SetDefaultJSContextCallback(runtime, DefaultJSContextCallback);
    js::SetActivityCallback(runtime, ActivityCallback, this);
    js::SetCTypesActivityCallback(runtime, CTypesActivityCallback);
    JS_SetInterruptCallback(runtime, InterruptCallback);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    UniquePtr<XPCJSSourceHook> hook(new XPCJSSourceHook);
    js::SetSourceHook(runtime, Move(hook));

    
    
    
    if (!xpc_LocalizeRuntime(runtime))
        NS_RUNTIMEABORT("xpc_LocalizeRuntime failed.");

    
    RegisterStrongMemoryReporter(new JSMainRuntimeCompartmentsReporter());
    RegisterStrongMemoryReporter(new JSMainRuntimeTemporaryPeakReporter());
    RegisterJSMainRuntimeGCHeapDistinguishedAmount(JSMainRuntimeGCHeapDistinguishedAmount);
    RegisterJSMainRuntimeTemporaryPeakDistinguishedAmount(JSMainRuntimeTemporaryPeakDistinguishedAmount);
    RegisterJSMainRuntimeCompartmentsSystemDistinguishedAmount(JSMainRuntimeCompartmentsSystemDistinguishedAmount);
    RegisterJSMainRuntimeCompartmentsUserDistinguishedAmount(JSMainRuntimeCompartmentsUserDistinguishedAmount);
    mozilla::RegisterJSSizeOfTab(JSSizeOfTab);

    
    ReloadPrefsCallback(nullptr, this);
    Preferences::RegisterCallback(ReloadPrefsCallback, JS_OPTIONS_DOT_STR, this);
}


XPCJSRuntime*
XPCJSRuntime::newXPCJSRuntime(nsXPConnect* aXPConnect)
{
    NS_PRECONDITION(aXPConnect,"bad param");

    XPCJSRuntime* self = new XPCJSRuntime(aXPConnect);

    if (self                                    &&
        self->Runtime()                         &&
        self->GetWrappedJSMap()                 &&
        self->GetWrappedJSClassMap()            &&
        self->GetIID2NativeInterfaceMap()       &&
        self->GetClassInfo2NativeSetMap()       &&
        self->GetNativeSetMap()                 &&
        self->GetThisTranslatorMap()            &&
        self->GetNativeScriptableSharedMap()    &&
        self->GetDyingWrappedNativeProtoMap()   &&
        self->mWatchdogManager) {
        return self;
    }

    NS_RUNTIMEABORT("new XPCJSRuntime failed to initialize.");

    delete self;
    return nullptr;
}

bool
XPCJSRuntime::OnJSContextNew(JSContext* cx)
{
    
    
    
    JSAutoRequest ar(cx);

    
    if (JSID_IS_VOID(mStrIDs[0])) {
        RootedString str(cx);
        for (unsigned i = 0; i < IDX_TOTAL_COUNT; i++) {
            str = JS_InternString(cx, mStrings[i]);
            if (!str) {
                mStrIDs[0] = JSID_VOID;
                return false;
            }
            mStrIDs[i] = INTERNED_STRING_TO_JSID(cx, str);
            mStrJSVals[i] = STRING_TO_JSVAL(str);
        }

        if (!mozilla::dom::DefineStaticJSVals(cx)) {
            return false;
        }
    }

    XPCContext* xpc = new XPCContext(this, cx);
    if (!xpc)
        return false;

    return true;
}

bool
XPCJSRuntime::DescribeCustomObjects(JSObject* obj, const js::Class* clasp,
                                    char (&name)[72]) const
{
    XPCNativeScriptableInfo* si = nullptr;

    if (!IS_PROTO_CLASS(clasp)) {
        return false;
    }

    XPCWrappedNativeProto* p =
        static_cast<XPCWrappedNativeProto*>(xpc_GetJSPrivate(obj));
    si = p->GetScriptableInfo();
    
    if (!si) {
        return false;
    }

    JS_snprintf(name, sizeof(name), "JS Object (%s - %s)",
                clasp->name, si->GetJSClass()->name);
    return true;
}

bool
XPCJSRuntime::NoteCustomGCThingXPCOMChildren(const js::Class* clasp, JSObject* obj,
                                             nsCycleCollectionTraversalCallback& cb) const
{
    if (clasp != &XPC_WN_Tearoff_JSClass) {
        return false;
    }

    
    
    
    XPCWrappedNativeTearOff* to =
        static_cast<XPCWrappedNativeTearOff*>(xpc_GetJSPrivate(obj));
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "xpc_GetJSPrivate(obj)->mNative");
    cb.NoteXPCOMChild(to->GetNative());
    return true;
}



#ifdef DEBUG
static PLDHashOperator
WrappedJSClassMapDumpEnumerator(PLDHashTable* table, PLDHashEntryHdr* hdr,
                                uint32_t number, void* arg)
{
    ((IID2WrappedJSClassMap::Entry*)hdr)->value->DebugDump(*(int16_t*)arg);
    return PL_DHASH_NEXT;
}
static PLDHashOperator
NativeSetDumpEnumerator(PLDHashTable* table, PLDHashEntryHdr* hdr,
                        uint32_t number, void* arg)
{
    ((NativeSetMap::Entry*)hdr)->key_value->DebugDump(*(int16_t*)arg);
    return PL_DHASH_NEXT;
}
#endif

void
XPCJSRuntime::DebugDump(int16_t depth)
{
#ifdef DEBUG
    depth--;
    XPC_LOG_ALWAYS(("XPCJSRuntime @ %x", this));
        XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("mJSRuntime @ %x", Runtime()));

        XPC_LOG_ALWAYS(("mWrappedJSToReleaseArray @ %x with %d wrappers(s)", \
                        &mWrappedJSToReleaseArray,
                        mWrappedJSToReleaseArray.Length()));

        int cxCount = 0;
        JSContext* iter = nullptr;
        while (JS_ContextIterator(Runtime(), &iter))
            ++cxCount;
        XPC_LOG_ALWAYS(("%d JS context(s)", cxCount));

        iter = nullptr;
        while (JS_ContextIterator(Runtime(), &iter)) {
            XPCContext* xpc = XPCContext::GetXPCContext(iter);
            XPC_LOG_INDENT();
            xpc->DebugDump(depth);
            XPC_LOG_OUTDENT();
        }

        XPC_LOG_ALWAYS(("mWrappedJSClassMap @ %x with %d wrapperclasses(s)",  \
                        mWrappedJSClassMap, mWrappedJSClassMap ?              \
                        mWrappedJSClassMap->Count() : 0));
        
        if (depth && mWrappedJSClassMap && mWrappedJSClassMap->Count()) {
            XPC_LOG_INDENT();
            mWrappedJSClassMap->Enumerate(WrappedJSClassMapDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }
        XPC_LOG_ALWAYS(("mWrappedJSMap @ %x with %d wrappers(s)",             \
                        mWrappedJSMap, mWrappedJSMap ?                        \
                        mWrappedJSMap->Count() : 0));
        
        if (depth && mWrappedJSMap && mWrappedJSMap->Count()) {
            XPC_LOG_INDENT();
            mWrappedJSMap->Dump(depth);
            XPC_LOG_OUTDENT();
        }

        XPC_LOG_ALWAYS(("mIID2NativeInterfaceMap @ %x with %d interface(s)",  \
                        mIID2NativeInterfaceMap, mIID2NativeInterfaceMap ?    \
                        mIID2NativeInterfaceMap->Count() : 0));

        XPC_LOG_ALWAYS(("mClassInfo2NativeSetMap @ %x with %d sets(s)",       \
                        mClassInfo2NativeSetMap, mClassInfo2NativeSetMap ?    \
                        mClassInfo2NativeSetMap->Count() : 0));

        XPC_LOG_ALWAYS(("mThisTranslatorMap @ %x with %d translator(s)",      \
                        mThisTranslatorMap, mThisTranslatorMap ?              \
                        mThisTranslatorMap->Count() : 0));

        XPC_LOG_ALWAYS(("mNativeSetMap @ %x with %d sets(s)",                 \
                        mNativeSetMap, mNativeSetMap ?                        \
                        mNativeSetMap->Count() : 0));

        
        if (depth && mNativeSetMap && mNativeSetMap->Count()) {
            XPC_LOG_INDENT();
            mNativeSetMap->Enumerate(NativeSetDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }

        XPC_LOG_OUTDENT();
#endif
}



void
XPCRootSetElem::AddToRootSet(XPCRootSetElem** listHead)
{
    MOZ_ASSERT(!mSelfp, "Must be not linked");

    mSelfp = listHead;
    mNext = *listHead;
    if (mNext) {
        MOZ_ASSERT(mNext->mSelfp == listHead, "Must be list start");
        mNext->mSelfp = &mNext;
    }
    *listHead = this;
}

void
XPCRootSetElem::RemoveFromRootSet()
{
    nsXPConnect* xpc = nsXPConnect::XPConnect();
    JS::PokeGC(xpc->GetRuntime()->Runtime());

    MOZ_ASSERT(mSelfp, "Must be linked");

    MOZ_ASSERT(*mSelfp == this, "Link invariant");
    *mSelfp = mNext;
    if (mNext)
        mNext->mSelfp = mSelfp;
#ifdef DEBUG
    mSelfp = nullptr;
    mNext = nullptr;
#endif
}

void
XPCJSRuntime::AddGCCallback(xpcGCCallback cb)
{
    MOZ_ASSERT(cb, "null callback");
    extraGCCallbacks.AppendElement(cb);
}

void
XPCJSRuntime::RemoveGCCallback(xpcGCCallback cb)
{
    MOZ_ASSERT(cb, "null callback");
    bool found = extraGCCallbacks.RemoveElement(cb);
    if (!found) {
        NS_ERROR("Removing a callback which was never added.");
    }
}

void
XPCJSRuntime::AddContextCallback(xpcContextCallback cb)
{
    MOZ_ASSERT(cb, "null callback");
    extraContextCallbacks.AppendElement(cb);
}

void
XPCJSRuntime::RemoveContextCallback(xpcContextCallback cb)
{
    MOZ_ASSERT(cb, "null callback");
    bool found = extraContextCallbacks.RemoveElement(cb);
    if (!found) {
        NS_ERROR("Removing a callback which was never added.");
    }
}

void
XPCJSRuntime::InitSingletonScopes()
{
    
    JSContext* cx = GetJSContextStack()->GetSafeJSContext();
    JSAutoRequest ar(cx);
    RootedValue v(cx);
    nsresult rv;

    
    SandboxOptions unprivilegedJunkScopeOptions;
    unprivilegedJunkScopeOptions.sandboxName.AssignLiteral("XPConnect Junk Compartment");
    unprivilegedJunkScopeOptions.invisibleToDebugger = true;
    rv = CreateSandboxObject(cx, &v, nullptr, unprivilegedJunkScopeOptions);
    MOZ_RELEASE_ASSERT(NS_SUCCEEDED(rv));
    mUnprivilegedJunkScope = js::UncheckedUnwrap(&v.toObject());

    
    SandboxOptions privilegedJunkScopeOptions;
    privilegedJunkScopeOptions.sandboxName.AssignLiteral("XPConnect Privileged Junk Compartment");
    privilegedJunkScopeOptions.invisibleToDebugger = true;
    privilegedJunkScopeOptions.wantComponents = false;
    rv = CreateSandboxObject(cx, &v, nsXPConnect::SystemPrincipal(), privilegedJunkScopeOptions);
    MOZ_RELEASE_ASSERT(NS_SUCCEEDED(rv));
    mPrivilegedJunkScope = js::UncheckedUnwrap(&v.toObject());

    
    SandboxOptions compilationScopeOptions;
    compilationScopeOptions.sandboxName.AssignLiteral("XPConnect Compilation Compartment");
    compilationScopeOptions.invisibleToDebugger = true;
    compilationScopeOptions.discardSource = ShouldDiscardSystemSource();
    rv = CreateSandboxObject(cx, &v,  nullptr, compilationScopeOptions);
    MOZ_RELEASE_ASSERT(NS_SUCCEEDED(rv));
    mCompilationScope = js::UncheckedUnwrap(&v.toObject());
}

void
XPCJSRuntime::DeleteSingletonScopes()
{
    mUnprivilegedJunkScope = nullptr;
    mPrivilegedJunkScope = nullptr;
    mCompilationScope = nullptr;
}
