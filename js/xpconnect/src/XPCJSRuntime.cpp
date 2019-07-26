







#include "mozilla/MemoryReporting.h"
#include "mozilla/Util.h"

#include "xpcprivate.h"
#include "xpcpublic.h"
#include "XPCJSMemoryReporter.h"
#include "WrapperFactory.h"
#include "dom_quickstubs.h"

#include "nsIMemoryReporter.h"
#include "nsIObserverService.h"
#include "amIAddonManager.h"
#include "nsPIDOMWindow.h"
#include "nsPrintfCString.h"
#include "prsystem.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"
#include "mozilla/Services.h"

#include "nsLayoutStatics.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsCCUncollectableMarker.h"
#include "nsCycleCollectionNoteRootCallback.h"
#include "nsCycleCollectorUtils.h"
#include "nsScriptLoader.h"
#include "jsfriendapi.h"
#include "js/MemoryMetrics.h"
#include "mozilla/dom/DOMJSClass.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Element.h"
#include "mozilla/Attributes.h"
#include "AccessCheck.h"

#include "GeckoProfiler.h"
#include "nsJSPrincipals.h"
#include <algorithm>

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

using namespace mozilla;
using namespace xpc;
using namespace JS;



const char* XPCJSRuntime::mStrings[] = {
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
};



struct CX_AND_XPCRT_Data
{
    JSContext* cx;
    XPCJSRuntime* rt;
};

static void * const UNMARK_ONLY = nullptr;
static void * const UNMARK_AND_SWEEP = (void *)1;

static PLDHashOperator
NativeInterfaceSweeper(PLDHashTable *table, PLDHashEntryHdr *hdr,
                       uint32_t number, void *arg)
{
    XPCNativeInterface* iface = ((IID2NativeInterfaceMap::Entry*)hdr)->value;
    if (iface->IsMarked()) {
        iface->Unmark();
        return PL_DHASH_NEXT;
    }

    if (arg == UNMARK_ONLY)
        return PL_DHASH_NEXT;

#ifdef XPC_REPORT_NATIVE_INTERFACE_AND_SET_FLUSHING
    fputs("- Destroying XPCNativeInterface for ", stdout);
    JS_PutString(JSVAL_TO_STRING(iface->GetName()), stdout);
    putc('\n', stdout);
#endif

    XPCNativeInterface::DestroyInstance(iface);
    return PL_DHASH_REMOVE;
}






static PLDHashOperator
NativeUnMarkedSetRemover(PLDHashTable *table, PLDHashEntryHdr *hdr,
                         uint32_t number, void *arg)
{
    XPCNativeSet* set = ((ClassInfo2NativeSetMap::Entry*)hdr)->value;
    if (set->IsMarked())
        return PL_DHASH_NEXT;
    return PL_DHASH_REMOVE;
}

static PLDHashOperator
NativeSetSweeper(PLDHashTable *table, PLDHashEntryHdr *hdr,
                 uint32_t number, void *arg)
{
    XPCNativeSet* set = ((NativeSetMap::Entry*)hdr)->key_value;
    if (set->IsMarked()) {
        set->Unmark();
        return PL_DHASH_NEXT;
    }

    if (arg == UNMARK_ONLY)
        return PL_DHASH_NEXT;

#ifdef XPC_REPORT_NATIVE_INTERFACE_AND_SET_FLUSHING
    printf("- Destroying XPCNativeSet for:\n");
    uint16_t count = set->GetInterfaceCount();
    for (uint16_t k = 0; k < count; k++) {
        XPCNativeInterface* iface = set->GetInterfaceAt(k);
        fputs("    ", stdout);
        JS_PutString(JSVAL_TO_STRING(iface->GetName()), stdout);
        putc('\n', stdout);
    }
#endif

    XPCNativeSet::DestroyInstance(set);
    return PL_DHASH_REMOVE;
}

static PLDHashOperator
JSClassSweeper(PLDHashTable *table, PLDHashEntryHdr *hdr,
               uint32_t number, void *arg)
{
    XPCNativeScriptableShared* shared =
        ((XPCNativeScriptableSharedMap::Entry*) hdr)->key;
    if (shared->IsMarked()) {
#ifdef off_XPC_REPORT_JSCLASS_FLUSHING
        printf("+ Marked XPCNativeScriptableShared for: %s @ %x\n",
               shared->GetJSClass()->name,
               shared->GetJSClass());
#endif
        shared->Unmark();
        return PL_DHASH_NEXT;
    }

    if (arg == UNMARK_ONLY)
        return PL_DHASH_NEXT;

#ifdef XPC_REPORT_JSCLASS_FLUSHING
    printf("- Destroying XPCNativeScriptableShared for: %s @ %x\n",
           shared->GetJSClass()->name,
           shared->GetJSClass());
#endif

    delete shared;
    return PL_DHASH_REMOVE;
}

static PLDHashOperator
DyingProtoKiller(PLDHashTable *table, PLDHashEntryHdr *hdr,
                 uint32_t number, void *arg)
{
    XPCWrappedNativeProto* proto =
        (XPCWrappedNativeProto*)((PLDHashEntryStub*)hdr)->key;
    delete proto;
    return PL_DHASH_REMOVE;
}

static PLDHashOperator
DetachedWrappedNativeProtoMarker(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                 uint32_t number, void *arg)
{
    XPCWrappedNativeProto* proto =
        (XPCWrappedNativeProto*)((PLDHashEntryStub*)hdr)->key;

    proto->Mark();
    return PL_DHASH_NEXT;
}


static JSBool
ContextCallback(JSContext *cx, unsigned operation)
{
    XPCJSRuntime* self = nsXPConnect::GetRuntimeInstance();
    if (self) {
        if (operation == JSCONTEXT_NEW) {
            if (!self->OnJSContextNew(cx))
                return false;
        } else if (operation == JSCONTEXT_DESTROY) {
            delete XPCContext::GetXPCContext(cx);
        }
    }
    return true;
}

namespace xpc {

CompartmentPrivate::~CompartmentPrivate()
{
    MOZ_COUNT_DTOR(xpc::CompartmentPrivate);
}

bool CompartmentPrivate::TryParseLocationURI()
{
    
    if (locationWasParsed)
      return false;
    locationWasParsed = true;

    
    if (location.IsEmpty())
        return false;

    
    
    
    
    
    
    
    
    
    
    

    static const nsDependentCString from("(from: ");
    static const nsDependentCString arrow(" -> ");
    static const size_t fromLength = from.Length();
    static const size_t arrowLength = arrow.Length();

    
    int32_t idx = location.Find(from);
    if (idx < 0)
        return TryParseLocationURICandidate(location);


    
    
    if (TryParseLocationURICandidate(Substring(location, 0, idx)))
        return true;

    
    
    

    
    int32_t ridx = location.RFind(NS_LITERAL_CSTRING(":"));
    nsAutoCString chain(Substring(location, idx + fromLength,
                                  ridx - idx - fromLength));

    
    
    for (;;) {
        idx = chain.RFind(arrow);
        if (idx < 0) {
            
            return TryParseLocationURICandidate(chain);
        }

        
        if (TryParseLocationURICandidate(Substring(chain, idx + arrowLength)))
            return true;

        
        
        idx -= 1;
        
        chain = Substring(chain, 0, idx);
    }
    MOZ_ASSUME_UNREACHABLE("Chain parser loop does not terminate");
}

bool CompartmentPrivate::TryParseLocationURICandidate(const nsACString& uristr)
{
    nsCOMPtr<nsIURI> uri;
    if (NS_FAILED(NS_NewURI(getter_AddRefs(uri), uristr)))
        return false;

    nsAutoCString scheme;
    if (NS_FAILED(uri->GetScheme(scheme)))
        return false;

    
    
    
    if (scheme.EqualsLiteral("data") || scheme.EqualsLiteral("blob"))
        return false;

    locationURI = uri.forget();
    return true;
}

CompartmentPrivate*
EnsureCompartmentPrivate(JSObject *obj)
{
    return EnsureCompartmentPrivate(js::GetObjectCompartment(obj));
}

CompartmentPrivate*
EnsureCompartmentPrivate(JSCompartment *c)
{
    CompartmentPrivate *priv = GetCompartmentPrivate(c);
    if (priv)
        return priv;
    priv = new CompartmentPrivate();
    JS_SetCompartmentPrivate(c, priv);
    return priv;
}

bool
IsXBLScope(JSCompartment *compartment)
{
    
    CompartmentPrivate *priv = GetCompartmentPrivate(compartment);
    if (!priv || !priv->scope)
        return false;
    return priv->scope->IsXBLScope();
}

bool
IsUniversalXPConnectEnabled(JSCompartment *compartment)
{
    CompartmentPrivate *priv = GetCompartmentPrivate(compartment);
    if (!priv)
        return false;
    return priv->universalXPConnectEnabled;
}

bool
IsUniversalXPConnectEnabled(JSContext *cx)
{
    JSCompartment *compartment = js::GetContextCompartment(cx);
    if (!compartment)
        return false;
    return IsUniversalXPConnectEnabled(compartment);
}

bool
EnableUniversalXPConnect(JSContext *cx)
{
    JSCompartment *compartment = js::GetContextCompartment(cx);
    if (!compartment)
        return true;
    
    
    if (AccessCheck::isChrome(compartment))
        return true;
    CompartmentPrivate *priv = GetCompartmentPrivate(compartment);
    if (!priv)
        return true;
    priv->universalXPConnectEnabled = true;

    
    
    bool ok = js::RecomputeWrappers(cx, js::SingleCompartment(compartment),
                                    js::AllCompartments());
    NS_ENSURE_TRUE(ok, false);

    
    
    
    XPCWrappedNativeScope *scope = priv->scope;
    if (!scope)
        return true;
    return nsXPCComponents::AttachComponentsObject(cx, scope);
}

JSObject *
GetJunkScope()
{
    XPCJSRuntime *self = nsXPConnect::GetRuntimeInstance();
    NS_ENSURE_TRUE(self, nullptr);
    return self->GetJunkScope();
}

nsIGlobalObject *
GetJunkScopeGlobal()
{
    JSObject *junkScope = GetJunkScope();
    
    
    if (!junkScope)
        return nullptr;
    return GetNativeForGlobal(junkScope);
}

}

