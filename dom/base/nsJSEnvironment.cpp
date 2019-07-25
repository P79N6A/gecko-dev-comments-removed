






































#include "nsJSEnvironment.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIDOMChromeWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIScriptSecurityManager.h"
#include "nsDOMCID.h"
#include "nsIServiceManager.h"
#include "nsIXPConnect.h"
#include "nsIJSContextStack.h"
#include "nsIJSRuntimeService.h"
#include "nsCOMPtr.h"
#include "nsISupportsPrimitives.h"
#include "nsReadableUtils.h"
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
#include "nsGUIEvent.h"
#include "nsThreadUtils.h"
#include "nsITimer.h"
#include "nsIAtom.h"
#include "nsContentUtils.h"
#include "nsEventDispatcher.h"
#include "nsIContent.h"
#include "nsCycleCollector.h"
#include "nsNetUtil.h"
#include "nsXPCOMCIDInternal.h"
#include "nsIXULRuntime.h"

#include "nsDOMClassInfo.h"
#include "xpcpublic.h"

#include "jsdbgapi.h"           
#include "jswrapper.h"
#include "nsIArray.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsDOMScriptObjectHolder.h"
#include "prmem.h"
#include "WrapperFactory.h"
#include "nsGlobalWindow.h"
#include "nsScriptNameSpaceManager.h"

#include "nsJSPrincipals.h"

#ifdef XP_MACOSX

#undef check
#endif
#include "AccessCheck.h"

#ifdef MOZ_JSDEBUGGER
#include "jsdIDebuggerService.h"
#endif
#ifdef MOZ_LOGGING

#define FORCE_PR_LOG 1
#endif
#include "prlog.h"
#include "prthread.h"

#include "mozilla/FunctionTimer.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"

#include "sampler.h"

using namespace mozilla;

const size_t gStackSize = 8192;

#ifdef PR_LOGGING
static PRLogModuleInfo* gJSDiagnostics;
#endif


#ifdef CompareString
#undef CompareString
#endif

#define NS_SHRINK_GC_BUFFERS_DELAY  4000 // ms



#define NS_FIRST_GC_DELAY           10000 // ms


#define NS_INTERSLICE_GC_DELAY      100 // ms



#define NS_CC_DELAY                 6000 // ms

#define NS_CC_SKIPPABLE_DELAY       400 // ms


#define NS_CC_FORCED                (2 * 60 * PR_USEC_PER_SEC) // 2 min


#define NS_MAX_CC_LOCKEDOUT_TIME    (5 * PR_USEC_PER_SEC) // 5 seconds


#define NS_CC_PURPLE_LIMIT          250

#define JAVASCRIPT nsIProgrammingLanguage::JAVASCRIPT



static nsITimer *sGCTimer;
static nsITimer *sShrinkGCBuffersTimer;
static nsITimer *sCCTimer;

static PRTime sLastCCEndTime;

static bool sCCLockedOut;
static PRTime sCCLockedOutTime;

static js::GCSliceCallback sPrevGCSliceCallback;







static PRUint32 sPendingLoadCount;
static bool sLoadingInProgress;

static PRUint32 sCCollectedWaitingForGC;
static bool sPostGCEventsToConsole;
static PRUint32 sCCTimerFireCount = 0;
static PRUint32 sMinForgetSkippableTime = PR_UINT32_MAX;
static PRUint32 sMaxForgetSkippableTime = 0;
static PRUint32 sTotalForgetSkippableTime = 0;
static PRUint32 sRemovedPurples = 0;
static PRUint32 sForgetSkippableBeforeCC = 0;
static PRUint32 sPreviousSuspectedCount = 0;

static bool sCleanupSinceLastGC = true;
static bool sNeedsFullCC = false;

nsScriptNameSpaceManager *gNameSpaceManager;

static nsIJSRuntimeService *sRuntimeService;
JSRuntime *nsJSRuntime::sRuntime;

static const char kJSRuntimeServiceContractID[] =
  "@mozilla.org/js/xpc/RuntimeService;1";

static PRTime sFirstCollectionTime;

static bool sIsInitialized;
static bool sDidShutdown;

static PRInt32 sContextCount;

static PRTime sMaxScriptRunTime;
static PRTime sMaxChromeScriptRunTime;

static nsIScriptSecurityManager *sSecurityManager;





static bool sGCOnMemoryPressure;

class nsMemoryPressureObserver : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS1(nsMemoryPressureObserver, nsIObserver)

NS_IMETHODIMP
nsMemoryPressureObserver::Observe(nsISupports* aSubject, const char* aTopic,
                                  const PRUnichar* aData)
{
  if (sGCOnMemoryPressure) {
    nsJSContext::GarbageCollectNow(js::gcreason::MEM_PRESSURE, nsGCShrinking);
    nsJSContext::CycleCollectNow();
  }
  return NS_OK;
}

class nsRootedJSValueArray {
public:
  explicit nsRootedJSValueArray(JSContext *cx) : avr(cx, vals.Length(), vals.Elements()) {}

  bool SetCapacity(JSContext *cx, size_t capacity) {
    bool ok = vals.SetCapacity(capacity);
    if (!ok)
      return false;
    
    memset(vals.Elements(), 0, vals.Capacity() * sizeof(jsval));
    resetRooter(cx);
    return true;
  }

  jsval *Elements() {
    return vals.Elements();
  }

private:
  void resetRooter(JSContext *cx) {
    avr.changeArray(vals.Elements(), vals.Length());
  }

  nsAutoTArray<jsval, 16> vals;
  JS::AutoArrayRooter avr;
};





class AutoFree {
public:
  AutoFree(void *aPtr) : mPtr(aPtr) {
  }
  ~AutoFree() {
    if (mPtr)
      nsMemory::Free(mPtr);
  }
  void Invalidate() {
    mPtr = 0;
  }
private:
  void *mPtr;
};





bool
NS_HandleScriptError(nsIScriptGlobalObject *aScriptGlobal,
                     nsScriptErrorEvent *aErrorEvent,
                     nsEventStatus *aStatus)
{
  bool called = false;
  nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(aScriptGlobal));
  nsIDocShell *docShell = win ? win->GetDocShell() : nsnull;
  if (docShell) {
    nsRefPtr<nsPresContext> presContext;
    docShell->GetPresContext(getter_AddRefs(presContext));

    static PRInt32 errorDepth; 
    ++errorDepth;

    if (presContext && errorDepth < 2) {
      
      
      nsEventDispatcher::Dispatch(win, presContext, aErrorEvent, nsnull,
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
  ScriptErrorEvent(nsIScriptGlobalObject* aScriptGlobal,
                   nsIPrincipal* aScriptOriginPrincipal,
                   PRUint32 aLineNr, PRUint32 aColumn, PRUint32 aFlags,
                   const nsAString& aErrorMsg,
                   const nsAString& aFileName,
                   const nsAString& aSourceLine,
                   bool aDispatchEvent,
                   PRUint64 aInnerWindowID)
    : mScriptGlobal(aScriptGlobal), mOriginPrincipal(aScriptOriginPrincipal),
      mLineNr(aLineNr), mColumn(aColumn),
    mFlags(aFlags), mErrorMsg(aErrorMsg), mFileName(aFileName),
    mSourceLine(aSourceLine), mDispatchEvent(aDispatchEvent),
    mInnerWindowID(aInnerWindowID)
  {}

  NS_IMETHOD Run()
  {
    nsEventStatus status = nsEventStatus_eIgnore;
    
    if (mDispatchEvent) {
      nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(mScriptGlobal));
      nsIDocShell* docShell = win ? win->GetDocShell() : nsnull;
      if (docShell &&
          !JSREPORT_IS_WARNING(mFlags) &&
          !sHandlingScriptError) {
        sHandlingScriptError = true; 

        nsRefPtr<nsPresContext> presContext;
        docShell->GetPresContext(getter_AddRefs(presContext));

        if (presContext) {
          nsScriptErrorEvent errorevent(true, NS_LOAD_ERROR);

          errorevent.fileName = mFileName.get();

          nsCOMPtr<nsIScriptObjectPrincipal> sop(do_QueryInterface(win));
          NS_ENSURE_STATE(sop);
          nsIPrincipal* p = sop->GetPrincipal();
          NS_ENSURE_STATE(p);

          bool sameOrigin = !mOriginPrincipal;

          if (p && !sameOrigin) {
            if (NS_FAILED(p->Subsumes(mOriginPrincipal, &sameOrigin))) {
              sameOrigin = false;
            }
          }

          NS_NAMED_LITERAL_STRING(xoriginMsg, "Script error.");
          if (sameOrigin) {
            errorevent.errorMsg = mErrorMsg.get();
            errorevent.lineNr = mLineNr;
          } else {
            NS_WARNING("Not same origin error!");
            errorevent.errorMsg = xoriginMsg.get();
            errorevent.lineNr = 0;
          }

          nsEventDispatcher::Dispatch(win, presContext, &errorevent, nsnull,
                                      &status);
        }

        sHandlingScriptError = false;
      }
    }

    if (status != nsEventStatus_eConsumeNoDefault) {
      
      
      nsCOMPtr<nsIScriptError> errorObject =
        do_CreateInstance("@mozilla.org/scripterror;1");

      if (errorObject != nsnull) {
        nsresult rv = NS_ERROR_NOT_AVAILABLE;

        
        nsCOMPtr<nsIScriptObjectPrincipal> scriptPrincipal =
          do_QueryInterface(mScriptGlobal);
        NS_ASSERTION(scriptPrincipal, "Global objects must implement "
                     "nsIScriptObjectPrincipal");
        nsCOMPtr<nsIPrincipal> systemPrincipal;
        sSecurityManager->GetSystemPrincipal(getter_AddRefs(systemPrincipal));
        const char * category =
          scriptPrincipal->GetPrincipal() == systemPrincipal
          ? "chrome javascript"
          : "content javascript";

        rv = errorObject->InitWithWindowID(mErrorMsg.get(), mFileName.get(),
                                           mSourceLine.get(),
                                           mLineNr, mColumn, mFlags,
                                           category, mInnerWindowID);

        if (NS_SUCCEEDED(rv)) {
          nsCOMPtr<nsIConsoleService> consoleService =
            do_GetService(NS_CONSOLESERVICE_CONTRACTID, &rv);
          if (NS_SUCCEEDED(rv)) {
            consoleService->LogMessage(errorObject);
          }
        }
      }
    }
    return NS_OK;
  }


  nsCOMPtr<nsIScriptGlobalObject> mScriptGlobal;
  nsCOMPtr<nsIPrincipal>          mOriginPrincipal;
  PRUint32                        mLineNr;
  PRUint32                        mColumn;
  PRUint32                        mFlags;
  nsString                        mErrorMsg;
  nsString                        mFileName;
  nsString                        mSourceLine;
  bool                            mDispatchEvent;
  PRUint64                        mInnerWindowID;

  static bool sHandlingScriptError;
};

bool ScriptErrorEvent::sHandlingScriptError = false;





void
NS_ScriptErrorReporter(JSContext *cx,
                       const char *message,
                       JSErrorReport *report)
{
  
  
  if (!JSREPORT_IS_WARNING(report->flags)) {
    JSStackFrame * fp = nsnull;
    while ((fp = JS_FrameIterator(cx, &fp))) {
      if (JS_IsScriptFrame(cx, fp)) {
        return;
      }
    }

    nsIXPConnect* xpc = nsContentUtils::XPConnect();
    if (xpc) {
      nsAXPCNativeCallContext *cc = nsnull;
      xpc->GetCurrentNativeCallContext(&cc);
      if (cc) {
        nsAXPCNativeCallContext *prev = cc;
        while (NS_SUCCEEDED(prev->GetPreviousCallContext(&prev)) && prev) {
          PRUint16 lang;
          if (NS_SUCCEEDED(prev->GetLanguage(&lang)) &&
            lang == nsAXPCNativeCallContext::LANG_JS) {
            return;
          }
        }
      }
    }
  }

  
  nsIScriptContext *context = nsJSUtils::GetDynamicScriptContext(cx);

  
  
  ::JS_ClearPendingException(cx);

  if (context) {
    nsIScriptGlobalObject *globalObject = context->GetGlobalObject();

    if (globalObject) {
      nsAutoString fileName, msg;
      if (!report->filename) {
        fileName.SetIsVoid(true);
      } else {
        fileName.AssignWithConversion(report->filename);
      }

      const PRUnichar *m = reinterpret_cast<const PRUnichar*>
                                             (report->ucmessage);
      if (m) {
        msg.Assign(m);
      }

      if (msg.IsEmpty() && message) {
        msg.AssignWithConversion(message);
      }


      





      nsAutoString sourceLine;
      sourceLine.Assign(reinterpret_cast<const PRUnichar*>(report->uclinebuf));
      nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(globalObject);
      PRUint64 innerWindowID = 0;
      if (win) {
        nsCOMPtr<nsPIDOMWindow> innerWin = win->GetCurrentInnerWindow();
        if (innerWin) {
          innerWindowID = innerWin->WindowID();
        }
      }
      nsContentUtils::AddScriptRunner(
        new ScriptErrorEvent(globalObject,
                             nsJSPrincipals::get(report->originPrincipals),
                             report->lineno,
                             report->uctokenptr - report->uclinebuf,
                             report->flags, msg, fileName, sourceLine,
                             report->errorNumber != JSMSG_OUT_OF_MEMORY,
                             innerWindowID));
    }
  }

#ifdef DEBUG
  
  
  nsCAutoString error;
  error.Assign("JavaScript ");
  if (JSREPORT_IS_STRICT(report->flags))
    error.Append("strict ");
  if (JSREPORT_IS_WARNING(report->flags))
    error.Append("warning: ");
  else
    error.Append("error: ");
  error.Append(report->filename);
  error.Append(", line ");
  error.AppendInt(report->lineno, 10);
  error.Append(": ");
  if (report->ucmessage) {
    AppendUTF16toUTF8(reinterpret_cast<const PRUnichar*>(report->ucmessage),
                      error);
  } else {
    error.Append(message);
  }

  fprintf(stderr, "%s\n", error.get());
  fflush(stderr);
#endif

#ifdef PR_LOGGING
  if (!gJSDiagnostics)
    gJSDiagnostics = PR_NewLogModule("JSDiagnostics");

  if (gJSDiagnostics) {
    PR_LOG(gJSDiagnostics,
           JSREPORT_IS_WARNING(report->flags) ? PR_LOG_WARNING : PR_LOG_ERROR,
           ("file %s, line %u: %s\n%s%s",
            report->filename, report->lineno, message,
            report->linebuf ? report->linebuf : "",
            (report->linebuf &&
             report->linebuf[strlen(report->linebuf)-1] != '\n')
            ? "\n"
            : ""));
  }
#endif
}

#ifdef DEBUG

nsGlobalWindow *
JSObject2Win(JSContext *cx, JSObject *obj)
{
  nsIXPConnect *xpc = nsContentUtils::XPConnect();
  if (!xpc) {
    return nsnull;
  }

  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  xpc->GetWrappedNativeOfJSObject(cx, obj, getter_AddRefs(wrapper));
  if (wrapper) {
    nsCOMPtr<nsPIDOMWindow> win = do_QueryWrappedNative(wrapper);
    if (win) {
      return static_cast<nsGlobalWindow *>
                        (static_cast<nsPIDOMWindow *>(win));
    }
  }

  return nsnull;
}

void
PrintWinURI(nsGlobalWindow *win)
{
  if (!win) {
    printf("No window passed in.\n");
    return;
  }

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(win->GetExtantDocument());
  if (!doc) {
    printf("No document in the window.\n");
    return;
  }

  nsIURI *uri = doc->GetDocumentURI();
  if (!uri) {
    printf("Document doesn't have a URI.\n");
    return;
  }

  nsCAutoString spec;
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

  nsCAutoString spec;
  uri->GetSpec(spec);
  printf("%s\n", spec.get());
}

void
DumpString(const nsAString &str)
{
  printf("%s\n", NS_ConvertUTF16toUTF8(str).get());
}
#endif

static already_AddRefed<nsIPrompt>
GetPromptFromContext(nsJSContext* ctx)
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(ctx->GetGlobalObject()));
  NS_ENSURE_TRUE(win, nsnull);

  nsIDocShell *docShell = win->GetDocShell();
  NS_ENSURE_TRUE(docShell, nsnull);

  nsCOMPtr<nsIInterfaceRequestor> ireq(do_QueryInterface(docShell));
  NS_ENSURE_TRUE(ireq, nsnull);

  
  nsIPrompt* prompt;
  ireq->GetInterface(NS_GET_IID(nsIPrompt), (void**)&prompt);
  return prompt;
}

