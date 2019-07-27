





#include "nsError.h"
#include "nsJSEnvironment.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIDOMChromeWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIScriptSecurityManager.h"
#include "nsDOMCID.h"
#include "nsIServiceManager.h"
#include "nsIXPConnect.h"
#include "nsIJSRuntimeService.h"
#include "nsCOMPtr.h"
#include "nsISupportsPrimitives.h"
#include "nsReadableUtils.h"
#include "nsDOMJSUtils.h"
#include "nsJSUtils.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsPresContext.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIPrompt.h"
#include "nsIObserverService.h"
#include "nsITimer.h"
#include "nsIAtom.h"
#include "nsContentUtils.h"
#include "mozilla/EventDispatcher.h"
#include "nsIContent.h"
#include "nsCycleCollector.h"
#include "nsNetUtil.h"
#include "nsXPCOMCIDInternal.h"
#include "nsIXULRuntime.h"
#include "nsTextFormatter.h"
#include "ScriptSettings.h"

#include "xpcpublic.h"

#include "jsapi.h"
#include "jswrapper.h"
#include "js/SliceBudget.h"
#include "nsIArray.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "prmem.h"
#include "WrapperFactory.h"
#include "nsGlobalWindow.h"
#include "nsScriptNameSpaceManager.h"
#include "StructuredCloneTags.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/dom/CryptoKey.h"
#include "mozilla/dom/ErrorEvent.h"
#include "mozilla/dom/ImageDataBinding.h"
#include "mozilla/dom/ImageData.h"
#ifdef MOZ_NFC
#include "mozilla/dom/MozNDEFRecord.h"
#endif 
#include "mozilla/dom/StructuredClone.h"
#include "mozilla/dom/SubtleCryptoBinding.h"
#include "mozilla/ipc/BackgroundUtils.h"
#include "mozilla/ipc/PBackgroundSharedTypes.h"
#include "nsAXPCNativeCallContext.h"
#include "mozilla/CycleCollectedJSRuntime.h"

#include "nsJSPrincipals.h"

#ifdef XP_MACOSX

#undef check
#endif
#include "AccessCheck.h"

#include "prlog.h"
#include "prthread.h"

#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/asmjscache/AsmJSCache.h"
#include "mozilla/dom/CanvasRenderingContext2DBinding.h"
#include "mozilla/CycleCollectedJSRuntime.h"
#include "mozilla/ContentEvents.h"

#include "nsCycleCollectionNoteRootCallback.h"
#include "GeckoProfiler.h"

using namespace mozilla;
using namespace mozilla::dom;

const size_t gStackSize = 8192;


#ifdef CompareString
#undef CompareString
#endif

#define NS_SHRINK_GC_BUFFERS_DELAY  4000 // ms



#define NS_FIRST_GC_DELAY           10000 // ms

#define NS_FULL_GC_DELAY            60000 // ms



#define NS_SHRINKING_GC_DELAY       15000 // ms


#define NS_INTERSLICE_GC_DELAY      100 // ms


#define NS_INTERSLICE_GC_BUDGET     40 // ms



#define NS_CC_DELAY                 6000 // ms

#define NS_CC_SKIPPABLE_DELAY       250 // ms


static const int64_t kICCIntersliceDelay = 32; 


static const int64_t kICCSliceBudget = 5; 


static const uint32_t kMaxICCDuration = 2000; 



#define NS_CC_FORCED                (2 * 60 * PR_USEC_PER_SEC) // 2 min
#define NS_CC_FORCED_PURPLE_LIMIT   10


#define NS_MAX_CC_LOCKEDOUT_TIME    (15 * PR_USEC_PER_SEC) // 15 seconds


#define NS_CC_PURPLE_LIMIT          200


#define NS_UNLIMITED_SCRIPT_RUNTIME (0x40000000LL << 32)



static nsITimer *sGCTimer;
static nsITimer *sShrinkGCBuffersTimer;
static nsITimer *sShrinkingGCTimer;
static nsITimer *sCCTimer;
static nsITimer *sICCTimer;
static nsITimer *sFullGCTimer;
static nsITimer *sInterSliceGCTimer;

static TimeStamp sLastCCEndTime;

static bool sCCLockedOut;
static PRTime sCCLockedOutTime;

static JS::GCSliceCallback sPrevGCSliceCallback;

static bool sHasRunGC;







static uint32_t sPendingLoadCount;
static bool sLoadingInProgress;

static uint32_t sCCollectedWaitingForGC;
static uint32_t sCCollectedZonesWaitingForGC;
static uint32_t sLikelyShortLivingObjectsNeedingGC;
static bool sPostGCEventsToConsole;
static bool sPostGCEventsToObserver;
static int32_t sCCTimerFireCount = 0;
static uint32_t sMinForgetSkippableTime = UINT32_MAX;
static uint32_t sMaxForgetSkippableTime = 0;
static uint32_t sTotalForgetSkippableTime = 0;
static uint32_t sRemovedPurples = 0;
static uint32_t sForgetSkippableBeforeCC = 0;
static uint32_t sPreviousSuspectedCount = 0;
static uint32_t sCleanupsSinceLastGC = UINT32_MAX;
static bool sNeedsFullCC = false;
static bool sNeedsGCAfterCC = false;
static bool sIncrementalCC = false;
static bool sDidPaintAfterPreviousICCSlice = false;

static nsScriptNameSpaceManager *gNameSpaceManager;

static nsIJSRuntimeService *sRuntimeService;

static const char kJSRuntimeServiceContractID[] =
  "@mozilla.org/js/xpc/RuntimeService;1";

static PRTime sFirstCollectionTime;

static JSRuntime *sRuntime;

static bool sIsInitialized;
static bool sDidShutdown;
static bool sShuttingDown;
static int32_t sContextCount;

static nsIScriptSecurityManager *sSecurityManager;





static bool sGCOnMemoryPressure;





static bool sCompactOnUserInactive;
static bool sIsCompactingOnUserInactive = false;




static int32_t sExpensiveCollectorPokes = 0;
static const int32_t kPokesBetweenExpensiveCollectorTriggers = 5;

static PRTime
GetCollectionTimeDelta()
{
  PRTime now = PR_Now();
  if (sFirstCollectionTime) {
    return now - sFirstCollectionTime;
  }
  sFirstCollectionTime = now;
  return 0;
}

static void
KillTimers()
{
  nsJSContext::KillGCTimer();
  nsJSContext::KillShrinkingGCTimer();
  nsJSContext::KillShrinkGCBuffersTimer();
  nsJSContext::KillCCTimer();
  nsJSContext::KillICCTimer();
  nsJSContext::KillFullGCTimer();
  nsJSContext::KillInterSliceGCTimer();
}



static bool
NeedsGCAfterCC()
{
  return sCCollectedWaitingForGC > 250 ||
    sCCollectedZonesWaitingForGC > 0 ||
    sLikelyShortLivingObjectsNeedingGC > 2500 ||
    sNeedsGCAfterCC;
}

class nsJSEnvironmentObserver final : public nsIObserver
{
  ~nsJSEnvironmentObserver() {}
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS(nsJSEnvironmentObserver, nsIObserver)

NS_IMETHODIMP
nsJSEnvironmentObserver::Observe(nsISupports* aSubject, const char* aTopic,
                                 const char16_t* aData)
{
  if (!nsCRT::strcmp(aTopic, "memory-pressure")) {
    if (sGCOnMemoryPressure) {
      if(StringBeginsWith(nsDependentString(aData),
                          NS_LITERAL_STRING("low-memory-ongoing"))) {
        
        
        return NS_OK;
      }
      nsJSContext::GarbageCollectNow(JS::gcreason::MEM_PRESSURE,
                                     nsJSContext::NonIncrementalGC,
                                     nsJSContext::ShrinkingGC);
      nsJSContext::CycleCollectNow();
      if (NeedsGCAfterCC()) {
        nsJSContext::GarbageCollectNow(JS::gcreason::MEM_PRESSURE,
                                       nsJSContext::NonIncrementalGC,
                                       nsJSContext::ShrinkingGC);
      }
    }
  } else if (!nsCRT::strcmp(aTopic, "user-interaction-inactive")) {
    if (sCompactOnUserInactive) {
      nsJSContext::PokeShrinkingGC();
    }
  } else if (!nsCRT::strcmp(aTopic, "user-interaction-active")) {
    nsJSContext::KillShrinkingGCTimer();
    if (sIsCompactingOnUserInactive) {
      JS::AbortIncrementalGC(sRuntime);
    }
    MOZ_ASSERT(!sIsCompactingOnUserInactive);
  } else if (!nsCRT::strcmp(aTopic, "quit-application") ||
             !nsCRT::strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    sShuttingDown = true;
    KillTimers();
  }

  return NS_OK;
}





class AutoFree {
public:
  explicit AutoFree(void* aPtr) : mPtr(aPtr) {
  }
  ~AutoFree() {
    if (mPtr)
      free(mPtr);
  }
  void Invalidate() {
    mPtr = 0;
  }
private:
  void *mPtr;
};





bool
NS_HandleScriptError(nsIScriptGlobalObject *aScriptGlobal,
                     const ErrorEventInit &aErrorEventInit,
                     nsEventStatus *aStatus)
{
  bool called = false;
  nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(aScriptGlobal));
  nsIDocShell *docShell = win ? win->GetDocShell() : nullptr;
  if (docShell) {
    nsRefPtr<nsPresContext> presContext;
    docShell->GetPresContext(getter_AddRefs(presContext));

    static int32_t errorDepth; 
    ++errorDepth;

    if (errorDepth < 2) {
      
      
      nsRefPtr<ErrorEvent> event =
        ErrorEvent::Constructor(static_cast<nsGlobalWindow*>(win.get()),
                                NS_LITERAL_STRING("error"),
                                aErrorEventInit);
      event->SetTrusted(true);

      EventDispatcher::DispatchDOMEvent(win, nullptr, event, presContext,
                                        aStatus);
      called = true;
    }
    --errorDepth;
  }
  return called;
}

class ScriptErrorEvent : public nsRunnable
{
public:
  ScriptErrorEvent(nsPIDOMWindow* aWindow,
                   JSRuntime* aRuntime,
                   xpc::ErrorReport* aReport,
                   JS::Handle<JS::Value> aError)
    : mWindow(aWindow)
    , mReport(aReport)
    , mError(aRuntime, aError)
  {}

