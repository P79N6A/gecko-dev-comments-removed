







#include "mozilla/Util.h"

#include "xpcprivate.h"
#include "xpcpublic.h"
#include "XPCJSMemoryReporter.h"
#include "WrapperFactory.h"
#include "dom_quickstubs.h"

#include "nsIMemoryReporter.h"
#include "nsPIDOMWindow.h"
#include "nsPrintfCString.h"
#include "prsystem.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"

#include "nsLayoutStatics.h"
#include "nsContentUtils.h"
#include "nsCCUncollectableMarker.h"
#include "nsScriptLoader.h"
#include "jsfriendapi.h"
#include "js/MemoryMetrics.h"
#include "mozilla/dom/DOMJSClass.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Element.h"
#include "mozilla/Attributes.h"
#include "AccessCheck.h"

#include "sampler.h"
#include "nsJSPrincipals.h"
#include <algorithm>

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

using namespace mozilla;
using namespace xpc;



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
    "baseURIObject",        
    "nodePrincipal",        
    "documentURIObject",    
    "mozMatchesSelector"    
};



struct CX_AND_XPCRT_Data
{
    JSContext* cx;
    XPCJSRuntime* rt;
};

static void * const UNMARK_ONLY = nullptr;
static void * const UNMARK_AND_SWEEP = (void *)1;

static JSDHashOperator
NativeInterfaceSweeper(JSDHashTable *table, JSDHashEntryHdr *hdr,
                       uint32_t number, void *arg)
{
    XPCNativeInterface* iface = ((IID2NativeInterfaceMap::Entry*)hdr)->value;
    if (iface->IsMarked()) {
        iface->Unmark();
        return JS_DHASH_NEXT;
    }

    if (arg == UNMARK_ONLY)
        return JS_DHASH_NEXT;

#ifdef XPC_REPORT_NATIVE_INTERFACE_AND_SET_FLUSHING
    fputs("- Destroying XPCNativeInterface for ", stdout);
    JS_PutString(JSVAL_TO_STRING(iface->GetName()), stdout);
    putc('\n', stdout);
#endif

    XPCNativeInterface::DestroyInstance(iface);
    return JS_DHASH_REMOVE;
}






static JSDHashOperator
NativeUnMarkedSetRemover(JSDHashTable *table, JSDHashEntryHdr *hdr,
                         uint32_t number, void *arg)
{
    XPCNativeSet* set = ((ClassInfo2NativeSetMap::Entry*)hdr)->value;
    if (set->IsMarked())
        return JS_DHASH_NEXT;
    return JS_DHASH_REMOVE;
}

static JSDHashOperator
NativeSetSweeper(JSDHashTable *table, JSDHashEntryHdr *hdr,
                 uint32_t number, void *arg)
{
    XPCNativeSet* set = ((NativeSetMap::Entry*)hdr)->key_value;
    if (set->IsMarked()) {
        set->Unmark();
        return JS_DHASH_NEXT;
    }

    if (arg == UNMARK_ONLY)
        return JS_DHASH_NEXT;

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
    return JS_DHASH_REMOVE;
}

static JSDHashOperator
JSClassSweeper(JSDHashTable *table, JSDHashEntryHdr *hdr,
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
        return JS_DHASH_NEXT;
    }

    if (arg == UNMARK_ONLY)
        return JS_DHASH_NEXT;

#ifdef XPC_REPORT_JSCLASS_FLUSHING
    printf("- Destroying XPCNativeScriptableShared for: %s @ %x\n",
           shared->GetJSClass()->name,
           shared->GetJSClass());
#endif

    delete shared;
    return JS_DHASH_REMOVE;
}

static JSDHashOperator
DyingProtoKiller(JSDHashTable *table, JSDHashEntryHdr *hdr,
                 uint32_t number, void *arg)
{
    XPCWrappedNativeProto* proto =
        (XPCWrappedNativeProto*)((JSDHashEntryStub*)hdr)->key;
    delete proto;
    return JS_DHASH_REMOVE;
}