JSBool
nsJSContext::DOMOperationCallback(JSContext *cx)
{
  nsresult rv;

  
  nsJSContext *ctx = static_cast<nsJSContext *>(::JS_GetContextPrivate(cx));

  if (!ctx) {
    
    return JS_TRUE;
  }

  
  
  
  
  PRTime callbackTime = ctx->mOperationCallbackTime;
  PRTime modalStateTime = ctx->mModalStateTime;

  
  ctx->mOperationCallbackTime = callbackTime;
  ctx->mModalStateTime = modalStateTime;

  PRTime now = PR_Now();

  if (callbackTime == 0) {
    
    
    ctx->mOperationCallbackTime = now;
    return JS_TRUE;
  }

  if (ctx->mModalStateDepth) {
    
    return JS_TRUE;
  }

  PRTime duration = now - callbackTime;

  
  
  JSObject* global = ::JS_GetGlobalForScopeChain(cx);
  bool isTrackingChromeCodeTime =
    global && xpc::AccessCheck::isChrome(js::GetObjectCompartment(global));
  if (duration < (isTrackingChromeCodeTime ?
                  sMaxChromeScriptRunTime : sMaxScriptRunTime)) {
    return JS_TRUE;
  }

  if (!nsContentUtils::IsSafeToRunScript()) {
    
    
    
    

    JS_ReportWarning(cx, "A long running script was terminated");
    return JS_FALSE;
  }

  
  
  
  nsCOMPtr<nsIPrompt> prompt = GetPromptFromContext(ctx);
  NS_ENSURE_TRUE(prompt, JS_FALSE);

  
  JSScript *script;
  unsigned lineno;
  JSBool hasFrame = ::JS_DescribeScriptedCaller(cx, &script, &lineno);

  bool debugPossible = hasFrame && js::CanCallContextDebugHandler(cx);
#ifdef MOZ_JSDEBUGGER
  
  if (debugPossible) {
    bool jsds_IsOn = false;
    const char jsdServiceCtrID[] = "@mozilla.org/js/jsd/debugger-service;1";
    nsCOMPtr<jsdIExecutionHook> jsdHook;
    nsCOMPtr<jsdIDebuggerService> jsds = do_GetService(jsdServiceCtrID, &rv);

    
    if (NS_SUCCEEDED(rv)) {
      jsds->GetDebuggerHook(getter_AddRefs(jsdHook));
      jsds->GetIsOn(&jsds_IsOn);
    }

    
    
    
    debugPossible = ((jsds_IsOn && (jsdHook != nsnull)) || !jsds_IsOn);
  }
#endif

  
  nsXPIDLString title, msg, stopButton, waitButton, debugButton, neverShowDlg;

  rv = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                          "KillScriptTitle",
                                          title);

  rv |= nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                           "StopScriptButton",
                                           stopButton);

  rv |= nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                           "WaitForScriptButton",
                                           waitButton);

  rv |= nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                           "DontAskAgain",
                                           neverShowDlg);


  if (debugPossible) {
    rv |= nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                             "DebugScriptButton",
                                             debugButton);

    rv |= nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                             "KillScriptWithDebugMessage",
                                             msg);
  }
  else {
    rv |= nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                             "KillScriptMessage",
                                             msg);
  }

  
  if (NS_FAILED(rv) || !title || !msg || !stopButton || !waitButton ||
      (!debugButton && debugPossible) || !neverShowDlg) {
    NS_ERROR("Failed to get localized strings.");
    return JS_TRUE;
  }

  
  if (script) {
    const char *filename = ::JS_GetScriptFilename(cx, script);
    if (filename) {
      nsXPIDLString scriptLocation;
      NS_ConvertUTF8toUTF16 filenameUTF16(filename);
      const PRUnichar *formatParams[] = { filenameUTF16.get() };
      rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                 "KillScriptLocation",
                                                 formatParams,
                                                 scriptLocation);

      if (NS_SUCCEEDED(rv) && scriptLocation) {
        msg.AppendLiteral("\n\n");
        msg.Append(scriptLocation);
        msg.Append(':');
        msg.AppendInt(lineno);
      }
    }
  }

  PRInt32 buttonPressed = 0; 
  bool neverShowDlgChk = false;
  PRUint32 buttonFlags = nsIPrompt::BUTTON_POS_1_DEFAULT +
                         (nsIPrompt::BUTTON_TITLE_IS_STRING *
                          (nsIPrompt::BUTTON_POS_0 + nsIPrompt::BUTTON_POS_1));

  
  if (debugPossible)
    buttonFlags += nsIPrompt::BUTTON_TITLE_IS_STRING * nsIPrompt::BUTTON_POS_2;

  
  ::JS_SetOperationCallback(cx, nsnull);

  
  rv = prompt->ConfirmEx(title, msg, buttonFlags, waitButton, stopButton,
                         debugButton, neverShowDlg, &neverShowDlgChk,
                         &buttonPressed);

  ::JS_SetOperationCallback(cx, DOMOperationCallback);

  if (NS_FAILED(rv) || (buttonPressed == 0)) {
    

    if (neverShowDlgChk) {
      Preferences::SetInt(isTrackingChromeCodeTime ?
        "dom.max_chrome_script_run_time" : "dom.max_script_run_time", 0);
    }

    ctx->mOperationCallbackTime = PR_Now();
    return JS_TRUE;
  }
  else if ((buttonPressed == 2) && debugPossible) {
    return js_CallContextDebugHandler(cx);
  }

  JS_ClearPendingException(cx);
  return JS_FALSE;
}

void
nsJSContext::EnterModalState()
{
  if (!mModalStateDepth) {
    mModalStateTime =  mOperationCallbackTime ? PR_Now() : 0;
  }
  ++mModalStateDepth;
}

void
nsJSContext::LeaveModalState()
{
  if (!mModalStateDepth) {
    NS_ERROR("Uh, mismatched LeaveModalState() call!");

    return;
  }

  --mModalStateDepth;

  
  
  if (mModalStateDepth || !mOperationCallbackTime) {
    return;
  }

  
  
  
  
  
  
  if (mModalStateTime) {
    mOperationCallbackTime += PR_Now() - mModalStateTime;
  }
  else {
    mOperationCallbackTime = PR_Now();
  }
}

#define JS_OPTIONS_DOT_STR "javascript.options."

static const char js_options_dot_str[]   = JS_OPTIONS_DOT_STR;
static const char js_strict_option_str[] = JS_OPTIONS_DOT_STR "strict";
#ifdef DEBUG
static const char js_strict_debug_option_str[] = JS_OPTIONS_DOT_STR "strict.debug";
#endif
static const char js_werror_option_str[] = JS_OPTIONS_DOT_STR "werror";
static const char js_relimit_option_str[]= JS_OPTIONS_DOT_STR "relimit";
#ifdef JS_GC_ZEAL
static const char js_zeal_option_str[]        = JS_OPTIONS_DOT_STR "gczeal";
static const char js_zeal_frequency_str[]     = JS_OPTIONS_DOT_STR "gczeal.frequency";
static const char js_zeal_compartment_str[]   = JS_OPTIONS_DOT_STR "gczeal.compartment_gc";
#endif
static const char js_methodjit_content_str[]  = JS_OPTIONS_DOT_STR "methodjit.content";
static const char js_methodjit_chrome_str[]   = JS_OPTIONS_DOT_STR "methodjit.chrome";
static const char js_methodjit_always_str[]   = JS_OPTIONS_DOT_STR "methodjit_always";
static const char js_typeinfer_str[]          = JS_OPTIONS_DOT_STR "typeinference";
static const char js_pccounts_content_str[]   = JS_OPTIONS_DOT_STR "pccounts.content";
static const char js_pccounts_chrome_str[]    = JS_OPTIONS_DOT_STR "pccounts.chrome";
static const char js_jit_hardening_str[]      = JS_OPTIONS_DOT_STR "jit_hardening";
static const char js_memlog_option_str[] = JS_OPTIONS_DOT_STR "mem.log";

