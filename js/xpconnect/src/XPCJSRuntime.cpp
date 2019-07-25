







#include "mozilla/Util.h"

#include "xpcprivate.h"
#include "xpcpublic.h"
#include "WrapperFactory.h"
#include "dom_quickstubs.h"

#include "nsIMemoryReporter.h"
#include "nsPrintfCString.h"
#include "mozilla/FunctionTimer.h"
#include "prsystem.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"

#include "nsContentUtils.h"
#include "nsCCUncollectableMarker.h"
#include "jsfriendapi.h"
#include "js/MemoryMetrics.h"
#include "mozilla/dom/DOMJSClass.h"

#include "nsJSPrincipals.h"

#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif

using namespace mozilla;



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
    "__scriptOnly__",       
    "baseURIObject",        
    "nodePrincipal",        
    "documentURIObject"     
};



static JSDHashOperator
WrappedJSDyingJSObjectFinder(JSDHashTable *table, JSDHashEntryHdr *hdr,
                             uint32_t number, void *arg)
{
    nsTArray<nsXPCWrappedJS*>* array = static_cast<nsTArray<nsXPCWrappedJS*>*>(arg);
    nsXPCWrappedJS* wrapper = ((JSObject2WrappedJSMap::Entry*)hdr)->value;
    NS_ASSERTION(wrapper, "found a null JS wrapper!");

    
    while (wrapper) {
        if (wrapper->IsSubjectToFinalization()) {
            if (JS_IsAboutToBeFinalized(wrapper->GetJSObjectPreserveColor()))
                array->AppendElement(wrapper);
        }
        wrapper = wrapper->GetNextWrapper();
    }
    return JS_DHASH_NEXT;
}

struct CX_AND_XPCRT_Data
{
    JSContext* cx;
    XPCJSRuntime* rt;
};

