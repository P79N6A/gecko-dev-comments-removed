







#include "mozilla/Assertions.h"
#include "mozilla/Base64.h"
#include "mozilla/Likely.h"

#include "xpcprivate.h"
#include "XPCWrapper.h"
#include "jsfriendapi.h"
#include "nsJSEnvironment.h"
#include "nsThreadUtils.h"
#include "nsDOMJSUtils.h"

#include "WrapperFactory.h"
#include "AccessCheck.h"

#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Exceptions.h"
#include "mozilla/dom/Promise.h"

#include "nsDOMMutationObserver.h"
#include "nsICycleCollectorListener.h"
#include "nsThread.h"
#include "mozilla/XPTInterfaceInfoManager.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsScriptSecurityManager.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace xpc;
using namespace JS;

NS_IMPL_ISUPPORTS(nsXPConnect,
                  nsIXPConnect,
                  nsISupportsWeakReference,
                  nsIThreadObserver,
                  nsIJSRuntimeService)

nsXPConnect* nsXPConnect::gSelf = nullptr;
bool         nsXPConnect::gOnceAliveNowDead = false;
uint32_t     nsXPConnect::gReportAllJSExceptions = 0;



nsIScriptSecurityManager *nsXPConnect::gScriptSecurityManager = nullptr;
nsIPrincipal *nsXPConnect::gSystemPrincipal = nullptr;

const char XPC_CONTEXT_STACK_CONTRACTID[] = "@mozilla.org/js/xpc/ContextStack;1";
const char XPC_RUNTIME_CONTRACTID[]       = "@mozilla.org/js/xpc/RuntimeService;1";
const char XPC_EXCEPTION_CONTRACTID[]     = "@mozilla.org/js/xpc/Exception;1";
const char XPC_CONSOLE_CONTRACTID[]       = "@mozilla.org/consoleservice;1";
const char XPC_SCRIPT_ERROR_CONTRACTID[]  = "@mozilla.org/scripterror;1";
const char XPC_ID_CONTRACTID[]            = "@mozilla.org/js/xpc/ID;1";
const char XPC_XPCONNECT_CONTRACTID[]     = "@mozilla.org/js/xpc/XPConnect;1";



nsXPConnect::nsXPConnect()
    :   mRuntime(nullptr),
        mShuttingDown(false),
        mEventDepth(0)
{
    mRuntime = XPCJSRuntime::newXPCJSRuntime(this);

    char* reportableEnv = PR_GetEnv("MOZ_REPORT_ALL_JS_EXCEPTIONS");
    if (reportableEnv && *reportableEnv)
        gReportAllJSExceptions = 1;
}

nsXPConnect::~nsXPConnect()
{
    mRuntime->DeleteSingletonScopes();
    mRuntime->DestroyJSContextStack();

    
    
    
    
    
    
    JS_GC(mRuntime->Runtime());

    mShuttingDown = true;
    XPCWrappedNativeScope::SystemIsBeingShutDown();
    mRuntime->SystemIsBeingShutDown();

    
    
    
    
    JS_GC(mRuntime->Runtime());

    NS_RELEASE(gSystemPrincipal);
    gScriptSecurityManager = nullptr;

    
    XPC_LOG_FINISH();

    delete mRuntime;

    gSelf = nullptr;
    gOnceAliveNowDead = true;
}


void
nsXPConnect::InitStatics()
{
    gSelf = new nsXPConnect();
    gOnceAliveNowDead = false;
    if (!gSelf->mRuntime) {
        NS_RUNTIMEABORT("Couldn't create XPCJSRuntime.");
    }

    
    
    NS_ADDREF(gSelf);

    
    if (NS_FAILED(nsThread::SetMainThreadObserver(gSelf))) {
        MOZ_CRASH();
    }

    
    nsScriptSecurityManager::InitStatics();
    gScriptSecurityManager = nsScriptSecurityManager::GetScriptSecurityManager();
    gScriptSecurityManager->GetSystemPrincipal(&gSystemPrincipal);
    MOZ_RELEASE_ASSERT(gSystemPrincipal);

    
    gSelf->mRuntime->GetJSContextStack()->InitSafeJSContext();

    
    gSelf->mRuntime->InitSingletonScopes();
}

nsXPConnect*
nsXPConnect::GetSingleton()
{
    nsXPConnect* xpc = nsXPConnect::XPConnect();
    NS_IF_ADDREF(xpc);
    return xpc;
}


void
nsXPConnect::ReleaseXPConnectSingleton()
{
    nsXPConnect* xpc = gSelf;
    if (xpc) {
        nsThread::SetMainThreadObserver(nullptr);

        nsrefcnt cnt;
        NS_RELEASE2(xpc, cnt);
    }
}


XPCJSRuntime*
nsXPConnect::GetRuntimeInstance()
{
    nsXPConnect* xpc = XPConnect();
    return xpc->GetRuntime();
}


