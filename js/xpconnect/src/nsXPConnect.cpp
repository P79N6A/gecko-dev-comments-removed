







#include "mozilla/Assertions.h"
#include "mozilla/Base64.h"
#include "mozilla/Likely.h"
#include "mozilla/Util.h"

#include "xpcprivate.h"
#include "XPCWrapper.h"
#include "nsBaseHashtable.h"
#include "nsHashKeys.h"
#include "jsfriendapi.h"
#include "dom_quickstubs.h"
#include "nsNullPrincipal.h"
#include "nsIURI.h"
#include "nsJSEnvironment.h"
#include "nsThreadUtils.h"

#include "XrayWrapper.h"
#include "WrapperFactory.h"
#include "AccessCheck.h"

#ifdef MOZ_JSDEBUGGER
#include "jsdIDebuggerService.h"
#endif

#include "XPCQuickStubs.h"

#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/TextDecoderBinding.h"
#include "mozilla/dom/TextEncoderBinding.h"

#include "nsWrapperCacheInlines.h"
#include "nsDOMMutationObserver.h"
#include "nsICycleCollectorListener.h"
#include "nsThread.h"

using namespace mozilla::dom;
using namespace xpc;

NS_IMPL_THREADSAFE_ISUPPORTS7(nsXPConnect,
                              nsIXPConnect,
                              nsISupportsWeakReference,
                              nsIThreadObserver,
                              nsIJSRuntimeService,
                              nsIJSContextStack,
                              nsIThreadJSContextStack,
                              nsIJSEngineTelemetryStats)

nsXPConnect* nsXPConnect::gSelf = nullptr;
JSBool       nsXPConnect::gOnceAliveNowDead = false;
uint32_t     nsXPConnect::gReportAllJSExceptions = 0;
JSBool       nsXPConnect::gDebugMode = false;
JSBool       nsXPConnect::gDesiredDebugMode = false;



nsIScriptSecurityManager *nsXPConnect::gScriptSecurityManager = nullptr;

const char XPC_CONTEXT_STACK_CONTRACTID[] = "@mozilla.org/js/xpc/ContextStack;1";
const char XPC_RUNTIME_CONTRACTID[]       = "@mozilla.org/js/xpc/RuntimeService;1";
const char XPC_EXCEPTION_CONTRACTID[]     = "@mozilla.org/js/xpc/Exception;1";
const char XPC_CONSOLE_CONTRACTID[]       = "@mozilla.org/consoleservice;1";
const char XPC_SCRIPT_ERROR_CONTRACTID[]  = "@mozilla.org/scripterror;1";
const char XPC_ID_CONTRACTID[]            = "@mozilla.org/js/xpc/ID;1";
const char XPC_XPCONNECT_CONTRACTID[]     = "@mozilla.org/js/xpc/XPConnect;1";



nsXPConnect::nsXPConnect()
    :   mRuntime(nullptr),
        mInterfaceInfoManager(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID)),
        mDefaultSecurityManager(nullptr),
        mDefaultSecurityManagerFlags(0),
        mShuttingDown(false),
        mEventDepth(0)
{
    mRuntime = XPCJSRuntime::newXPCJSRuntime(this);

    nsCycleCollector_registerJSRuntime(this);

    char* reportableEnv = PR_GetEnv("MOZ_REPORT_ALL_JS_EXCEPTIONS");
    if (reportableEnv && *reportableEnv)
        gReportAllJSExceptions = 1;
}

nsXPConnect::~nsXPConnect()
{
    nsCycleCollector_forgetJSRuntime();

    JSContext *cx = nullptr;
    if (mRuntime) {
        
        
        
        cx = JS_NewContext(mRuntime->GetJSRuntime(), 8192);
    }

    
    
    mRuntime->DestroyJSContextStack();

    mShuttingDown = true;
    if (cx) {
        
        XPCWrappedNativeScope::SystemIsBeingShutDown();

        mRuntime->SystemIsBeingShutDown();
        JS_DestroyContext(cx);
    }

    NS_IF_RELEASE(mDefaultSecurityManager);

    gScriptSecurityManager = nullptr;

    
    XPC_LOG_FINISH();

    delete mRuntime;

    gSelf = nullptr;
    gOnceAliveNowDead = true;
}


nsXPConnect*
nsXPConnect::GetXPConnect()
{
    
    
    
    if (!MOZ_LIKELY(NS_IsMainThread() || NS_IsCycleCollectorThread()))
        MOZ_CRASH();

    if (!gSelf) {
        if (gOnceAliveNowDead)
            return nullptr;
        gSelf = new nsXPConnect();
        if (!gSelf)
            return nullptr;

        if (!gSelf->mRuntime) {
            NS_RUNTIMEABORT("Couldn't create XPCJSRuntime.");
        }
        if (!gSelf->mInterfaceInfoManager) {
            NS_RUNTIMEABORT("Couldn't get global interface info manager.");
        }

        
        
        NS_ADDREF(gSelf);

        
        
        
        
        MOZ_ASSERT(NS_IsMainThread());
        if (NS_FAILED(nsThread::SetMainThreadObserver(gSelf))) {
            NS_RELEASE(gSelf);
            
        }
    }
    return gSelf;
}


nsXPConnect*
nsXPConnect::GetSingleton()
{
    nsXPConnect* xpc = nsXPConnect::GetXPConnect();
    NS_IF_ADDREF(xpc);
    return xpc;
}


void
nsXPConnect::ReleaseXPConnectSingleton()
{
    nsXPConnect* xpc = gSelf;
    if (xpc) {
        nsThread::SetMainThreadObserver(nullptr);

#ifdef DEBUG
        
        
        {
            const char* dumpName = getenv("XPC_SHUTDOWN_HEAP_DUMP");
            if (dumpName) {
                FILE* dumpFile = (*dumpName == '\0' ||
                                  strcmp(dumpName, "stdout") == 0)
                                 ? stdout
                                 : fopen(dumpName, "w");
                if (dumpFile) {
                    JS_DumpHeap(xpc->GetRuntime()->GetJSRuntime(), dumpFile, nullptr,
                                JSTRACE_OBJECT, nullptr, static_cast<size_t>(-1), nullptr);
                    if (dumpFile != stdout)
                        fclose(dumpFile);
                }
            }
        }
#endif
#ifdef XPC_DUMP_AT_SHUTDOWN
        
        
        xpc->DebugDump(7);
#endif
        nsrefcnt cnt;
        NS_RELEASE2(xpc, cnt);
#ifdef XPC_DUMP_AT_SHUTDOWN
        if (0 != cnt)
            printf("*** dangling reference to nsXPConnect: refcnt=%d\n", cnt);
        else
            printf("+++ XPConnect had no dangling references.\n");
#endif
    }
}


nsresult
nsXPConnect::GetInterfaceInfoManager(nsIInterfaceInfoSuperManager** iim,
                                     nsXPConnect* xpc )
{
    if (!xpc && !(xpc = GetXPConnect()))
        return NS_ERROR_FAILURE;

    *iim = xpc->mInterfaceInfoManager;
    NS_IF_ADDREF(*iim);
    return NS_OK;
}


XPCJSRuntime*
nsXPConnect::GetRuntimeInstance()
{
    nsXPConnect* xpc = GetXPConnect();
    NS_ASSERTION(xpc, "Must not be called if XPC failed to initialize");
    return xpc->GetRuntime();
}


JSBool
nsXPConnect::IsISupportsDescendant(nsIInterfaceInfo* info)
{
    bool found = false;
    if (info)
        info->HasAncestor(&NS_GET_IID(nsISupports), &found);
    return found;
}



typedef bool (*InfoTester)(nsIInterfaceInfoManager* manager, const void* data,
                           nsIInterfaceInfo** info);

static bool IIDTester(nsIInterfaceInfoManager* manager, const void* data,
                      nsIInterfaceInfo** info)
{
    return NS_SUCCEEDED(manager->GetInfoForIID((const nsIID *) data, info)) &&
           *info;
}

static bool NameTester(nsIInterfaceInfoManager* manager, const void* data,
                       nsIInterfaceInfo** info)
{
    return NS_SUCCEEDED(manager->GetInfoForName((const char *) data, info)) &&
           *info;
}

static nsresult FindInfo(InfoTester tester, const void* data,
                         nsIInterfaceInfoSuperManager* iism,
                         nsIInterfaceInfo** info)
{
    if (tester(iism, data, info))
        return NS_OK;

    

    bool yes;
    nsCOMPtr<nsISimpleEnumerator> list;

    if (NS_SUCCEEDED(iism->HasAdditionalManagers(&yes)) && yes &&
        NS_SUCCEEDED(iism->EnumerateAdditionalManagers(getter_AddRefs(list))) &&
        list) {
        bool more;
        nsCOMPtr<nsIInterfaceInfoManager> current;

        while (NS_SUCCEEDED(list->HasMoreElements(&more)) && more &&
               NS_SUCCEEDED(list->GetNext(getter_AddRefs(current))) && current) {
            if (tester(current, data, info))
                return NS_OK;
        }
    }

    return NS_ERROR_NO_INTERFACE;
}

nsresult
nsXPConnect::GetInfoForIID(const nsIID * aIID, nsIInterfaceInfo** info)
{
    return FindInfo(IIDTester, aIID, mInterfaceInfoManager, info);
}

nsresult
nsXPConnect::GetInfoForName(const char * name, nsIInterfaceInfo** info)
{
    return FindInfo(NameTester, name, mInterfaceInfoManager, info);
}

bool
nsXPConnect::NeedCollect()
{
    return !js::AreGCGrayBitsValid(GetRuntime()->GetJSRuntime());
}

void
nsXPConnect::Collect(uint32_t reason)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    MOZ_ASSERT(reason < JS::gcreason::NUM_REASONS);
    JS::gcreason::Reason gcreason = (JS::gcreason::Reason)reason;

    JSRuntime *rt = GetRuntime()->GetJSRuntime();
    JS::PrepareForFullGC(rt);
    JS::GCForReason(rt, gcreason);
}