static JSDHashOperator
NativeInterfaceSweeper(JSDHashTable *table, JSDHashEntryHdr *hdr,
                       uint32_t number, void *arg)
{
    XPCNativeInterface* iface = ((IID2NativeInterfaceMap::Entry*)hdr)->value;
    if (iface->IsMarked()) {
        iface->Unmark();
        return JS_DHASH_NEXT;
    }

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

#ifdef XPC_REPORT_NATIVE_INTERFACE_AND_SET_FLUSHING
    printf("- Destroying XPCNativeSet for:\n");
    PRUint16 count = set->GetInterfaceCount();
    for (PRUint16 k = 0; k < count; k++) {
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

xpc::CompartmentPrivate::~CompartmentPrivate()
{
    MOZ_COUNT_DTOR(xpc::CompartmentPrivate);
}

static void
CompartmentDestroyedCallback(JSFreeOp *fop, JSCompartment *compartment)
{
    XPCJSRuntime* self = nsXPConnect::GetRuntimeInstance();
    if (!self)
        return;
    XPCCompartmentSet &set = self->GetCompartmentSet();

    
    
    nsAutoPtr<xpc::CompartmentPrivate>
      priv(static_cast<xpc::CompartmentPrivate*>(JS_GetCompartmentPrivate(compartment)));
    JS_SetCompartmentPrivate(compartment, nsnull);

    
    
    
    
    if (!priv) {
        MOZ_ASSERT(!set.has(compartment));
        return;
    }

    
    MOZ_ASSERT(set.has(compartment));
    set.remove(compartment);
    return;
}

struct ObjectHolder : public JSDHashEntryHdr
{
    void *holder;
    nsScriptObjectTracer* tracer;
};

nsresult
XPCJSRuntime::AddJSHolder(void* aHolder, nsScriptObjectTracer* aTracer)
{
    if (!mJSHolders.ops)
        return NS_ERROR_OUT_OF_MEMORY;

    ObjectHolder *entry =
        reinterpret_cast<ObjectHolder*>(JS_DHashTableOperate(&mJSHolders,
                                                             aHolder,
                                                             JS_DHASH_ADD));
    if (!entry)
        return NS_ERROR_OUT_OF_MEMORY;

    entry->holder = aHolder;
    entry->tracer = aTracer;

    return NS_OK;
}

nsresult
XPCJSRuntime::RemoveJSHolder(void* aHolder)
{
    if (!mJSHolders.ops)
        return NS_ERROR_OUT_OF_MEMORY;

    JS_DHashTableOperate(&mJSHolders, aHolder, JS_DHASH_REMOVE);

    return NS_OK;
}


void XPCJSRuntime::TraceBlackJS(JSTracer* trc, void* data)
{
    XPCJSRuntime* self = (XPCJSRuntime*)data;

    
    
    if (!self->GetXPConnect()->IsShuttingDown()) {
        Mutex* threadLock = XPCPerThreadData::GetLock();
        if (threadLock)
        { 
            MutexAutoLock lock(*threadLock);

            XPCPerThreadData* iterp = nsnull;
            XPCPerThreadData* thread;

            while (nsnull != (thread =
                              XPCPerThreadData::IterateThreads(&iterp))) {
                
                thread->TraceJS(trc);
            }
        }
    }

    {
        XPCAutoLock lock(self->mMapLock);

        
        
        XPCRootSetElem *e;
        for (e = self->mObjectHolderRoots; e; e = e->GetNextRoot())
            static_cast<XPCJSObjectHolder*>(e)->TraceJS(trc);
    }

    dom::TraceBlackJS(trc);

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
                   js_GetGCThingTraceKind(aScriptThing), name);
}

static JSDHashOperator
TraceJSHolder(JSDHashTable *table, JSDHashEntryHdr *hdr, uint32_t number,
              void *arg)
{
    ObjectHolder* entry = reinterpret_cast<ObjectHolder*>(hdr);

    entry->tracer->Trace(entry->holder, TraceJSObject, arg);

    return JS_DHASH_NEXT;
}

static PLDHashOperator
TraceExpandos(XPCWrappedNative *wn, JSObject *&expando, void *aClosure)
{
    if (wn->IsWrapperExpired())
        return PL_DHASH_REMOVE;
    JS_CALL_OBJECT_TRACER(static_cast<JSTracer *>(aClosure), expando, "expando object");
    return PL_DHASH_NEXT;
}

static PLDHashOperator
TraceDOMExpandos(nsPtrHashKey<JSObject> *expando, void *aClosure)
{
    JS_CALL_OBJECT_TRACER(static_cast<JSTracer *>(aClosure), expando->GetKey(),
                          "DOM expando object");
    return PL_DHASH_NEXT;
}

void XPCJSRuntime::TraceXPConnectRoots(JSTracer *trc)
{
    JSContext *iter = nsnull;
    while (JSContext *acx = JS_ContextIterator(GetJSRuntime(), &iter)) {
        JS_ASSERT(js::HasUnrootedGlobal(acx));
        if (JSObject *global = JS_GetGlobalObject(acx))
            JS_CALL_OBJECT_TRACER(trc, global, "XPC global object");
    }

    XPCAutoLock lock(mMapLock);

    XPCWrappedNativeScope::TraceJS(trc, this);

    for (XPCRootSetElem *e = mVariantRoots; e ; e = e->GetNextRoot())
        static_cast<XPCTraceableVariant*>(e)->TraceJS(trc);

    for (XPCRootSetElem *e = mWrappedJSRoots; e ; e = e->GetNextRoot())
        static_cast<nsXPCWrappedJS*>(e)->TraceJS(trc);

    if (mJSHolders.ops)
        JS_DHashTableEnumerate(&mJSHolders, TraceJSHolder, trc);

    
    XPCCompartmentSet &set = GetCompartmentSet();
    for (XPCCompartmentRange r = set.all(); !r.empty(); r.popFront()) {
        xpc::CompartmentPrivate *priv = (xpc::CompartmentPrivate *)
            JS_GetCompartmentPrivate(r.front());
        if (priv->expandoMap)
            priv->expandoMap->Enumerate(TraceExpandos, trc);
        if (priv->domExpandoMap)
            priv->domExpandoMap->EnumerateEntries(TraceDOMExpandos, trc);
    }
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

    if (AddToCCKind(js_GetGCThingTraceKind(aThing)) &&
        xpc_IsGrayGCThing(aThing))
    {
        closure->cycleCollectionEnabled = true;
    }
}

static JSDHashOperator
NoteJSHolder(JSDHashTable *table, JSDHashEntryHdr *hdr, uint32_t number,
             void *arg)
{
    ObjectHolder* entry = reinterpret_cast<ObjectHolder*>(hdr);
    Closure *closure = static_cast<Closure*>(arg);

    closure->cycleCollectionEnabled = false;
    entry->tracer->Trace(entry->holder, CheckParticipatesInCycleCollection,
                         closure);
    if (!closure->cycleCollectionEnabled)
        return JS_DHASH_NEXT;

    closure->cb->NoteNativeRoot(entry->holder, entry->tracer);

    return JS_DHASH_NEXT;
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

static PLDHashOperator
SuspectExpandos(XPCWrappedNative *wrapper, JSObject *expando, void *arg)
{
    Closure* closure = static_cast<Closure*>(arg);
    XPCJSRuntime::SuspectWrappedNative(wrapper, *closure->cb);

    return PL_DHASH_NEXT;
}

static PLDHashOperator
SuspectDOMExpandos(nsPtrHashKey<JSObject> *key, void *arg)
{
    Closure *closure = static_cast<Closure*>(arg);
    JSObject* obj = key->GetKey();
    nsISupports* native = nsnull;
    if (js::IsProxy(obj)) {
        NS_ASSERTION(mozilla::dom::binding::instanceIsProxy(obj),
                     "Not a DOM proxy?");
        native = static_cast<nsISupports*>(js::GetProxyPrivate(obj).toPrivate());
    }
    else {
        NS_ASSERTION(mozilla::dom::DOMJSClass::FromJSClass(JS_GetClass(obj))->mDOMObjectIsISupports,
                     "Someone added a wrapper for a non-nsISupports native to DOMExpandos!");
        native = static_cast<nsISupports*>(js::GetReservedSlot(obj, DOM_OBJECT_SLOT).toPrivate());
    }
    closure->cb->NoteXPCOMRoot(native);
    return PL_DHASH_NEXT;
}

void
XPCJSRuntime::AddXPConnectRoots(nsCycleCollectionTraversalCallback &cb)
{
    
    
    
    
    
    
    

    JSContext *iter = nsnull, *acx;
    while ((acx = JS_ContextIterator(GetJSRuntime(), &iter))) {
        cb.NoteNativeRoot(acx, nsXPConnect::JSContextParticipant());
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
        JSObject *obj = wrappedJS->GetJSObjectPreserveColor();
        
        
        
        if (nsCCUncollectableMarker::sGeneration &&
            !cb.WantAllTraces() && (!obj || !xpc_IsGrayGCThing(obj)) &&
            !wrappedJS->IsSubjectToFinalization() &&
            wrappedJS->GetRootWrapper() == wrappedJS &&
            !wrappedJS->IsAggregatedToNative()) {
            continue;
        }

        cb.NoteXPCOMRoot(static_cast<nsIXPConnectWrappedJS *>(wrappedJS));
    }

    Closure closure = { true, &cb };
    if (mJSHolders.ops) {
        JS_DHashTableEnumerate(&mJSHolders, NoteJSHolder, &closure);
    }

    
    XPCCompartmentSet &set = GetCompartmentSet();
    for (XPCCompartmentRange r = set.all(); !r.empty(); r.popFront()) {
        xpc::CompartmentPrivate *priv = (xpc::CompartmentPrivate *)
            JS_GetCompartmentPrivate(r.front());
        if (priv->expandoMap)
            priv->expandoMap->EnumerateRead(SuspectExpandos, &closure);
        if (priv->domExpandoMap)
            priv->domExpandoMap->EnumerateEntries(SuspectDOMExpandos, &closure);
    }
}

static JSDHashOperator
UnmarkJSHolder(JSDHashTable *table, JSDHashEntryHdr *hdr, uint32_t number,
               void *arg)
{
    ObjectHolder* entry = reinterpret_cast<ObjectHolder*>(hdr);
    entry->tracer->CanSkip(entry->holder, true);
    return JS_DHASH_NEXT;
}

void
XPCJSRuntime::UnmarkSkippableJSHolders()
{
    XPCAutoLock lock(mMapLock);
    if (mJSHolders.ops) {             
        JS_DHashTableEnumerate(&mJSHolders, UnmarkJSHolder, nsnull);
    }
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
        PRUint32 count = array.Length();
        if (!count) {
            array.Compact();
            break;
        }
        T wrapper = array[count-1];
        array.RemoveElementAt(count-1);
        NS_RELEASE(wrapper);
    }
}

static PLDHashOperator
SweepExpandos(XPCWrappedNative *wn, JSObject *&expando, void *arg)
{
    return JS_IsAboutToBeFinalized(wn->GetFlatJSObjectPreserveColor())
           ? PL_DHASH_REMOVE
           : PL_DHASH_NEXT;
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
            
            
            JSContext *iter = nsnull;
            while (JSContext *acx = JS_ContextIterator(rt, &iter)) {
                if (!js::HasUnrootedGlobal(acx))
                    JS_ToggleOptions(acx, JSOPTION_UNROOTED_GLOBAL);
            }
            break;
        }
        case JSGC_END:
        {
            
#ifdef XPC_TRACK_DEFERRED_RELEASES
            printf("XPC - Begin deferred Release of %d nsISupports pointers\n",
                   self->mNativesToReleaseArray.Length());
#endif
            DoDeferredRelease(self->mNativesToReleaseArray);
#ifdef XPC_TRACK_DEFERRED_RELEASES
            printf("XPC - End deferred Releases\n");
#endif

            self->GetXPConnect()->ClearGCBeforeCC();
            break;
        }
    }

    nsTArray<JSGCCallback> callbacks(self->extraGCCallbacks);
    for (PRUint32 i = 0; i < callbacks.Length(); ++i)
        callbacks[i](rt, status);
}

 void