static void
CompartmentDestroyedCallback(JSFreeOp *fop, JSCompartment *compartment)
{
    XPCJSRuntime* self = nsXPConnect::GetRuntimeInstance();
    if (!self)
        return;

    
    
    nsAutoPtr<CompartmentPrivate> priv(GetCompartmentPrivate(compartment));
    JS_SetCompartmentPrivate(compartment, nullptr);
}

void XPCJSRuntime::TraceNativeBlackRoots(JSTracer* trc)
{
    
    
    if (!nsXPConnect::XPConnect()->IsShuttingDown()) {
        
        if (AutoMarkingPtr *roots = Get()->mAutoRoots)
            roots->TraceJSAll(trc);
    }

    {
        XPCAutoLock lock(mMapLock);

        
        
        XPCRootSetElem *e;
        for (e = mObjectHolderRoots; e; e = e->GetNextRoot())
            static_cast<XPCJSObjectHolder*>(e)->TraceJS(trc);
    }

    dom::TraceBlackJS(trc, JS_GetGCParameter(Runtime(), JSGC_NUMBER),
                      nsXPConnect::XPConnect()->IsShuttingDown());
}

void XPCJSRuntime::TraceAdditionalNativeGrayRoots(JSTracer *trc)
{
    XPCAutoLock lock(mMapLock);

    XPCWrappedNativeScope::TraceWrappedNativesInAllScopes(trc, this);

    for (XPCRootSetElem *e = mVariantRoots; e ; e = e->GetNextRoot())
        static_cast<XPCTraceableVariant*>(e)->TraceJS(trc);

    for (XPCRootSetElem *e = mWrappedJSRoots; e ; e = e->GetNextRoot())
        static_cast<nsXPCWrappedJS*>(e)->TraceJS(trc);
}


void
XPCJSRuntime::SuspectWrappedNative(XPCWrappedNative *wrapper,
                                   nsCycleCollectionNoteRootCallback &cb)
{
    if (!wrapper->IsValid() || wrapper->IsWrapperExpired())
        return;

    MOZ_ASSERT(NS_IsMainThread() || NS_IsCycleCollectorThread(),
               "Suspecting wrapped natives from non-CC thread");

    
    
    JSObject* obj = wrapper->GetFlatJSObjectPreserveColor();
    if (xpc_IsGrayGCThing(obj) || cb.WantAllTraces())
        cb.NoteJSRoot(obj);
}

bool
CanSkipWrappedJS(nsXPCWrappedJS *wrappedJS)
{
    JSObject *obj = wrappedJS->GetJSObjectPreserveColor();
    
    
    
    bool isRootWrappedJS = wrappedJS->GetRootWrapper() == wrappedJS;
    if (nsCCUncollectableMarker::sGeneration &&
        (!obj || !xpc_IsGrayGCThing(obj)) &&
        !wrappedJS->IsSubjectToFinalization() &&
        (isRootWrappedJS || CanSkipWrappedJS(wrappedJS->GetRootWrapper()))) {
        if (!wrappedJS->IsAggregatedToNative() || !isRootWrappedJS) {
            return true;
        } else {
            nsISupports* agg = wrappedJS->GetAggregatedNativeObject();
            nsXPCOMCycleCollectionParticipant* cp = nullptr;
            CallQueryInterface(agg, &cp);
            nsISupports* canonical = nullptr;
            agg->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                                reinterpret_cast<void**>(&canonical));
            if (cp && canonical && cp->CanSkipInCC(canonical)) {
                return true;
            }
        }
    }
    return false;
}

void
XPCJSRuntime::TraverseAdditionalNativeRoots(nsCycleCollectionNoteRootCallback &cb)
{
    XPCAutoLock lock(mMapLock);

    XPCWrappedNativeScope::SuspectAllWrappers(this, cb);

    for (XPCRootSetElem *e = mVariantRoots; e ; e = e->GetNextRoot()) {
        XPCTraceableVariant* v = static_cast<XPCTraceableVariant*>(e);
        if (nsCCUncollectableMarker::InGeneration(cb,
                                                  v->CCGeneration())) {
           jsval val = v->GetJSValPreserveColor();
           if (val.isObject() && !xpc_IsGrayGCThing(&val.toObject()))
               continue;
        }
        cb.NoteXPCOMRoot(v);
    }

    for (XPCRootSetElem *e = mWrappedJSRoots; e ; e = e->GetNextRoot()) {
        nsXPCWrappedJS *wrappedJS = static_cast<nsXPCWrappedJS*>(e);
        if (!cb.WantAllTraces() &&
            CanSkipWrappedJS(wrappedJS)) {
            continue;
        }

        cb.NoteXPCOMRoot(static_cast<nsIXPConnectWrappedJS *>(wrappedJS));
    }
}

void
XPCJSRuntime::UnmarkSkippableJSHolders()
{
    XPCAutoLock lock(mMapLock);
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
XPCJSRuntime::PrepareForCollection()
{
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
        obs->NotifyObservers(nullptr, "cycle-collector-begin", nullptr);
    }
}

void
xpc_UnmarkSkippableJSHolders()
{
    if (nsXPConnect::XPConnect()->GetRuntime()) {
        nsXPConnect::XPConnect()->GetRuntime()->UnmarkSkippableJSHolders();
    }
}

template<class T> static void
DoDeferredRelease(nsTArray<T> &array)
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
XPCJSRuntime::GCSliceCallback(JSRuntime *rt,
                              JS::GCProgress progress,
                              const JS::GCDescription &desc)
{
    XPCJSRuntime *self = nsXPConnect::GetRuntimeInstance();
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
    switch (status) {
        case JSGC_BEGIN:
        {
            
            
            JSContext *iter = nullptr;
            while (JSContext *acx = JS_ContextIterator(Runtime(), &iter)) {
                if (!js::HasUnrootedGlobal(acx))
                    JS_ToggleOptions(acx, JSOPTION_UNROOTED_GLOBAL);
            }
            break;
        }
        case JSGC_END:
        {
            break;
        }
    }

    nsTArray<xpcGCCallback> callbacks(extraGCCallbacks);
    for (uint32_t i = 0; i < callbacks.Length(); ++i)
        callbacks[i](status);
}

 void