  NS_IMETHOD Run()
  {
    nsEventStatus status = nsEventStatus_eIgnore;
    nsPIDOMWindow* win = mWindow;
    MOZ_ASSERT(win);
    
    
    if (win->IsCurrentInnerWindow() && win->GetDocShell() && !sHandlingScriptError) {
      AutoRestore<bool> recursionGuard(sHandlingScriptError);
      sHandlingScriptError = true;

      nsRefPtr<nsPresContext> presContext;
      win->GetDocShell()->GetPresContext(getter_AddRefs(presContext));

      ThreadsafeAutoJSContext cx;
      RootedDictionary<ErrorEventInit> init(cx);
      init.mCancelable = true;
      init.mFilename = mReport->mFileName;
      init.mBubbles = true;

      NS_NAMED_LITERAL_STRING(xoriginMsg, "Script error.");
      if (!mReport->mIsMuted) {
        init.mMessage = mReport->mErrorMsg;
        init.mLineno = mReport->mLineNumber;
        init.mColno = mReport->mColumn;
        init.mError = mError;
      } else {
        NS_WARNING("Not same origin error!");
        init.mMessage = xoriginMsg;
        init.mLineno = 0;
      }

      nsRefPtr<ErrorEvent> event =
        ErrorEvent::Constructor(static_cast<nsGlobalWindow*>(win),
                                NS_LITERAL_STRING("error"), init);
      event->SetTrusted(true);

      EventDispatcher::DispatchDOMEvent(win, nullptr, event, presContext,
                                        &status);
    }

    if (status != nsEventStatus_eConsumeNoDefault) {
      mReport->LogToConsole();
    }

    return NS_OK;
  }

private:
  nsCOMPtr<nsPIDOMWindow>         mWindow;
  nsRefPtr<xpc::ErrorReport>      mReport;
  JS::PersistentRootedValue       mError;

  static bool sHandlingScriptError;
};

bool ScriptErrorEvent::sHandlingScriptError = false;



namespace xpc {

void
SystemErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
  JS::Rooted<JS::Value> exception(cx);
  ::JS_GetPendingException(cx, &exception);

  
  ::JS_ClearPendingException(cx);

  MOZ_ASSERT(cx == nsContentUtils::GetCurrentJSContext());
  nsCOMPtr<nsIGlobalObject> globalObject;

  
  
  
  
  
  
  
  
  
  
  if (nsIScriptContext* scx = GetScriptContextFromJSContext(cx)) {
    nsCOMPtr<nsPIDOMWindow> outer = do_QueryInterface(scx->GetGlobalObject());
    if (outer) {
      globalObject = static_cast<nsGlobalWindow*>(outer->GetCurrentInnerWindow());
    }
  }

  
  
  
  
  
  
  
  
  
  if (!globalObject && JS::CurrentGlobalOrNull(cx)) {
    globalObject = xpc::AddonWindowOrNull(JS::CurrentGlobalOrNull(cx));
  }

  if (!globalObject) {
    globalObject = xpc::NativeGlobal(xpc::PrivilegedJunkScope());
  }

  if (globalObject) {
    nsRefPtr<xpc::ErrorReport> xpcReport = new xpc::ErrorReport();
    bool isChrome = nsContentUtils::IsSystemPrincipal(globalObject->PrincipalOrNull());
    nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(globalObject);
    xpcReport->Init(report, message, isChrome, win ? win->WindowID() : 0);

    
    
    
    if (!win || JSREPORT_IS_WARNING(xpcReport->mFlags) ||
        report->errorNumber == JSMSG_OUT_OF_MEMORY)
    {
      xpcReport->LogToConsole();
      return;
    }

    
    
    DispatchScriptErrorEvent(win, JS_GetRuntime(cx), xpcReport, exception);
  }
}

void
DispatchScriptErrorEvent(nsPIDOMWindow *win, JSRuntime *rt, xpc::ErrorReport *xpcReport,
                         JS::Handle<JS::Value> exception)
{
  nsContentUtils::AddScriptRunner(new ScriptErrorEvent(win, rt, xpcReport, exception));
}

} 

#ifdef DEBUG

nsGlobalWindow *
JSObject2Win(JSObject *obj)
{
  return xpc::WindowOrNull(obj);
}

void
PrintWinURI(nsGlobalWindow *win)
{
  if (!win) {
    printf("No window passed in.\n");
    return;
  }

  nsCOMPtr<nsIDocument> doc = win->GetExtantDoc();
  if (!doc) {
    printf("No document in the window.\n");
    return;
  }

  nsIURI *uri = doc->GetDocumentURI();
  if (!uri) {
    printf("Document doesn't have a URI.\n");
    return;
  }

  nsAutoCString spec;
  uri->GetSpec(spec);
  printf("%s\n", spec.get());
}

void
PrintWinCodebase(nsGlobalWindow *win)
{
  if (!win) {
    printf("No window passed in.\n");
    return;
  }

  nsIPrincipal *prin = win->GetPrincipal();
  if (!prin) {
    printf("Window doesn't have principals.\n");
    return;
  }

  nsCOMPtr<nsIURI> uri;
  prin->GetURI(getter_AddRefs(uri));
  if (!uri) {
    printf("No URI, maybe the system principal.\n");
    return;
  }

  nsAutoCString spec;
  uri->GetSpec(spec);
  printf("%s\n", spec.get());
}

void
DumpString(const nsAString &str)
{
  printf("%s\n", NS_ConvertUTF16toUTF8(str).get());
}
#endif

#define JS_OPTIONS_DOT_STR "javascript.options."

static const char js_options_dot_str[]   = JS_OPTIONS_DOT_STR;
#ifdef JS_GC_ZEAL
static const char js_zeal_option_str[]        = JS_OPTIONS_DOT_STR "gczeal";
static const char js_zeal_frequency_str[]     = JS_OPTIONS_DOT_STR "gczeal.frequency";
#endif
static const char js_memlog_option_str[]      = JS_OPTIONS_DOT_STR "mem.log";
static const char js_memnotify_option_str[]   = JS_OPTIONS_DOT_STR "mem.notify";

void
nsJSContext::JSOptionChangedCallback(const char *pref, void *data)
{
  sPostGCEventsToConsole = Preferences::GetBool(js_memlog_option_str);
  sPostGCEventsToObserver = Preferences::GetBool(js_memnotify_option_str);

#ifdef JS_GC_ZEAL
  nsJSContext *context = reinterpret_cast<nsJSContext *>(data);
  int32_t zeal = Preferences::GetInt(js_zeal_option_str, -1);
  int32_t frequency = Preferences::GetInt(js_zeal_frequency_str, JS_DEFAULT_ZEAL_FREQ);
  if (zeal >= 0)
    ::JS_SetGCZeal(context->mContext, (uint8_t)zeal, frequency);
#endif
}

nsJSContext::nsJSContext(bool aGCOnDestruction,
                         nsIScriptGlobalObject* aGlobalObject)
  : mWindowProxy(nullptr)
  , mGCOnDestruction(aGCOnDestruction)
  , mGlobalObjectRef(aGlobalObject)
{
  EnsureStatics();

  ++sContextCount;

  mContext = ::JS_NewContext(sRuntime, gStackSize);
  if (mContext) {
    ::JS_SetContextPrivate(mContext, static_cast<nsIScriptContext *>(this));

    
    JS::ContextOptionsRef(mContext).setPrivateIsNSISupports(true);

    
    Preferences::RegisterCallback(JSOptionChangedCallback,
                                  js_options_dot_str, this);
  }
  mIsInitialized = false;
  mProcessingScriptTag = false;
  HoldJSObjects(this);
}

nsJSContext::~nsJSContext()
{
  mGlobalObjectRef = nullptr;

  DestroyJSContext();

  --sContextCount;

  if (!sContextCount && sDidShutdown) {
    
    
    

    NS_IF_RELEASE(sRuntimeService);
    NS_IF_RELEASE(sSecurityManager);
  }
}






void
nsJSContext::DestroyJSContext()
{
  if (!mContext) {
    return;
  }

  
  ::JS_SetContextPrivate(mContext, nullptr);

  
  Preferences::UnregisterCallback(JSOptionChangedCallback,
                                  js_options_dot_str, this);

  if (mGCOnDestruction) {
    PokeGC(JS::gcreason::NSJSCONTEXT_DESTROY);
  }

  JS_DestroyContextNoGC(mContext);
  mContext = nullptr;
  DropJSObjects(this);
}


NS_IMPL_CYCLE_COLLECTION_CLASS(nsJSContext)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsJSContext)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mWindowProxy)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsJSContext)
  NS_ASSERTION(!tmp->mContext || !js::ContextHasOutstandingRequests(tmp->mContext),
               "Trying to unlink a context with outstanding requests.");
  tmp->mIsInitialized = false;
  tmp->mGCOnDestruction = false;
  tmp->mWindowProxy = nullptr;
  tmp->DestroyJSContext();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mGlobalObjectRef)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INTERNAL(nsJSContext)
  NS_IMPL_CYCLE_COLLECTION_DESCRIBE(nsJSContext, tmp->GetCCRefcnt())
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mGlobalObjectRef)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsJSContext)
  NS_INTERFACE_MAP_ENTRY(nsIScriptContext)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF(nsJSContext)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsJSContext)

nsrefcnt
nsJSContext::GetCCRefcnt()
{
  nsrefcnt refcnt = mRefCnt.get();

  
  
  
  if (mContext && js::ContextHasOutstandingRequests(mContext)) {
    refcnt++;
  }

  return refcnt;
}

#ifdef DEBUG
bool
AtomIsEventHandlerName(nsIAtom *aName)
{
  const char16_t *name = aName->GetUTF16String();

  const char16_t *cp;
  char16_t c;
  for (cp = name; *cp != '\0'; ++cp)
  {
    c = *cp;
    if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z'))
      return false;
  }

  return true;
}
#endif

nsIScriptGlobalObject *
nsJSContext::GetGlobalObject()
{
  
  
  if (!mWindowProxy) {
    return nullptr;
  }

  MOZ_ASSERT(mGlobalObjectRef);
  return mGlobalObjectRef;
}

JSContext*
nsJSContext::GetNativeContext()
{
  return mContext;
}

nsresult
nsJSContext::InitContext()
{
  
  
  NS_ENSURE_TRUE(!mIsInitialized, NS_ERROR_ALREADY_INITIALIZED);

  if (!mContext)
    return NS_ERROR_OUT_OF_MEMORY;

  JSOptionChangedCallback(js_options_dot_str, this);

  return NS_OK;
}