int
nsJSContext::JSOptionChangedCallback(const char *pref, void *data)
{
  nsJSContext *context = reinterpret_cast<nsJSContext *>(data);
  PRUint32 oldDefaultJSOptions = context->mDefaultJSOptions;
  PRUint32 newDefaultJSOptions = oldDefaultJSOptions;

  sPostGCEventsToConsole = Preferences::GetBool(js_memlog_option_str);

  bool strict = Preferences::GetBool(js_strict_option_str);
  if (strict)
    newDefaultJSOptions |= JSOPTION_STRICT;
  else
    newDefaultJSOptions &= ~JSOPTION_STRICT;

  nsIScriptGlobalObject *global = context->GetGlobalObject();
  
  
  nsCOMPtr<nsIDOMChromeWindow> chromeWindow(do_QueryInterface(global));

  bool useMethodJIT = Preferences::GetBool(chromeWindow ?
                                               js_methodjit_chrome_str :
                                               js_methodjit_content_str);
  bool usePCCounts = Preferences::GetBool(chromeWindow ?
                                            js_pccounts_chrome_str :
                                            js_pccounts_content_str);
  bool useMethodJITAlways = Preferences::GetBool(js_methodjit_always_str);
  bool useTypeInference = !chromeWindow && Preferences::GetBool(js_typeinfer_str);
  bool useHardening = Preferences::GetBool(js_jit_hardening_str);
  nsCOMPtr<nsIXULRuntime> xr = do_GetService(XULRUNTIME_SERVICE_CONTRACTID);
  if (xr) {
    bool safeMode = false;
    xr->GetInSafeMode(&safeMode);
    if (safeMode) {
      useMethodJIT = false;
      usePCCounts = false;
      useTypeInference = false;
      useMethodJITAlways = true;
      useHardening = false;
    }
  }    

  if (useMethodJIT)
    newDefaultJSOptions |= JSOPTION_METHODJIT;
  else
    newDefaultJSOptions &= ~JSOPTION_METHODJIT;

  if (usePCCounts)
    newDefaultJSOptions |= JSOPTION_PCCOUNT;
  else
    newDefaultJSOptions &= ~JSOPTION_PCCOUNT;

  if (useMethodJITAlways)
    newDefaultJSOptions |= JSOPTION_METHODJIT_ALWAYS;
  else
    newDefaultJSOptions &= ~JSOPTION_METHODJIT_ALWAYS;

  if (useTypeInference)
    newDefaultJSOptions |= JSOPTION_TYPE_INFERENCE;
  else
    newDefaultJSOptions &= ~JSOPTION_TYPE_INFERENCE;

#ifdef DEBUG
  
  
  bool strictDebug = Preferences::GetBool(js_strict_debug_option_str);
  if (strictDebug && (newDefaultJSOptions & JSOPTION_STRICT) == 0) {
    if (chromeWindow)
      newDefaultJSOptions |= JSOPTION_STRICT;
  }
#endif

  bool werror = Preferences::GetBool(js_werror_option_str);
  if (werror)
    newDefaultJSOptions |= JSOPTION_WERROR;
  else
    newDefaultJSOptions &= ~JSOPTION_WERROR;

  bool relimit = Preferences::GetBool(js_relimit_option_str);
  if (relimit)
    newDefaultJSOptions |= JSOPTION_RELIMIT;
  else
    newDefaultJSOptions &= ~JSOPTION_RELIMIT;

  ::JS_SetOptions(context->mContext, newDefaultJSOptions & JSRUNOPTION_MASK);

  
  context->mDefaultJSOptions = newDefaultJSOptions;

  JSRuntime *rt = JS_GetRuntime(context->mContext);
  JS_SetJitHardening(rt, useHardening);

#ifdef JS_GC_ZEAL
  PRInt32 zeal = Preferences::GetInt(js_zeal_option_str, -1);
  PRInt32 frequency = Preferences::GetInt(js_zeal_frequency_str, JS_DEFAULT_ZEAL_FREQ);
  bool compartment = Preferences::GetBool(js_zeal_compartment_str, false);
  if (zeal >= 0)
    ::JS_SetGCZeal(context->mContext, (PRUint8)zeal, frequency, compartment);
#endif

  return 0;
}

nsJSContext::nsJSContext(JSRuntime *aRuntime)
  : mGCOnDestruction(true),
    mExecuteDepth(0)
{

  ++sContextCount;

  mDefaultJSOptions = JSOPTION_PRIVATE_IS_NSISUPPORTS;

  mContext = ::JS_NewContext(aRuntime, gStackSize);
  if (mContext) {
    ::JS_SetContextPrivate(mContext, static_cast<nsIScriptContext *>(this));

    
    mDefaultJSOptions |= ::JS_GetOptions(mContext);

    
    ::JS_SetOptions(mContext, mDefaultJSOptions);

    
    Preferences::RegisterCallback(JSOptionChangedCallback,
                                  js_options_dot_str, this);

    ::JS_SetOperationCallback(mContext, DOMOperationCallback);

    xpc_LocalizeContext(mContext);
  }
  mIsInitialized = false;
  mTerminations = nsnull;
  mScriptsEnabled = true;
  mOperationCallbackTime = 0;
  mModalStateTime = 0;
  mModalStateDepth = 0;
  mProcessingScriptTag = false;
}

nsJSContext::~nsJSContext()
{
#ifdef DEBUG
  nsCycleCollector_DEBUG_wasFreed(static_cast<nsIScriptContext*>(this));
#endif

  
  
  
  delete mTerminations;

  mGlobalObjectRef = nsnull;

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

  
  ::JS_SetContextPrivate(mContext, nsnull);

  
  Preferences::UnregisterCallback(JSOptionChangedCallback,
                                  js_options_dot_str, this);

  if (mGCOnDestruction) {
    PokeGC(js::gcreason::NSJSCONTEXT_DESTROY);
  }
        
  
  nsIXPConnect *xpc = nsContentUtils::XPConnect();
  if (xpc) {
    xpc->ReleaseJSContext(mContext, true);
  } else {
    ::JS_DestroyContextNoGC(mContext);
  }
  mContext = nsnull;
}


NS_IMPL_CYCLE_COLLECTION_CLASS(nsJSContext)
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsJSContext)
NS_IMPL_CYCLE_COLLECTION_TRACE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsJSContext)
  NS_ASSERTION(!tmp->mContext || js::GetContextOutstandingRequests(tmp->mContext) == 0,
               "Trying to unlink a context with outstanding requests.");
  tmp->mIsInitialized = false;
  tmp->mGCOnDestruction = false;
  tmp->DestroyJSContext();
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mGlobalObjectRef)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INTERNAL(nsJSContext)
  NS_IMPL_CYCLE_COLLECTION_DESCRIBE(nsJSContext, tmp->GetCCRefcnt())
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mGlobalObjectRef)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mContext");
  nsContentUtils::XPConnect()->NoteJSContext(tmp->mContext, cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsJSContext)
  NS_INTERFACE_MAP_ENTRY(nsIScriptContext)
  NS_INTERFACE_MAP_ENTRY(nsIScriptContextPrincipal)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptNotify)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptContext)
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF(nsJSContext)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsJSContext)

nsrefcnt
nsJSContext::GetCCRefcnt()
{
  nsrefcnt refcnt = mRefCnt.get();
  if (NS_LIKELY(mContext))
    refcnt += js::GetContextOutstandingRequests(mContext);
  return refcnt;
}

nsresult
nsJSContext::EvaluateStringWithValue(const nsAString& aScript,
                                     JSObject* aScopeObject,
                                     nsIPrincipal *aPrincipal,
                                     const char *aURL,
                                     PRUint32 aLineNo,
                                     PRUint32 aVersion,
                                     JS::Value* aRetValue,
                                     bool* aIsUndefined)
{
  NS_TIME_FUNCTION_MIN_FMT(1.0, "%s (line %d) (url: %s, line: %d)", MOZ_FUNCTION_NAME,
                           __LINE__, aURL, aLineNo);

  SAMPLE_LABEL("JS", "EvaluateStringWithValue");
  NS_ABORT_IF_FALSE(aScopeObject,
    "Shouldn't call EvaluateStringWithValue with null scope object.");

  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (!mScriptsEnabled) {
    if (aIsUndefined) {
      *aIsUndefined = true;
    }

    return NS_OK;
  }

  
  
  
  nsCOMPtr<nsIPrincipal> principal = aPrincipal;
  nsresult rv;
  if (!aPrincipal) {
    nsIScriptGlobalObject *global = GetGlobalObject();
    if (!global)
      return NS_ERROR_FAILURE;
    nsCOMPtr<nsIScriptObjectPrincipal> objPrincipal =
      do_QueryInterface(global, &rv);
    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;
    principal = objPrincipal->GetPrincipal();
    if (!principal)
      return NS_ERROR_FAILURE;
  }

  bool ok = false;

  rv = sSecurityManager->CanExecuteScripts(mContext, principal, &ok);
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  nsCOMPtr<nsIJSContextStack> stack =
           do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
  if (NS_FAILED(rv) || NS_FAILED(stack->Push(mContext))) {
    return NS_ERROR_FAILURE;
  }

  jsval val;

  rv = sSecurityManager->PushContextPrincipal(mContext, nsnull, principal);
  NS_ENSURE_SUCCESS(rv, rv);

  nsJSContext::TerminationFuncHolder holder(this);

  
  
  
  if (ok && ((JSVersion)aVersion) != JSVERSION_UNKNOWN) {

    JSAutoRequest ar(mContext);

    JSAutoEnterCompartment ac;
    if (!ac.enter(mContext, aScopeObject)) {
      stack->Pop(nsnull);
      return NS_ERROR_FAILURE;
    }

    ++mExecuteDepth;

    ok = ::JS_EvaluateUCScriptForPrincipalsVersion(mContext,
                                                   aScopeObject,
                                                   nsJSPrincipals::get(principal),
                                                   static_cast<const jschar*>(PromiseFlatString(aScript).get()),
                                                   aScript.Length(),
                                                   aURL,
                                                   aLineNo,
                                                   &val,
                                                   JSVersion(aVersion));

    --mExecuteDepth;

    if (!ok) {
      
      
      

      ReportPendingException();
    }
  }

  
  if (ok) {
    if (aIsUndefined) {
      *aIsUndefined = JSVAL_IS_VOID(val);
    }

    *aRetValue = val;
    
    
    
  }
  else {
    if (aIsUndefined) {
      *aIsUndefined = true;
    }
  }

  sSecurityManager->PopContextPrincipal(mContext);

  
  if (NS_FAILED(stack->Pop(nsnull)))
    rv = NS_ERROR_FAILURE;

  
  ScriptEvaluated(true);

  return rv;

}



static nsresult
JSValueToAString(JSContext *cx, jsval val, nsAString *result,
                 bool *isUndefined)
{
  if (isUndefined) {
    *isUndefined = JSVAL_IS_VOID(val);
  }

  if (!result) {
    return NS_OK;
  }

  JSString* jsstring = ::JS_ValueToString(cx, val);
  if (!jsstring) {
    goto error;
  }

  size_t length;
  const jschar *chars;
  chars = ::JS_GetStringCharsAndLength(cx, jsstring, &length);
  if (!chars) {
    goto error;
  }

  result->Assign(chars, length);
  return NS_OK;

error:
  
  
  

  result->Truncate();

  if (isUndefined) {
    *isUndefined = true;
  }

  if (!::JS_IsExceptionPending(cx)) {
    
    

    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

nsIScriptObjectPrincipal*
nsJSContext::GetObjectPrincipal()
{
  nsCOMPtr<nsIScriptObjectPrincipal> prin = do_QueryInterface(GetGlobalObject());
  return prin;
}

nsresult
nsJSContext::EvaluateString(const nsAString& aScript,
                            JSObject* aScopeObject,
                            nsIPrincipal *aPrincipal,
                            nsIPrincipal *aOriginPrincipal,
                            const char *aURL,
                            PRUint32 aLineNo,
                            PRUint32 aVersion,
                            nsAString *aRetValue,
                            bool* aIsUndefined)
{
  NS_TIME_FUNCTION_MIN_FMT(1.0, "%s (line %d) (url: %s, line: %d)", MOZ_FUNCTION_NAME,
                           __LINE__, aURL, aLineNo);

  SAMPLE_LABEL("JS", "EvaluateString");
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (!mScriptsEnabled) {
    if (aIsUndefined) {
      *aIsUndefined = true;
    }

    if (aRetValue) {
      aRetValue->Truncate();
    }

    return NS_OK;
  }

  if (!aScopeObject) {
    aScopeObject = JS_GetGlobalObject(mContext);
  }

  
  
  
  nsCOMPtr<nsIPrincipal> principal = aPrincipal;
  if (!aPrincipal) {
    nsCOMPtr<nsIScriptObjectPrincipal> objPrincipal =
      do_QueryInterface(GetGlobalObject());
    if (!objPrincipal)
      return NS_ERROR_FAILURE;
    principal = objPrincipal->GetPrincipal();
    if (!principal)
      return NS_ERROR_FAILURE;
  }

  bool ok = false;

  nsresult rv = sSecurityManager->CanExecuteScripts(mContext, principal, &ok);
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  nsCOMPtr<nsIJSContextStack> stack =
           do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
  if (NS_FAILED(rv) || NS_FAILED(stack->Push(mContext))) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  jsval val = JSVAL_VOID;
  jsval* vp = aRetValue ? &val : NULL;

  rv = sSecurityManager->PushContextPrincipal(mContext, nsnull, principal);
  NS_ENSURE_SUCCESS(rv, rv);

  nsJSContext::TerminationFuncHolder holder(this);

  ++mExecuteDepth;

  
  
  
  if (ok && JSVersion(aVersion) != JSVERSION_UNKNOWN) {
    JSAutoRequest ar(mContext);
    JSAutoEnterCompartment ac;
    if (!ac.enter(mContext, aScopeObject)) {
      stack->Pop(nsnull);
      return NS_ERROR_FAILURE;
    }

    ok = JS_EvaluateUCScriptForPrincipalsVersionOrigin(
      mContext, aScopeObject,
      nsJSPrincipals::get(principal), nsJSPrincipals::get(aOriginPrincipal),
      static_cast<const jschar*>(PromiseFlatString(aScript).get()),
      aScript.Length(), aURL, aLineNo, vp, JSVersion(aVersion));

    if (!ok) {
      
      
      

      ReportPendingException();
    }
  }

  
  if (ok) {
    JSAutoRequest ar(mContext);
    JSAutoEnterCompartment ac;
    if (!ac.enter(mContext, aScopeObject)) {
      stack->Pop(nsnull);
    }
    rv = JSValueToAString(mContext, val, aRetValue, aIsUndefined);
  }
  else {
    if (aIsUndefined) {
      *aIsUndefined = true;
    }

    if (aRetValue) {
      aRetValue->Truncate();
    }
  }

  --mExecuteDepth;

  sSecurityManager->PopContextPrincipal(mContext);

  
  if (NS_FAILED(stack->Pop(nsnull)))
    rv = NS_ERROR_FAILURE;

  
  ScriptEvaluated(true);

  return rv;
}

nsresult
nsJSContext::CompileScript(const PRUnichar* aText,
                           PRInt32 aTextLength,
                           nsIPrincipal *aPrincipal,
                           const char *aURL,
                           PRUint32 aLineNo,
                           PRUint32 aVersion,
                           nsScriptObjectHolder<JSScript>& aScriptObject)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aPrincipal);

  JSObject* scopeObject = ::JS_GetGlobalObject(mContext);

  bool ok = false;

  nsresult rv = sSecurityManager->CanExecuteScripts(mContext, aPrincipal, &ok);
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  aScriptObject.drop(); 

  
  
  
  if (!ok || JSVersion(aVersion) == JSVERSION_UNKNOWN)
    return NS_OK;
    
  JSAutoRequest ar(mContext);

  JSScript* script =
    ::JS_CompileUCScriptForPrincipalsVersion(mContext,
                                             scopeObject,
                                             nsJSPrincipals::get(aPrincipal),
                                             static_cast<const jschar*>(aText),
                                             aTextLength,
                                             aURL,
                                             aLineNo,
                                             JSVersion(aVersion));
  if (!script) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ASSERTION(aScriptObject.getScriptTypeID()==JAVASCRIPT,
               "Expecting JS script object holder");
  return aScriptObject.set(script);
}