XPCJSRuntime::FinalizeCallback(JSFreeOp *fop, JSFinalizeStatus status, JSBool isCompartmentGC)
{
    XPCJSRuntime* self = nsXPConnect::GetRuntimeInstance();
    if (!self)
        return;

    switch (status) {
        case JSFINALIZE_GROUP_START:
        {
            NS_ASSERTION(!self->mDoingFinalization, "bad state");

            
            { 
                XPCAutoLock lock(self->GetMapLock());
                NS_ASSERTION(!self->mThreadRunningGC, "bad state");
                self->mThreadRunningGC = PR_GetCurrentThread();
            }

            nsTArray<nsXPCWrappedJS*>* dyingWrappedJSArray =
                &self->mWrappedJSToReleaseArray;

            
            
            
            
            
            
            self->mWrappedJSMap->FindDyingJSObjects(dyingWrappedJSArray);

            
            XPCWrappedNativeScope::StartFinalizationPhaseOfGC(fop, self);

            XPCStringConvert::ClearCache();

            self->mDoingFinalization = true;
            break;
        }
        case JSFINALIZE_GROUP_END:
        {
            NS_ASSERTION(self->mDoingFinalization, "bad state");
            self->mDoingFinalization = false;

            
            
            DoDeferredRelease(self->mWrappedJSToReleaseArray);

            
            XPCWrappedNativeScope::FinishedFinalizationPhaseOfGC();

            
            
            { 
                XPCAutoLock lock(self->GetMapLock());
                NS_ASSERTION(self->mThreadRunningGC == PR_GetCurrentThread(), "bad state");
                self->mThreadRunningGC = nullptr;
                xpc_NotifyAll(self->GetMapLock());
            }

            break;
        }
        case JSFINALIZE_COLLECTION_END:
        {
            
            { 
                XPCAutoLock lock(self->GetMapLock());
                NS_ASSERTION(!self->mThreadRunningGC, "bad state");
                self->mThreadRunningGC = PR_GetCurrentThread();
            }

#ifdef XPC_REPORT_NATIVE_INTERFACE_AND_SET_FLUSHING
            printf("--------------------------------------------------------------\n");
            int setsBefore = (int) self->mNativeSetMap->Count();
            int ifacesBefore = (int) self->mIID2NativeInterfaceMap->Count();
#endif

            
            

            
            XPCWrappedNativeScope::MarkAllWrappedNativesAndProtos();

            self->mDetachedWrappedNativeProtoMap->
                Enumerate(DetachedWrappedNativeProtoMarker, nullptr);

            DOM_MarkInterfaces();

            
            
            
            
            
            

            
            
            if (!nsXPConnect::XPConnect()->IsShuttingDown()) {

                
                if (AutoMarkingPtr *roots = Get()->mAutoRoots)
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

            
            
            
            
            
            
            
            
            
            
            
            void *sweepArg = isCompartmentGC ? UNMARK_ONLY : UNMARK_AND_SWEEP;

            
            
            
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

#ifdef XPC_REPORT_NATIVE_INTERFACE_AND_SET_FLUSHING
            int setsAfter = (int) self->mNativeSetMap->Count();
            int ifacesAfter = (int) self->mIID2NativeInterfaceMap->Count();

            printf("\n");
            printf("XPCNativeSets:        before: %d  collected: %d  remaining: %d\n",
                   setsBefore, setsBefore - setsAfter, setsAfter);
            printf("XPCNativeInterfaces:  before: %d  collected: %d  remaining: %d\n",
                   ifacesBefore, ifacesBefore - ifacesAfter, ifacesAfter);
            printf("--------------------------------------------------------------\n");
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

            
            
            { 
                XPCAutoLock lock(self->GetMapLock());
                NS_ASSERTION(self->mThreadRunningGC == PR_GetCurrentThread(), "bad state");
                self->mThreadRunningGC = nullptr;
                xpc_NotifyAll(self->GetMapLock());
            }

            break;
        }
    }
}

static void WatchdogMain(void *arg);
class Watchdog;
class WatchdogManager;
class AutoLockWatchdog {
    Watchdog* const mWatchdog;
  public:
    AutoLockWatchdog(Watchdog* aWatchdog);
    ~AutoLockWatchdog();
};

class Watchdog
{
  public:
    Watchdog(WatchdogManager *aManager)
      : mManager(aManager)
      , mLock(nullptr)
      , mWakeup(nullptr)
      , mThread(nullptr)
      , mHibernating(false)
      , mInitialized(false)
      , mShuttingDown(false)
    {}
    ~Watchdog() { MOZ_ASSERT(!Initialized()); }

    WatchdogManager* Manager() { return mManager; }
    bool Initialized() { return mInitialized; }
    bool ShuttingDown() { return mShuttingDown; }
    PRLock *GetLock() { return mLock; }
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
                                      PR_PRIORITY_NORMAL, PR_LOCAL_THREAD,
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

  private:
    WatchdogManager *mManager;

    PRLock *mLock;
    PRCondVar *mWakeup;
    PRThread *mThread;
    bool mHibernating;
    bool mInitialized;
    bool mShuttingDown;
};

class WatchdogManager : public nsIObserver
{
  public:

    NS_DECL_ISUPPORTS
    WatchdogManager(XPCJSRuntime *aRuntime) : mRuntime(aRuntime)
                                            , mRuntimeState(RUNTIME_INACTIVE)
    {
        
        PodArrayZero(mTimestamps);
        mTimestamps[TimestampRuntimeStateChange] = PR_Now();

        
        RefreshWatchdog();

        
        mozilla::Preferences::AddStrongObserver(this, "dom.use_watchdog");
    }
    virtual ~WatchdogManager()
    {
        
        
        
        MOZ_ASSERT(!mWatchdog);
        mozilla::Preferences::RemoveObserver(this, "dom.use_watchdog");
    }

    NS_IMETHOD Observe(nsISupports* aSubject, const char* aTopic,
                       const PRUnichar* aData)
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
            lock.construct(mWatchdog);

        
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
            maybeLock.construct(mWatchdog);
        mTimestamps[aCategory] = PR_Now();
    }
    PRTime GetTimestamp(WatchdogTimestampCategory aCategory)
    {
        
        Maybe<AutoLockWatchdog> maybeLock;
        if (NS_IsMainThread() && mWatchdog)
            maybeLock.construct(mWatchdog);
        return mTimestamps[aCategory];
    }

    XPCJSRuntime* Runtime() { return mRuntime; }
    Watchdog* GetWatchdog() { return mWatchdog; }

    void RefreshWatchdog()
    {
        bool wantWatchdog = Preferences::GetBool("dom.use_watchdog", true);
        if (wantWatchdog == !!mWatchdog)
            return;
        if (wantWatchdog)
            StartWatchdog();
        else
            StopWatchdog();
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
    XPCJSRuntime *mRuntime;
    nsAutoPtr<Watchdog> mWatchdog;

    enum { RUNTIME_ACTIVE, RUNTIME_INACTIVE } mRuntimeState;
    PRTime mTimestamps[TimestampCount];
};

NS_IMPL_ISUPPORTS1(WatchdogManager, nsIObserver)

AutoLockWatchdog::AutoLockWatchdog(Watchdog *aWatchdog) : mWatchdog(aWatchdog)
{
    PR_Lock(mWatchdog->GetLock());
}

AutoLockWatchdog::~AutoLockWatchdog()
{
    PR_Unlock(mWatchdog->GetLock());
}

static void
WatchdogMain(void *arg)
{
    PR_SetCurrentThreadName("JS Watchdog");

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

        
        
        
        if (manager->IsRuntimeActive() &&
            manager->TimeSinceLastRuntimeStateChange() >= PRTime(PR_USEC_PER_SEC))
        {
            JS_TriggerOperationCallback(manager->Runtime()->Runtime());
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
XPCJSRuntime::ActivityCallback(void *arg, JSBool active)
{
    XPCJSRuntime* self = static_cast<XPCJSRuntime*>(arg);
    self->mWatchdogManager->RecordRuntimeActivity(active);
}






void
XPCJSRuntime::CTypesActivityCallback(JSContext *cx, js::CTypesActivityType type)
{
  if (type == js::CTYPES_CALLBACK_BEGIN) {
    if (!xpc::PushJSContextNoScriptContext(cx))
      MOZ_CRASH();
  } else if (type == js::CTYPES_CALLBACK_END) {
    xpc::PopJSContextNoScriptContext();
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

XPCReadableJSStringWrapper *
XPCJSRuntime::NewStringWrapper(const PRUnichar *str, uint32_t len)
{
    for (uint32_t i = 0; i < XPCCCX_STRING_CACHE_SIZE; ++i) {
        StringWrapperEntry& ent = mScratchStrings[i];

        if (!ent.mInUse) {
            ent.mInUse = true;

            

            return new (ent.mString.addr()) XPCReadableJSStringWrapper(str, len);
        }
    }

    

    return new XPCReadableJSStringWrapper(str, len);
}

void
XPCJSRuntime::DeleteString(nsAString *string)
{
    for (uint32_t i = 0; i < XPCCCX_STRING_CACHE_SIZE; ++i) {
        StringWrapperEntry& ent = mScratchStrings[i];
        if (string == ent.mString.addr()) {
            
            

            ent.mInUse = false;
            ent.mString.addr()->~XPCReadableJSStringWrapper();

            return;
        }
    }

    
    
    delete string;
}



#ifdef XPC_CHECK_WRAPPERS_AT_SHUTDOWN
static PLDHashOperator
DEBUG_WrapperChecker(PLDHashTable *table, PLDHashEntryHdr *hdr,
                     uint32_t number, void *arg)
{
    XPCWrappedNative* wrapper = (XPCWrappedNative*)((PLDHashEntryStub*)hdr)->key;
    NS_ASSERTION(!wrapper->IsValid(), "found a 'valid' wrapper!");
    ++ *((int*)arg);
    return PL_DHASH_NEXT;
}
#endif

static PLDHashOperator
DetachedWrappedNativeProtoShutdownMarker(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                         uint32_t number, void *arg)
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
    DOM_ClearInterfaces();

    if (mDetachedWrappedNativeProtoMap)
        mDetachedWrappedNativeProtoMap->
            Enumerate(DetachedWrappedNativeProtoShutdownMarker, nullptr);
}

XPCJSRuntime::~XPCJSRuntime()
{
    JS::SetGCSliceCallback(Runtime(), mPrevGCSliceCallback);

    xpc_DelocalizeRuntime(Runtime());

    if (mWatchdogManager->GetWatchdog())
        mWatchdogManager->StopWatchdog();

    if (mCallContext)
        mCallContext->SystemIsBeingShutDown();

#ifdef XPC_DUMP_AT_SHUTDOWN
    {
    
    JSContext* iter = nullptr;
    int count = 0;
    while (JS_ContextIterator(Runtime(), &iter))
        count ++;
    if (count)
        printf("deleting XPCJSRuntime with %d live JSContexts\n", count);
    }
#endif

    
    if (mWrappedJSMap) {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32_t count = mWrappedJSMap->Count();
        if (count)
            printf("deleting XPCJSRuntime with %d live wrapped JSObject\n", (int)count);
#endif
        mWrappedJSMap->ShutdownMarker(Runtime());
        delete mWrappedJSMap;
    }

    if (mWrappedJSClassMap) {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32_t count = mWrappedJSClassMap->Count();
        if (count)
            printf("deleting XPCJSRuntime with %d live nsXPCWrappedJSClass\n", (int)count);
#endif
        delete mWrappedJSClassMap;
    }

    if (mIID2NativeInterfaceMap) {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32_t count = mIID2NativeInterfaceMap->Count();
        if (count)
            printf("deleting XPCJSRuntime with %d live XPCNativeInterfaces\n", (int)count);
#endif
        delete mIID2NativeInterfaceMap;
    }

    if (mClassInfo2NativeSetMap) {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32_t count = mClassInfo2NativeSetMap->Count();
        if (count)
            printf("deleting XPCJSRuntime with %d live XPCNativeSets\n", (int)count);
#endif
        delete mClassInfo2NativeSetMap;
    }

    if (mNativeSetMap) {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32_t count = mNativeSetMap->Count();
        if (count)
            printf("deleting XPCJSRuntime with %d live XPCNativeSets\n", (int)count);
#endif
        delete mNativeSetMap;
    }

    if (mMapLock)
        XPCAutoLock::DestroyLock(mMapLock);

    if (mThisTranslatorMap) {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32_t count = mThisTranslatorMap->Count();
        if (count)
            printf("deleting XPCJSRuntime with %d live ThisTranslator\n", (int)count);
#endif
        delete mThisTranslatorMap;
    }

#ifdef XPC_CHECK_WRAPPERS_AT_SHUTDOWN
    if (DEBUG_WrappedNativeHashtable) {
        int LiveWrapperCount = 0;
        PL_DHashTableEnumerate(DEBUG_WrappedNativeHashtable,
                               DEBUG_WrapperChecker, &LiveWrapperCount);
        if (LiveWrapperCount)
            printf("deleting XPCJSRuntime with %d live XPCWrappedNative (found in wrapper check)\n", (int)LiveWrapperCount);
        PL_DHashTableDestroy(DEBUG_WrappedNativeHashtable);
    }
#endif

    if (mNativeScriptableSharedMap) {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32_t count = mNativeScriptableSharedMap->Count();
        if (count)
            printf("deleting XPCJSRuntime with %d live XPCNativeScriptableShared\n", (int)count);
#endif
        delete mNativeScriptableSharedMap;
    }

    if (mDyingWrappedNativeProtoMap) {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32_t count = mDyingWrappedNativeProtoMap->Count();
        if (count)
            printf("deleting XPCJSRuntime with %d live but dying XPCWrappedNativeProto\n", (int)count);
#endif
        delete mDyingWrappedNativeProtoMap;
    }

    if (mDetachedWrappedNativeProtoMap) {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32_t count = mDetachedWrappedNativeProtoMap->Count();
        if (count)
            printf("deleting XPCJSRuntime with %d live detached XPCWrappedNativeProto\n", (int)count);
#endif
        delete mDetachedWrappedNativeProtoMap;
    }

    JS_ShutDown();
#ifdef MOZ_ENABLE_PROFILER_SPS
    
    if (PseudoStack *stack = mozilla_get_pseudo_stack())
        stack->sampleRuntime(nullptr);
#endif

#ifdef DEBUG
    for (uint32_t i = 0; i < XPCCCX_STRING_CACHE_SIZE; ++i) {
        NS_ASSERTION(!mScratchStrings[i].mInUse, "Uh, string wrapper still in use!");
    }
#endif
}

static void
GetCompartmentName(JSCompartment *c, nsCString &name, bool replaceSlashes)
{
    if (js::IsAtomsCompartment(c)) {
        name.AssignLiteral("atoms");
    } else if (JSPrincipals *principals = JS_GetCompartmentPrincipals(c)) {
        nsJSPrincipals::get(principals)->GetScriptLocation(name);

        
        
        
        
        CompartmentPrivate *compartmentPrivate = GetCompartmentPrivate(c);
        if (compartmentPrivate) {
            const nsACString& location = compartmentPrivate->GetLocation();
            if (!location.IsEmpty() && !location.Equals(name)) {
                name.AppendLiteral(", ");
                name.Append(location);
            }
        }

        
        
        
        if (replaceSlashes)
            name.ReplaceChar('/', '\\');
    } else {
        name.AssignLiteral("null-principal");
    }
}

static int64_t
GetGCChunkTotalBytes()
{
    JSRuntime *rt = nsXPConnect::GetRuntimeInstance()->Runtime();
    return int64_t(JS_GetGCParameter(rt, JSGC_TOTAL_CHUNKS)) * js::gc::ChunkSize;
}




NS_MEMORY_REPORTER_IMPLEMENT(XPConnectJSGCHeap,
                             "js-gc-heap",
                             KIND_OTHER,
                             nsIMemoryReporter::UNITS_BYTES,
                             GetGCChunkTotalBytes,
                             "Memory used by the garbage-collected JavaScript heap.")

static int64_t
GetJSSystemCompartmentCount()
{
    return JS::SystemCompartmentCount(nsXPConnect::GetRuntimeInstance()->Runtime());
}

static int64_t
GetJSUserCompartmentCount()
{
    return JS::UserCompartmentCount(nsXPConnect::GetRuntimeInstance()->Runtime());
}








NS_MEMORY_REPORTER_IMPLEMENT(XPConnectJSSystemCompartmentCount,
    "js-compartments/system",
    KIND_OTHER,
    nsIMemoryReporter::UNITS_COUNT,
    GetJSSystemCompartmentCount,
    "The number of JavaScript compartments for system code.  The sum of this "
    "and 'js-compartments-user' might not match the number of compartments "
    "listed under 'js' if a garbage collection occurs at an inopportune time, "
    "but such cases should be rare.")

NS_MEMORY_REPORTER_IMPLEMENT(XPConnectJSUserCompartmentCount,
    "js-compartments/user",
    KIND_OTHER,
    nsIMemoryReporter::UNITS_COUNT,
    GetJSUserCompartmentCount,
    "The number of JavaScript compartments for user code.  The sum of this "
    "and 'js-compartments-system' might not match the number of compartments "
    "listed under 'js' if a garbage collection occurs at an inopportune time, "
    "but such cases should be rare.")

static int64_t
GetJSMainRuntimeTemporaryPeakSize()
{
    return JS::PeakSizeOfTemporary(nsXPConnect::GetRuntimeInstance()->Runtime());
}


NS_MEMORY_REPORTER_IMPLEMENT(JSMainRuntimeTemporaryPeak,
    "js-main-runtime-temporary-peak",
    KIND_OTHER,
    nsIMemoryReporter::UNITS_BYTES,
    GetJSMainRuntimeTemporaryPeakSize,
    "The peak size of the transient storage in the main JSRuntime (the "
    "current size of which is reported as "
    "'explicit/js-non-window/runtime/temporary').");






#define SUNDRIES_THRESHOLD js::MemoryReportingSundriesThreshold()

#define REPORT(_path, _kind, _units, _amount, _desc)                          \
    do {                                                                      \
        nsresult rv;                                                          \
        rv = cb->Callback(EmptyCString(), _path, _kind, _units, _amount,      \
                          NS_LITERAL_CSTRING(_desc), closure);                \
        NS_ENSURE_SUCCESS(rv, rv);                                            \
    } while (0)

#define REPORT_BYTES(_path, _kind, _amount, _desc)                            \
    REPORT(_path, _kind, nsIMemoryReporter::UNITS_BYTES, _amount, _desc);

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






#define ZCREPORT_BYTES(_path, _amount, _descLiteral)                          \
    do {                                                                      \
        /* Assign _descLiteral plus "" into a char* to prove that it's */     \
        /* actually a literal. */                                             \
        const char* unusedDesc = _descLiteral "";                             \
        (void) unusedDesc;                                                    \
        ZCREPORT_BYTES2(_path, _amount, NS_LITERAL_CSTRING(_descLiteral));    \
    } while (0)



#define ZCREPORT_BYTES2(_path, _amount, _desc)                                \
    do {                                                                      \
        size_t amount = _amount;  /* evaluate _amount only once */            \
        if (amount >= SUNDRIES_THRESHOLD) {                                   \
            nsresult rv;                                                      \
            rv = cb->Callback(EmptyCString(), _path,                          \
                              nsIMemoryReporter::KIND_HEAP,                   \
                              nsIMemoryReporter::UNITS_BYTES, amount,         \
                              _desc, closure);                                \
            NS_ENSURE_SUCCESS(rv, rv);                                        \
        } else {                                                              \
            otherSundries += amount;                                          \
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
            gcHeapSundries += amount;                                         \
        }                                                                     \
    } while (0)

#define RREPORT_BYTES(_path, _kind, _amount, _desc)                           \
    do {                                                                      \
        size_t amount = _amount;  /* evaluate _amount only once */            \
        nsresult rv;                                                          \
        rv = cb->Callback(EmptyCString(), _path, _kind,                       \
                          nsIMemoryReporter::UNITS_BYTES, amount,             \
                          NS_LITERAL_CSTRING(_desc), closure);                \
        NS_ENSURE_SUCCESS(rv, rv);                                            \
        rtTotal += amount;                                                    \
    } while (0)

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(JsMallocSizeOf)

namespace xpc {

static nsresult
ReportZoneStats(const JS::ZoneStats &zStats,
                const xpc::ZoneStatsExtras &extras,
                nsIMemoryMultiReporterCallback *cb,
                nsISupports *closure, size_t *gcTotalOut = NULL)
{
    const nsAutoCString& pathPrefix = extras.pathPrefix;
    size_t gcTotal = 0, gcHeapSundries = 0, otherSundries = 0;

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap/arena-admin"),
                      zStats.gcHeapArenaAdmin,
                      "Memory on the garbage-collected JavaScript "
                      "heap, within arenas, that is used (a) to hold internal "
                      "bookkeeping information, and (b) to provide padding to "
                      "align GC things.");

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap/unused-gc-things"),
                      zStats.gcHeapUnusedGcThings,
                      "Memory on the garbage-collected JavaScript "
                      "heap taken by empty GC thing slots within non-empty "
                      "arenas.");

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap/strings/normal"),
                      zStats.gcHeapStringsNormal,
                      "Memory on the garbage-collected JavaScript "
                      "heap that holds normal string headers.  String headers contain "
                      "various pieces of information about a string, but do not "
                      "contain (except in the case of very short strings) the "
                      "string characters;  characters in longer strings are "
                      "counted under 'gc-heap/string-chars' instead.");

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap/strings/short"),
                      zStats.gcHeapStringsShort,
                      "Memory on the garbage-collected JavaScript "
                      "heap that holds over-sized string headers, in which "
                      "string characters are stored inline.");

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap/lazy-scripts"),
                      zStats.gcHeapLazyScripts,
                      "Memory on the garbage-collected JavaScript "
                      "heap that represents scripts which haven't executed yet.");

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap/type-objects"),
                      zStats.gcHeapTypeObjects,
                      "Memory on the garbage-collected JavaScript "
                      "heap that holds type inference information.");

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap/ion-codes"),
                      zStats.gcHeapIonCodes,
                      "Memory on the garbage-collected JavaScript "
                      "heap that holds references to executable code pools "
                      "used by the IonMonkey JIT.");

    ZCREPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("lazy-script-data"),
                   zStats.lazyScripts,
                   "Memory holding miscellaneous additional information associated with lazy "
                   "scripts.");

    ZCREPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("type-objects"),
                   zStats.typeObjects,
                   "Memory holding miscellaneous additional information associated with type "
                   "objects.");

    ZCREPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("type-pool"),
                   zStats.typePool,
                   "Memory holding contents of type sets and related data.");

    ZCREPORT_BYTES2(pathPrefix + NS_LITERAL_CSTRING("string-chars/non-huge"),
                    zStats.stringCharsNonHuge, nsPrintfCString(
                    "Memory allocated to hold characters of strings whose "
                    "characters take up less than than %d bytes of memory.\n\n"
                    "Sometimes more memory is allocated than necessary, to "
                    "simplify string concatenation.  Each string also includes a "
                    "header which is stored on the compartment's JavaScript heap; "
                    "that header is not counted here, but in 'gc-heap/strings' "
                    "instead.",
                    JS::HugeStringInfo::MinSize()));

    for (size_t i = 0; i < zStats.hugeStrings.length(); i++) {
        const JS::HugeStringInfo& info = zStats.hugeStrings[i];

        nsDependentCString hugeString(info.buffer);

        
        
        
        nsCString escapedString(hugeString);
        escapedString.ReplaceSubstring("/", "\\/");

        ZCREPORT_BYTES2(
            pathPrefix +
            nsPrintfCString("string-chars/huge/string(length=%d, \"%s...\")",
                            info.length, escapedString.get()),
            info.size,
            nsPrintfCString("Memory allocated to hold characters of "
            "a length-%d string which begins \"%s\".\n\n"
            "Sometimes more memory is allocated than necessary, to simplify "
            "string concatenation.  Each string also includes a header which is "
            "stored on the compartment's JavaScript heap; that header is not "
            "counted here, but in 'gc-heap/strings' instead.",
            info.length, hugeString.get()));
    }

    if (gcHeapSundries > 0) {
        
        REPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap/sundries"),
                        gcHeapSundries,
                        "The sum of all the gc-heap measurements that are too "
                        "small to be worth showing individually.");
    }

    if (otherSundries > 0) {
        
        REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("other-sundries"),
                     nsIMemoryReporter::KIND_HEAP, otherSundries,
                     "The sum of all the non-gc-heap measurements that are too "
                     "small to be worth showing individually.");
    }

    if (gcTotalOut)
        *gcTotalOut += gcTotal;

    return NS_OK;
}