nsresult
nsJSContext::SetProperty(JS::Handle<JSObject*> aTarget, const char* aPropName, nsISupports* aArgs)
{
  AutoJSAPI jsapi;
  if (NS_WARN_IF(!jsapi.InitWithLegacyErrorReporting(GetGlobalObject()))) {
    return NS_ERROR_FAILURE;
  }
  MOZ_ASSERT(jsapi.cx() == mContext,
             "AutoJSAPI should have found our own JSContext*");

  JS::AutoValueVector args(mContext);

  JS::Rooted<JSObject*> global(mContext, GetWindowProxy());
  nsresult rv =
    ConvertSupportsTojsvals(aArgs, global, args);
  NS_ENSURE_SUCCESS(rv, rv);

  

  for (uint32_t i = 0; i < args.length(); ++i) {
    if (!JS_WrapValue(mContext, args[i])) {
      return NS_ERROR_FAILURE;
    }
  }

  JS::Rooted<JSObject*> array(mContext, ::JS_NewArrayObject(mContext, args));
  if (!array) {
    return NS_ERROR_FAILURE;
  }

  return JS_DefineProperty(mContext, aTarget, aPropName, array, 0) ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
nsJSContext::ConvertSupportsTojsvals(nsISupports* aArgs,
                                     JS::Handle<JSObject*> aScope,
                                     JS::AutoValueVector& aArgsOut)
{
  nsresult rv = NS_OK;

  
  nsCOMPtr<nsIJSArgArray> fastArray = do_QueryInterface(aArgs);
  if (fastArray) {
    uint32_t argc;
    JS::Value* argv;
    rv = fastArray->GetArgs(&argc, reinterpret_cast<void **>(&argv));
    if (NS_SUCCEEDED(rv) && !aArgsOut.append(argv, argc)) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
    return rv;
  }

  
  
  

  nsIXPConnect *xpc = nsContentUtils::XPConnect();
  NS_ENSURE_TRUE(xpc, NS_ERROR_UNEXPECTED);
  AutoJSContext cx;

  if (!aArgs)
    return NS_OK;
  uint32_t argCount;
  
  
  nsCOMPtr<nsIArray> argsArray(do_QueryInterface(aArgs));

  if (argsArray) {
    rv = argsArray->GetLength(&argCount);
    NS_ENSURE_SUCCESS(rv, rv);
    if (argCount == 0)
      return NS_OK;
  } else {
    argCount = 1; 
  }

  
  if (!aArgsOut.resize(argCount)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (argsArray) {
    for (uint32_t argCtr = 0; argCtr < argCount && NS_SUCCEEDED(rv); argCtr++) {
      nsCOMPtr<nsISupports> arg;
      JS::MutableHandle<JS::Value> thisVal = aArgsOut[argCtr];
      argsArray->QueryElementAt(argCtr, NS_GET_IID(nsISupports),
                                getter_AddRefs(arg));
      if (!arg) {
        thisVal.setNull();
        continue;
      }
      nsCOMPtr<nsIVariant> variant(do_QueryInterface(arg));
      if (variant != nullptr) {
        rv = xpc->VariantToJS(cx, aScope, variant, thisVal);
      } else {
        
        
        
        rv = AddSupportsPrimitiveTojsvals(arg, thisVal.address());
        if (rv == NS_ERROR_NO_INTERFACE) {
          
          
#ifdef DEBUG
          
          
          nsCOMPtr<nsISupportsPrimitive> prim(do_QueryInterface(arg));
          NS_ASSERTION(prim == nullptr,
                       "Don't pass nsISupportsPrimitives - use nsIVariant!");
#endif
          JSAutoCompartment ac(cx, aScope);
          rv = nsContentUtils::WrapNative(cx, arg, thisVal);
        }
      }
    }
  } else {
    nsCOMPtr<nsIVariant> variant = do_QueryInterface(aArgs);
    if (variant) {
      rv = xpc->VariantToJS(cx, aScope, variant, aArgsOut[0]);
    } else {
      NS_ERROR("Not an array, not an interface?");
      rv = NS_ERROR_UNEXPECTED;
    }
  }
  return rv;
}


nsresult
nsJSContext::AddSupportsPrimitiveTojsvals(nsISupports *aArg, JS::Value *aArgv)
{
  NS_PRECONDITION(aArg, "Empty arg");

  nsCOMPtr<nsISupportsPrimitive> argPrimitive(do_QueryInterface(aArg));
  if (!argPrimitive)
    return NS_ERROR_NO_INTERFACE;

  AutoJSContext cx;
  uint16_t type;
  argPrimitive->GetType(&type);

  switch(type) {
    case nsISupportsPrimitive::TYPE_CSTRING : {
      nsCOMPtr<nsISupportsCString> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      nsAutoCString data;

      p->GetData(data);


      JSString *str = ::JS_NewStringCopyN(cx, data.get(), data.Length());
      NS_ENSURE_TRUE(str, NS_ERROR_OUT_OF_MEMORY);

      *aArgv = STRING_TO_JSVAL(str);

      break;
    }
    case nsISupportsPrimitive::TYPE_STRING : {
      nsCOMPtr<nsISupportsString> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      nsAutoString data;

      p->GetData(data);

      
      
      JSString *str =
        ::JS_NewUCStringCopyN(cx, data.get(), data.Length());
      NS_ENSURE_TRUE(str, NS_ERROR_OUT_OF_MEMORY);

      *aArgv = STRING_TO_JSVAL(str);
      break;
    }
    case nsISupportsPrimitive::TYPE_PRBOOL : {
      nsCOMPtr<nsISupportsPRBool> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      bool data;

      p->GetData(&data);

      *aArgv = BOOLEAN_TO_JSVAL(data);

      break;
    }
    case nsISupportsPrimitive::TYPE_PRUINT8 : {
      nsCOMPtr<nsISupportsPRUint8> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      uint8_t data;

      p->GetData(&data);

      *aArgv = INT_TO_JSVAL(data);

      break;
    }
    case nsISupportsPrimitive::TYPE_PRUINT16 : {
      nsCOMPtr<nsISupportsPRUint16> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      uint16_t data;

      p->GetData(&data);

      *aArgv = INT_TO_JSVAL(data);

      break;
    }
    case nsISupportsPrimitive::TYPE_PRUINT32 : {
      nsCOMPtr<nsISupportsPRUint32> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      uint32_t data;

      p->GetData(&data);

      *aArgv = INT_TO_JSVAL(data);

      break;
    }
    case nsISupportsPrimitive::TYPE_CHAR : {
      nsCOMPtr<nsISupportsChar> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      char data;

      p->GetData(&data);

      JSString *str = ::JS_NewStringCopyN(cx, &data, 1);
      NS_ENSURE_TRUE(str, NS_ERROR_OUT_OF_MEMORY);

      *aArgv = STRING_TO_JSVAL(str);

      break;
    }
    case nsISupportsPrimitive::TYPE_PRINT16 : {
      nsCOMPtr<nsISupportsPRInt16> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      int16_t data;

      p->GetData(&data);

      *aArgv = INT_TO_JSVAL(data);

      break;
    }
    case nsISupportsPrimitive::TYPE_PRINT32 : {
      nsCOMPtr<nsISupportsPRInt32> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      int32_t data;

      p->GetData(&data);

      *aArgv = INT_TO_JSVAL(data);

      break;
    }
    case nsISupportsPrimitive::TYPE_FLOAT : {
      nsCOMPtr<nsISupportsFloat> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      float data;

      p->GetData(&data);

      *aArgv = ::JS_NumberValue(data);

      break;
    }
    case nsISupportsPrimitive::TYPE_DOUBLE : {
      nsCOMPtr<nsISupportsDouble> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      double data;

      p->GetData(&data);

      *aArgv = ::JS_NumberValue(data);

      break;
    }
    case nsISupportsPrimitive::TYPE_INTERFACE_POINTER : {
      nsCOMPtr<nsISupportsInterfacePointer> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      nsCOMPtr<nsISupports> data;
      nsIID *iid = nullptr;

      p->GetData(getter_AddRefs(data));
      p->GetDataIID(&iid);
      NS_ENSURE_TRUE(iid, NS_ERROR_UNEXPECTED);

      AutoFree iidGuard(iid); 

      JS::Rooted<JSObject*> scope(cx, GetWindowProxy());
      JS::Rooted<JS::Value> v(cx);
      JSAutoCompartment ac(cx, scope);
      nsresult rv = nsContentUtils::WrapNative(cx, data, iid, &v);
      NS_ENSURE_SUCCESS(rv, rv);

      *aArgv = v;

      break;
    }
    case nsISupportsPrimitive::TYPE_ID :
    case nsISupportsPrimitive::TYPE_PRUINT64 :
    case nsISupportsPrimitive::TYPE_PRINT64 :
    case nsISupportsPrimitive::TYPE_PRTIME :
    case nsISupportsPrimitive::TYPE_VOID : {
      NS_WARNING("Unsupported primitive type used");
      *aArgv = JSVAL_NULL;
      break;
    }
    default : {
      NS_WARNING("Unknown primitive type used");
      *aArgv = JSVAL_NULL;
      break;
    }
  }
  return NS_OK;
}

#ifdef MOZ_JPROF

#include <signal.h>

inline bool
IsJProfAction(struct sigaction *action)
{
    return (action->sa_sigaction &&
            (action->sa_flags & (SA_RESTART | SA_SIGINFO)) == (SA_RESTART | SA_SIGINFO));
}

void NS_JProfStartProfiling();
void NS_JProfStopProfiling();
void NS_JProfClearCircular();

static bool
JProfStartProfilingJS(JSContext *cx, unsigned argc, JS::Value *vp)
{
  NS_JProfStartProfiling();
  return true;
}

void NS_JProfStartProfiling()
{
    
    
    
    struct sigaction action;

    
    sigaction(SIGALRM, nullptr, &action);
    
    if (IsJProfAction(&action)) {
        
        raise(SIGALRM);
        return;
    }

    sigaction(SIGPROF, nullptr, &action);
    
    if (IsJProfAction(&action)) {
        
        raise(SIGPROF);
        return;
    }

    sigaction(SIGPOLL, nullptr, &action);
    
    if (IsJProfAction(&action)) {
        
        raise(SIGPOLL);
        return;
    }

    printf("Could not start jprof-profiling since JPROF_FLAGS was not set.\n");
}

static bool
JProfStopProfilingJS(JSContext *cx, unsigned argc, JS::Value *vp)
{
  NS_JProfStopProfiling();
  return true;
}

void
NS_JProfStopProfiling()
{
    raise(SIGUSR1);
    
}

static bool
JProfClearCircularJS(JSContext *cx, unsigned argc, JS::Value *vp)
{
  NS_JProfClearCircular();
  return true;
}

void
NS_JProfClearCircular()
{
    raise(SIGUSR2);
    
}

static bool
JProfSaveCircularJS(JSContext *cx, unsigned argc, JS::Value *vp)
{
  
  NS_JProfStopProfiling();
  NS_JProfStartProfiling();
  return true;
}

static const JSFunctionSpec JProfFunctions[] = {
    JS_FS("JProfStartProfiling",        JProfStartProfilingJS,      0, 0),
    JS_FS("JProfStopProfiling",         JProfStopProfilingJS,       0, 0),
    JS_FS("JProfClearCircular",         JProfClearCircularJS,       0, 0),
    JS_FS("JProfSaveCircular",          JProfSaveCircularJS,        0, 0),
    JS_FS_END
};

#endif 

nsresult
nsJSContext::InitClasses(JS::Handle<JSObject*> aGlobalObj)
{
  JSOptionChangedCallback(js_options_dot_str, this);
  AutoJSAPI jsapi;
  jsapi.Init();
  JSContext* cx = jsapi.cx();
  JSAutoCompartment ac(cx, aGlobalObj);

  
  ::JS_DefineProfilingFunctions(cx, aGlobalObj);

#ifdef MOZ_JPROF
  
  ::JS_DefineFunctions(cx, aGlobalObj, JProfFunctions);
#endif

  return NS_OK;
}

void
nsJSContext::WillInitializeContext()
{
  mIsInitialized = false;
}

void
nsJSContext::DidInitializeContext()
{
  mIsInitialized = true;
}

bool
nsJSContext::IsContextInitialized()
{
  return mIsInitialized;
}

bool
nsJSContext::GetProcessingScriptTag()
{
  return mProcessingScriptTag;
}

void
nsJSContext::SetProcessingScriptTag(bool aFlag)
{
  mProcessingScriptTag = aFlag;
}

void
FullGCTimerFired(nsITimer* aTimer, void* aClosure)
{
  nsJSContext::KillFullGCTimer();
  MOZ_ASSERT(!aClosure, "Don't pass a closure to FullGCTimerFired");
  nsJSContext::GarbageCollectNow(JS::gcreason::FULL_GC_TIMER,
                                 nsJSContext::IncrementalGC);
}


void
nsJSContext::GarbageCollectNow(JS::gcreason::Reason aReason,
                               IsIncremental aIncremental,
                               IsShrinking aShrinking,
                               int64_t aSliceMillis)
{
  PROFILER_LABEL("nsJSContext", "GarbageCollectNow",
    js::ProfileEntry::Category::GC);

  MOZ_ASSERT_IF(aSliceMillis, aIncremental == IncrementalGC);

  KillGCTimer();
  KillShrinkGCBuffersTimer();

  
  
  
  
  
  
  sPendingLoadCount = 0;
  sLoadingInProgress = false;

  if (!nsContentUtils::XPConnect() || !sRuntime) {
    return;
  }

  if (sCCLockedOut && aIncremental == IncrementalGC) {
    
    JS::PrepareForIncrementalGC(sRuntime);
    JS::IncrementalGCSlice(sRuntime, aReason, aSliceMillis);
    return;
  }

  JSGCInvocationKind gckind = aShrinking == ShrinkingGC ? GC_SHRINK : GC_NORMAL;
  JS::PrepareForFullGC(sRuntime);
  if (aIncremental == IncrementalGC) {
    JS::StartIncrementalGC(sRuntime, gckind, aReason, aSliceMillis);
  } else {
    JS::GCForReason(sRuntime, gckind, aReason);
  }
}


void
nsJSContext::ShrinkGCBuffersNow()
{
  PROFILER_LABEL("nsJSContext", "ShrinkGCBuffersNow",
    js::ProfileEntry::Category::GC);

  KillShrinkGCBuffersTimer();

  JS::ShrinkGCBuffers(sRuntime);
}

static void
FinishAnyIncrementalGC()
{
  if (sCCLockedOut) {
    
    JS::PrepareForIncrementalGC(sRuntime);
    JS::FinishIncrementalGC(sRuntime, JS::gcreason::CC_FORCED);
  }
}

static void
FireForgetSkippable(uint32_t aSuspected, bool aRemoveChildless)
{
  PRTime startTime = PR_Now();
  FinishAnyIncrementalGC();
  bool earlyForgetSkippable =
    sCleanupsSinceLastGC < NS_MAJOR_FORGET_SKIPPABLE_CALLS;
  nsCycleCollector_forgetSkippable(aRemoveChildless, earlyForgetSkippable);
  sPreviousSuspectedCount = nsCycleCollector_suspectedCount();
  ++sCleanupsSinceLastGC;
  PRTime delta = PR_Now() - startTime;
  if (sMinForgetSkippableTime > delta) {
    sMinForgetSkippableTime = delta;
  }
  if (sMaxForgetSkippableTime < delta) {
    sMaxForgetSkippableTime = delta;
  }
  sTotalForgetSkippableTime += delta;
  sRemovedPurples += (aSuspected - sPreviousSuspectedCount);
  ++sForgetSkippableBeforeCC;
}

MOZ_ALWAYS_INLINE
static uint32_t
TimeBetween(TimeStamp start, TimeStamp end)
{
  MOZ_ASSERT(end >= start);
  return (uint32_t) ((end - start).ToMilliseconds());
}

static uint32_t
TimeUntilNow(TimeStamp start)
{
  if (start.IsNull()) {
    return 0;
  }
  return TimeBetween(start, TimeStamp::Now());
}

struct CycleCollectorStats
{
  void Init()
  {
    Clear();
    mMaxSliceTimeSinceClear = 0;
  }

  void Clear()
  {
    mBeginSliceTime = TimeStamp();
    mEndSliceTime = TimeStamp();
    mBeginTime = TimeStamp();
    mMaxGCDuration = 0;
    mRanSyncForgetSkippable = false;
    mSuspected = 0;
    mMaxSkippableDuration = 0;
    mMaxSliceTime = 0;
    mTotalSliceTime = 0;
    mAnyLockedOut = false;
    mExtraForgetSkippableCalls = 0;
  }

  void PrepareForCycleCollectionSlice(int32_t aExtraForgetSkippableCalls = 0);

  void FinishCycleCollectionSlice()
  {
    if (mBeginSliceTime.IsNull()) {
      
      return;
    }

    mEndSliceTime = TimeStamp::Now();
    uint32_t sliceTime = TimeBetween(mBeginSliceTime, mEndSliceTime);
    mMaxSliceTime = std::max(mMaxSliceTime, sliceTime);
    mMaxSliceTimeSinceClear = std::max(mMaxSliceTimeSinceClear, sliceTime);
    mTotalSliceTime += sliceTime;
    mBeginSliceTime = TimeStamp();
    MOZ_ASSERT(mExtraForgetSkippableCalls == 0, "Forget to reset extra forget skippable calls?");
  }

  void RunForgetSkippable();

  
  TimeStamp mBeginSliceTime;

  
  TimeStamp mEndSliceTime;

  
  TimeStamp mBeginTime;

  
  uint32_t mMaxGCDuration;

  
  bool mRanSyncForgetSkippable;

  
  uint32_t mSuspected;

  
  
  uint32_t mMaxSkippableDuration;

  
  uint32_t mMaxSliceTime;

  
  uint32_t mMaxSliceTimeSinceClear;

  
  uint32_t mTotalSliceTime;

  
  bool mAnyLockedOut;

  int32_t mExtraForgetSkippableCalls;
};

CycleCollectorStats gCCStats;

void
CycleCollectorStats::PrepareForCycleCollectionSlice(int32_t aExtraForgetSkippableCalls)
{
  mBeginSliceTime = TimeStamp::Now();

  
  if (sCCLockedOut) {
    mAnyLockedOut = true;
    FinishAnyIncrementalGC();
    uint32_t gcTime = TimeBetween(mBeginSliceTime, TimeStamp::Now());
    mMaxGCDuration = std::max(mMaxGCDuration, gcTime);
  }

  mExtraForgetSkippableCalls = aExtraForgetSkippableCalls;
}

void
CycleCollectorStats::RunForgetSkippable()
{
  
  
  if (mExtraForgetSkippableCalls >= 0) {
    TimeStamp beginForgetSkippable = TimeStamp::Now();
    bool ranSyncForgetSkippable = false;
    while (sCleanupsSinceLastGC < NS_MAJOR_FORGET_SKIPPABLE_CALLS) {
      FireForgetSkippable(nsCycleCollector_suspectedCount(), false);
      ranSyncForgetSkippable = true;
    }

    for (int32_t i = 0; i < mExtraForgetSkippableCalls; ++i) {
      FireForgetSkippable(nsCycleCollector_suspectedCount(), false);
      ranSyncForgetSkippable = true;
    }

    if (ranSyncForgetSkippable) {
      mMaxSkippableDuration =
        std::max(mMaxSkippableDuration, TimeUntilNow(beginForgetSkippable));
      mRanSyncForgetSkippable = true;
    }

  }
  mExtraForgetSkippableCalls = 0;
}


void
nsJSContext::CycleCollectNow(nsICycleCollectorListener *aListener,
                             int32_t aExtraForgetSkippableCalls)
{
  if (!NS_IsMainThread()) {
    return;
  }

  PROFILER_LABEL("nsJSContext", "CycleCollectNow",
    js::ProfileEntry::Category::CC);

  gCCStats.PrepareForCycleCollectionSlice(aExtraForgetSkippableCalls);
  nsCycleCollector_collect(aListener);
  gCCStats.FinishCycleCollectionSlice();
}


void
nsJSContext::RunCycleCollectorSlice()
{
  if (!NS_IsMainThread()) {
    return;
  }

  PROFILER_LABEL("nsJSContext", "RunCycleCollectorSlice",
    js::ProfileEntry::Category::CC);

  gCCStats.PrepareForCycleCollectionSlice();

  
  
  js::SliceBudget budget;

  if (sIncrementalCC) {
    if (gCCStats.mBeginTime.IsNull()) {
      
      budget = js::SliceBudget(js::TimeBudget(kICCSliceBudget));
    } else {
      TimeStamp now = TimeStamp::Now();

      
      if (TimeBetween(gCCStats.mBeginTime, now) < kMaxICCDuration) {
        float sliceMultiplier = std::max(TimeBetween(gCCStats.mEndSliceTime, now) / (float)kICCIntersliceDelay, 1.0f);
        budget = js::SliceBudget(js::TimeBudget(kICCSliceBudget * sliceMultiplier));
      }
    }
  }

  nsCycleCollector_collectSlice(budget, sDidPaintAfterPreviousICCSlice);
  sDidPaintAfterPreviousICCSlice = false;

  gCCStats.FinishCycleCollectionSlice();
}


void
nsJSContext::RunCycleCollectorWorkSlice(int64_t aWorkBudget)
{
  if (!NS_IsMainThread()) {
    return;
  }

  PROFILER_LABEL("nsJSContext", "RunCycleCollectorWorkSlice",
    js::ProfileEntry::Category::CC);

  gCCStats.PrepareForCycleCollectionSlice();

  js::SliceBudget budget = js::SliceBudget(js::WorkBudget(aWorkBudget));
  nsCycleCollector_collectSlice(budget);

  gCCStats.FinishCycleCollectionSlice();
}

void
nsJSContext::ClearMaxCCSliceTime()
{
  gCCStats.mMaxSliceTimeSinceClear = 0;
}

uint32_t
nsJSContext::GetMaxCCSliceTimeSinceClear()
{
  return gCCStats.mMaxSliceTimeSinceClear;
}

static void
ICCTimerFired(nsITimer* aTimer, void* aClosure)
{
  if (sDidShutdown) {
    return;
  }

  
  

  if (sCCLockedOut) {
    PRTime now = PR_Now();
    if (sCCLockedOutTime == 0) {
      sCCLockedOutTime = now;
      return;
    }
    if (now - sCCLockedOutTime < NS_MAX_CC_LOCKEDOUT_TIME) {
      return;
    }
  }

  nsJSContext::RunCycleCollectorSlice();
}


void
nsJSContext::BeginCycleCollectionCallback()
{
  MOZ_ASSERT(NS_IsMainThread());

  gCCStats.mBeginTime = gCCStats.mBeginSliceTime.IsNull() ? TimeStamp::Now() : gCCStats.mBeginSliceTime;
  gCCStats.mSuspected = nsCycleCollector_suspectedCount();

  KillCCTimer();

  gCCStats.RunForgetSkippable();

  MOZ_ASSERT(!sICCTimer, "Tried to create a new ICC timer when one already existed.");

  
  
  CallCreateInstance("@mozilla.org/timer;1", &sICCTimer);
  if (sICCTimer) {
    sICCTimer->InitWithFuncCallback(ICCTimerFired,
                                    nullptr,
                                    kICCIntersliceDelay,
                                    nsITimer::TYPE_REPEATING_SLACK);
  }
}

static_assert(NS_GC_DELAY > kMaxICCDuration, "A max duration ICC shouldn't reduce GC delay to 0");


void
nsJSContext::EndCycleCollectionCallback(CycleCollectorResults &aResults)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsJSContext::KillICCTimer();

  
  
  
  gCCStats.FinishCycleCollectionSlice();

  sCCollectedWaitingForGC += aResults.mFreedGCed;
  sCCollectedZonesWaitingForGC += aResults.mFreedJSZones;

  TimeStamp endCCTimeStamp = TimeStamp::Now();
  uint32_t ccNowDuration = TimeBetween(gCCStats.mBeginTime, endCCTimeStamp);

  if (NeedsGCAfterCC()) {
    PokeGC(JS::gcreason::CC_WAITING,
           NS_GC_DELAY - std::min(ccNowDuration, kMaxICCDuration));
  }

  PRTime endCCTime;
  if (sPostGCEventsToObserver) {
    endCCTime = PR_Now();
  }

  
  Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR_FINISH_IGC, gCCStats.mAnyLockedOut);
  Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR_SYNC_SKIPPABLE, gCCStats.mRanSyncForgetSkippable);
  Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR_FULL, ccNowDuration);
  Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR_MAX_PAUSE, gCCStats.mMaxSliceTime);

  if (!sLastCCEndTime.IsNull()) {
    
    uint32_t timeBetween = TimeBetween(sLastCCEndTime, gCCStats.mBeginTime) / 1000;
    Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR_TIME_BETWEEN, timeBetween);
  }
  sLastCCEndTime = endCCTimeStamp;

  Telemetry::Accumulate(Telemetry::FORGET_SKIPPABLE_MAX,
                        sMaxForgetSkippableTime / PR_USEC_PER_MSEC);

  PRTime delta = GetCollectionTimeDelta();

  uint32_t cleanups = sForgetSkippableBeforeCC ? sForgetSkippableBeforeCC : 1;
  uint32_t minForgetSkippableTime = (sMinForgetSkippableTime == UINT32_MAX)
    ? 0 : sMinForgetSkippableTime;

  if (sPostGCEventsToConsole) {
    nsCString mergeMsg;
    if (aResults.mMergedZones) {
      mergeMsg.AssignLiteral(" merged");
    }

    nsCString gcMsg;
    if (aResults.mForcedGC) {
      gcMsg.AssignLiteral(", forced a GC");
    }

    NS_NAMED_MULTILINE_LITERAL_STRING(kFmt,
      MOZ_UTF16("CC(T+%.1f) max pause: %lums, total time: %lums, slices: %lu, suspected: %lu, visited: %lu RCed and %lu%s GCed, collected: %lu RCed and %lu GCed (%lu|%lu|%lu waiting for GC)%s\n")
      MOZ_UTF16("ForgetSkippable %lu times before CC, min: %lu ms, max: %lu ms, avg: %lu ms, total: %lu ms, max sync: %lu ms, removed: %lu"));
    nsString msg;
    msg.Adopt(nsTextFormatter::smprintf(kFmt.get(), double(delta) / PR_USEC_PER_SEC,
                                        gCCStats.mMaxSliceTime, gCCStats.mTotalSliceTime,
                                        aResults.mNumSlices, gCCStats.mSuspected,
                                        aResults.mVisitedRefCounted, aResults.mVisitedGCed, mergeMsg.get(),
                                        aResults.mFreedRefCounted, aResults.mFreedGCed,
                                        sCCollectedWaitingForGC, sCCollectedZonesWaitingForGC, sLikelyShortLivingObjectsNeedingGC,
                                        gcMsg.get(),
                                        sForgetSkippableBeforeCC,
                                        minForgetSkippableTime / PR_USEC_PER_MSEC,
                                        sMaxForgetSkippableTime / PR_USEC_PER_MSEC,
                                        (sTotalForgetSkippableTime / cleanups) /
                                          PR_USEC_PER_MSEC,
                                        sTotalForgetSkippableTime / PR_USEC_PER_MSEC,
                                        gCCStats.mMaxSkippableDuration, sRemovedPurples));
    nsCOMPtr<nsIConsoleService> cs =
      do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    if (cs) {
      cs->LogStringMessage(msg.get());
    }
  }

  if (sPostGCEventsToObserver) {
    NS_NAMED_MULTILINE_LITERAL_STRING(kJSONFmt,
       MOZ_UTF16("{ \"timestamp\": %llu, ")
         MOZ_UTF16("\"duration\": %lu, ")
         MOZ_UTF16("\"max_slice_pause\": %lu, ")
         MOZ_UTF16("\"total_slice_pause\": %lu, ")
         MOZ_UTF16("\"max_finish_gc_duration\": %lu, ")
         MOZ_UTF16("\"max_sync_skippable_duration\": %lu, ")
         MOZ_UTF16("\"suspected\": %lu, ")
         MOZ_UTF16("\"visited\": { ")
             MOZ_UTF16("\"RCed\": %lu, ")
             MOZ_UTF16("\"GCed\": %lu }, ")
         MOZ_UTF16("\"collected\": { ")
             MOZ_UTF16("\"RCed\": %lu, ")
             MOZ_UTF16("\"GCed\": %lu }, ")
         MOZ_UTF16("\"waiting_for_gc\": %lu, ")
         MOZ_UTF16("\"zones_waiting_for_gc\": %lu, ")
         MOZ_UTF16("\"short_living_objects_waiting_for_gc\": %lu, ")
         MOZ_UTF16("\"forced_gc\": %d, ")
         MOZ_UTF16("\"forget_skippable\": { ")
             MOZ_UTF16("\"times_before_cc\": %lu, ")
             MOZ_UTF16("\"min\": %lu, ")
             MOZ_UTF16("\"max\": %lu, ")
             MOZ_UTF16("\"avg\": %lu, ")
             MOZ_UTF16("\"total\": %lu, ")
             MOZ_UTF16("\"removed\": %lu } ")
       MOZ_UTF16("}"));
    nsString json;
    json.Adopt(nsTextFormatter::smprintf(kJSONFmt.get(), endCCTime, ccNowDuration,
                                         gCCStats.mMaxSliceTime,
                                         gCCStats.mTotalSliceTime,
                                         gCCStats.mMaxGCDuration,
                                         gCCStats.mMaxSkippableDuration,
                                         gCCStats.mSuspected,
                                         aResults.mVisitedRefCounted, aResults.mVisitedGCed,
                                         aResults.mFreedRefCounted, aResults.mFreedGCed,
                                         sCCollectedWaitingForGC,
                                         sCCollectedZonesWaitingForGC,
                                         sLikelyShortLivingObjectsNeedingGC,
                                         aResults.mForcedGC,
                                         sForgetSkippableBeforeCC,
                                         minForgetSkippableTime / PR_USEC_PER_MSEC,
                                         sMaxForgetSkippableTime / PR_USEC_PER_MSEC,
                                         (sTotalForgetSkippableTime / cleanups) /
                                           PR_USEC_PER_MSEC,
                                         sTotalForgetSkippableTime / PR_USEC_PER_MSEC,
                                         sRemovedPurples));
    nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
    if (observerService) {
      observerService->NotifyObservers(nullptr, "cycle-collection-statistics", json.get());
    }
  }

  
  sMinForgetSkippableTime = UINT32_MAX;
  sMaxForgetSkippableTime = 0;
  sTotalForgetSkippableTime = 0;
  sRemovedPurples = 0;
  sForgetSkippableBeforeCC = 0;
  sNeedsFullCC = false;
  sNeedsGCAfterCC = false;
  gCCStats.Clear();
}