bool
nsXPConnect::IsISupportsDescendant(nsIInterfaceInfo* info)
{
    bool found = false;
    if (info)
        info->HasAncestor(&NS_GET_IID(nsISupports), &found);
    return found;
}

void
xpc::ErrorReport::Init(JSErrorReport *aReport, const char *aFallbackMessage,
                       bool aIsChrome, uint64_t aWindowID)
{
    mCategory = aIsChrome ? NS_LITERAL_CSTRING("chrome javascript")
                          : NS_LITERAL_CSTRING("content javascript");
    mWindowID = aWindowID;

    const char16_t* m = static_cast<const char16_t*>(aReport->ucmessage);
    if (m) {
        JSFlatString* name = js::GetErrorTypeName(CycleCollectedJSRuntime::Get()->Runtime(), aReport->exnType);
        if (name) {
            AssignJSFlatString(mErrorMsg, name);
            mErrorMsg.AppendLiteral(": ");
        }
        mErrorMsg.Append(m);
    }

    if (mErrorMsg.IsEmpty() && aFallbackMessage) {
        mErrorMsg.AssignWithConversion(aFallbackMessage);
    }

    if (!aReport->filename) {
        mFileName.SetIsVoid(true);
    } else {
        mFileName.AssignWithConversion(aReport->filename);
    }

    mSourceLine = static_cast<const char16_t*>(aReport->uclinebuf);

    mLineNumber = aReport->lineno;
    mColumn = aReport->column;
    mFlags = aReport->flags;
    mIsMuted = aReport->isMuted;
}

#ifdef PR_LOGGING
static PRLogModuleInfo* gJSDiagnostics;
#endif

void
xpc::ErrorReport::LogToConsole()
{
    
    if (nsContentUtils::DOMWindowDumpEnabled()) {
        nsAutoCString error;
        error.AssignLiteral("JavaScript ");
        if (JSREPORT_IS_STRICT(mFlags))
            error.AppendLiteral("strict ");
        if (JSREPORT_IS_WARNING(mFlags))
            error.AppendLiteral("warning: ");
        else
            error.AppendLiteral("error: ");
        error.Append(NS_LossyConvertUTF16toASCII(mFileName));
        error.AppendLiteral(", line ");
        error.AppendInt(mLineNumber, 10);
        error.AppendLiteral(": ");
        error.Append(NS_LossyConvertUTF16toASCII(mErrorMsg));

        fprintf(stderr, "%s\n", error.get());
        fflush(stderr);
    }

#ifdef PR_LOGGING
    
    if (!gJSDiagnostics)
        gJSDiagnostics = PR_NewLogModule("JSDiagnostics");
    if (gJSDiagnostics) {
        PR_LOG(gJSDiagnostics,
                JSREPORT_IS_WARNING(mFlags) ? PR_LOG_WARNING : PR_LOG_ERROR,
                ("file %s, line %u\n%s", NS_LossyConvertUTF16toASCII(mFileName).get(),
                 mLineNumber, NS_LossyConvertUTF16toASCII(mErrorMsg).get()));
    }
#endif

    
    
    
    nsCOMPtr<nsIConsoleService> consoleService =
      do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    nsCOMPtr<nsIScriptError> errorObject =
      do_CreateInstance("@mozilla.org/scripterror;1");
    NS_ENSURE_TRUE_VOID(consoleService && errorObject);

    nsresult rv = errorObject->InitWithWindowID(mErrorMsg, mFileName, mSourceLine,
                                                mLineNumber, mColumn, mFlags,
                                                mCategory, mWindowID);
    NS_ENSURE_SUCCESS_VOID(rv);
    consoleService->LogMessage(errorObject);

}




nsresult
nsXPConnect::GetInfoForIID(const nsIID * aIID, nsIInterfaceInfo** info)
{
  return XPTInterfaceInfoManager::GetSingleton()->GetInfoForIID(aIID, info);
}