static nsresult
ReportCompartmentStats(const JS::CompartmentStats &cStats,
                       const xpc::CompartmentStatsExtras &extras,
                       amIAddonManager *addonManager,
                       nsIMemoryMultiReporterCallback *cb,
                       nsISupports *closure, size_t *gcTotalOut = NULL)
{
    static const nsDependentCString addonPrefix("explicit/add-ons/");

    size_t gcTotal = 0, gcHeapSundries = 0, otherSundries = 0;
    nsAutoCString cJSPathPrefix = extras.jsPathPrefix;
    nsAutoCString cDOMPathPrefix = extras.domPathPrefix;

    
    
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

    ZCREPORT_GC_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("gc-heap/objects/ordinary"),
                      cStats.gcHeapObjectsOrdinary,
                      "Memory on the garbage-collected JavaScript "
                      "heap that holds ordinary (i.e. not otherwise distinguished "
                      "my memory reporters) objects.");

    ZCREPORT_GC_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("gc-heap/objects/function"),
                      cStats.gcHeapObjectsFunction,
                      "Memory on the garbage-collected JavaScript "
                      "heap that holds function objects.");

    ZCREPORT_GC_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("gc-heap/objects/dense-array"),
                      cStats.gcHeapObjectsDenseArray,
                      "Memory on the garbage-collected JavaScript "
                      "heap that holds dense array objects.");

    ZCREPORT_GC_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("gc-heap/objects/slow-array"),
                      cStats.gcHeapObjectsSlowArray,
                      "Memory on the garbage-collected JavaScript "
                      "heap that holds slow array objects.");

    ZCREPORT_GC_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("gc-heap/objects/cross-compartment-wrapper"),
                      cStats.gcHeapObjectsCrossCompartmentWrapper,
                      "Memory on the garbage-collected JavaScript "
                      "heap that holds cross-compartment wrapper objects.");

    ZCREPORT_GC_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("gc-heap/scripts"),
                      cStats.gcHeapScripts,
                      "Memory on the garbage-collected JavaScript "
                      "heap that holds JSScript instances. A JSScript is "
                      "created for each user-defined function in a script. One "
                      "is also created for the top-level code in a script.");

    ZCREPORT_GC_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("gc-heap/shapes/tree/global-parented"),
                      cStats.gcHeapShapesTreeGlobalParented,
                      "Memory on the garbage-collected JavaScript heap that "
                      "holds shapes that (a) are in a property tree, and (b) "
                      "represent an object whose parent is the global object.");

    ZCREPORT_GC_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("gc-heap/shapes/tree/non-global-parented"),
                      cStats.gcHeapShapesTreeNonGlobalParented,
                      "Memory on the garbage-collected JavaScript heap that "
                      "holds shapes that (a) are in a property tree, and (b) "
                      "represent an object whose parent is not the global object.");

    ZCREPORT_GC_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("gc-heap/shapes/dict"),
                      cStats.gcHeapShapesDict,
                      "Memory on the garbage-collected JavaScript "
                      "heap that holds shapes that are in dictionary mode.");

    ZCREPORT_GC_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("gc-heap/shapes/base"),
                      cStats.gcHeapShapesBase,
                      "Memory on the garbage-collected JavaScript "
                      "heap that collates data common to many shapes.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("objects-extra/slots"),
                   cStats.objectsExtra.slots,
                   "Memory allocated for the non-fixed object "
                   "slot arrays, which are used to represent object properties. "
                   "Some objects also contain a fixed number of slots which are "
                   "stored on the JavaScript heap; those slots "
                   "are not counted here, but in 'gc-heap/objects' instead.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("objects-extra/elements/non-asm.js"),
                   cStats.objectsExtra.elementsNonAsmJS,
                   "Memory allocated for non-asm.js object element arrays, "
                   "which are used to represent indexed object properties.");

    
    
    
    
    
    #define ASM_JS_DESC "Memory allocated for object element " \
                        "arrays used as asm.js array buffers."
    size_t asmJSHeap    = cStats.objectsExtra.elementsAsmJSHeap;
    size_t asmJSNonHeap = cStats.objectsExtra.elementsAsmJSNonHeap;
    JS_ASSERT(asmJSHeap == 0 || asmJSNonHeap == 0);
    if (asmJSHeap > 0) {
        REPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("objects-extra/elements/asm.js"),
                     nsIMemoryReporter::KIND_HEAP, asmJSHeap, ASM_JS_DESC);
    }
    if (asmJSNonHeap > 0) {
        REPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("objects-extra/elements/asm.js"),
                     nsIMemoryReporter::KIND_NONHEAP, asmJSNonHeap, ASM_JS_DESC);
    }

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("objects-extra/arguments-data"),
                   cStats.objectsExtra.argumentsData,
                   "Memory allocated for data belonging to arguments objects.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("objects-extra/regexp-statics"),
                   cStats.objectsExtra.regExpStatics,
                   "Memory allocated for data belonging to the RegExpStatics object.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("objects-extra/property-iterator-data"),
                   cStats.objectsExtra.propertyIteratorData,
                   "Memory allocated for data belonging to property iterator objects.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("objects-extra/ctypes-data"),
                   cStats.objectsExtra.ctypesData,
                   "Memory allocated for data belonging to ctypes objects.");

    
    
    
    ZCREPORT_BYTES(cDOMPathPrefix + NS_LITERAL_CSTRING("orphan-nodes"),
                   cStats.objectsExtra.private_,
                   "Memory used by orphan DOM nodes that are only reachable "
                   "from JavaScript objects.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("shapes-extra/tree-tables"),
                   cStats.shapesExtraTreeTables,
                   "Memory allocated for the property tables "
                   "that belong to shapes that are in a property tree.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("shapes-extra/dict-tables"),
                   cStats.shapesExtraDictTables,
                   "Memory allocated for the property tables "
                   "that belong to shapes that are in dictionary mode.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("shapes-extra/tree-shape-kids"),
                   cStats.shapesExtraTreeShapeKids,
                   "Memory allocated for the kid hashes that "
                   "belong to shapes that are in a property tree.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("shapes-extra/compartment-tables"),
                   cStats.shapesCompartmentTables,
                   "Memory used by compartment-wide tables storing shape "
                   "information for use during object construction.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("script-data"),
                   cStats.scriptData,
                   "Memory allocated for various variable-length tables in JSScript.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("baseline/data"),
                   cStats.baselineData,
                   "Memory used by the Baseline JIT for compilation data: "
                   "BaselineScripts.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("baseline/stubs/fallback"),
                   cStats.baselineStubsFallback,
                   "Memory used by the Baseline JIT for fallback IC stubs "
                   "(excluding code).");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("baseline/stubs/optimized"),
                   cStats.baselineStubsOptimized,
                   "Memory used by the Baseline JIT for optimized IC stubs "
                   "(excluding code).");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("ion-data"),
                   cStats.ionData,
                   "Memory used by the IonMonkey JIT for compilation data: "
                   "IonScripts.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("compartment-object"),
                   cStats.compartmentObject,
                   "Memory used for the JSCompartment object itself.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("cross-compartment-wrapper-table"),
                   cStats.crossCompartmentWrappersTable,
                   "Memory used by the cross-compartment wrapper table.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("regexp-compartment"),
                   cStats.regexpCompartment,
                   "Memory used by the regexp compartment.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("debuggees-set"),
                   cStats.debuggeesSet,
                   "Memory used by the debuggees set.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("type-inference/type-scripts"),
                   cStats.typeInference.typeScripts,
                   "Memory used by type sets associated with scripts.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("type-inference/type-results"),
                   cStats.typeInference.typeResults,
                   "Memory used by dynamic type results produced by scripts.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("type-inference/analysis-pool"),
                   cStats.typeInference.analysisPool,
                   "Memory holding transient analysis information used during type inference and "
                   "compilation.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("type-inference/pending-arrays"),
                   cStats.typeInference.pendingArrays,
                   "Memory used for solving constraints during type inference.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("type-inference/allocation-site-tables"),
                   cStats.typeInference.allocationSiteTables,
                   "Memory indexing type objects associated with allocation sites.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("type-inference/array-type-tables"),
                   cStats.typeInference.arrayTypeTables,
                   "Memory indexing type objects associated with array literals.");

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("type-inference/object-type-tables"),
                   cStats.typeInference.objectTypeTables,
                   "Memory indexing type objects associated with object literals.");

    if (gcHeapSundries > 0) {
        
        REPORT_GC_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("gc-heap/sundries"),
                        gcHeapSundries,
                        "The sum of all the gc-heap "
                        "measurements that are too small to be worth showing "
                        "individually.");
    }

    if (otherSundries > 0) {
        
        REPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("other-sundries"),
                     nsIMemoryReporter::KIND_HEAP, otherSundries,
                     "The sum of all the non-gc-heap "
                     "measurements that are too small to be worth showing "
                     "individually.");
    }

    if (gcTotalOut)
        *gcTotalOut += gcTotal;

    return NS_OK;
}