void
InterSliceGCTimerFired(nsITimer *aTimer, void *aClosure)
{
  nsJSContext::KillInterSliceGCTimer();
  nsJSContext::GarbageCollectNow(JS::gcreason::INTER_SLICE_GC,
                                 nsJSContext::IncrementalGC,
                                 nsJSContext::NonShrinkingGC,
                                 NS_INTERSLICE_GC_BUDGET);
}


void
GCTimerFired(nsITimer *aTimer, void *aClosure)
{
  nsJSContext::KillGCTimer();
  uintptr_t reason = reinterpret_cast<uintptr_t>(aClosure);
  nsJSContext::GarbageCollectNow(static_cast<JS::gcreason::Reason>(reason),
                                 nsJSContext::IncrementalGC);
}

void
ShrinkGCBuffersTimerFired(nsITimer *aTimer, void *aClosure)
{
  nsJSContext::KillShrinkGCBuffersTimer();
  nsJSContext::ShrinkGCBuffersNow();
}


void
ShrinkingGCTimerFired(nsITimer* aTimer, void* aClosure)
{
  nsJSContext::KillShrinkingGCTimer();
  sIsCompactingOnUserInactive = true;
  nsJSContext::GarbageCollectNow(JS::gcreason::USER_INACTIVE,
                                 nsJSContext::IncrementalGC,
                                 nsJSContext::ShrinkingGC);
}