NS_IMETHODIMP
nsXPConnect::GarbageCollect(uint32_t reason)
{
    Collect(reason);
    return NS_OK;
}

struct NoteWeakMapChildrenTracer : public JSTracer
{
    NoteWeakMapChildrenTracer(nsCycleCollectionTraversalCallback &cb)
        : mCb(cb)
    {
    }
    nsCycleCollectionTraversalCallback &mCb;
    bool mTracedAny;
    JSObject *mMap;
    void *mKey;
    void *mKeyDelegate;
};

static void
TraceWeakMappingChild(JSTracer *trc, void **thingp, JSGCTraceKind kind)
{
    MOZ_ASSERT(trc->callback == TraceWeakMappingChild);
    void *thing = *thingp;
    NoteWeakMapChildrenTracer *tracer =
        static_cast<NoteWeakMapChildrenTracer *>(trc);
    if (kind == JSTRACE_STRING)
        return;
    if (!xpc_IsGrayGCThing(thing) && !tracer->mCb.WantAllTraces())
        return;
    if (AddToCCKind(kind)) {
        tracer->mCb.NoteWeakMapping(tracer->mMap, tracer->mKey, tracer->mKeyDelegate, thing);
        tracer->mTracedAny = true;
    } else {
        JS_TraceChildren(trc, thing, kind);
    }
}

struct NoteWeakMapsTracer : public js::WeakMapTracer
{
    NoteWeakMapsTracer(JSRuntime *rt, js::WeakMapTraceCallback cb,
                       nsCycleCollectionTraversalCallback &cccb)
      : js::WeakMapTracer(rt, cb), mCb(cccb), mChildTracer(cccb)
    {
        JS_TracerInit(&mChildTracer, rt, TraceWeakMappingChild);
    }
    nsCycleCollectionTraversalCallback &mCb;
    NoteWeakMapChildrenTracer mChildTracer;
};

static void
TraceWeakMapping(js::WeakMapTracer *trc, JSObject *m,
                 void *k, JSGCTraceKind kkind,
                 void *v, JSGCTraceKind vkind)
{
    MOZ_ASSERT(trc->callback == TraceWeakMapping);
    NoteWeakMapsTracer *tracer = static_cast<NoteWeakMapsTracer *>(trc);

    
    if ((!k || !xpc_IsGrayGCThing(k)) && MOZ_LIKELY(!tracer->mCb.WantAllTraces())) {
        if (!v || !xpc_IsGrayGCThing(v) || vkind == JSTRACE_STRING)
            return;
    }

    
    
    
    
    MOZ_ASSERT(AddToCCKind(kkind));

    
    
    
    
    if (!AddToCCKind(kkind))
        k = nullptr;

    JSObject *kdelegate = nullptr;
    if (k && kkind == JSTRACE_OBJECT)
        kdelegate = js::GetWeakmapKeyDelegate((JSObject *)k);

    if (AddToCCKind(vkind)) {
        tracer->mCb.NoteWeakMapping(m, k, kdelegate, v);
    } else {
        tracer->mChildTracer.mTracedAny = false;
        tracer->mChildTracer.mMap = m;
        tracer->mChildTracer.mKey = k;
        tracer->mChildTracer.mKeyDelegate = kdelegate;

        if (v && vkind != JSTRACE_STRING)
            JS_TraceChildren(&tracer->mChildTracer, v, vkind);

        
        
        if (!tracer->mChildTracer.mTracedAny && k && xpc_IsGrayGCThing(k) && kdelegate)
            tracer->mCb.NoteWeakMapping(m, k, kdelegate, nullptr);
    }
}


struct FixWeakMappingGrayBitsTracer : public js::WeakMapTracer
{
    FixWeakMappingGrayBitsTracer(JSRuntime *rt)
        : js::WeakMapTracer(rt, FixWeakMappingGrayBits)
    {}

    void
    FixAll()
    {
        do {
            mAnyMarked = false;
            js::TraceWeakMaps(this);
        } while (mAnyMarked);
    }

private:

    static void
    FixWeakMappingGrayBits(js::WeakMapTracer *trc, JSObject *m,
                           void *k, JSGCTraceKind kkind,
                           void *v, JSGCTraceKind vkind)
    {
        MOZ_ASSERT(!JS::IsIncrementalGCInProgress(trc->runtime),
                   "Don't call FixWeakMappingGrayBits during a GC.");

        FixWeakMappingGrayBitsTracer *tracer = static_cast<FixWeakMappingGrayBitsTracer*>(trc);

        
        bool delegateMightNeedMarking = k && xpc_IsGrayGCThing(k);
        bool valueMightNeedMarking = v && xpc_IsGrayGCThing(v) && vkind != JSTRACE_STRING;
        if (!delegateMightNeedMarking && !valueMightNeedMarking)
            return;

        if (!AddToCCKind(kkind))
            k = nullptr;

        if (delegateMightNeedMarking && kkind == JSTRACE_OBJECT) {
            JSObject *kdelegate = js::GetWeakmapKeyDelegate((JSObject *)k);
            if (kdelegate && !xpc_IsGrayGCThing(kdelegate)) {
                JS::UnmarkGrayGCThingRecursively(k, JSTRACE_OBJECT);
                tracer->mAnyMarked = true;
            }
        }

        if (v && xpc_IsGrayGCThing(v) &&
            (!k || !xpc_IsGrayGCThing(k)) &&
            (!m || !xpc_IsGrayGCThing(m)) &&
            vkind != JSTRACE_SHAPE)
        {
            JS::UnmarkGrayGCThingRecursively(v, vkind);
            tracer->mAnyMarked = true;
        }

    }

    bool mAnyMarked;
};

void
nsXPConnect::FixWeakMappingGrayBits()
{
    FixWeakMappingGrayBitsTracer fixer(GetRuntime()->GetJSRuntime());
    fixer.FixAll();
}

nsresult
nsXPConnect::BeginCycleCollection(nsCycleCollectionTraversalCallback &cb)
{
    JSRuntime* rt = GetRuntime()->GetJSRuntime();
    static bool gcHasRun = false;
    if (!gcHasRun) {
        uint32_t gcNumber = JS_GetGCParameter(rt, JSGC_NUMBER);
        if (!gcNumber)
            NS_RUNTIMEABORT("Cannot cycle collect if GC has not run first!");
        gcHasRun = true;
    }

    GetRuntime()->AddXPConnectRoots(cb);

    NoteWeakMapsTracer trc(rt, TraceWeakMapping, cb);
    js::TraceWeakMaps(&trc);

    return NS_OK;
}

bool
nsXPConnect::NotifyLeaveMainThread()
{
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "Off main thread");
    JSRuntime *rt = mRuntime->GetJSRuntime();
    if (JS_IsInRequest(rt))
        return false;
    JS_ClearRuntimeThread(rt);
    return true;
}

void
nsXPConnect::NotifyEnterCycleCollectionThread()
{
    NS_ABORT_IF_FALSE(!NS_IsMainThread(), "On main thread");
    JS_SetRuntimeThread(mRuntime->GetJSRuntime());
}

void
nsXPConnect::NotifyLeaveCycleCollectionThread()
{
    NS_ABORT_IF_FALSE(!NS_IsMainThread(), "On main thread");
    JS_ClearRuntimeThread(mRuntime->GetJSRuntime());
}

void
nsXPConnect::NotifyEnterMainThread()
{
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "Off main thread");
    JS_SetRuntimeThread(mRuntime->GetJSRuntime());
}

class nsXPConnectParticipant: public nsCycleCollectionParticipant
{
public:
    static NS_METHOD RootImpl(void *n)
    {
        return NS_OK;
    }
    static NS_METHOD UnlinkImpl(void *n)
    {
        return NS_OK;
    }
    static NS_METHOD UnrootImpl(void *n)
    {
        return NS_OK;
    }
    static NS_METHOD_(void) UnmarkIfPurpleImpl(void *n)
    {
    }
    static NS_METHOD TraverseImpl(nsXPConnectParticipant *that, void *n,
                                  nsCycleCollectionTraversalCallback &cb);
};

static const CCParticipantVTable<nsXPConnectParticipant>::Type
XPConnect_cycleCollectorGlobal =
{
  NS_IMPL_CYCLE_COLLECTION_NATIVE_VTABLE(nsXPConnectParticipant)
};

nsCycleCollectionParticipant *
nsXPConnect::GetParticipant()
{
    return XPConnect_cycleCollectorGlobal.GetParticipant();
}

JSBool
xpc_GCThingIsGrayCCThing(void *thing)
{
    return AddToCCKind(js::GCThingTraceKind(thing)) &&
           xpc_IsGrayGCThing(thing);
}

struct TraversalTracer : public JSTracer
{
    TraversalTracer(nsCycleCollectionTraversalCallback &aCb) : cb(aCb)
    {
    }
    nsCycleCollectionTraversalCallback &cb;
};

static void
NoteJSChild(JSTracer *trc, void *thing, JSGCTraceKind kind)
{
    TraversalTracer *tracer = static_cast<TraversalTracer*>(trc);

    
    if (!xpc_IsGrayGCThing(thing) && !tracer->cb.WantAllTraces())
        return;

    







    if (AddToCCKind(kind)) {
        if (MOZ_UNLIKELY(tracer->cb.WantDebugInfo())) {
            
            if (tracer->debugPrinter) {
                char buffer[200];
                tracer->debugPrinter(trc, buffer, sizeof(buffer));
                tracer->cb.NoteNextEdgeName(buffer);
            } else if (tracer->debugPrintIndex != (size_t)-1) {
                char buffer[200];
                JS_snprintf(buffer, sizeof(buffer), "%s[%lu]",
                            static_cast<const char *>(tracer->debugPrintArg),
                            tracer->debugPrintIndex);
                tracer->cb.NoteNextEdgeName(buffer);
            } else {
                tracer->cb.NoteNextEdgeName(static_cast<const char*>(tracer->debugPrintArg));
            }
        }
        tracer->cb.NoteJSChild(thing);
    } else if (kind == JSTRACE_SHAPE) {
        JS_TraceShapeCycleCollectorChildren(trc, thing);
    } else if (kind != JSTRACE_STRING) {
        JS_TraceChildren(trc, thing, kind);
    }
}