static nsresult
ReportJSRuntimeExplicitTreeStats(const JS::RuntimeStats &rtStats,
                                 const nsACString &rtPath,
                                 amIAddonManager* addonManager,
                                 nsIMemoryMultiReporterCallback *cb,
                                 nsISupports *closure, size_t *rtTotalOut)
{
    nsresult rv;

    size_t gcTotal = 0;

    for (size_t i = 0; i < rtStats.zoneStatsVector.length(); i++) {
        JS::ZoneStats zStats = rtStats.zoneStatsVector[i];
        const xpc::ZoneStatsExtras *extras =
          static_cast<const xpc::ZoneStatsExtras*>(zStats.extra);
        rv = ReportZoneStats(zStats, *extras, cb, closure, &gcTotal);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    for (size_t i = 0; i < rtStats.compartmentStatsVector.length(); i++) {
        JS::CompartmentStats cStats = rtStats.compartmentStatsVector[i];
        const xpc::CompartmentStatsExtras *extras =
            static_cast<const xpc::CompartmentStatsExtras*>(cStats.extra);
        rv = ReportCompartmentStats(cStats, *extras, addonManager, cb, closure,
                                    &gcTotal);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    

    size_t rtTotal = 0;

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/runtime-object"),
                  nsIMemoryReporter::KIND_HEAP, rtStats.runtime.object,
                  "Memory used by the JSRuntime object.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/atoms-table"),
                  nsIMemoryReporter::KIND_HEAP, rtStats.runtime.atomsTable,
                  "Memory used by the atoms table.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/contexts"),
                  nsIMemoryReporter::KIND_HEAP, rtStats.runtime.contexts,
                  "Memory used by JSContext objects and certain structures "
                  "hanging off them.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/dtoa"),
                  nsIMemoryReporter::KIND_HEAP, rtStats.runtime.dtoa,
                  "Memory used by DtoaState, which is used for converting "
                  "strings to numbers and vice versa.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/temporary"),
                  nsIMemoryReporter::KIND_HEAP, rtStats.runtime.temporary,
                  "Memory held transiently in JSRuntime and used during "
                  "compilation.  It mostly holds parse nodes.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/code/ion"),
                  nsIMemoryReporter::KIND_NONHEAP, rtStats.runtime.code.ion,
                  "Memory used by the IonMonkey JIT to hold generated code.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/code/baseline"),
                  nsIMemoryReporter::KIND_NONHEAP, rtStats.runtime.code.baseline,
                  "Memory used by the Baseline JIT to hold generated code.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/code/asm.js"),
                  nsIMemoryReporter::KIND_NONHEAP, rtStats.runtime.code.asmJS,
                  "Memory used by AOT-compiled asm.js code.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/code/regexp"),
                  nsIMemoryReporter::KIND_NONHEAP, rtStats.runtime.code.regexp,
                  "Memory used by the regexp JIT to hold generated code.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/code/other"),
                  nsIMemoryReporter::KIND_NONHEAP, rtStats.runtime.code.other,
                  "Memory used by the JITs to hold generated code for "
                  "wrappers and trampolines.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/code/unused"),
                  nsIMemoryReporter::KIND_NONHEAP, rtStats.runtime.code.unused,
                  "Memory allocated by one of the JITs to hold code, "
                  "but which is currently unused.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/regexp-data"),
                  nsIMemoryReporter::KIND_NONHEAP, rtStats.runtime.regexpData,
                  "Memory used by the regexp JIT to hold data.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/interpreter-stack"),
                  nsIMemoryReporter::KIND_HEAP, rtStats.runtime.interpreterStack,
                  "Memory used for JS interpreter frames.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/gc-marker"),
                  nsIMemoryReporter::KIND_HEAP, rtStats.runtime.gcMarker,
                  "Memory used for the GC mark stack and gray roots.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/math-cache"),
                  nsIMemoryReporter::KIND_HEAP, rtStats.runtime.mathCache,
                  "Memory used for the math cache.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/script-data"),
                  nsIMemoryReporter::KIND_HEAP, rtStats.runtime.scriptData,
                  "Memory used for the table holding script data shared in "
                  "the runtime.");

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/script-sources"),
                  nsIMemoryReporter::KIND_HEAP, rtStats.runtime.scriptSources,
                  "Memory use for storing JavaScript source code and filenames.");

    if (rtTotalOut)
        *rtTotalOut = rtTotal;

    

    
    
    nsCString rtPath2(rtPath);
    rtPath2.Replace(0, strlen("explicit"), NS_LITERAL_CSTRING("decommitted"));
    REPORT_GC_BYTES(rtPath2 + NS_LITERAL_CSTRING("gc-heap/decommitted-arenas"),
                    rtStats.gcHeapDecommittedArenas,
                    "Memory on the garbage-collected JavaScript heap, in "
                    "arenas in non-empty chunks, that is returned to the OS. "
                    "This means it takes up address space but no physical "
                    "memory or swap space.");

    REPORT_GC_BYTES(rtPath + NS_LITERAL_CSTRING("gc-heap/unused-chunks"),
                    rtStats.gcHeapUnusedChunks,
                    "Memory on the garbage-collected JavaScript heap taken by "
                    "empty chunks, which will soon be released unless claimed "
                    "for new allocations.");

    REPORT_GC_BYTES(rtPath + NS_LITERAL_CSTRING("gc-heap/unused-arenas"),
                    rtStats.gcHeapUnusedArenas,
                    "Memory on the garbage-collected JavaScript heap taken by "
                    "empty arenas within non-empty chunks.");

    REPORT_GC_BYTES(rtPath + NS_LITERAL_CSTRING("gc-heap/chunk-admin"),
                    rtStats.gcHeapChunkAdmin,
                    "Memory on the garbage-collected JavaScript heap, within "
                    "chunks, that is used to hold internal bookkeeping "
                    "information.");

    
    
    MOZ_ASSERT(gcTotal == rtStats.gcHeapChunkTotal);

    return NS_OK;
}

