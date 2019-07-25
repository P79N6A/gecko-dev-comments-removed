






































#include "jscntxt.h"
#include "nsJSEnvironment.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIDOMChromeWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMText.h"
#include "nsIDOMAttr.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLOptionElement.h"
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
#include "jsxdrapi.h"
#include "nsIArray.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsITimelineService.h"
#include "nsDOMScriptObjectHolder.h"
#include "prmem.h"
#include "WrapperFactory.h"
#include "nsGlobalWindow.h"

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

const size_t gStackSize = 8192;

#ifdef PR_LOGGING
static PRLogModuleInfo* gJSDiagnostics;
#endif


#ifndef WINCE
#ifdef CompareString
#undef CompareString
#endif
#endif 



#define NS_GC_DELAY                 4000 // ms



#define NS_FIRST_GC_DELAY           10000 // ms



#define NS_CC_DELAY                 5000 // ms

#define JAVASCRIPT nsIProgrammingLanguage::JAVASCRIPT



static nsITimer *sGCTimer;
static nsITimer *sCCTimer;

static bool sGCHasRun;







static PRUint32 sPendingLoadCount;
static PRBool sLoadingInProgress;

static PRUint32 sCCollectedWaitingForGC;
static PRBool sPostGCEventsToConsole;

nsScriptNameSpaceManager *gNameSpaceManager;

static nsIJSRuntimeService *sRuntimeService;
JSRuntime *nsJSRuntime::sRuntime;

static const char kJSRuntimeServiceContractID[] =
  "@mozilla.org/js/xpc/RuntimeService;1";

static JSGCCallback gOldJSGCCallback;

static PRBool sIsInitialized;
static PRBool sDidShutdown;

static PRInt32 sContextCount;

static PRTime sMaxScriptRunTime;
static PRTime sMaxChromeScriptRunTime;

static nsIScriptSecurityManager *sSecurityManager;




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
  nsJSContext::GarbageCollectNow();
  nsJSContext::CycleCollectNow();
  return NS_OK;
}





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

class nsAutoPoolRelease {
public:
  nsAutoPoolRelease(JSArenaPool *p, void *m) : mPool(p), mMark(m) {}
  ~nsAutoPoolRelease() { JS_ARENA_RELEASE(mPool, mMark); }
private:
  JSArenaPool *mPool;
  void *mMark;
};





PRBool
NS_HandleScriptError(nsIScriptGlobalObject *aScriptGlobal,
                     nsScriptErrorEvent *aErrorEvent,
                     nsEventStatus *aStatus)
{
  PRBool called = PR_FALSE;
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
      called = PR_TRUE;
    }
    --errorDepth;
  }
  return called;
}

class ScriptErrorEvent : public nsRunnable
{
public:
  ScriptErrorEvent(nsIScriptGlobalObject* aScriptGlobal,
                   PRUint32 aLineNr, PRUint32 aColumn, PRUint32 aFlags,
                   const nsAString& aErrorMsg,
                   const nsAString& aFileName,
                   const nsAString& aSourceLine,
                   PRBool aDispatchEvent,
                   PRUint64 aWindowID)
  : mScriptGlobal(aScriptGlobal), mLineNr(aLineNr), mColumn(aColumn),
    mFlags(aFlags), mErrorMsg(aErrorMsg), mFileName(aFileName),
    mSourceLine(aSourceLine), mDispatchEvent(aDispatchEvent),
    mWindowID(aWindowID)
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
        sHandlingScriptError = PR_TRUE; 

        nsRefPtr<nsPresContext> presContext;
        docShell->GetPresContext(getter_AddRefs(presContext));

        if (presContext) {
          nsScriptErrorEvent errorevent(PR_TRUE, NS_LOAD_ERROR);

          errorevent.fileName = mFileName.get();

          nsCOMPtr<nsIScriptObjectPrincipal> sop(do_QueryInterface(win));
          NS_ENSURE_STATE(sop);
          nsIPrincipal* p = sop->GetPrincipal();
          NS_ENSURE_STATE(p);

          PRBool sameOrigin = mFileName.IsVoid();

          if (p && !sameOrigin) {
            nsCOMPtr<nsIURI> errorURI;
            NS_NewURI(getter_AddRefs(errorURI), mFileName);
            if (errorURI) {
              
              
              
              
              sameOrigin = NS_SUCCEEDED(p->CheckMayLoad(errorURI, PR_FALSE));
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
            
            
            
            
            
            static PRUnichar nullFilename[] = { PRUnichar(0) };
            errorevent.fileName = nullFilename;
          }

          nsEventDispatcher::Dispatch(win, presContext, &errorevent, nsnull,
                                      &status);
        }

        sHandlingScriptError = PR_FALSE;
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

        nsCOMPtr<nsIScriptError2> error2(do_QueryInterface(errorObject));
        if (error2) {
          rv = error2->InitWithWindowID(mErrorMsg.get(), mFileName.get(),
                                        mSourceLine.get(),
                                        mLineNr, mColumn, mFlags,
                                        category, mWindowID);
        } else {
          rv = errorObject->Init(mErrorMsg.get(), mFileName.get(),
                                 mSourceLine.get(),
                                 mLineNr, mColumn, mFlags,
                                 category);
        }

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
  PRUint32                        mLineNr;
  PRUint32                        mColumn;
  PRUint32                        mFlags;
  nsString                        mErrorMsg;
  nsString                        mFileName;
  nsString                        mSourceLine;
  PRBool                          mDispatchEvent;
  PRUint64                        mWindowID;

  static PRBool sHandlingScriptError;
};

PRBool ScriptErrorEvent::sHandlingScriptError = PR_FALSE;





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
        fileName.SetIsVoid(PR_TRUE);
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
      PRUint64 windowID = win ? win->WindowID() : 0;
      nsContentUtils::AddScriptRunner(
        new ScriptErrorEvent(globalObject, report->lineno,
                             report->uctokenptr - report->uclinebuf,
                             report->flags, msg, fileName, sourceLine,
                             report->errorNumber != JSMSG_OUT_OF_MEMORY,
                             windowID));
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
  PRBool isTrackingChromeCodeTime =
    global && xpc::AccessCheck::isChrome(global->getCompartment());
  if (duration < (isTrackingChromeCodeTime ?
                  sMaxChromeScriptRunTime : sMaxScriptRunTime)) {
    return JS_TRUE;
  }

  if (!nsContentUtils::IsSafeToRunScript()) {
    
    
    
    

    JS_ReportWarning(cx, "A long running script was terminated");
    return JS_FALSE;
  }

  
  
  
  nsCOMPtr<nsIPrompt> prompt = GetPromptFromContext(ctx);
  NS_ENSURE_TRUE(prompt, JS_TRUE);

  
  JSStackFrame* fp = ::JS_GetScriptedCaller(cx, NULL);
  PRBool debugPossible = (fp != nsnull && cx->debugHooks &&
                          cx->debugHooks->debuggerHandler != nsnull);
#ifdef MOZ_JSDEBUGGER
  
  if (debugPossible) {
    PRBool jsds_IsOn = PR_FALSE;
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

  
  JSScript *script = fp ? ::JS_GetFrameScript(cx, fp) : nsnull;
  if (script) {
    const char *filename = ::JS_GetScriptFilename(cx, script);
    if (filename) {
      nsXPIDLString scriptLocation;
      NS_ConvertUTF8toUTF16 filenameUTF16(filename);
      const PRUnichar *formatParams[] = { filenameUTF16.get() };
      rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                 "KillScriptLocation",
                                                 formatParams, 1,
                                                 scriptLocation);

      if (NS_SUCCEEDED(rv) && scriptLocation) {
        msg.AppendLiteral("\n\n");
        msg.Append(scriptLocation);

        JSStackFrame *fp, *iterator = nsnull;
        fp = ::JS_FrameIterator(cx, &iterator);
        if (fp) {
          jsbytecode *pc = ::JS_GetFramePC(cx, fp);
          if (pc) {
            PRUint32 lineno = ::JS_PCToLineNumber(cx, script, pc);
            msg.Append(':');
            msg.AppendInt(lineno);
          }
        }
      }
    }
  }

  PRInt32 buttonPressed = 0; 
  PRBool neverShowDlgChk = PR_FALSE;
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
      nsIPrefBranch *prefBranch = nsContentUtils::GetPrefBranch();