static JSDHashOperator
DetachedWrappedNativeProtoMarker(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                 uint32_t number, void *arg)
{
    XPCWrappedNativeProto* proto =
        (XPCWrappedNativeProto*)((JSDHashEntryStub*)hdr)->key;

    proto->Mark();
    return JS_DHASH_NEXT;
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

    
    
    return js::RecomputeWrappers(cx, js::SingleCompartment(compartment),
                                 js::AllCompartments());
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

void
XPCJSRuntime::AddJSHolder(void* aHolder, nsScriptObjectTracer* aTracer)
{
    MOZ_ASSERT(aTracer->Trace, "AddJSHolder needs a non-null Trace function");
    bool wasEmpty = mJSHolders.Count() == 0;
    mJSHolders.Put(aHolder, aTracer);
    if (wasEmpty && mJSHolders.Count() == 1) {
      nsLayoutStatics::AddRef();
    }
}

#ifdef DEBUG
static void
AssertNoGcThing(void* aGCThing, const char* aName, void* aClosure)
{
    MOZ_ASSERT(!aGCThing);
}

void
XPCJSRuntime::AssertNoObjectsToTrace(void* aPossibleJSHolder)
{
    nsScriptObjectTracer* tracer = mJSHolders.Get(aPossibleJSHolder);
    if (tracer && tracer->Trace) {
        tracer->Trace(aPossibleJSHolder, AssertNoGcThing, nullptr);
    }
}
#endif

void
XPCJSRuntime::RemoveJSHolder(void* aHolder)
{
#ifdef DEBUG
    
    
    
    
    if (aHolder != mObjectToUnlink) {
        AssertNoObjectsToTrace(aHolder);
    }
#endif
    bool hadOne = mJSHolders.Count() == 1;
    mJSHolders.Remove(aHolder);
    if (hadOne && mJSHolders.Count() == 0) {
      nsLayoutStatics::Release();
    }
}

bool
XPCJSRuntime::TestJSHolder(void* aHolder)
{
    return mJSHolders.Get(aHolder, nullptr);
}


void XPCJSRuntime::TraceBlackJS(JSTracer* trc, void* data)
{
    XPCJSRuntime* self = (XPCJSRuntime*)data;

    
    
    if (!self->GetXPConnect()->IsShuttingDown()) {
        
        if (AutoMarkingPtr *roots = Get()->mAutoRoots)
            roots->TraceJSAll(trc);
    }

    {
        XPCAutoLock lock(self->mMapLock);

        
        
        XPCRootSetElem *e;
        for (e = self->mObjectHolderRoots; e; e = e->GetNextRoot())
            static_cast<XPCJSObjectHolder*>(e)->TraceJS(trc);
    }

    dom::TraceBlackJS(trc, JS_GetGCParameter(self->GetJSRuntime(), JSGC_NUMBER));
}


void XPCJSRuntime::TraceGrayJS(JSTracer* trc, void* data)
{
    XPCJSRuntime* self = (XPCJSRuntime*)data;

    
    self->TraceXPConnectRoots(trc);
}

static void
TraceJSObject(void *aScriptThing, const char *name, void *aClosure)
{
    JS_CALL_TRACER(static_cast<JSTracer*>(aClosure), aScriptThing,
                   js::GCThingTraceKind(aScriptThing), name);
}

static PLDHashOperator
TraceJSHolder(void *holder, nsScriptObjectTracer *&tracer, void *arg)
{
    tracer->Trace(holder, TraceJSObject, arg);

    return PL_DHASH_NEXT;
}

void XPCJSRuntime::TraceXPConnectRoots(JSTracer *trc)
{
    JSContext *iter = nullptr;
    while (JSContext *acx = JS_ContextIterator(GetJSRuntime(), &iter)) {
        MOZ_ASSERT(js::HasUnrootedGlobal(acx));
        if (JSObject *global = JS_GetGlobalObject(acx))
            JS_CALL_OBJECT_TRACER(trc, global, "XPC global object");
    }

    XPCAutoLock lock(mMapLock);

    XPCWrappedNativeScope::TraceWrappedNativesInAllScopes(trc, this);

    for (XPCRootSetElem *e = mVariantRoots; e ; e = e->GetNextRoot())
        static_cast<XPCTraceableVariant*>(e)->TraceJS(trc);

    for (XPCRootSetElem *e = mWrappedJSRoots; e ; e = e->GetNextRoot())
        static_cast<nsXPCWrappedJS*>(e)->TraceJS(trc);

    mJSHolders.Enumerate(TraceJSHolder, trc);
}

struct Closure
{
    bool cycleCollectionEnabled;
    nsCycleCollectionTraversalCallback *cb;
};

static void
CheckParticipatesInCycleCollection(void *aThing, const char *name, void *aClosure)
{
    Closure *closure = static_cast<Closure*>(aClosure);

    if (closure->cycleCollectionEnabled)
        return;

    if (AddToCCKind(js::GCThingTraceKind(aThing)) &&
        xpc_IsGrayGCThing(aThing))
    {
        closure->cycleCollectionEnabled = true;
    }
}

static PLDHashOperator
NoteJSHolder(void *holder, nsScriptObjectTracer *&tracer, void *arg)
{
    Closure *closure = static_cast<Closure*>(arg);

    closure->cycleCollectionEnabled = false;
    tracer->Trace(holder, CheckParticipatesInCycleCollection,
                  closure);
    if (closure->cycleCollectionEnabled)
        closure->cb->NoteNativeRoot(holder, tracer);

    return PL_DHASH_NEXT;
}


void
XPCJSRuntime::SuspectWrappedNative(XPCWrappedNative *wrapper,
                                   nsCycleCollectionTraversalCallback &cb)
{
    if (!wrapper->IsValid() || wrapper->IsWrapperExpired())
        return;

    NS_ASSERTION(NS_IsMainThread() || NS_IsCycleCollectorThread(),
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
XPCJSRuntime::AddXPConnectRoots(nsCycleCollectionTraversalCallback &cb)
{
    
    
    
    
    
    
    

    JSContext *iter = nullptr, *acx;
    while ((acx = JS_ContextIterator(GetJSRuntime(), &iter))) {
        
        
        JSObject* global = JS_GetGlobalObject(acx);
        if (global && xpc_IsGrayGCThing(global)) {
            cb.NoteNativeRoot(acx, nsXPConnect::JSContextParticipant());
        }
    }

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

    Closure closure = { true, &cb };
    mJSHolders.Enumerate(NoteJSHolder, &closure);
}

static PLDHashOperator
UnmarkJSHolder(void *holder, nsScriptObjectTracer *&tracer, void *arg)
{
    tracer->CanSkip(holder, true);
    return PL_DHASH_NEXT;
}

void
XPCJSRuntime::UnmarkSkippableJSHolders()
{
    XPCAutoLock lock(mMapLock);
    mJSHolders.Enumerate(UnmarkJSHolder, nullptr);
}

void
xpc_UnmarkSkippableJSHolders()
{
    if (nsXPConnect::GetXPConnect() &&
        nsXPConnect::GetXPConnect()->GetRuntime()) {
        nsXPConnect::GetXPConnect()->GetRuntime()->UnmarkSkippableJSHolders();
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

struct DeferredFinalizeFunction
{
    XPCJSRuntime::DeferredFinalizeFunction run;
    void *data;
};

class XPCIncrementalReleaseRunnable : public nsRunnable
{
    XPCJSRuntime *runtime;
    nsTArray<nsISupports *> items;
    nsAutoTArray<DeferredFinalizeFunction, 16> deferredFinalizeFunctions;
    uint32_t finalizeFunctionToRun;

    static const PRTime SliceMillis = 10; 

  public:
    XPCIncrementalReleaseRunnable(XPCJSRuntime *rt, nsTArray<nsISupports *> &items);
    virtual ~XPCIncrementalReleaseRunnable();

    void ReleaseNow(bool limited);

    NS_DECL_NSIRUNNABLE
};

bool
ReleaseSliceNow(uint32_t slice, void *data)
{
    MOZ_ASSERT(slice > 0, "nonsensical/useless call with slice == 0");
    nsTArray<nsISupports *> *items = static_cast<nsTArray<nsISupports *>*>(data);

    slice = std::min(slice, items->Length());
    for (uint32_t i = 0; i < slice; ++i) {
        
        uint32_t lastItemIdx = items->Length() - 1;

        nsISupports *wrapper = items->ElementAt(lastItemIdx);
        items->RemoveElementAt(lastItemIdx);
        NS_RELEASE(wrapper);
    }

    return items->IsEmpty();
}


XPCIncrementalReleaseRunnable::XPCIncrementalReleaseRunnable(XPCJSRuntime *rt,
                                                             nsTArray<nsISupports *> &items)
  : runtime(rt),
    finalizeFunctionToRun(0)
{
    nsLayoutStatics::AddRef();
    this->items.SwapElements(items);
    DeferredFinalizeFunction *function = deferredFinalizeFunctions.AppendElement();
    function->run = ReleaseSliceNow;
    function->data = &this->items;
    for (uint32_t i = 0; i < rt->mDeferredFinalizeFunctions.Length(); ++i) {
        void *data = (rt->mDeferredFinalizeFunctions[i].start)();
        if (data) {
            function = deferredFinalizeFunctions.AppendElement();
            function->run = rt->mDeferredFinalizeFunctions[i].run;
            function->data = data;
        }
    }
}

XPCIncrementalReleaseRunnable::~XPCIncrementalReleaseRunnable()
{
    MOZ_ASSERT(this != runtime->mReleaseRunnable);
    nsLayoutStatics::Release();
}

void
XPCIncrementalReleaseRunnable::ReleaseNow(bool limited)
{
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(deferredFinalizeFunctions.Length() != 0,
               "We should have at least ReleaseSliceNow to run");
    MOZ_ASSERT(finalizeFunctionToRun < deferredFinalizeFunctions.Length(),
               "No more finalizers to run?");

    TimeDuration sliceTime = TimeDuration::FromMilliseconds(SliceMillis);
    TimeStamp started = TimeStamp::Now();
    bool timeout = false;
    do {
        const DeferredFinalizeFunction &function =
            deferredFinalizeFunctions[finalizeFunctionToRun];
        if (limited) {
            bool done = false;
            while (!timeout && !done) {
                



                done = function.run(100, function.data);
                timeout = TimeStamp::Now() - started >= sliceTime;
            }
            if (done)
                ++finalizeFunctionToRun;
            if (timeout)
                break;
        } else {
            function.run(UINT32_MAX, function.data);
            MOZ_ASSERT(!items.Length());
            ++finalizeFunctionToRun;
        }
    } while (finalizeFunctionToRun < deferredFinalizeFunctions.Length());

    if (finalizeFunctionToRun == deferredFinalizeFunctions.Length()) {
        MOZ_ASSERT(runtime->mReleaseRunnable == this);
        runtime->mReleaseRunnable = nullptr;
    }
}

NS_IMETHODIMP
XPCIncrementalReleaseRunnable::Run()
{
    if (runtime->mReleaseRunnable != this) {
        
        MOZ_ASSERT(!items.Length());
        return NS_OK;
    }

    ReleaseNow(true);

    if (items.Length()) {
        nsresult rv = NS_DispatchToMainThread(this);
        if (NS_FAILED(rv))
            ReleaseNow(false);
    }

    return NS_OK;
}

void
XPCJSRuntime::ReleaseIncrementally(nsTArray<nsISupports *> &array)
{
    MOZ_ASSERT(!mReleaseRunnable);
    mReleaseRunnable = new XPCIncrementalReleaseRunnable(this, array);

    nsresult rv = NS_DispatchToMainThread(mReleaseRunnable);
    if (NS_FAILED(rv))
        mReleaseRunnable->ReleaseNow(false);
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
XPCJSRuntime::GCCallback(JSRuntime *rt, JSGCStatus status)
{
    XPCJSRuntime* self = nsXPConnect::GetRuntimeInstance();
    if (!self)
        return;

    switch (status) {
        case JSGC_BEGIN:
        {
            
            
            JSContext *iter = nullptr;
            while (JSContext *acx = JS_ContextIterator(rt, &iter)) {
                if (!js::HasUnrootedGlobal(acx))
                    JS_ToggleOptions(acx, JSOPTION_UNROOTED_GLOBAL);
            }
            break;
        }
        case JSGC_END:
        {
            






            if (self->mReleaseRunnable)
                self->mReleaseRunnable->ReleaseNow(false);

            
            if (JS::WasIncrementalGC(rt)) {
                self->ReleaseIncrementally(self->mNativesToReleaseArray);
            } else {
                DoDeferredRelease(self->mNativesToReleaseArray);
                for (uint32_t i = 0; i < self->mDeferredFinalizeFunctions.Length(); ++i) {
                    if (void *data = self->mDeferredFinalizeFunctions[i].start())
                        self->mDeferredFinalizeFunctions[i].run(UINT32_MAX, data);
                }
            }
            break;
        }
    }

    nsTArray<JSGCCallback> callbacks(self->extraGCCallbacks);
    for (uint32_t i = 0; i < callbacks.Length(); ++i)
        callbacks[i](rt, status);
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

            
            
            
            
            
            

            
            
            if (!self->GetXPConnect()->IsShuttingDown()) {

                
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

            
            
            
            if (!self->GetXPConnect()->IsShuttingDown()) {
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

            
            
            
            
            
            
            
            
            
            
            

            
            
            if (!self->GetXPConnect()->IsShuttingDown()) {
                

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

class AutoLockWatchdog {
    XPCJSRuntime* const mRuntime;

  public:
    AutoLockWatchdog(XPCJSRuntime* aRuntime)
      : mRuntime(aRuntime) {
        PR_Lock(mRuntime->mWatchdogLock);
    }

    ~AutoLockWatchdog() {
        PR_Unlock(mRuntime->mWatchdogLock);
    }
};


void
XPCJSRuntime::WatchdogMain(void *arg)
{
    PR_SetCurrentThreadName("JS Watchdog");

    XPCJSRuntime* self = static_cast<XPCJSRuntime*>(arg);

    
    AutoLockWatchdog lock(self);

    PRIntervalTime sleepInterval;
    while (self->mWatchdogThread) {
        
        if (self->mLastActiveTime == -1 || PR_Now() - self->mLastActiveTime <= PRTime(2*PR_USEC_PER_SEC))
            sleepInterval = PR_TicksPerSecond();
        else {
            sleepInterval = PR_INTERVAL_NO_TIMEOUT;
            self->mWatchdogHibernating = true;
        }
        MOZ_ALWAYS_TRUE(PR_WaitCondVar(self->mWatchdogWakeup, sleepInterval) == PR_SUCCESS);
        JS_TriggerOperationCallback(self->mJSRuntime);
    }

    
    PR_NotifyCondVar(self->mWatchdogWakeup);
}


void
XPCJSRuntime::ActivityCallback(void *arg, JSBool active)
{
    XPCJSRuntime* self = static_cast<XPCJSRuntime*>(arg);

    AutoLockWatchdog lock(self);
    
    if (active) {
        self->mLastActiveTime = -1;
        if (self->mWatchdogHibernating) {
            self->mWatchdogHibernating = false;
            PR_NotifyCondVar(self->mWatchdogWakeup);
        }
    } else {
        self->mLastActiveTime = PR_Now();
    }
}






void
XPCJSRuntime::CTypesActivityCallback(JSContext *cx, js::CTypesActivityType type)
{
  if (type == js::CTYPES_CALLBACK_BEGIN) {
    if (!Get()->GetJSContextStack()->Push(cx))
      MOZ_CRASH();
  } else if (type == js::CTYPES_CALLBACK_END) {
    Get()->GetJSContextStack()->Pop();
  }
}

size_t
XPCJSRuntime::SizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    n += mWrappedJSMap->SizeOfIncludingThis(mallocSizeOf);
    n += mIID2NativeInterfaceMap->SizeOfIncludingThis(mallocSizeOf);
    n += mClassInfo2NativeSetMap->ShallowSizeOfIncludingThis(mallocSizeOf);
    n += mNativeSetMap->SizeOfIncludingThis(mallocSizeOf);

    
    
    n += mJSHolders.SizeOfExcludingThis(nullptr, mallocSizeOf);

    
    
    

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
static JSDHashOperator
DEBUG_WrapperChecker(JSDHashTable *table, JSDHashEntryHdr *hdr,
                     uint32_t number, void *arg)
{
    XPCWrappedNative* wrapper = (XPCWrappedNative*)((JSDHashEntryStub*)hdr)->key;
    NS_ASSERTION(!wrapper->IsValid(), "found a 'valid' wrapper!");
    ++ *((int*)arg);
    return JS_DHASH_NEXT;
}
#endif

static JSDHashOperator
DetachedWrappedNativeProtoShutdownMarker(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                         uint32_t number, void *arg)
{
    XPCWrappedNativeProto* proto =
        (XPCWrappedNativeProto*)((JSDHashEntryStub*)hdr)->key;

    proto->SystemIsBeingShutDown();
    return JS_DHASH_NEXT;
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
    MOZ_ASSERT(!mReleaseRunnable);

    JS::SetGCSliceCallback(mJSRuntime, mPrevGCSliceCallback);

    xpc_DelocalizeRuntime(mJSRuntime);

    if (mWatchdogWakeup) {
        
        
        
        
        {
            AutoLockWatchdog lock(this);
            if (mWatchdogThread) {
                mWatchdogThread = nullptr;
                PR_NotifyCondVar(mWatchdogWakeup);
                PR_WaitCondVar(mWatchdogWakeup, PR_INTERVAL_NO_TIMEOUT);
            }
        }
        PR_DestroyCondVar(mWatchdogWakeup);
        PR_DestroyLock(mWatchdogLock);
        mWatchdogWakeup = nullptr;
    }

    if (mCallContext)
        mCallContext->SystemIsBeingShutDown();

#ifdef XPC_DUMP_AT_SHUTDOWN
    {
    
    JSContext* iter = nullptr;
    int count = 0;
    while (JS_ContextIterator(mJSRuntime, &iter))
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
        mWrappedJSMap->ShutdownMarker(mJSRuntime);
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
        JS_DHashTableEnumerate(DEBUG_WrappedNativeHashtable,
                               DEBUG_WrapperChecker, &LiveWrapperCount);
        if (LiveWrapperCount)
            printf("deleting XPCJSRuntime with %d live XPCWrappedNative (found in wrapper check)\n", (int)LiveWrapperCount);
        JS_DHashTableDestroy(DEBUG_WrappedNativeHashtable);
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

    if (mJSRuntime) {
        JS_DestroyRuntime(mJSRuntime);
        JS_ShutDown();
#ifdef DEBUG_shaver_off
        fprintf(stderr, "nJRSI: destroyed runtime %p\n", (void *)mJSRuntime);
#endif
    }
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
    JSRuntime *rt = nsXPConnect::GetRuntimeInstance()->GetJSRuntime();
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
    return JS::SystemCompartmentCount(nsXPConnect::GetRuntimeInstance()->GetJSRuntime());
}

static int64_t
GetJSUserCompartmentCount()
{
    return JS::UserCompartmentCount(nsXPConnect::GetRuntimeInstance()->GetJSRuntime());
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
    return JS::PeakSizeOfTemporary(nsXPConnect::GetRuntimeInstance()->GetJSRuntime());
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
                const nsACString &pathPrefix,
                nsIMemoryMultiReporterCallback *cb,
                nsISupports *closure, size_t *gcTotalOut = NULL)
{
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

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap/type-objects"),
                      zStats.gcHeapTypeObjects,
                      "Memory on the garbage-collected JavaScript "
                      "heap that holds type inference information.");

    ZCREPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap/ion-codes"),
                      zStats.gcHeapIonCodes,
                      "Memory on the garbage-collected JavaScript "
                      "heap that holds references to executable code pools "
                      "used by IonMonkey.");

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
                       const nsACString &cJSPathPrefix,
                       const nsACString &cDOMPathPrefix,
                       nsIMemoryMultiReporterCallback *cb,
                       nsISupports *closure, size_t *gcTotalOut = NULL)
{
    size_t gcTotal = 0, gcHeapSundries = 0, otherSundries = 0;

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

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("objects-extra/elements"),
                   cStats.objectsExtra.elements,
                   "Memory allocated for object element "
                   "arrays, which are used to represent indexed object "
                   "properties.");

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

    ZCREPORT_BYTES(cJSPathPrefix + NS_LITERAL_CSTRING("jaeger-data"),
                   cStats.jaegerData,
                   "Memory used by the JaegerMonkey JIT for compilation data: "
                   "JITScripts, native maps, and inline cache structs.");

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

nsresult
ReportJSRuntimeExplicitTreeStats(const JS::RuntimeStats &rtStats,
                                 const nsACString &rtPath,
                                 nsIMemoryMultiReporterCallback *cb,
                                 nsISupports *closure, size_t *rtTotalOut)
{
    nsresult rv;

    size_t gcTotal = 0;

    for (size_t i = 0; i < rtStats.zoneStatsVector.length(); i++) {
        JS::ZoneStats zStats = rtStats.zoneStatsVector[i];
        nsCString path(static_cast<char *>(zStats.extra1));

        rv = ReportZoneStats(zStats, path, cb, closure, &gcTotal);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    for (size_t i = 0; i < rtStats.compartmentStatsVector.length(); i++) {
        JS::CompartmentStats cStats = rtStats.compartmentStatsVector[i];
        nsCString cJSPathPrefix(static_cast<char *>(cStats.extra1));
        nsCString cDOMPathPrefix(static_cast<char *>(cStats.extra2));

        rv = ReportCompartmentStats(cStats, cJSPathPrefix, cDOMPathPrefix, cb, closure, &gcTotal);
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

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/code/jaeger"),
                  nsIMemoryReporter::KIND_NONHEAP, rtStats.runtime.code.jaeger,
                  "Memory used by the JaegerMonkey JIT to hold generated code.");

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

    RREPORT_BYTES(rtPath + NS_LITERAL_CSTRING("runtime/stack"),
                  nsIMemoryReporter::KIND_NONHEAP, rtStats.runtime.stack,
                  "Memory used for the JS call stack.  This is the committed "
                  "portion of the stack on Windows; on *nix, it is the resident "
                  "portion of the stack.  Therefore, on *nix, if part of the "
                  "stack is swapped out to disk, we do not count it here.\n\n"
                  "Note that debug builds usually have stack poisoning enabled, "
                  "which causes the whole stack to be committed (and likely "
                  "resident).");

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

    

    REPORT_GC_BYTES(rtPath + NS_LITERAL_CSTRING("gc-heap/unused-arenas"),
                    rtStats.gcHeapUnusedArenas,
                    "Memory on the garbage-collected JavaScript heap taken by "
                    "empty arenas within non-empty chunks.");

    REPORT_GC_BYTES(rtPath + NS_LITERAL_CSTRING("gc-heap/unused-chunks"),
                    rtStats.gcHeapUnusedChunks,
                    "Memory on the garbage-collected JavaScript heap taken by "
                    "empty chunks, which will soon be released unless claimed "
                    "for new allocations.");

    REPORT_GC_BYTES(rtPath + NS_LITERAL_CSTRING("gc-heap/decommitted-arenas"),
                    rtStats.gcHeapDecommittedArenas,
                    "Memory on the garbage-collected JavaScript heap, "
                    "in arenas in non-empty chunks, that is returned to the OS. "
                    "This means it takes up address space but no physical "
                    "memory or swap space.");

    REPORT_GC_BYTES(rtPath + NS_LITERAL_CSTRING("gc-heap/chunk-admin"),
                    rtStats.gcHeapChunkAdmin,
                    "Memory on the garbage-collected JavaScript heap, within "
                    "chunks, that is used to hold internal bookkeeping "
                    "information.");

    
    
    MOZ_ASSERT(gcTotal == rtStats.gcHeapChunkTotal);

    return NS_OK;
}

} 

class JSCompartmentsMultiReporter MOZ_FINAL : public nsIMemoryMultiReporter
{
  public:
    NS_DECL_ISUPPORTS

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
        JS_IterateCompartments(nsXPConnect::GetRuntimeInstance()->GetJSRuntime(),
                               &paths, CompartmentCallback);

        
        for (size_t i = 0; i < paths.length(); i++)
            
            REPORT(nsCString(paths[i]),
                   nsIMemoryReporter::KIND_OTHER,
                   nsIMemoryReporter::UNITS_COUNT,
                   1, "");

        return NS_OK;
    }

    NS_IMETHOD
    GetExplicitNonHeap(int64_t *n)
    {
        
        *n = 0;
        return NS_OK;
    }
};

NS_IMPL_THREADSAFE_ISUPPORTS1(JSCompartmentsMultiReporter
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

  public:
    XPCJSRuntimeStats(WindowPaths *windowPaths, WindowPaths *topWindowPaths)
      : JS::RuntimeStats(JsMallocSizeOf), mWindowPaths(windowPaths), mTopWindowPaths(topWindowPaths)
    {}

    ~XPCJSRuntimeStats() {
        for (size_t i = 0; i != compartmentStatsVector.length(); ++i) {
            free(compartmentStatsVector[i].extra1);
            free(compartmentStatsVector[i].extra2);
        }

        for (size_t i = 0; i != zoneStatsVector.length(); ++i)
            free(zoneStatsVector[i].extra1);
    }

    virtual void initExtraZoneStats(JS::Zone *zone, JS::ZoneStats *zStats) MOZ_OVERRIDE {
        
        nsXPConnect *xpc = nsXPConnect::GetXPConnect();
        JSContext *cx = xpc->GetSafeJSContext();
        JSCompartment *comp = js::GetAnyCompartmentInZone(zone);
        nsCString pathPrefix("explicit/js-non-window/zones/");
        if (JSObject *global = JS_GetGlobalForCompartmentOrNull(cx, comp)) {
            
            
            JSAutoCompartment ac(cx, global);
            nsISupports *native = xpc->GetNativeOfWrapper(cx, global);
            if (nsCOMPtr<nsPIDOMWindow> piwindow = do_QueryInterface(native)) {
                
                
                if (mTopWindowPaths->Get(piwindow->WindowID(), &pathPrefix))
                    pathPrefix.AppendLiteral("/js-");
            }
        }

        pathPrefix += nsPrintfCString("zone(%p)/", (void *)zone);

        zStats->extra1 = strdup(pathPrefix.get());
    }

    virtual void initExtraCompartmentStats(JSCompartment *c,
                                           JS::CompartmentStats *cstats) MOZ_OVERRIDE
    {
        nsAutoCString cJSPathPrefix, cDOMPathPrefix;
        nsCString cName;
        GetCompartmentName(c, cName, true);

        
        nsXPConnect *xpc = nsXPConnect::GetXPConnect();
        JSContext *cx = xpc->GetSafeJSContext();
        bool needZone = true;
        if (JSObject *global = JS_GetGlobalForCompartmentOrNull(cx, c)) {
            
            
            JSAutoCompartment ac(cx, global);
            nsISupports *native = xpc->GetNativeOfWrapper(cx, global);
            if (nsCOMPtr<nsPIDOMWindow> piwindow = do_QueryInterface(native)) {
                
                
                if (mWindowPaths->Get(piwindow->WindowID(), &cJSPathPrefix)) {
                    cDOMPathPrefix.Assign(cJSPathPrefix);
                    cDOMPathPrefix.AppendLiteral("/dom/");
                    cJSPathPrefix.AppendLiteral("/js-");
                    needZone = false;
                } else {
                    cJSPathPrefix.AssignLiteral("explicit/js-non-window/zones/");
                    cDOMPathPrefix.AssignLiteral("explicit/dom/unknown-window-global?!/");
                }
            } else {
                cJSPathPrefix.AssignLiteral("explicit/js-non-window/zones/");
                cDOMPathPrefix.AssignLiteral("explicit/dom/non-window-global?!/");
            }
        } else {
            cJSPathPrefix.AssignLiteral("explicit/js-non-window/zones/");
            cDOMPathPrefix.AssignLiteral("explicit/dom/no-global?!/");
        }

        if (needZone)
            cJSPathPrefix += nsPrintfCString("zone(%p)/", (void *)js::GetCompartmentZone(c));

        cJSPathPrefix += NS_LITERAL_CSTRING("compartment(") + cName + NS_LITERAL_CSTRING(")/");

        
        
        
        
        
        
        
        
        
        

        cstats->extra1 = strdup(cJSPathPrefix.get());
        cstats->extra2 = strdup(cDOMPathPrefix.get());
    }
};

nsresult
JSMemoryMultiReporter::CollectReports(WindowPaths *windowPaths,
                                      WindowPaths *topWindowPaths,
                                      nsIMemoryMultiReporterCallback *cb,
                                      nsISupports *closure)
{
    XPCJSRuntime *xpcrt = nsXPConnect::GetRuntimeInstance();

    
    
    
    
    

    XPCJSRuntimeStats rtStats(windowPaths, topWindowPaths);
    OrphanReporter orphanReporter(XPCConvert::GetISupportsFromJSObject);
    if (!JS::CollectRuntimeStats(xpcrt->GetJSRuntime(), &rtStats, &orphanReporter))
        return NS_ERROR_FAILURE;

    size_t xpconnect =
        xpcrt->SizeOfIncludingThis(JsMallocSizeOf) +
        XPCWrappedNativeScope::SizeOfAllScopesIncludingThis(JsMallocSizeOf);

    
    

    nsresult rv;
    size_t rtTotal = 0;
    rv = xpc::ReportJSRuntimeExplicitTreeStats(rtStats,
                                               NS_LITERAL_CSTRING("explicit/js-non-window/"),
                                               cb, closure, &rtTotal);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = ReportCompartmentStats(rtStats.cTotals,
                                NS_LITERAL_CSTRING("js-main-runtime/compartments/"),
                                NS_LITERAL_CSTRING("window-objects/dom/"),
                                cb, closure);
    rv = ReportZoneStats(rtStats.zTotals,
                         NS_LITERAL_CSTRING("js-main-runtime/zones/"),
                         cb, closure);
    NS_ENSURE_SUCCESS(rv, rv);

    
    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime/runtime"),
                 nsIMemoryReporter::KIND_OTHER, rtTotal,
                 "The sum of all measurements under 'explicit/js-non-window/runtime/'.");

    

    REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime/gc-heap/decommitted-arenas"),
                 nsIMemoryReporter::KIND_OTHER,
                 rtStats.gcHeapDecommittedArenas,
                 "The same as 'explicit/js-non-window/gc-heap/decommitted-arenas'.");

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

nsresult
JSMemoryMultiReporter::GetExplicitNonHeap(int64_t *n)
{
    JSRuntime *rt = nsXPConnect::GetRuntimeInstance()->GetJSRuntime();
    *reinterpret_cast<int64_t*>(n) = JS::GetExplicitNonHeapForRuntime(rt, JsMallocSizeOf);
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

bool XPCJSRuntime::gXBLScopesEnabled;

static bool
PreserveWrapper(JSContext *cx, JSObject *obj)
{
    MOZ_ASSERT(cx);
    MOZ_ASSERT(obj);
    MOZ_ASSERT(js::GetObjectClass(obj)->ext.isWrappedNative ||
               mozilla::dom::IsDOMObject(obj));

    XPCCallContext ccx(NATIVE_CALLER, cx);
    if (!ccx.IsValid())
        return false;

    if (!IS_WRAPPER_CLASS(js::GetObjectClass(obj)))
        return mozilla::dom::TryPreserveWrapper(obj);

    nsISupports *supports = nullptr;
    if (IS_WN_WRAPPER_OBJECT(obj))
        supports = XPCWrappedNative::Get(obj)->Native();
    else
        supports = static_cast<nsISupports*>(xpc_GetJSPrivate(obj));

    
    if (nsCOMPtr<nsINode> node = do_QueryInterface(supports)) {
        nsContentUtils::PreserveWrapper(supports, node);
        return true;
    }
    return false;
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
SourceHook(JSContext *cx, JSScript *script, jschar **src, uint32_t *length)
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

XPCJSRuntime::XPCJSRuntime(nsXPConnect* aXPConnect)
 : mXPConnect(aXPConnect),
   mJSRuntime(nullptr),
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
   mWatchdogLock(nullptr),
   mWatchdogWakeup(nullptr),
   mWatchdogThread(nullptr),
   mWatchdogHibernating(false),
   mLastActiveTime(-1),
   mExceptionManagerNotAvailable(false)
#ifdef DEBUG
   , mObjectToUnlink(nullptr)
#endif
{
#ifdef XPC_CHECK_WRAPPERS_AT_SHUTDOWN
    DEBUG_WrappedNativeHashtable =
        JS_NewDHashTable(JS_DHashGetStubOps(), nullptr,
                         sizeof(JSDHashEntryStub), 128);
#endif

    DOM_InitInterfaces();
    Preferences::AddBoolVarCache(&gXBLScopesEnabled,
                                 "dom.xbl_scopes",
                                 false);


    
    mStrIDs[0] = JSID_VOID;

    mJSRuntime = JS_NewRuntime(32L * 1024L * 1024L, JS_USE_HELPER_THREADS); 
    if (!mJSRuntime)
        NS_RUNTIMEABORT("JS_NewRuntime failed.");

    
    
    
    
    
    
    JS_SetGCParameter(mJSRuntime, JSGC_MAX_BYTES, 0xffffffff);
#if defined(MOZ_ASAN) || (defined(DEBUG) && !defined(XP_WIN))
    
    
    
    JS_SetNativeStackQuota(mJSRuntime, 2 * 128 * sizeof(size_t) * 1024);
#else
    JS_SetNativeStackQuota(mJSRuntime, 128 * sizeof(size_t) * 1024);
#endif
    JS_SetContextCallback(mJSRuntime, ContextCallback);
    JS_SetDestroyCompartmentCallback(mJSRuntime, CompartmentDestroyedCallback);
    JS_SetCompartmentNameCallback(mJSRuntime, CompartmentNameCallback);
    JS_SetGCCallback(mJSRuntime, GCCallback);
    mPrevGCSliceCallback = JS::SetGCSliceCallback(mJSRuntime, GCSliceCallback);
    JS_SetFinalizeCallback(mJSRuntime, FinalizeCallback);
    JS_SetExtraGCRootsTracer(mJSRuntime, TraceBlackJS, this);
    JS_SetGrayGCRootsTracer(mJSRuntime, TraceGrayJS, this);
    JS_SetWrapObjectCallbacks(mJSRuntime,
                              xpc::WrapperFactory::Rewrap,
                              xpc::WrapperFactory::WrapForSameCompartment,
                              xpc::WrapperFactory::PrepareForWrapping);
    js::SetPreserveWrapperCallback(mJSRuntime, PreserveWrapper);
#ifdef MOZ_CRASHREPORTER
    JS_EnumerateDiagnosticMemoryRegions(DiagnosticMemoryCallback);
#endif
#ifdef MOZ_ENABLE_PROFILER_SPS
    if (PseudoStack *stack = mozilla_get_pseudo_stack())
        stack->sampleRuntime(mJSRuntime);
#endif
    JS_SetAccumulateTelemetryCallback(mJSRuntime, AccumulateTelemetryCallback);
    js::SetActivityCallback(mJSRuntime, ActivityCallback, this);
    js::SetCTypesActivityCallback(mJSRuntime, CTypesActivityCallback);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    JS_SetSourceHook(mJSRuntime, SourceHook);

    
    
    
    if (!xpc_LocalizeRuntime(mJSRuntime))
        NS_RUNTIMEABORT("xpc_LocalizeRuntime failed.");

    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(XPConnectJSGCHeap));
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(XPConnectJSSystemCompartmentCount));
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(XPConnectJSUserCompartmentCount));
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(JSMainRuntimeTemporaryPeak));
    NS_RegisterMemoryMultiReporter(new JSCompartmentsMultiReporter);

    mJSHolders.Init(512);

    
#ifdef DEBUG
    if (!JS_GetGlobalDebugHooks(mJSRuntime)->debuggerHandler)
        xpc_InstallJSDebuggerKeywordHandler(mJSRuntime);
#endif

    mWatchdogLock = PR_NewLock();
    if (!mWatchdogLock)
        NS_RUNTIMEABORT("PR_NewLock failed.");
    mWatchdogWakeup = PR_NewCondVar(mWatchdogLock);
    if (!mWatchdogWakeup)
        NS_RUNTIMEABORT("PR_NewCondVar failed.");

    {
        AutoLockWatchdog lock(this);

        mWatchdogThread = PR_CreateThread(PR_USER_THREAD, WatchdogMain, this,
                                          PR_PRIORITY_NORMAL, PR_LOCAL_THREAD,
                                          PR_UNJOINABLE_THREAD, 0);
        if (!mWatchdogThread)
            NS_RUNTIMEABORT("PR_CreateThread failed!");
    }
}


XPCJSRuntime*
XPCJSRuntime::newXPCJSRuntime(nsXPConnect* aXPConnect)
{
    NS_PRECONDITION(aXPConnect,"bad param");

    XPCJSRuntime* self = new XPCJSRuntime(aXPConnect);

    if (self                                    &&
        self->GetJSRuntime()                    &&
        self->GetWrappedJSMap()                 &&
        self->GetWrappedJSClassMap()            &&
        self->GetIID2NativeInterfaceMap()       &&
        self->GetClassInfo2NativeSetMap()       &&
        self->GetNativeSetMap()                 &&
        self->GetThisTranslatorMap()            &&
        self->GetNativeScriptableSharedMap()    &&
        self->GetDyingWrappedNativeProtoMap()   &&
        self->GetMapLock()                      &&
        self->mWatchdogThread) {
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
    
    if (JSID_IS_VOID(mStrIDs[0])) {
        JS_SetGCParameterForThread(cx, JSGC_MAX_CODE_CACHE_BYTES, 16 * 1024 * 1024);
        {
            
            
            JSAutoRequest ar(cx);
            for (unsigned i = 0; i < IDX_TOTAL_COUNT; i++) {
                JSString* str = JS_InternString(cx, mStrings[i]);
                if (!str || !JS_ValueToId(cx, STRING_TO_JSVAL(str), &mStrIDs[i])) {
                    mStrIDs[0] = JSID_VOID;
                    return false;
                }
                mStrJSVals[i] = STRING_TO_JSVAL(str);
            }
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
XPCJSRuntime::DeferredRelease(nsISupports *obj)
{
    MOZ_ASSERT(obj);

    if (mNativesToReleaseArray.IsEmpty()) {
        
        
        
        mNativesToReleaseArray.SetCapacity(256);
    }
    return mNativesToReleaseArray.AppendElement(obj) != nullptr;
}



#ifdef DEBUG
static JSDHashOperator
WrappedJSClassMapDumpEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                uint32_t number, void *arg)
{
    ((IID2WrappedJSClassMap::Entry*)hdr)->value->DebugDump(*(int16_t*)arg);
    return JS_DHASH_NEXT;
}
static JSDHashOperator
NativeSetDumpEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                        uint32_t number, void *arg)
{
    ((NativeSetMap::Entry*)hdr)->key_value->DebugDump(*(int16_t*)arg);
    return JS_DHASH_NEXT;
}
#endif

void
XPCJSRuntime::DebugDump(int16_t depth)
{
#ifdef DEBUG
    depth--;
    XPC_LOG_ALWAYS(("XPCJSRuntime @ %x", this));
        XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("mXPConnect @ %x", mXPConnect));
        XPC_LOG_ALWAYS(("mJSRuntime @ %x", mJSRuntime));
        XPC_LOG_ALWAYS(("mMapLock @ %x", mMapLock));

        XPC_LOG_ALWAYS(("mWrappedJSToReleaseArray @ %x with %d wrappers(s)", \
                        &mWrappedJSToReleaseArray,
                        mWrappedJSToReleaseArray.Length()));

        int cxCount = 0;
        JSContext* iter = nullptr;
        while (JS_ContextIterator(mJSRuntime, &iter))
            ++cxCount;
        XPC_LOG_ALWAYS(("%d JS context(s)", cxCount));

        iter = nullptr;
        while (JS_ContextIterator(mJSRuntime, &iter)) {
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
    if (nsXPConnect *xpc = nsXPConnect::GetXPConnect())
        JS::PokeGC(xpc->GetRuntime()->GetJSRuntime());

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
XPCJSRuntime::AddGCCallback(JSGCCallback cb)
{
    NS_ASSERTION(cb, "null callback");
    extraGCCallbacks.AppendElement(cb);
}

void
XPCJSRuntime::RemoveGCCallback(JSGCCallback cb)
{
    NS_ASSERTION(cb, "null callback");
    bool found = extraGCCallbacks.RemoveElement(cb);
    if (!found) {
        NS_ERROR("Removing a callback which was never added.");
    }
}