static bool
ShouldTriggerCC(uint32_t aSuspected)
{
  return sNeedsFullCC ||
         aSuspected > NS_CC_PURPLE_LIMIT ||
         (aSuspected > NS_CC_FORCED_PURPLE_LIMIT &&
          TimeUntilNow(sLastCCEndTime) > NS_CC_FORCED);
}

static void
CCTimerFired(nsITimer *aTimer, void *aClosure)
{
  if (sDidShutdown) {
    return;
  }

  static uint32_t ccDelay = NS_CC_DELAY;
  if (sCCLockedOut) {
    ccDelay = NS_CC_DELAY / 3;

    PRTime now = PR_Now();
    if (sCCLockedOutTime == 0) {
      
      
      
      
      
      sCCTimerFireCount = 0;
      sCCLockedOutTime = now;
      return;
    }
    if (now - sCCLockedOutTime < NS_MAX_CC_LOCKEDOUT_TIME) {
      return;
    }
  }

  ++sCCTimerFireCount;

  
  
  
  
  int32_t numEarlyTimerFires = std::max((int32_t)ccDelay / NS_CC_SKIPPABLE_DELAY - 2, 1);
  bool isLateTimerFire = sCCTimerFireCount > numEarlyTimerFires;
  uint32_t suspected = nsCycleCollector_suspectedCount();
  if (isLateTimerFire && ShouldTriggerCC(suspected)) {
    if (sCCTimerFireCount == numEarlyTimerFires + 1) {
      FireForgetSkippable(suspected, true);
      if (ShouldTriggerCC(nsCycleCollector_suspectedCount())) {
        
        
        return;
      }
    } else {
      
      
      
      nsJSContext::RunCycleCollectorSlice();
    }
  } else if (((sPreviousSuspectedCount + 100) <= suspected) ||
             (sCleanupsSinceLastGC < NS_MAJOR_FORGET_SKIPPABLE_CALLS)) {
      
      
      FireForgetSkippable(suspected, false);
  }

  if (isLateTimerFire) {
    ccDelay = NS_CC_DELAY;

    
    
    sPreviousSuspectedCount = 0;
    nsJSContext::KillCCTimer();
  }
}


uint32_t
nsJSContext::CleanupsSinceLastGC()
{
  return sCleanupsSinceLastGC;
}


void
nsJSContext::LoadStart()
{
  sLoadingInProgress = true;
  ++sPendingLoadCount;
}