      if (prefBranch) {
        prefBranch->SetIntPref(isTrackingChromeCodeTime ?
                               "dom.max_chrome_script_run_time" :
                               "dom.max_script_run_time", 0);
      }
    }

    ctx->mOperationCallbackTime = PR_Now();
    return JS_TRUE;
  }
  else if ((buttonPressed == 2) && debugPossible) {
    
    jsval rval;
    switch(cx->debugHooks->debuggerHandler(cx, script, ::JS_GetFramePC(cx, fp),
                                           &rval,
                                           cx->debugHooks->
                                           debuggerHandlerData)) {
      case JSTRAP_RETURN:
        JS_SetFrameReturnValue(cx, fp, rval);
        return JS_TRUE;
      case JSTRAP_ERROR:
        JS_ClearPendingException(cx);
        return JS_FALSE;
      case JSTRAP_THROW:
        JS_SetPendingException(cx, rval);
        return JS_FALSE;
      case JSTRAP_CONTINUE:
      default:
        return JS_TRUE;
    }
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
static const char js_zeal_option_str[]   = JS_OPTIONS_DOT_STR "gczeal";
#endif
static const char js_tracejit_content_str[]   = JS_OPTIONS_DOT_STR "tracejit.content";
static const char js_tracejit_chrome_str[]    = JS_OPTIONS_DOT_STR "tracejit.chrome";
static const char js_methodjit_content_str[]  = JS_OPTIONS_DOT_STR "methodjit.content";
static const char js_methodjit_chrome_str[]   = JS_OPTIONS_DOT_STR "methodjit.chrome";
static const char js_profiling_content_str[]  = JS_OPTIONS_DOT_STR "jitprofiling.content";
static const char js_profiling_chrome_str[]   = JS_OPTIONS_DOT_STR "jitprofiling.chrome";
static const char js_methodjit_always_str[]   = JS_OPTIONS_DOT_STR "methodjit_always";
static const char js_memlog_option_str[] = JS_OPTIONS_DOT_STR "mem.log";

int
nsJSContext::JSOptionChangedCallback(const char *pref, void *data)
{
  nsJSContext *context = reinterpret_cast<nsJSContext *>(data);
  PRUint32 oldDefaultJSOptions = context->mDefaultJSOptions;
  PRUint32 newDefaultJSOptions = oldDefaultJSOptions;

  sPostGCEventsToConsole = nsContentUtils::GetBoolPref(js_memlog_option_str);

  PRBool strict = nsContentUtils::GetBoolPref(js_strict_option_str);
  if (strict)
    newDefaultJSOptions |= JSOPTION_STRICT;
  else
    newDefaultJSOptions &= ~JSOPTION_STRICT;

  nsIScriptGlobalObject *global = context->GetGlobalObject();
  
  
  nsCOMPtr<nsIDOMChromeWindow> chromeWindow(do_QueryInterface(global));

  PRBool useTraceJIT = nsContentUtils::GetBoolPref(chromeWindow ?
                                                   js_tracejit_chrome_str :
                                                   js_tracejit_content_str);
  PRBool useMethodJIT = nsContentUtils::GetBoolPref(chromeWindow ?
                                                    js_methodjit_chrome_str :
                                                    js_methodjit_content_str);
  PRBool useProfiling = nsContentUtils::GetBoolPref(chromeWindow ?
                                                    js_profiling_chrome_str :
                                                    js_profiling_content_str);
  PRBool useMethodJITAlways = nsContentUtils::GetBoolPref(js_methodjit_always_str);
  nsCOMPtr<nsIXULRuntime> xr = do_GetService(XULRUNTIME_SERVICE_CONTRACTID);
  if (xr) {
    PRBool safeMode = PR_FALSE;
    xr->GetInSafeMode(&safeMode);
    if (safeMode) {
      useTraceJIT = PR_FALSE;
      useMethodJIT = PR_FALSE;
      useProfiling = PR_FALSE;
      useMethodJITAlways = PR_TRUE;
    }
  }    

  if (useTraceJIT)
    newDefaultJSOptions |= JSOPTION_JIT;
  else
    newDefaultJSOptions &= ~JSOPTION_JIT;

  if (useMethodJIT)
    newDefaultJSOptions |= JSOPTION_METHODJIT;
  else
    newDefaultJSOptions &= ~JSOPTION_METHODJIT;

  if (useProfiling)
    newDefaultJSOptions |= JSOPTION_PROFILING;
  else
    newDefaultJSOptions &= ~JSOPTION_PROFILING;

  if (useMethodJITAlways)
    newDefaultJSOptions |= JSOPTION_METHODJIT_ALWAYS;
  else
    newDefaultJSOptions &= ~JSOPTION_METHODJIT_ALWAYS;

#ifdef DEBUG
  
  
  PRBool strictDebug = nsContentUtils::GetBoolPref(js_strict_debug_option_str);
  
  
  if (strictDebug && (newDefaultJSOptions & JSOPTION_STRICT) == 0) {
    if (chromeWindow)
      newDefaultJSOptions |= JSOPTION_STRICT;
  }
#endif

  PRBool werror = nsContentUtils::GetBoolPref(js_werror_option_str);
  if (werror)
    newDefaultJSOptions |= JSOPTION_WERROR;
  else
    newDefaultJSOptions &= ~JSOPTION_WERROR;

  PRBool relimit = nsContentUtils::GetBoolPref(js_relimit_option_str);
  if (relimit)
    newDefaultJSOptions |= JSOPTION_RELIMIT;
  else
    newDefaultJSOptions &= ~JSOPTION_RELIMIT;

  ::JS_SetOptions(context->mContext, newDefaultJSOptions & JSRUNOPTION_MASK);

  
  context->mDefaultJSOptions = newDefaultJSOptions;

#ifdef JS_GC_ZEAL
  PRInt32 zeal = nsContentUtils::GetIntPref(js_zeal_option_str, -1);
  if (zeal >= 0)
    ::JS_SetGCZeal(context->mContext, (PRUint8)zeal);
#endif

  return 0;
}

nsJSContext::nsJSContext(JSRuntime *aRuntime)
  : mGCOnDestruction(PR_TRUE),
    mExecuteDepth(0)
{

  ++sContextCount;

  mDefaultJSOptions = JSOPTION_PRIVATE_IS_NSISUPPORTS | JSOPTION_ANONFUNFIX;

  mContext = ::JS_NewContext(aRuntime, gStackSize);
  if (mContext) {
    ::JS_SetContextPrivate(mContext, static_cast<nsIScriptContext *>(this));

    
    mDefaultJSOptions |= ::JS_GetOptions(mContext);

    
    ::JS_SetOptions(mContext, mDefaultJSOptions);

    
    nsContentUtils::RegisterPrefCallback(js_options_dot_str,
                                         JSOptionChangedCallback,
                                         this);

    ::JS_SetOperationCallback(mContext, DOMOperationCallback);

    xpc_LocalizeContext(mContext);
  }
  mIsInitialized = PR_FALSE;
  mTerminations = nsnull;
  mScriptsEnabled = PR_TRUE;
  mOperationCallbackTime = 0;
  mModalStateTime = 0;
  mModalStateDepth = 0;
  mProcessingScriptTag = PR_FALSE;
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
  if (!mContext)
    return;

  
  ::JS_SetContextPrivate(mContext, nsnull);

  
  nsContentUtils::UnregisterPrefCallback(js_options_dot_str,
                                         JSOptionChangedCallback,
                                         this);

  PRBool do_gc = mGCOnDestruction && !sGCTimer;

  
  nsIXPConnect *xpc = nsContentUtils::XPConnect();
  if (xpc) {
    xpc->ReleaseJSContext(mContext, !do_gc);
  } else if (do_gc) {
    ::JS_DestroyContext(mContext);
  } else {
    ::JS_DestroyContextNoGC(mContext);
  }
  mContext = nsnull;
}


NS_IMPL_CYCLE_COLLECTION_CLASS(nsJSContext)
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsJSContext)
NS_IMPL_CYCLE_COLLECTION_TRACE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsJSContext)
  NS_ASSERTION(!tmp->mContext || tmp->mContext->outstandingRequests == 0,
               "Trying to unlink a context with outstanding requests.");
  tmp->mIsInitialized = PR_FALSE;
  tmp->mGCOnDestruction = PR_FALSE;
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
    refcnt += mContext->outstandingRequests;
  return refcnt;
}