nsresult
nsJSContext::ExecuteScript(JSScript* aScriptObject,
                           JSObject* aScopeObject,
                           nsAString* aRetValue,
                           bool* aIsUndefined)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (!mScriptsEnabled) {
    if (aIsUndefined) {
      *aIsUndefined = true;
    }

    if (aRetValue) {
      aRetValue->Truncate();
    }

    return NS_OK;
  }

  if (!aScopeObject) {
    aScopeObject = JS_GetGlobalObject(mContext);
  }

  
  
  nsresult rv;
  nsCOMPtr<nsIJSContextStack> stack =
           do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
  if (NS_FAILED(rv) || NS_FAILED(stack->Push(mContext))) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIPrincipal> principal;
  rv = sSecurityManager->GetObjectPrincipal(mContext,
                                            JS_GetGlobalFromScript(aScriptObject),
                                            getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sSecurityManager->PushContextPrincipal(mContext, nsnull, principal);
  NS_ENSURE_SUCCESS(rv, rv);

  nsJSContext::TerminationFuncHolder holder(this);
  JSAutoRequest ar(mContext);
  ++mExecuteDepth;

  
  
  
  jsval val;
  bool ok = JS_ExecuteScript(mContext, aScopeObject, aScriptObject, &val);
  if (ok) {
    
    rv = JSValueToAString(mContext, val, aRetValue, aIsUndefined);
  } else {
    ReportPendingException();

    if (aIsUndefined) {
      *aIsUndefined = true;
    }

    if (aRetValue) {
      aRetValue->Truncate();
    }
  }

  --mExecuteDepth;

  sSecurityManager->PopContextPrincipal(mContext);

  
  if (NS_FAILED(stack->Pop(nsnull)))
    rv = NS_ERROR_FAILURE;

  
  ScriptEvaluated(true);

  return rv;
}


#ifdef DEBUG
bool
AtomIsEventHandlerName(nsIAtom *aName)
{
  const PRUnichar *name = aName->GetUTF16String();

  const PRUnichar *cp;
  PRUnichar c;
  for (cp = name; *cp != '\0'; ++cp)
  {
    c = *cp;
    if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z'))
      return false;
  }

  return true;
}
#endif



nsresult
nsJSContext::JSObjectFromInterface(nsISupports* aTarget, JSObject* aScope, JSObject** aRet)
{
  
  if (!aTarget) {
    *aRet = nsnull;
    return NS_OK;
  }

  
  
  
  jsval v;
  nsresult rv = nsContentUtils::WrapNative(mContext, aScope, aTarget, &v);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef NS_DEBUG
  nsCOMPtr<nsISupports> targetSupp = do_QueryInterface(aTarget);
  nsCOMPtr<nsISupports> native =
    nsContentUtils::XPConnect()->GetNativeOfWrapper(mContext,
                                                    JSVAL_TO_OBJECT(v));
  NS_ASSERTION(native == targetSupp, "Native should be the target!");
#endif

  *aRet = JSVAL_TO_OBJECT(v);

  return NS_OK;
}


nsresult
nsJSContext::CompileEventHandler(nsIAtom *aName,
                                 PRUint32 aArgCount,
                                 const char** aArgNames,
                                 const nsAString& aBody,
                                 const char *aURL, PRUint32 aLineNo,
                                 PRUint32 aVersion,
                                 nsScriptObjectHolder<JSObject>& aHandler)
{
  NS_TIME_FUNCTION_MIN_FMT(1.0, "%s (line %d) (url: %s, line: %d)", MOZ_FUNCTION_NAME,
                           __LINE__, aURL, aLineNo);

  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_PRECONDITION(AtomIsEventHandlerName(aName), "Bad event name");
  NS_PRECONDITION(!::JS_IsExceptionPending(mContext),
                  "Why are we being called with a pending exception?");

  if (!sSecurityManager) {
    NS_ERROR("Huh, we need a script security manager to compile "
             "an event handler!");

    return NS_ERROR_UNEXPECTED;
  }

  
  
  if ((JSVersion)aVersion == JSVERSION_UNKNOWN) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

#ifdef DEBUG
  JSContext* top = nsContentUtils::GetCurrentJSContext();
  NS_ASSERTION(mContext == top, "Context not properly pushed!");
#endif

  
  
  
  JSAutoRequest ar(mContext);

  JSFunction* fun =
      ::JS_CompileUCFunctionForPrincipalsVersion(mContext,
                                                 nsnull, nsnull,
                                                 nsAtomCString(aName).get(), aArgCount, aArgNames,
                                                 (jschar*)PromiseFlatString(aBody).get(),
                                                 aBody.Length(),
                                                 aURL, aLineNo, JSVersion(aVersion));

  if (!fun) {
    ReportPendingException();
    return NS_ERROR_ILLEGAL_VALUE;
  }

  JSObject *handler = ::JS_GetFunctionObject(fun);
  NS_ASSERTION(aHandler.getScriptTypeID()==JAVASCRIPT,
               "Expecting JS script object holder");
  return aHandler.set(handler);
}



nsresult
nsJSContext::CompileFunction(JSObject* aTarget,
                             const nsACString& aName,
                             PRUint32 aArgCount,
                             const char** aArgArray,
                             const nsAString& aBody,
                             const char* aURL,
                             PRUint32 aLineNo,
                             PRUint32 aVersion,
                             bool aShared,
                             JSObject** aFunctionObject)
{
  NS_TIME_FUNCTION_FMT(1.0, "%s (line %d) (function: %s, url: %s, line: %d)", MOZ_FUNCTION_NAME,
                       __LINE__, aName.BeginReading(), aURL, aLineNo);

  NS_ABORT_IF_FALSE(aFunctionObject,
    "Shouldn't call CompileFunction with null return value.");

  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  
  
  if ((JSVersion)aVersion == JSVERSION_UNKNOWN) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsIScriptGlobalObject *global = GetGlobalObject();
  nsCOMPtr<nsIPrincipal> principal;
  if (global) {
    
    nsCOMPtr<nsIScriptObjectPrincipal> globalData = do_QueryInterface(global);
    if (globalData) {
      principal = globalData->GetPrincipal();
      if (!principal)
        return NS_ERROR_FAILURE;
    }
  }

  JSObject *target = aTarget;

  JSAutoRequest ar(mContext);

  JSFunction* fun =
      ::JS_CompileUCFunctionForPrincipalsVersion(mContext,
                                                 aShared ? nsnull : target,
                                                 nsJSPrincipals::get(principal),
                                                 PromiseFlatCString(aName).get(),
                                                 aArgCount, aArgArray,
                                                 static_cast<const jschar*>(PromiseFlatString(aBody).get()),
                                                 aBody.Length(),
                                                 aURL, aLineNo,
                                                 JSVersion(aVersion));

  if (!fun)
    return NS_ERROR_FAILURE;

  *aFunctionObject = JS_GetFunctionObject(fun);
  return NS_OK;
}

nsresult
nsJSContext::CallEventHandler(nsISupports* aTarget, JSObject* aScope,
                              JSObject* aHandler, nsIArray* aargv,
                              nsIVariant** arv)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (!mScriptsEnabled) {
    return NS_OK;
  }

#ifdef NS_FUNCTION_TIMER
  {
    JSObject *obj = aHandler;
    if (js::IsFunctionProxy(obj))
      obj = js::UnwrapObject(obj);
    JSString *id = JS_GetFunctionId(static_cast<JSFunction *>(JS_GetPrivate(obj)));
    JSAutoByteString bytes;
    const char *name = !id ? "anonymous" : bytes.encode(mContext, id) ? bytes.ptr() : "<error>";
    NS_TIME_FUNCTION_FMT(1.0, "%s (line %d) (function: %s)", MOZ_FUNCTION_NAME, __LINE__, name);
  }
#endif
  SAMPLE_LABEL("JS", "CallEventHandler");

  JSAutoRequest ar(mContext);
  JSObject* target = nsnull;
  nsresult rv = JSObjectFromInterface(aTarget, aScope, &target);
  NS_ENSURE_SUCCESS(rv, rv);

  JS::AutoObjectRooter targetVal(mContext, target);
  jsval rval = JSVAL_VOID;

  
  
  
  

  nsCxPusher pusher;
  if (!pusher.Push(mContext, true))
    return NS_ERROR_FAILURE;

  
  rv = sSecurityManager->CheckFunctionAccess(mContext, aHandler, target);

  nsJSContext::TerminationFuncHolder holder(this);

  if (NS_SUCCEEDED(rv)) {
    
    PRUint32 argc = 0;
    jsval *argv = nsnull;

    JSObject *funobj = aHandler;
    nsCOMPtr<nsIPrincipal> principal;
    rv = sSecurityManager->GetObjectPrincipal(mContext, funobj,
                                              getter_AddRefs(principal));
    NS_ENSURE_SUCCESS(rv, rv);

    JSStackFrame *currentfp = nsnull;
    rv = sSecurityManager->PushContextPrincipal(mContext,
                                                JS_FrameIterator(mContext, &currentfp),
                                                principal);
    NS_ENSURE_SUCCESS(rv, rv);

    jsval funval = OBJECT_TO_JSVAL(funobj);
    JSAutoEnterCompartment ac;
    js::ForceFrame ff(mContext, funobj);
    if (!ac.enter(mContext, funobj) || !ff.enter() ||
        !JS_WrapObject(mContext, &target)) {
      ReportPendingException();
      sSecurityManager->PopContextPrincipal(mContext);
      return NS_ERROR_FAILURE;
    }

    Maybe<nsRootedJSValueArray> tempStorage;

    
    
    
    
    
    rv = ConvertSupportsTojsvals(aargv, target, &argc, &argv, tempStorage);
    NS_ENSURE_SUCCESS(rv, rv);

    ++mExecuteDepth;
    bool ok = ::JS_CallFunctionValue(mContext, target,
                                       funval, argc, argv, &rval);
    --mExecuteDepth;

    if (!ok) {
      
      rval = JSVAL_VOID;

      
      rv = NS_ERROR_FAILURE;
    } else if (rval == JSVAL_NULL) {
      *arv = nsnull;
    } else if (!JS_WrapValue(mContext, &rval)) {
      rv = NS_ERROR_FAILURE;
    } else {
      rv = nsContentUtils::XPConnect()->JSToVariant(mContext, rval, arv);
    }

    
    
    
    if (NS_FAILED(rv))
      ReportPendingException();

    sSecurityManager->PopContextPrincipal(mContext);
  }

  pusher.Pop();

  
  ScriptEvaluated(true);

  return rv;
}