nsresult
nsXPConnect::GetInfoForName(const char * name, nsIInterfaceInfo** info)
{
  nsresult rv = XPTInterfaceInfoManager::GetSingleton()->GetInfoForName(name, info);
  return NS_FAILED(rv) ? NS_OK : NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP
nsXPConnect::GarbageCollect(uint32_t reason)
{
    GetRuntime()->GarbageCollect(reason);
    return NS_OK;
}

bool
xpc_GCThingIsGrayCCThing(void *thing)
{
    return AddToCCKind(js::GCThingTraceKind(thing)) &&
           xpc_IsGrayGCThing(thing);
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





template<typename T>
static inline T UnexpectedFailure(T rv)
{
    NS_ERROR("This is not supposed to fail!");
    return rv;
}

void
xpc::TraceXPCGlobal(JSTracer *trc, JSObject *obj)
{
    if (js::GetObjectClass(obj)->flags & JSCLASS_DOM_GLOBAL)
        mozilla::dom::TraceProtoAndIfaceCache(trc, obj);

    
    
    
    xpc::CompartmentPrivate* compartmentPrivate = xpc::CompartmentPrivate::Get(obj);
    if (compartmentPrivate && compartmentPrivate->scope)
        compartmentPrivate->scope->TraceInside(trc);
}

namespace xpc {

JSObject*
CreateGlobalObject(JSContext *cx, const JSClass *clasp, nsIPrincipal *principal,
                   JS::CompartmentOptions& aOptions)
{
    MOZ_ASSERT(NS_IsMainThread(), "using a principal off the main thread?");
    MOZ_ASSERT(principal);

    MOZ_RELEASE_ASSERT(principal != nsContentUtils::GetNullSubjectPrincipal(),
                       "The null subject principal is getting inherited - fix that!");

    RootedObject global(cx,
                        JS_NewGlobalObject(cx, clasp, nsJSPrincipals::get(principal),
                                           JS::DontFireOnNewGlobalHook, aOptions));
    if (!global)
        return nullptr;
    JSAutoCompartment ac(cx, global);

    
    
    (void) new XPCWrappedNativeScope(cx, global);

#ifdef DEBUG
    
    
    
    if (!((const js::Class*)clasp)->ext.isWrappedNative)
    {
        VerifyTraceProtoAndIfaceCacheCalledTracer trc(JS_GetRuntime(cx));
        JS_TraceChildren(&trc, global, JSTRACE_OBJECT);
        MOZ_ASSERT(trc.ok, "Trace hook on global needs to call TraceXPCGlobal for XPConnect compartments.");
    }
#endif

    if (clasp->flags & JSCLASS_DOM_GLOBAL) {
        const char* className = clasp->name;
        AllocateProtoAndIfaceCache(global,
                                   (strcmp(className, "Window") == 0 ||
                                    strcmp(className, "ChromeWindow") == 0)
                                   ? ProtoAndIfaceCache::WindowLike
                                   : ProtoAndIfaceCache::NonWindowLike);
    }

    return global;
}

bool
InitGlobalObject(JSContext* aJSContext, JS::Handle<JSObject*> aGlobal, uint32_t aFlags)
{
    
    
    JSAutoCompartment ac(aJSContext, aGlobal);
    if (!(aFlags & nsIXPConnect::OMIT_COMPONENTS_OBJECT)) {
        
        if (!CompartmentPrivate::Get(aGlobal)->scope->AttachComponentsObject(aJSContext) ||
            !XPCNativeWrapper::AttachNewConstructorObject(aJSContext, aGlobal)) {
            return UnexpectedFailure(false);
        }
    }

    if (ShouldDiscardSystemSource()) {
        nsIPrincipal *prin = GetObjectPrincipal(aGlobal);
        bool isSystem = nsContentUtils::IsSystemPrincipal(prin);
        if (!isSystem) {
            short status = prin->GetAppStatus();
            isSystem = status == nsIPrincipal::APP_STATUS_PRIVILEGED ||
                       status == nsIPrincipal::APP_STATUS_CERTIFIED;
        }
        JS::CompartmentOptionsRef(aGlobal).setDiscardSource(isSystem);
    }

    if (ExtraWarningsForSystemJS()) {
        nsIPrincipal *prin = GetObjectPrincipal(aGlobal);
        bool isSystem = nsContentUtils::IsSystemPrincipal(prin);
        if (isSystem)
            JS::CompartmentOptionsRef(aGlobal).extraWarningsOverride().set(true);
    }

    
    MOZ_ASSERT(js::GetObjectClass(aGlobal)->flags & JSCLASS_DOM_GLOBAL);

    if (!(aFlags & nsIXPConnect::DONT_FIRE_ONNEWGLOBALHOOK))
        JS_FireOnNewGlobalObject(aJSContext, aGlobal);

    return true;
}

} 

NS_IMETHODIMP
nsXPConnect::InitClassesWithNewWrappedGlobal(JSContext * aJSContext,
                                             nsISupports *aCOMObj,
                                             nsIPrincipal * aPrincipal,
                                             uint32_t aFlags,
                                             JS::CompartmentOptions& aOptions,
                                             nsIXPConnectJSObjectHolder **_retval)
{
    MOZ_ASSERT(aJSContext, "bad param");
    MOZ_ASSERT(aCOMObj, "bad param");
    MOZ_ASSERT(_retval, "bad param");

    
    
    MOZ_ASSERT(aPrincipal);

    
    
    xpcObjectHelper helper(aCOMObj);
    MOZ_ASSERT(helper.GetScriptableFlags() & nsIXPCScriptable::IS_GLOBAL_OBJECT);
    nsRefPtr<XPCWrappedNative> wrappedGlobal;
    nsresult rv =
        XPCWrappedNative::WrapNewGlobal(helper, aPrincipal,
                                        aFlags & nsIXPConnect::INIT_JS_STANDARD_CLASSES,
                                        aOptions, getter_AddRefs(wrappedGlobal));
    NS_ENSURE_SUCCESS(rv, rv);

    
    RootedObject global(aJSContext, wrappedGlobal->GetFlatJSObject());
    MOZ_ASSERT(!js::GetObjectParent(global));

    if (!InitGlobalObject(aJSContext, global, aFlags))
        return UnexpectedFailure(NS_ERROR_FAILURE);

    wrappedGlobal.forget(_retval);
    return NS_OK;
}

static nsresult
NativeInterface2JSObject(HandleObject aScope,
                         nsISupports *aCOMObj,
                         nsWrapperCache *aCache,
                         const nsIID * aIID,
                         bool aAllowWrapping,
                         MutableHandleValue aVal,
                         nsIXPConnectJSObjectHolder **aHolder)
{
    AutoJSContext cx;
    JSAutoCompartment ac(cx, aScope);

    nsresult rv;
    xpcObjectHelper helper(aCOMObj, aCache);
    if (!XPCConvert::NativeInterface2JSObject(aVal, aHolder, helper, aIID,
                                              nullptr, aAllowWrapping, &rv))
        return rv;

    MOZ_ASSERT(aAllowWrapping || !xpc::WrapperFactory::IsXrayWrapper(&aVal.toObject()),
               "Shouldn't be returning a xray wrapper here");

    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::WrapNative(JSContext * aJSContext,
                        JSObject * aScopeArg,
                        nsISupports *aCOMObj,
                        const nsIID & aIID,
                        nsIXPConnectJSObjectHolder **aHolder)
{
    MOZ_ASSERT(aHolder, "bad param");
    MOZ_ASSERT(aJSContext, "bad param");
    MOZ_ASSERT(aScopeArg, "bad param");
    MOZ_ASSERT(aCOMObj, "bad param");

    RootedObject aScope(aJSContext, aScopeArg);
    RootedValue v(aJSContext);
    return NativeInterface2JSObject(aScope, aCOMObj, nullptr, &aIID,
                                    true, &v, aHolder);
}


NS_IMETHODIMP
nsXPConnect::WrapNativeToJSVal(JSContext *aJSContext,
                               JSObject *aScopeArg,
                               nsISupports *aCOMObj,
                               nsWrapperCache *aCache,
                               const nsIID *aIID,
                               bool aAllowWrapping,
                               MutableHandleValue aVal)
{
    MOZ_ASSERT(aJSContext, "bad param");
    MOZ_ASSERT(aScopeArg, "bad param");
    MOZ_ASSERT(aCOMObj, "bad param");

    RootedObject aScope(aJSContext, aScopeArg);
    return NativeInterface2JSObject(aScope, aCOMObj, aCache, aIID,
                                    aAllowWrapping, aVal, nullptr);
}


NS_IMETHODIMP
nsXPConnect::WrapJS(JSContext * aJSContext,
                    JSObject * aJSObjArg,
                    const nsIID & aIID,
                    void * *result)
{
    MOZ_ASSERT(aJSContext, "bad param");
    MOZ_ASSERT(aJSObjArg, "bad param");
    MOZ_ASSERT(result, "bad param");

    *result = nullptr;

    RootedObject aJSObj(aJSContext, aJSObjArg);
    JSAutoCompartment ac(aJSContext, aJSObj);

    nsresult rv = NS_ERROR_UNEXPECTED;
    if (!XPCConvert::JSObject2NativeInterface(result, aJSObj,
                                              &aIID, nullptr, &rv))
        return rv;
    return NS_OK;
}

NS_IMETHODIMP
nsXPConnect::JSValToVariant(JSContext *cx,
                            HandleValue aJSVal,
                            nsIVariant **aResult)
{
    NS_PRECONDITION(aResult, "bad param");

    nsRefPtr<XPCVariant> variant = XPCVariant::newVariant(cx, aJSVal);
    variant.forget(aResult);
    NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);

    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::WrapJSAggregatedToNative(nsISupports *aOuter,
                                      JSContext *aJSContext,
                                      JSObject *aJSObjArg,
                                      const nsIID &aIID,
                                      void **result)
{
    MOZ_ASSERT(aOuter, "bad param");
    MOZ_ASSERT(aJSContext, "bad param");
    MOZ_ASSERT(aJSObjArg, "bad param");
    MOZ_ASSERT(result, "bad param");

    *result = nullptr;

    RootedObject aJSObj(aJSContext, aJSObjArg);
    nsresult rv;
    if (!XPCConvert::JSObject2NativeInterface(result, aJSObj,
                                              &aIID, aOuter, &rv))
        return rv;
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::GetWrappedNativeOfJSObject(JSContext * aJSContext,
                                        JSObject * aJSObjArg,
                                        nsIXPConnectWrappedNative **_retval)
{
    MOZ_ASSERT(aJSContext, "bad param");
    MOZ_ASSERT(aJSObjArg, "bad param");
    MOZ_ASSERT(_retval, "bad param");

    RootedObject aJSObj(aJSContext, aJSObjArg);
    aJSObj = js::CheckedUnwrap(aJSObj,  false);
    if (!aJSObj || !IS_WN_REFLECTOR(aJSObj)) {
        *_retval = nullptr;
        return NS_ERROR_FAILURE;
    }

    nsRefPtr<XPCWrappedNative> temp = XPCWrappedNative::Get(aJSObj);
    temp.forget(_retval);
    return NS_OK;
}

nsISupports*
xpc::UnwrapReflectorToISupports(JSObject *reflector)
{
    
    reflector = js::CheckedUnwrap(reflector,  false);
    if (!reflector)
        return nullptr;

    
    if (IS_WN_REFLECTOR(reflector)) {
        XPCWrappedNative *wn = XPCWrappedNative::Get(reflector);
        if (!wn)
            return nullptr;
        return wn->Native();
    }

    
    nsCOMPtr<nsISupports> canonical =
        do_QueryInterface(mozilla::dom::UnwrapDOMObjectToISupports(reflector));
    return canonical;
}


NS_IMETHODIMP_(nsISupports*)
nsXPConnect::GetNativeOfWrapper(JSContext *aJSContext,
                                JSObject *aJSObj)
{
    return UnwrapReflectorToISupports(aJSObj);
}


NS_IMETHODIMP
nsXPConnect::GetWrappedNativeOfNativeObject(JSContext * aJSContext,
                                            JSObject * aScopeArg,
                                            nsISupports *aCOMObj,
                                            const nsIID & aIID,
                                            nsIXPConnectWrappedNative **_retval)
{
    MOZ_ASSERT(aJSContext, "bad param");
    MOZ_ASSERT(aScopeArg, "bad param");
    MOZ_ASSERT(aCOMObj, "bad param");
    MOZ_ASSERT(_retval, "bad param");

    *_retval = nullptr;

    RootedObject aScope(aJSContext, aScopeArg);

    XPCWrappedNativeScope* scope = ObjectScope(aScope);
    if (!scope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    AutoMarkingNativeInterfacePtr iface(aJSContext);
    iface = XPCNativeInterface::GetNewOrUsed(&aIID);
    if (!iface)
        return NS_ERROR_FAILURE;

    XPCWrappedNative* wrapper;

    nsresult rv = XPCWrappedNative::GetUsedOnly(aCOMObj, scope, iface, &wrapper);
    if (NS_FAILED(rv))
        return NS_ERROR_FAILURE;
    *_retval = static_cast<nsIXPConnectWrappedNative*>(wrapper);
    return NS_OK;
}





NS_IMETHODIMP
nsXPConnect::ReparentWrappedNativeIfFound(JSContext * aJSContext,
                                          JSObject * aScopeArg,
                                          JSObject * aNewParentArg,
                                          nsISupports *aCOMObj)
{
    RootedObject aScope(aJSContext, aScopeArg);
    RootedObject aNewParent(aJSContext, aNewParentArg);

    XPCWrappedNativeScope* scope = ObjectScope(aScope);
    XPCWrappedNativeScope* scope2 = ObjectScope(aNewParent);
    if (!scope || !scope2)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    RootedObject newParent(aJSContext, aNewParent);
    return XPCWrappedNative::
        ReparentWrapperIfFound(scope, scope2, newParent, aCOMObj);
}

static PLDHashOperator
MoveableWrapperFinder(PLDHashTable *table, PLDHashEntryHdr *hdr,
                      uint32_t number, void *arg)
{
    nsTArray<nsRefPtr<XPCWrappedNative> > *array =
        static_cast<nsTArray<nsRefPtr<XPCWrappedNative> > *>(arg);
    XPCWrappedNative *wn = ((Native2WrappedNativeMap::Entry*)hdr)->value;

    
    
    if (!wn->IsWrapperExpired())
        array->AppendElement(wn);
    return PL_DHASH_NEXT;
}


NS_IMETHODIMP
nsXPConnect::RescueOrphansInScope(JSContext *aJSContext, JSObject *aScopeArg)
{
    RootedObject aScope(aJSContext, aScopeArg);

    XPCWrappedNativeScope *scope = ObjectScope(aScope);
    if (!scope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    
    
    nsTArray<nsRefPtr<XPCWrappedNative> > wrappersToMove;

    Native2WrappedNativeMap *map = scope->GetWrappedNativeMap();
    wrappersToMove.SetCapacity(map->Count());
    map->Enumerate(MoveableWrapperFinder, &wrappersToMove);

    
    for (uint32_t i = 0, stop = wrappersToMove.Length(); i < stop; ++i) {
        nsresult rv = wrappersToMove[i]->RescueOrphans();
        NS_ENSURE_SUCCESS(rv, rv);
    }

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
    MOZ_ASSERT(_retval, "bad param");

    nsCOMPtr<nsIStackFrame> stackFrame =
        exceptions::CreateStackFrameLocation(aLanguage,
                                             aFilename,
                                             aFunctionName,
                                             aLineNumber,
                                             aCaller);
    stackFrame.forget(_retval);
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::GetCurrentJSStack(nsIStackFrame * *aCurrentJSStack)
{
    MOZ_ASSERT(aCurrentJSStack, "bad param");

    nsCOMPtr<nsIStackFrame> currentStack = dom::GetCurrentJSStack();
    currentStack.forget(aCurrentJSStack);

    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::GetCurrentNativeCallContext(nsAXPCNativeCallContext * *aCurrentNativeCallContext)
{
    MOZ_ASSERT(aCurrentNativeCallContext, "bad param");

    *aCurrentNativeCallContext = XPCJSRuntime::Get()->GetCallContext();
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::SetFunctionThisTranslator(const nsIID & aIID,
                                       nsIXPCFunctionThisTranslator *aTranslator)
{
    XPCJSRuntime* rt = GetRuntime();
    IID2ThisTranslatorMap* map = rt->GetThisTranslatorMap();
    map->Add(aIID, aTranslator);
    return NS_OK;
}

NS_IMETHODIMP
nsXPConnect::CreateSandbox(JSContext *cx, nsIPrincipal *principal,
                           nsIXPConnectJSObjectHolder **_retval)
{
    *_retval = nullptr;

    RootedValue rval(cx);
    SandboxOptions options;
    nsresult rv = CreateSandboxObject(cx, &rval, principal, options);
    MOZ_ASSERT(NS_FAILED(rv) || !rval.isPrimitive(),
               "Bad return value from xpc_CreateSandboxObject()!");

    if (NS_SUCCEEDED(rv) && !rval.isPrimitive()) {
        *_retval = XPCJSObjectHolder::newHolder(rval.toObjectOrNull());
        NS_ENSURE_TRUE(*_retval, NS_ERROR_OUT_OF_MEMORY);

        NS_ADDREF(*_retval);
    }

    return rv;
}

NS_IMETHODIMP
nsXPConnect::EvalInSandboxObject(const nsAString& source, const char *filename,
                                 JSContext *cx, JSObject *sandboxArg,
                                 MutableHandleValue rval)
{
    if (!sandboxArg)
        return NS_ERROR_INVALID_ARG;

    RootedObject sandbox(cx, sandboxArg);
    nsCString filenameStr;
    if (filename) {
        filenameStr.Assign(filename);
    } else {
        filenameStr = NS_LITERAL_CSTRING("x-bogus://XPConnect/Sandbox");
    }
    return EvalInSandbox(cx, sandbox, source, filenameStr, 1,
                         JSVERSION_DEFAULT, rval);
}


NS_IMETHODIMP
nsXPConnect::GetWrappedNativePrototype(JSContext * aJSContext,
                                       JSObject * aScopeArg,
                                       nsIClassInfo *aClassInfo,
                                       nsIXPConnectJSObjectHolder **_retval)
{
    RootedObject aScope(aJSContext, aScopeArg);
    JSAutoCompartment ac(aJSContext, aScope);

    XPCWrappedNativeScope* scope = ObjectScope(aScope);
    if (!scope)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    XPCNativeScriptableCreateInfo sciProto;
    XPCWrappedNative::GatherProtoScriptableCreateInfo(aClassInfo, sciProto);

    AutoMarkingWrappedNativeProtoPtr proto(aJSContext);
    proto = XPCWrappedNativeProto::GetNewOrUsed(scope, aClassInfo, &sciProto);
    if (!proto)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    nsIXPConnectJSObjectHolder* holder;
    *_retval = holder = XPCJSObjectHolder::newHolder(proto->GetJSProtoObject());
    if (!holder)
        return UnexpectedFailure(NS_ERROR_FAILURE);

    NS_ADDREF(holder);
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
    xpc_DumpJSStack(showArgs, showLocals, showThisProps);

    return NS_OK;
}

char*
nsXPConnect::DebugPrintJSStack(bool showArgs,
                               bool showLocals,
                               bool showThisProps)
{
    JSContext* cx = GetCurrentJSContext();
    if (!cx)
        printf("there is no JSContext on the nsIThreadJSContextStack!\n");
    else
        return xpc_PrintJSStack(cx, showArgs, showLocals, showThisProps);

    return nullptr;
}


NS_IMETHODIMP
nsXPConnect::VariantToJS(JSContext* ctx, JSObject* scopeArg, nsIVariant* value,
                         MutableHandleValue _retval)
{
    NS_PRECONDITION(ctx, "bad param");
    NS_PRECONDITION(scopeArg, "bad param");
    NS_PRECONDITION(value, "bad param");

    RootedObject scope(ctx, scopeArg);
    MOZ_ASSERT(js::IsObjectInContextCompartment(scope, ctx));

    nsresult rv = NS_OK;
    if (!XPCVariant::VariantDataToJS(value, &rv, _retval)) {
        if (NS_FAILED(rv))
            return rv;

        return NS_ERROR_FAILURE;
    }
    return NS_OK;
}


NS_IMETHODIMP
nsXPConnect::JSToVariant(JSContext* ctx, HandleValue value, nsIVariant** _retval)
{
    NS_PRECONDITION(ctx, "bad param");
    NS_PRECONDITION(_retval, "bad param");

    nsRefPtr<XPCVariant> variant = XPCVariant::newVariant(ctx, value);
    variant.forget(_retval);
    if (!(*_retval))
        return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP
nsXPConnect::OnProcessNextEvent(nsIThreadInternal *aThread, bool aMayWait,
                                uint32_t aRecursionDepth)
{
    
    
    
    if (aMayWait) {
        Promise::PerformMicroTaskCheckpoint();
    }

    
    mEventDepth++;

    
    
    MOZ_ASSERT(NS_IsMainThread());
    bool ok = PushJSContextNoScriptContext(nullptr);
    NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);
    return NS_OK;
}

NS_IMETHODIMP
nsXPConnect::AfterProcessNextEvent(nsIThreadInternal *aThread,
                                   uint32_t aRecursionDepth,
                                   bool aEventWasProcessed)
{
    
    if (MOZ_UNLIKELY(mEventDepth == 0))
        return NS_OK;
    mEventDepth--;

    
    mRuntime->OnAfterProcessNextEvent();

    
    MOZ_ASSERT(NS_IsMainThread());
    nsJSContext::MaybePokeCC();

    nsContentUtils::PerformMainThreadMicroTaskCheckpoint();

    Promise::PerformMicroTaskCheckpoint();

    PopJSContextNoScriptContext();

    return NS_OK;
}

NS_IMETHODIMP
nsXPConnect::OnDispatchedEvent(nsIThreadInternal* aThread)
{
    NS_NOTREACHED("Why tell us?");
    return NS_ERROR_UNEXPECTED;
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

    JSRuntime *rt = GetRuntime()->Runtime();
    JS_AbortIfWrongThread(rt);
    *runtime = rt;
    return NS_OK;
}


NS_IMETHODIMP_(void)
nsXPConnect::RegisterGCCallback(xpcGCCallback func)
{
    mRuntime->AddGCCallback(func);
}


NS_IMETHODIMP_(void)
nsXPConnect::UnregisterGCCallback(xpcGCCallback func)
{
    mRuntime->RemoveGCCallback(func);
}


NS_IMETHODIMP_(void)
nsXPConnect::RegisterContextCallback(xpcContextCallback func)
{
    mRuntime->AddContextCallback(func);
}


NS_IMETHODIMP_(void)
nsXPConnect::UnregisterContextCallback(xpcContextCallback func)
{
    mRuntime->RemoveContextCallback(func);
}


JSContext*
nsXPConnect::GetCurrentJSContext()
{
    return GetRuntime()->GetJSContextStack()->Peek();
}


JSContext*
nsXPConnect::GetSafeJSContext()
{
    return GetRuntime()->GetJSContextStack()->GetSafeJSContext();
}

namespace xpc {

bool
PushJSContextNoScriptContext(JSContext *aCx)
{
    MOZ_ASSERT_IF(aCx, !GetScriptContextFromJSContext(aCx));
    return XPCJSRuntime::Get()->GetJSContextStack()->Push(aCx);
}

void
PopJSContextNoScriptContext()
{
    XPCJSRuntime::Get()->GetJSContextStack()->Pop();
}

} 

nsIPrincipal*
nsXPConnect::GetPrincipal(JSObject* obj, bool allowShortCircuit) const
{
    MOZ_ASSERT(IS_WN_REFLECTOR(obj), "What kind of wrapper is this?");

    XPCWrappedNative *xpcWrapper = XPCWrappedNative::Get(obj);
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

    return nullptr;
}

NS_IMETHODIMP
nsXPConnect::HoldObject(JSContext *aJSContext, JSObject *aObjectArg,
                        nsIXPConnectJSObjectHolder **aHolder)
{
    RootedObject aObject(aJSContext, aObjectArg);
    XPCJSObjectHolder* objHolder = XPCJSObjectHolder::newHolder(aObject);
    if (!objHolder)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aHolder = objHolder);
    return NS_OK;
}

namespace xpc {

bool
Base64Encode(JSContext *cx, HandleValue val, MutableHandleValue out)
{
    MOZ_ASSERT(cx);

    nsAutoCString encodedString;
    if (!ConvertJSValueToByteString(cx, val, false, encodedString)) {
        return false;
    }

    nsAutoCString result;
    if (NS_FAILED(mozilla::Base64Encode(encodedString, result))) {
        JS_ReportError(cx, "Failed to encode base64 data!");
        return false;
    }

    JSString *str = JS_NewStringCopyN(cx, result.get(), result.Length());
    if (!str)
        return false;

    out.setString(str);
    return true;
}

bool
Base64Decode(JSContext *cx, HandleValue val, MutableHandleValue out)
{
    MOZ_ASSERT(cx);

    nsAutoCString encodedString;
    if (!ConvertJSValueToByteString(cx, val, false, encodedString)) {
        return false;
    }

    nsAutoCString result;
    if (NS_FAILED(mozilla::Base64Decode(encodedString, result))) {
        JS_ReportError(cx, "Failed to decode base64 string!");
        return false;
    }

    JSString *str = JS_NewStringCopyN(cx, result.get(), result.Length());
    if (!str)
        return false;

    out.setString(str);
    return true;
}

void
SetLocationForGlobal(JSObject *global, const nsACString& location)
{
    MOZ_ASSERT(global);
    CompartmentPrivate::Get(global)->SetLocation(location);
}

void
SetLocationForGlobal(JSObject *global, nsIURI *locationURI)
{
    MOZ_ASSERT(global);
    CompartmentPrivate::Get(global)->SetLocationURI(locationURI);
}

} 

NS_IMETHODIMP
nsXPConnect::NotifyDidPaint()
{
    JS::NotifyDidPaint(GetRuntime()->Runtime());
    return NS_OK;
}

static nsresult
WriteScriptOrFunction(nsIObjectOutputStream *stream, JSContext *cx,
                      JSScript *scriptArg, HandleObject functionObj)
{
    
    MOZ_ASSERT(!scriptArg != !functionObj);

    RootedScript script(cx, scriptArg);
    if (!script) {
        RootedFunction fun(cx, JS_GetObjectFunction(functionObj));
        script.set(JS_GetFunctionScript(cx, fun));
    }

    uint8_t flags = 0; 
    nsresult rv = stream->Write8(flags);
    if (NS_FAILED(rv))
        return rv;


    uint32_t size;
    void* data;
    {
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

    
    
    
    MOZ_RELEASE_ASSERT(nsContentUtils::IsCallerChrome() ||
                       CurrentGlobalOrNull(cx) == xpc::CompilationScope());

    uint32_t size;
    rv = stream->Read32(&size);
    if (NS_FAILED(rv))
        return rv;

    char* data;
    rv = stream->ReadBytes(size, &data);
    if (NS_FAILED(rv))
        return rv;

    {
        if (scriptp) {
            JSScript *script = JS_DecodeScript(cx, data, size);
            if (!script)
                rv = NS_ERROR_OUT_OF_MEMORY;
            else
                *scriptp = script;
        } else {
            JSObject *funobj = JS_DecodeInterpretedFunction(cx, data, size);
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
    return WriteScriptOrFunction(stream, cx, script, NullPtr());
}

NS_IMETHODIMP
nsXPConnect::ReadScript(nsIObjectInputStream *stream, JSContext *cx, JSScript **scriptp)
{
    return ReadScriptOrFunction(stream, cx, scriptp, nullptr);
}

NS_IMETHODIMP
nsXPConnect::WriteFunction(nsIObjectOutputStream *stream, JSContext *cx, JSObject *functionObjArg)
{
    RootedObject functionObj(cx, functionObjArg);
    return WriteScriptOrFunction(stream, cx, nullptr, functionObj);
}

NS_IMETHODIMP
nsXPConnect::ReadFunction(nsIObjectInputStream *stream, JSContext *cx, JSObject **functionObjp)
{
    return ReadScriptOrFunction(stream, cx, nullptr, functionObjp);
}


extern "C" {
JS_EXPORT_API(void) DumpJSStack()
{
    xpc_DumpJSStack(true, true, false);
}

JS_EXPORT_API(char*) PrintJSStack()
{
    nsresult rv;
    nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
    return (NS_SUCCEEDED(rv) && xpc) ?
        xpc->DebugPrintJSStack(true, true, false) :
        nullptr;
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

} 

namespace xpc {

bool
Atob(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (!args.length())
        return true;

    return xpc::Base64Decode(cx, args[0], args.rval());
}

bool
Btoa(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (!args.length())
        return true;

    return xpc::Base64Encode(cx, args[0], args.rval());
}

bool
IsXrayWrapper(JSObject *obj)
{
    return WrapperFactory::IsXrayWrapper(obj);
}

JSAddonId *
NewAddonId(JSContext *cx, const nsACString &id)
{
    JS::RootedString str(cx, JS_NewStringCopyN(cx, id.BeginReading(), id.Length()));
    if (!str)
        return nullptr;
    return JS::NewAddonId(cx, str);
}

bool
SetAddonInterposition(const nsACString &addonIdStr, nsIAddonInterposition *interposition)
{
    JSAddonId *addonId;
    {
        
        
        AutoJSAPI jsapi;
        jsapi.Init(xpc::PrivilegedJunkScope());
        addonId = NewAddonId(jsapi.cx(), addonIdStr);
        if (!addonId)
            return false;
    }

    return XPCWrappedNativeScope::SetAddonInterposition(addonId, interposition);
}

} 

namespace mozilla {
namespace dom {

bool
IsChromeOrXBL(JSContext* cx, JSObject* )
{
    MOZ_ASSERT(NS_IsMainThread());
    JSCompartment* c = js::GetContextCompartment(cx);

    
    
    
    
    
    
    return AccessCheck::isChrome(c) || IsContentXBLScope(c) || !AllowContentXBLScope(c);
}

} 
} 