nsresult
nsJSContext::EvaluateStringWithValue(const nsAString& aScript,
                                     void *aScopeObject,
                                     nsIPrincipal *aPrincipal,
                                     const char *aURL,
                                     PRUint32 aLineNo,
                                     PRUint32 aVersion,
                                     void* aRetValue,
                                     PRBool* aIsUndefined)
{
  NS_TIME_FUNCTION_MIN_FMT(1.0, "%s (line %d) (url: %s, line: %d)", MOZ_FUNCTION_NAME,
                           __LINE__, aURL, aLineNo);

  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (!mScriptsEnabled) {
    if (aIsUndefined) {
      *aIsUndefined = PR_TRUE;
    }

    return NS_OK;
  }

  nsresult rv;
  if (!aScopeObject)
    aScopeObject = ::JS_GetGlobalObject(mContext);

  
  
  
  JSPrincipals *jsprin;
  nsIPrincipal *principal = aPrincipal;
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

  principal->GetJSPrincipals(mContext, &jsprin);

  

  PRBool ok = PR_FALSE;

  rv = sSecurityManager->CanExecuteScripts(mContext, principal, &ok);
  if (NS_FAILED(rv)) {
    JSPRINCIPALS_DROP(mContext, jsprin);
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  nsCOMPtr<nsIJSContextStack> stack =
           do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
  if (NS_FAILED(rv) || NS_FAILED(stack->Push(mContext))) {
    JSPRINCIPALS_DROP(mContext, jsprin);
    return NS_ERROR_FAILURE;
  }

  jsval val;

  rv = sSecurityManager->PushContextPrincipal(mContext, nsnull, principal);
  NS_ENSURE_SUCCESS(rv, rv);

  nsJSContext::TerminationFuncHolder holder(this);

  
  
  
  if (ok && ((JSVersion)aVersion) != JSVERSION_UNKNOWN) {

    JSAutoRequest ar(mContext);

    JSAutoEnterCompartment ac;
    if (!ac.enter(mContext, (JSObject *)aScopeObject)) {
      JSPRINCIPALS_DROP(mContext, jsprin);
      stack->Pop(nsnull);
      return NS_ERROR_FAILURE;
    }

    ++mExecuteDepth;

    ok = ::JS_EvaluateUCScriptForPrincipalsVersion(mContext,
                                                   (JSObject *)aScopeObject,
                                                   jsprin,
                                                   (jschar*)PromiseFlatString(aScript).get(),
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

  
  JSPRINCIPALS_DROP(mContext, jsprin);

  
  if (ok) {
    if (aIsUndefined) {
      *aIsUndefined = JSVAL_IS_VOID(val);
    }

    *static_cast<jsval*>(aRetValue) = val;
    
    
    
  }
  else {
    if (aIsUndefined) {
      *aIsUndefined = PR_TRUE;
    }
  }

  sSecurityManager->PopContextPrincipal(mContext);

  
  if (NS_FAILED(stack->Pop(nsnull)))
    rv = NS_ERROR_FAILURE;

  
  ScriptEvaluated(PR_TRUE);

  return rv;

}



static nsresult
JSValueToAString(JSContext *cx, jsval val, nsAString *result,
                 PRBool *isUndefined)
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
    *isUndefined = PR_TRUE;
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
                            void *aScopeObject,
                            nsIPrincipal *aPrincipal,
                            const char *aURL,
                            PRUint32 aLineNo,
                            PRUint32 aVersion,
                            nsAString *aRetValue,
                            PRBool* aIsUndefined)
{
  NS_TIME_FUNCTION_MIN_FMT(1.0, "%s (line %d) (url: %s, line: %d)", MOZ_FUNCTION_NAME,
                           __LINE__, aURL, aLineNo);

  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (!mScriptsEnabled) {
    if (aIsUndefined) {
      *aIsUndefined = PR_TRUE;
    }

    if (aRetValue) {
      aRetValue->Truncate();
    }

    return NS_OK;
  }

  nsresult rv;
  if (!aScopeObject)
    aScopeObject = ::JS_GetGlobalObject(mContext);

  
  
  
  JSPrincipals *jsprin;
  nsIPrincipal *principal = aPrincipal;
  if (aPrincipal) {
    aPrincipal->GetJSPrincipals(mContext, &jsprin);
  }
  else {
    nsCOMPtr<nsIScriptObjectPrincipal> objPrincipal =
      do_QueryInterface(GetGlobalObject(), &rv);
    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;
    principal = objPrincipal->GetPrincipal();
    if (!principal)
      return NS_ERROR_FAILURE;
    principal->GetJSPrincipals(mContext, &jsprin);
  }

  

  PRBool ok = PR_FALSE;

  rv = sSecurityManager->CanExecuteScripts(mContext, principal, &ok);
  if (NS_FAILED(rv)) {
    JSPRINCIPALS_DROP(mContext, jsprin);
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  nsCOMPtr<nsIJSContextStack> stack =
           do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
  if (NS_FAILED(rv) || NS_FAILED(stack->Push(mContext))) {
    JSPRINCIPALS_DROP(mContext, jsprin);
    return NS_ERROR_FAILURE;
  }

  
  
  
  jsval val = JSVAL_VOID;
  jsval* vp = aRetValue ? &val : NULL;

  rv = sSecurityManager->PushContextPrincipal(mContext, nsnull, principal);
  NS_ENSURE_SUCCESS(rv, rv);

  nsJSContext::TerminationFuncHolder holder(this);

  ++mExecuteDepth;

  
  
  
  if (ok && ((JSVersion)aVersion) != JSVERSION_UNKNOWN) {
    JSAutoRequest ar(mContext);
    JSAutoEnterCompartment ac;
    if (!ac.enter(mContext, (JSObject *)aScopeObject)) {
      stack->Pop(nsnull);
      JSPRINCIPALS_DROP(mContext, jsprin);
      return NS_ERROR_FAILURE;
    }

    ok = ::JS_EvaluateUCScriptForPrincipalsVersion(mContext,
                                                   (JSObject *)aScopeObject,
                                                   jsprin,
                                                   (jschar*)PromiseFlatString(aScript).get(),
                                                   aScript.Length(),
                                                   aURL,
                                                   aLineNo,
                                                   vp,
                                                   JSVersion(aVersion));

    if (!ok) {
      
      
      

      ReportPendingException();
    }
  }

  
  JSPRINCIPALS_DROP(mContext, jsprin);

  
  if (ok) {
    JSAutoRequest ar(mContext);
    JSAutoEnterCompartment ac;
    if (!ac.enter(mContext, (JSObject *)aScopeObject)) {
      stack->Pop(nsnull);
    }
    rv = JSValueToAString(mContext, val, aRetValue, aIsUndefined);
  }
  else {
    if (aIsUndefined) {
      *aIsUndefined = PR_TRUE;
    }

    if (aRetValue) {
      aRetValue->Truncate();
    }
  }

  --mExecuteDepth;

  sSecurityManager->PopContextPrincipal(mContext);

  
  if (NS_FAILED(stack->Pop(nsnull)))
    rv = NS_ERROR_FAILURE;

  
  ScriptEvaluated(PR_TRUE);

  return rv;
}

nsresult
nsJSContext::CompileScript(const PRUnichar* aText,
                           PRInt32 aTextLength,
                           void *aScopeObject,
                           nsIPrincipal *aPrincipal,
                           const char *aURL,
                           PRUint32 aLineNo,
                           PRUint32 aVersion,
                           nsScriptObjectHolder &aScriptObject)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;
  NS_ENSURE_ARG_POINTER(aPrincipal);

  if (!aScopeObject)
    aScopeObject = ::JS_GetGlobalObject(mContext);

  JSPrincipals *jsprin;
  aPrincipal->GetJSPrincipals(mContext, &jsprin);
  

  PRBool ok = PR_FALSE;

  rv = sSecurityManager->CanExecuteScripts(mContext, aPrincipal, &ok);
  if (NS_FAILED(rv)) {
    JSPRINCIPALS_DROP(mContext, jsprin);
    return NS_ERROR_FAILURE;
  }

  aScriptObject.drop(); 

  
  
  
  if (ok && ((JSVersion)aVersion) != JSVERSION_UNKNOWN) {
    JSAutoRequest ar(mContext);

    JSObject* scriptObj =
        ::JS_CompileUCScriptForPrincipalsVersion(mContext,
                                                 (JSObject *)aScopeObject,
                                                 jsprin,
                                                 (jschar*) aText,
                                                 aTextLength,
                                                 aURL,
                                                 aLineNo,
                                                 JSVersion(aVersion));
    if (scriptObj) {
      NS_ASSERTION(aScriptObject.getScriptTypeID()==JAVASCRIPT,
                   "Expecting JS script object holder");
      rv = aScriptObject.set(scriptObj);
    } else {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }

  
  JSPRINCIPALS_DROP(mContext, jsprin);
  return rv;
}

nsresult
nsJSContext::ExecuteScript(void *aScriptObject,
                           void *aScopeObject,
                           nsAString* aRetValue,
                           PRBool* aIsUndefined)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (!mScriptsEnabled) {
    if (aIsUndefined) {
      *aIsUndefined = PR_TRUE;
    }

    if (aRetValue) {
      aRetValue->Truncate();
    }

    return NS_OK;
  }

  nsresult rv;

  if (!aScopeObject)
    aScopeObject = ::JS_GetGlobalObject(mContext);

  
  
  nsCOMPtr<nsIJSContextStack> stack =
           do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
  if (NS_FAILED(rv) || NS_FAILED(stack->Push(mContext))) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  jsval val;
  JSBool ok;

  JSObject *scriptObj = (JSObject*)aScriptObject;
  nsCOMPtr<nsIPrincipal> principal;

  rv = sSecurityManager->GetObjectPrincipal(mContext, scriptObj, getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sSecurityManager->PushContextPrincipal(mContext, nsnull, principal);
  NS_ENSURE_SUCCESS(rv, rv);

  nsJSContext::TerminationFuncHolder holder(this);
  JSAutoRequest ar(mContext);
  ++mExecuteDepth;
  ok = ::JS_ExecuteScript(mContext, (JSObject *)aScopeObject, scriptObj, &val);

  if (ok) {
    
    rv = JSValueToAString(mContext, val, aRetValue, aIsUndefined);
  } else {
    if (aIsUndefined) {
      *aIsUndefined = PR_TRUE;
    }

    if (aRetValue) {
      aRetValue->Truncate();
    }
  }

  --mExecuteDepth;

  sSecurityManager->PopContextPrincipal(mContext);

  
  if (NS_FAILED(stack->Pop(nsnull)))
    rv = NS_ERROR_FAILURE;

  
  ScriptEvaluated(PR_TRUE);

  return rv;
}


#ifdef DEBUG
PRBool
AtomIsEventHandlerName(nsIAtom *aName)
{
  const PRUnichar *name = aName->GetUTF16String();

  const PRUnichar *cp;
  PRUnichar c;
  for (cp = name; *cp != '\0'; ++cp)
  {
    c = *cp;
    if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z'))
      return PR_FALSE;
  }

  return PR_TRUE;
}
#endif



nsresult
nsJSContext::JSObjectFromInterface(nsISupports* aTarget, void *aScope, JSObject **aRet)
{
  
  if (!aTarget) {
      *aRet = nsnull;
      return NS_OK;
  }

  
  
  
  nsresult rv;
  jsval v;
  rv = nsContentUtils::WrapNative(mContext, (JSObject *)aScope, aTarget, &v);
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
                                 nsScriptObjectHolder &aHandler)
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
  return aHandler.set((void *)handler);
}



nsresult
nsJSContext::CompileFunction(void* aTarget,
                             const nsACString& aName,
                             PRUint32 aArgCount,
                             const char** aArgArray,
                             const nsAString& aBody,
                             const char* aURL,
                             PRUint32 aLineNo,
                             PRUint32 aVersion,
                             PRBool aShared,
                             void** aFunctionObject)
{
  NS_TIME_FUNCTION_FMT(1.0, "%s (line %d) (function: %s, url: %s, line: %d)", MOZ_FUNCTION_NAME,
                       __LINE__, aName.BeginReading(), aURL, aLineNo);

  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  
  
  if ((JSVersion)aVersion == JSVERSION_UNKNOWN) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  JSPrincipals *jsprin = nsnull;

  nsIScriptGlobalObject *global = GetGlobalObject();
  if (global) {
    
    nsCOMPtr<nsIScriptObjectPrincipal> globalData = do_QueryInterface(global);
    if (globalData) {
      nsIPrincipal *prin = globalData->GetPrincipal();
      if (!prin)
        return NS_ERROR_FAILURE;
      prin->GetJSPrincipals(mContext, &jsprin);
    }
  }

  JSObject *target = (JSObject*)aTarget;

  JSAutoRequest ar(mContext);

  JSFunction* fun =
      ::JS_CompileUCFunctionForPrincipalsVersion(mContext,
                                                 aShared ? nsnull : target, jsprin,
                                                 PromiseFlatCString(aName).get(),
                                                 aArgCount, aArgArray,
                                                 (jschar*)PromiseFlatString(aBody).get(),
                                                 aBody.Length(),
                                                 aURL, aLineNo,
                                                 JSVersion(aVersion));

  if (jsprin)
    JSPRINCIPALS_DROP(mContext, jsprin);
  if (!fun)
    return NS_ERROR_FAILURE;

  JSObject *handler = ::JS_GetFunctionObject(fun);
  if (aFunctionObject)
    *aFunctionObject = (void*) handler;
  return NS_OK;
}

nsresult
nsJSContext::CallEventHandler(nsISupports* aTarget, void *aScope, void *aHandler,
                              nsIArray *aargv, nsIVariant **arv)
{
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  if (!mScriptsEnabled) {
    return NS_OK;
  }

#ifdef NS_FUNCTION_TIMER
  {
    JSObject *obj = static_cast<JSObject *>(aHandler);
    JSString *id = JS_GetFunctionId(static_cast<JSFunction *>(JS_GetPrivate(mContext, obj)));
    JSAutoByteString bytes;
    const char *name = !id ? "anonymous" : bytes.encode(mContext, id) ? bytes.ptr() : "<error>";
    NS_TIME_FUNCTION_FMT(1.0, "%s (line %d) (function: %s)", MOZ_FUNCTION_NAME, __LINE__, name);
  }
#endif

  JSAutoRequest ar(mContext);
  JSObject* target = nsnull;
  nsresult rv = JSObjectFromInterface(aTarget, aScope, &target);
  NS_ENSURE_SUCCESS(rv, rv);

  js::AutoObjectRooter targetVal(mContext, target);
  jsval rval = JSVAL_VOID;

  
  
  
  

  nsCxPusher pusher;
  if (!pusher.Push(mContext, PR_TRUE))
    return NS_ERROR_FAILURE;

  
  rv = sSecurityManager->CheckFunctionAccess(mContext, aHandler, target);

  nsJSContext::TerminationFuncHolder holder(this);

  if (NS_SUCCEEDED(rv)) {
    
    PRUint32 argc = 0;
    jsval *argv = nsnull;

    JSObject *funobj = static_cast<JSObject *>(aHandler);
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
    if (!ac.enter(mContext, funobj) || !JS_WrapObject(mContext, &target)) {
      sSecurityManager->PopContextPrincipal(mContext);
      return NS_ERROR_FAILURE;
    }

    js::LazilyConstructed<nsAutoPoolRelease> poolRelease;
    js::LazilyConstructed<js::AutoArrayRooter> tvr;

    
    
    
    
    
    rv = ConvertSupportsTojsvals(aargv, target, &argc,
                                 &argv, poolRelease, tvr);
    NS_ENSURE_SUCCESS(rv, rv);

    ++mExecuteDepth;
    PRBool ok = ::JS_CallFunctionValue(mContext, target,
                                       funval, argc, argv, &rval);
    --mExecuteDepth;

    if (!ok) {
      
      
      

      ReportPendingException();

      
      rval = JSVAL_VOID;

      
      rv = NS_ERROR_FAILURE;
    }

    sSecurityManager->PopContextPrincipal(mContext);
  }

  pusher.Pop();

  
  
  if (NS_SUCCEEDED(rv)) {
    if (rval == JSVAL_NULL)
      *arv = nsnull;
    else
      rv = nsContentUtils::XPConnect()->JSToVariant(mContext, rval, arv);
  }

  
  ScriptEvaluated(PR_TRUE);

  return rv;
}

nsresult
nsJSContext::BindCompiledEventHandler(nsISupports* aTarget, void *aScope,
                                      nsIAtom *aName,
                                      void *aHandler)
{
  NS_ENSURE_ARG(aHandler);
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

  NS_PRECONDITION(AtomIsEventHandlerName(aName), "Bad event name");

  JSAutoRequest ar(mContext);

  
  JSObject *target = nsnull;
  nsresult rv = JSObjectFromInterface(aTarget, aScope, &target);
  NS_ENSURE_SUCCESS(rv, rv);

  JSObject *funobj = (JSObject*) aHandler;

#ifdef DEBUG
  {
    JSAutoEnterCompartment ac;
    if (!ac.enter(mContext, funobj)) {
      return NS_ERROR_FAILURE;
    }

    NS_ASSERTION(JS_TypeOfValue(mContext,
                                OBJECT_TO_JSVAL(funobj)) == JSTYPE_FUNCTION,
                 "Event handler object not a function");
  }
#endif

  JSAutoEnterCompartment ac;
  if (!ac.enter(mContext, target)) {
    return NS_ERROR_FAILURE;
  }

  
  
  nsCOMPtr<nsIJSContextStack> stack =
           do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
  if (NS_FAILED(rv) || NS_FAILED(stack->Push(mContext))) {
    return NS_ERROR_FAILURE;
  }

  
  if (funobj) { 
    funobj = ::JS_CloneFunctionObject(mContext, funobj, target);
    if (!funobj)
      rv = NS_ERROR_OUT_OF_MEMORY;
  }

  if (NS_SUCCEEDED(rv) &&
      
      !::JS_DefineProperty(mContext, target, nsAtomCString(aName).get(),
                           OBJECT_TO_JSVAL(funobj), nsnull, nsnull,
                           JSPROP_ENUMERATE | JSPROP_PERMANENT)) {
    ReportPendingException();
    rv = NS_ERROR_FAILURE;
  }

  
  

  if (NS_FAILED(stack->Pop(nsnull)) && NS_SUCCEEDED(rv)) {
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

nsresult
nsJSContext::GetBoundEventHandler(nsISupports* aTarget, void *aScope,
                                  nsIAtom* aName,
                                  nsScriptObjectHolder &aHandler)
{
    NS_PRECONDITION(AtomIsEventHandlerName(aName), "Bad event name");

    JSAutoRequest ar(mContext);
    JSObject *obj = nsnull;
    nsresult rv = JSObjectFromInterface(aTarget, aScope, &obj);
    NS_ENSURE_SUCCESS(rv, rv);

    JSAutoEnterCompartment ac;
    if (!ac.enter(mContext, obj)) {
      return NS_ERROR_FAILURE;
    }

    jsval funval;
    if (!JS_LookupProperty(mContext, obj,
                           nsAtomCString(aName).get(), &funval))
        return NS_ERROR_FAILURE;

    if (JS_TypeOfValue(mContext, funval) != JSTYPE_FUNCTION) {
        NS_WARNING("Event handler object not a function");
        aHandler.drop();
        return NS_OK;
    }
    NS_ASSERTION(aHandler.getScriptTypeID()==JAVASCRIPT,
                 "Expecting JS script object holder");
    return aHandler.set(JSVAL_TO_OBJECT(funval));
}


nsresult
nsJSContext::Serialize(nsIObjectOutputStream* aStream, void *aScriptObject)
{
    JSObject *mJSObject = (JSObject *)aScriptObject;
    if (!mJSObject)
        return NS_ERROR_FAILURE;

    nsresult rv;

    JSContext* cx = mContext;
    JSXDRState *xdr = ::JS_XDRNewMem(cx, JSXDR_ENCODE);
    if (! xdr)
        return NS_ERROR_OUT_OF_MEMORY;
    xdr->userdata = (void*) aStream;

    JSAutoRequest ar(cx);
    if (! ::JS_XDRScriptObject(xdr, &mJSObject)) {
        rv = NS_ERROR_FAILURE;  
    } else {
        
        
        
        
        
        
        
        
        
        
        
        
        

        uint32 size;
        const char* data = reinterpret_cast<const char*>
                                           (::JS_XDRMemGetData(xdr, &size));
        NS_ASSERTION(data, "no decoded JSXDRState data!");

        rv = aStream->Write32(size);
        if (NS_SUCCEEDED(rv))
            rv = aStream->WriteBytes(data, size);
    }

    ::JS_XDRDestroy(xdr);
    if (NS_FAILED(rv)) return rv;

    return rv;
}

nsresult
nsJSContext::Deserialize(nsIObjectInputStream* aStream,
                         nsScriptObjectHolder &aResult)
{
    JSObject *result = nsnull;
    nsresult rv;

    NS_TIME_FUNCTION_MIN(1.0);

    NS_TIMELINE_MARK_FUNCTION("js script deserialize");

    PRUint32 size;
    rv = aStream->Read32(&size);
    if (NS_FAILED(rv)) return rv;

    char* data;
    rv = aStream->ReadBytes(size, &data);
    if (NS_FAILED(rv)) return rv;

    JSContext* cx = mContext;

    JSXDRState *xdr = ::JS_XDRNewMem(cx, JSXDR_DECODE);
    if (! xdr) {
        rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
        xdr->userdata = (void*) aStream;
        JSAutoRequest ar(cx);
        ::JS_XDRMemSetData(xdr, data, size);

        if (! ::JS_XDRScriptObject(xdr, &result)) {
            rv = NS_ERROR_FAILURE;  
        }

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        uint32 junk;
        data = (char*) ::JS_XDRMemGetData(xdr, &junk);
        if (data)
            ::JS_XDRMemSetData(xdr, NULL, 0);
        ::JS_XDRDestroy(xdr);
    }

    
    
    if (data)
        nsMemory::Free(data);
    NS_ASSERTION(aResult.getScriptTypeID()==JAVASCRIPT,
                 "Expecting JS script object holder");

    
    
    NS_ENSURE_SUCCESS(rv, rv);

    return aResult.set(result);
}

void
nsJSContext::SetDefaultLanguageVersion(PRUint32 aVersion)
{
  ::JS_SetVersion(mContext, (JSVersion)aVersion);
}

nsIScriptGlobalObject *
nsJSContext::GetGlobalObject()
{
  JSObject *global = ::JS_GetGlobalObject(mContext);

  if (!global) {
    return nsnull;
  }

  OBJ_TO_INNER_OBJECT(mContext, global);
  if (!global) {
    return nsnull;
  }

  JSClass *c = JS_GET_CLASS(mContext, global);

  if (!c || ((~c->flags) & (JSCLASS_HAS_PRIVATE |
                            JSCLASS_PRIVATE_IS_NSISUPPORTS))) {
    return nsnull;
  }

  nsISupports *priv = (nsISupports *)global->getPrivate();

  nsCOMPtr<nsIXPConnectWrappedNative> wrapped_native =
    do_QueryInterface(priv);

  nsCOMPtr<nsIScriptGlobalObject> sgo;
  if (wrapped_native) {
    
    

    sgo = do_QueryWrappedNative(wrapped_native);
  } else {
    sgo = do_QueryInterface(priv);
  }

  
  
  nsCOMPtr<nsPIDOMWindow> pwin(do_QueryInterface(sgo));
  if (!pwin)
    return sgo;

  return static_cast<nsGlobalWindow *>(pwin->GetOuterWindow());
}

void *
nsJSContext::GetNativeGlobal()
{
    return ::JS_GetGlobalObject(mContext);
}

nsresult
nsJSContext::CreateNativeGlobalForInner(
                                nsIScriptGlobalObject *aNewInner,
                                PRBool aIsChrome,
                                nsIPrincipal *aPrincipal,
                                void **aNativeGlobal, nsISupports **aHolder)
{
  nsIXPConnect *xpc = nsContentUtils::XPConnect();
  PRUint32 flags = aIsChrome? nsIXPConnect::FLAG_SYSTEM_GLOBAL_OBJECT : 0;
  nsCOMPtr<nsIXPConnectJSObjectHolder> jsholder;

  nsCOMPtr<nsIPrincipal> systemPrincipal;
  if (aIsChrome) {
    nsIScriptSecurityManager *ssm = nsContentUtils::GetSecurityManager();
    ssm->GetSystemPrincipal(getter_AddRefs(systemPrincipal));
  }

  nsresult rv = xpc->
          InitClassesWithNewWrappedGlobal(mContext,
                                          aNewInner, NS_GET_IID(nsISupports),
                                          aIsChrome ? systemPrincipal.get() : aPrincipal,
                                          nsnull, flags,
                                          getter_AddRefs(jsholder));
  if (NS_FAILED(rv))
    return rv;
  jsholder->GetJSObject(reinterpret_cast<JSObject **>(aNativeGlobal));
  *aHolder = jsholder.get();
  NS_ADDREF(*aHolder);
  return NS_OK;
}

nsresult
nsJSContext::ConnectToInner(nsIScriptGlobalObject *aNewInner, void *aOuterGlobal)
{
  NS_ENSURE_ARG(aNewInner);
  JSObject *newInnerJSObject = (JSObject *)aNewInner->GetScriptGlobal(JAVASCRIPT);
  JSObject *outerGlobal = (JSObject *)aOuterGlobal;

  
  
  
  
  JS_SetGlobalObject(mContext, outerGlobal);
  NS_ASSERTION(JS_GetPrototype(mContext, outerGlobal) ==
               JS_GetPrototype(mContext, newInnerJSObject),
               "outer and inner globals should have the same prototype");

  return NS_OK;
}

void *
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

  return NS_OK;
}

nsresult
nsJSContext::CreateOuterObject(nsIScriptGlobalObject *aGlobalObject,
                               nsIScriptGlobalObject *aCurrentInner)
{
  mGlobalObjectRef = aGlobalObject;

  nsCOMPtr<nsIDOMChromeWindow> chromeWindow(do_QueryInterface(aGlobalObject));
  PRUint32 flags = 0;

  if (chromeWindow) {
    
    
    
    flags = nsIXPConnect::FLAG_SYSTEM_GLOBAL_OBJECT;

    
    
    
    
    JS_SetOptions(mContext, JS_GetOptions(mContext) | JSOPTION_XML);
  }

  JSObject *outer =
    NS_NewOuterWindowProxy(mContext, aCurrentInner->GetGlobalJSObject());
  if (!outer) {
    return NS_ERROR_FAILURE;
  }

  return SetOuterObject(outer);
}

nsresult
nsJSContext::SetOuterObject(void *aOuterObject)
{
  JSObject *outer = static_cast<JSObject *>(aOuterObject);

  
  JS_SetGlobalObject(mContext, outer);

  
  JSObject *inner = JS_GetParent(mContext, outer);

  nsIXPConnect *xpc = nsContentUtils::XPConnect();
  nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
  nsresult rv = xpc->GetWrappedNativeOfJSObject(mContext, inner,
                                                getter_AddRefs(wrapper));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ABORT_IF_FALSE(wrapper, "bad wrapper");

  wrapper->RefreshPrototype();
  JS_SetPrototype(mContext, outer, JS_GetPrototype(mContext, inner));

  return NS_OK;
}

nsresult
nsJSContext::InitOuterWindow()
{
  JSObject *global = JS_GetGlobalObject(mContext);
  OBJ_TO_INNER_OBJECT(mContext, global);

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
nsJSContext::SetProperty(void *aTarget, const char *aPropName, nsISupports *aArgs)
{
  PRUint32  argc;
  jsval    *argv = nsnull;

  JSAutoRequest ar(mContext);

  js::LazilyConstructed<nsAutoPoolRelease> poolRelease;
  js::LazilyConstructed<js::AutoArrayRooter> tvr;

  nsresult rv;
  rv = ConvertSupportsTojsvals(aArgs, GetNativeGlobal(), &argc,
                               &argv, poolRelease, tvr);
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

  
  
  rv = ::JS_DefineProperty(mContext, reinterpret_cast<JSObject *>(aTarget),
                           aPropName, vargs, nsnull, nsnull, 0) ?
       NS_OK : NS_ERROR_FAILURE;

  return rv;
}

nsresult
nsJSContext::ConvertSupportsTojsvals(nsISupports *aArgs,
                                     void *aScope,
                                     PRUint32 *aArgc,
                                     jsval **aArgv,
                                     js::LazilyConstructed<nsAutoPoolRelease> &aPoolRelease,
                                     js::LazilyConstructed<js::AutoArrayRooter> &aRooter)
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
  PRUint32 argCtr, argCount;
  
  
  nsCOMPtr<nsIArray> argsArray(do_QueryInterface(aArgs));

  if (argsArray) {
    rv = argsArray->GetLength(&argCount);
    NS_ENSURE_SUCCESS(rv, rv);
    if (argCount == 0)
      return NS_OK;
  } else {
    argCount = 1; 
  }

  void *mark = JS_ARENA_MARK(&mContext->tempPool);
  jsval *argv;
  size_t nbytes = argCount * sizeof(jsval);
  JS_ARENA_ALLOCATE_CAST(argv, jsval *, &mContext->tempPool, nbytes);
  NS_ENSURE_TRUE(argv, NS_ERROR_OUT_OF_MEMORY);
  memset(argv, 0, nbytes);  

  
  aPoolRelease.construct(&mContext->tempPool, mark);
  aRooter.construct(mContext, argCount, argv);

  if (argsArray) {
    for (argCtr = 0; argCtr < argCount && NS_SUCCEEDED(rv); argCtr++) {
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
        rv = xpc->VariantToJS(mContext, (JSObject *)aScope, variant,
                              thisval);
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
          rv = nsContentUtils::WrapNative(mContext, (JSObject *)aScope, arg,
                                          &v, getter_AddRefs(wrapper));
          if (NS_SUCCEEDED(rv)) {
            *thisval = v;
          }
        }
      }
    }
  } else {
    nsCOMPtr<nsIVariant> variant(do_QueryInterface(aArgs));
    if (variant)
      rv = xpc->VariantToJS(mContext, (JSObject *)aScope, variant, argv);
    else {
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

      PRBool data;

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
    PRBool hasCap = PR_FALSE;
    nsresult rv = nsContentUtils::GetSecurityManager()->
                    IsCapabilityEnabled("UniversalXPConnect", &hasCap);
    if (NS_SUCCEEDED(rv) && hasCap)
        return JS_TRUE;
    JS_ReportError(cx, "trace-malloc functions require UniversalXPConnect");
    return JS_FALSE;
}

static JSBool
TraceMallocDisable(JSContext *cx, uintN argc, jsval *vp)
{
    if (!CheckUniversalXPConnectForTraceMalloc(cx))
        return JS_FALSE;

    NS_TraceMallocDisable();
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSBool
TraceMallocEnable(JSContext *cx, uintN argc, jsval *vp)
{
    if (!CheckUniversalXPConnectForTraceMalloc(cx))
        return JS_FALSE;

    NS_TraceMallocEnable();
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return JS_TRUE;
}

static JSBool
TraceMallocOpenLogFile(JSContext *cx, uintN argc, jsval *vp)
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
TraceMallocChangeLogFD(JSContext *cx, uintN argc, jsval *vp)
{
    int32 fd, oldfd;

    if (!CheckUniversalXPConnectForTraceMalloc(cx))
        return JS_FALSE;

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
TraceMallocCloseLogFD(JSContext *cx, uintN argc, jsval *vp)
{
    int32 fd;

    if (!CheckUniversalXPConnectForTraceMalloc(cx))
        return JS_FALSE;

    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    if (argc == 0)
        return JS_TRUE;
    if (!JS_ValueToECMAInt32(cx, JS_ARGV(cx, vp)[0], &fd))
        return JS_FALSE;
    NS_TraceMallocCloseLogFD((int) fd);
    return JS_TRUE;
}

static JSBool
TraceMallocLogTimestamp(JSContext *cx, uintN argc, jsval *vp)
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
TraceMallocDumpAllocations(JSContext *cx, uintN argc, jsval *vp)
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

inline PRBool
IsJProfAction(struct sigaction *action)
{
    return (action->sa_sigaction &&
            action->sa_flags == (SA_RESTART | SA_SIGINFO));
}

void NS_JProfStartProfiling();
void NS_JProfStopProfiling();

static JSBool
JProfStartProfilingJS(JSContext *cx, uintN argc, jsval *vp)
{
  NS_JProfStartProfiling();
  return JS_TRUE;
}

void NS_JProfStartProfiling()
{
    
    
    
    struct sigaction action;

    sigaction(SIGALRM, nsnull, &action);
    if (IsJProfAction(&action)) {
        printf("Beginning real-time jprof profiling.\n");
        raise(SIGALRM);
        return;
    }

    sigaction(SIGPROF, nsnull, &action);
    if (IsJProfAction(&action)) {
        printf("Beginning process-time jprof profiling.\n");
        raise(SIGPROF);
        return;
    }

    sigaction(SIGPOLL, nsnull, &action);
    if (IsJProfAction(&action)) {
        printf("Beginning rtc-based jprof profiling.\n");
        raise(SIGPOLL);
        return;
    }

    printf("Could not start jprof-profiling since JPROF_FLAGS was not set.\n");
}

static JSBool
JProfStopProfilingJS(JSContext *cx, uintN argc, jsval *vp)
{
  NS_JProfStopProfiling();
  return JS_TRUE;
}

void
NS_JProfStopProfiling()
{
    raise(SIGUSR1);
    printf("Stopped jprof profiling.\n");
}

static JSFunctionSpec JProfFunctions[] = {
    {"JProfStartProfiling",        JProfStartProfilingJS,      0, 0},
    {"JProfStopProfiling",         JProfStopProfilingJS,       0, 0},
    {nsnull,                       nsnull,                     0, 0}
};

#endif 

#ifdef MOZ_CALLGRIND
static JSFunctionSpec CallgrindFunctions[] = {
    {"startCallgrind",             js_StartCallgrind,          0, 0},
    {"stopCallgrind",              js_StopCallgrind,           0, 0},
    {"dumpCallgrind",              js_DumpCallgrind,           1, 0},
    {nsnull,                       nsnull,                     0, 0}
};
#endif

#ifdef MOZ_VTUNE
static JSFunctionSpec VtuneFunctions[] = {
    {"startVtune",                 js_StartVtune,              1, 0},
    {"stopVtune",                  js_StopVtune,               0, 0},
    {"pauseVtune",                 js_PauseVtune,              0, 0},
    {"resumeVtune",                js_ResumeVtune,             0, 0},
    {nsnull,                       nsnull,                     0, 0}
};
#endif

#ifdef MOZ_TRACEVIS
static JSFunctionSpec EthogramFunctions[] = {
    {"initEthogram",               js_InitEthogram,            0, 0},
    {"shutdownEthogram",           js_ShutdownEthogram,        0, 0},
    {nsnull,                       nsnull,                     0, 0}
};
#endif

nsresult
nsJSContext::InitClasses(void *aGlobalObj)
{
  nsresult rv = NS_OK;

  JSObject *globalObj = static_cast<JSObject *>(aGlobalObj);

  rv = InitializeExternalClasses();
  NS_ENSURE_SUCCESS(rv, rv);

  JSAutoRequest ar(mContext);

  ::JS_SetOptions(mContext, mDefaultJSOptions);

  
  ::JS_DefineProfilingFunctions(mContext, globalObj);

#ifdef NS_TRACE_MALLOC
  
  ::JS_DefineFunctions(mContext, globalObj, TraceMallocFunctions);
#endif

#ifdef MOZ_JPROF
  
  ::JS_DefineFunctions(mContext, globalObj, JProfFunctions);
#endif

#ifdef MOZ_CALLGRIND
  
  ::JS_DefineFunctions(mContext, globalObj, CallgrindFunctions);
#endif

#ifdef MOZ_VTUNE
  
  ::JS_DefineFunctions(mContext, globalObj, VtuneFunctions);
#endif

#ifdef MOZ_TRACEVIS
  
  ::JS_DefineFunctions(mContext, globalObj, EthogramFunctions);
#endif

  JSOptionChangedCallback(js_options_dot_str, this);

  return rv;
}

void
nsJSContext::ClearScope(void *aGlobalObj, PRBool aClearFromProtoChain)
{
  
  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1");
  if (stack && NS_FAILED(stack->Push(mContext))) {
    stack = nsnull;
  }

  if (aGlobalObj) {
    JSObject *obj = (JSObject *)aGlobalObj;
    JSAutoRequest ar(mContext);

    JSAutoEnterCompartment ac;
    ac.enterAndIgnoreErrors(mContext, obj);

    
    
    
    
    jsval window;
    if (!JS_GetProperty(mContext, obj, "window", &window)) {
      window = JSVAL_VOID;

      JS_ClearPendingException(mContext);
    }

    JS_ClearScope(mContext, obj);

    NS_ABORT_IF_FALSE(!xpc::WrapperFactory::IsXrayWrapper(obj), "unexpected wrapper");

    if (window != JSVAL_VOID) {
      if (!JS_DefineProperty(mContext, obj, "window", window,
                             JS_PropertyStub, JS_StrictPropertyStub,
                             JSPROP_ENUMERATE | JSPROP_READONLY |
                             JSPROP_PERMANENT)) {
        JS_ClearPendingException(mContext);
      }
    }

    if (!obj->getParent()) {
      JS_ClearRegExpStatics(mContext, obj);
    }

    
    
    
    
    
    
    
    
    
    
    ::JS_ClearWatchPointsForObject(mContext, obj);

    
    
    
    
    if (aClearFromProtoChain) {
      nsWindowSH::InvalidateGlobalScopePolluter(mContext, obj);

      
      for (JSObject *o = ::JS_GetPrototype(mContext, obj), *next;
           o && (next = ::JS_GetPrototype(mContext, o)); o = next)
        ::JS_ClearScope(mContext, o);
    }
  }

  if (stack) {
    stack->Pop(nsnull);
  }
}

void
nsJSContext::WillInitializeContext()
{
  mIsInitialized = PR_FALSE;
}

void
nsJSContext::DidInitializeContext()
{
  mIsInitialized = PR_TRUE;
}

PRBool
nsJSContext::IsContextInitialized()
{
  return mIsInitialized;
}

void
nsJSContext::FinalizeContext()
{
  ;
}

void
nsJSContext::ScriptEvaluated(PRBool aTerminated)
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

nsresult
nsJSContext::SetTerminationFunction(nsScriptTerminationFunc aFunc,
                                    nsISupports* aRef)
{
  NS_PRECONDITION(GetExecutingScript(), "should be executing script");

  nsJSContext::TerminationFuncClosure* newClosure =
    new nsJSContext::TerminationFuncClosure(aFunc, aRef, mTerminations);
  if (!newClosure) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mTerminations = newClosure;
  return NS_OK;
}

PRBool
nsJSContext::GetScriptsEnabled()
{
  return mScriptsEnabled;
}

void
nsJSContext::SetScriptsEnabled(PRBool aEnabled, PRBool aFireTimeouts)
{
  
  
  mScriptsEnabled = aEnabled;

  nsIScriptGlobalObject *global = GetGlobalObject();

  if (global) {
    global->SetScriptsEnabled(aEnabled, aFireTimeouts);
  }
}


PRBool
nsJSContext::GetProcessingScriptTag()
{
  return mProcessingScriptTag;
}

void
nsJSContext::SetProcessingScriptTag(PRBool aFlag)
{
  mProcessingScriptTag = aFlag;
}

PRBool
nsJSContext::GetExecutingScript()
{
  return JS_IsRunning(mContext) || mExecuteDepth > 0;
}

void
nsJSContext::SetGCOnDestruction(PRBool aGCOnDestruction)
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
nsJSContext::GarbageCollectNow()
{
  NS_TIME_FUNCTION_MIN(1.0);

  KillGCTimer();

  
  
  
  
  
  
  sPendingLoadCount = 0;
  sLoadingInProgress = PR_FALSE;

  if (nsContentUtils::XPConnect()) {
    nsContentUtils::XPConnect()->GarbageCollect();
  }
}


void
nsJSContext::CycleCollectNow(nsICycleCollectorListener *aListener)
{
  if (!NS_IsMainThread()) {
    return;
  }

  NS_TIME_FUNCTION_MIN(1.0);

  KillCCTimer();

  PRTime start = PR_Now();

  PRUint32 suspected = nsCycleCollector_suspectedCount();
  PRUint32 collected = nsCycleCollector_collect(aListener);
  sCCollectedWaitingForGC += collected;

  
  
  if (sCCollectedWaitingForGC > 250) {
    PokeGC();
  }

  if (sPostGCEventsToConsole) {
    PRTime now = PR_Now();
    NS_NAMED_LITERAL_STRING(kFmt,
                            "CC timestamp: %lld, collected: %lu (%lu waiting for GC), suspected: %lu, duration: %llu ms.");
    nsString msg;
    msg.Adopt(nsTextFormatter::smprintf(kFmt.get(), now,
                                        collected, sCCollectedWaitingForGC, suspected,
                                        (now - start) / PR_USEC_PER_MSEC));
    nsCOMPtr<nsIConsoleService> cs =
      do_GetService(NS_CONSOLESERVICE_CONTRACTID);
    if (cs) {
      cs->LogStringMessage(msg.get());
    }
  }
}


void
GCTimerFired(nsITimer *aTimer, void *aClosure)
{
  NS_RELEASE(sGCTimer);

  nsJSContext::GarbageCollectNow();
}


void
CCTimerFired(nsITimer *aTimer, void *aClosure)
{
  NS_RELEASE(sCCTimer);

  nsJSContext::CycleCollectNow();
}


void
nsJSContext::LoadStart()
{
  sLoadingInProgress = PR_TRUE;
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

  
  sLoadingInProgress = PR_FALSE;
  PokeGC();
}


void
nsJSContext::PokeGC()
{
  if (sGCTimer) {
    
    return;
  }

  CallCreateInstance("@mozilla.org/timer;1", &sGCTimer);

  if (!sGCTimer) {
    NS_WARNING("Failed to create timer");
    return;
  }

  static PRBool first = PR_TRUE;

  sGCTimer->InitWithFuncCallback(GCTimerFired, nsnull,
                                 first
                                 ? NS_FIRST_GC_DELAY
                                 : NS_GC_DELAY,
                                 nsITimer::TYPE_ONE_SHOT);

  first = PR_FALSE;
}


void
nsJSContext::MaybePokeCC()
{
  if (nsCycleCollector_suspectedCount() > 1000) {
    PokeCC();
  }
}


void
nsJSContext::PokeCC()
{
  if (sCCTimer || !sGCHasRun) {
    
    return;
  }

  CallCreateInstance("@mozilla.org/timer;1", &sCCTimer);

  if (!sCCTimer) {
    NS_WARNING("Failed to create timer");
    return;
  }

  sCCTimer->InitWithFuncCallback(CCTimerFired, nsnull,
                                 NS_CC_DELAY,
                                 nsITimer::TYPE_ONE_SHOT);
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
nsJSContext::KillCCTimer()
{
  if (sCCTimer) {
    sCCTimer->Cancel();

    NS_RELEASE(sCCTimer);
  }
}

void
nsJSContext::GC()
{
  PokeGC();
}

static JSBool
DOMGCCallback(JSContext *cx, JSGCStatus status)
{
  static PRTime start;

  if (sPostGCEventsToConsole && NS_IsMainThread()) {
    if (status == JSGC_BEGIN) {
      start = PR_Now();
    } else if (status == JSGC_END) {
      PRTime now = PR_Now();
      NS_NAMED_LITERAL_STRING(kFmt, "GC mode: %s, timestamp: %lld, duration: %llu ms.");
      nsString msg;
      msg.Adopt(nsTextFormatter::smprintf(kFmt.get(),
                cx->runtime->gcTriggerCompartment ? "compartment" : "full",
                now,
                (now - start) / PR_USEC_PER_MSEC));
      nsCOMPtr<nsIConsoleService> cs = do_GetService(NS_CONSOLESERVICE_CONTRACTID);
      if (cs) {
        cs->LogStringMessage(msg.get());
      }
    }
  }

  if (status == JSGC_END) {
    sCCollectedWaitingForGC = 0;
    if (sGCTimer) {
      
      nsJSContext::KillGCTimer();

      
      
      
      
      
      if (cx->runtime->gcTriggerCompartment) {
        nsJSContext::PokeGC();

        
        nsJSContext::KillCCTimer();
      }
    } else {
      
      if (!cx->runtime->gcTriggerCompartment) {
        sGCHasRun = true;
        nsJSContext::PokeCC();
      }
    }

    
    
    if (!sGCTimer && JS_GetGCParameter(cx->runtime, JSGC_UNUSED_CHUNKS) > 0) {
      nsJSContext::PokeGC();
    }
  }

  JSBool result = gOldJSGCCallback ? gOldJSGCCallback(cx, status) : JS_TRUE;

  if (status == JSGC_BEGIN && !NS_IsMainThread())
    return JS_FALSE;

  return result;
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
    JSStackFrame* frame = JS_SaveFrameChain(mContext);
    ::JS_ReportPendingException(mContext);
    JS_RestoreFrameChain(mContext, frame);
  }
}






NS_INTERFACE_MAP_BEGIN(nsJSRuntime)
  NS_INTERFACE_MAP_ENTRY(nsIScriptRuntime)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsJSRuntime)
NS_IMPL_RELEASE(nsJSRuntime)

nsresult
nsJSRuntime::CreateContext(nsIScriptContext **aContext)
{
  nsCOMPtr<nsIScriptContext> scriptContext;

  *aContext = new nsJSContext(sRuntime);
  NS_ENSURE_TRUE(*aContext, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aContext);
  return NS_OK;
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
  sGCHasRun = false;
  sPendingLoadCount = 0;
  sLoadingInProgress = PR_FALSE;
  sCCollectedWaitingForGC = 0;
  sPostGCEventsToConsole = PR_FALSE;
  gNameSpaceManager = nsnull;
  sRuntimeService = nsnull;
  sRuntime = nsnull;
  gOldJSGCCallback = nsnull;
  sIsInitialized = PR_FALSE;
  sDidShutdown = PR_FALSE;
  sContextCount = 0;
  sSecurityManager = nsnull;
}

static int
MaxScriptRunTimePrefChangedCallback(const char *aPrefName, void *aClosure)
{
  
  
  PRBool isChromePref =
    strcmp(aPrefName, "dom.max_chrome_script_run_time") == 0;
  PRInt32 time = nsContentUtils::GetIntPref(aPrefName, isChromePref ? 20 : 10);

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
  PRBool reportAll = nsContentUtils::GetBoolPref(aPrefName, PR_FALSE);
  nsContentUtils::XPConnect()->SetReportAllJSExceptions(reportAll);
  return 0;
}

static int
SetMemoryHighWaterMarkPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  PRInt32 highwatermark = nsContentUtils::GetIntPref(aPrefName, 128);

  JS_SetGCParameter(nsJSRuntime::sRuntime, JSGC_MAX_MALLOC_BYTES,
                    highwatermark * 1024L * 1024L);
  return 0;
}

static int
SetMemoryMaxPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  PRInt32 pref = nsContentUtils::GetIntPref(aPrefName, -1);
  
  PRUint32 max = (pref <= 0 || pref >= 0x1000) ? -1 : (PRUint32)pref * 1024 * 1024;
  JS_SetGCParameter(nsJSRuntime::sRuntime, JSGC_MAX_BYTES, max);
  return 0;
}

static int
SetMemoryGCFrequencyPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  PRInt32 triggerFactor = nsContentUtils::GetIntPref(aPrefName, 300);
  JS_SetGCParameter(nsJSRuntime::sRuntime, JSGC_TRIGGER_FACTOR, triggerFactor);
  return 0;
}

static int
SetMemoryGCModePrefChangedCallback(const char* aPrefName, void* aClosure)
{
  PRBool enableCompartmentGC = nsContentUtils::GetBoolPref(aPrefName);
  JS_SetGCParameter(nsJSRuntime::sRuntime, JSGC_MODE, enableCompartmentGC
                                                      ? JSGC_MODE_COMPARTMENT
                                                      : JSGC_MODE_GLOBAL);
  return 0;
}

static JSPrincipals *
ObjectPrincipalFinder(JSContext *cx, JSObject *obj)
{
  if (!sSecurityManager)
    return nsnull;

  nsCOMPtr<nsIPrincipal> principal;
  nsresult rv =
    sSecurityManager->GetObjectPrincipal(cx, obj,
                                         getter_AddRefs(principal));

  if (NS_FAILED(rv) || !principal) {
    return nsnull;
  }

  JSPrincipals *jsPrincipals = nsnull;
  principal->GetJSPrincipals(cx, &jsPrincipals);

  
  
  

  JSPRINCIPALS_DROP(cx, jsPrincipals);

  return jsPrincipals;
}

static JSObject*
DOMReadStructuredClone(JSContext* cx,
                       JSStructuredCloneReader* reader,
                       uint32 tag,
                       uint32 data,
                       void* closure)
{
  
  nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_DOM_DATA_CLONE_ERR);
  return nsnull;
}