nsresult
ReportJSRuntimeExplicitTreeStats(const JS::RuntimeStats &rtStats,
                                 const nsACString &rtPath,
                                 nsIMemoryMultiReporterCallback *cb,
                                 nsISupports *closure, size_t *rtTotalOut)
{
    nsCOMPtr<amIAddonManager> am =
      do_GetService("@mozilla.org/addons/integration;1");
    return ReportJSRuntimeExplicitTreeStats(rtStats, rtPath, am.get(), cb,
                                            closure, rtTotalOut);
}


} 

class JSCompartmentsMultiReporter MOZ_FINAL : public nsIMemoryMultiReporter
{
  public:
    NS_DECL_THREADSAFE_ISUPPORTS

    NS_IMETHOD GetName(nsACString &name) {
        name.AssignLiteral("compartments");
        return NS_OK;
    }

    typedef js::Vector<nsCString, 0, js::SystemAllocPolicy> Paths;

    static void CompartmentCallback(JSRuntime *rt, void* data, JSCompartment *c) {
        
        Paths *paths = static_cast<Paths *>(data);
        nsCString path;
        GetCompartmentName(c, path, true);
        path.Insert(js::IsSystemCompartment(c)
                    ? NS_LITERAL_CSTRING("compartments/system/")
                    : NS_LITERAL_CSTRING("compartments/user/"),
                    0);
        paths->append(path);
    }