nsresult
nsJSContext::BindCompiledEventHandler(nsISupports* aTarget, JSObject* aScope,
                                      JSObject* aHandler,
                                      nsScriptObjectHolder<JSObject>& aBoundHandler)
{
  NS_ENSURE_ARG(aHandler);
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);
  NS_PRECONDITION(!aBoundHandler, "Shouldn't already have a bound handler!");

  JSAutoRequest ar(mContext);

  
  JSObject *target = nsnull;
  nsresult rv = JSObjectFromInterface(aTarget, aScope, &target);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
  {
    JSAutoEnterCompartment ac;
    if (!ac.enter(mContext, aHandler)) {
      return NS_ERROR_FAILURE;
    }

    NS_ASSERTION(JS_TypeOfValue(mContext,
                                OBJECT_TO_JSVAL(aHandler)) == JSTYPE_FUNCTION,
                 "Event handler object not a function");
  }
#endif

  JSAutoEnterCompartment ac;
  if (!ac.enter(mContext, target)) {
    return NS_ERROR_FAILURE;
  }

  JSObject* funobj;
  
  if (aHandler) {
    funobj = JS_CloneFunctionObject(mContext, aHandler, target);
    if (!funobj) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  } else {
    funobj = NULL;
  }

  aBoundHandler.set(funobj);

  return rv;
}


nsresult
nsJSContext::Serialize(nsIObjectOutputStream* aStream, JSScript* aScriptObject)
{
  if (!aScriptObject)
    return NS_ERROR_FAILURE;

  return nsContentUtils::XPConnect()->WriteScript(aStream, mContext, aScriptObject);
}

nsresult
nsJSContext::Deserialize(nsIObjectInputStream* aStream,
                         nsScriptObjectHolder<JSScript>& aResult)
{
  NS_TIME_FUNCTION_MIN(1.0);
  
  JSScript *script;
  nsresult rv = nsContentUtils::XPConnect()->ReadScript(aStream, mContext, &script);
  if (NS_FAILED(rv)) return rv;
    
  return aResult.set(script);
}

nsIScriptGlobalObject *
nsJSContext::GetGlobalObject()
{
  JSObject *global = ::JS_GetGlobalObject(mContext);

  if (!global) {
    return nsnull;
  }

  if (mGlobalObjectRef)
    return mGlobalObjectRef;

#ifdef DEBUG
  {
    JSObject *inner = JS_ObjectToInnerObject(mContext, global);

    
    
    NS_ASSERTION(inner == global, "Shouldn't be able to innerize here");
  }
#endif

  JSClass *c = JS_GetClass(global);

  if (!c || ((~c->flags) & (JSCLASS_HAS_PRIVATE |
                            JSCLASS_PRIVATE_IS_NSISUPPORTS))) {
    return nsnull;
  }

  nsISupports *priv = (nsISupports *)js::GetObjectPrivate(global);

  nsCOMPtr<nsIXPConnectWrappedNative> wrapped_native =
    do_QueryInterface(priv);

  nsCOMPtr<nsIScriptGlobalObject> sgo;
  if (wrapped_native) {
    
    

    sgo = do_QueryWrappedNative(wrapped_native);
  } else {
    sgo = do_QueryInterface(priv);
  }

  
  
  return sgo;
}

JSObject*
nsJSContext::GetNativeGlobal()
{
    return JS_GetGlobalObject(mContext);
}

nsresult
nsJSContext::CreateNativeGlobalForInner(
                                nsIScriptGlobalObject *aNewInner,
                                bool aIsChrome,
                                nsIPrincipal *aPrincipal,
                                JSObject** aNativeGlobal, nsISupports **aHolder)
{
  nsIXPConnect *xpc = nsContentUtils::XPConnect();
  PRUint32 flags = aIsChrome? nsIXPConnect::FLAG_SYSTEM_GLOBAL_OBJECT : 0;

  nsCOMPtr<nsIPrincipal> systemPrincipal;
  if (aIsChrome) {
    nsIScriptSecurityManager *ssm = nsContentUtils::GetSecurityManager();
    ssm->GetSystemPrincipal(getter_AddRefs(systemPrincipal));
  }

  nsRefPtr<nsIXPConnectJSObjectHolder> jsholder;
  nsresult rv = xpc->
          InitClassesWithNewWrappedGlobal(mContext, aNewInner,
                                          aIsChrome ? systemPrincipal.get() : aPrincipal,
                                          flags, getter_AddRefs(jsholder));
  if (NS_FAILED(rv)) {
    return rv;
  }
  jsholder->GetJSObject(aNativeGlobal);
  jsholder.forget(aHolder);
  return NS_OK;
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

  ::JS_SetErrorReporter(mContext, NS_ScriptErrorReporter);

  JSOptionChangedCallback(js_options_dot_str, this);

  return NS_OK;
}