XPCJSRuntime::FinalizeCallback(JSFreeOp *fop, JSFinalizeStatus status)
{
    XPCJSRuntime* self = nsXPConnect::GetRuntimeInstance();
    if (!self)
        return;

    switch (status) {
        case JSFINALIZE_START:
        {
            NS_ASSERTION(!self->mDoingFinalization, "bad state");

            
            { 
                XPCAutoLock lock(self->GetMapLock());
                NS_ASSERTION(!self->mThreadRunningGC, "bad state");
                self->mThreadRunningGC = PR_GetCurrentThread();
            }

            nsTArray<nsXPCWrappedJS*>* dyingWrappedJSArray =
                &self->mWrappedJSToReleaseArray;

            
            
            
            
            
            
            self->mWrappedJSMap->
                Enumerate(WrappedJSDyingJSObjectFinder, dyingWrappedJSArray);

            
            XPCWrappedNativeScope::StartFinalizationPhaseOfGC(fop, self);

            
            XPCCompartmentSet &set = self->GetCompartmentSet();
            for (XPCCompartmentRange r = set.all(); !r.empty(); r.popFront()) {
                xpc::CompartmentPrivate *priv = (xpc::CompartmentPrivate *)
                    JS_GetCompartmentPrivate(r.front());
                if (priv->waiverWrapperMap)
                    priv->waiverWrapperMap->Sweep();
                if (priv->expandoMap)
                    priv->expandoMap->Enumerate(SweepExpandos, NULL);
            }

            self->mDoingFinalization = true;
            break;
        }
        case JSFINALIZE_END:
        {
            NS_ASSERTION(self->mDoingFinalization, "bad state");
            self->mDoingFinalization = false;

            
            
            DoDeferredRelease(self->mWrappedJSToReleaseArray);

#ifdef XPC_REPORT_NATIVE_INTERFACE_AND_SET_FLUSHING
            printf("--------------------------------------------------------------\n");
            int setsBefore = (int) self->mNativeSetMap->Count();
            int ifacesBefore = (int) self->mIID2NativeInterfaceMap->Count();
#endif

            
            

            
            XPCWrappedNativeScope::MarkAllWrappedNativesAndProtos();

            self->mDetachedWrappedNativeProtoMap->
                Enumerate(DetachedWrappedNativeProtoMarker, nsnull);

            DOM_MarkInterfaces();

            
            
            
            
            
            

            
            
            if (!self->GetXPConnect()->IsShuttingDown()) {
                Mutex* threadLock = XPCPerThreadData::GetLock();
                if (threadLock)
                { 
                    MutexAutoLock lock(*threadLock);

                    XPCPerThreadData* iterp = nsnull;
                    XPCPerThreadData* thread;

                    while (nsnull != (thread =
                                      XPCPerThreadData::IterateThreads(&iterp))) {
                        
                        thread->MarkAutoRootsAfterJSFinalize();

                        XPCCallContext* ccxp = thread->GetCallContext();
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
                }
            }

            

            
            
            
            if (!self->GetXPConnect()->IsShuttingDown()) {
                self->mNativeScriptableSharedMap->
                    Enumerate(JSClassSweeper, nsnull);
            }

            self->mClassInfo2NativeSetMap->
                Enumerate(NativeUnMarkedSetRemover, nsnull);

            self->mNativeSetMap->
                Enumerate(NativeSetSweeper, nsnull);

            self->mIID2NativeInterfaceMap->
                Enumerate(NativeInterfaceSweeper, nsnull);

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

            
            XPCWrappedNativeScope::FinishedFinalizationPhaseOfGC();

            
            
            
            
            
            
            
            
            
            
            

            
            
            if (!self->GetXPConnect()->IsShuttingDown()) {
                Mutex* threadLock = XPCPerThreadData::GetLock();
                if (threadLock) {
                    

                    { 
                        MutexAutoLock lock(*threadLock);

                        XPCPerThreadData* iterp = nsnull;
                        XPCPerThreadData* thread;

                        while (nsnull != (thread =
                                          XPCPerThreadData::IterateThreads(&iterp))) {
                            XPCCallContext* ccxp = thread->GetCallContext();
                            while (ccxp) {
                                
                                
                                
                                
                                if (ccxp->CanGetTearOff()) {
                                    XPCWrappedNativeTearOff* to =
                                        ccxp->GetTearOff();
                                    if (to)
                                        to->Mark();
                                }
                                ccxp = ccxp->GetPrevCallContext();
                            }
                        }
                    }

                    
                    XPCWrappedNativeScope::SweepAllWrappedNativeTearOffs();
                }
            }

            
            
            
            
            
            
            
            
            
            
            
            
            

            self->mDyingWrappedNativeProtoMap->
                Enumerate(DyingProtoKiller, nsnull);


            
            
            { 
                XPCAutoLock lock(self->GetMapLock());
                NS_ASSERTION(self->mThreadRunningGC == PR_GetCurrentThread(), "bad state");
                self->mThreadRunningGC = nsnull;
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

size_t
XPCJSRuntime::SizeOfIncludingThis(nsMallocSizeOfFun mallocSizeOf)
{
    size_t n = 0;
    n += mallocSizeOf(this);
    n += mWrappedJSMap->SizeOfIncludingThis(mallocSizeOf);
    n += mIID2NativeInterfaceMap->SizeOfIncludingThis(mallocSizeOf);
    n += mClassInfo2NativeSetMap->ShallowSizeOfIncludingThis(mallocSizeOf);
    n += mNativeSetMap->SizeOfIncludingThis(mallocSizeOf);

    
    
    n += JS_DHashTableSizeOfExcludingThis(&mJSHolders, NULL, mallocSizeOf);

    
    
    

    return n;
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
WrappedJSShutdownMarker(JSDHashTable *table, JSDHashEntryHdr *hdr,
                        uint32_t number, void *arg)
{
    JSRuntime* rt = (JSRuntime*) arg;
    nsXPCWrappedJS* wrapper = ((JSObject2WrappedJSMap::Entry*)hdr)->value;
    NS_ASSERTION(wrapper, "found a null JS wrapper!");
    NS_ASSERTION(wrapper->IsValid(), "found an invalid JS wrapper!");
    wrapper->SystemIsBeingShutDown(rt);
    return JS_DHASH_NEXT;
}

static JSDHashOperator
DetachedWrappedNativeProtoShutdownMarker(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                         uint32_t number, void *arg)
{
    XPCWrappedNativeProto* proto =
        (XPCWrappedNativeProto*)((JSDHashEntryStub*)hdr)->key;

    proto->SystemIsBeingShutDown();
    return JS_DHASH_NEXT;
}

void XPCJSRuntime::SystemIsBeingShutDown()
{
    DOM_ClearInterfaces();

    if (mDetachedWrappedNativeProtoMap)
        mDetachedWrappedNativeProtoMap->
            Enumerate(DetachedWrappedNativeProtoShutdownMarker, nsnull);
}

JSContext *
XPCJSRuntime::GetJSCycleCollectionContext()
{
    if (!mJSCycleCollectionContext) {
        mJSCycleCollectionContext = JS_NewContext(mJSRuntime, 0);
        if (!mJSCycleCollectionContext)
            return nsnull;
    }
    return mJSCycleCollectionContext;
}

XPCJSRuntime::~XPCJSRuntime()
{
    if (mWatchdogWakeup) {
        
        
        
        
        {
            AutoLockWatchdog lock(this);
            if (mWatchdogThread) {
                mWatchdogThread = nsnull;
                PR_NotifyCondVar(mWatchdogWakeup);
                PR_WaitCondVar(mWatchdogWakeup, PR_INTERVAL_NO_TIMEOUT);
            }
        }
        PR_DestroyCondVar(mWatchdogWakeup);
        PR_DestroyLock(mWatchdogLock);
        mWatchdogWakeup = nsnull;
    }

    if (mJSCycleCollectionContext)
        JS_DestroyContextNoGC(mJSCycleCollectionContext);

#ifdef XPC_DUMP_AT_SHUTDOWN
    {
    
    JSContext* iter = nsnull;
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
        mWrappedJSMap->Enumerate(WrappedJSShutdownMarker, mJSRuntime);
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

    if (mJSHolders.ops) {
        JS_DHashTableFinish(&mJSHolders);
        mJSHolders.ops = nsnull;
    }

    if (mJSRuntime) {
        JS_DestroyRuntime(mJSRuntime);
        JS_ShutDown();
#ifdef DEBUG_shaver_off
        fprintf(stderr, "nJRSI: destroyed runtime %p\n", (void *)mJSRuntime);
#endif
    }

    XPCPerThreadData::ShutDown();
}

static void
GetCompartmentName(JSCompartment *c, nsCString &name)
{
    if (js::IsAtomsCompartment(c)) {
        name.AssignLiteral("atoms");
    } else if (JSPrincipals *principals = JS_GetCompartmentPrincipals(c)) {
        nsJSPrincipals::get(principals)->GetScriptLocation(name);

        
        
        
        
        xpc::CompartmentPrivate *compartmentPrivate =
            static_cast<xpc::CompartmentPrivate*>(JS_GetCompartmentPrivate(c));
        if (compartmentPrivate &&
            !compartmentPrivate->location.IsEmpty() &&
            !compartmentPrivate->location.Equals(name)) {
            name.AppendLiteral(", ");
            name.Append(compartmentPrivate->location);
        }
        
        
        
        
        name.ReplaceChar('/', '\\');
    } else {
        name.AssignLiteral("null-principal");
    }
}

static PRInt64
GetGCChunkTotalBytes()
{
    JSRuntime *rt = nsXPConnect::GetRuntimeInstance()->GetJSRuntime();
    return PRInt64(JS_GetGCParameter(rt, JSGC_TOTAL_CHUNKS)) * js::gc::ChunkSize;
}




NS_MEMORY_REPORTER_IMPLEMENT(XPConnectJSGCHeap,
                             "js-gc-heap",
                             KIND_OTHER,
                             nsIMemoryReporter::UNITS_BYTES,
                             GetGCChunkTotalBytes,
                             "Memory used by the garbage-collected JavaScript heap.")
static PRInt64
GetJSSystemCompartmentCount()
{
    return JS::SystemCompartmentCount(nsXPConnect::GetRuntimeInstance()->GetJSRuntime());
}

static PRInt64
GetJSUserCompartmentCount()
{
    return JS::UserCompartmentCount(nsXPConnect::GetRuntimeInstance()->GetJSRuntime());
}








NS_MEMORY_REPORTER_IMPLEMENT(XPConnectJSSystemCompartmentCount,
    "js-compartments-system",
    KIND_OTHER,
    nsIMemoryReporter::UNITS_COUNT,
    GetJSSystemCompartmentCount,
    "The number of JavaScript compartments for system code.  The sum of this "
    "and 'js-compartments-user' might not match the number of compartments "
    "listed under 'js' if a garbage collection occurs at an inopportune time, "
    "but such cases should be rare.")

NS_MEMORY_REPORTER_IMPLEMENT(XPConnectJSUserCompartmentCount,
    "js-compartments-user",
    KIND_OTHER,
    nsIMemoryReporter::UNITS_COUNT,
    GetJSUserCompartmentCount,
    "The number of JavaScript compartments for user code.  The sum of this "
    "and 'js-compartments-system' might not match the number of compartments "
    "listed under 'js' if a garbage collection occurs at an inopportune time, "
    "but such cases should be rare.")





static const size_t SUNDRIES_THRESHOLD = 8192;

#define REPORT(_path, _kind, _units, _amount, _desc)                          \
    do {                                                                      \
        nsresult rv;                                                          \
        rv = cb->Callback(EmptyCString(), _path, _kind, _units, _amount,      \
                          NS_LITERAL_CSTRING(_desc), closure);                \
        NS_ENSURE_SUCCESS(rv, rv);                                            \
    } while (0)

#define CREPORT(_path, _kind, _units, _amount, _desc)                         \
    do {                                                                      \
        size_t amount = _amount;  /* evaluate _amount only once */            \
        if (amount >= SUNDRIES_THRESHOLD) {                                   \
            nsresult rv;                                                      \
            rv = cb->Callback(EmptyCString(), _path, _kind, _units, amount,   \
                              NS_LITERAL_CSTRING(_desc), closure);            \
            NS_ENSURE_SUCCESS(rv, rv);                                        \
        } else {                                                              \
            otherSundries += amount;                                          \
        }                                                                     \
    } while (0)

#define REPORT_BYTES(_path, _kind, _amount, _desc)                            \
    REPORT(_path, _kind, nsIMemoryReporter::UNITS_BYTES, _amount, _desc);

#define CREPORT_BYTES(_path, _kind, _amount, _desc)                           \
    CREPORT(_path, _kind, nsIMemoryReporter::UNITS_BYTES, _amount, _desc);

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

#define CREPORT_GC_BYTES(_path, _amount, _desc)                               \
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

template <int N>
inline const nsCString
MakePath(const nsACString &pathPrefix, const JS::CompartmentStats &cStats,
         const char (&reporterName)[N])
{
    const char *name = static_cast<char *>(cStats.extra);
    if (!name)
        name = "error while initializing compartment name";
    return pathPrefix + NS_LITERAL_CSTRING("compartment(") +
           nsDependentCString(name) + NS_LITERAL_CSTRING(")/") +
           nsDependentCString(reporterName);
}

namespace xpc {

static nsresult
ReportCompartmentStats(const JS::CompartmentStats &cStats,
                       const nsACString &pathPrefix,
                       nsIMemoryMultiReporterCallback *cb,
                       nsISupports *closure, size_t *gcTotalOut)
{
    size_t gcTotal = 0, gcHeapSundries = 0, otherSundries = 0;

    CREPORT_GC_BYTES(MakePath(pathPrefix, cStats, "gc-heap/arena/headers"),
                     cStats.gcHeapArenaHeaders,
                     "Memory on the compartment's garbage-collected JavaScript "
                     "heap, within arenas, that is used to hold internal "
                     "bookkeeping information.");

    CREPORT_GC_BYTES(MakePath(pathPrefix, cStats, "gc-heap/arena/padding"),
                     cStats.gcHeapArenaPadding,
                     "Memory on the compartment's garbage-collected JavaScript "
                     "heap, within arenas, that is unused and present only so "
                     "that other data is aligned. This constitutes internal "
                     "fragmentation.");

    CREPORT_GC_BYTES(MakePath(pathPrefix, cStats, "gc-heap/arena/unused"),
                     cStats.gcHeapArenaUnused,
                     "Memory on the compartment's garbage-collected JavaScript "
                     "heap, within arenas, that could be holding useful data "
                     "but currently isn't.");

    CREPORT_GC_BYTES(MakePath(pathPrefix, cStats, "gc-heap/objects/non-function"),
                     cStats.gcHeapObjectsNonFunction,
                     "Memory on the compartment's garbage-collected JavaScript "
                     "heap that holds non-function objects.");

    CREPORT_GC_BYTES(MakePath(pathPrefix, cStats, "gc-heap/objects/function"),
                     cStats.gcHeapObjectsFunction,
                     "Memory on the compartment's garbage-collected JavaScript "
                     "heap that holds function objects.");

    CREPORT_GC_BYTES(MakePath(pathPrefix, cStats, "gc-heap/strings"),
                     cStats.gcHeapStrings,
                     "Memory on the compartment's garbage-collected JavaScript "
                     "heap that holds string headers.  String headers contain "
                     "various pieces of information about a string, but do not "
                     "contain (except in the case of very short strings) the "
                     "string characters;  characters in longer strings are "
                     "counted " "under 'gc-heap/string-chars' instead.");

    CREPORT_GC_BYTES(MakePath(pathPrefix, cStats, "gc-heap/scripts"),
                     cStats.gcHeapScripts,
                     "Memory on the compartment's garbage-collected JavaScript "
                     "heap that holds JSScript instances. A JSScript is "
                     "created for each user-defined function in a script. One "
                     "is also created for the top-level code in a script.");

    CREPORT_GC_BYTES(MakePath(pathPrefix, cStats, "gc-heap/shapes/tree"),
                     cStats.gcHeapShapesTree,
                     "Memory on the compartment's garbage-collected JavaScript "
                     "heap that holds shapes that are in a property tree.");

    CREPORT_GC_BYTES(MakePath(pathPrefix, cStats, "gc-heap/shapes/dict"),
                     cStats.gcHeapShapesDict,
                     "Memory on the compartment's garbage-collected JavaScript "
                     "heap that holds shapes that are in dictionary mode.");

    CREPORT_GC_BYTES(MakePath(pathPrefix, cStats, "gc-heap/shapes/base"),
                     cStats.gcHeapShapesBase,
                     "Memory on the compartment's garbage-collected JavaScript "
                     "heap that collates data common to many shapes.");

    CREPORT_GC_BYTES(MakePath(pathPrefix, cStats, "gc-heap/type-objects"),
                     cStats.gcHeapTypeObjects,
                     "Memory on the compartment's garbage-collected JavaScript "
                     "heap that holds type inference information.");

#if JS_HAS_XML_SUPPORT
    CREPORT_GC_BYTES(MakePath(pathPrefix, cStats, "gc-heap/xml"),
                     cStats.gcHeapXML,
                     "Memory on the compartment's garbage-collected JavaScript "
                     "heap that holds E4X XML objects.");
#endif

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "objects/slots"),
                  nsIMemoryReporter::KIND_HEAP, cStats.objectSlots,
                  "Memory allocated for the compartment's non-fixed object "
                  "slot arrays, which are used to represent object properties. "
                  "Some objects also contain a fixed number of slots which are "
                  "stored on the compartment's JavaScript heap; those slots "
                  "are not counted here, but in 'gc-heap/objects' instead.");

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "objects/elements"),
                  nsIMemoryReporter::KIND_HEAP, cStats.objectElements,
                  "Memory allocated for the compartment's object element "
                  "arrays, which are used to represent indexed object "
                  "properties.");

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "objects/misc"),
                  nsIMemoryReporter::KIND_HEAP, cStats.objectMisc,
                  "Memory allocated for various small, miscellaneous "
                  "structures that hang off certain kinds of objects.");

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "string-chars"),
                  nsIMemoryReporter::KIND_HEAP, cStats.stringChars,
                  "Memory allocated to hold the compartment's string "
                  "characters.  Sometimes more memory is allocated than "
                  "necessary, to simplify string concatenation.  Each string "
                  "also includes a header which is stored on the "
                  "compartment's JavaScript heap;  that header is not counted "
                  "here, but in 'gc-heap/strings' instead.");

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "shapes-extra/tree-tables"),
                  nsIMemoryReporter::KIND_HEAP, cStats.shapesExtraTreeTables,
                  "Memory allocated for the compartment's property tables "
                  "that belong to shapes that are in a property tree.");

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "shapes-extra/dict-tables"),
                  nsIMemoryReporter::KIND_HEAP, cStats.shapesExtraDictTables,
                  "Memory allocated for the compartment's property tables "
                  "that belong to shapes that are in dictionary mode.");

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "shapes-extra/tree-shape-kids"),
                  nsIMemoryReporter::KIND_HEAP, cStats.shapesExtraTreeShapeKids,
                  "Memory allocated for the compartment's kid hashes that "
                  "belong to shapes that are in a property tree.");

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "shapes-extra/compartment-tables"),
                  nsIMemoryReporter::KIND_HEAP, cStats.shapesCompartmentTables,
                  "Memory used by compartment wide tables storing shape "
                  "information for use during object construction.");

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "script-data"),
                  nsIMemoryReporter::KIND_HEAP, cStats.scriptData,
                  "Memory allocated for JSScript bytecode and various "
                  "variable-length tables.");

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "mjit-data"),
                  nsIMemoryReporter::KIND_HEAP, cStats.mjitData,
                  "Memory used by the method JIT for the compartment's "
                  "compilation data: JITScripts, native maps, and inline "
                  "cache structs.");

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "cross-compartment-wrappers"),
                  nsIMemoryReporter::KIND_HEAP, cStats.crossCompartmentWrappers,
                  "Memory used by the compartment's cross-compartment "
                  "wrappers.");

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "type-inference/script-main"),
                  nsIMemoryReporter::KIND_HEAP,
                  cStats.typeInferenceSizes.scripts,
                  "Memory used during type inference to store type sets of "
                  "variables and dynamically observed types.");

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "type-inference/object-main"),
                  nsIMemoryReporter::KIND_HEAP,
                  cStats.typeInferenceSizes.objects,
                  "Memory used during type inference to store types and "
                  "possible property types of JS objects.");

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "type-inference/tables"),
                  nsIMemoryReporter::KIND_HEAP,
                  cStats.typeInferenceSizes.tables,
                  "Memory used during type inference for compartment-wide "
                  "tables.");

    CREPORT_BYTES(MakePath(pathPrefix, cStats, "analysis-temporary"),
                  nsIMemoryReporter::KIND_HEAP,
                  cStats.typeInferenceSizes.temporary,
                  "Memory used during type inference and compilation to hold "
                  "transient analysis information.  Cleared on GC.");

    if (gcHeapSundries > 0) {
        REPORT_GC_BYTES(MakePath(pathPrefix, cStats, "gc-heap/sundries"),
                        gcHeapSundries,
                        "The sum of all this compartment's gc-heap "
                        "measurements that are too small to be worth showing "
                        "individually.");
    }

    if (otherSundries > 0) {
        REPORT_BYTES(MakePath(pathPrefix, cStats, "other-sundries"),
                     nsIMemoryReporter::KIND_HEAP,
                     otherSundries,
                     "The sum of all this compartment's non-gc-heap "
                     "measurements that are too small to be worth showing "
                     "individually.");
    }

    *gcTotalOut += gcTotal;

    return NS_OK;
}