    NS_IMETHOD CollectReports(nsIMemoryMultiReporterCallback *cb,
                              nsISupports *closure)
    {
        
        
        

        

        Paths paths;
        JS_IterateCompartments(nsXPConnect::GetRuntimeInstance()->Runtime(),
                               &paths, CompartmentCallback);

        
        for (size_t i = 0; i < paths.length(); i++)
            
            REPORT(nsCString(paths[i]),
                   nsIMemoryReporter::KIND_OTHER,
                   nsIMemoryReporter::UNITS_COUNT,
                   1, "");

        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS1(JSCompartmentsMultiReporter
                              , nsIMemoryMultiReporter
                              )

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(OrphanMallocSizeOf)

namespace xpc {

static size_t
SizeOfTreeIncludingThis(nsINode *tree)
{
    size_t n = tree->SizeOfIncludingThis(OrphanMallocSizeOf);
    for (nsIContent* child = tree->GetFirstChild(); child; child = child->GetNextNode(tree))
        n += child->SizeOfIncludingThis(OrphanMallocSizeOf);

    return n;
}

class OrphanReporter : public JS::ObjectPrivateVisitor
{
  public:
    OrphanReporter(GetISupportsFun aGetISupports)
      : JS::ObjectPrivateVisitor(aGetISupports)
    {
        mAlreadyMeasuredOrphanTrees.Init();
    }

    virtual size_t sizeOfIncludingThis(nsISupports *aSupports) {
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

class XPCJSRuntimeStats : public JS::RuntimeStats
{
    WindowPaths *mWindowPaths;
    WindowPaths *mTopWindowPaths;
    bool mGetLocations;

  public:
    XPCJSRuntimeStats(WindowPaths *windowPaths, WindowPaths *topWindowPaths,
                      bool getLocations)
      : JS::RuntimeStats(JsMallocSizeOf),
        mWindowPaths(windowPaths),
        mTopWindowPaths(topWindowPaths),
        mGetLocations(getLocations)
    {}

    ~XPCJSRuntimeStats() {
        for (size_t i = 0; i != compartmentStatsVector.length(); ++i)
            delete static_cast<xpc::CompartmentStatsExtras*>(compartmentStatsVector[i].extra);


        for (size_t i = 0; i != zoneStatsVector.length(); ++i)
            delete static_cast<xpc::ZoneStatsExtras*>(zoneStatsVector[i].extra);
    }

    virtual void initExtraZoneStats(JS::Zone *zone, JS::ZoneStats *zStats) MOZ_OVERRIDE {
        
        nsXPConnect *xpc = nsXPConnect::XPConnect();
        AutoSafeJSContext cx;
        JSCompartment *comp = js::GetAnyCompartmentInZone(zone);
        xpc::ZoneStatsExtras *extras = new xpc::ZoneStatsExtras;
        extras->pathPrefix.AssignLiteral("explicit/js-non-window/zones/");
        RootedObject global(cx, JS_GetGlobalForCompartmentOrNull(cx, comp));
        if (global) {
            
            
            JSAutoCompartment ac(cx, global);
            nsISupports *native = xpc->GetNativeOfWrapper(cx, global);
            if (nsCOMPtr<nsPIDOMWindow> piwindow = do_QueryInterface(native)) {
                
                
                if (mTopWindowPaths->Get(piwindow->WindowID(),
                                         &extras->pathPrefix))
                    extras->pathPrefix.AppendLiteral("/js-");
            }
        }

        extras->pathPrefix += nsPrintfCString("zone(0x%p)/", (void *)zone);

        zStats->extra = extras;
    }

    virtual void initExtraCompartmentStats(JSCompartment *c,
                                           JS::CompartmentStats *cstats) MOZ_OVERRIDE
    {
        xpc::CompartmentStatsExtras *extras = new xpc::CompartmentStatsExtras;
        nsCString cName;
        GetCompartmentName(c, cName, true);
        if (mGetLocations) {
            CompartmentPrivate *cp = GetCompartmentPrivate(c);
            if (cp)
              cp->GetLocationURI(getter_AddRefs(extras->location));
            
            
            
        }

        
        nsXPConnect *xpc = nsXPConnect::XPConnect();
        AutoSafeJSContext cx;
        bool needZone = true;
        RootedObject global(cx, JS_GetGlobalForCompartmentOrNull(cx, c));
        if (global) {
            
            
            JSAutoCompartment ac(cx, global);
            nsISupports *native = xpc->GetNativeOfWrapper(cx, global);
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
            extras->jsPathPrefix += nsPrintfCString("zone(0x%p)/", (void *)js::GetCompartmentZone(c));

        extras->jsPathPrefix += NS_LITERAL_CSTRING("compartment(") + cName + NS_LITERAL_CSTRING(")/");

        
        
        
        
        
        
        
        
        
        
        

        cstats->extra = extras;
    }
};

nsresult
JSMemoryMultiReporter::CollectReports(WindowPaths *windowPaths,
                                      WindowPaths *topWindowPaths,
                                      nsIMemoryMultiReporterCallback *cb,
                                      nsISupports *closure)
{
    XPCJSRuntime *xpcrt = nsXPConnect::GetRuntimeInstance();

    
    
    
    
    

    nsCOMPtr<amIAddonManager> addonManager =
      do_GetService("@mozilla.org/addons/integration;1");
    bool getLocations = !!addonManager;
    XPCJSRuntimeStats rtStats(windowPaths, topWindowPaths, getLocations);
    OrphanReporter orphanReporter(XPCConvert::GetISupportsFromJSObject);
    if (!JS::CollectRuntimeStats(xpcrt->Runtime(), &rtStats, &orphanReporter))
        return NS_ERROR_FAILURE;

    size_t xpconnect =
        xpcrt->SizeOfIncludingThis(JsMallocSizeOf) +
        XPCWrappedNativeScope::SizeOfAllScopesIncludingThis(JsMallocSizeOf);

    
    

    nsresult rv;
    size_t rtTotal = 0;
    rv = xpc::ReportJSRuntimeExplicitTreeStats(rtStats,
                                               NS_LITERAL_CSTRING("explicit/js-non-window/"),
                                               addonManager, cb, closure,
                                               &rtTotal);
    NS_ENSURE_SUCCESS(rv, rv);

    
    xpc::CompartmentStatsExtras cExtrasTotal;
    cExtrasTotal.jsPathPrefix.AssignLiteral("js-main-runtime/compartments/");
    cExtrasTotal.domPathPrefix.AssignLiteral("window-objects/dom/");
    rv = ReportCompartmentStats(rtStats.cTotals, cExtrasTotal, addonManager,
                                cb, closure);
    NS_ENSURE_SUCCESS(rv, rv);

    xpc::ZoneStatsExtras zExtrasTotal;
    zExtrasTotal.pathPrefix.AssignLiteral("js-main-runtime/zones/");
    rv = ReportZoneStats(rtStats.zTotals, zExtrasTotal, cb, closure);
    NS_ENSURE_SUCCESS(rv, rv);

    
    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime/runtime"),
                 nsIMemoryReporter::KIND_OTHER, rtTotal,
                 "The sum of all measurements under 'explicit/js-non-window/runtime/'.");

    

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime/gc-heap/unused-chunks"),
                 nsIMemoryReporter::KIND_OTHER,
                 rtStats.gcHeapUnusedChunks,
                 "The same as 'explicit/js-non-window/gc-heap/unused-chunks'.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime/gc-heap/unused-arenas"),
                 nsIMemoryReporter::KIND_OTHER,
                 rtStats.gcHeapUnusedArenas,
                 "The same as 'explicit/js-non-window/gc-heap/unused-arenas'.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime/gc-heap/chunk-admin"),
                 nsIMemoryReporter::KIND_OTHER,
                 rtStats.gcHeapChunkAdmin,
                 "The same as 'explicit/js-non-window/gc-heap/chunk-admin'.");

    

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/unused/chunks"),
                 nsIMemoryReporter::KIND_OTHER,
                 rtStats.gcHeapUnusedChunks,
                 "The same as 'explicit/js-non-window/gc-heap/unused-chunks'.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/unused/arenas"),
                 nsIMemoryReporter::KIND_OTHER,
                 rtStats.gcHeapUnusedArenas,
                 "The same as 'explicit/js-non-window/gc-heap/unused-arenas'.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/unused/gc-things"),
                 nsIMemoryReporter::KIND_OTHER,
                 rtStats.zTotals.gcHeapUnusedGcThings,
                 "The same as 'js-main-runtime/compartments/gc-heap/unused-gc-things'.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/used/chunk-admin"),
                 nsIMemoryReporter::KIND_OTHER,
                 rtStats.gcHeapChunkAdmin,
                 "The same as 'explicit/js-non-window/gc-heap/chunk-admin'.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/used/arena-admin"),
                 nsIMemoryReporter::KIND_OTHER,
                 rtStats.zTotals.gcHeapArenaAdmin,
                 "The same as 'js-main-runtime/compartments/gc-heap/arena-admin'.");

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed/used/gc-things"),
                 nsIMemoryReporter::KIND_OTHER,
                 rtStats.gcHeapGcThings,
                 "Memory on the garbage-collected JavaScript heap that holds GC things such "
                 "as objects, strings, scripts, etc.")

    

    REPORT_BYTES(NS_LITERAL_CSTRING("explicit/xpconnect"),
                 nsIMemoryReporter::KIND_HEAP, xpconnect,
                 "Memory used by XPConnect.");

    return NS_OK;
}

} 

#ifdef MOZ_CRASHREPORTER
static JSBool
DiagnosticMemoryCallback(void *ptr, size_t size)
{
    return CrashReporter::RegisterAppMemory(ptr, size) == NS_OK;
}
#endif

static void
AccumulateTelemetryCallback(int id, uint32_t sample)
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
    }
}

static void
CompartmentNameCallback(JSRuntime *rt, JSCompartment *comp,
                        char *buf, size_t bufsize)
{
    nsCString name;
    GetCompartmentName(comp, name, false);
    if (name.Length() >= bufsize)
        name.Truncate(bufsize - 1);
    memcpy(buf, name.get(), name.Length() + 1);
}

static bool
PreserveWrapper(JSContext *cx, JSObject *obj)
{
    MOZ_ASSERT(cx);
    MOZ_ASSERT(obj);
    MOZ_ASSERT(IS_WN_REFLECTOR(obj) || mozilla::dom::IsDOMObject(obj));

    return mozilla::dom::IsDOMObject(obj) && mozilla::dom::TryPreserveWrapper(obj);
}

static nsresult
ReadSourceFromFilename(JSContext *cx, const char *filename, jschar **src, uint32_t *len)
{
  nsresult rv;

  
  
  const char *arrow;
  while ((arrow = strstr(filename, " -> ")))
    filename = arrow + strlen(" -> ");

  
  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), filename);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChannel> scriptChannel;
  rv = NS_NewChannel(getter_AddRefs(scriptChannel), uri);
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

  unsigned char *ptr = buf, *end = ptr + rawLen;
  while (ptr < end) {
    uint32_t bytesRead;
    rv = scriptStream->Read(reinterpret_cast<char *>(ptr), end - ptr, &bytesRead);
    if (NS_FAILED(rv))
      return rv;
    NS_ASSERTION(bytesRead > 0, "stream promised more bytes before EOF");
    ptr += bytesRead;
  }

  nsString decoded;
  rv = nsScriptLoader::ConvertToUTF16(scriptChannel, buf, rawLen, EmptyString(), NULL, decoded);
  NS_ENSURE_SUCCESS(rv, rv);

  
  *len = decoded.Length();
  *src = static_cast<jschar *>(JS_malloc(cx, decoded.Length()*sizeof(jschar)));
  if (!*src)
    return NS_ERROR_FAILURE;
  memcpy(*src, decoded.get(), decoded.Length()*sizeof(jschar));

  return NS_OK;
}





static bool
SourceHook(JSContext *cx, JS::Handle<JSScript*> script, jschar **src,
           uint32_t *length)
{
  *src = NULL;
  *length = 0;

  if (!nsContentUtils::IsCallerChrome())
    return true;

  const char *filename = JS_GetScriptFilename(cx, script);
  if (!filename)
    return true;

  nsresult rv = ReadSourceFromFilename(cx, filename, src, length);
  if (NS_FAILED(rv)) {
    xpc::Throw(cx, rv);
    return false;
  }

  return true;
}



static Atomic<size_t> sSizeOfICU;

static int64_t
GetICUSize()
{
    return sSizeOfICU;
}

NS_MEMORY_REPORTER_IMPLEMENT(ICU,
    "explicit/icu",
    KIND_HEAP,
    UNITS_BYTES,
    GetICUSize,
    "Memory used by ICU, a Unicode and globalization support library."
)

NS_MEMORY_REPORTER_MALLOC_SIZEOF_ON_ALLOC_FUN(ICUMallocSizeOfOnAlloc)
NS_MEMORY_REPORTER_MALLOC_SIZEOF_ON_FREE_FUN(ICUMallocSizeOfOnFree)

static void *
ICUAlloc(const void *, size_t size)
{
    void *p = malloc(size);
    sSizeOfICU += ICUMallocSizeOfOnAlloc(p);
    return p;
}

static void *
ICURealloc(const void *, void *p, size_t size)
{
    sSizeOfICU -= ICUMallocSizeOfOnFree(p);
    void *pnew = realloc(p, size);
    if (pnew) {
        sSizeOfICU += ICUMallocSizeOfOnAlloc(pnew);
    } else {
        
        sSizeOfICU += ICUMallocSizeOfOnAlloc(p);
    }
    return pnew;
}

static void
ICUFree(const void *, void *p)
{
    sSizeOfICU -= ICUMallocSizeOfOnFree(p);
    free(p);
}

