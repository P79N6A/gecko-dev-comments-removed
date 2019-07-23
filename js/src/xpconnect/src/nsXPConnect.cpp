











































#include "xpcprivate.h"
#include "XPCNativeWrapper.h"
#include "XPCWrapper.h"
#include "nsBaseHashtable.h"
#include "nsHashKeys.h"
#include "jsatom.h"
#include "jsfun.h"
#include "jsscript.h"
#include "nsThreadUtilsInternal.h"
#include "dom_quickstubs.h"

NS_IMPL_THREADSAFE_ISUPPORTS6(nsXPConnect,
                              nsIXPConnect,
                              nsISupportsWeakReference,
                              nsIThreadObserver,
                              nsIJSRuntimeService,
                              nsIJSContextStack,
                              nsIThreadJSContextStack)

nsXPConnect* nsXPConnect::gSelf = nsnull;
JSBool       nsXPConnect::gOnceAliveNowDead = JS_FALSE;
PRUint32     nsXPConnect::gReportAllJSExceptions = 0;



nsIScriptSecurityManager *gScriptSecurityManager = nsnull;

const char XPC_CONTEXT_STACK_CONTRACTID[] = "@mozilla.org/js/xpc/ContextStack;1";
const char XPC_RUNTIME_CONTRACTID[]       = "@mozilla.org/js/xpc/RuntimeService;1";
const char XPC_EXCEPTION_CONTRACTID[]     = "@mozilla.org/js/xpc/Exception;1";
const char XPC_CONSOLE_CONTRACTID[]       = "@mozilla.org/consoleservice;1";
const char XPC_SCRIPT_ERROR_CONTRACTID[]  = "@mozilla.org/scripterror;1";
const char XPC_ID_CONTRACTID[]            = "@mozilla.org/js/xpc/ID;1";
const char XPC_XPCONNECT_CONTRACTID[]     = "@mozilla.org/js/xpc/XPConnect;1";



nsXPConnect::nsXPConnect()
    :   mRuntime(nsnull),
        mInterfaceInfoManager(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID)),
        mDefaultSecurityManager(nsnull),
        mDefaultSecurityManagerFlags(0),
        mShuttingDown(JS_FALSE),
        mCycleCollectionContext(nsnull),
        mCycleCollecting(PR_FALSE)
{
    mRuntime = XPCJSRuntime::newXPCJSRuntime(this);

    nsCycleCollector_registerRuntime(nsIProgrammingLanguage::JAVASCRIPT, this);
#ifdef DEBUG_CC
    mJSRoots.ops = nsnull;
#endif

#ifdef XPC_TOOLS_SUPPORT
  {
    char* filename = PR_GetEnv("MOZILLA_JS_PROFILER_OUTPUT");
    if(filename && *filename)
    {
        mProfilerOutputFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID);
        if(mProfilerOutputFile &&
           NS_SUCCEEDED(mProfilerOutputFile->InitWithNativePath(nsDependentCString(filename))))
        {
            mProfiler = do_GetService(XPCTOOLS_PROFILER_CONTRACTID);
            if(mProfiler)
            {
                if(NS_SUCCEEDED(mProfiler->Start()))
                {
#ifdef DEBUG
                    printf("***** profiling JavaScript. Output to: %s\n",
                           filename);
#endif
                }
            }
        }
    }
  }
#endif
    char* reportableEnv = PR_GetEnv("MOZ_REPORT_ALL_JS_EXCEPTIONS");
    if(reportableEnv && *reportableEnv)
        gReportAllJSExceptions = 1;
}

nsXPConnect::~nsXPConnect()
{
    NS_ASSERTION(!mCycleCollectionContext,
                 "Didn't call FinishCycleCollection?");
    nsCycleCollector_forgetRuntime(nsIProgrammingLanguage::JAVASCRIPT);

    JSContext *cx = nsnull;
    if (mRuntime) {
        
        
        
        
        
        cx = JS_NewContext(mRuntime->GetJSRuntime(), 8192);
    }

    XPCPerThreadData::CleanupAllThreads();
    mShuttingDown = JS_TRUE;
    if (cx) {
        JS_BeginRequest(cx);

        
        XPCWrappedNativeScope::SystemIsBeingShutDown(cx);

        mRuntime->SystemIsBeingShutDown(cx);

        JS_EndRequest(cx);
        JS_DestroyContext(cx);
    }

    NS_IF_RELEASE(mDefaultSecurityManager);

    gScriptSecurityManager = nsnull;

    
    XPC_LOG_FINISH();

    delete mRuntime;

    gSelf = nsnull;
    gOnceAliveNowDead = JS_TRUE;
}