nsresult
ReportJSRuntimeExplicitTreeStats(const JS::RuntimeStats &rtStats,
                                 const nsACString &pathPrefix,
                                 nsIMemoryMultiReporterCallback *cb,
                                 nsISupports *closure)
{
    nsresult rv;
    size_t gcTotal = 0;
    for (size_t index = 0;
         index < rtStats.compartmentStatsVector.length();
         index++) {
        rv = ReportCompartmentStats(rtStats.compartmentStatsVector[index],
                                    pathPrefix, cb, closure, &gcTotal);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("runtime/runtime-object"),
                 nsIMemoryReporter::KIND_HEAP, rtStats.runtime.object,
                 "Memory used by the JSRuntime object.");

    REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("runtime/atoms-table"),
                 nsIMemoryReporter::KIND_HEAP, rtStats.runtime.atomsTable,
                 "Memory used by the atoms table.");

    REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("runtime/contexts"),
                 nsIMemoryReporter::KIND_HEAP, rtStats.runtime.contexts,
                 "Memory used by JSContext objects and certain structures "
                 "hanging off them.");

    REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("runtime/dtoa"),
                 nsIMemoryReporter::KIND_HEAP, rtStats.runtime.dtoa,
                 "Memory used by DtoaState, which is used for converting "
                 "strings to numbers and vice versa.");

    REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("runtime/temporary"),
                 nsIMemoryReporter::KIND_HEAP, rtStats.runtime.temporary,
                 "Memory held transiently in JSRuntime and used during "
                 "compilation.  It mostly holds parse nodes.");

    REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("runtime/mjit-code"),
                 nsIMemoryReporter::KIND_NONHEAP, rtStats.runtime.mjitCode,
                 "Memory used by the method JIT to hold the runtime's "
                 "generated code.");

    REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("runtime/regexp-code"),
                 nsIMemoryReporter::KIND_NONHEAP, rtStats.runtime.regexpCode,
                 "Memory used by the regexp JIT to hold generated code.");

    REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("runtime/unused-code-memory"),
                 nsIMemoryReporter::KIND_NONHEAP, rtStats.runtime.unusedCodeMemory,
                 "Memory allocated by the method and/or regexp JIT to hold the "
                 "runtime's code, but which is currently unused.");

    REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("runtime/stack-committed"),
                 nsIMemoryReporter::KIND_NONHEAP, rtStats.runtime.stackCommitted,
                 "Memory used for the JS call stack.  This is the committed "
                 "portion of the stack; the uncommitted portion is not "
                 "measured because it hardly costs anything.");

    REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("runtime/gc-marker"),
                 nsIMemoryReporter::KIND_HEAP, rtStats.runtime.gcMarker,
                 "Memory used for the GC mark stack and gray roots.");

    REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("runtime/math-cache"),
                 nsIMemoryReporter::KIND_HEAP, rtStats.runtime.mathCache,
                 "Memory used for the math cache.");

    REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("runtime/script-filenames"),
                 nsIMemoryReporter::KIND_HEAP, rtStats.runtime.scriptFilenames,
                 "Memory used for the table holding script filenames.");

    REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("runtime/compartment-objects"),
                 nsIMemoryReporter::KIND_HEAP, rtStats.runtime.compartmentObjects,
                 "Memory used for JSCompartment objects.  These are fairly "
                 "small and all the same size, so they're not worth reporting "
                 "on a per-compartment basis.");

    REPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap-chunk-dirty-unused"),
                    rtStats.gcHeapChunkDirtyUnused,
                    "Memory on the garbage-collected JavaScript heap, within "
                    "chunks with at least one allocated GC thing, that could "
                    "be holding useful data but currently isn't.  Memory here "
                    "is mutually exclusive with memory reported under "
                    "'explicit/js/gc-heap-decommitted'.");

    REPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap-chunk-clean-unused"),
                    rtStats.gcHeapChunkCleanUnused,
                    "Memory on the garbage-collected JavaScript heap taken by "
                    "completely empty chunks, that soon will be released "
                    "unless claimed for new allocations.  Memory here is "
                    "mutually exclusive with memory reported under "
                    "'explicit/js/gc-heap-decommitted'.");

    REPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap-decommitted"),
                    rtStats.gcHeapChunkCleanDecommitted + rtStats.gcHeapChunkDirtyDecommitted,
                    "Memory in the address space of the garbage-collected "
                    "JavaScript heap that is currently returned to the OS.");

    REPORT_GC_BYTES(pathPrefix + NS_LITERAL_CSTRING("gc-heap-chunk-admin"),
                    rtStats.gcHeapChunkAdmin,
                    "Memory on the garbage-collected JavaScript heap, within "
                    "chunks, that is used to hold internal bookkeeping "
                    "information.");

    
    
    JS_ASSERT(gcTotal == rtStats.gcHeapChunkTotal);

    return NS_OK;
}

} 

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(JsMallocSizeOf, "js")