XPCJSRuntime::XPCJSRuntime(nsXPConnect* aXPConnect)
   : CycleCollectedJSRuntime(32L * 1024L * 1024L, JS_USE_HELPER_THREADS, true),
   mJSContextStack(new XPCJSContextStack()),
   mCallContext(nullptr),
   mAutoRoots(nullptr),
   mResolveName(JSID_VOID),
   mResolvingWrapper(nullptr),
   mWrappedJSMap(JSObject2WrappedJSMap::newMap(XPC_JS_MAP_SIZE)),
   mWrappedJSClassMap(IID2WrappedJSClassMap::newMap(XPC_JS_CLASS_MAP_SIZE)),
   mIID2NativeInterfaceMap(IID2NativeInterfaceMap::newMap(XPC_NATIVE_INTERFACE_MAP_SIZE)),
   mClassInfo2NativeSetMap(ClassInfo2NativeSetMap::newMap(XPC_NATIVE_SET_MAP_SIZE)),
   mNativeSetMap(NativeSetMap::newMap(XPC_NATIVE_SET_MAP_SIZE)),
   mThisTranslatorMap(IID2ThisTranslatorMap::newMap(XPC_THIS_TRANSLATOR_MAP_SIZE)),
   mNativeScriptableSharedMap(XPCNativeScriptableSharedMap::newMap(XPC_NATIVE_JSCLASS_MAP_SIZE)),
   mDyingWrappedNativeProtoMap(XPCWrappedNativeProtoMap::newMap(XPC_DYING_NATIVE_PROTO_MAP_SIZE)),
   mDetachedWrappedNativeProtoMap(XPCWrappedNativeProtoMap::newMap(XPC_DETACHED_NATIVE_PROTO_MAP_SIZE)),
   mMapLock(XPCAutoLock::NewLock("XPCJSRuntime::mMapLock")),
   mThreadRunningGC(nullptr),
   mWrappedJSToReleaseArray(),
   mNativesToReleaseArray(),
   mDoingFinalization(false),
   mVariantRoots(nullptr),
   mWrappedJSRoots(nullptr),
   mObjectHolderRoots(nullptr),
   mWatchdogManager(new WatchdogManager(this)),
   mJunkScope(nullptr),
   mExceptionManagerNotAvailable(false)
{
#ifdef XPC_CHECK_WRAPPERS_AT_SHUTDOWN
    DEBUG_WrappedNativeHashtable =
        PL_NewDHashTable(PL_DHashGetStubOps(), nullptr,
                         sizeof(PLDHashEntryStub), 128);
#endif

    DOM_InitInterfaces();

    
    mStrIDs[0] = JSID_VOID;

    MOZ_ASSERT(Runtime());
    JSRuntime* runtime = Runtime();

    
    
    
    
    
    
    JS_SetGCParameter(runtime, JSGC_MAX_BYTES, 0xffffffff);
#if defined(MOZ_ASAN) || (defined(DEBUG) && !defined(XP_WIN))
    
    
    
    JS_SetNativeStackQuota(runtime, 2 * 128 * sizeof(size_t) * 1024);
#elif defined(XP_WIN)
    
    JS_SetNativeStackQuota(runtime, 900 * 1024);
#elif defined(XP_MACOSX) || defined(DARWIN)
    
    JS_SetNativeStackQuota(runtime, 7 * 1024 * 1024);
#else
    JS_SetNativeStackQuota(runtime, 128 * sizeof(size_t) * 1024);
#endif
    JS_SetContextCallback(runtime, ContextCallback);
    JS_SetDestroyCompartmentCallback(runtime, CompartmentDestroyedCallback);
    JS_SetCompartmentNameCallback(runtime, CompartmentNameCallback);
    mPrevGCSliceCallback = JS::SetGCSliceCallback(runtime, GCSliceCallback);
    JS_SetFinalizeCallback(runtime, FinalizeCallback);
    JS_SetWrapObjectCallbacks(runtime,
                              xpc::WrapperFactory::Rewrap,
                              xpc::WrapperFactory::WrapForSameCompartment,
                              xpc::WrapperFactory::PrepareForWrapping);
    js::SetPreserveWrapperCallback(runtime, PreserveWrapper);
#ifdef MOZ_CRASHREPORTER
    JS_EnumerateDiagnosticMemoryRegions(DiagnosticMemoryCallback);
#endif
#ifdef MOZ_ENABLE_PROFILER_SPS
    if (PseudoStack *stack = mozilla_get_pseudo_stack())
        stack->sampleRuntime(runtime);
#endif
    JS_SetAccumulateTelemetryCallback(runtime, AccumulateTelemetryCallback);
    js::SetActivityCallback(runtime, ActivityCallback, this);
    js::SetCTypesActivityCallback(runtime, CTypesActivityCallback);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    JS_SetSourceHook(runtime, SourceHook);

    if (!JS_SetICUMemoryFunctions(ICUAlloc, ICURealloc, ICUFree))
        NS_RUNTIMEABORT("JS_SetICUMemoryFunctions failed.");

    
    
    
    if (!xpc_LocalizeRuntime(runtime))
        NS_RUNTIMEABORT("xpc_LocalizeRuntime failed.");

    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(XPConnectJSGCHeap));
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(XPConnectJSSystemCompartmentCount));
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(XPConnectJSUserCompartmentCount));
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(JSMainRuntimeTemporaryPeak));
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(ICU));
    NS_RegisterMemoryMultiReporter(new JSCompartmentsMultiReporter);

    
#ifdef DEBUG
    if (!JS_GetGlobalDebugHooks(runtime)->debuggerHandler)
        xpc_InstallJSDebuggerKeywordHandler(runtime);
#endif
}


XPCJSRuntime*
XPCJSRuntime::newXPCJSRuntime(nsXPConnect* aXPConnect)
{
    NS_PRECONDITION(aXPConnect,"bad param");

    XPCJSRuntime* self = new XPCJSRuntime(aXPConnect);

    if (self                                    &&
        self->Runtime()                    &&
        self->GetWrappedJSMap()                 &&
        self->GetWrappedJSClassMap()            &&
        self->GetIID2NativeInterfaceMap()       &&
        self->GetClassInfo2NativeSetMap()       &&
        self->GetNativeSetMap()                 &&
        self->GetThisTranslatorMap()            &&
        self->GetNativeScriptableSharedMap()    &&
        self->GetDyingWrappedNativeProtoMap()   &&
        self->GetMapLock()                      &&
        self->mWatchdogManager) {
        return self;
    }

    NS_RUNTIMEABORT("new XPCJSRuntime failed to initialize.");

    delete self;
    return nullptr;
}


bool InternStaticDictionaryJSVals(JSContext* aCx);

JSBool
XPCJSRuntime::OnJSContextNew(JSContext *cx)
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

        if (!mozilla::dom::DefineStaticJSVals(cx) ||
            !InternStaticDictionaryJSVals(cx)) {
            return false;
        }
    }

    XPCContext* xpc = new XPCContext(this, cx);
    if (!xpc)
        return false;

    
    JS_ToggleOptions(cx, JSOPTION_UNROOTED_GLOBAL);

    return true;
}

bool
XPCJSRuntime::DescribeCustomObjects(JSObject* obj, js::Class* clasp,
                                    char (&name)[72]) const
{
    XPCNativeScriptableInfo *si = nullptr;

    if (!IS_PROTO_CLASS(clasp)) {
        return false;
    }

    XPCWrappedNativeProto *p =
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
XPCJSRuntime::NoteCustomGCThingXPCOMChildren(js::Class* clasp, JSObject* obj,
                                             nsCycleCollectionTraversalCallback& cb) const
{
    if (clasp != &XPC_WN_Tearoff_JSClass) {
        return false;
    }

    
    
    
    XPCWrappedNativeTearOff *to =
        static_cast<XPCWrappedNativeTearOff*>(xpc_GetJSPrivate(obj));
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "xpc_GetJSPrivate(obj)->mNative");
    cb.NoteXPCOMChild(to->GetNative());
    return true;
}



#ifdef DEBUG
static PLDHashOperator
WrappedJSClassMapDumpEnumerator(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                uint32_t number, void *arg)
{
    ((IID2WrappedJSClassMap::Entry*)hdr)->value->DebugDump(*(int16_t*)arg);
    return PL_DHASH_NEXT;
}
static PLDHashOperator
NativeSetDumpEnumerator(PLDHashTable *table, PLDHashEntryHdr *hdr,
                        uint32_t number, void *arg)
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
        XPC_LOG_ALWAYS(("mMapLock @ %x", mMapLock));

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
            XPCContext *xpc = XPCContext::GetXPCContext(iter);
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
XPCRootSetElem::AddToRootSet(XPCLock *lock, XPCRootSetElem **listHead)
{
    NS_ASSERTION(!mSelfp, "Must be not linked");

    XPCAutoLock autoLock(lock);

    mSelfp = listHead;
    mNext = *listHead;
    if (mNext) {
        NS_ASSERTION(mNext->mSelfp == listHead, "Must be list start");
        mNext->mSelfp = &mNext;
    }
    *listHead = this;
}

void
XPCRootSetElem::RemoveFromRootSet(XPCLock *lock)
{
    nsXPConnect *xpc = nsXPConnect::XPConnect();
    JS::PokeGC(xpc->GetRuntime()->Runtime());

    NS_ASSERTION(mSelfp, "Must be linked");

    XPCAutoLock autoLock(lock);

    NS_ASSERTION(*mSelfp == this, "Link invariant");
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
    NS_ASSERTION(cb, "null callback");
    extraGCCallbacks.AppendElement(cb);
}

void
XPCJSRuntime::RemoveGCCallback(xpcGCCallback cb)
{
    NS_ASSERTION(cb, "null callback");
    bool found = extraGCCallbacks.RemoveElement(cb);
    if (!found) {
        NS_ERROR("Removing a callback which was never added.");
    }
}

JSObject *
XPCJSRuntime::GetJunkScope()
{
    if (!mJunkScope) {
        AutoSafeJSContext cx;
        SandboxOptions options(cx);
        options.sandboxName.AssignASCII("XPConnect Junk Compartment");
        RootedValue v(cx);
        nsresult rv = xpc_CreateSandboxObject(cx, v.address(),
                                              nsContentUtils::GetSystemPrincipal(),
                                              options);

        NS_ENSURE_SUCCESS(rv, nullptr);

        mJunkScope = js::UncheckedUnwrap(&v.toObject());
        JS_AddNamedObjectRoot(cx, &mJunkScope, "XPConnect Junk Compartment");
    }
    return mJunkScope;
}

void
XPCJSRuntime::DeleteJunkScope()
{
    if(!mJunkScope)
        return;

    AutoSafeJSContext cx;
    JS_RemoveObjectRoot(cx, &mJunkScope);
    mJunkScope = nullptr;
}