void
xpc_MarkInCCGeneration(nsISupports* aVariant, uint32_t aGeneration)
{
    nsCOMPtr<XPCVariant> variant = do_QueryInterface(aVariant);
    if (variant) {
        variant->SetCCGeneration(aGeneration);
        variant->GetJSVal(); 
        XPCVariant* weak = variant.get();
        variant = nullptr;
        if (weak->IsPurple()) {
          weak->RemovePurple();
        }
    }
}

void
xpc_TryUnmarkWrappedGrayObject(nsISupports* aWrappedJS)
{
    nsCOMPtr<nsIXPConnectWrappedJS> wjs = do_QueryInterface(aWrappedJS);
    if (wjs) {
        
        static_cast<nsXPCWrappedJS*>(wjs.get())->GetJSObject();
    }
}

static inline void
DescribeGCThing(bool isMarked, void *p, JSGCTraceKind traceKind,
                nsCycleCollectionTraversalCallback &cb)
{
    if (cb.WantDebugInfo()) {
        char name[72];
        if (traceKind == JSTRACE_OBJECT) {
            JSObject *obj = static_cast<JSObject*>(p);
            js::Class *clasp = js::GetObjectClass(obj);
            XPCNativeScriptableInfo *si = nullptr;

            if (IS_PROTO_CLASS(clasp)) {
                XPCWrappedNativeProto *p =
                    static_cast<XPCWrappedNativeProto*>(xpc_GetJSPrivate(obj));
                si = p->GetScriptableInfo();
            }
            if (si) {
                JS_snprintf(name, sizeof(name), "JS Object (%s - %s)",
                            clasp->name, si->GetJSClass()->name);
            } else if (clasp == &js::FunctionClass) {
                JSFunction *fun = JS_GetObjectFunction(obj);
                JSString *str = JS_GetFunctionDisplayId(fun);
                if (str) {
                    NS_ConvertUTF16toUTF8 fname(JS_GetInternedStringChars(str));
                    JS_snprintf(name, sizeof(name),
                                "JS Object (Function - %s)", fname.get());
                } else {
                    JS_snprintf(name, sizeof(name), "JS Object (Function)");
                }
            } else {
                JS_snprintf(name, sizeof(name), "JS Object (%s)",
                            clasp->name);
            }
        } else {
            static const char trace_types[][11] = {
                "Object",
                "String",
                "Script",
                "IonCode",
                "Shape",
                "BaseShape",
                "TypeObject",
            };
            JS_STATIC_ASSERT(NS_ARRAY_LENGTH(trace_types) == JSTRACE_LAST + 1);
            JS_snprintf(name, sizeof(name), "JS %s", trace_types[traceKind]);
        }

        
        cb.DescribeGCedNode(isMarked, name);
    } else {
        cb.DescribeGCedNode(isMarked, "JS Object");
    }
}

static void
NoteJSChildTracerShim(JSTracer *trc, void **thingp, JSGCTraceKind kind)
{
    NoteJSChild(trc, *thingp, kind);
}

static inline void
NoteGCThingJSChildren(JSRuntime *rt, void *p, JSGCTraceKind traceKind,
                      nsCycleCollectionTraversalCallback &cb)
{
    MOZ_ASSERT(rt);
    TraversalTracer trc(cb);
    JS_TracerInit(&trc, rt, NoteJSChildTracerShim);
    trc.eagerlyTraceWeakMaps = DoNotTraceWeakMaps;
    JS_TraceChildren(&trc, p, traceKind);
}

static inline void
NoteGCThingXPCOMChildren(js::Class *clasp, JSObject *obj,
                         nsCycleCollectionTraversalCallback &cb)
{
    MOZ_ASSERT(clasp);
    MOZ_ASSERT(clasp == js::GetObjectClass(obj));

    if (clasp == &XPC_WN_Tearoff_JSClass) {
        
        
        
        XPCWrappedNativeTearOff *to =
            static_cast<XPCWrappedNativeTearOff*>(xpc_GetJSPrivate(obj));
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "xpc_GetJSPrivate(obj)->mNative");
        cb.NoteXPCOMChild(to->GetNative());
    }
    
    
    else if (clasp->flags & JSCLASS_HAS_PRIVATE &&
             clasp->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS) {
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "xpc_GetJSPrivate(obj)");
        cb.NoteXPCOMChild(static_cast<nsISupports*>(xpc_GetJSPrivate(obj)));
    } else {
        const DOMClass* domClass = GetDOMClass(obj);
        if (domClass) {
            NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "UnwrapDOMObject(obj)");
            if (domClass->mDOMObjectIsISupports) {
                cb.NoteXPCOMChild(UnwrapDOMObject<nsISupports>(obj));
            } else if (domClass->mParticipant) {
                cb.NoteNativeChild(UnwrapDOMObject<void>(obj),
                                   domClass->mParticipant);
            }
        }
    }
}

enum TraverseSelect {
    TRAVERSE_CPP,
    TRAVERSE_FULL
};

static void
TraverseGCThing(TraverseSelect ts, void *p, JSGCTraceKind traceKind,
                nsCycleCollectionTraversalCallback &cb)
{
    MOZ_ASSERT(traceKind == js::GCThingTraceKind(p));
    bool isMarkedGray = xpc_IsGrayGCThing(p);

    if (ts == TRAVERSE_FULL)
        DescribeGCThing(!isMarkedGray, p, traceKind, cb);

    
    
    
    
    if (!isMarkedGray && !cb.WantAllTraces())
        return;

    if (ts == TRAVERSE_FULL)
        NoteGCThingJSChildren(nsXPConnect::GetRuntimeInstance()->GetJSRuntime(),
                              p, traceKind, cb);
 
    if (traceKind == JSTRACE_OBJECT) {
        JSObject *obj = static_cast<JSObject*>(p);
        NoteGCThingXPCOMChildren(js::GetObjectClass(obj), obj, cb);
    }
}

NS_METHOD
nsXPConnectParticipant::TraverseImpl(nsXPConnectParticipant *that, void *p,
                                     nsCycleCollectionTraversalCallback &cb)
{
    TraverseGCThing(TRAVERSE_FULL, p, js::GCThingTraceKind(p), cb);
    return NS_OK;
}

class JSContextParticipant : public nsCycleCollectionParticipant
{
public:
    static NS_METHOD RootImpl(void *n)
    {
        return NS_OK;
    }
    static NS_METHOD UnlinkImpl(void *n)
    {
        return NS_OK;
    }
    static NS_METHOD UnrootImpl(void *n)
    {
        return NS_OK;
    }
    static NS_METHOD_(void) UnmarkIfPurpleImpl(void *n)
    {
    }
    static NS_METHOD TraverseImpl(JSContextParticipant *that, void *n,
                                  nsCycleCollectionTraversalCallback &cb)
    {
        JSContext *cx = static_cast<JSContext*>(n);

        
        
        
        
        
        unsigned refCount = js::ContextHasOutstandingRequests(cx) ? 2 : 1;

        cb.DescribeRefCountedNode(refCount, "JSContext");
        if (JSObject *global = JS_GetGlobalObject(cx)) {
            NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[global object]");
            cb.NoteJSChild(global);
        }

        return NS_OK;
    }
};

static const CCParticipantVTable<JSContextParticipant>::Type
JSContext_cycleCollectorGlobal =
{
  NS_IMPL_CYCLE_COLLECTION_NATIVE_VTABLE(JSContextParticipant)
};


nsCycleCollectionParticipant*
nsXPConnect::JSContextParticipant()
{
    return JSContext_cycleCollectorGlobal.GetParticipant();
}

NS_IMETHODIMP_(void)
nsXPConnect::NoteJSContext(JSContext *aJSContext,
                           nsCycleCollectionTraversalCallback &aCb)
{
    aCb.NoteNativeChild(aJSContext, JSContext_cycleCollectorGlobal.GetParticipant());
}






inline nsresult UnexpectedFailure(nsresult rv)
{
    NS_ERROR("This is not supposed to fail!");
    return rv;
}