class JSCompartmentsMultiReporter : public nsIMemoryMultiReporter
{
  public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD GetName(nsACString &name)
    {
        name.AssignLiteral("compartments");
        return NS_OK;
    }

    typedef js::Vector<nsCString, 0, js::SystemAllocPolicy> Paths; 

    static void CompartmentCallback(JSRuntime *rt, void* data, JSCompartment *c)
    {
        
        Paths *paths = static_cast<Paths *>(data);
        nsCString path;
        GetCompartmentName(c, path);
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
                   nsIMemoryReporter::KIND_SUMMARY,
                   nsIMemoryReporter::UNITS_COUNT,
                   1, "");

        return NS_OK;
    }

    NS_IMETHOD
    GetExplicitNonHeap(PRInt64 *n)
    {
        
        *n = 0;
        return NS_OK;
    }
};

NS_IMPL_THREADSAFE_ISUPPORTS1(JSCompartmentsMultiReporter
                              , nsIMemoryMultiReporter
                              )

struct XPCJSRuntimeStats : public JS::RuntimeStats {
    XPCJSRuntimeStats()
      : JS::RuntimeStats(JsMallocSizeOf) { }
    
    ~XPCJSRuntimeStats() {
        for (size_t i = 0; i != compartmentStatsVector.length(); ++i)
            free(compartmentStatsVector[i].extra);
    }