void
nsJSContext::LoadEnd()
{
  if (!sLoadingInProgress)
    return;

  
  
  if (sPendingLoadCount > 0) {
    --sPendingLoadCount;
    return;
  }

  
  sLoadingInProgress = false;
  PokeGC(JS::gcreason::LOAD_END);
}


static bool
ReadyToTriggerExpensiveCollectorTimer()
{
  bool ready = kPokesBetweenExpensiveCollectorTriggers < ++sExpensiveCollectorPokes;
  if (ready) {
    sExpensiveCollectorPokes = 0;
  }
  return ready;
}












void
nsJSContext::RunNextCollectorTimer()
{
  if (sShuttingDown) {
    return;
  }

  if (sGCTimer) {
    if (ReadyToTriggerExpensiveCollectorTimer()) {
      GCTimerFired(nullptr, reinterpret_cast<void *>(JS::gcreason::DOM_WINDOW_UTILS));
    }
    return;
  }

  if (sInterSliceGCTimer) {
    InterSliceGCTimerFired(nullptr, nullptr);
    return;
  }

  
  
  MOZ_ASSERT(!sCCLockedOut, "Don't check the CC timers if the CC is locked out.");

  if (sCCTimer) {
    if (ReadyToTriggerExpensiveCollectorTimer()) {
      CCTimerFired(nullptr, nullptr);
    }
    return;
  }

  if (sICCTimer) {
    ICCTimerFired(nullptr, nullptr);
    return;
  }
}


void
nsJSContext::PokeGC(JS::gcreason::Reason aReason, int aDelay)
{
  if (sGCTimer || sInterSliceGCTimer || sShuttingDown) {
    
    return;
  }

  if (sCCTimer) {
    
    sNeedsFullCC = true;
    
    sNeedsGCAfterCC = true;
    return;
  }

  if (sICCTimer) {
    
    
    sNeedsGCAfterCC = true;
    return;
  }

  CallCreateInstance("@mozilla.org/timer;1", &sGCTimer);

  if (!sGCTimer) {
    
    return;
  }

  static bool first = true;

  sGCTimer->InitWithFuncCallback(GCTimerFired, reinterpret_cast<void *>(aReason),
                                 aDelay
                                 ? aDelay
                                 : (first
                                    ? NS_FIRST_GC_DELAY
                                    : NS_GC_DELAY),
                                 nsITimer::TYPE_ONE_SHOT);

  first = false;
}


void
nsJSContext::PokeShrinkGCBuffers()
{
  if (sShrinkGCBuffersTimer || sShuttingDown) {
    return;
  }

  CallCreateInstance("@mozilla.org/timer;1", &sShrinkGCBuffersTimer);

  if (!sShrinkGCBuffersTimer) {
    
    return;
  }

  sShrinkGCBuffersTimer->InitWithFuncCallback(ShrinkGCBuffersTimerFired, nullptr,
                                              NS_SHRINK_GC_BUFFERS_DELAY,
                                              nsITimer::TYPE_ONE_SHOT);
}


void
nsJSContext::PokeShrinkingGC()
{
  if (sShrinkingGCTimer || sShuttingDown) {
    return;
  }

  CallCreateInstance("@mozilla.org/timer;1", &sShrinkingGCTimer);

  if (!sShrinkingGCTimer) {
    
    return;
  }

  sShrinkingGCTimer->InitWithFuncCallback(ShrinkingGCTimerFired, nullptr,
                                          NS_SHRINKING_GC_DELAY,
                                          nsITimer::TYPE_ONE_SHOT);
}


void
nsJSContext::MaybePokeCC()
{
  if (sCCTimer || sICCTimer || sShuttingDown || !sHasRunGC) {
    return;
  }

  if (ShouldTriggerCC(nsCycleCollector_suspectedCount())) {
    sCCTimerFireCount = 0;
    CallCreateInstance("@mozilla.org/timer;1", &sCCTimer);
    if (!sCCTimer) {
      return;
    }
    
    nsCycleCollector_dispatchDeferredDeletion();

    sCCTimer->InitWithFuncCallback(CCTimerFired, nullptr,
                                   NS_CC_SKIPPABLE_DELAY,
                                   nsITimer::TYPE_REPEATING_SLACK);
  }
}


void
nsJSContext::KillGCTimer()
{
  if (sGCTimer) {
    sGCTimer->Cancel();
    NS_RELEASE(sGCTimer);
  }
}

void
nsJSContext::KillFullGCTimer()
{
  if (sFullGCTimer) {
    sFullGCTimer->Cancel();
    NS_RELEASE(sFullGCTimer);
  }
}

void
nsJSContext::KillInterSliceGCTimer()
{
  if (sInterSliceGCTimer) {
    sInterSliceGCTimer->Cancel();
    NS_RELEASE(sInterSliceGCTimer);
  }
}


void
nsJSContext::KillShrinkGCBuffersTimer()
{
  if (sShrinkGCBuffersTimer) {
    sShrinkGCBuffersTimer->Cancel();
    NS_RELEASE(sShrinkGCBuffersTimer);
  }
}


void
nsJSContext::KillShrinkingGCTimer()
{
  if (sShrinkingGCTimer) {
    sShrinkingGCTimer->Cancel();
    NS_RELEASE(sShrinkingGCTimer);
  }
}


void
nsJSContext::KillCCTimer()
{
  sCCLockedOutTime = 0;
  if (sCCTimer) {
    sCCTimer->Cancel();
    NS_RELEASE(sCCTimer);
  }
}


void
nsJSContext::KillICCTimer()
{
  sCCLockedOutTime = 0;

  if (sICCTimer) {
    sICCTimer->Cancel();
    NS_RELEASE(sICCTimer);
  }
}

class NotifyGCEndRunnable : public nsRunnable
{
  nsString mMessage;

public:
  explicit NotifyGCEndRunnable(const nsString& aMessage) : mMessage(aMessage) {}

  NS_DECL_NSIRUNNABLE
};

NS_IMETHODIMP
NotifyGCEndRunnable::Run()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
  if (!observerService) {
    return NS_OK;
  }

  const char16_t oomMsg[3] = { '{', '}', 0 };
  const char16_t *toSend = mMessage.get() ? mMessage.get() : oomMsg;
  observerService->NotifyObservers(nullptr, "garbage-collection-statistics", toSend);

  return NS_OK;
}

static void
DOMGCSliceCallback(JSRuntime *aRt, JS::GCProgress aProgress, const JS::GCDescription &aDesc)
{
  NS_ASSERTION(NS_IsMainThread(), "GCs must run on the main thread");

  switch (aProgress) {
    case JS::GC_CYCLE_BEGIN: {
      
      sCCLockedOut = true;

      nsJSContext::KillShrinkGCBuffersTimer();

      break;
    }

    case JS::GC_CYCLE_END: {
      PRTime delta = GetCollectionTimeDelta();

      if (sPostGCEventsToConsole) {
        NS_NAMED_LITERAL_STRING(kFmt, "GC(T+%.1f) ");
        nsString prefix, gcstats;
        gcstats.Adopt(aDesc.formatMessage(aRt));
        prefix.Adopt(nsTextFormatter::smprintf(kFmt.get(),
                                             double(delta) / PR_USEC_PER_SEC));
        nsString msg = prefix + gcstats;
        nsCOMPtr<nsIConsoleService> cs = do_GetService(NS_CONSOLESERVICE_CONTRACTID);
        if (cs) {
          cs->LogStringMessage(msg.get());
        }
      }

      if (sPostGCEventsToObserver) {
        nsString json;
        json.Adopt(aDesc.formatJSON(aRt, PR_Now()));
        nsRefPtr<NotifyGCEndRunnable> notify = new NotifyGCEndRunnable(json);
        NS_DispatchToMainThread(notify);
      }

      sCCLockedOut = false;
      sIsCompactingOnUserInactive = false;

      
      nsJSContext::KillInterSliceGCTimer();

      sCCollectedWaitingForGC = 0;
      sCCollectedZonesWaitingForGC = 0;
      sLikelyShortLivingObjectsNeedingGC = 0;
      sCleanupsSinceLastGC = 0;
      sNeedsFullCC = true;
      sHasRunGC = true;
      nsJSContext::MaybePokeCC();

      if (aDesc.isCompartment_) {
        if (!sFullGCTimer && !sShuttingDown) {
          CallCreateInstance("@mozilla.org/timer;1", &sFullGCTimer);
          sFullGCTimer->InitWithFuncCallback(FullGCTimerFired,
                                             nullptr,
                                             NS_FULL_GC_DELAY,
                                             nsITimer::TYPE_ONE_SHOT);
        }
      } else {
        nsJSContext::KillFullGCTimer();

        
        
        
        if (aDesc.invocationKind_ == GC_NORMAL) {
          nsJSContext::PokeShrinkGCBuffers();
        }
      }

      if (ShouldTriggerCC(nsCycleCollector_suspectedCount())) {
        nsCycleCollector_dispatchDeferredDeletion();
      }

      break;
    }

    case JS::GC_SLICE_BEGIN:
      break;

    case JS::GC_SLICE_END:

      
      nsJSContext::KillInterSliceGCTimer();
      if (!sShuttingDown) {
        CallCreateInstance("@mozilla.org/timer;1", &sInterSliceGCTimer);
        sInterSliceGCTimer->InitWithFuncCallback(InterSliceGCTimerFired,
                                                 nullptr,
                                                 NS_INTERSLICE_GC_DELAY,
                                                 nsITimer::TYPE_ONE_SHOT);
      }

      if (ShouldTriggerCC(nsCycleCollector_suspectedCount())) {
        nsCycleCollector_dispatchDeferredDeletion();
      }

      break;

    default:
      MOZ_CRASH("Unexpected GCProgress value");
  }

  if (sPrevGCSliceCallback) {
    (*sPrevGCSliceCallback)(aRt, aProgress, aDesc);
  }

}

void
nsJSContext::SetWindowProxy(JS::Handle<JSObject*> aWindowProxy)
{
  mWindowProxy = aWindowProxy;
}

JSObject*
nsJSContext::GetWindowProxy()
{
  JSObject* windowProxy = GetWindowProxyPreserveColor();
  if (windowProxy) {
    JS::ExposeObjectToActiveJS(windowProxy);
  }

  return windowProxy;
}

JSObject*
nsJSContext::GetWindowProxyPreserveColor()
{
  return mWindowProxy;
}

void
nsJSContext::LikelyShortLivingObjectCreated()
{
  ++sLikelyShortLivingObjectsNeedingGC;
}

void
mozilla::dom::StartupJSEnvironment()
{
  
  sGCTimer = sShrinkingGCTimer = sFullGCTimer = sCCTimer = sICCTimer = nullptr;
  sCCLockedOut = false;
  sCCLockedOutTime = 0;
  sLastCCEndTime = TimeStamp();
  sHasRunGC = false;
  sPendingLoadCount = 0;
  sLoadingInProgress = false;
  sCCollectedWaitingForGC = 0;
  sCCollectedZonesWaitingForGC = 0;
  sLikelyShortLivingObjectsNeedingGC = 0;
  sPostGCEventsToConsole = false;
  sNeedsFullCC = false;
  sNeedsGCAfterCC = false;
  gNameSpaceManager = nullptr;
  sRuntimeService = nullptr;
  sRuntime = nullptr;
  sIsInitialized = false;
  sDidShutdown = false;
  sShuttingDown = false;
  sContextCount = 0;
  sSecurityManager = nullptr;
  gCCStats.Init();
  sExpensiveCollectorPokes = 0;
}