static JSBool
DOMWriteStructuredClone(JSContext* cx,
                        JSStructuredCloneWriter* writer,
                        JSObject* obj,
                        void *closure)
{
  
  nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_DOM_DATA_CLONE_ERR);
  return JS_FALSE;
}

static void
DOMStructuredCloneError(JSContext* cx,
                        uint32 errorid)
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

  NS_ASSERTION(!gOldJSGCCallback,
               "nsJSRuntime initialized more than once");

  
  gOldJSGCCallback = ::JS_SetGCCallbackRT(sRuntime, DOMGCCallback);

  JSSecurityCallbacks *callbacks = JS_GetRuntimeSecurityCallbacks(sRuntime);
  NS_ASSERTION(callbacks, "SecMan should have set security callbacks!");

  callbacks->findObjectPrincipals = ObjectPrincipalFinder;

  
  static JSStructuredCloneCallbacks cloneCallbacks = {
    DOMReadStructuredClone,
    DOMWriteStructuredClone,
    DOMStructuredCloneError
  };
  JS_SetStructuredCloneCallbacks(sRuntime, &cloneCallbacks);

  
  nsContentUtils::RegisterPrefCallback("dom.max_script_run_time",
                                       MaxScriptRunTimePrefChangedCallback,
                                       nsnull);
  MaxScriptRunTimePrefChangedCallback("dom.max_script_run_time", nsnull);

  nsContentUtils::RegisterPrefCallback("dom.max_chrome_script_run_time",
                                       MaxScriptRunTimePrefChangedCallback,
                                       nsnull);
  MaxScriptRunTimePrefChangedCallback("dom.max_chrome_script_run_time",
                                      nsnull);

  nsContentUtils::RegisterPrefCallback("dom.report_all_js_exceptions",
                                       ReportAllJSExceptionsPrefChangedCallback,
                                       nsnull);
  ReportAllJSExceptionsPrefChangedCallback("dom.report_all_js_exceptions",
                                           nsnull);

  nsContentUtils::RegisterPrefCallback("javascript.options.mem.high_water_mark",
                                       SetMemoryHighWaterMarkPrefChangedCallback,
                                       nsnull);
  SetMemoryHighWaterMarkPrefChangedCallback("javascript.options.mem.high_water_mark",
                                            nsnull);

  nsContentUtils::RegisterPrefCallback("javascript.options.mem.max",
                                       SetMemoryMaxPrefChangedCallback,
                                       nsnull);
  SetMemoryMaxPrefChangedCallback("javascript.options.mem.max",
                                  nsnull);

  nsContentUtils::RegisterPrefCallback("javascript.options.mem.gc_frequency",
                                       SetMemoryGCFrequencyPrefChangedCallback,
                                       nsnull);
  SetMemoryGCFrequencyPrefChangedCallback("javascript.options.mem.gc_frequency",
                                          nsnull);

  nsContentUtils::RegisterPrefCallback("javascript.options.mem.gc_per_compartment",
                                       SetMemoryGCModePrefChangedCallback,
                                       nsnull);
  SetMemoryGCModePrefChangedCallback("javascript.options.mem.gc_per_compartment",
                                     nsnull);

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (!obs)
    return NS_ERROR_FAILURE;

  nsIObserver* memPressureObserver = new nsMemoryPressureObserver();
  NS_ENSURE_TRUE(memPressureObserver, NS_ERROR_OUT_OF_MEMORY);
  obs->AddObserver(memPressureObserver, "memory-pressure", PR_FALSE);

  sIsInitialized = PR_TRUE;

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
  nsJSContext::KillCCTimer();

  NS_IF_RELEASE(gNameSpaceManager);

  if (!sContextCount) {
    
    

    if (sRuntimeService && sSecurityManager) {
      JSSecurityCallbacks *callbacks = JS_GetRuntimeSecurityCallbacks(sRuntime);
      if (callbacks) {
        NS_ASSERTION(callbacks->findObjectPrincipals == ObjectPrincipalFinder,
                     "Fighting over the findObjectPrincipals callback!");
        callbacks->findObjectPrincipals = NULL;
      }
    }
    NS_IF_RELEASE(sRuntimeService);
    NS_IF_RELEASE(sSecurityManager);
  }

  sDidShutdown = PR_TRUE;
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







class nsJSArgArray : public nsIJSArgArray, public nsIArray {
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
                                                JSVAL_TO_GCTHING(*argv))
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
  if (!mArgv) {
    NS_WARNING("nsJSArgArray has no argv!");
    return NS_ERROR_UNEXPECTED;
  }
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
                         nsIArray **aArray)
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