    virtual void initExtraCompartmentStats(JSCompartment *c,
                                           JS::CompartmentStats *cstats) MOZ_OVERRIDE {
        nsCAutoString name;
        GetCompartmentName(c, name);
        cstats->extra = strdup(name.get());
    }
};
    
class JSMemoryMultiReporter : public nsIMemoryMultiReporter
{
public:
    NS_DECL_ISUPPORTS

    NS_IMETHOD GetName(nsACString &name)
    {
        name.AssignLiteral("js");
        return NS_OK;
    }

    NS_IMETHOD CollectReports(nsIMemoryMultiReporterCallback *cb,
                              nsISupports *closure)
    {
        XPCJSRuntime *xpcrt = nsXPConnect::GetRuntimeInstance();

        
        
        
        
        
        XPCJSRuntimeStats rtStats;
        if (!JS::CollectRuntimeStats(xpcrt->GetJSRuntime(), &rtStats))
            return NS_ERROR_FAILURE;

        size_t xpconnect =
            xpcrt->SizeOfIncludingThis(JsMallocSizeOf) +
            XPCWrappedNativeScope::SizeOfAllScopesIncludingThis(JsMallocSizeOf);

        NS_NAMED_LITERAL_CSTRING(pathPrefix, "explicit/js/");

        
        

        nsresult rv =
            xpc::ReportJSRuntimeExplicitTreeStats(rtStats, pathPrefix, cb,
                                                  closure);
        NS_ENSURE_SUCCESS(rv, rv);

        REPORT_BYTES(pathPrefix + NS_LITERAL_CSTRING("xpconnect"),
                     nsIMemoryReporter::KIND_HEAP, xpconnect,
                     "Memory used by XPConnect.");

        REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-chunk-dirty-unused"),
                     nsIMemoryReporter::KIND_OTHER,
                     rtStats.gcHeapChunkDirtyUnused,
                     "The same as 'explicit/js/gc-heap-chunk-dirty-unused'. "
                     "Shown here for easy comparison with other 'js-gc' "
                     "reporters.");

        REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-chunk-clean-unused"),
                     nsIMemoryReporter::KIND_OTHER,
                     rtStats.gcHeapChunkCleanUnused,
                     "The same as 'explicit/js/gc-heap-chunk-clean-unused'.  "
                     "Shown here for easy comparison with other 'js-gc' "
                     "reporters.");

        REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-arena-unused"),
                     nsIMemoryReporter::KIND_OTHER,
                     rtStats.gcHeapArenaUnused,
                     "Memory on the main JSRuntime's garbage-collected "
                     "JavaScript heap, within arenas, that could be holding "
                     "useful data but currently isn't. This is the sum of all "
                     "compartments' 'gc-heap/arena-unused' numbers.");

        REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed-unused"),
                     nsIMemoryReporter::KIND_OTHER,
                     rtStats.gcHeapUnused,
                     "Amount of the GC heap that's committed, but that is "
                     "neither part of an active allocation nor being used for "
                     "bookkeeping.  Equal to 'gc-heap-chunk-dirty-unused' + "
                     "'gc-heap-chunk-clean-unused' + 'gc-heap-arena-unused'.");

        
        
        
        REPORT(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed-unused-ratio"),
               nsIMemoryReporter::KIND_OTHER,
               nsIMemoryReporter::UNITS_PERCENTAGE,
               (PRInt64) 10000 * rtStats.gcHeapUnused /
                   ((double) rtStats.gcHeapCommitted - rtStats.gcHeapUnused),
               "Ratio of committed, unused bytes to allocated bytes; i.e. "
               "'gc-heap-committed-unused' / 'gc-heap-allocated'.  This "
               "measures the overhead of the GC heap allocator relative to the "
               "amount of memory allocated.");

        REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-committed"),
                     nsIMemoryReporter::KIND_OTHER,
                     rtStats.gcHeapCommitted,
                     "Committed memory (i.e., in physical memory or swap) "
                     "used by the garbage-collected JavaScript heap.");

        REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-allocated"),
                     nsIMemoryReporter::KIND_OTHER,
                     (rtStats.gcHeapCommitted - rtStats.gcHeapUnused),
                     "Amount of the GC heap used for active allocations and "
                     "bookkeeping.  This is calculated as 'gc-heap-committed' "
                     "- 'gc-heap-unused'.");

        REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-gc-heap-decommitted"),
                     nsIMemoryReporter::KIND_OTHER,
                     rtStats.gcHeapChunkCleanDecommitted + rtStats.gcHeapChunkDirtyDecommitted,
                     "The same as 'explicit/js/gc-heap-decommitted'.  Shown "
                     "here for easy comparison with other 'js-gc' reporters.");

        REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-objects"),
                     nsIMemoryReporter::KIND_OTHER, rtStats.totalObjects,
                     "Memory used for all object-related data in the main "
                     "JSRuntime. This is the sum of all compartments' "
                     "'gc-heap/objects-non-function', "
                     "'gc-heap/objects-function' and 'object-slots' numbers.");

        REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-shapes"),
                     nsIMemoryReporter::KIND_OTHER, rtStats.totalShapes,
                     "Memory used for all shape-related data in the main "
                     "JSRuntime. This is the sum of all compartments' "
                     "'gc-heap/shapes/tree', 'gc-heap/shapes/dict', "
                     "'gc-heap/shapes/base', 'shapes-extra/tree-tables', "
                     "'shapes-extra/dict-tables', "
                     "'shapes-extra/tree-shape-kids' and "
                     "'shapes-extra/empty-shape-arrays'.");

        REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-scripts"),
                     nsIMemoryReporter::KIND_OTHER, rtStats.totalScripts,
                     "Memory used for all script-related data in the main "
                     "JSRuntime. This is the sum of all compartments' "
                     "'gc-heap/scripts' and 'script-data' numbers.");

        REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-strings"),
                     nsIMemoryReporter::KIND_OTHER, rtStats.totalStrings,
                     "Memory used for all string-related data in the main "
                     "JSRuntime. This is the sum of all compartments' "
                     "'gc-heap/strings' and 'string-chars' numbers.");

        REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-mjit"),
                     nsIMemoryReporter::KIND_OTHER, rtStats.totalMjit,
                     "Memory used by the method JIT in the main JSRuntime.  "
                     "This is the sum of all compartments' 'mjit/code', and "
                     "'mjit/data' numbers.");

        REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-type-inference"),
                     nsIMemoryReporter::KIND_OTHER, rtStats.totalTypeInference,
                     "Non-transient memory used by type inference in the main "
                     "JSRuntime. This is the sum of all compartments' "
                     "'gc-heap/type-objects', 'type-inference/script-main', "
                     "'type-inference/object-main' and "
                     "'type-inference/tables' numbers.");

        REPORT_BYTES(NS_LITERAL_CSTRING("js-main-runtime-analysis-temporary"),
                     nsIMemoryReporter::KIND_OTHER, rtStats.totalAnalysisTemp,
                     "Memory used transiently during type inference and "
                     "compilation in the main JSRuntime. This is the sum of "
                     "all compartments' 'analysis-temporary' numbers.");

        return NS_OK;
    }

    NS_IMETHOD
    GetExplicitNonHeap(PRInt64 *n)
    {
        JSRuntime *rt = nsXPConnect::GetRuntimeInstance()->GetJSRuntime();
        *reinterpret_cast<int64_t*>(n) = JS::GetExplicitNonHeapForRuntime(rt, JsMallocSizeOf);
        return NS_OK;
    }
};

NS_IMPL_THREADSAFE_ISUPPORTS1(JSMemoryMultiReporter
                              , nsIMemoryMultiReporter
                              )

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
        Telemetry::Accumulate(Telemetry::GC_REASON, sample);
        break;
      case JS_TELEMETRY_GC_IS_COMPARTMENTAL:
        Telemetry::Accumulate(Telemetry::GC_IS_COMPARTMENTAL, sample);
        break;
      case JS_TELEMETRY_GC_MS:
        Telemetry::Accumulate(Telemetry::GC_MS, sample);
        break;
      case JS_TELEMETRY_GC_MARK_MS:
        Telemetry::Accumulate(Telemetry::GC_MARK_MS, sample);
        break;
      case JS_TELEMETRY_GC_SWEEP_MS:
        Telemetry::Accumulate(Telemetry::GC_SWEEP_MS, sample);
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
    }
}