static void
ReportAllJSExceptionsPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  bool reportAll = Preferences::GetBool(aPrefName, false);
  nsContentUtils::XPConnect()->SetReportAllJSExceptions(reportAll);
}

static void
SetMemoryHighWaterMarkPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  int32_t highwatermark = Preferences::GetInt(aPrefName, 128);

  JS_SetGCParameter(sRuntime, JSGC_MAX_MALLOC_BYTES,
                    highwatermark * 1024L * 1024L);
}

static void
SetMemoryMaxPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  int32_t pref = Preferences::GetInt(aPrefName, -1);
  
  uint32_t max = (pref <= 0 || pref >= 0x1000) ? -1 : (uint32_t)pref * 1024 * 1024;
  JS_SetGCParameter(sRuntime, JSGC_MAX_BYTES, max);
}

static void
SetMemoryGCModePrefChangedCallback(const char* aPrefName, void* aClosure)
{
  bool enableCompartmentGC = Preferences::GetBool("javascript.options.mem.gc_per_compartment");
  bool enableIncrementalGC = Preferences::GetBool("javascript.options.mem.gc_incremental");
  JSGCMode mode;
  if (enableIncrementalGC) {
    mode = JSGC_MODE_INCREMENTAL;
  } else if (enableCompartmentGC) {
    mode = JSGC_MODE_COMPARTMENT;
  } else {
    mode = JSGC_MODE_GLOBAL;
  }
  JS_SetGCParameter(sRuntime, JSGC_MODE, mode);
}

static void
SetMemoryGCSliceTimePrefChangedCallback(const char* aPrefName, void* aClosure)
{
  int32_t pref = Preferences::GetInt(aPrefName, -1);
  
  if (pref > 0 && pref < 100000)
    JS_SetGCParameter(sRuntime, JSGC_SLICE_TIME_BUDGET, pref);
}

static void
SetMemoryGCCompactingPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  bool pref = Preferences::GetBool(aPrefName);
  JS_SetGCParameter(sRuntime, JSGC_COMPACTING_ENABLED, pref);
}

static void
SetMemoryGCPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  int32_t pref = Preferences::GetInt(aPrefName, -1);
  
  if (pref >= 0 && pref < 10000)
    JS_SetGCParameter(sRuntime, (JSGCParamKey)(intptr_t)aClosure, pref);
}

static void
SetMemoryGCDynamicHeapGrowthPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  bool pref = Preferences::GetBool(aPrefName);
  JS_SetGCParameter(sRuntime, JSGC_DYNAMIC_HEAP_GROWTH, pref);
}

static void
SetMemoryGCDynamicMarkSlicePrefChangedCallback(const char* aPrefName, void* aClosure)
{
  bool pref = Preferences::GetBool(aPrefName);
  JS_SetGCParameter(sRuntime, JSGC_DYNAMIC_MARK_SLICE, pref);
}

static void
SetIncrementalCCPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  bool pref = Preferences::GetBool(aPrefName);
  sIncrementalCC = pref;
}

JSObject*
NS_DOMReadStructuredClone(JSContext* cx,
                          JSStructuredCloneReader* reader,
                          uint32_t tag,
                          uint32_t data,
                          void* closure)
{
  if (tag == SCTAG_DOM_IMAGEDATA) {
    return ReadStructuredCloneImageData(cx, reader);
  } else if (tag == SCTAG_DOM_WEBCRYPTO_KEY) {
    nsIGlobalObject *global = xpc::NativeGlobal(JS::CurrentGlobalOrNull(cx));
    if (!global) {
      return nullptr;
    }

    
    JS::Rooted<JSObject*> result(cx);
    {
      nsRefPtr<CryptoKey> key = new CryptoKey(global);
      if (!key->ReadStructuredClone(reader)) {
        result = nullptr;
      } else {
        result = key->WrapObject(cx, JS::NullPtr());
      }
    }
    return result;
  } else if (tag == SCTAG_DOM_NULL_PRINCIPAL ||
             tag == SCTAG_DOM_SYSTEM_PRINCIPAL ||
             tag == SCTAG_DOM_CONTENT_PRINCIPAL) {
    mozilla::ipc::PrincipalInfo info;
    if (tag == SCTAG_DOM_SYSTEM_PRINCIPAL) {
      info = mozilla::ipc::SystemPrincipalInfo();
    } else if (tag == SCTAG_DOM_NULL_PRINCIPAL) {
      info = mozilla::ipc::NullPrincipalInfo();
    } else {
      uint32_t appId = data;

      uint32_t isInBrowserElement, specLength;
      if (!JS_ReadUint32Pair(reader, &isInBrowserElement, &specLength)) {
        return nullptr;
      }

      nsAutoCString spec;
      spec.SetLength(specLength);
      if (!JS_ReadBytes(reader, spec.BeginWriting(), specLength)) {
        return nullptr;
      }

      info = mozilla::ipc::ContentPrincipalInfo(appId, isInBrowserElement, spec);
    }

    nsresult rv;
    nsCOMPtr<nsIPrincipal> principal = PrincipalInfoToPrincipal(info, &rv);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      xpc::Throw(cx, NS_ERROR_DOM_DATA_CLONE_ERR);
      return nullptr;
    }

    JS::RootedValue result(cx);
    rv = nsContentUtils::WrapNative(cx, principal, &NS_GET_IID(nsIPrincipal), &result);
    if (NS_FAILED(rv)) {
      xpc::Throw(cx, NS_ERROR_DOM_DATA_CLONE_ERR);
      return nullptr;
    }

    return result.toObjectOrNull();
  } else if (tag == SCTAG_DOM_NFC_NDEF) {
#ifdef MOZ_NFC
    nsIGlobalObject *global = xpc::NativeGlobal(JS::CurrentGlobalOrNull(cx));
    if (!global) {
      return nullptr;
    }

    
    JS::Rooted<JSObject*> result(cx);
    {
      nsRefPtr<MozNDEFRecord> ndefRecord = new MozNDEFRecord(global);
      result = ndefRecord->ReadStructuredClone(cx, reader) ?
               ndefRecord->WrapObject(cx, JS::NullPtr()) : nullptr;
    }
    return result;
#else
    return nullptr;
#endif
  }

  
  xpc::Throw(cx, NS_ERROR_DOM_DATA_CLONE_ERR);
  return nullptr;
}

bool
NS_DOMWriteStructuredClone(JSContext* cx,
                           JSStructuredCloneWriter* writer,
                           JS::Handle<JSObject*> obj,
                           void *closure)
{
  
  ImageData* imageData;
  if (NS_SUCCEEDED(UNWRAP_OBJECT(ImageData, obj, imageData))) {
    return WriteStructuredCloneImageData(cx, writer, imageData);
  }

  
  CryptoKey* key;
  if (NS_SUCCEEDED(UNWRAP_OBJECT(CryptoKey, obj, key))) {
    return JS_WriteUint32Pair(writer, SCTAG_DOM_WEBCRYPTO_KEY, 0) &&
           key->WriteStructuredClone(writer);
  }

  if (xpc::IsReflector(obj)) {
    nsCOMPtr<nsISupports> base = xpc::UnwrapReflectorToISupports(obj);
    nsCOMPtr<nsIPrincipal> principal = do_QueryInterface(base);
    if (principal) {
      mozilla::ipc::PrincipalInfo info;
      if (NS_WARN_IF(NS_FAILED(PrincipalToPrincipalInfo(principal, &info)))) {
        xpc::Throw(cx, NS_ERROR_DOM_DATA_CLONE_ERR);
        return false;
      }

      if (info.type() == mozilla::ipc::PrincipalInfo::TNullPrincipalInfo) {
        return JS_WriteUint32Pair(writer, SCTAG_DOM_NULL_PRINCIPAL, 0);
      }
      if (info.type() == mozilla::ipc::PrincipalInfo::TSystemPrincipalInfo) {
        return JS_WriteUint32Pair(writer, SCTAG_DOM_SYSTEM_PRINCIPAL, 0);
      }

      MOZ_ASSERT(info.type() == mozilla::ipc::PrincipalInfo::TContentPrincipalInfo);
      const mozilla::ipc::ContentPrincipalInfo& cInfo = info;
      return JS_WriteUint32Pair(writer, SCTAG_DOM_CONTENT_PRINCIPAL, cInfo.appId()) &&
             JS_WriteUint32Pair(writer, cInfo.isInBrowserElement(), cInfo.spec().Length()) &&
             JS_WriteBytes(writer, cInfo.spec().get(), cInfo.spec().Length());
    }
  }

#ifdef MOZ_NFC
  MozNDEFRecord* ndefRecord;
  if (NS_SUCCEEDED(UNWRAP_OBJECT(MozNDEFRecord, obj, ndefRecord))) {
    return JS_WriteUint32Pair(writer, SCTAG_DOM_NFC_NDEF, 0) &&
           ndefRecord->WriteStructuredClone(cx, writer);
  }
#endif 

  
  xpc::Throw(cx, NS_ERROR_DOM_DATA_CLONE_ERR);
  return false;
}

void
NS_DOMStructuredCloneError(JSContext* cx,
                           uint32_t errorid)
{
  
  xpc::Throw(cx, NS_ERROR_DOM_DATA_CLONE_ERR);
}

static bool
AsmJSCacheOpenEntryForRead(JS::Handle<JSObject*> aGlobal,
                           const char16_t* aBegin,
                           const char16_t* aLimit,
                           size_t* aSize,
                           const uint8_t** aMemory,
                           intptr_t *aHandle)
{
  nsIPrincipal* principal =
    nsJSPrincipals::get(JS_GetCompartmentPrincipals(js::GetObjectCompartment(aGlobal)));
  return asmjscache::OpenEntryForRead(principal, aBegin, aLimit, aSize, aMemory,
                                      aHandle);
}

static JS::AsmJSCacheResult
AsmJSCacheOpenEntryForWrite(JS::Handle<JSObject*> aGlobal,
                            bool aInstalled,
                            const char16_t* aBegin,
                            const char16_t* aEnd,
                            size_t aSize,
                            uint8_t** aMemory,
                            intptr_t* aHandle)
{
  nsIPrincipal* principal =
    nsJSPrincipals::get(JS_GetCompartmentPrincipals(js::GetObjectCompartment(aGlobal)));
  return asmjscache::OpenEntryForWrite(principal, aInstalled, aBegin, aEnd,
                                       aSize, aMemory, aHandle);
}

static NS_DEFINE_CID(kDOMScriptObjectFactoryCID, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);