nsresult
nsJSContext::InitOuterWindow()
{
  JSObject *global = JS_ObjectToInnerObject(mContext, JS_GetGlobalObject(mContext));

  nsresult rv = InitClasses(global); 
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsJSContext::InitializeExternalClasses()
{
  nsScriptNameSpaceManager *nameSpaceManager = nsJSRuntime::GetNameSpaceManager();
  NS_ENSURE_TRUE(nameSpaceManager, NS_ERROR_NOT_INITIALIZED);

  return nameSpaceManager->InitForContext(this);
}

nsresult
nsJSContext::SetProperty(JSObject* aTarget, const char* aPropName, nsISupports* aArgs)
{
  PRUint32  argc;
  jsval    *argv = nsnull;

  JSAutoRequest ar(mContext);

  Maybe<nsRootedJSValueArray> tempStorage;

  nsresult rv =
    ConvertSupportsTojsvals(aArgs, GetNativeGlobal(), &argc, &argv, tempStorage);
  NS_ENSURE_SUCCESS(rv, rv);

  jsval vargs;

  

  
  
  if (strcmp(aPropName, "dialogArguments") == 0 && argc <= 1) {
    vargs = argc ? argv[0] : JSVAL_VOID;
  } else {
    for (PRUint32 i = 0; i < argc; ++i) {
      if (!JS_WrapValue(mContext, &argv[i])) {
        return NS_ERROR_FAILURE;
      }
    }

    JSObject *args = ::JS_NewArrayObject(mContext, argc, argv);
    vargs = OBJECT_TO_JSVAL(args);
  }

  
  
  return JS_DefineProperty(mContext, aTarget, aPropName, vargs, NULL, NULL, 0)
    ? NS_OK
    : NS_ERROR_FAILURE;
}

nsresult
nsJSContext::ConvertSupportsTojsvals(nsISupports *aArgs,
                                     JSObject *aScope,
                                     PRUint32 *aArgc,
                                     jsval **aArgv,
                                     Maybe<nsRootedJSValueArray> &aTempStorage)
{
  nsresult rv = NS_OK;

  
  nsCOMPtr<nsIJSArgArray> fastArray = do_QueryInterface(aArgs);
  if (fastArray != nsnull)
    return fastArray->GetArgs(aArgc, reinterpret_cast<void **>(aArgv));

  
  
  

  *aArgv = nsnull;
  *aArgc = 0;

  nsIXPConnect *xpc = nsContentUtils::XPConnect();
  NS_ENSURE_TRUE(xpc, NS_ERROR_UNEXPECTED);

  if (!aArgs)
    return NS_OK;
  PRUint32 argCount;
  
  
  nsCOMPtr<nsIArray> argsArray(do_QueryInterface(aArgs));

  if (argsArray) {
    rv = argsArray->GetLength(&argCount);
    NS_ENSURE_SUCCESS(rv, rv);
    if (argCount == 0)
      return NS_OK;
  } else {
    argCount = 1; 
  }

  
  aTempStorage.construct(mContext);
  bool ok = aTempStorage.ref().SetCapacity(mContext, argCount);
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
  jsval *argv = aTempStorage.ref().Elements();

  if (argsArray) {
    for (PRUint32 argCtr = 0; argCtr < argCount && NS_SUCCEEDED(rv); argCtr++) {
      nsCOMPtr<nsISupports> arg;
      jsval *thisval = argv + argCtr;
      argsArray->QueryElementAt(argCtr, NS_GET_IID(nsISupports),
                                getter_AddRefs(arg));
      if (!arg) {
        *thisval = JSVAL_NULL;
        continue;
      }
      nsCOMPtr<nsIVariant> variant(do_QueryInterface(arg));
      if (variant != nsnull) {
        rv = xpc->VariantToJS(mContext, aScope, variant, thisval);
      } else {
        
        
        
        rv = AddSupportsPrimitiveTojsvals(arg, thisval);
        if (rv == NS_ERROR_NO_INTERFACE) {
          
          
#ifdef NS_DEBUG
          
          
          nsCOMPtr<nsISupportsPrimitive> prim(do_QueryInterface(arg));
          NS_ASSERTION(prim == nsnull,
                       "Don't pass nsISupportsPrimitives - use nsIVariant!");
#endif
          nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
          jsval v;
          rv = nsContentUtils::WrapNative(mContext, aScope, arg, &v,
                                          getter_AddRefs(wrapper));
          if (NS_SUCCEEDED(rv)) {
            *thisval = v;
          }
        }
      }
    }
  } else {
    nsCOMPtr<nsIVariant> variant = do_QueryInterface(aArgs);
    if (variant) {
      rv = xpc->VariantToJS(mContext, aScope, variant, argv);
    } else {
      NS_ERROR("Not an array, not an interface?");
      rv = NS_ERROR_UNEXPECTED;
    }
  }
  if (NS_FAILED(rv))
    return rv;
  *aArgv = argv;
  *aArgc = argCount;
  return NS_OK;
}


nsresult
nsJSContext::AddSupportsPrimitiveTojsvals(nsISupports *aArg, jsval *aArgv)
{
  NS_PRECONDITION(aArg, "Empty arg");

  nsCOMPtr<nsISupportsPrimitive> argPrimitive(do_QueryInterface(aArg));
  if (!argPrimitive)
    return NS_ERROR_NO_INTERFACE;

  JSContext *cx = mContext;
  PRUint16 type;
  argPrimitive->GetType(&type);

  switch(type) {
    case nsISupportsPrimitive::TYPE_CSTRING : {
      nsCOMPtr<nsISupportsCString> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      nsCAutoString data;

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
        ::JS_NewUCStringCopyN(cx,
                              reinterpret_cast<const jschar *>(data.get()),
                              data.Length());
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

      PRUint8 data;

      p->GetData(&data);

      *aArgv = INT_TO_JSVAL(data);

      break;
    }
    case nsISupportsPrimitive::TYPE_PRUINT16 : {
      nsCOMPtr<nsISupportsPRUint16> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      PRUint16 data;

      p->GetData(&data);

      *aArgv = INT_TO_JSVAL(data);

      break;
    }
    case nsISupportsPrimitive::TYPE_PRUINT32 : {
      nsCOMPtr<nsISupportsPRUint32> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      PRUint32 data;

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

      PRInt16 data;

      p->GetData(&data);

      *aArgv = INT_TO_JSVAL(data);

      break;
    }
    case nsISupportsPrimitive::TYPE_PRINT32 : {
      nsCOMPtr<nsISupportsPRInt32> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      PRInt32 data;

      p->GetData(&data);

      *aArgv = INT_TO_JSVAL(data);

      break;
    }
    case nsISupportsPrimitive::TYPE_FLOAT : {
      nsCOMPtr<nsISupportsFloat> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      float data;

      p->GetData(&data);

      JSBool ok = ::JS_NewNumberValue(cx, data, aArgv);
      NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);

      break;
    }
    case nsISupportsPrimitive::TYPE_DOUBLE : {
      nsCOMPtr<nsISupportsDouble> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      double data;

      p->GetData(&data);

      JSBool ok = ::JS_NewNumberValue(cx, data, aArgv);
      NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);

      break;
    }
    case nsISupportsPrimitive::TYPE_INTERFACE_POINTER : {
      nsCOMPtr<nsISupportsInterfacePointer> p(do_QueryInterface(argPrimitive));
      NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);

      nsCOMPtr<nsISupports> data;
      nsIID *iid = nsnull;

      p->GetData(getter_AddRefs(data));
      p->GetDataIID(&iid);
      NS_ENSURE_TRUE(iid, NS_ERROR_UNEXPECTED);

      AutoFree iidGuard(iid); 

      nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
      jsval v;
      nsresult rv = nsContentUtils::WrapNative(cx, ::JS_GetGlobalObject(cx),
                                               data, iid, &v,
                                               getter_AddRefs(wrapper));
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

#ifdef NS_TRACE_MALLOC

#include <errno.h>              
#include <fcntl.h>
#ifdef XP_UNIX
#include <unistd.h>
#endif
#ifdef XP_WIN32
#include <io.h>
#endif
#include "nsTraceMalloc.h"

static JSBool
CheckUniversalXPConnectForTraceMalloc(JSContext *cx)
{
    bool hasCap = false;
    nsresult rv = nsContentUtils::GetSecurityManager()->
                    IsCapabilityEnabled("UniversalXPConnect", &hasCap);
    if (NS_SUCCEEDED(rv) && hasCap)
        return JS_TRUE;
    JS_ReportError(cx, "trace-malloc functions require UniversalXPConnect");
    return JS_FALSE;
}

static JSBool
TraceMallocDisable(JSContext *cx, unsigned argc, jsval *vp)
{
    if (!CheckUniversalXPConnectForTraceMalloc(cx))
        return JS_FALSE;

    NS_TraceMallocDisable();
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSBool
TraceMallocEnable(JSContext *cx, unsigned argc, jsval *vp)
{
    if (!CheckUniversalXPConnectForTraceMalloc(cx))
        return JS_FALSE;

    NS_TraceMallocEnable();
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSBool
TraceMallocOpenLogFile(JSContext *cx, unsigned argc, jsval *vp)
{
    int fd;
    JSString *str;

    if (!CheckUniversalXPConnectForTraceMalloc(cx))
        return JS_FALSE;

    if (argc == 0) {
        fd = -1;
    } else {
        str = JS_ValueToString(cx, JS_ARGV(cx, vp)[0]);
        if (!str)
            return JS_FALSE;
        JSAutoByteString filename(cx, str);
        if (!filename)
            return JS_FALSE;
        fd = open(filename.ptr(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) {
            JS_ReportError(cx, "can't open %s: %s", filename.ptr(), strerror(errno));
            return JS_FALSE;
        }
    }
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(fd));
    return JS_TRUE;
}

static JSBool
TraceMallocChangeLogFD(JSContext *cx, unsigned argc, jsval *vp)
{
    if (!CheckUniversalXPConnectForTraceMalloc(cx))
        return JS_FALSE;

    int32_t fd, oldfd;
    if (argc == 0) {
        oldfd = -1;
    } else {
        if (!JS_ValueToECMAInt32(cx, JS_ARGV(cx, vp)[0], &fd))
            return JS_FALSE;
        oldfd = NS_TraceMallocChangeLogFD(fd);
        if (oldfd == -2) {
            JS_ReportOutOfMemory(cx);
            return JS_FALSE;
        }
    }
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(oldfd));
    return JS_TRUE;
}

static JSBool
TraceMallocCloseLogFD(JSContext *cx, unsigned argc, jsval *vp)
{
    if (!CheckUniversalXPConnectForTraceMalloc(cx))
        return JS_FALSE;

    int32_t fd;
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    if (argc == 0)
        return JS_TRUE;
    if (!JS_ValueToECMAInt32(cx, JS_ARGV(cx, vp)[0], &fd))
        return JS_FALSE;
    NS_TraceMallocCloseLogFD((int) fd);
    return JS_TRUE;
}

static JSBool
TraceMallocLogTimestamp(JSContext *cx, unsigned argc, jsval *vp)
{
    if (!CheckUniversalXPConnectForTraceMalloc(cx))
        return JS_FALSE;

    JSString *str = JS_ValueToString(cx, argc ? JS_ARGV(cx, vp)[0] : JSVAL_VOID);
    if (!str)
        return JS_FALSE;
    JSAutoByteString caption(cx, str);
    if (!caption)
        return JS_FALSE;
    NS_TraceMallocLogTimestamp(caption.ptr());
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSBool
TraceMallocDumpAllocations(JSContext *cx, unsigned argc, jsval *vp)
{
    if (!CheckUniversalXPConnectForTraceMalloc(cx))
        return JS_FALSE;

    JSString *str = JS_ValueToString(cx, argc ? JS_ARGV(cx, vp)[0] : JSVAL_VOID);
    if (!str)
        return JS_FALSE;
    JSAutoByteString pathname(cx, str);
    if (!pathname)
        return JS_FALSE;
    if (NS_TraceMallocDumpAllocations(pathname.ptr()) < 0) {
        JS_ReportError(cx, "can't dump to %s: %s", pathname.ptr(), strerror(errno));
        return JS_FALSE;
    }
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSFunctionSpec TraceMallocFunctions[] = {
    {"TraceMallocDisable",         TraceMallocDisable,         0, 0},
    {"TraceMallocEnable",          TraceMallocEnable,          0, 0},
    {"TraceMallocOpenLogFile",     TraceMallocOpenLogFile,     1, 0},
    {"TraceMallocChangeLogFD",     TraceMallocChangeLogFD,     1, 0},
    {"TraceMallocCloseLogFD",      TraceMallocCloseLogFD,      1, 0},
    {"TraceMallocLogTimestamp",    TraceMallocLogTimestamp,    1, 0},
    {"TraceMallocDumpAllocations", TraceMallocDumpAllocations, 1, 0},
    {nsnull,                       nsnull,                     0, 0}
};

#endif 

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

static JSBool
JProfStartProfilingJS(JSContext *cx, unsigned argc, jsval *vp)
{
  NS_JProfStartProfiling();
  return JS_TRUE;
}

void NS_JProfStartProfiling()
{
    
    
    
    struct sigaction action;

    
    sigaction(SIGALRM, nsnull, &action);
    
    if (IsJProfAction(&action)) {
        
        raise(SIGALRM);
        return;
    }

    sigaction(SIGPROF, nsnull, &action);
    
    if (IsJProfAction(&action)) {
        
        raise(SIGPROF);
        return;
    }

    sigaction(SIGPOLL, nsnull, &action);
    
    if (IsJProfAction(&action)) {
        
        raise(SIGPOLL);
        return;
    }

    printf("Could not start jprof-profiling since JPROF_FLAGS was not set.\n");
}

static JSBool
JProfStopProfilingJS(JSContext *cx, unsigned argc, jsval *vp)
{
  NS_JProfStopProfiling();
  return JS_TRUE;
}

void
NS_JProfStopProfiling()
{
    raise(SIGUSR1);
    
}

static JSBool
JProfClearCircularJS(JSContext *cx, unsigned argc, jsval *vp)
{
  NS_JProfClearCircular();
  return JS_TRUE;
}

void
NS_JProfClearCircular()
{
    raise(SIGUSR2);
    
}

static JSBool
JProfSaveCircularJS(JSContext *cx, unsigned argc, jsval *vp)
{
  
  NS_JProfStopProfiling();
  NS_JProfStartProfiling();
  return JS_TRUE;
}

static JSFunctionSpec JProfFunctions[] = {
    {"JProfStartProfiling",        JProfStartProfilingJS,      0, 0},
    {"JProfStopProfiling",         JProfStopProfilingJS,       0, 0},
    {"JProfClearCircular",         JProfClearCircularJS,       0, 0},
    {"JProfSaveCircular",          JProfSaveCircularJS,        0, 0},
    {nsnull,                       nsnull,                     0, 0}
};

#endif 

#ifdef MOZ_DMD




static JSBool
DMDCheckJS(JSContext *cx, unsigned argc, jsval *vp)
{
  mozilla::DMDCheckAndDump();
  return JS_TRUE;
}

static JSFunctionSpec DMDFunctions[] = {
    {"DMD",                        DMDCheckJS,                 0, 0},
    {nsnull,                       nsnull,                     0, 0}
};

#endif 

nsresult
nsJSContext::InitClasses(JSObject* aGlobalObj)
{
  nsresult rv = InitializeExternalClasses();
  NS_ENSURE_SUCCESS(rv, rv);

  JSAutoRequest ar(mContext);

  ::JS_SetOptions(mContext, mDefaultJSOptions);

  
  ::JS_DefineProfilingFunctions(mContext, aGlobalObj);

#ifdef NS_TRACE_MALLOC
  
  ::JS_DefineFunctions(mContext, aGlobalObj, TraceMallocFunctions);
#endif

#ifdef MOZ_JPROF
  
  ::JS_DefineFunctions(mContext, aGlobalObj, JProfFunctions);
#endif

#ifdef MOZ_DMD
  
  ::JS_DefineFunctions(mContext, aGlobalObj, DMDFunctions);
#endif

  return rv;
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

void
nsJSContext::ScriptEvaluated(bool aTerminated)
{
  if (aTerminated && mTerminations) {
    
    
    nsJSContext::TerminationFuncClosure* start = mTerminations;
    mTerminations = nsnull;

    for (nsJSContext::TerminationFuncClosure* cur = start;
         cur;
         cur = cur->mNext) {
      (*(cur->mTerminationFunc))(cur->mTerminationFuncArg);
    }
    delete start;
  }

  JS_MaybeGC(mContext);

  if (aTerminated) {
    mOperationCallbackTime = 0;
    mModalStateTime = 0;
  }
}

void
nsJSContext::SetTerminationFunction(nsScriptTerminationFunc aFunc,
                                    nsIDOMWindow* aRef)
{
  NS_PRECONDITION(GetExecutingScript(), "should be executing script");

  nsJSContext::TerminationFuncClosure* newClosure =
    new nsJSContext::TerminationFuncClosure(aFunc, aRef, mTerminations);
  mTerminations = newClosure;
}

bool
nsJSContext::GetScriptsEnabled()
{
  return mScriptsEnabled;
}

void
nsJSContext::SetScriptsEnabled(bool aEnabled, bool aFireTimeouts)
{
  
  
  mScriptsEnabled = aEnabled;

  nsIScriptGlobalObject *global = GetGlobalObject();

  if (global) {
    global->SetScriptsEnabled(aEnabled, aFireTimeouts);
  }
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

bool
nsJSContext::GetExecutingScript()
{
  return JS_IsRunning(mContext) || mExecuteDepth > 0;
}

void
nsJSContext::SetGCOnDestruction(bool aGCOnDestruction)
{
  mGCOnDestruction = aGCOnDestruction;
}

NS_IMETHODIMP
nsJSContext::ScriptExecuted()
{
  ScriptEvaluated(!::JS_IsRunning(mContext));

  return NS_OK;
}


void
nsJSContext::GarbageCollectNow(js::gcreason::Reason reason, PRUint32 gckind)
{
  NS_TIME_FUNCTION_MIN(1.0);
  SAMPLE_LABEL("GC", "GarbageCollectNow");

  KillGCTimer();
  KillShrinkGCBuffersTimer();

  
  
  
  
  
  
  sPendingLoadCount = 0;
  sLoadingInProgress = false;

  if (nsContentUtils::XPConnect()) {
    nsContentUtils::XPConnect()->GarbageCollect(reason, gckind);
  }
}


void
nsJSContext::ShrinkGCBuffersNow()
{
  NS_TIME_FUNCTION_MIN(1.0);
  SAMPLE_LABEL("GC", "ShrinkGCBuffersNow");

  KillShrinkGCBuffersTimer();

  JS_ShrinkGCBuffers(nsJSRuntime::sRuntime);
}


void
nsJSContext::CycleCollectNow(nsICycleCollectorListener *aListener,
                             PRInt32 aExtraForgetSkippableCalls)
{
  if (!NS_IsMainThread()) {
    return;
  }

  if (sCCLockedOut) {
    
    nsJSContext::GarbageCollectNow(js::gcreason::CC_FORCED, nsGCNormal);
  }

  SAMPLE_LABEL("GC", "CycleCollectNow");
  NS_TIME_FUNCTION_MIN(1.0);

  KillCCTimer();

  PRTime start = PR_Now();

  PRUint32 suspected = nsCycleCollector_suspectedCount();

  for (PRInt32 i = 0; i < aExtraForgetSkippableCalls; ++i) {
    nsCycleCollector_forgetSkippable();
  }

  
  if (!sCleanupSinceLastGC && aExtraForgetSkippableCalls >= 0) {
    nsCycleCollector_forgetSkippable();
  }
  nsCycleCollectorResults ccResults;
  nsCycleCollector_collect(&ccResults, aListener);
  sCCollectedWaitingForGC += ccResults.mFreedRefCounted + ccResults.mFreedGCed;

  
  
  if (sCCollectedWaitingForGC > 250) {
    PokeGC(js::gcreason::CC_WAITING);
  }

  PRTime now = PR_Now();

  if (sLastCCEndTime) {
    PRUint32 timeBetween = (PRUint32)(start - sLastCCEndTime) / PR_USEC_PER_SEC;
    Telemetry::Accumulate(Telemetry::CYCLE_COLLECTOR_TIME_BETWEEN, timeBetween);
  }
  sLastCCEndTime = now;

  Telemetry::Accumulate(Telemetry::FORGET_SKIPPABLE_MAX,
                        sMaxForgetSkippableTime / PR_USEC_PER_MSEC);

  if (sPostGCEventsToConsole) {
    PRTime delta = 0;
    if (sFirstCollectionTime) {
      delta = now - sFirstCollectionTime;
    } else {
      sFirstCollectionTime = now;
    }

    nsString gcmsg;
    if (ccResults.mForcedGC) {
      gcmsg.AssignLiteral(", forced a GC");
    }

    NS_NAMED_MULTILINE_LITERAL_STRING(kFmt,
      NS_LL("CC(T+%.1f) duration: %llums, suspected: %lu, visited: %lu RCed and %lu GCed, collected: %lu RCed and %lu GCed (%lu waiting for GC)%s\n")
      NS_LL("ForgetSkippable %lu times before CC, min: %lu ms, max: %lu ms, avg: %lu ms, total: %lu ms, removed: %lu"));
    nsString msg;
    PRUint32 cleanups = sForgetSkippableBeforeCC ? sForgetSkippableBeforeCC : 1;
    sMinForgetSkippableTime = (sMinForgetSkippableTime == PR_UINT32_MAX)
      ? 0 : sMinForgetSkippableTime;
    msg.Adopt(nsTextFormatter::smprintf(kFmt.get(), double(delta) / PR_USEC_PER_SEC,
                                        (now - start) / PR_USEC_PER_MSEC, suspected,
                                        ccResults.mVisitedRefCounted, ccResults.mVisitedGCed,
                                        ccResults.mFreedRefCounted, ccResults.mFreedGCed,
                                        sCCollectedWaitingForGC, gcmsg.get(),
                                        sForgetSkippableBeforeCC,
                                        sMinForgetSkippableTime / PR_USEC_PER_MSEC,
                                        sMaxForgetSkippableTime / PR_USEC_PER_MSEC,
                                        (sTotalForgetSkippableTime / cleanups) /
                                          PR_USEC_PER_MSEC,
                                        sTotalForgetSkippableTime / PR_USEC_PER_MSEC,
                                        sRemovedPurples));
    nsCOMPtr<nsIConsoleService> cs =
      do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    if (cs) {
      cs->LogStringMessage(msg.get());
    }

    NS_NAMED_MULTILINE_LITERAL_STRING(kJSONFmt,
       NS_LL("{ \"timestamp\": %llu, ")
         NS_LL("\"duration\": %llu, ")
         NS_LL("\"suspected\": %lu, ")
         NS_LL("\"visited\": { ")
             NS_LL("\"RCed\": %lu, ")
             NS_LL("\"GCed\": %lu }, ")
         NS_LL("\"collected\": { ")
             NS_LL("\"RCed\": %lu, ")
             NS_LL("\"GCed\": %lu }, ")
         NS_LL("\"waiting_for_gc\": %lu, ")
         NS_LL("\"forced_gc\": %d, ")
         NS_LL("\"forget_skippable\": { ")
             NS_LL("\"times_before_cc\": %lu, ")
             NS_LL("\"min\": %lu, ")
             NS_LL("\"max\": %lu, ")
             NS_LL("\"avg\": %lu, ")
             NS_LL("\"total\": %lu, ")
             NS_LL("\"removed\": %lu } ")
       NS_LL("}"));
    nsString json;
    json.Adopt(nsTextFormatter::smprintf(kJSONFmt.get(),
                                         now, (now - start) / PR_USEC_PER_MSEC, suspected,
                                         ccResults.mVisitedRefCounted, ccResults.mVisitedGCed,
                                         ccResults.mFreedRefCounted, ccResults.mFreedGCed,
                                         sCCollectedWaitingForGC,
                                         ccResults.mForcedGC,
                                         sForgetSkippableBeforeCC,
                                         sMinForgetSkippableTime / PR_USEC_PER_MSEC,
                                         sMaxForgetSkippableTime / PR_USEC_PER_MSEC,
                                         (sTotalForgetSkippableTime / cleanups) /
                                           PR_USEC_PER_MSEC,
                                         sTotalForgetSkippableTime / PR_USEC_PER_MSEC,
                                         sRemovedPurples));
    nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
    if (observerService) {
      observerService->NotifyObservers(nsnull, "cycle-collection-statistics", json.get());
    }
  }
  sMinForgetSkippableTime = PR_UINT32_MAX;
  sMaxForgetSkippableTime = 0;
  sTotalForgetSkippableTime = 0;
  sRemovedPurples = 0;
  sForgetSkippableBeforeCC = 0;
  sCleanupSinceLastGC = true;
  sNeedsFullCC = false;
}


void
GCTimerFired(nsITimer *aTimer, void *aClosure)
{
  NS_RELEASE(sGCTimer);

  uintptr_t reason = reinterpret_cast<uintptr_t>(aClosure);
  nsJSContext::GarbageCollectNow(static_cast<js::gcreason::Reason>(reason), nsGCIncremental);
}

void
ShrinkGCBuffersTimerFired(nsITimer *aTimer, void *aClosure)
{
  NS_RELEASE(sShrinkGCBuffersTimer);

  nsJSContext::ShrinkGCBuffersNow();
}

static bool
ShouldTriggerCC(PRUint32 aSuspected)
{
  return sNeedsFullCC ||
         aSuspected > NS_CC_PURPLE_LIMIT ||
         sLastCCEndTime + NS_CC_FORCED < PR_Now();
}

static void
TimerFireForgetSkippable(PRUint32 aSuspected, bool aRemoveChildless)
{
  PRTime startTime = PR_Now();
  nsCycleCollector_forgetSkippable(aRemoveChildless);
  sPreviousSuspectedCount = nsCycleCollector_suspectedCount();
  sCleanupSinceLastGC = true;
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

static void
CCTimerFired(nsITimer *aTimer, void *aClosure)
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

    
    nsJSContext::GarbageCollectNow(js::gcreason::CC_FORCED, nsGCNormal);
  }

  ++sCCTimerFireCount;

  
  
  
  const PRUint32 numEarlyTimerFires = NS_CC_DELAY / NS_CC_SKIPPABLE_DELAY - 2;
  bool isLateTimerFire = sCCTimerFireCount > numEarlyTimerFires;
  PRUint32 suspected = nsCycleCollector_suspectedCount();
  if (isLateTimerFire && ShouldTriggerCC(suspected)) {
    if (sCCTimerFireCount == numEarlyTimerFires + 1) {
      TimerFireForgetSkippable(suspected, true);
      if (ShouldTriggerCC(nsCycleCollector_suspectedCount())) {
        
        
        return;
      }
    } else {
      
      
      nsJSContext::CycleCollectNow();
    }
  } else if ((sPreviousSuspectedCount + 100) <= suspected) {
    
    TimerFireForgetSkippable(suspected, false);
  }

  if (isLateTimerFire) {
    
    
    sPreviousSuspectedCount = 0;
    nsJSContext::KillCCTimer();
  }
}


bool
nsJSContext::CleanupSinceLastGC()
{
  return sCleanupSinceLastGC;
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
  PokeGC(js::gcreason::LOAD_END);
}


void
nsJSContext::PokeGC(js::gcreason::Reason aReason, int aDelay)
{
  if (sGCTimer) {
    
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
  if (sShrinkGCBuffersTimer) {
    return;
  }

  CallCreateInstance("@mozilla.org/timer;1", &sShrinkGCBuffersTimer);

  if (!sShrinkGCBuffersTimer) {
    
    return;
  }

  sShrinkGCBuffersTimer->InitWithFuncCallback(ShrinkGCBuffersTimerFired, nsnull,
                                              NS_SHRINK_GC_BUFFERS_DELAY,
                                              nsITimer::TYPE_ONE_SHOT);
}


void
nsJSContext::MaybePokeCC()
{
  if (sCCTimer || sDidShutdown) {
    return;
  }

  if (sNeedsFullCC ||
      nsCycleCollector_suspectedCount() > 100 ||
      sLastCCEndTime + NS_CC_FORCED < PR_Now()) {
    sCCTimerFireCount = 0;
    CallCreateInstance("@mozilla.org/timer;1", &sCCTimer);
    if (!sCCTimer) {
      return;
    }
    sCCTimer->InitWithFuncCallback(CCTimerFired, nsnull,
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
nsJSContext::KillShrinkGCBuffersTimer()
{
  if (sShrinkGCBuffersTimer) {
    sShrinkGCBuffersTimer->Cancel();

    NS_RELEASE(sShrinkGCBuffersTimer);
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
nsJSContext::GC(js::gcreason::Reason aReason)
{
  PokeGC(aReason);
}

class NotifyGCEndRunnable : public nsRunnable
{
  nsString mMessage;

public:
  NotifyGCEndRunnable(const nsString& aMessage) : mMessage(aMessage) {}

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

  const jschar oomMsg[3] = { '{', '}', 0 };
  const jschar *toSend = mMessage.get() ? mMessage.get() : oomMsg;
  observerService->NotifyObservers(nsnull, "garbage-collection-statistics", toSend);

  return NS_OK;
}

static void
DOMGCSliceCallback(JSRuntime *aRt, js::GCProgress aProgress, const js::GCDescription &aDesc)
{
  NS_ASSERTION(NS_IsMainThread(), "GCs must run on the main thread");

  if (aProgress == js::GC_CYCLE_END && sPostGCEventsToConsole) {
    PRTime now = PR_Now();
    PRTime delta = 0;
    if (sFirstCollectionTime) {
      delta = now - sFirstCollectionTime;
    } else {
      sFirstCollectionTime = now;
    }

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

    nsString json;
    json.Adopt(aDesc.formatJSON(aRt, now));
    nsRefPtr<NotifyGCEndRunnable> notify = new NotifyGCEndRunnable(json);
    NS_DispatchToMainThread(notify);
  }

  
  if (aProgress == js::GC_CYCLE_BEGIN) {
    sCCLockedOut = true;
    nsJSContext::KillShrinkGCBuffersTimer();
  } else if (aProgress == js::GC_CYCLE_END) {
    sCCLockedOut = false;
  }

  
  if (aProgress == js::GC_SLICE_END) {
    nsJSContext::KillGCTimer();
    nsJSContext::PokeGC(js::gcreason::INTER_SLICE_GC, NS_INTERSLICE_GC_DELAY);
  }

  if (aProgress == js::GC_CYCLE_END) {
    
    nsJSContext::KillGCTimer();

    sCCollectedWaitingForGC = 0;
    sCleanupSinceLastGC = false;

    if (aDesc.isCompartment) {
      
      
      
      
      
      nsJSContext::PokeGC(js::gcreason::POST_COMPARTMENT);
    }

    sNeedsFullCC = true;
    nsJSContext::MaybePokeCC();

    if (!aDesc.isCompartment) {
      
      
      nsJSContext::PokeShrinkGCBuffers();
    }
  }

  if (sPrevGCSliceCallback)
    (*sPrevGCSliceCallback)(aRt, aProgress, aDesc);
}



nsresult
nsJSContext::HoldScriptObject(void* aScriptObject)
{
    NS_ASSERTION(sIsInitialized, "runtime not initialized");
    if (! nsJSRuntime::sRuntime) {
        NS_NOTREACHED("couldn't add GC root - no runtime");
        return NS_ERROR_FAILURE;
    }

    ::JS_LockGCThingRT(nsJSRuntime::sRuntime, aScriptObject);
    return NS_OK;
}

nsresult
nsJSContext::DropScriptObject(void* aScriptObject)
{
  NS_ASSERTION(sIsInitialized, "runtime not initialized");
  if (! nsJSRuntime::sRuntime) {
    NS_NOTREACHED("couldn't remove GC root");
    return NS_ERROR_FAILURE;
  }

  ::JS_UnlockGCThingRT(nsJSRuntime::sRuntime, aScriptObject);
  return NS_OK;
}

void
nsJSContext::ReportPendingException()
{
  
  
  if (mIsInitialized && ::JS_IsExceptionPending(mContext)) {
    bool saved = ::JS_SaveFrameChain(mContext);
    ::JS_ReportPendingException(mContext);
    if (saved)
        ::JS_RestoreFrameChain(mContext);
  }
}






NS_INTERFACE_MAP_BEGIN(nsJSRuntime)
  NS_INTERFACE_MAP_ENTRY(nsIScriptRuntime)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsJSRuntime)
NS_IMPL_RELEASE(nsJSRuntime)

already_AddRefed<nsIScriptContext>
nsJSRuntime::CreateContext()
{
  nsCOMPtr<nsIScriptContext> scriptContext = new nsJSContext(sRuntime);
  return scriptContext.forget();
}

nsresult
nsJSRuntime::ParseVersion(const nsString &aVersionStr, PRUint32 *flags)
{
    NS_PRECONDITION(flags, "Null flags param?");
    JSVersion jsVersion = JSVERSION_UNKNOWN;
    if (aVersionStr.Length() != 3 || aVersionStr[0] != '1' || aVersionStr[1] != '.')
        jsVersion = JSVERSION_UNKNOWN;
    else switch (aVersionStr[2]) {
        case '0': jsVersion = JSVERSION_1_0; break;
        case '1': jsVersion = JSVERSION_1_1; break;
        case '2': jsVersion = JSVERSION_1_2; break;
        case '3': jsVersion = JSVERSION_1_3; break;
        case '4': jsVersion = JSVERSION_1_4; break;
        case '5': jsVersion = JSVERSION_1_5; break;
        case '6': jsVersion = JSVERSION_1_6; break;
        case '7': jsVersion = JSVERSION_1_7; break;
        case '8': jsVersion = JSVERSION_1_8; break;
        default:  jsVersion = JSVERSION_UNKNOWN;
    }
    *flags = (PRUint32)jsVersion;
    return NS_OK;
}


void
nsJSRuntime::Startup()
{
  
  sGCTimer = sCCTimer = nsnull;
  sCCLockedOut = false;
  sCCLockedOutTime = 0;
  sLastCCEndTime = 0;
  sPendingLoadCount = 0;
  sLoadingInProgress = false;
  sCCollectedWaitingForGC = 0;
  sPostGCEventsToConsole = false;
  sNeedsFullCC = false;
  gNameSpaceManager = nsnull;
  sRuntimeService = nsnull;
  sRuntime = nsnull;
  sIsInitialized = false;
  sDidShutdown = false;
  sContextCount = 0;
  sSecurityManager = nsnull;
}

static int
MaxScriptRunTimePrefChangedCallback(const char *aPrefName, void *aClosure)
{
  
  
  bool isChromePref =
    strcmp(aPrefName, "dom.max_chrome_script_run_time") == 0;
  PRInt32 time = Preferences::GetInt(aPrefName, isChromePref ? 20 : 10);

  PRTime t;
  if (time <= 0) {
    
    t = LL_INIT(0x40000000, 0);
  } else {
    t = time * PR_USEC_PER_SEC;
  }

  if (isChromePref) {
    sMaxChromeScriptRunTime = t;
  } else {
    sMaxScriptRunTime = t;
  }

  return 0;
}

static int
ReportAllJSExceptionsPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  bool reportAll = Preferences::GetBool(aPrefName, false);
  nsContentUtils::XPConnect()->SetReportAllJSExceptions(reportAll);
  return 0;
}

static int
SetMemoryHighWaterMarkPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  PRInt32 highwatermark = Preferences::GetInt(aPrefName, 128);

  JS_SetGCParameter(nsJSRuntime::sRuntime, JSGC_MAX_MALLOC_BYTES,
                    highwatermark * 1024L * 1024L);
  return 0;
}

static int
SetMemoryMaxPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  PRInt32 pref = Preferences::GetInt(aPrefName, -1);
  
  PRUint32 max = (pref <= 0 || pref >= 0x1000) ? -1 : (PRUint32)pref * 1024 * 1024;
  JS_SetGCParameter(nsJSRuntime::sRuntime, JSGC_MAX_BYTES, max);
  return 0;
}

static int
SetMemoryGCModePrefChangedCallback(const char* aPrefName, void* aClosure)
{
  PRBool enableCompartmentGC = Preferences::GetBool("javascript.options.mem.gc_per_compartment");
  PRBool enableIncrementalGC = Preferences::GetBool("javascript.options.mem.gc_incremental");
  JSGCMode mode;
  if (enableIncrementalGC) {
    mode = JSGC_MODE_INCREMENTAL;
  } else if (enableCompartmentGC) {
    mode = JSGC_MODE_COMPARTMENT;
  } else {
    mode = JSGC_MODE_GLOBAL;
  }
  JS_SetGCParameter(nsJSRuntime::sRuntime, JSGC_MODE, mode);
  return 0;
}

static int
SetMemoryGCSliceTimePrefChangedCallback(const char* aPrefName, void* aClosure)
{
  PRInt32 pref = Preferences::GetInt(aPrefName, -1);
  
  if (pref > 0 && pref < 100000)
    JS_SetGCParameter(nsJSRuntime::sRuntime, JSGC_SLICE_TIME_BUDGET, pref);
  return 0;
}

JSObject*
NS_DOMReadStructuredClone(JSContext* cx,
                          JSStructuredCloneReader* reader,
                          uint32_t tag,
                          uint32_t data,
                          void* closure)
{
  
  nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_DOM_DATA_CLONE_ERR);
  return nsnull;
}

JSBool
NS_DOMWriteStructuredClone(JSContext* cx,
                           JSStructuredCloneWriter* writer,
                           JSObject* obj,
                           void *closure)
{
  
  nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_DOM_DATA_CLONE_ERR);
  return JS_FALSE;
}

void
NS_DOMStructuredCloneError(JSContext* cx,
                           uint32_t errorid)
{
  
  nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_DOM_DATA_CLONE_ERR);
}


nsresult
nsJSRuntime::Init()
{
  if (sIsInitialized) {
    if (!nsContentUtils::XPConnect())
      return NS_ERROR_NOT_AVAILABLE;

    return NS_OK;
  }

  nsresult rv = CallGetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID,
                               &sSecurityManager);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallGetService(kJSRuntimeServiceContractID, &sRuntimeService);
  
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sRuntimeService->GetRuntime(&sRuntime);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ASSERTION(NS_IsMainThread(), "bad");

  sPrevGCSliceCallback = js::SetGCSliceCallback(sRuntime, DOMGCSliceCallback);

  
  static JSStructuredCloneCallbacks cloneCallbacks = {
    NS_DOMReadStructuredClone,
    NS_DOMWriteStructuredClone,
    NS_DOMStructuredCloneError
  };
  JS_SetStructuredCloneCallbacks(sRuntime, &cloneCallbacks);

  
  Preferences::RegisterCallback(MaxScriptRunTimePrefChangedCallback,
                                "dom.max_script_run_time");
  MaxScriptRunTimePrefChangedCallback("dom.max_script_run_time", nsnull);

  Preferences::RegisterCallback(MaxScriptRunTimePrefChangedCallback,
                                "dom.max_chrome_script_run_time");
  MaxScriptRunTimePrefChangedCallback("dom.max_chrome_script_run_time",
                                      nsnull);

  Preferences::RegisterCallback(ReportAllJSExceptionsPrefChangedCallback,
                                "dom.report_all_js_exceptions");
  ReportAllJSExceptionsPrefChangedCallback("dom.report_all_js_exceptions",
                                           nsnull);

  Preferences::RegisterCallback(SetMemoryHighWaterMarkPrefChangedCallback,
                                "javascript.options.mem.high_water_mark");
  SetMemoryHighWaterMarkPrefChangedCallback("javascript.options.mem.high_water_mark",
                                            nsnull);

  Preferences::RegisterCallback(SetMemoryMaxPrefChangedCallback,
                                "javascript.options.mem.max");
  SetMemoryMaxPrefChangedCallback("javascript.options.mem.max",
                                  nsnull);

  Preferences::RegisterCallback(SetMemoryGCModePrefChangedCallback,
                                "javascript.options.mem.gc_per_compartment");
  SetMemoryGCModePrefChangedCallback("javascript.options.mem.gc_per_compartment",
                                     nsnull);

  Preferences::RegisterCallback(SetMemoryGCModePrefChangedCallback,
                                "javascript.options.mem.gc_incremental");
  SetMemoryGCModePrefChangedCallback("javascript.options.mem.gc_incremental",
                                     nsnull);

  Preferences::RegisterCallback(SetMemoryGCSliceTimePrefChangedCallback,
                                "javascript.options.mem.gc_incremental_slice_ms");
  SetMemoryGCSliceTimePrefChangedCallback("javascript.options.mem.gc_incremental_slice_ms",
                                          nsnull);

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (!obs)
    return NS_ERROR_FAILURE;

  Preferences::AddBoolVarCache(&sGCOnMemoryPressure,
                               "javascript.options.gc_on_memory_pressure",
                               true);

  nsIObserver* memPressureObserver = new nsMemoryPressureObserver();
  NS_ENSURE_TRUE(memPressureObserver, NS_ERROR_OUT_OF_MEMORY);
  obs->AddObserver(memPressureObserver, "memory-pressure", false);

  sIsInitialized = true;

  return NS_OK;
}


nsScriptNameSpaceManager*
nsJSRuntime::GetNameSpaceManager()
{
  if (sDidShutdown)
    return nsnull;

  if (!gNameSpaceManager) {
    gNameSpaceManager = new nsScriptNameSpaceManager;
    NS_ADDREF(gNameSpaceManager);

    nsresult rv = gNameSpaceManager->Init();
    NS_ENSURE_SUCCESS(rv, nsnull);
  }

  return gNameSpaceManager;
}


void
nsJSRuntime::Shutdown()
{
  nsJSContext::KillGCTimer();
  nsJSContext::KillShrinkGCBuffersTimer();
  nsJSContext::KillCCTimer();

  NS_IF_RELEASE(gNameSpaceManager);

  if (!sContextCount) {
    
    

    NS_IF_RELEASE(sRuntimeService);
    NS_IF_RELEASE(sSecurityManager);
  }

  sDidShutdown = true;
}



nsresult
nsJSRuntime::HoldScriptObject(void* aScriptObject)
{
    NS_ASSERTION(sIsInitialized, "runtime not initialized");
    if (! sRuntime) {
        NS_NOTREACHED("couldn't remove GC root - no runtime");
        return NS_ERROR_FAILURE;
    }

    ::JS_LockGCThingRT(sRuntime, aScriptObject);
    return NS_OK;
}

nsresult
nsJSRuntime::DropScriptObject(void* aScriptObject)
{
  NS_ASSERTION(sIsInitialized, "runtime not initialized");
  if (! sRuntime) {
    NS_NOTREACHED("couldn't remove GC root");
    return NS_ERROR_FAILURE;
  }

  ::JS_UnlockGCThingRT(sRuntime, aScriptObject);
  return NS_OK;
}


nsresult NS_CreateJSRuntime(nsIScriptRuntime **aRuntime)
{
  nsresult rv = nsJSRuntime::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  *aRuntime = new nsJSRuntime();
  if (*aRuntime == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_IF_ADDREF(*aRuntime);
  return NS_OK;
}







class nsJSArgArray : public nsIJSArgArray {
public:
  nsJSArgArray(JSContext *aContext, PRUint32 argc, jsval *argv, nsresult *prv);
  ~nsJSArgArray();
  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsJSArgArray,
                                                         nsIJSArgArray)

  
  NS_DECL_NSIARRAY

  
  nsresult GetArgs(PRUint32 *argc, void **argv);

  void ReleaseJSObjects();

protected:
  JSContext *mContext;
  jsval *mArgv;
  PRUint32 mArgc;
};

nsJSArgArray::nsJSArgArray(JSContext *aContext, PRUint32 argc, jsval *argv,
                           nsresult *prv) :
    mContext(aContext),
    mArgv(nsnull),
    mArgc(argc)
{
  
  
  if (argc) {
    mArgv = (jsval *) PR_CALLOC(argc * sizeof(jsval));
    if (!mArgv) {
      *prv = NS_ERROR_OUT_OF_MEMORY;
      return;
    }
  }

  
  
  if (argv) {
    for (PRUint32 i = 0; i < argc; ++i)
      mArgv[i] = argv[i];
  }

  *prv = argc > 0 ? NS_HOLD_JS_OBJECTS(this, nsJSArgArray) : NS_OK;
}

nsJSArgArray::~nsJSArgArray()
{
  ReleaseJSObjects();
}

void
nsJSArgArray::ReleaseJSObjects()
{
  if (mArgc > 0)
    NS_DROP_JS_OBJECTS(this, nsJSArgArray);
  if (mArgv) {
    PR_DELETE(mArgv);
  }
  mArgc = 0;
}


NS_IMPL_CYCLE_COLLECTION_CLASS(nsJSArgArray)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsJSArgArray)
  tmp->ReleaseJSObjects();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsJSArgArray)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsJSArgArray)
  jsval *argv = tmp->mArgv;
  if (argv) {
    jsval *end;
    for (end = argv + tmp->mArgc; argv < end; ++argv) {
      if (JSVAL_IS_GCTHING(*argv))
        NS_IMPL_CYCLE_COLLECTION_TRACE_CALLBACK(JAVASCRIPT,
                                                JSVAL_TO_GCTHING(*argv),
                                                "mArgv[i]")
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
nsJSArgArray::GetArgs(PRUint32 *argc, void **argv)
{
  *argv = (void *)mArgv;
  *argc = mArgc;
  return NS_OK;
}


NS_IMETHODIMP nsJSArgArray::GetLength(PRUint32 *aLength)
{
  *aLength = mArgc;
  return NS_OK;
}


NS_IMETHODIMP nsJSArgArray::QueryElementAt(PRUint32 index, const nsIID & uuid, void * *result)
{
  *result = nsnull;
  if (index >= mArgc)
    return NS_ERROR_INVALID_ARG;

  if (uuid.Equals(NS_GET_IID(nsIVariant)) || uuid.Equals(NS_GET_IID(nsISupports))) {
    return nsContentUtils::XPConnect()->JSToVariant(mContext, mArgv[index],
                                                    (nsIVariant **)result);
  }
  NS_WARNING("nsJSArgArray only handles nsIVariant");
  return NS_ERROR_NO_INTERFACE;
}


NS_IMETHODIMP nsJSArgArray::IndexOf(PRUint32 startIndex, nsISupports *element, PRUint32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsJSArgArray::Enumerate(nsISimpleEnumerator **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


nsresult NS_CreateJSArgv(JSContext *aContext, PRUint32 argc, void *argv,
                         nsIJSArgArray **aArray)
{
  nsresult rv;
  nsJSArgArray *ret = new nsJSArgArray(aContext, argc,
                                       static_cast<jsval *>(argv), &rv);
  if (ret == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  if (NS_FAILED(rv)) {
    delete ret;
    return rv;
  }
  return ret->QueryInterface(NS_GET_IID(nsIArray), (void **)aArray);
}