bool XPCJSRuntime::gNewDOMBindingsEnabled;
bool XPCJSRuntime::gExperimentalBindingsEnabled;

bool PreserveWrapper(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(IS_WRAPPER_CLASS(js::GetObjectClass(obj)));
    nsISupports *native = nsXPConnect::GetXPConnect()->GetNativeOfWrapper(cx, obj);
    if (!native)
        return false;
    nsresult rv;
    nsCOMPtr<nsINode> node = do_QueryInterface(native, &rv);
    if (NS_FAILED(rv))
        return false;
    nsContentUtils::PreserveWrapper(native, node);
    return true;
}

XPCJSRuntime::XPCJSRuntime(nsXPConnect* aXPConnect)
 : mXPConnect(aXPConnect),
   mJSRuntime(nsnull),
   mJSCycleCollectionContext(nsnull),
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
   mThreadRunningGC(nsnull),
   mWrappedJSToReleaseArray(),
   mNativesToReleaseArray(),
   mDoingFinalization(false),
   mVariantRoots(nsnull),
   mWrappedJSRoots(nsnull),
   mObjectHolderRoots(nsnull),
   mWatchdogLock(nsnull),
   mWatchdogWakeup(nsnull),
   mWatchdogThread(nsnull),
   mWatchdogHibernating(false),
   mLastActiveTime(-1)
{
#ifdef XPC_CHECK_WRAPPERS_AT_SHUTDOWN
    DEBUG_WrappedNativeHashtable =
        JS_NewDHashTable(JS_DHashGetStubOps(), nsnull,
                         sizeof(JSDHashEntryStub), 128);
#endif
    NS_TIME_FUNCTION;

    DOM_InitInterfaces();
    Preferences::AddBoolVarCache(&gNewDOMBindingsEnabled, "dom.new_bindings",
                                 false);
    Preferences::AddBoolVarCache(&gExperimentalBindingsEnabled,
                                 "dom.experimental_bindings",
                                 false);


    
    mStrIDs[0] = JSID_VOID;

    mJSRuntime = JS_NewRuntime(32L * 1024L * 1024L); 
    if (!mJSRuntime)
        NS_RUNTIMEABORT("JS_NewRuntime failed.");

    
    
    
    
    
    
    JS_SetGCParameter(mJSRuntime, JSGC_MAX_BYTES, 0xffffffff);
#ifdef MOZ_ASAN
    
    JS_SetNativeStackQuota(mJSRuntime, 2 * 128 * sizeof(size_t) * 1024);
#else  
    JS_SetNativeStackQuota(mJSRuntime, 128 * sizeof(size_t) * 1024);
#endif
    JS_SetContextCallback(mJSRuntime, ContextCallback);
    JS_SetDestroyCompartmentCallback(mJSRuntime, CompartmentDestroyedCallback);
    JS_SetGCCallback(mJSRuntime, GCCallback);
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
    JS_SetAccumulateTelemetryCallback(mJSRuntime, AccumulateTelemetryCallback);
    js::SetActivityCallback(mJSRuntime, ActivityCallback, this);
        
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(XPConnectJSGCHeap));
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(XPConnectJSSystemCompartmentCount));
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(XPConnectJSUserCompartmentCount));
    NS_RegisterMemoryMultiReporter(new JSMemoryMultiReporter);
    NS_RegisterMemoryMultiReporter(new JSCompartmentsMultiReporter);

    if (!JS_DHashTableInit(&mJSHolders, JS_DHashGetStubOps(), nsnull,
                           sizeof(ObjectHolder), 512))
        mJSHolders.ops = nsnull;

    mCompartmentSet.init();

    
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
        self->GetCompartmentSet().initialized() &&
        self->mWatchdogThread) {
        return self;
    }

    NS_RUNTIMEABORT("new XPCJSRuntime failed to initialize.");

    delete self;
    return nsnull;
}


bool InternStaticDictionaryJSVals(JSContext* aCx);

JSBool
XPCJSRuntime::OnJSContextNew(JSContext *cx)
{
    NS_TIME_FUNCTION;

    
    JSBool ok = true;
    if (JSID_IS_VOID(mStrIDs[0])) {
        JS_SetGCParameterForThread(cx, JSGC_MAX_CODE_CACHE_BYTES, 16 * 1024 * 1024);
        {
            
            
            JSAutoRequest ar(cx);
            for (unsigned i = 0; i < IDX_TOTAL_COUNT; i++) {
                JSString* str = JS_InternString(cx, mStrings[i]);
                if (!str || !JS_ValueToId(cx, STRING_TO_JSVAL(str), &mStrIDs[i])) {
                    mStrIDs[0] = JSID_VOID;
                    ok = false;
                    break;
                }
                mStrJSVals[i] = STRING_TO_JSVAL(str);
            }
        }

        ok = mozilla::dom::binding::DefineStaticJSVals(cx);
        if (!ok)
            return false;

        ok = InternStaticDictionaryJSVals(cx);
    }
    if (!ok)
        return false;

    XPCPerThreadData* tls = XPCPerThreadData::GetData(cx);
    if (!tls)
        return false;

    XPCContext* xpc = new XPCContext(this, cx);
    if (!xpc)
        return false;

    
    JS_ToggleOptions(cx, JSOPTION_UNROOTED_GLOBAL);

    return true;
}

JSBool
XPCJSRuntime::DeferredRelease(nsISupports* obj)
{
    NS_ASSERTION(obj, "bad param");

    if (mNativesToReleaseArray.IsEmpty()) {
        
        
        
        mNativesToReleaseArray.SetCapacity(256);
    }
    return mNativesToReleaseArray.AppendElement(obj) != nsnull;
}



#ifdef DEBUG
static JSDHashOperator
WrappedJSClassMapDumpEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                uint32_t number, void *arg)
{
    ((IID2WrappedJSClassMap::Entry*)hdr)->value->DebugDump(*(PRInt16*)arg);
    return JS_DHASH_NEXT;
}
static JSDHashOperator
WrappedJSMapDumpEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                           uint32_t number, void *arg)
{
    ((JSObject2WrappedJSMap::Entry*)hdr)->value->DebugDump(*(PRInt16*)arg);
    return JS_DHASH_NEXT;
}
static JSDHashOperator
NativeSetDumpEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                        uint32_t number, void *arg)
{
    ((NativeSetMap::Entry*)hdr)->key_value->DebugDump(*(PRInt16*)arg);
    return JS_DHASH_NEXT;
}
#endif

void
XPCJSRuntime::DebugDump(PRInt16 depth)
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
        JSContext* iter = nsnull;
        while (JS_ContextIterator(mJSRuntime, &iter))
            ++cxCount;
        XPC_LOG_ALWAYS(("%d JS context(s)", cxCount));

        iter = nsnull;
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
            mWrappedJSMap->Enumerate(WrappedJSMapDumpEnumerator, &depth);
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
    NS_ASSERTION(mSelfp, "Must be linked");

    XPCAutoLock autoLock(lock);

    NS_ASSERTION(*mSelfp == this, "Link invariant");
    *mSelfp = mNext;
    if (mNext)
        mNext->mSelfp = mSelfp;
#ifdef DEBUG
    mSelfp = nsnull;
    mNext = nsnull;
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