nsXPConnect*
nsXPConnect::GetXPConnect()
{
    if(!gSelf)
    {
        if(gOnceAliveNowDead)
            return nsnull;
        gSelf = new nsXPConnect();
        if(!gSelf)
            return nsnull;

        if(!gSelf->mRuntime ||
           !gSelf->mInterfaceInfoManager)
        {
            
            delete gSelf;
            gSelf = nsnull;
        }
        else
        {
            
            
            NS_ADDREF(gSelf);
            if (NS_FAILED(NS_SetGlobalThreadObserver(gSelf))) {
                NS_RELEASE(gSelf);
                
            }
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
    if(xpc)
    {
        NS_SetGlobalThreadObserver(nsnull);

#ifdef XPC_TOOLS_SUPPORT
        if(xpc->mProfiler)
        {
            xpc->mProfiler->Stop();
            xpc->mProfiler->WriteResults(xpc->mProfilerOutputFile);
        }
#endif

#ifdef DEBUG
        
        
        {
            
            XPCCallContext ccx(NATIVE_CALLER);
            if(ccx.IsValid())
            {
                const char* dumpName = getenv("XPC_SHUTDOWN_HEAP_DUMP");
                if(dumpName)
                {
                    FILE* dumpFile = (*dumpName == '\0' ||
                                      strcmp(dumpName, "stdout") == 0)
                                     ? stdout
                                     : fopen(dumpName, "w");
                    if(dumpFile)
                    {
                        JS_DumpHeap(ccx, dumpFile, nsnull, 0, nsnull,
                                    static_cast<size_t>(-1), nsnull);
                        if(dumpFile != stdout)
                            fclose(dumpFile);
                    }
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
        if(0 != cnt)
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
    if(!xpc && !(xpc = GetXPConnect()))
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
    PRBool found = PR_FALSE;
    if(info)
        info->HasAncestor(&NS_GET_IID(nsISupports), &found);
    return found;
}



typedef PRBool (*InfoTester)(nsIInterfaceInfoManager* manager, const void* data,
                             nsIInterfaceInfo** info);

static PRBool IIDTester(nsIInterfaceInfoManager* manager, const void* data,
                        nsIInterfaceInfo** info)
{
    return NS_SUCCEEDED(manager->GetInfoForIID((const nsIID *) data, info)) &&
           *info;
}

static PRBool NameTester(nsIInterfaceInfoManager* manager, const void* data,
                      nsIInterfaceInfo** info)
{
    return NS_SUCCEEDED(manager->GetInfoForName((const char *) data, info)) &&
           *info;
}

static nsresult FindInfo(InfoTester tester, const void* data, 
                         nsIInterfaceInfoSuperManager* iism,
                         nsIInterfaceInfo** info)
{
    if(tester(iism, data, info))
        return NS_OK;
    
    

    PRBool yes;
    nsCOMPtr<nsISimpleEnumerator> list;

    if(NS_SUCCEEDED(iism->HasAdditionalManagers(&yes)) && yes &&
       NS_SUCCEEDED(iism->EnumerateAdditionalManagers(getter_AddRefs(list))) &&
       list)
    {
        PRBool more;
        nsCOMPtr<nsIInterfaceInfoManager> current;

        while(NS_SUCCEEDED(list->HasMoreElements(&more)) && more &&
              NS_SUCCEEDED(list->GetNext(getter_AddRefs(current))) && current)
        {
            if(tester(current, data, info))
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

static JSGCCallback gOldJSGCCallback;

static PRBool gDidCollection;

static PRBool gInCollection;

static PRBool gCollected;

static JSBool
XPCCycleCollectGCCallback(JSContext *cx, JSGCStatus status)
{
    
    switch(status)
    {
      case JSGC_BEGIN:
        nsXPConnect::GetRuntimeInstance()->UnrootContextGlobals();
        break;

      case JSGC_MARK_END:
        
        
        if(!gDidCollection)
        {
            NS_ASSERTION(!gInCollection, "Recursing?");

            gDidCollection = PR_TRUE;
            gInCollection = nsCycleCollector_beginCollection();
        }

        
        
        nsXPConnect::GetRuntimeInstance()->
            TraceXPConnectRoots(cx->runtime->gcMarkingTracer, JS_TRUE);
        break;

      case JSGC_END:
        if(gInCollection)
        {
            gInCollection = PR_FALSE;
            gCollected = nsCycleCollector_finishCollection();
        }
        break;

      default:
        break;
    }

    return gOldJSGCCallback ? gOldJSGCCallback(cx, status) : JS_TRUE;
}

PRBool
nsXPConnect::Collect()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    XPCCallContext cycleCollectionContext(NATIVE_CALLER);
    if(!cycleCollectionContext.IsValid())
    {
        return PR_FALSE;
    }

    mCycleCollecting = PR_TRUE;
    mCycleCollectionContext = &cycleCollectionContext;
    gDidCollection = PR_FALSE;
    gInCollection = PR_FALSE;
    gCollected = PR_FALSE;

    JSContext *cx = mCycleCollectionContext->GetJSContext();
    gOldJSGCCallback = JS_SetGCCallback(cx, XPCCycleCollectGCCallback);
    JS_GC(cx);
    JS_SetGCCallback(cx, gOldJSGCCallback);
    gOldJSGCCallback = nsnull;

    mCycleCollectionContext = nsnull;
    mCycleCollecting = PR_FALSE;

    return gCollected;
}



#define ADD_TO_CC(_kind)    ((_kind) == JSTRACE_OBJECT || (_kind) == JSTRACE_XML)

#ifdef DEBUG_CC
struct NoteJSRootTracer : public JSTracer
{
    NoteJSRootTracer(PLDHashTable *aObjects,
                     nsCycleCollectionTraversalCallback& cb)
      : mObjects(aObjects),
        mCb(cb)
    {
    }
    PLDHashTable* mObjects;
    nsCycleCollectionTraversalCallback& mCb;
};

static void
NoteJSRoot(JSTracer *trc, void *thing, uint32 kind)
{
    if(ADD_TO_CC(kind))
    {
        NoteJSRootTracer *tracer = static_cast<NoteJSRootTracer*>(trc);
        PLDHashEntryHdr *entry = PL_DHashTableOperate(tracer->mObjects, thing,
                                                      PL_DHASH_ADD);
        if(entry && !reinterpret_cast<PLDHashEntryStub*>(entry)->key)
        {
            reinterpret_cast<PLDHashEntryStub*>(entry)->key = thing;
            tracer->mCb.NoteRoot(nsIProgrammingLanguage::JAVASCRIPT, thing,
                                 nsXPConnect::GetXPConnect());
        }
    }
    else if(kind != JSTRACE_DOUBLE && kind != JSTRACE_STRING)
    {
        JS_TraceChildren(trc, thing, kind);
    }
}
#endif

nsresult 
nsXPConnect::BeginCycleCollection(nsCycleCollectionTraversalCallback &cb)
{
#ifdef DEBUG_CC
    NS_ASSERTION(!mJSRoots.ops, "Didn't call FinishCollection?");

    if(!mCycleCollectionContext)
    {
        
        mExplainCycleCollectionContext = new XPCCallContext(NATIVE_CALLER);
        if(!mExplainCycleCollectionContext ||
           !mExplainCycleCollectionContext->IsValid())
        {
            mExplainCycleCollectionContext = nsnull;
            return PR_FALSE;
        }

        mCycleCollectionContext = mExplainCycleCollectionContext;

        
        
        
        
        if(!PL_DHashTableInit(&mJSRoots, PL_DHashGetStubOps(), nsnull,
                              sizeof(PLDHashEntryStub), PL_DHASH_MIN_SIZE)) {
            mJSRoots.ops = nsnull;

            return NS_ERROR_OUT_OF_MEMORY;
        }

        GetRuntime()->UnrootContextGlobals();

        PRBool alreadyCollecting = mCycleCollecting;
        mCycleCollecting = PR_TRUE;
        NoteJSRootTracer trc(&mJSRoots, cb);
        JS_TRACER_INIT(&trc, mCycleCollectionContext->GetJSContext(),
                       NoteJSRoot);
        JS_TraceRuntime(&trc);
        mCycleCollecting = alreadyCollecting;
    }
#else
    NS_ASSERTION(mCycleCollectionContext,
                 "Didn't call nsXPConnect::Collect()?");
#endif

    GetRuntime()->AddXPConnectRoots(mCycleCollectionContext->GetJSContext(),
                                    cb);

    return NS_OK;
}

nsresult 
nsXPConnect::FinishCycleCollection()
{
#ifdef DEBUG_CC
    if(mExplainCycleCollectionContext)
    {
        mCycleCollectionContext = nsnull;
        mExplainCycleCollectionContext = nsnull;

        GetRuntime()->RootContextGlobals();
    }
#endif

#ifdef DEBUG_CC
    if(mJSRoots.ops)
    {
        PL_DHashTableFinish(&mJSRoots);
        mJSRoots.ops = nsnull;
    }
#endif

    return NS_OK;
}

nsCycleCollectionParticipant *
nsXPConnect::ToParticipant(void *p)
{
    return this;
}

NS_IMETHODIMP
nsXPConnect::RootAndUnlinkJSObjects(void *p)
{
    return NS_OK;
}

#ifdef DEBUG_CC
void
nsXPConnect::PrintAllReferencesTo(void *p)
{
#ifdef DEBUG
    if(!mCycleCollectionContext) {
        NS_NOTREACHED("no context");
        return;
    }
    JS_DumpHeap(*mCycleCollectionContext, stdout, nsnull, 0, p,
                0x7fffffff, nsnull);
#endif
}
#endif

NS_IMETHODIMP
nsXPConnect::Unlink(void *p)
{
    return NS_OK;
}

NS_IMETHODIMP
nsXPConnect::Unroot(void *p)
{
    return NS_OK;
}

struct TraversalTracer : public JSTracer
{
    TraversalTracer(nsCycleCollectionTraversalCallback &aCb) : cb(aCb)
    {
    }
    nsCycleCollectionTraversalCallback &cb;
};

static void
NoteJSChild(JSTracer *trc, void *thing, uint32 kind)
{
    if(ADD_TO_CC(kind))
    {
        TraversalTracer *tracer = static_cast<TraversalTracer*>(trc);
#if defined(DEBUG)
        if (NS_UNLIKELY(tracer->cb.WantDebugInfo())) {
            
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
                tracer->cb.NoteNextEdgeName(
                  static_cast<const char*>(tracer->debugPrintArg));
            }
        }
#endif
        tracer->cb.NoteScriptChild(nsIProgrammingLanguage::JAVASCRIPT, thing);
    }
    else if(kind != JSTRACE_DOUBLE && kind != JSTRACE_STRING)
    {
        JS_TraceChildren(trc, thing, kind);
    }
}

static JSBool
WrapperIsNotMainThreadOnly(XPCWrappedNative *wrapper)
{
    XPCWrappedNativeProto *proto = wrapper->GetProto();
    if(proto && proto->ClassIsMainThreadOnly())
        return PR_FALSE;

    
    
    
    nsXPCOMCycleCollectionParticipant* participant;
    return NS_FAILED(CallQueryInterface(wrapper->Native(), &participant));
}

NS_IMETHODIMP
nsXPConnect::Traverse(void *p, nsCycleCollectionTraversalCallback &cb)
{
    if(!mCycleCollectionContext)
        return NS_ERROR_FAILURE;

    JSContext *cx = mCycleCollectionContext->GetJSContext();

    uint32 traceKind = js_GetGCThingTraceKind(p);
    JSObject *obj;
    JSClass *clazz;

    
    
    
    
    
    
    JSBool dontTraverse = PR_FALSE;
    JSBool markJSObject = PR_FALSE;
    if(traceKind == JSTRACE_OBJECT)
    {
        obj = static_cast<JSObject*>(p);
        clazz = OBJ_GET_CLASS(cx, obj);

        if(clazz == &XPC_WN_Tearoff_JSClass)
        {
            XPCWrappedNative *wrapper =
                (XPCWrappedNative*)xpc_GetJSPrivate(STOBJ_GET_PARENT(obj));
            dontTraverse = WrapperIsNotMainThreadOnly(wrapper);
        }
        else if(IS_WRAPPER_CLASS(clazz))
        {
            XPCWrappedNative *wrapper = (XPCWrappedNative*)xpc_GetJSPrivate(obj);
            dontTraverse = WrapperIsNotMainThreadOnly(wrapper);
            markJSObject = dontTraverse && wrapper->HasExternalReference();
        }
    }

    CCNodeType type;

#ifdef DEBUG_CC
    
    
    
    
    
    
    
    
    
    
    if(mJSRoots.ops)
    {
        
        PLDHashEntryHdr* entry =
            PL_DHashTableOperate(&mJSRoots, p, PL_DHASH_LOOKUP);
        type = markJSObject || PL_DHASH_ENTRY_IS_BUSY(entry) ? GCMarked :
                                                               GCUnmarked;
    }
    else
#endif
    {
        
        type = !markJSObject && JS_IsAboutToBeFinalized(cx, p) ? GCUnmarked :
                                                                 GCMarked;
    }

    if (cb.WantDebugInfo()) {
        char name[72];
        if(traceKind == JSTRACE_OBJECT)
        {
            JSObject *obj = static_cast<JSObject*>(p);
            JSClass *clazz = OBJ_GET_CLASS(cx, obj);
            if(XPCNativeWrapper::IsNativeWrapperClass(clazz))
            {
                XPCWrappedNative* wn;
                if(XPCNativeWrapper::GetWrappedNative(cx, obj, &wn) && wn)
                {
                    XPCNativeScriptableInfo* si = wn->GetScriptableInfo();
                    if(si)
                    {
                        JS_snprintf(name, sizeof(name), "XPCNativeWrapper (%s)",
                                    si->GetJSClass()->name);
                    }
                    else
                    {
                        nsIClassInfo* ci = wn->GetClassInfo();
                        char* className = nsnull;
                        if(ci)
                            ci->GetClassDescription(&className);
                        if(className)
                        {
                            JS_snprintf(name, sizeof(name),
                                        "XPCNativeWrapper (%s)", className);
                            PR_Free(className);
                        }
                        else
                        {
                            XPCNativeSet* set = wn->GetSet();
                            XPCNativeInterface** array =
                                set->GetInterfaceArray();
                            PRUint16 count = set->GetInterfaceCount();

                            if(count > 0)
                                JS_snprintf(name, sizeof(name),
                                            "XPCNativeWrapper (%s)",
                                            array[0]->GetNameString());
                            else
                                JS_snprintf(name, sizeof(name),
                                            "XPCNativeWrapper");
                        }
                    }
                }
                else
                {
                    JS_snprintf(name, sizeof(name), "XPCNativeWrapper");
                }
            }
            else
            {
                XPCNativeScriptableInfo* si = nsnull;
                if(IS_PROTO_CLASS(clazz))
                {
                    XPCWrappedNativeProto* p =
                        (XPCWrappedNativeProto*) xpc_GetJSPrivate(obj);
                    si = p->GetScriptableInfo();
                }
                if(si)
                {
                    JS_snprintf(name, sizeof(name), "JS Object (%s - %s)",
                                clazz->name, si->GetJSClass()->name);
                }
                else if(clazz == &js_ScriptClass)
                {
                    JSScript* script = (JSScript*) xpc_GetJSPrivate(obj);
                    if(script->filename)
                    {
                        JS_snprintf(name, sizeof(name),
                                    "JS Object (Script - %s)",
                                    script->filename);
                    }
                    else
                    {
                        JS_snprintf(name, sizeof(name), "JS Object (Script)");
                    }
                }
                else if(clazz == &js_FunctionClass)
                {
                    JSFunction* fun = (JSFunction*) xpc_GetJSPrivate(obj);
                    JSString* str = JS_GetFunctionId(fun);
                    if(str)
                    {
                        NS_ConvertUTF16toUTF8
                            fname(reinterpret_cast<const PRUnichar*>(JS_GetStringChars(str)));
                        JS_snprintf(name, sizeof(name),
                                    "JS Object (Function - %s)", fname.get());
                    }
                    else
                    {
                        JS_snprintf(name, sizeof(name), "JS Object (Function)");
                    }
                }
                else
                {
                    JS_snprintf(name, sizeof(name), "JS Object (%s)",
                                clazz->name);
                }
            }
        }
        else
        {
            static const char trace_types[JSTRACE_LIMIT][7] = {
                "Object",
                "Double",
                "String",
                "Xml"
            };
            JS_snprintf(name, sizeof(name), "JS %s", trace_types[traceKind]);
        }

        if(traceKind == JSTRACE_OBJECT) {
            JSObject *global = static_cast<JSObject*>(p), *parent;
            while((parent = JS_GetParent(cx, global)))
                global = parent;
            char fullname[100];
            JS_snprintf(fullname, sizeof(fullname),
                        "%s (global=%p)", name, global);
            cb.DescribeNode(type, 0, sizeof(JSObject), fullname);
        } else {
            cb.DescribeNode(type, 0, sizeof(JSObject), name);
        }
    } else {
        cb.DescribeNode(type, 0, sizeof(JSObject), "JS Object");
    }

    if(!ADD_TO_CC(traceKind))
        return NS_OK;

    
    
    
    
    if(!cb.WantAllTraces() && type == GCMarked)
        return NS_OK;

    TraversalTracer trc(cb);

    JS_TRACER_INIT(&trc, cx, NoteJSChild);
    JS_TraceChildren(&trc, p, traceKind);

    if(traceKind != JSTRACE_OBJECT || dontTraverse)
        return NS_OK;
    
    if(clazz == &XPC_WN_Tearoff_JSClass)
    {
        
        
        
        XPCWrappedNativeTearOff *to =
            (XPCWrappedNativeTearOff*) xpc_GetJSPrivate(obj);
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "xpc_GetJSPrivate(obj)->mNative");
        cb.NoteXPCOMChild(to->GetNative());
    }
    
    
    
    
    else if(clazz->flags & JSCLASS_HAS_PRIVATE &&
            clazz->flags & JSCLASS_PRIVATE_IS_NSISUPPORTS &&
            !XPCNativeWrapper::IsNativeWrapperClass(clazz))
    {
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "xpc_GetJSPrivate(obj)");
        cb.NoteXPCOMChild(static_cast<nsISupports*>(xpc_GetJSPrivate(obj)));
    }

    return NS_OK;
}

PRInt32
nsXPConnect::GetRequestDepth(JSContext* cx)
{
    PRInt32 requestDepth = cx->outstandingRequests;
    XPCCallContext* context = GetCycleCollectionContext();
    if(context && cx == context->GetJSContext())
        
        
        --requestDepth;
    return requestDepth;
}

class JSContextParticipant : public nsCycleCollectionParticipant
{
public:
    NS_IMETHOD RootAndUnlinkJSObjects(void *n)
    {
        return NS_OK;
    }
    NS_IMETHOD Unlink(void *n)
    {
        
        
        return NS_OK;
    }
    NS_IMETHOD Unroot(void *n)
    {
        return NS_OK;
    }
    NS_IMETHODIMP Traverse(void *n, nsCycleCollectionTraversalCallback &cb)
    {
        JSContext *cx = static_cast<JSContext*>(n);

        
        
        
        
        PRInt32 refCount = nsXPConnect::GetXPConnect()->GetRequestDepth(cx) + 1;

        cb.DescribeNode(RefCounted, refCount, sizeof(JSContext),
                        "JSContext");
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[global object]");
        cb.NoteScriptChild(nsIProgrammingLanguage::JAVASCRIPT,
                           cx->globalObject);

        return NS_OK;
    }
};

static JSContextParticipant JSContext_cycleCollectorGlobal;


nsCycleCollectionParticipant*
nsXPConnect::JSContextParticipant()
{
    return &JSContext_cycleCollectorGlobal;
}

NS_IMETHODIMP_(void)
nsXPConnect::NoteJSContext(JSContext *aJSContext,
                           nsCycleCollectionTraversalCallback &aCb)
{
    aCb.NoteNativeChild(aJSContext, &JSContext_cycleCollectorGlobal);
}






inline nsresult UnexpectedFailure(nsresult rv)
{
    NS_ERROR("This is not supposed to fail!");
    return rv;
}

class SaveFrame
{
public:
    SaveFrame(JSContext *cx)
        : mJSContext(cx) {
        mFrame = JS_SaveFrameChain(mJSContext);
    }

    ~SaveFrame() {
        JS_RestoreFrameChain(mJSContext, mFrame);
    }

private:
    JSContext *mJSContext;
    JSStackFrame *mFrame;
};


NS_IMETHODIMP
nsXPConnect::InitClasses(JSContext * aJSContext, JSObject * aGlobalJSObj)
{
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aGlobalJSObj, "bad param");

    SaveFrame sf(aJSContext);
    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    if(!xpc_InitJSxIDClassObjects())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    if(!xpc_InitWrappedNativeJSOps())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCWrappedNativeScope* scope =
        XPCWrappedNativeScope::GetNewOrUsed(ccx, aGlobalJSObj);

    if(!scope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    scope->RemoveWrappedNativeProtos();

    if(!nsXPCComponents::AttachNewComponentsObject(ccx, scope, aGlobalJSObj))
        return UnexpectedFailure(NS_ERROR_FAILURE);

#ifdef XPC_IDISPATCH_SUPPORT
    
    XPCIDispatchExtension::Initialize(ccx, aGlobalJSObj);
#endif

    if (!XPCNativeWrapper::AttachNewConstructorObject(ccx, aGlobalJSObj))
        return UnexpectedFailure(NS_ERROR_FAILURE);

    if (!XPC_SJOW_AttachNewConstructorObject(ccx, aGlobalJSObj))
        return UnexpectedFailure(NS_ERROR_FAILURE);

    return NS_OK;
}

static JSBool
TempGlobalResolve(JSContext *aJSContext, JSObject *obj, jsval id)
{
    JSBool resolved;
    return JS_ResolveStandardClass(aJSContext, obj, id, &resolved);
}

static JSClass xpcTempGlobalClass = {
    "xpcTempGlobalClass", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, TempGlobalResolve, JS_ConvertStub,   nsnull,
    JSCLASS_NO_OPTIONAL_MEMBERS
};


NS_IMETHODIMP
nsXPConnect::InitClassesWithNewWrappedGlobal(JSContext * aJSContext,
                                             nsISupports *aCOMObj,
                                             const nsIID & aIID,
                                             PRUint32 aFlags,
                                             nsIXPConnectJSObjectHolder **_retval)
{
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aCOMObj, "bad param");
    NS_ASSERTION(_retval, "bad param");

    
    
    
    

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);

    PRBool system = (aFlags & nsIXPConnect::FLAG_SYSTEM_GLOBAL_OBJECT) != 0;
    JSObject* tempGlobal = JS_NewSystemObject(aJSContext, &xpcTempGlobalClass,
                                              nsnull, nsnull, system);

    if(!tempGlobal ||
       !JS_SetParent(aJSContext, tempGlobal, nsnull) ||
       !JS_SetPrototype(aJSContext, tempGlobal, nsnull))
        return UnexpectedFailure(NS_ERROR_FAILURE);

    jsval v;
    nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
    {
        
        
        AUTO_MARK_JSVAL(ccx, OBJECT_TO_JSVAL(tempGlobal));

        if(NS_FAILED(InitClasses(aJSContext, tempGlobal)))
            return UnexpectedFailure(NS_ERROR_FAILURE);

        nsresult rv;
        if(!XPCConvert::NativeInterface2JSObject(ccx, &v,
                                                 getter_AddRefs(holder),
                                                 aCOMObj, &aIID, nsnull,
                                                 nsnull, tempGlobal,
                                                 PR_FALSE, OBJ_IS_GLOBAL, &rv))
            return UnexpectedFailure(rv);

        NS_ASSERTION(NS_SUCCEEDED(rv) && holder, "Didn't wrap properly");
    }

    JSObject* globalJSObj = JSVAL_TO_OBJECT(v);
    if(!globalJSObj)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    if(aFlags & nsIXPConnect::FLAG_SYSTEM_GLOBAL_OBJECT)
        NS_ASSERTION(JS_IsSystemObject(aJSContext, globalJSObj), "huh?!");

    

    JS_SetParent(aJSContext, globalJSObj, nsnull);

    JSObject* oldGlobal = JS_GetGlobalObject(aJSContext);
    if(!oldGlobal || oldGlobal == tempGlobal)
        JS_SetGlobalObject(aJSContext, globalJSObj);

    if((aFlags & nsIXPConnect::INIT_JS_STANDARD_CLASSES) &&
       !JS_InitStandardClasses(aJSContext, globalJSObj))
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCWrappedNative* wrapper =
        reinterpret_cast<XPCWrappedNative*>(holder.get());
    XPCWrappedNativeScope* scope = wrapper->GetScope();

    if(!scope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    NS_ASSERTION(scope->GetGlobalJSObject() == tempGlobal, "stealing scope!");

    scope->SetGlobal(ccx, globalJSObj);

    JSObject* protoJSObject = wrapper->HasProto() ?
                                    wrapper->GetProto()->GetJSProtoObject() :
                                    globalJSObj;
    if(protoJSObject)
    {
        if(protoJSObject != globalJSObj)
            JS_SetParent(aJSContext, protoJSObject, globalJSObj);
        JS_SetPrototype(aJSContext, protoJSObject, scope->GetPrototypeJSObject());
    }

    if(!(aFlags & nsIXPConnect::OMIT_COMPONENTS_OBJECT)) {
        SaveFrame sf(ccx);
        if(!nsXPCComponents::AttachNewComponentsObject(ccx, scope, globalJSObj))
            return UnexpectedFailure(NS_ERROR_FAILURE);

        if(!XPCNativeWrapper::AttachNewConstructorObject(ccx, globalJSObj))
            return UnexpectedFailure(NS_ERROR_FAILURE);

        if(!XPC_SJOW_AttachNewConstructorObject(ccx, globalJSObj))
            return UnexpectedFailure(NS_ERROR_FAILURE);
    }

    NS_ADDREF(*_retval = holder);

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

    jsval v;
    return WrapNativeToJSVal(aJSContext, aScope, aCOMObj, &aIID, PR_FALSE,
                             &v, aHolder);
}


NS_IMETHODIMP
nsXPConnect::WrapNativeToJSVal(JSContext * aJSContext,
                               JSObject * aScope,
                               nsISupports *aCOMObj,
                               const nsIID * aIID,
                               PRBool aAllowWrapping,
                               jsval *aVal,
                               nsIXPConnectJSObjectHolder **aHolder)
{
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aScope, "bad param");
    NS_ASSERTION(aCOMObj, "bad param");

    if(aHolder)
        *aHolder = nsnull;

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    nsresult rv;
    if(!XPCConvert::NativeInterface2JSObject(ccx, aVal, aHolder, aCOMObj, aIID,
                                             nsnull, nsnull, aScope, aAllowWrapping,
                                             OBJ_IS_NOT_GLOBAL, &rv))
        return rv;

#ifdef DEBUG
    NS_ASSERTION(!XPCNativeWrapper::IsNativeWrapper(JSVAL_TO_OBJECT(*aVal)),
                 "Shouldn't be returning a native wrapper here");
#endif
    
    return NS_OK;
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

    *result = nsnull;

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    nsresult rv;
    if(!XPCConvert::JSObject2NativeInterface(ccx, result, aJSObj,
                                             &aIID, nsnull, &rv))
        return rv;
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

    *result = nsnull;

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    nsresult rv;
    if(!XPCConvert::JSObject2NativeInterface(ccx, result, aJSObj,
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
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    SLIM_LOG_WILL_MORPH(aJSContext, aJSObj);
    if(!IS_SLIM_WRAPPER(aJSObj) || MorphSlimWrapper(aJSContext, aJSObj))
    {
        nsIXPConnectWrappedNative* wrapper =
            XPCWrappedNative::GetWrappedNativeOfJSObject(aJSContext, aJSObj);
        if(wrapper)
        {
            NS_ADDREF(wrapper);
            *_retval = wrapper;
            return NS_OK;
        }
    }

    
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP_(nsISupports*)
nsXPConnect::GetNativeOfWrapper(JSContext * aJSContext,
                                JSObject * aJSObj)
{
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aJSObj, "bad param");

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
    {
        UnexpectedFailure(NS_ERROR_FAILURE);
        return nsnull;
    }

    JSObject* obj2 = nsnull;
    nsIXPConnectWrappedNative* wrapper =
        XPCWrappedNative::GetWrappedNativeOfJSObject(aJSContext, aJSObj, nsnull,
                                                     &obj2);

    return wrapper ? wrapper->Native() :
                     (obj2 ? (nsISupports*)xpc_GetJSPrivate(obj2) : nsnull);
}


NS_IMETHODIMP
nsXPConnect::GetJSObjectOfWrapper(JSContext * aJSContext,
                                  JSObject * aJSObj,
                                  JSObject **_retval)
{
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aJSObj, "bad param");
    NS_ASSERTION(_retval, "bad param");

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    JSObject* obj2 = nsnull;
    nsIXPConnectWrappedNative* wrapper =
        XPCWrappedNative::GetWrappedNativeOfJSObject(aJSContext, aJSObj, nsnull,
                                                     &obj2);
    if(wrapper)
    {
        wrapper->GetJSObject(_retval);
        return NS_OK;
    }
    if(obj2)
    {
        *_retval = obj2;
        return NS_OK;
    }
    
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
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

    *_retval = nsnull;

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCWrappedNativeScope* scope =
        XPCWrappedNativeScope::FindInJSObjectScope(ccx, aScope);
    if(!scope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    AutoMarkingNativeInterfacePtr iface(ccx);
    iface = XPCNativeInterface::GetNewOrUsed(ccx, &aIID);
    if(!iface)
        return NS_ERROR_FAILURE;

    XPCWrappedNative* wrapper;

    nsresult rv = XPCWrappedNative::GetUsedOnly(ccx, aCOMObj, scope, iface,
                                                &wrapper);
    if(NS_FAILED(rv))
        return NS_ERROR_FAILURE;
    *_retval = static_cast<nsIXPConnectWrappedNative*>(wrapper);
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::ReparentWrappedNativeIfFound(JSContext * aJSContext,
                                          JSObject * aScope,
                                          JSObject * aNewParent,
                                          nsISupports *aCOMObj,
                                          nsIXPConnectJSObjectHolder **_retval)
{
    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCWrappedNativeScope* scope =
        XPCWrappedNativeScope::FindInJSObjectScope(ccx, aScope);
    if(!scope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCWrappedNativeScope* scope2 =
        XPCWrappedNativeScope::FindInJSObjectScope(ccx, aNewParent);
    if(!scope2)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    return XPCWrappedNative::
        ReparentWrapperIfFound(ccx, scope, scope2, aNewParent, aCOMObj,
                               (XPCWrappedNative**) _retval);
}

static JSDHashOperator
MoveableWrapperFinder(JSDHashTable *table, JSDHashEntryHdr *hdr,
                      uint32 number, void *arg)
{
    
    nsTArray<nsRefPtr<XPCWrappedNative> > *array =
        static_cast<nsTArray<nsRefPtr<XPCWrappedNative> > *>(arg);
    array->AppendElement(((Native2WrappedNativeMap::Entry*)hdr)->value);
    return JS_DHASH_NEXT;
}


NS_IMETHODIMP
nsXPConnect::ReparentScopeAwareWrappers(JSContext *aJSContext,
                                        JSObject *aOldScope,
                                        JSObject *aNewScope)
{
    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCWrappedNativeScope *oldScope =
        XPCWrappedNativeScope::FindInJSObjectScope(ccx, aOldScope);
    if(!oldScope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCWrappedNativeScope *newScope =
        XPCWrappedNativeScope::FindInJSObjectScope(ccx, aNewScope);
    if(!newScope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    
    
    nsTArray<nsRefPtr<XPCWrappedNative> > wrappersToMove;

    {   
        XPCAutoLock lock(GetRuntime()->GetMapLock());
        Native2WrappedNativeMap *map = oldScope->GetWrappedNativeMap();
        wrappersToMove.SetCapacity(map->Count());
        map->Enumerate(MoveableWrapperFinder, &wrappersToMove);
    }

    
    for(PRUint32 i = 0, stop = wrappersToMove.Length(); i < stop; ++i)
    {
        
        

        XPCWrappedNative *wrapper = wrappersToMove[i];
        nsISupports *identity = wrapper->GetIdentityObject();
        nsCOMPtr<nsIClassInfo> info(do_QueryInterface(identity));

        
        
        
        if(SameCOMIdentity(identity, info))
            info = nsnull;

        if(!info)
            continue;

        XPCNativeScriptableCreateInfo sciProto;
        XPCNativeScriptableCreateInfo sciWrapper;

        nsresult rv =
            XPCWrappedNative::GatherScriptableCreateInfo(identity,
                                                         info.get(),
                                                         &sciProto,
                                                         &sciWrapper);
        if(NS_FAILED(rv))
            return NS_ERROR_FAILURE;

        
        
        if(!sciWrapper.GetFlags().WantPreCreate())
            continue;

        JSObject *newParent = aOldScope;
        rv = sciWrapper.GetCallback()->PreCreate(identity, ccx, aOldScope,
                                                 &newParent);
        if(NS_FAILED(rv))
            return rv;

        if(newParent != aOldScope)
        {
            
            
            

            XPCWrappedNativeScope *betterScope =
                XPCWrappedNativeScope::FindInJSObjectScope(ccx, newParent);
            if(betterScope == oldScope)
                continue;

            NS_ASSERTION(betterScope == newScope, "Weird scope returned");
        }
        else
        {
            
            continue;
        }

        
        

        nsRefPtr<XPCWrappedNative> junk;
        rv = XPCWrappedNative::ReparentWrapperIfFound(ccx, oldScope,
                                                      newScope, newParent,
                                                      wrapper->GetIdentityObject(),
                                                      getter_AddRefs(junk));
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::SetSecurityManagerForJSContext(JSContext * aJSContext,
                                            nsIXPCSecurityManager *aManager,
                                            PRUint16 flags)
{
    NS_ASSERTION(aJSContext, "bad param");

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
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
                                            PRUint16 *flags)
{
    NS_ASSERTION(aJSContext, "bad param");
    NS_ASSERTION(aManager, "bad param");
    NS_ASSERTION(flags, "bad param");

    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
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
                                       PRUint16 flags)
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
                                       PRUint16 *flags)
{
    NS_ASSERTION(aManager, "bad param");
    NS_ASSERTION(flags, "bad param");

    NS_IF_ADDREF(mDefaultSecurityManager);
    *aManager = mDefaultSecurityManager;
    *flags = mDefaultSecurityManagerFlags;
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::CreateStackFrameLocation(PRUint32 aLanguage,
                                      const char *aFilename,
                                      const char *aFunctionName,
                                      PRInt32 aLineNumber,
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
    *aCurrentJSStack = nsnull;

    JSContext* cx;
    
    if(NS_SUCCEEDED(Peek(&cx)) && cx)
    {
        nsCOMPtr<nsIStackFrame> stack;
        XPCJSStack::CreateStack(cx, getter_AddRefs(stack));
        if(stack)
        {
            
            PRUint32 language;
            nsCOMPtr<nsIStackFrame> caller;
            while(stack &&
                  NS_SUCCEEDED(stack->GetLanguage(&language)) &&
                  language != nsIProgrammingLanguage::JAVASCRIPT &&
                  NS_SUCCEEDED(stack->GetCaller(getter_AddRefs(caller))) &&
                  caller)
            {
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

    XPCPerThreadData* data = XPCPerThreadData::GetData(nsnull);
    if(data)
    {
        *aCurrentNativeCallContext = data->GetCallContext();
        return NS_OK;
    }
    
    *aCurrentNativeCallContext = nsnull;
    return UnexpectedFailure(NS_ERROR_FAILURE);
}


NS_IMETHODIMP
nsXPConnect::GetPendingException(nsIException * *aPendingException)
{
    NS_ASSERTION(aPendingException, "bad param");

    XPCPerThreadData* data = XPCPerThreadData::GetData(nsnull);
    if(!data)
    {
        *aPendingException = nsnull;
        return UnexpectedFailure(NS_ERROR_FAILURE);
    }

    return data->GetException(aPendingException);
}

NS_IMETHODIMP
nsXPConnect::SetPendingException(nsIException * aPendingException)
{
    XPCPerThreadData* data = XPCPerThreadData::GetData(nsnull);
    if(!data)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    data->SetException(aPendingException);
    return NS_OK;
}

NS_IMETHODIMP
nsXPConnect::SyncJSContexts(void)
{
    
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::SetFunctionThisTranslator(const nsIID & aIID,
                                       nsIXPCFunctionThisTranslator *aTranslator,
                                       nsIXPCFunctionThisTranslator **_retval)
{
    XPCJSRuntime* rt = GetRuntime();
    nsIXPCFunctionThisTranslator* old;
    IID2ThisTranslatorMap* map = rt->GetThisTranslatorMap();

    {
        XPCAutoLock lock(rt->GetMapLock()); 
        if(_retval)
        {
            old = map->Find(aIID);
            NS_IF_ADDREF(old);
            *_retval = old;
        }
        map->Add(aIID, aTranslator);
    }
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::GetFunctionThisTranslator(const nsIID & aIID,
                                       nsIXPCFunctionThisTranslator **_retval)
{
    XPCJSRuntime* rt = GetRuntime();
    nsIXPCFunctionThisTranslator* old;
    IID2ThisTranslatorMap* map = rt->GetThisTranslatorMap();

    {
        XPCAutoLock lock(rt->GetMapLock()); 
        old = map->Find(aIID);
        NS_IF_ADDREF(old);
        *_retval = old;
    }
    return NS_OK;
}


NS_IMETHODIMP 
nsXPConnect::SetSafeJSContextForCurrentThread(JSContext * cx)
{
    XPCCallContext ccx(NATIVE_CALLER);
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);
    return ccx.GetThreadData()->GetJSContextStack()->SetSafeJSContext(cx);
}


NS_IMETHODIMP
nsXPConnect::ClearAllWrappedNativeSecurityPolicies()
{
    XPCCallContext ccx(NATIVE_CALLER);
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    return XPCWrappedNativeScope::ClearAllWrappedNativeSecurityPolicies(ccx);
}


NS_IMETHODIMP 
nsXPConnect::RestoreWrappedNativePrototype(JSContext * aJSContext, 
                                           JSObject * aScope, 
                                           nsIClassInfo * aClassInfo, 
                                           nsIXPConnectJSObjectHolder * aPrototype)
{
    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    if(!aClassInfo || !aPrototype)
        return UnexpectedFailure(NS_ERROR_INVALID_ARG);

    JSObject *protoJSObject;
    nsresult rv = aPrototype->GetJSObject(&protoJSObject);
    if(NS_FAILED(rv))
        return UnexpectedFailure(rv);

    if(!IS_PROTO_CLASS(STOBJ_GET_CLASS(protoJSObject)))
        return UnexpectedFailure(NS_ERROR_INVALID_ARG);

    XPCWrappedNativeScope* scope =
        XPCWrappedNativeScope::FindInJSObjectScope(ccx, aScope);
    if(!scope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCWrappedNativeProto *proto =
        (XPCWrappedNativeProto*)xpc_GetJSPrivate(protoJSObject);
    if(!proto)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    if(scope != proto->GetScope())
    {
        NS_ERROR("Attempt to reset prototype to a prototype from a"
                 "different scope!");

        return UnexpectedFailure(NS_ERROR_INVALID_ARG);
    }

    XPCNativeScriptableInfo *si = proto->GetScriptableInfo();

    if(si && si->GetFlags().DontSharePrototype())
        return UnexpectedFailure(NS_ERROR_INVALID_ARG);

    ClassInfo2WrappedNativeProtoMap* map =
        scope->GetWrappedNativeProtoMap(proto->ClassIsMainThreadOnly());
    XPCLock* lock = GetRuntime()->GetMapLock();

    {   
        XPCAutoLock al(lock);

        XPCWrappedNativeProtoMap* detachedMap =
            GetRuntime()->GetDetachedWrappedNativeProtoMap();

        
        
        
        XPCWrappedNativeProto *oldProto = map->Find(aClassInfo);
        if(oldProto)
        {
            detachedMap->Add(oldProto);

            
            
            
            
            map->Remove(aClassInfo);

            
            
            
            NS_ASSERTION(proto != oldProto,
                         "Restoring current prototype, fix caller!");
        }

        map->Add(aClassInfo, proto);

        
        
        
        detachedMap->Remove(proto);
    }

    
    
    
    
    scope->SetGlobal(ccx, aScope);

    return NS_OK;
}

NS_IMETHODIMP
nsXPConnect::CreateSandbox(JSContext *cx, nsIPrincipal *principal,
                           nsIXPConnectJSObjectHolder **_retval)
{
#ifdef XPCONNECT_STANDALONE
    return NS_ERROR_NOT_AVAILABLE;
#else 
    XPCCallContext ccx(NATIVE_CALLER, cx);
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    *_retval = nsnull;

    jsval rval = JSVAL_VOID;
    AUTO_MARK_JSVAL(ccx, &rval);

    nsresult rv = xpc_CreateSandboxObject(cx, &rval, principal);
    NS_ASSERTION(NS_FAILED(rv) || !JSVAL_IS_PRIMITIVE(rval),
                 "Bad return value from xpc_CreateSandboxObject()!");

    if (NS_SUCCEEDED(rv) && !JSVAL_IS_PRIMITIVE(rval)) {
        *_retval = XPCJSObjectHolder::newHolder(ccx, JSVAL_TO_OBJECT(rval));
        NS_ENSURE_TRUE(*_retval, NS_ERROR_OUT_OF_MEMORY);

        NS_ADDREF(*_retval);
    }

    return rv;
#endif 
}

NS_IMETHODIMP
nsXPConnect::EvalInSandboxObject(const nsAString& source, JSContext *cx,
                                 nsIXPConnectJSObjectHolder *sandbox,
                                 PRBool returnStringOnly, jsval *rval)
{
#ifdef XPCONNECT_STANDALONE
    return NS_ERROR_NOT_AVAILABLE;
#else 
    if (!sandbox)
        return NS_ERROR_INVALID_ARG;

    JSObject *obj;
    nsresult rv = sandbox->GetJSObject(&obj);
    NS_ENSURE_SUCCESS(rv, rv);

    return xpc_EvalInSandbox(cx, obj, source,
                             NS_ConvertUTF16toUTF8(source).get(), 1,
                             JSVERSION_DEFAULT, returnStringOnly, rval);
#endif 
}


NS_IMETHODIMP
nsXPConnect::GetXPCWrappedNativeJSClassInfo(JSEqualityOp *equality1,
                                            JSEqualityOp *equality2)
{
    
    

    *equality1 = &XPC_WN_Equality;
    *equality2 = &XPC_SWN_Equality;

    return NS_OK;
}


NS_IMETHODIMP 
nsXPConnect::GetWrappedNativePrototype(JSContext * aJSContext, 
                                       JSObject * aScope, 
                                       nsIClassInfo *aClassInfo, 
                                       nsIXPConnectJSObjectHolder **_retval)
{
    XPCCallContext ccx(NATIVE_CALLER, aJSContext);
    if(!ccx.IsValid())
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCWrappedNativeScope* scope =
        XPCWrappedNativeScope::FindInJSObjectScope(ccx, aScope);
    if(!scope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCNativeScriptableCreateInfo sciProto;
    XPCWrappedNative::GatherProtoScriptableCreateInfo(aClassInfo, &sciProto);

    AutoMarkingWrappedNativeProtoPtr proto(ccx);
    proto = XPCWrappedNativeProto::GetNewOrUsed(ccx, scope, aClassInfo, 
                                                &sciProto, JS_FALSE,
                                                OBJ_IS_NOT_GLOBAL);
    if(!proto)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    nsIXPConnectJSObjectHolder* holder;
    *_retval = holder = XPCJSObjectHolder::newHolder(ccx, 
                                                     proto->GetJSProtoObject());
    if(!holder)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    NS_ADDREF(holder);
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::GetXOWForObject(JSContext * aJSContext,
                             JSObject * aParent,
                             JSObject * aWrappedObj,
                             jsval * rval)
{
    *rval = OBJECT_TO_JSVAL(aWrappedObj);
    return XPC_XOW_WrapObject(aJSContext, aParent, rval)
           ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXPConnect::GetCOWForObject(JSContext * aJSContext,
                             JSObject * aParent,
                             JSObject * aWrappedObj,
                             jsval * rval)
{
    *rval = OBJECT_TO_JSVAL(aWrappedObj);
    return XPC_COW_WrapObject(aJSContext, aParent, *rval, rval)
           ? NS_OK : NS_ERROR_FAILURE;
}

static inline PRBool
PerformOp(JSContext *cx, PRUint32 aWay, JSObject *obj)
{
    NS_ASSERTION(aWay == nsIXPConnect::XPC_XOW_CLEARSCOPE,
                 "Nothing else is implemented yet");

    JS_ClearScope(cx, obj);
    return PR_TRUE;
}




NS_IMETHODIMP
nsXPConnect::UpdateXOWs(JSContext* aJSContext,
                        nsIXPConnectWrappedNative* aObject,
                        PRUint32 aWay)
{
    typedef WrappedNative2WrapperMap::Link Link;
    XPCWrappedNative* wn = static_cast<XPCWrappedNative *>(aObject);
    XPCWrappedNativeScope* scope = wn->GetScope();
    WrappedNative2WrapperMap* map = scope->GetWrapperMap();
    Link* list;

    {
        XPCAutoLock al(GetRuntime()->GetMapLock());

        list = map->FindLink(wn->GetFlatJSObject());
    }

    if(!list)
        return NS_OK; 

    AutoJSRequestWithNoCallContext req(aJSContext);

    Link* cur = list;
    if(cur->obj && !PerformOp(aJSContext, aWay, cur->obj))
        return NS_ERROR_FAILURE;

    for(cur = (Link *)PR_NEXT_LINK(list); cur != list;
        cur = (Link *)PR_NEXT_LINK(cur))
    {
        if(!PerformOp(aJSContext, aWay, cur->obj))
            return NS_ERROR_FAILURE;
    }

    return NS_OK;
}


NS_IMETHODIMP 
nsXPConnect::ReleaseJSContext(JSContext * aJSContext, PRBool noGC)
{
    NS_ASSERTION(aJSContext, "bad param");
    XPCPerThreadData* tls = XPCPerThreadData::GetData(aJSContext);
    if(tls)
    {
        XPCCallContext* ccx = nsnull;
        for(XPCCallContext* cur = tls->GetCallContext(); 
            cur; 
            cur = cur->GetPrevCallContext())
        {
            if(cur->GetJSContext() == aJSContext)
            {
                ccx = cur;
                
            }
        }
    
        if(ccx)
        {
#ifdef DEBUG_xpc_hacker
            printf("!xpc - deferring destruction of JSContext @ %p\n", 
                   (void *)aJSContext);
#endif
            ccx->SetDestroyJSContextInDestructor(JS_TRUE);
            JS_ClearNewbornRoots(aJSContext);
            return NS_OK;
        }
        

        NS_ASSERTION(!tls->GetJSContextStack() || 
                     !tls->GetJSContextStack()->
                        DEBUG_StackHasJSContext(aJSContext),
                     "JSContext still in threadjscontextstack!");
    }
    
    if(noGC)
        JS_DestroyContextNoGC(aJSContext);
    else
        JS_DestroyContext(aJSContext);
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::DebugDump(PRInt16 depth)
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
        if(mRuntime)
        {
            if(depth)
                mRuntime->DebugDump(depth);
            else
                XPC_LOG_ALWAYS(("XPCJSRuntime @ %x", mRuntime));
        }
        else
            XPC_LOG_ALWAYS(("mRuntime is null"));
        XPCWrappedNativeScope::DebugDumpAllScopes(depth);
    XPC_LOG_OUTDENT();
#endif
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::DebugDumpObject(nsISupports *p, PRInt16 depth)
{
#ifdef DEBUG
    if(!depth)
        return NS_OK;
    if(!p)
    {
        XPC_LOG_ALWAYS(("*** Cound not dump object with NULL address"));
        return NS_OK;
    }

    nsIXPConnect* xpc;
    nsIXPCWrappedJSClass* wjsc;
    nsIXPConnectWrappedNative* wn;
    nsIXPConnectWrappedJS* wjs;

    if(NS_SUCCEEDED(p->QueryInterface(NS_GET_IID(nsIXPConnect),
                        (void**)&xpc)))
    {
        XPC_LOG_ALWAYS(("Dumping a nsIXPConnect..."));
        xpc->DebugDump(depth);
        NS_RELEASE(xpc);
    }
    else if(NS_SUCCEEDED(p->QueryInterface(NS_GET_IID(nsIXPCWrappedJSClass),
                        (void**)&wjsc)))
    {
        XPC_LOG_ALWAYS(("Dumping a nsIXPCWrappedJSClass..."));
        wjsc->DebugDump(depth);
        NS_RELEASE(wjsc);
    }
    else if(NS_SUCCEEDED(p->QueryInterface(NS_GET_IID(nsIXPConnectWrappedNative),
                        (void**)&wn)))
    {
        XPC_LOG_ALWAYS(("Dumping a nsIXPConnectWrappedNative..."));
        wn->DebugDump(depth);
        NS_RELEASE(wn);
    }
    else if(NS_SUCCEEDED(p->QueryInterface(NS_GET_IID(nsIXPConnectWrappedJS),
                        (void**)&wjs)))
    {
        XPC_LOG_ALWAYS(("Dumping a nsIXPConnectWrappedJS..."));
        wjs->DebugDump(depth);
        NS_RELEASE(wjs);
    }
    else
        XPC_LOG_ALWAYS(("*** Could not dump the nsISupports @ %x", p));
#endif
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::DebugDumpJSStack(PRBool showArgs,
                              PRBool showLocals,
                              PRBool showThisProps)
{
    JSContext* cx;
    if(NS_FAILED(Peek(&cx)))
        printf("failed to peek into nsIThreadJSContextStack service!\n");
    else if(!cx)
        printf("there is no JSContext on the nsIThreadJSContextStack!\n");
    else
        xpc_DumpJSStack(cx, showArgs, showLocals, showThisProps);

    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::DebugDumpEvalInJSStackFrame(PRUint32 aFrameNumber, const char *aSourceText)
{
    JSContext* cx;
    if(NS_FAILED(Peek(&cx)))
        printf("failed to peek into nsIThreadJSContextStack service!\n");
    else if(!cx)
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
    if(!ccx.IsValid())
        return NS_ERROR_FAILURE;

    nsresult rv = NS_OK;
    if(!XPCVariant::VariantDataToJS(ccx, value, scope, &rv, _retval))
    {
        if(NS_FAILED(rv)) 
            return rv;

        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}


NS_IMETHODIMP 
nsXPConnect::JSToVariant(JSContext* ctx, jsval value, nsIVariant** _retval)
{
    NS_PRECONDITION(ctx, "bad param");
    NS_PRECONDITION(value, "bad param");
    NS_PRECONDITION(_retval, "bad param");

    XPCCallContext ccx(NATIVE_CALLER, ctx);
    if(!ccx.IsValid())
        return NS_ERROR_FAILURE;

    *_retval = XPCVariant::newVariant(ccx, value);
    if(!(*_retval)) 
        return NS_ERROR_FAILURE;

    return NS_OK;
}



NS_IMETHODIMP 
nsXPConnect::FlagSystemFilenamePrefix(const char *aFilenamePrefix,
                                      PRBool aWantNativeWrappers)
{
    NS_PRECONDITION(aFilenamePrefix, "bad param");

    JSRuntime* rt = GetRuntime()->GetJSRuntime();;
    uint32 flags = JSFILENAME_SYSTEM;
    if(aWantNativeWrappers)
        flags |= JSFILENAME_PROTECTED;
    if(!JS_FlagScriptFilenamePrefix(rt, aFilenamePrefix, flags))
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

NS_IMETHODIMP
nsXPConnect::OnProcessNextEvent(nsIThreadInternal *aThread, PRBool aMayWait,
                                PRUint32 aRecursionDepth)
{
    
    
    return Push(nsnull);
}

NS_IMETHODIMP
nsXPConnect::AfterProcessNextEvent(nsIThreadInternal *aThread,
                                   PRUint32 aRecursionDepth)
{
    return Pop(nsnull);
}

NS_IMETHODIMP
nsXPConnect::OnDispatchedEvent(nsIThreadInternal* aThread)
{
    NS_NOTREACHED("Why tell us?");
    return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsXPConnect::AddJSHolder(void* aHolder, nsScriptObjectTracer* aTracer)
{
    return mRuntime->AddJSHolder(aHolder, aTracer);
}

NS_IMETHODIMP
nsXPConnect::RemoveJSHolder(void* aHolder)
{
    return mRuntime->RemoveJSHolder(aHolder);
}

NS_IMETHODIMP
nsXPConnect::SetReportAllJSExceptions(PRBool newval)
{
    
    if (gReportAllJSExceptions != 1)
        gReportAllJSExceptions = newval ? 2 : 0;

    return NS_OK;
}


NS_IMETHODIMP_(PRBool)
nsXPConnect::DefineDOMQuickStubs(JSContext * cx,
                                 JSObject * proto,
                                 PRUint32 flags,
                                 PRUint32 interfaceCount,
                                 const nsIID * *interfaceArray)
{
    return DOM_DefineQuickStubs(cx, proto, flags,
                                interfaceCount, interfaceArray);
}

NS_IMETHODIMP
nsXPConnect::GetWrapperForObject(JSContext* aJSContext,
                                 JSObject* aObject,
                                 JSObject* aScope,
                                 nsIPrincipal* aPrincipal,
                                 PRUint32 aFilenameFlags,
                                 jsval* _retval)
{
    NS_ASSERTION(aFilenameFlags != JSFILENAME_NULL, "Null filename!");
    NS_ASSERTION(XPCPerThreadData::IsMainThread(aJSContext),
                 "Must only be called from the main thread as these wrappers "
                 "are not threadsafe!");

    JSAutoRequest ar(aJSContext);

    XPCWrappedNativeScope *objectscope;
    XPCWrappedNative *wrapper = nsnull;
    if(IS_SLIM_WRAPPER(aObject))
    {
        objectscope = GetSlimWrapperProto(aObject)->GetScope();
    }
    else
    {
        wrapper =
            XPCWrappedNative::GetWrappedNativeOfJSObject(aJSContext, aObject);
        if(!wrapper)
        {
            
            
            *_retval = OBJECT_TO_JSVAL(aObject);
            return NS_OK;
        }
        objectscope = wrapper->GetScope();
    }

    XPCWrappedNativeScope *xpcscope =
            XPCWrappedNativeScope::FindInJSObjectScope(aJSContext, aScope);
    if(!xpcscope)
        return NS_ERROR_FAILURE;

#ifdef DEBUG_mrbkap
    {
        JSObject *scopeobj = xpcscope->GetGlobalJSObject();
        JSObject *toInnerize = scopeobj;
        OBJ_TO_INNER_OBJECT(aJSContext, toInnerize);
        NS_ASSERTION(toInnerize == scopeobj, "Scope chain ending in outer object?");
    }
#endif

    {
        JSObject *possibleOuter = objectscope->GetGlobalJSObject();
        OBJ_TO_INNER_OBJECT(aJSContext, possibleOuter);
        if(!possibleOuter)
            return NS_ERROR_FAILURE;

        if(objectscope->GetGlobalJSObject() != possibleOuter)
        {
            objectscope =
                XPCWrappedNativeScope::FindInJSObjectScope(aJSContext,
                                                           possibleOuter);
            NS_ASSERTION(objectscope, "Unable to find a scope from an object");
        }
    }

    *_retval = OBJECT_TO_JSVAL(aObject);

    JSBool sameOrigin;
    JSBool sameScope = xpc_SameScope(objectscope, xpcscope, &sameOrigin);
    JSBool forceXOW = XPC_XOW_ClassNeedsXOW(STOBJ_GET_CLASS(aObject)->name);

    
    
    
    
    
    
    
    if(STOBJ_IS_SYSTEM(aObject) ||
       (sameScope &&
        (!forceXOW || (aFilenameFlags & JSFILENAME_SYSTEM))))
        return NS_OK;

    if(!wrapper)
    {
        SLIM_LOG_WILL_MORPH(aJSContext, aObject);
        if(!MorphSlimWrapper(aJSContext, aObject))
            return NS_ERROR_FAILURE;

        wrapper = static_cast<XPCWrappedNative*>(xpc_GetJSPrivate(aObject));
    }

    JSObject* wrappedObj = nsnull;

    if(aFilenameFlags & JSFILENAME_PROTECTED)
    {
        wrappedObj = XPCNativeWrapper::GetNewOrUsed(aJSContext, wrapper,
                                                    aPrincipal);
    }
    else if(aFilenameFlags & JSFILENAME_SYSTEM)
    {
        jsval val = OBJECT_TO_JSVAL(aObject);
        if(XPC_SJOW_Construct(aJSContext, nsnull, 1, &val, &val))
            wrappedObj = JSVAL_TO_OBJECT(val);
    }
    else
    {
        
        
        if(sameOrigin && !forceXOW)
            return NS_OK;

        jsval val = OBJECT_TO_JSVAL(aObject);
        if(XPC_XOW_WrapObject(aJSContext, aScope, &val, wrapper))
            wrappedObj = JSVAL_TO_OBJECT(val);
    }

    if(!wrappedObj)
        return NS_ERROR_FAILURE;

    *_retval = OBJECT_TO_JSVAL(wrappedObj);
    if(wrapper->NeedsChromeWrapper() &&
       !XPC_SOW_WrapObject(aJSContext, aScope, *_retval, _retval))
        return NS_ERROR_FAILURE;
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::GetRuntime(JSRuntime **runtime)
{
    if(!runtime)
        return NS_ERROR_NULL_POINTER;

    *runtime = GetRuntime()->GetJSRuntime();
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::GetBackstagePass(nsIXPCScriptable **bsp)
{
    if(!mBackstagePass) {
#ifndef XPCONNECT_STANDALONE
        nsCOMPtr<nsIPrincipal> sysprin;
        nsCOMPtr<nsIScriptSecurityManager> secman =
            do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
        if(!secman)
            return NS_ERROR_NOT_AVAILABLE;
        if(NS_FAILED(secman->GetSystemPrincipal(getter_AddRefs(sysprin))))
            return NS_ERROR_NOT_AVAILABLE;

        mBackstagePass = new BackstagePass(sysprin);
#else
        mBackstagePass = new BackstagePass();
#endif
        if(!mBackstagePass)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(*bsp = mBackstagePass);
    return NS_OK;
}




NS_IMETHODIMP
nsXPConnect::GetCount(PRInt32 *aCount)
{
    if(!aCount)
        return NS_ERROR_NULL_POINTER;

    XPCPerThreadData* data = XPCPerThreadData::GetData(nsnull);

    if(!data)
    {
        *aCount = 0;
        return NS_ERROR_FAILURE;
    }

    return data->GetJSContextStack()->GetCount(aCount);
}


NS_IMETHODIMP
nsXPConnect::Peek(JSContext * *_retval)
{
    if(!_retval)
        return NS_ERROR_NULL_POINTER;

    XPCPerThreadData* data = XPCPerThreadData::GetData(nsnull);

    if(!data)
    {
        *_retval = nsnull;
        return NS_ERROR_FAILURE;
    }

    return data->GetJSContextStack()->Peek(_retval);
}


NS_IMETHODIMP
nsXPConnect::Pop(JSContext * *_retval)
{
    XPCPerThreadData* data = XPCPerThreadData::GetData(nsnull);

    if(!data)
    {
        if(_retval)
            *_retval = nsnull;
        return NS_ERROR_FAILURE;
    }

    return data->GetJSContextStack()->Pop(_retval);
}


NS_IMETHODIMP
nsXPConnect::Push(JSContext * cx)
{
    XPCPerThreadData* data = XPCPerThreadData::GetData(cx);

    if(!data)
        return NS_ERROR_FAILURE;

    return data->GetJSContextStack()->Push(cx);
}


NS_IMETHODIMP
nsXPConnect::GetSafeJSContext(JSContext * *aSafeJSContext)
{
    NS_ASSERTION(aSafeJSContext, "loser!");

    XPCPerThreadData* data = XPCPerThreadData::GetData(nsnull);

    if(!data)
    {
        *aSafeJSContext = nsnull;
        return NS_ERROR_FAILURE;
    }

    return data->GetJSContextStack()->GetSafeJSContext(aSafeJSContext);
}


NS_IMETHODIMP
nsXPConnect::SetSafeJSContext(JSContext * aSafeJSContext)
{
    XPCPerThreadData* data = XPCPerThreadData::GetData(aSafeJSContext);

    if(!data)
        return NS_ERROR_FAILURE;

    return data->GetJSContextStack()->SetSafeJSContext(aSafeJSContext);
}

nsIPrincipal*
nsXPConnect::GetPrincipal(JSObject* obj, PRBool allowShortCircuit) const
{
    NS_ASSERTION(IS_WRAPPER_CLASS(STOBJ_GET_CLASS(obj)) || IS_SLIM_WRAPPER(obj),
                 "What kind of wrapper is this?");

    if(IS_WRAPPER_CLASS(STOBJ_GET_CLASS(obj)))
    {
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
    }
    else
    {
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

    return nsnull;
}


JS_BEGIN_EXTERN_C
JS_EXPORT_API(void) DumpJSStack()
{
    nsresult rv;
    nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
    if(NS_SUCCEEDED(rv) && xpc)
        xpc->DebugDumpJSStack(PR_TRUE, PR_TRUE, PR_FALSE);
    else
        printf("failed to get XPConnect service!\n");
}

JS_EXPORT_API(void) DumpJSEval(PRUint32 frameno, const char* text)
{
    nsresult rv;
    nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
    if(NS_SUCCEEDED(rv) && xpc)
        xpc->DebugDumpEvalInJSStackFrame(frameno, text);
    else
        printf("failed to get XPConnect service!\n");
}

JS_EXPORT_API(void) DumpJSObject(JSObject* obj)
{
    xpc_DumpJSObject(obj);
}

JS_EXPORT_API(void) DumpJSValue(jsval val)
{
    printf("Dumping 0x%p. Value tag is %u.\n", (void *) val, (PRUint32) JSVAL_TAG(val));
    if(JSVAL_IS_NULL(val)) {
        printf("Value is null\n");
    }
    else if(JSVAL_IS_OBJECT(val)) {
        printf("Value is an object\n");
        JSObject* obj = JSVAL_TO_OBJECT(val);
        DumpJSObject(obj);
    }
    else if(JSVAL_IS_NUMBER(val)) {
        printf("Value is a number: ");
        if(JSVAL_IS_INT(val))
          printf("Integer %i\n", JSVAL_TO_INT(val));
        else if(JSVAL_IS_DOUBLE(val))
          printf("Floating-point value %f\n", *JSVAL_TO_DOUBLE(val));
    }
    else if(JSVAL_IS_STRING(val)) {
        printf("Value is a string: ");
        JSString* string = JSVAL_TO_STRING(val);
        char* bytes = JS_GetStringBytes(string);
        printf("<%s>\n", bytes);
    }
    else if(JSVAL_IS_BOOLEAN(val)) {
        printf("Value is boolean: ");
        printf(JSVAL_TO_BOOLEAN(val) ? "true" : "false");
    }
    else if(JSVAL_IS_VOID(val)) {
        printf("Value is undefined\n");
    }
    else {
        printf("No idea what this value is.\n");
    }
}
JS_END_EXTERN_C