NS_IMETHODIMP
nsXPConnect::InitClasses(JSContext * aJSContext, JSObject * aGlobalJSObj)
{
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aGlobalJSObj, "bad param");

    
    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if (!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    JSAutoCompartment ac(ccx, aGlobalJSObj);

    XPCWrappedNativeScope* scope =
        XPCWrappedNativeScope::GetNewOrUsed(ccx, aGlobalJSObj);

    if (!scope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    scope->RemoveWrappedNativeProtos();

    if (!XPCNativeWrapper::AttachNewConstructorObject(ccx, aGlobalJSObj))
        return UnexpectedFailure(NS_ERROR_FAILURE);

    return NS_OK;
}

#ifdef DEBUG
struct VerifyTraceXPCGlobalCalledTracer
{
    JSTracer base;
    bool ok;
};

static void
VerifyTraceXPCGlobalCalled(JSTracer *trc, void **thingp, JSGCTraceKind kind)
{
    
    
}
#endif

void
TraceXPCGlobal(JSTracer *trc, JSObject *obj)
{
#ifdef DEBUG
    if (trc->callback == VerifyTraceXPCGlobalCalled) {
        
        
        reinterpret_cast<VerifyTraceXPCGlobalCalledTracer*>(trc)->ok = true;
        return;
    }
#endif

    if (js::GetObjectClass(obj)->flags & JSCLASS_DOM_GLOBAL)
        mozilla::dom::TraceProtoAndIfaceCache(trc, obj);
}

#ifdef DEBUG
#include "mozilla/Preferences.h"
#include "nsIXULRuntime.h"
static void
CheckTypeInference(JSContext *cx, JSClass *clasp, nsIPrincipal *principal)
{
    
    if (strcmp(clasp->name, "Sandbox") ||
        strcmp(clasp->name, "nsXBLPrototypeScript compilation scope") ||
        strcmp(clasp->name, "nsXULPrototypeScript compilation scope"))
        return;

    
    if (!mozilla::Preferences::GetBool("javascript.options.typeinference"))
        return;

    
    bool isSystem;
    nsIScriptSecurityManager* ssm;
    ssm = XPCWrapper::GetSecurityManager();
    if (NS_FAILED(ssm->IsSystemPrincipal(principal, &isSystem)) || !isSystem)
        return;

    
    bool safeMode;
    nsCOMPtr<nsIXULRuntime> xr = do_GetService("@mozilla.org/xre/runtime;1");
    if (!xr) {
        NS_WARNING("Couldn't get XUL runtime!");
        return;
    }
    if (NS_FAILED(xr->GetInSafeMode(&safeMode)) || safeMode)
        return;

    
    MOZ_ASSERT(JS_GetOptions(cx) & JSOPTION_TYPE_INFERENCE);
}
#else
#define CheckTypeInference(cx, clasp, principal) {}
#endif

namespace xpc {

JSObject*
CreateGlobalObject(JSContext *cx, JSClass *clasp, nsIPrincipal *principal,
                   JS::ZoneSpecifier zoneSpec)
{
    
    
    CheckTypeInference(cx, clasp, principal);

    NS_ABORT_IF_FALSE(NS_IsMainThread(), "using a principal off the main thread?");
    MOZ_ASSERT(principal);

    JSObject *global = JS_NewGlobalObject(cx, clasp, nsJSPrincipals::get(principal), zoneSpec);
    if (!global)
        return nullptr;
    JSAutoCompartment ac(cx, global);
    
    
    (void) new XPCWrappedNativeScope(cx, global);

#ifdef DEBUG
    
    
    
    if (!((js::Class*)clasp)->ext.isWrappedNative)
    {
        VerifyTraceXPCGlobalCalledTracer trc;
        JS_TracerInit(&trc.base, JS_GetRuntime(cx), VerifyTraceXPCGlobalCalled);
        trc.ok = false;
        JS_TraceChildren(&trc.base, global, JSTRACE_OBJECT);
        NS_ABORT_IF_FALSE(trc.ok, "Trace hook on global needs to call TraceXPCGlobal for XPConnect compartments.");
    }
#endif

    if (clasp->flags & JSCLASS_DOM_GLOBAL) {
        AllocateProtoAndIfaceCache(global);
    }

    return global;
}

} 

NS_IMETHODIMP
nsXPConnect::InitClassesWithNewWrappedGlobal(JSContext * aJSContext,
                                             nsISupports *aCOMObj,
                                             nsIPrincipal * aPrincipal,
                                             uint32_t aFlags,
                                             JS::ZoneSpecifier zoneSpec,
                                             nsIXPConnectJSObjectHolder **_retval)
{
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aCOMObj, "bad param");
    NS_ASSERTION(_retval, "bad param");

    
    
    MOZ_ASSERT(aPrincipal);

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);

    
    
    xpcObjectHelper helper(aCOMObj);
    MOZ_ASSERT(helper.GetScriptableFlags() & nsIXPCScriptable::IS_GLOBAL_OBJECT);
    nsRefPtr<XPCWrappedNative> wrappedGlobal;
    nsresult rv =
        XPCWrappedNative::WrapNewGlobal(ccx, helper, aPrincipal,
                                        aFlags & nsIXPConnect::INIT_JS_STANDARD_CLASSES,
                                        zoneSpec,
                                        getter_AddRefs(wrappedGlobal));
    NS_ENSURE_SUCCESS(rv, rv);

    
    JSObject *global = wrappedGlobal->GetFlatJSObject();
    MOZ_ASSERT(!js::GetObjectParent(global));
    JSAutoCompartment ac(ccx, global);

    if (!(aFlags & nsIXPConnect::OMIT_COMPONENTS_OBJECT)) {
        
        if (!nsXPCComponents::AttachComponentsObject(ccx, wrappedGlobal->GetScope()))
            return UnexpectedFailure(NS_ERROR_FAILURE);

        if (!XPCNativeWrapper::AttachNewConstructorObject(ccx, global))
            return UnexpectedFailure(NS_ERROR_FAILURE);
    }

    
    
    
    MOZ_ASSERT(js::GetObjectClass(global)->flags & JSCLASS_DOM_GLOBAL);

    
    if (!TextDecoderBinding::GetConstructorObject(aJSContext, global) ||
        !TextEncoderBinding::GetConstructorObject(aJSContext, global)) {
        return UnexpectedFailure(NS_ERROR_FAILURE);
    }

    wrappedGlobal.forget(_retval);
    return NS_OK;
}

nsresult
xpc_MorphSlimWrapper(JSContext *cx, nsISupports *tomorph)
{
    nsWrapperCache *cache;
    CallQueryInterface(tomorph, &cache);
    if (!cache)
        return NS_OK;

    JSObject *obj = cache->GetWrapper();
    if (!obj || !IS_SLIM_WRAPPER(obj))
        return NS_OK;
    NS_ENSURE_STATE(MorphSlimWrapper(cx, obj));
    return NS_OK;
}

static nsresult
NativeInterface2JSObject(XPCLazyCallContext & lccx,
                         JSObject * aScope,
                         nsISupports *aCOMObj,
                         nsWrapperCache *aCache,
                         const nsIID * aIID,
                         bool aAllowWrapping,
                         jsval *aVal,
                         nsIXPConnectJSObjectHolder **aHolder)
{
    JSAutoCompartment ac(lccx.GetJSContext(), aScope);
    lccx.SetScopeForNewJSObjects(aScope);

    nsresult rv;
    xpcObjectHelper helper(aCOMObj, aCache);
    if (!XPCConvert::NativeInterface2JSObject(lccx, aVal, aHolder, helper, aIID,
                                              nullptr, aAllowWrapping, &rv))
        return rv;

#ifdef DEBUG
    NS_ASSERTION(aAllowWrapping ||
                 !xpc::WrapperFactory::IsXrayWrapper(JSVAL_TO_OBJECT(*aVal)),
                 "Shouldn't be returning a xray wrapper here");
#endif

    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::WrapNative(JSContext * aJSContext,
                        JSObject * aScope,
                        nsISupports *aCOMObj,
                        const nsIID & aIID,
                        nsIXPConnectJSObjectHolder **aHolder)
{
    NS_ASSERTION(aHolder, "bad param");
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aScope, "bad param");
    NS_ASSERTION(aCOMObj, "bad param");

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if (!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);
    XPCLazyCallContext lccx(ccx);

    jsval v;
    return NativeInterface2JSObject(lccx, aScope, aCOMObj, nullptr, &aIID,
                                    false, &v, aHolder);
}


NS_IMETHODIMP
nsXPConnect::WrapNativeToJSVal(JSContext * aJSContext,
                               JSObject * aScope,
                               nsISupports *aCOMObj,
                               nsWrapperCache *aCache,
                               const nsIID * aIID,
                               bool aAllowWrapping,
                               jsval *aVal,
                               nsIXPConnectJSObjectHolder **aHolder)
{
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aScope, "bad param");
    NS_ASSERTION(aCOMObj, "bad param");

    if (aHolder)
        *aHolder = nullptr;

    XPCLazyCallContext lccx(NATIVE_CALLER, aJSContext);

    return NativeInterface2JSObject(lccx, aScope, aCOMObj, aCache, aIID,
                                    aAllowWrapping, aVal, aHolder);
}


NS_IMETHODIMP
nsXPConnect::WrapJS(JSContext * aJSContext,
                    JSObject * aJSObj,
                    const nsIID & aIID,
                    void * *result)
{
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aJSObj, "bad param");
    NS_ASSERTION(result, "bad param");

    *result = nullptr;

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if (!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    JSAutoCompartment ac(ccx, aJSObj);

    nsresult rv = NS_ERROR_UNEXPECTED;
    if (!XPCConvert::JSObject2NativeInterface(ccx, result, aJSObj,
                                              &aIID, nullptr, &rv))
        return rv;
    return NS_OK;
}