void
nsJSContext::EnsureStatics()
{
  if (sIsInitialized) {
    if (!nsContentUtils::XPConnect()) {
      MOZ_CRASH();
    }
    return;
  }

  nsresult rv = CallGetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID,
                               &sSecurityManager);
  if (NS_FAILED(rv)) {
    MOZ_CRASH();
  }

  rv = CallGetService(kJSRuntimeServiceContractID, &sRuntimeService);
  if (NS_FAILED(rv)) {
    MOZ_CRASH();
  }

  rv = sRuntimeService->GetRuntime(&sRuntime);
  if (NS_FAILED(rv)) {
    MOZ_CRASH();
  }

  
  MOZ_ASSERT(NS_IsMainThread());

  sPrevGCSliceCallback = JS::SetGCSliceCallback(sRuntime, DOMGCSliceCallback);

  
  static const JSStructuredCloneCallbacks cloneCallbacks = {
    NS_DOMReadStructuredClone,
    NS_DOMWriteStructuredClone,
    NS_DOMStructuredCloneError,
    nullptr,
    nullptr,
    nullptr
  };
  JS_SetStructuredCloneCallbacks(sRuntime, &cloneCallbacks);

  
  static const JS::AsmJSCacheOps asmJSCacheOps = {
    AsmJSCacheOpenEntryForRead,
    asmjscache::CloseEntryForRead,
    AsmJSCacheOpenEntryForWrite,
    asmjscache::CloseEntryForWrite,
    asmjscache::GetBuildId
  };
  JS::SetAsmJSCacheOps(sRuntime, &asmJSCacheOps);

  
  Preferences::RegisterCallbackAndCall(ReportAllJSExceptionsPrefChangedCallback,
                                       "dom.report_all_js_exceptions");

  Preferences::RegisterCallbackAndCall(SetMemoryHighWaterMarkPrefChangedCallback,
                                       "javascript.options.mem.high_water_mark");

  Preferences::RegisterCallbackAndCall(SetMemoryMaxPrefChangedCallback,
                                       "javascript.options.mem.max");

  Preferences::RegisterCallbackAndCall(SetMemoryGCModePrefChangedCallback,
                                       "javascript.options.mem.gc_per_compartment");

  Preferences::RegisterCallbackAndCall(SetMemoryGCModePrefChangedCallback,
                                       "javascript.options.mem.gc_incremental");

  Preferences::RegisterCallbackAndCall(SetMemoryGCSliceTimePrefChangedCallback,
                                       "javascript.options.mem.gc_incremental_slice_ms");

  Preferences::RegisterCallbackAndCall(SetMemoryGCCompactingPrefChangedCallback,
                                       "javascript.options.mem.gc_compacting");

  Preferences::RegisterCallbackAndCall(SetMemoryGCPrefChangedCallback,
                                       "javascript.options.mem.gc_high_frequency_time_limit_ms",
                                       (void *)JSGC_HIGH_FREQUENCY_TIME_LIMIT);

  Preferences::RegisterCallbackAndCall(SetMemoryGCDynamicMarkSlicePrefChangedCallback,
                                       "javascript.options.mem.gc_dynamic_mark_slice");

  Preferences::RegisterCallbackAndCall(SetMemoryGCDynamicHeapGrowthPrefChangedCallback,
                                       "javascript.options.mem.gc_dynamic_heap_growth");

  Preferences::RegisterCallbackAndCall(SetMemoryGCPrefChangedCallback,
                                       "javascript.options.mem.gc_low_frequency_heap_growth",
                                       (void *)JSGC_LOW_FREQUENCY_HEAP_GROWTH);

  Preferences::RegisterCallbackAndCall(SetMemoryGCPrefChangedCallback,
                                       "javascript.options.mem.gc_high_frequency_heap_growth_min",
                                       (void *)JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MIN);

  Preferences::RegisterCallbackAndCall(SetMemoryGCPrefChangedCallback,
                                       "javascript.options.mem.gc_high_frequency_heap_growth_max",
                                       (void *)JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MAX);

  Preferences::RegisterCallbackAndCall(SetMemoryGCPrefChangedCallback,
                                       "javascript.options.mem.gc_high_frequency_low_limit_mb",
                                       (void *)JSGC_HIGH_FREQUENCY_LOW_LIMIT);

  Preferences::RegisterCallbackAndCall(SetMemoryGCPrefChangedCallback,
                                       "javascript.options.mem.gc_high_frequency_high_limit_mb",
                                       (void *)JSGC_HIGH_FREQUENCY_HIGH_LIMIT);

  Preferences::RegisterCallbackAndCall(SetMemoryGCPrefChangedCallback,
                                       "javascript.options.mem.gc_allocation_threshold_mb",
                                       (void *)JSGC_ALLOCATION_THRESHOLD);

  Preferences::RegisterCallbackAndCall(SetMemoryGCPrefChangedCallback,
                                       "javascript.options.mem.gc_decommit_threshold_mb",
                                       (void *)JSGC_DECOMMIT_THRESHOLD);

  Preferences::RegisterCallbackAndCall(SetIncrementalCCPrefChangedCallback,
                                       "dom.cycle_collector.incremental");

  Preferences::RegisterCallbackAndCall(SetMemoryGCPrefChangedCallback,
                                       "javascript.options.mem.gc_min_empty_chunk_count",
                                       (void *)JSGC_MIN_EMPTY_CHUNK_COUNT);

  Preferences::RegisterCallbackAndCall(SetMemoryGCPrefChangedCallback,
                                       "javascript.options.mem.gc_max_empty_chunk_count",
                                       (void *)JSGC_MAX_EMPTY_CHUNK_COUNT);

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (!obs) {
    MOZ_CRASH();
  }

  Preferences::AddBoolVarCache(&sGCOnMemoryPressure,
                               "javascript.options.gc_on_memory_pressure",
                               true);

  Preferences::AddBoolVarCache(&sCompactOnUserInactive,
                               "javascript.options.compact_on_user_inactive",
                               true);

  nsIObserver* observer = new nsJSEnvironmentObserver();
  obs->AddObserver(observer, "memory-pressure", false);
  obs->AddObserver(observer, "user-interaction-inactive", false);
  obs->AddObserver(observer, "user-interaction-active", false);
  obs->AddObserver(observer, "quit-application", false);
  obs->AddObserver(observer, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);

  
  
  
  
  nsCOMPtr<nsIDOMScriptObjectFactory> factory = do_GetService(kDOMScriptObjectFactoryCID);
  if (!factory) {
    MOZ_CRASH();
  }

  sIsInitialized = true;
}

void
nsJSContext::NotifyDidPaint()
{
  sDidPaintAfterPreviousICCSlice = true;
}

nsScriptNameSpaceManager*
mozilla::dom::GetNameSpaceManager()
{
  if (sDidShutdown)
    return nullptr;

  if (!gNameSpaceManager) {
    gNameSpaceManager = new nsScriptNameSpaceManager;
    NS_ADDREF(gNameSpaceManager);

    nsresult rv = gNameSpaceManager->Init();
    NS_ENSURE_SUCCESS(rv, nullptr);
  }

  return gNameSpaceManager;
}

nsScriptNameSpaceManager*
mozilla::dom::PeekNameSpaceManager()
{
  return gNameSpaceManager;
}

void
mozilla::dom::ShutdownJSEnvironment()
{
  KillTimers();

  NS_IF_RELEASE(gNameSpaceManager);

  if (!sContextCount) {
    
    

    NS_IF_RELEASE(sRuntimeService);
    NS_IF_RELEASE(sSecurityManager);
  }

  sShuttingDown = true;
  sDidShutdown = true;
}







class nsJSArgArray final : public nsIJSArgArray {
public:
  nsJSArgArray(JSContext *aContext, uint32_t argc, JS::Value *argv,
               nsresult *prv);

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsJSArgArray,
                                                         nsIJSArgArray)

  
  NS_DECL_NSIARRAY

  
  nsresult GetArgs(uint32_t* argc, void** argv) override;

  void ReleaseJSObjects();

protected:
  ~nsJSArgArray();
  JSContext *mContext;
  JS::Heap<JS::Value> *mArgv;
  uint32_t mArgc;
};

nsJSArgArray::nsJSArgArray(JSContext *aContext, uint32_t argc, JS::Value *argv,
                           nsresult *prv) :
    mContext(aContext),
    mArgv(nullptr),
    mArgc(argc)
{
  
  
  if (argc) {
    mArgv = new (fallible) JS::Heap<JS::Value>[argc];
    if (!mArgv) {
      *prv = NS_ERROR_OUT_OF_MEMORY;
      return;
    }
  }

  
  
  if (argv) {
    for (uint32_t i = 0; i < argc; ++i)
      mArgv[i] = argv[i];
  }

  if (argc > 0) {
    mozilla::HoldJSObjects(this);
  }

  *prv = NS_OK;
}

nsJSArgArray::~nsJSArgArray()
{
  ReleaseJSObjects();
}

void
nsJSArgArray::ReleaseJSObjects()
{
  if (mArgv) {
    delete [] mArgv;
  }
  if (mArgc > 0) {
    mArgc = 0;
    mozilla::DropJSObjects(this);
  }
}


NS_IMPL_CYCLE_COLLECTION_CLASS(nsJSArgArray)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsJSArgArray)
  tmp->ReleaseJSObjects();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsJSArgArray)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsJSArgArray)
  if (tmp->mArgv) {
    for (uint32_t i = 0; i < tmp->mArgc; ++i) {
      NS_IMPL_CYCLE_COLLECTION_TRACE_JSVAL_MEMBER_CALLBACK(mArgv[i])
      }
  }
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsJSArgArray)
  NS_INTERFACE_MAP_ENTRY(nsIArray)
  NS_INTERFACE_MAP_ENTRY(nsIJSArgArray)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIJSArgArray)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsJSArgArray)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsJSArgArray)

nsresult
nsJSArgArray::GetArgs(uint32_t *argc, void **argv)
{
  *argv = (void *)mArgv;
  *argc = mArgc;
  return NS_OK;
}


NS_IMETHODIMP nsJSArgArray::GetLength(uint32_t *aLength)
{
  *aLength = mArgc;
  return NS_OK;
}


NS_IMETHODIMP nsJSArgArray::QueryElementAt(uint32_t index, const nsIID & uuid, void * *result)
{
  *result = nullptr;
  if (index >= mArgc)
    return NS_ERROR_INVALID_ARG;

  if (uuid.Equals(NS_GET_IID(nsIVariant)) || uuid.Equals(NS_GET_IID(nsISupports))) {
    
    JS::Rooted<JS::Value> val(mContext, mArgv[index]);
    return nsContentUtils::XPConnect()->JSToVariant(mContext, val,
                                                    (nsIVariant **)result);
  }
  NS_WARNING("nsJSArgArray only handles nsIVariant");
  return NS_ERROR_NO_INTERFACE;
}


NS_IMETHODIMP nsJSArgArray::IndexOf(uint32_t startIndex, nsISupports *element, uint32_t *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsJSArgArray::Enumerate(nsISimpleEnumerator **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


nsresult NS_CreateJSArgv(JSContext *aContext, uint32_t argc, void *argv,
                         nsIJSArgArray **aArray)
{
  nsresult rv;
  nsCOMPtr<nsIJSArgArray> ret = new nsJSArgArray(aContext, argc,
                                                static_cast<JS::Value *>(argv), &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }
  ret.forget(aArray);
  return NS_OK;
}