NS_IMETHODIMP
nsXPConnect::JSValToVariant(JSContext *cx,
                            jsval *aJSVal,
                            nsIVariant ** aResult)
{
    NS_PRECONDITION(aJSVal, "bad param");
    NS_PRECONDITION(aResult, "bad param");
    *aResult = nullptr;

    XPCCallContext ccx(NATIVE_CALLER, cx);
    if (!ccx.IsValid())
      return NS_ERROR_FAILURE;

    *aResult = XPCVariant::newVariant(ccx, *aJSVal);
    NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);

    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::WrapJSAggregatedToNative(nsISupports *aOuter,
                                      JSContext * aJSContext,
                                      JSObject * aJSObj,
                                      const nsIID & aIID,
                                      void * *result)
{
    NS_ASSERTION(aOuter, "bad param");
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aJSObj, "bad param");
    NS_ASSERTION(result, "bad param");

    *result = nullptr;

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if (!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    nsresult rv;
    if (!XPCConvert::JSObject2NativeInterface(ccx, result, aJSObj,
                                              &aIID, aOuter, &rv))
        return rv;
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::GetWrappedNativeOfJSObject(JSContext * aJSContext,
                                        JSObject * aJSObj,
                                        nsIXPConnectWrappedNative **_retval)
{
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aJSObj, "bad param");
    NS_ASSERTION(_retval, "bad param");

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if (!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    SLIM_LOG_WILL_MORPH(aJSContext, aJSObj);
    nsIXPConnectWrappedNative* wrapper =
        XPCWrappedNative::GetAndMorphWrappedNativeOfJSObject(aJSContext, aJSObj);
    if (wrapper) {
        NS_ADDREF(wrapper);
        *_retval = wrapper;
        return NS_OK;
    }

    
    *_retval = nullptr;
    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP_(nsISupports*)
nsXPConnect::GetNativeOfWrapper(JSContext * aJSContext,
                                JSObject * aJSObj)
{
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aJSObj, "bad param");

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if (!ccx.IsValid()) {
        UnexpectedFailure(NS_ERROR_FAILURE);
        return nullptr;
    }

    aJSObj = js::UnwrapObjectChecked(aJSObj,  false);
    if (!aJSObj) {
        JS_ReportError(aJSContext, "Permission denied to get native of security wrapper");
        return nullptr;
    }
    if (IS_WRAPPER_CLASS(js::GetObjectClass(aJSObj))) {
        if (IS_SLIM_WRAPPER_OBJECT(aJSObj))
            return (nsISupports*)xpc_GetJSPrivate(aJSObj);
        else if (XPCWrappedNative *wn = XPCWrappedNative::Get(aJSObj))
            return wn->Native();
        return nullptr;
    }

    nsISupports* supports = nullptr;
    mozilla::dom::UnwrapDOMObjectToISupports(aJSObj, supports);
    nsCOMPtr<nsISupports> canonical = do_QueryInterface(supports);
    return canonical;
}


NS_IMETHODIMP
nsXPConnect::GetWrappedNativeOfNativeObject(JSContext * aJSContext,
                                            JSObject * aScope,
                                            nsISupports *aCOMObj,
                                            const nsIID & aIID,
                                            nsIXPConnectWrappedNative **_retval)
{
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aScope, "bad param");
    NS_ASSERTION(aCOMObj, "bad param");
    NS_ASSERTION(_retval, "bad param");

    *_retval = nullptr;

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if (!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCWrappedNativeScope* scope = GetObjectScope(aScope);
    if (!scope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    AutoMarkingNativeInterfacePtr iface(ccx);
    iface = XPCNativeInterface::GetNewOrUsed(ccx, &aIID);
    if (!iface)
        return NS_ERROR_FAILURE;

    XPCWrappedNative* wrapper;

    nsresult rv = XPCWrappedNative::GetUsedOnly(ccx, aCOMObj, scope, iface,
                                                &wrapper);
    if (NS_FAILED(rv))
        return NS_ERROR_FAILURE;
    *_retval = static_cast<nsIXPConnectWrappedNative*>(wrapper);
    return NS_OK;
}





NS_IMETHODIMP
nsXPConnect::ReparentWrappedNativeIfFound(JSContext * aJSContext,
                                          JSObject * aScope,
                                          JSObject * aNewParent,
                                          nsISupports *aCOMObj)
{
    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if (!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCWrappedNativeScope* scope = GetObjectScope(aScope);
    XPCWrappedNativeScope* scope2 = GetObjectScope(aNewParent);
    if (!scope || !scope2)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    return XPCWrappedNative::
        ReparentWrapperIfFound(ccx, scope, scope2, aNewParent,
                               aCOMObj);
}

static JSDHashOperator
MoveableWrapperFinder(JSDHashTable *table, JSDHashEntryHdr *hdr,
                      uint32_t number, void *arg)
{
    nsTArray<nsRefPtr<XPCWrappedNative> > *array =
        static_cast<nsTArray<nsRefPtr<XPCWrappedNative> > *>(arg);
    XPCWrappedNative *wn = ((Native2WrappedNativeMap::Entry*)hdr)->value;

    
    
    if (!wn->IsWrapperExpired())
        array->AppendElement(wn);
    return JS_DHASH_NEXT;
}


NS_IMETHODIMP
nsXPConnect::RescueOrphansInScope(JSContext *aJSContext, JSObject *aScope)
{
    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if (!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCWrappedNativeScope *scope = GetObjectScope(aScope);
    if (!scope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    
    
    nsTArray<nsRefPtr<XPCWrappedNative> > wrappersToMove;

    {   
        XPCAutoLock lock(GetRuntime()->GetMapLock());
        Native2WrappedNativeMap *map = scope->GetWrappedNativeMap();
        wrappersToMove.SetCapacity(map->Count());
        map->Enumerate(MoveableWrapperFinder, &wrappersToMove);
    }

    
    for (uint32_t i = 0, stop = wrappersToMove.Length(); i < stop; ++i) {
        nsresult rv = wrappersToMove[i]->RescueOrphans(ccx);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::SetSecurityManagerForJSContext(JSContext * aJSContext,
                                            nsIXPCSecurityManager *aManager,
                                            uint16_t flags)
{
    NS_ASSERTION(aJSContext, "bad param");

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if (!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCContext* xpcc = ccx.GetXPCContext();

    NS_IF_ADDREF(aManager);
    nsIXPCSecurityManager* oldManager = xpcc->GetSecurityManager();
    NS_IF_RELEASE(oldManager);

    xpcc->SetSecurityManager(aManager);
    xpcc->SetSecurityManagerFlags(flags);
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::GetSecurityManagerForJSContext(JSContext * aJSContext,
                                            nsIXPCSecurityManager **aManager,
                                            uint16_t *flags)
{
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aManager, "bad param");
    NS_ASSERTION(flags, "bad param");

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if (!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCContext* xpcc = ccx.GetXPCContext();

    nsIXPCSecurityManager* manager = xpcc->GetSecurityManager();
    NS_IF_ADDREF(manager);
    *aManager = manager;
    *flags = xpcc->GetSecurityManagerFlags();
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::SetDefaultSecurityManager(nsIXPCSecurityManager *aManager,
                                       uint16_t flags)
{
    NS_IF_ADDREF(aManager);
    NS_IF_RELEASE(mDefaultSecurityManager);
    mDefaultSecurityManager = aManager;
    mDefaultSecurityManagerFlags = flags;

    nsCOMPtr<nsIScriptSecurityManager> ssm =
        do_QueryInterface(mDefaultSecurityManager);

    
    
    gScriptSecurityManager = ssm;

    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::GetDefaultSecurityManager(nsIXPCSecurityManager **aManager,
                                       uint16_t *flags)
{
    NS_ASSERTION(aManager, "bad param");
    NS_ASSERTION(flags, "bad param");

    NS_IF_ADDREF(mDefaultSecurityManager);
    *aManager = mDefaultSecurityManager;
    *flags = mDefaultSecurityManagerFlags;
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::CreateStackFrameLocation(uint32_t aLanguage,
                                      const char *aFilename,
                                      const char *aFunctionName,
                                      int32_t aLineNumber,
                                      nsIStackFrame *aCaller,
                                      nsIStackFrame **_retval)
{
    NS_ASSERTION(_retval, "bad param");

    return XPCJSStack::CreateStackFrameLocation(aLanguage,
                                                aFilename,
                                                aFunctionName,
                                                aLineNumber,
                                                aCaller,
                                                _retval);
}


NS_IMETHODIMP
nsXPConnect::GetCurrentJSStack(nsIStackFrame * *aCurrentJSStack)
{
    NS_ASSERTION(aCurrentJSStack, "bad param");
    *aCurrentJSStack = nullptr;

    JSContext* cx;
    
    if (NS_SUCCEEDED(Peek(&cx)) && cx) {
        nsCOMPtr<nsIStackFrame> stack;
        XPCJSStack::CreateStack(cx, getter_AddRefs(stack));
        if (stack) {
            
            uint32_t language;
            nsCOMPtr<nsIStackFrame> caller;
            while (stack &&
                   NS_SUCCEEDED(stack->GetLanguage(&language)) &&
                   language != nsIProgrammingLanguage::JAVASCRIPT &&
                   NS_SUCCEEDED(stack->GetCaller(getter_AddRefs(caller))) &&
                   caller) {
                stack = caller;
            }
            NS_IF_ADDREF(*aCurrentJSStack = stack);
        }
    }
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::GetCurrentNativeCallContext(nsAXPCNativeCallContext * *aCurrentNativeCallContext)
{
    NS_ASSERTION(aCurrentNativeCallContext, "bad param");

    *aCurrentNativeCallContext = XPCJSRuntime::Get()->GetCallContext();
    return NS_OK;
}

NS_IMETHODIMP
nsXPConnect::SyncJSContexts(void)
{
    
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::SetFunctionThisTranslator(const nsIID & aIID,
                                       nsIXPCFunctionThisTranslator *aTranslator)
{
    XPCJSRuntime* rt = GetRuntime();
    IID2ThisTranslatorMap* map = rt->GetThisTranslatorMap();
    {
        XPCAutoLock lock(rt->GetMapLock()); 
        map->Add(aIID, aTranslator);
    }
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::ClearAllWrappedNativeSecurityPolicies()
{
    XPCCallContext ccx(NATIVE_CALLER);
    if (!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    return XPCWrappedNativeScope::ClearAllWrappedNativeSecurityPolicies(ccx);
}

NS_IMETHODIMP
nsXPConnect::CreateSandbox(JSContext *cx, nsIPrincipal *principal,
                           nsIXPConnectJSObjectHolder **_retval)
{
    XPCCallContext ccx(NATIVE_CALLER, cx);
    if (!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    *_retval = nullptr;

    jsval rval = JSVAL_VOID;
    AUTO_MARK_JSVAL(ccx, &rval);

    SandboxOptions options;
    nsresult rv = xpc_CreateSandboxObject(cx, &rval, principal, options);
    NS_ASSERTION(NS_FAILED(rv) || !JSVAL_IS_PRIMITIVE(rval),
                 "Bad return value from xpc_CreateSandboxObject()!");

    if (NS_SUCCEEDED(rv) && !JSVAL_IS_PRIMITIVE(rval)) {
        *_retval = XPCJSObjectHolder::newHolder(ccx, JSVAL_TO_OBJECT(rval));
        NS_ENSURE_TRUE(*_retval, NS_ERROR_OUT_OF_MEMORY);

        NS_ADDREF(*_retval);
    }

    return rv;
}

NS_IMETHODIMP
nsXPConnect::EvalInSandboxObject(const nsAString& source, JSContext *cx,
                                 nsIXPConnectJSObjectHolder *sandbox,
                                 bool returnStringOnly, jsval *rval)
{
    if (!sandbox)
        return NS_ERROR_INVALID_ARG;

    JSObject *obj;
    nsresult rv = sandbox->GetJSObject(&obj);
    NS_ENSURE_SUCCESS(rv, rv);

    return xpc_EvalInSandbox(cx, obj, source,
                             NS_ConvertUTF16toUTF8(source).get(), 1,
                             JSVERSION_DEFAULT, returnStringOnly, rval);
}


NS_IMETHODIMP
nsXPConnect::GetWrappedNativePrototype(JSContext * aJSContext,
                                       JSObject * aScope,
                                       nsIClassInfo *aClassInfo,
                                       nsIXPConnectJSObjectHolder **_retval)
{
    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if (!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    JSAutoCompartment ac(aJSContext, aScope);

    XPCWrappedNativeScope* scope = GetObjectScope(aScope);
    if (!scope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCNativeScriptableCreateInfo sciProto;
    XPCWrappedNative::GatherProtoScriptableCreateInfo(aClassInfo, sciProto);

    AutoMarkingWrappedNativeProtoPtr proto(ccx);
    proto = XPCWrappedNativeProto::GetNewOrUsed(ccx, scope, aClassInfo, &sciProto);
    if (!proto)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    nsIXPConnectJSObjectHolder* holder;
    *_retval = holder = XPCJSObjectHolder::newHolder(ccx,
                                                     proto->GetJSProtoObject());
    if (!holder)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    NS_ADDREF(holder);
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::ReleaseJSContext(JSContext * aJSContext, bool noGC)
{
    NS_ASSERTION(aJSContext, "bad param");
    XPCCallContext* ccx = nullptr;
    for (XPCCallContext* cur = GetRuntime()->GetCallContext();
         cur;
         cur = cur->GetPrevCallContext()) {
        if (cur->GetJSContext() == aJSContext) {
            ccx = cur;
            
        }
    }

    if (ccx) {
#ifdef DEBUG_xpc_hacker
        printf("!xpc - deferring destruction of JSContext @ %p\n",
               (void *)aJSContext);
#endif
        ccx->SetDestroyJSContextInDestructor(true);
        return NS_OK;
    }
    

    NS_ASSERTION(!XPCJSRuntime::Get()->GetJSContextStack()->
                 DEBUG_StackHasJSContext(aJSContext),
                 "JSContext still in threadjscontextstack!");

    if (noGC)
        JS_DestroyContextNoGC(aJSContext);
    else
        JS_DestroyContext(aJSContext);
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::DebugDump(int16_t depth)
{
#ifdef DEBUG
    depth-- ;
    XPC_LOG_ALWAYS(("nsXPConnect @ %x with mRefCnt = %d", this, mRefCnt.get()));
    XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("gSelf @ %x", gSelf));
        XPC_LOG_ALWAYS(("gOnceAliveNowDead is %d", (int)gOnceAliveNowDead));
        XPC_LOG_ALWAYS(("mDefaultSecurityManager @ %x", mDefaultSecurityManager));
        XPC_LOG_ALWAYS(("mDefaultSecurityManagerFlags of %x", mDefaultSecurityManagerFlags));
        XPC_LOG_ALWAYS(("mInterfaceInfoManager @ %x", mInterfaceInfoManager.get()));
        if (mRuntime) {
            if (depth)
                mRuntime->DebugDump(depth);
            else
                XPC_LOG_ALWAYS(("XPCJSRuntime @ %x", mRuntime));
        } else
            XPC_LOG_ALWAYS(("mRuntime is null"));
        XPCWrappedNativeScope::DebugDumpAllScopes(depth);
    XPC_LOG_OUTDENT();
#endif
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::DebugDumpObject(nsISupports *p, int16_t depth)
{
#ifdef DEBUG
    if (!depth)
        return NS_OK;
    if (!p) {
        XPC_LOG_ALWAYS(("*** Cound not dump object with NULL address"));
        return NS_OK;
    }

    nsIXPConnect* xpc;
    nsIXPCWrappedJSClass* wjsc;
    nsIXPConnectWrappedNative* wn;
    nsIXPConnectWrappedJS* wjs;

    if (NS_SUCCEEDED(p->QueryInterface(NS_GET_IID(nsIXPConnect),
                                       (void**)&xpc))) {
        XPC_LOG_ALWAYS(("Dumping a nsIXPConnect..."));
        xpc->DebugDump(depth);
        NS_RELEASE(xpc);
    } else if (NS_SUCCEEDED(p->QueryInterface(NS_GET_IID(nsIXPCWrappedJSClass),
                                              (void**)&wjsc))) {
        XPC_LOG_ALWAYS(("Dumping a nsIXPCWrappedJSClass..."));
        wjsc->DebugDump(depth);
        NS_RELEASE(wjsc);
    } else if (NS_SUCCEEDED(p->QueryInterface(NS_GET_IID(nsIXPConnectWrappedNative),
                                              (void**)&wn))) {
        XPC_LOG_ALWAYS(("Dumping a nsIXPConnectWrappedNative..."));
        wn->DebugDump(depth);
        NS_RELEASE(wn);
    } else if (NS_SUCCEEDED(p->QueryInterface(NS_GET_IID(nsIXPConnectWrappedJS),
                                              (void**)&wjs))) {
        XPC_LOG_ALWAYS(("Dumping a nsIXPConnectWrappedJS..."));
        wjs->DebugDump(depth);
        NS_RELEASE(wjs);
    } else
        XPC_LOG_ALWAYS(("*** Could not dump the nsISupports @ %x", p));
#endif
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::DebugDumpJSStack(bool showArgs,
                              bool showLocals,
                              bool showThisProps)
{
    JSContext* cx;
    if (NS_FAILED(Peek(&cx)))
        printf("failed to peek into nsIThreadJSContextStack service!\n");
    else if (!cx)
        printf("there is no JSContext on the nsIThreadJSContextStack!\n");
    else
        xpc_DumpJSStack(cx, showArgs, showLocals, showThisProps);

    return NS_OK;
}

char*
nsXPConnect::DebugPrintJSStack(bool showArgs,
                               bool showLocals,
                               bool showThisProps)
{
    JSContext* cx;
    if (NS_FAILED(Peek(&cx)))
        printf("failed to peek into nsIThreadJSContextStack service!\n");
    else if (!cx)
        printf("there is no JSContext on the nsIThreadJSContextStack!\n");
    else
        return xpc_PrintJSStack(cx, showArgs, showLocals, showThisProps);

    return nullptr;
}


NS_IMETHODIMP
nsXPConnect::DebugDumpEvalInJSStackFrame(uint32_t aFrameNumber, const char *aSourceText)
{
    JSContext* cx;
    if (NS_FAILED(Peek(&cx)))
        printf("failed to peek into nsIThreadJSContextStack service!\n");
    else if (!cx)
        printf("there is no JSContext on the nsIThreadJSContextStack!\n");
    else
        xpc_DumpEvalInJSStackFrame(cx, aFrameNumber, aSourceText);

    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::VariantToJS(JSContext* ctx, JSObject* scope, nsIVariant* value, jsval* _retval)
{
    NS_PRECONDITION(ctx, "bad param");
    NS_PRECONDITION(scope, "bad param");
    NS_PRECONDITION(value, "bad param");
    NS_PRECONDITION(_retval, "bad param");

    XPCCallContext ccx(NATIVE_CALLER, ctx);
    if (!ccx.IsValid())
        return NS_ERROR_FAILURE;
    XPCLazyCallContext lccx(ccx);

    ccx.SetScopeForNewJSObjects(scope);

    nsresult rv = NS_OK;
    if (!XPCVariant::VariantDataToJS(lccx, value, &rv, _retval)) {
        if (NS_FAILED(rv))
            return rv;

        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::JSToVariant(JSContext* ctx, const jsval &value, nsIVariant** _retval)
{
    NS_PRECONDITION(ctx, "bad param");
    NS_PRECONDITION(value != JSVAL_NULL, "bad param");
    NS_PRECONDITION(_retval, "bad param");

    XPCCallContext ccx(NATIVE_CALLER, ctx);
    if (!ccx.IsValid())
        return NS_ERROR_FAILURE;

    *_retval = XPCVariant::newVariant(ccx, value);
    if (!(*_retval))
        return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP
nsXPConnect::OnProcessNextEvent(nsIThreadInternal *aThread, bool aMayWait,
                                uint32_t aRecursionDepth)
{
    
    mEventDepth++;

    
    
    MOZ_ASSERT(NS_IsMainThread());
    return Push(nullptr);
}

NS_IMETHODIMP
nsXPConnect::AfterProcessNextEvent(nsIThreadInternal *aThread,
                                   uint32_t aRecursionDepth)
{
    
    if (MOZ_UNLIKELY(mEventDepth == 0))
        return NS_OK;
    mEventDepth--;

    
    MOZ_ASSERT(NS_IsMainThread());
    nsJSContext::MaybePokeCC();
    nsDOMMutationObserver::HandleMutations();

    return Pop(nullptr);
}

NS_IMETHODIMP
nsXPConnect::OnDispatchedEvent(nsIThreadInternal* aThread)
{
    NS_NOTREACHED("Why tell us?");
    return NS_ERROR_UNEXPECTED;
}

void
nsXPConnect::AddJSHolder(void* aHolder, nsScriptObjectTracer* aTracer)
{
    mRuntime->AddJSHolder(aHolder, aTracer);
}

void
nsXPConnect::RemoveJSHolder(void* aHolder)
{
    mRuntime->RemoveJSHolder(aHolder);
}

bool
nsXPConnect::TestJSHolder(void* aHolder)
{
#ifdef DEBUG
    return mRuntime->TestJSHolder(aHolder);
#else
    return false;
#endif
}

NS_IMETHODIMP
nsXPConnect::SetReportAllJSExceptions(bool newval)
{
    
    if (gReportAllJSExceptions != 1)
        gReportAllJSExceptions = newval ? 2 : 0;

    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::GetRuntime(JSRuntime **runtime)
{
    if (!runtime)
        return NS_ERROR_NULL_POINTER;

    JSRuntime *rt = GetRuntime()->GetJSRuntime();
    JS_AbortIfWrongThread(rt);
    *runtime = rt;
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::GetBackstagePass(nsIXPCScriptable **bsp)
{
    if (!mBackstagePass) {
        nsCOMPtr<nsIPrincipal> sysprin;
        nsCOMPtr<nsIScriptSecurityManager> secman =
            do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
        if (!secman)
            return NS_ERROR_NOT_AVAILABLE;
        if (NS_FAILED(secman->GetSystemPrincipal(getter_AddRefs(sysprin))))
            return NS_ERROR_NOT_AVAILABLE;

        mBackstagePass = new BackstagePass(sysprin);
        if (!mBackstagePass)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(*bsp = mBackstagePass);
    return NS_OK;
}


NS_IMETHODIMP_(void)
nsXPConnect::RegisterGCCallback(JSGCCallback func)
{
    mRuntime->AddGCCallback(func);
}


NS_IMETHODIMP_(void)
nsXPConnect::UnregisterGCCallback(JSGCCallback func)
{
    mRuntime->RemoveGCCallback(func);
}




NS_IMETHODIMP
nsXPConnect::GetCount(int32_t *aCount)
{
    MOZ_ASSERT(aCount);

    *aCount = XPCJSRuntime::Get()->GetJSContextStack()->Count();
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::Peek(JSContext * *_retval)
{
    MOZ_ASSERT(_retval);

    *_retval = xpc_UnmarkGrayContext(XPCJSRuntime::Get()->GetJSContextStack()->Peek());
    return NS_OK;
}

#ifdef MOZ_JSDEBUGGER
void
nsXPConnect::CheckForDebugMode(JSRuntime *rt)
{
    JSContext *cx = NULL;

    if (gDebugMode == gDesiredDebugMode) {
        return;
    }

    
    
    if (!NS_IsMainThread()) {
        return;
    }

    JS_SetRuntimeDebugMode(rt, gDesiredDebugMode);

    nsresult rv;
    const char jsdServiceCtrID[] = "@mozilla.org/js/jsd/debugger-service;1";
    nsCOMPtr<jsdIDebuggerService> jsds = do_GetService(jsdServiceCtrID, &rv);
    if (NS_FAILED(rv)) {
        goto fail;
    }

    if (!(cx = JS_NewContext(rt, 256))) {
        goto fail;
    }

    {
        struct AutoDestroyContext {
            JSContext *cx;
            AutoDestroyContext(JSContext *cx) : cx(cx) {}
            ~AutoDestroyContext() { JS_DestroyContext(cx); }
        } adc(cx);
        JSAutoRequest ar(cx);

        if (!JS_SetDebugModeForAllCompartments(cx, gDesiredDebugMode))
            goto fail;
    }

    if (gDesiredDebugMode) {
        rv = jsds->ActivateDebugger(rt);
    }

    gDebugMode = gDesiredDebugMode;
    return;

fail:
    if (jsds)
        jsds->DeactivateDebugger();

    




    if (gDesiredDebugMode)
        JS_SetRuntimeDebugMode(rt, false);
    gDesiredDebugMode = gDebugMode = false;
}
#else 
void
nsXPConnect::CheckForDebugMode(JSRuntime *rt)
{
    gDesiredDebugMode = gDebugMode = false;
}
#endif 


NS_EXPORT_(void)
xpc_ActivateDebugMode()
{
    XPCJSRuntime* rt = nsXPConnect::GetRuntimeInstance();
    nsXPConnect::GetXPConnect()->SetDebugModeWhenPossible(true, true);
    nsXPConnect::CheckForDebugMode(rt->GetJSRuntime());
}


NS_IMETHODIMP
nsXPConnect::Pop(JSContext * *_retval)
{
    JSContext *cx = XPCJSRuntime::Get()->GetJSContextStack()->Pop();
    if (_retval)
        *_retval = xpc_UnmarkGrayContext(cx);
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::Push(JSContext * cx)
{
     if (gDebugMode != gDesiredDebugMode && NS_IsMainThread()) {
         const InfallibleTArray<XPCJSContextInfo>* stack =
             XPCJSRuntime::Get()->GetJSContextStack()->GetStack();
         if (!gDesiredDebugMode) {
             
             CheckForDebugMode(mRuntime->GetJSRuntime());
         } else {
             bool runningJS = false;
             for (uint32_t i = 0; i < stack->Length(); ++i) {
                 JSContext *cx = (*stack)[i].cx;
                 if (cx && js::IsContextRunningJS(cx)) {
                     runningJS = true;
                     break;
                 }
             }
             if (!runningJS)
                 CheckForDebugMode(mRuntime->GetJSRuntime());
         }
     }

     return XPCJSRuntime::Get()->GetJSContextStack()->Push(cx) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


JSContext*
nsXPConnect::GetSafeJSContext()
{
    return XPCJSRuntime::Get()->GetJSContextStack()->GetSafeJSContext();
}

nsIPrincipal*
nsXPConnect::GetPrincipal(JSObject* obj, bool allowShortCircuit) const
{
    NS_ASSERTION(IS_WRAPPER_CLASS(js::GetObjectClass(obj)),
                 "What kind of wrapper is this?");

    if (IS_WN_WRAPPER_OBJECT(obj)) {
        XPCWrappedNative *xpcWrapper =
            (XPCWrappedNative *)xpc_GetJSPrivate(obj);
        if (xpcWrapper) {
            if (allowShortCircuit) {
                nsIPrincipal *result = xpcWrapper->GetObjectPrincipal();
                if (result) {
                    return result;
                }
            }

            
            nsCOMPtr<nsIScriptObjectPrincipal> objPrin =
                do_QueryInterface(xpcWrapper->Native());
            if (objPrin) {
                nsIPrincipal *result = objPrin->GetPrincipal();
                if (result) {
                    return result;
                }
            }
        }
    } else {
        if (allowShortCircuit) {
            nsIPrincipal *result =
                GetSlimWrapperProto(obj)->GetScope()->GetPrincipal();
            if (result) {
                return result;
            }
        }

        nsCOMPtr<nsIScriptObjectPrincipal> objPrin =
            do_QueryInterface((nsISupports*)xpc_GetJSPrivate(obj));
        if (objPrin) {
            nsIPrincipal *result = objPrin->GetPrincipal();
            if (result) {
                return result;
            }
        }
    }

    return nullptr;
}

NS_IMETHODIMP
nsXPConnect::HoldObject(JSContext *aJSContext, JSObject *aObject,
                        nsIXPConnectJSObjectHolder **aHolder)
{
    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    XPCJSObjectHolder* objHolder = XPCJSObjectHolder::newHolder(ccx, aObject);
    if (!objHolder)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aHolder = objHolder);
    return NS_OK;
}

NS_IMETHODIMP_(void)
nsXPConnect::GetCaller(JSContext **aJSContext, JSObject **aObj)
{
    XPCCallContext *ccx = XPCJSRuntime::Get()->GetCallContext();
    *aJSContext = ccx->GetJSContext();

    
    *aObj = ccx->GetFlattenedJSObject();
}

namespace xpc {

bool
DeferredRelease(nsISupports *obj)
{
    return nsXPConnect::GetRuntimeInstance()->DeferredRelease(obj);
}

NS_EXPORT_(bool)
Base64Encode(JSContext *cx, JS::Value val, JS::Value *out)
{
    MOZ_ASSERT(cx);
    MOZ_ASSERT(out);

    JS::Value root = val;
    xpc_qsACString encodedString(cx, root, &root, xpc_qsACString::eNull,
                                 xpc_qsACString::eStringify);
    if (!encodedString.IsValid())
        return false;

    nsAutoCString result;
    if (NS_FAILED(mozilla::Base64Encode(encodedString, result))) {
        JS_ReportError(cx, "Failed to encode base64 data!");
        return false;
    }

    JSString *str = JS_NewStringCopyN(cx, result.get(), result.Length());
    if (!str)
        return false;

    *out = STRING_TO_JSVAL(str);
    return true;
}

NS_EXPORT_(bool)
Base64Decode(JSContext *cx, JS::Value val, JS::Value *out)
{
    MOZ_ASSERT(cx);
    MOZ_ASSERT(out);

    JS::Value root = val;
    xpc_qsACString encodedString(cx, root, &root, xpc_qsACString::eNull,
                                 xpc_qsACString::eNull);
    if (!encodedString.IsValid())
        return false;

    nsAutoCString result;
    if (NS_FAILED(mozilla::Base64Decode(encodedString, result))) {
        JS_ReportError(cx, "Failed to decode base64 string!");
        return false;
    }

    JSString *str = JS_NewStringCopyN(cx, result.get(), result.Length());
    if (!str)
        return false;

    *out = STRING_TO_JSVAL(str);
    return true;
}

void
DumpJSHeap(FILE* file)
{
    NS_ABORT_IF_FALSE(NS_IsMainThread(), "Must dump GC heap on main thread.");
    nsXPConnect* xpc = nsXPConnect::GetXPConnect();
    if (!xpc) {
        NS_ERROR("Failed to get nsXPConnect instance!");
        return;
    }
    js::DumpHeapComplete(xpc->GetRuntime()->GetJSRuntime(), file);
}

void
SetLocationForGlobal(JSObject *global, const nsACString& location)
{
    MOZ_ASSERT(global);
    EnsureCompartmentPrivate(global)->SetLocation(location);
}

void
SetLocationForGlobal(JSObject *global, nsIURI *locationURI)
{
    MOZ_ASSERT(global);
    EnsureCompartmentPrivate(global)->SetLocation(locationURI);
}

} 

static void
NoteJSChildGrayWrapperShim(void *data, void *thing)
{
    TraversalTracer *trc = static_cast<TraversalTracer*>(data);
    NoteJSChild(trc, thing, js::GCThingTraceKind(thing));
}

static void
TraverseObjectShim(void *data, void *thing)
{
    nsCycleCollectionTraversalCallback *cb =
        static_cast<nsCycleCollectionTraversalCallback*>(data);

    MOZ_ASSERT(js::GCThingTraceKind(thing) == JSTRACE_OBJECT);
    TraverseGCThing(TRAVERSE_CPP, thing, JSTRACE_OBJECT, *cb);
}





















class JSZoneParticipant : public nsCycleCollectionParticipant
{
public:
    static NS_METHOD TraverseImpl(JSZoneParticipant *that, void *p,
                                  nsCycleCollectionTraversalCallback &cb)
    {
        MOZ_ASSERT(!cb.WantAllTraces());
        JS::Zone *zone = static_cast<JS::Zone *>(p);

        









        cb.DescribeGCedNode(false, "JS Zone");

        







        TraversalTracer trc(cb);
        JSRuntime *rt = nsXPConnect::GetRuntimeInstance()->GetJSRuntime();
        JS_TracerInit(&trc, rt, NoteJSChildTracerShim);
        trc.eagerlyTraceWeakMaps = DoNotTraceWeakMaps;
        js::VisitGrayWrapperTargets(zone, NoteJSChildGrayWrapperShim, &trc);

        



        js::IterateGrayObjects(zone, TraverseObjectShim, &cb);

        return NS_OK;
    }

    static NS_METHOD RootImpl(void *p)
    {
        return NS_OK;
    }

    static NS_METHOD UnlinkImpl(void *p)
    {
        return NS_OK;
    }

    static NS_METHOD UnrootImpl(void *p)
    {
        return NS_OK;
    }

    static NS_METHOD_(void) UnmarkIfPurpleImpl(void *n)
    {
    }
};

static const CCParticipantVTable<JSZoneParticipant>::Type
JSZone_cycleCollectorGlobal = {
    NS_IMPL_CYCLE_COLLECTION_NATIVE_VTABLE(JSZoneParticipant)
};

nsCycleCollectionParticipant *
xpc_JSZoneParticipant()
{
    return JSZone_cycleCollectorGlobal.GetParticipant();
}

NS_IMETHODIMP
nsXPConnect::SetDebugModeWhenPossible(bool mode, bool allowSyncDisable)
{
    gDesiredDebugMode = mode;
    if (!mode && allowSyncDisable)
        CheckForDebugMode(mRuntime->GetJSRuntime());
    return NS_OK;
}

NS_IMETHODIMP
nsXPConnect::GetTelemetryValue(JSContext *cx, jsval *rval)
{
    JSObject *obj = JS_NewObject(cx, NULL, NULL, NULL);
    if (!obj)
        return NS_ERROR_OUT_OF_MEMORY;

    unsigned attrs = JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT;

    size_t i = JS_SetProtoCalled(cx);
    jsval v = DOUBLE_TO_JSVAL(i);
    if (!JS_DefineProperty(cx, obj, "setProto", v, NULL, NULL, attrs))
        return NS_ERROR_OUT_OF_MEMORY;

    i = JS_GetCustomIteratorCount(cx);
    v = DOUBLE_TO_JSVAL(i);
    if (!JS_DefineProperty(cx, obj, "customIter", v, NULL, NULL, attrs))
        return NS_ERROR_OUT_OF_MEMORY;

    *rval = OBJECT_TO_JSVAL(obj);
    return NS_OK;
}

NS_IMETHODIMP
nsXPConnect::NotifyDidPaint()
{
    JS::NotifyDidPaint(GetRuntime()->GetJSRuntime());
    return NS_OK;
}

const uint8_t HAS_PRINCIPALS_FLAG               = 1;
const uint8_t HAS_ORIGIN_PRINCIPALS_FLAG        = 2;

static nsresult
WriteScriptOrFunction(nsIObjectOutputStream *stream, JSContext *cx,
                      JSScript *script, JSObject *functionObj)
{
    
    MOZ_ASSERT(!script != !functionObj);

    if (!script)
        script = JS_GetFunctionScript(cx, JS_GetObjectFunction(functionObj));

    nsIPrincipal *principal =
        nsJSPrincipals::get(JS_GetScriptPrincipals(script));
    nsIPrincipal *originPrincipal =
        nsJSPrincipals::get(JS_GetScriptOriginPrincipals(script));

    uint8_t flags = 0;
    if (principal)
        flags |= HAS_PRINCIPALS_FLAG;

    
    
    
    if (originPrincipal && originPrincipal != principal)
        flags |= HAS_ORIGIN_PRINCIPALS_FLAG;

    nsresult rv = stream->Write8(flags);
    if (NS_FAILED(rv))
        return rv;

    if (flags & HAS_PRINCIPALS_FLAG) {
        rv = stream->WriteObject(principal, true);
        if (NS_FAILED(rv))
            return rv;
    }

    if (flags & HAS_ORIGIN_PRINCIPALS_FLAG) {
        rv = stream->WriteObject(originPrincipal, true);
        if (NS_FAILED(rv))
            return rv;
    }

    uint32_t size;
    void* data;
    {
        JSAutoRequest ar(cx);
        if (functionObj)
            data = JS_EncodeInterpretedFunction(cx, functionObj, &size);
        else
            data = JS_EncodeScript(cx, script, &size);
    }

    if (!data)
        return NS_ERROR_OUT_OF_MEMORY;
    MOZ_ASSERT(size);
    rv = stream->Write32(size);
    if (NS_SUCCEEDED(rv))
        rv = stream->WriteBytes(static_cast<char *>(data), size);
    js_free(data);

    return rv;
}

static nsresult
ReadScriptOrFunction(nsIObjectInputStream *stream, JSContext *cx,
                     JSScript **scriptp, JSObject **functionObjp)
{
    
    MOZ_ASSERT(!scriptp != !functionObjp);

    uint8_t flags;
    nsresult rv = stream->Read8(&flags);
    if (NS_FAILED(rv))
        return rv;

    nsJSPrincipals* principal = nullptr;
    nsCOMPtr<nsIPrincipal> readPrincipal;
    if (flags & HAS_PRINCIPALS_FLAG) {
        rv = stream->ReadObject(true, getter_AddRefs(readPrincipal));
        if (NS_FAILED(rv))
            return rv;
        principal = nsJSPrincipals::get(readPrincipal);
    }

    nsJSPrincipals* originPrincipal = nullptr;
    nsCOMPtr<nsIPrincipal> readOriginPrincipal;
    if (flags & HAS_ORIGIN_PRINCIPALS_FLAG) {
        rv = stream->ReadObject(true, getter_AddRefs(readOriginPrincipal));
        if (NS_FAILED(rv))
            return rv;
        originPrincipal = nsJSPrincipals::get(readOriginPrincipal);
    }

    uint32_t size;
    rv = stream->Read32(&size);
    if (NS_FAILED(rv))
        return rv;

    char* data;
    rv = stream->ReadBytes(size, &data);
    if (NS_FAILED(rv))
        return rv;

    {
        JSAutoRequest ar(cx);
        if (scriptp) {
            JSScript *script = JS_DecodeScript(cx, data, size, principal, originPrincipal);
            if (!script)
                rv = NS_ERROR_OUT_OF_MEMORY;
            else
                *scriptp = script;
        } else {
            JSObject *funobj = JS_DecodeInterpretedFunction(cx, data, size,
                                                            principal, originPrincipal);
            if (!funobj)
                rv = NS_ERROR_OUT_OF_MEMORY;
            else
                *functionObjp = funobj;
        }
    }

    nsMemory::Free(data);
    return rv;
}

NS_IMETHODIMP
nsXPConnect::WriteScript(nsIObjectOutputStream *stream, JSContext *cx, JSScript *script)
{
    return WriteScriptOrFunction(stream, cx, script, nullptr);
}

NS_IMETHODIMP
nsXPConnect::ReadScript(nsIObjectInputStream *stream, JSContext *cx, JSScript **scriptp)
{
    return ReadScriptOrFunction(stream, cx, scriptp, nullptr);
}

NS_IMETHODIMP
nsXPConnect::WriteFunction(nsIObjectOutputStream *stream, JSContext *cx, JSObject *functionObj)
{
    return WriteScriptOrFunction(stream, cx, nullptr, functionObj);
}

NS_IMETHODIMP
nsXPConnect::ReadFunction(nsIObjectInputStream *stream, JSContext *cx, JSObject **functionObjp)
{
    return ReadScriptOrFunction(stream, cx, nullptr, functionObjp);
}

#ifdef DEBUG
void
nsXPConnect::SetObjectToUnlink(void* aObject)
{
    if (mRuntime)
        mRuntime->SetObjectToUnlink(aObject);
}

void
nsXPConnect::AssertNoObjectsToTrace(void* aPossibleJSHolder)
{
    if (mRuntime)
        mRuntime->AssertNoObjectsToTrace(aPossibleJSHolder);
}
#endif


JS_BEGIN_EXTERN_C
JS_EXPORT_API(void) DumpJSStack()
{
    nsresult rv;
    nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
    if (NS_SUCCEEDED(rv) && xpc)
        xpc->DebugDumpJSStack(true, true, false);
    else
        printf("failed to get XPConnect service!\n");
}

JS_EXPORT_API(char*) PrintJSStack()
{
    nsresult rv;
    nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
    return (NS_SUCCEEDED(rv) && xpc) ?
        xpc->DebugPrintJSStack(true, true, false) :
        nullptr;
}

JS_EXPORT_API(void) DumpJSEval(uint32_t frameno, const char* text)
{
    nsresult rv;
    nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
    if (NS_SUCCEEDED(rv) && xpc)
        xpc->DebugDumpEvalInJSStackFrame(frameno, text);
    else
        printf("failed to get XPConnect service!\n");
}

JS_EXPORT_API(void) DumpCompleteHeap()
{
    nsCOMPtr<nsICycleCollectorListener> listener =
      do_CreateInstance("@mozilla.org/cycle-collector-logger;1");
    if (!listener) {
      NS_WARNING("Failed to create CC logger");
      return;
    }

    nsCOMPtr<nsICycleCollectorListener> alltracesListener;
    listener->AllTraces(getter_AddRefs(alltracesListener));
    if (!alltracesListener) {
      NS_WARNING("Failed to get all traces logger");
      return;
    }

    nsJSContext::CycleCollectNow(alltracesListener);
}

JS_END_EXTERN_C

