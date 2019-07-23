






































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
#include "jscntxt.h"
#include "nsEventDispatcher.h"
#include "nsIContent.h"
#include "nsCycleCollector.h"
#include "nsNetUtil.h"
#include "nsXPCOMCIDInternal.h"
#include "nsIXULRuntime.h"


#include "plstr.h"
#include "nsIPlatformCharset.h"
#include "nsICharsetConverterManager.h"
#include "nsUnicharUtils.h"
#include "nsILocaleService.h"
#include "nsICollation.h"
#include "nsCollationCID.h"
#include "nsDOMClassInfo.h"

#include "jsdbgapi.h"           
#include "jsxdrapi.h"
#include "nsIArray.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsITimelineService.h"
#include "nsDOMScriptObjectHolder.h"
#include "prmem.h"

#ifdef NS_DEBUG
#include "nsGlobalWindow.h"
#endif

#ifdef MOZ_JSDEBUGGER
#include "jsdIDebuggerService.h"
#endif
#ifdef MOZ_LOGGING

#define FORCE_PR_LOG 1
#endif
#include "prlog.h"
#include "prthread.h"

const size_t gStackSize = 8192;

#ifdef PR_LOGGING
static PRLogModuleInfo* gJSDiagnostics;
#endif


#ifndef WINCE
#ifdef CompareString
#undef CompareString
#endif
#endif 



#define NS_GC_DELAY                 2000 // ms





#define NS_LOAD_IN_PROCESS_GC_DELAY 4000 // ms



#define NS_FIRST_GC_DELAY           10000 // ms

#define JAVASCRIPT nsIProgrammingLanguage::JAVASCRIPT


#define NS_MAX_DELAYED_CCOLLECT     45


#define NS_CC_SOFT_LIMIT_INACTIVE   6


#define NS_CC_SOFT_LIMIT_ACTIVE     12


#define NS_PROBABILITY_MULTIPLIER   3



#define NS_MIN_CC_INTERVAL          10000 // ms


#define NS_COLLECTED_OBJECTS_LIMIT  5000


#define NS_MAX_GC_COUNT             5
#define NS_MIN_SUSPECT_CHANGES      10


#define NS_MAX_SUSPECT_CHANGES      100



static PRUint32 sDelayedCCollectCount;
static PRUint32 sCCollectCount;
static PRBool sUserIsActive;
static PRTime sPreviousCCTime;
static PRUint32 sCollectedObjectsCounts;
static PRUint32 sSavedGCCount;
static PRUint32 sCCSuspectChanges;
static PRUint32 sCCSuspectedCount;
static nsITimer *sGCTimer;
static PRBool sReadyForGC;







static PRUint32 sPendingLoadCount;




static PRBool sLoadInProgressGCTimer;

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

static nsICollation *gCollation;

static nsIUnicodeDecoder *gDecoder;











class nsUserActivityObserver : public nsIObserver
{
public:
  nsUserActivityObserver()
  : mUserActivityCounter(0), mOldCCollectCount(0) {}
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
private:
  PRUint32 mUserActivityCounter;
  PRUint32 mOldCCollectCount;
};

NS_IMPL_ISUPPORTS1(nsUserActivityObserver, nsIObserver)

NS_IMETHODIMP
nsUserActivityObserver::Observe(nsISupports* aSubject, const char* aTopic,
                                const PRUnichar* aData)
{
  if (mOldCCollectCount != sCCollectCount) {
    mOldCCollectCount = sCCollectCount;
    
    
    mUserActivityCounter = 0;
  }
  PRBool higherProbability = PR_FALSE;
  ++mUserActivityCounter;
  if (!strcmp(aTopic, "user-interaction-inactive")) {
#ifdef DEBUG_smaug
    printf("user-interaction-inactive\n");
#endif
    if (sUserIsActive) {
      sUserIsActive = PR_FALSE;
      if (!sGCTimer) {
        nsJSContext::IntervalCC();
        return NS_OK;
      }
    }
    higherProbability = (mUserActivityCounter > NS_CC_SOFT_LIMIT_INACTIVE);
  } else if (!strcmp(aTopic, "user-interaction-active")) {
#ifdef DEBUG_smaug
    printf("user-interaction-active\n");
#endif
    sUserIsActive = PR_TRUE;
    higherProbability = (mUserActivityCounter > NS_CC_SOFT_LIMIT_ACTIVE);
  } else if (!strcmp(aTopic, "xpcom-shutdown")) {
    nsCOMPtr<nsIObserverService> obs =
      do_GetService("@mozilla.org/observer-service;1");
    if (obs) {
      obs->RemoveObserver(this, "user-interaction-active");
      obs->RemoveObserver(this, "user-interaction-inactive");
      obs->RemoveObserver(this, "xpcom-shutdown");
    }
    return NS_OK;
  }
  nsJSContext::MaybeCC(higherProbability);
  return NS_OK;
}




class nsCCMemoryPressureObserver : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS1(nsCCMemoryPressureObserver, nsIObserver)

NS_IMETHODIMP
nsCCMemoryPressureObserver::Observe(nsISupports* aSubject, const char* aTopic,
                                    const PRUnichar* aData)
{
  nsJSContext::CC();
  return NS_OK;
}

class nsJSVersionSetter {
public:
  nsJSVersionSetter(JSContext *aContext, PRUint32 aVersion);
  ~nsJSVersionSetter();

private:
  JSContext* mContext;
  uint32 mOldOptions;
  JSVersion mOldVersion;
  JSBool mOptionsChanged;
};

nsJSVersionSetter::nsJSVersionSetter(JSContext *aContext, PRUint32 aVersion)
  : mContext(aContext)
{
  
  
  
  JSBool hasxml = (aVersion & JSVERSION_HAS_XML) != 0;
  mOldOptions = ::JS_GetOptions(mContext);
  mOptionsChanged = ((hasxml) ^ !!(mOldOptions & JSOPTION_XML));

  if (mOptionsChanged) {
    ::JS_SetOptions(mContext,
                    hasxml
                    ? mOldOptions | JSOPTION_XML
                    : mOldOptions & ~JSOPTION_XML);
  }

  
  
  JSVersion newVer = (JSVersion)(aVersion & JSVERSION_MASK);
  mOldVersion = ::JS_SetVersion(mContext, newVer);
}

nsJSVersionSetter::~nsJSVersionSetter()
{
  ::JS_SetVersion(mContext, mOldVersion);

  if (mOptionsChanged) {
      ::JS_SetOptions(mContext, mOldOptions);
  }
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

class AutoFreeJSStack {
public:
  AutoFreeJSStack(JSContext *ctx, void *aPtr) : mContext(ctx), mStack(aPtr) {
  }
  JS_REQUIRES_STACK ~AutoFreeJSStack() {
    if (mContext && mStack)
      js_FreeStack(mContext, mStack);
  }
private:
  JSContext *mContext;
  void *mStack;
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
    nsCOMPtr<nsPresContext> presContext;
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





void
NS_ScriptErrorReporter(JSContext *cx,
                       const char *message,
                       JSErrorReport *report)
{
  
  
  if (!JSREPORT_IS_WARNING(report->flags)) {
    JSStackFrame * fp = nsnull;
    while ((fp = JS_FrameIterator(cx, &fp))) {
      if (!JS_IsNativeFrame(cx, fp)) {
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

  nsEventStatus status = nsEventStatus_eIgnore;

  
  
  ::JS_ClearPendingException(cx);

  if (context) {
    nsIScriptGlobalObject *globalObject = context->GetGlobalObject();

    if (globalObject) {
      nsAutoString fileName, msg;
      NS_NAMED_LITERAL_STRING(xoriginMsg, "Script error.");

      fileName.AssignWithConversion(report->filename);

      const PRUnichar *m = reinterpret_cast<const PRUnichar*>
                                             (report->ucmessage);
      if (m) {
        msg.Assign(m);
      }

      if (msg.IsEmpty() && message) {
        msg.AssignWithConversion(message);
      }

      
      





      {
        
        
        
        nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(globalObject));
        nsIDocShell *docShell = win ? win->GetDocShell() : nsnull;
        if (docShell &&
            (report->errorNumber != JSMSG_OUT_OF_MEMORY &&
              !JSREPORT_IS_WARNING(report->flags))) {
          static PRInt32 errorDepth; 
          ++errorDepth;

          nsCOMPtr<nsPresContext> presContext;
          docShell->GetPresContext(getter_AddRefs(presContext));

          if (presContext && errorDepth < 2) {
            nsScriptErrorEvent errorevent(PR_TRUE, NS_LOAD_ERROR);

            errorevent.fileName = fileName.get();

            nsCOMPtr<nsIScriptObjectPrincipal> sop(do_QueryInterface(win));
            nsIPrincipal *p = sop->GetPrincipal();

            PRBool sameOrigin = (report->filename == nsnull);

            if (p && !sameOrigin) {
              nsCOMPtr<nsIURI> errorURI;
              NS_NewURI(getter_AddRefs(errorURI), report->filename);

              if (errorURI) {
                
                
                
                
                sameOrigin = NS_SUCCEEDED(p->CheckMayLoad(errorURI, PR_FALSE));
              }
            }

            if (sameOrigin) {
              errorevent.errorMsg = msg.get();
              errorevent.lineNr = report->lineno;
            } else {
              errorevent.errorMsg = xoriginMsg.get();
              errorevent.lineNr = 0;
            }

            
            
            nsEventDispatcher::Dispatch(win, presContext, &errorevent, nsnull,
                                        &status);
          }

          --errorDepth;
        }
      }

      if (status != nsEventStatus_eConsumeNoDefault) {
        
        
        nsCOMPtr<nsIScriptError> errorObject =
          do_CreateInstance("@mozilla.org/scripterror;1");

        if (errorObject != nsnull) {
          nsresult rv = NS_ERROR_NOT_AVAILABLE;

          
          nsCOMPtr<nsIScriptObjectPrincipal> scriptPrincipal =
            do_QueryInterface(globalObject);
          NS_ASSERTION(scriptPrincipal, "Global objects must implement "
                       "nsIScriptObjectPrincipal");
          nsCOMPtr<nsIPrincipal> systemPrincipal;
          sSecurityManager->GetSystemPrincipal(getter_AddRefs(systemPrincipal));
          const char * category =
            scriptPrincipal->GetPrincipal() == systemPrincipal
            ? "chrome javascript"
            : "content javascript";

          PRUint32 column = report->uctokenptr - report->uclinebuf;

          rv = errorObject->Init(msg.get(), fileName.get(),
                                 reinterpret_cast<const PRUnichar*>
                                                 (report->uclinebuf),
                                 report->lineno, column, report->flags,
                                 category);

          if (NS_SUCCEEDED(rv)) {
            nsCOMPtr<nsIConsoleService> consoleService =
              do_GetService(NS_CONSOLESERVICE_CONTRACTID, &rv);
            if (NS_SUCCEEDED(rv)) {
              consoleService->LogMessage(errorObject);
            }
          }
        }
      }
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
  if (status != nsEventStatus_eIgnore && !JSREPORT_IS_WARNING(report->flags))
    error.Append(" Error was suppressed by event handler\n");
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

static JSBool
LocaleToUnicode(JSContext *cx, char *src, jsval *rval)
{
  nsresult rv;

  if (!gDecoder) {
    
    nsCOMPtr<nsILocaleService> localeService =
      do_GetService(NS_LOCALESERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsILocale> appLocale;
      rv = localeService->GetApplicationLocale(getter_AddRefs(appLocale));
      if (NS_SUCCEEDED(rv)) {
        nsAutoString localeStr;
        rv = appLocale->
          GetCategory(NS_LITERAL_STRING(NSILOCALE_TIME), localeStr);
        NS_ASSERTION(NS_SUCCEEDED(rv), "failed to get app locale info");

        nsCOMPtr<nsIPlatformCharset> platformCharset =
          do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);

        if (NS_SUCCEEDED(rv)) {
          nsCAutoString charset;
          rv = platformCharset->GetDefaultCharsetForLocale(localeStr, charset);
          if (NS_SUCCEEDED(rv)) {
            
            nsCOMPtr<nsICharsetConverterManager> ccm =
              do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
            if (NS_SUCCEEDED(rv))
              ccm->GetUnicodeDecoder(charset.get(), &gDecoder);
          }
        }
      }
    }
  }

  JSString *str = nsnull;
  PRInt32 srcLength = PL_strlen(src);

  if (gDecoder) {
    PRInt32 unicharLength = srcLength;
    PRUnichar *unichars =
      (PRUnichar *)JS_malloc(cx, (srcLength + 1) * sizeof(PRUnichar));
    if (unichars) {
      rv = gDecoder->Convert(src, &srcLength, unichars, &unicharLength);
      if (NS_SUCCEEDED(rv)) {
        
        unichars[unicharLength] = 0;

        
        if (unicharLength + 1 < srcLength + 1) {
          PRUnichar *shrunkUnichars =
            (PRUnichar *)JS_realloc(cx, unichars,
                                    (unicharLength + 1) * sizeof(PRUnichar));
          if (shrunkUnichars)
            unichars = shrunkUnichars;
        }
        str = JS_NewUCString(cx,
                             reinterpret_cast<jschar*>(unichars),
                             unicharLength);
      }
      if (!str)
        JS_free(cx, unichars);
    }
  }

  if (!str) {
    nsDOMClassInfo::ThrowJSException(cx, NS_ERROR_OUT_OF_MEMORY);
    return JS_FALSE;
  }

  *rval = STRING_TO_JSVAL(str);
  return JS_TRUE;
}


static JSBool
ChangeCase(JSContext *cx, JSString *src, jsval *rval,
           void(* changeCaseFnc)(const nsAString&, nsAString&))
{
  nsAutoString result;
  changeCaseFnc(nsDependentJSString(src), result);

  JSString *ucstr = JS_NewUCStringCopyN(cx, (jschar*)result.get(), result.Length());
  if (!ucstr) {
    return JS_FALSE;
  }

  *rval = STRING_TO_JSVAL(ucstr);

  return JS_TRUE;
}

static JSBool
LocaleToUpperCase(JSContext *cx, JSString *src, jsval *rval)
{
  return ChangeCase(cx, src, rval, ToUpperCase);
}

static JSBool
LocaleToLowerCase(JSContext *cx, JSString *src, jsval *rval)
{
  return ChangeCase(cx, src, rval, ToLowerCase);
}

static JSBool
LocaleCompare(JSContext *cx, JSString *src1, JSString *src2, jsval *rval)
{
  nsresult rv;

  if (!gCollation) {
    nsCOMPtr<nsILocaleService> localeService =
      do_GetService(NS_LOCALESERVICE_CONTRACTID, &rv);

    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsILocale> locale;
      rv = localeService->GetApplicationLocale(getter_AddRefs(locale));

      if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsICollationFactory> colFactory =
          do_CreateInstance(NS_COLLATIONFACTORY_CONTRACTID, &rv);

        if (NS_SUCCEEDED(rv)) {
          rv = colFactory->CreateCollation(locale, &gCollation);
        }
      }
    }

    if (NS_FAILED(rv)) {
      nsDOMClassInfo::ThrowJSException(cx, rv);

      return JS_FALSE;
    }
  }

  PRInt32 result;
  rv = gCollation->CompareString(nsICollation::kCollationStrengthDefault,
                                 nsDependentJSString(src1),
                                 nsDependentJSString(src2),
                                 &result);

  if (NS_FAILED(rv)) {
    nsDOMClassInfo::ThrowJSException(cx, rv);

    return JS_FALSE;
  }

  *rval = INT_TO_JSVAL(result);

  return JS_TRUE;
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
#endif

static void
MaybeGC(JSContext *cx)
{
  size_t bytes = cx->runtime->gcBytes;
  size_t lastBytes = cx->runtime->gcLastBytes;
  if ((bytes > 8192 && bytes > lastBytes * 16)
#ifdef DEBUG
      || cx->runtime->gcZeal > 0
#endif
      ) {
    JS_GC(cx);
  }
}

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

  MaybeGC(cx);

  
  ctx->mOperationCallbackTime = callbackTime;
  ctx->mModalStateTime = modalStateTime;

  
  nsCOMPtr<nsIMemory> mem;
  NS_GetMemoryManager(getter_AddRefs(mem));
  if (!mem) {
    JS_ClearPendingException(cx);
    return JS_FALSE;
  }

  PRBool lowMemory;
  mem->IsLowMemory(&lowMemory);
  if (lowMemory) {
    
    nsJSContext::CC();

    
    if (!::JS_IsSystemObject(cx, ::JS_GetGlobalObject(cx))) {

      
      mem->IsLowMemory(&lowMemory);
      if (lowMemory) {

        if (nsContentUtils::GetBoolPref("dom.prevent_oom_dialog", PR_FALSE)) {
          JS_ClearPendingException(cx);
          return JS_FALSE;
        }
        
        nsCOMPtr<nsIPrompt> prompt = GetPromptFromContext(ctx);
        
        nsXPIDLString title, msg;
        rv = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                "LowMemoryTitle",
                                                title);
        
        rv |= nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                 "LowMemoryMessage",
                                                 msg);
        
        
        if (NS_FAILED(rv) || !title || !msg) {
          NS_ERROR("Failed to get localized strings.");
          JS_ClearPendingException(cx);
          return JS_FALSE;
        }
        
        prompt->Alert(title, msg);
        JS_ClearPendingException(cx);
        return JS_FALSE;
      }
    }
  }

  PRTime now = PR_Now();

  if (callbackTime == 0) {
    
    
    ctx->mOperationCallbackTime = now;
    return JS_TRUE;
  }

  if (ctx->mModalStateDepth) {
    

    return JS_TRUE;
  }

  PRTime duration = now - callbackTime;

  
  
  PRBool isTrackingChromeCodeTime =
    ::JS_IsSystemObject(cx, ::JS_GetGlobalObject(cx));
  if (duration < (isTrackingChromeCodeTime ?
                  sMaxChromeScriptRunTime : sMaxScriptRunTime)) {
    return JS_TRUE;
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
      if (jsds_IsOn) { 
        rv = jsds->OnForRuntime(cx->runtime);
        jsds_IsOn = NS_SUCCEEDED(rv);
      }
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

  PRInt32 buttonPressed = 1; 
  PRBool neverShowDlgChk = PR_FALSE;
  PRUint32 buttonFlags = (nsIPrompt::BUTTON_TITLE_IS_STRING *
                          (nsIPrompt::BUTTON_POS_0 + nsIPrompt::BUTTON_POS_1));

  
  if (debugPossible)
    buttonFlags += nsIPrompt::BUTTON_TITLE_IS_STRING * nsIPrompt::BUTTON_POS_2;

  
  ::JS_SetOperationCallback(cx, nsnull);

  
  rv = prompt->ConfirmEx(title, msg, buttonFlags, stopButton, waitButton,
                         debugButton, neverShowDlg, &neverShowDlgChk,
                         &buttonPressed);

  ::JS_SetOperationCallback(cx, DOMOperationCallback);

  if (NS_FAILED(rv) || (buttonPressed == 1)) {
    

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
        fp->rval = rval;
        return JS_TRUE;
      case JSTRAP_ERROR:
        cx->throwing = JS_FALSE;
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
static const char js_werror_option_str[] = JS_OPTIONS_DOT_STR "werror";
static const char js_relimit_option_str[]= JS_OPTIONS_DOT_STR "relimit";
#ifdef JS_GC_ZEAL
static const char js_zeal_option_str[]   = JS_OPTIONS_DOT_STR "gczeal";
#endif
static const char js_jit_content_str[]   = JS_OPTIONS_DOT_STR "jit.content";
static const char js_jit_chrome_str[]    = JS_OPTIONS_DOT_STR "jit.chrome";

int
nsJSContext::JSOptionChangedCallback(const char *pref, void *data)
{
  nsJSContext *context = reinterpret_cast<nsJSContext *>(data);
  PRUint32 oldDefaultJSOptions = context->mDefaultJSOptions;
  PRUint32 newDefaultJSOptions = oldDefaultJSOptions;

  PRBool strict = nsContentUtils::GetBoolPref(js_strict_option_str);
  if (strict)
    newDefaultJSOptions |= JSOPTION_STRICT;
  else
    newDefaultJSOptions &= ~JSOPTION_STRICT;

  nsIScriptGlobalObject *global = context->GetGlobalObject();
  
  
  nsCOMPtr<nsIDOMChromeWindow> chromeWindow(do_QueryInterface(global));

  PRBool useJIT = nsContentUtils::GetBoolPref(chromeWindow ?
                                              js_jit_chrome_str :
                                              js_jit_content_str);
  nsCOMPtr<nsIXULRuntime> xr = do_GetService(XULRUNTIME_SERVICE_CONTRACTID);
  if (xr) {
    PRBool safeMode = PR_FALSE;
    xr->GetInSafeMode(&safeMode);
    if (safeMode)
      useJIT = PR_FALSE;
  }    

  if (useJIT)
    newDefaultJSOptions |= JSOPTION_JIT;
  else
    newDefaultJSOptions &= ~JSOPTION_JIT;

#ifdef DEBUG
  
  
  
  if ((newDefaultJSOptions & JSOPTION_STRICT) == 0) {
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

  if (newDefaultJSOptions != oldDefaultJSOptions) {
    
    
    if (::JS_GetOptions(context->mContext) == oldDefaultJSOptions)
      ::JS_SetOptions(context->mContext, newDefaultJSOptions);

    
    context->mDefaultJSOptions = newDefaultJSOptions;
  }

#ifdef JS_GC_ZEAL
  PRInt32 zeal = nsContentUtils::GetIntPref(js_zeal_option_str, -1);
  if (zeal >= 0)
    ::JS_SetGCZeal(context->mContext, (PRUint8)zeal);
#endif

  return 0;
}

nsJSContext::nsJSContext(JSRuntime *aRuntime) : mGCOnDestruction(PR_TRUE)
{

  ++sContextCount;

  mDefaultJSOptions = JSOPTION_PRIVATE_IS_NSISUPPORTS | JSOPTION_ANONFUNFIX;

  mContext = ::JS_NewContext(aRuntime, gStackSize);
  if (mContext) {
    ::JS_SetContextPrivate(mContext, static_cast<nsIScriptContext *>(this));

    
    ::JS_SetOptions(mContext, mDefaultJSOptions);

    
    nsContentUtils::RegisterPrefCallback(js_options_dot_str,
                                         JSOptionChangedCallback,
                                         this);

    ::JS_SetOperationCallback(mContext, DOMOperationCallback);

    static JSLocaleCallbacks localeCallbacks =
      {
        LocaleToUpperCase,
        LocaleToLowerCase,
        LocaleCompare,
        LocaleToUnicode
      };

    ::JS_SetLocaleCallbacks(mContext, &localeCallbacks);
  }
  mIsInitialized = PR_FALSE;
  mNumEvaluations = 0;
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
  NS_PRECONDITION(!mTerminations, "Shouldn't have termination funcs by now");

  mGlobalWrapperRef = nsnull;

  DestroyJSContext();

  --sContextCount;

  if (!sContextCount && sDidShutdown) {
    
    
    

    NS_IF_RELEASE(sRuntimeService);
    NS_IF_RELEASE(sSecurityManager);
    NS_IF_RELEASE(gCollation);
    NS_IF_RELEASE(gDecoder);
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

  PRBool do_gc = mGCOnDestruction && !sGCTimer && sReadyForGC;

  
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
NS_IMPL_CYCLE_COLLECTION_ROOT_BEGIN(nsJSContext)
  NS_ASSERTION(!tmp->mContext || tmp->mContext->outstandingRequests == 0,
               "Trying to unlink a context with outstanding requests.");
  tmp->mIsInitialized = PR_FALSE;
  tmp->mGCOnDestruction = PR_FALSE;
  tmp->DestroyJSContext();
NS_IMPL_CYCLE_COLLECTION_ROOT_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsJSContext)
NS_IMPL_CYCLE_COLLECTION_TRACE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsJSContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mGlobalWrapperRef)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_REFCNT(nsJSContext, tmp->GetCCRefcnt())
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mGlobalWrapperRef)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mContext");
  nsContentUtils::XPConnect()->NoteJSContext(tmp->mContext, cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsJSContext)
  NS_INTERFACE_MAP_ENTRY(nsIScriptContext)
  NS_INTERFACE_MAP_ENTRY(nsIXPCScriptNotify)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIScriptContext)
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsJSContext, nsIScriptContext)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsJSContext, nsIScriptContext)

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

  nsJSContext::TerminationFuncHolder holder(this);

  
  
  
  if (ok && ((JSVersion)aVersion) != JSVERSION_UNKNOWN) {

    JSAutoRequest ar(mContext);
    nsJSVersionSetter setVersion(mContext, aVersion);

    ok = ::JS_EvaluateUCScriptForPrincipals(mContext,
                                            (JSObject *)aScopeObject,
                                            jsprin,
                                            (jschar*)PromiseFlatString(aScript).get(),
                                            aScript.Length(),
                                            aURL,
                                            aLineNo,
                                            &val);

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
  if (jsstring) {
    result->Assign(reinterpret_cast<const PRUnichar*>
                                   (::JS_GetStringChars(jsstring)),
                   ::JS_GetStringLength(jsstring));
  } else {
    result->Truncate();

    
    
    

    if (isUndefined) {
      *isUndefined = PR_TRUE;
    }

    if (!::JS_IsExceptionPending(cx)) {
      
      

      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return NS_OK;
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

  nsJSContext::TerminationFuncHolder holder(this);

  
  
  
  if (ok && ((JSVersion)aVersion) != JSVERSION_UNKNOWN) {
    JSAutoRequest ar(mContext);
    nsJSVersionSetter setVersion(mContext, aVersion);

    ok = ::JS_EvaluateUCScriptForPrincipals(mContext,
                                            (JSObject *)aScopeObject,
                                            jsprin,
                                            (jschar*)PromiseFlatString(aScript).get(),
                                            aScript.Length(),
                                            aURL,
                                            aLineNo,
                                            vp);

    if (!ok) {
      
      
      

      ReportPendingException();
    }
  }

  
  JSPRINCIPALS_DROP(mContext, jsprin);

  
  if (ok) {
    JSAutoRequest ar(mContext);
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
    nsJSVersionSetter setVersion(mContext, aVersion);

    JSScript* script =
        ::JS_CompileUCScriptForPrincipals(mContext,
                                          (JSObject *)aScopeObject,
                                          jsprin,
                                          (jschar*) aText,
                                          aTextLength,
                                          aURL,
                                          aLineNo);
    if (script) {
      JSObject *scriptObject = ::JS_NewScriptObject(mContext, script);
      if (scriptObject) {
        NS_ASSERTION(aScriptObject.getScriptTypeID()==JAVASCRIPT,
                     "Expecting JS script object holder");
        rv = aScriptObject.set(scriptObject);
      } else {
        ::JS_DestroyScript(mContext, script);
        script = nsnull;
      }
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

  nsJSContext::TerminationFuncHolder holder(this);
  JSAutoRequest ar(mContext);
  ok = ::JS_ExecuteScript(mContext,
                          (JSObject *)aScopeObject,
                          (JSScript*)::JS_GetPrivate(mContext,
                          (JSObject*)aScriptObject),
                          &val);

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

  
  if (NS_FAILED(stack->Pop(nsnull)))
    rv = NS_ERROR_FAILURE;

  
  ScriptEvaluated(PR_TRUE);

  return rv;
}


static inline const char *
AtomToEventHandlerName(nsIAtom *aName)
{
  const char *name;

  aName->GetUTF8String(&name);

#ifdef DEBUG
  const char *cp;
  char c;
  for (cp = name; *cp != '\0'; ++cp)
  {
    c = *cp;
    NS_ASSERTION (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'),
                  "non-ASCII non-alphabetic event handler name");
  }
#endif

  return name;
}



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
  NS_ENSURE_TRUE(mIsInitialized, NS_ERROR_NOT_INITIALIZED);

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

  const char *charName = AtomToEventHandlerName(aName);

#ifdef DEBUG
  JSContext* top = nsContentUtils::GetCurrentJSContext();
  NS_ASSERTION(mContext == top, "Context not properly pushed!");
#endif

  
  
  
  JSAutoRequest ar(mContext);
  nsJSVersionSetter setVersion(mContext, aVersion);

  JSFunction* fun =
      ::JS_CompileUCFunctionForPrincipals(mContext,
                                          nsnull, nsnull,
                                          charName, aArgCount, aArgNames,
                                          (jschar*)PromiseFlatString(aBody).get(),
                                          aBody.Length(),
                                          aURL, aLineNo);

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
  nsJSVersionSetter setVersion(mContext, aVersion);

  JSFunction* fun =
      ::JS_CompileUCFunctionForPrincipals(mContext,
                                          aShared ? nsnull : target, jsprin,
                                          PromiseFlatCString(aName).get(),
                                          aArgCount, aArgArray,
                                          (jschar*)PromiseFlatString(aBody).get(),
                                          aBody.Length(),
                                          aURL, aLineNo);

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

  jsval targetVal = JSVAL_VOID;
  JSAutoTempValueRooter tvr(mContext, 1, &targetVal);

  JSObject* target = nsnull;
  nsresult rv = JSObjectFromInterface(aTarget, aScope, &target);
  NS_ENSURE_SUCCESS(rv, rv);

  targetVal = OBJECT_TO_JSVAL(target);

  jsval rval = JSVAL_VOID;

  
  
  
  

  nsCOMPtr<nsIJSContextStack> stack =
    do_GetService("@mozilla.org/js/xpc/ContextStack;1", &rv);
  if (NS_FAILED(rv) || NS_FAILED(stack->Push(mContext)))
    return NS_ERROR_FAILURE;

  
  rv = sSecurityManager->CheckFunctionAccess(mContext, aHandler, target);

  nsJSContext::TerminationFuncHolder holder(this);

  if (NS_SUCCEEDED(rv)) {
    
    void *mark;
    PRUint32 argc = 0;
    jsval *argv = nsnull;

    
    
    
    
    
    rv = ConvertSupportsTojsvals(aargv, target, &argc,
                                 reinterpret_cast<void **>(&argv), &mark);
    if (NS_FAILED(rv)) {
      stack->Pop(nsnull);
      return rv;
    }

    AutoFreeJSStack stackGuard(mContext, mark); 

    jsval funval = OBJECT_TO_JSVAL(static_cast<JSObject *>(aHandler));
    JSAutoRequest ar(mContext);
    PRBool ok = ::JS_CallFunctionValue(mContext, target,
                                       funval, argc, argv, &rval);

    if (!ok) {
      
      
      

      ReportPendingException();

      
      rval = JSVAL_VOID;

      
      rv = NS_ERROR_FAILURE;
    }
  }

  if (NS_FAILED(stack->Pop(nsnull)))
    return NS_ERROR_FAILURE;

  
  
  JSAutoRequest ar(mContext);
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

  const char *charName = AtomToEventHandlerName(aName);
  nsresult rv;

  
  JSObject *target = nsnull;
  nsAutoGCRoot root(&target, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = JSObjectFromInterface(aTarget, aScope, &target);
  NS_ENSURE_SUCCESS(rv, rv);

  JSObject *funobj = (JSObject*) aHandler;

  JSAutoRequest ar(mContext);

  NS_ASSERTION(JS_TypeOfValue(mContext, OBJECT_TO_JSVAL(funobj)) == JSTYPE_FUNCTION,
               "Event handler object not a function");

  
  
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
      
      !::JS_DefineProperty(mContext, target, charName,
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
    nsresult rv;
    JSObject *obj = nsnull;
    nsAutoGCRoot root(&obj, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    JSAutoRequest ar(mContext);
    rv = JSObjectFromInterface(aTarget, aScope, &obj);
    NS_ENSURE_SUCCESS(rv, rv);

    const char *charName = AtomToEventHandlerName(aName);

    jsval funval;
    if (!JS_LookupProperty(mContext, obj,
                           charName, &funval))
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
    JSScript *script = reinterpret_cast<JSScript*>
                                       (::JS_GetPrivate(cx, mJSObject));
    if (! ::JS_XDRScript(xdr, &script)) {
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

        JSScript *script = nsnull;
        if (! ::JS_XDRScript(xdr, &script)) {
            rv = NS_ERROR_FAILURE;  
        } else {
            result = ::JS_NewScriptObject(cx, script);
            if (! result) {
                rv = NS_ERROR_OUT_OF_MEMORY;    
                ::JS_DestroyScript(cx, script);
            }
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
    NS_WARNING("Context has no global.");
    return nsnull;
  }

  JSClass *c = JS_GET_CLASS(mContext, global);

  if (!c || ((~c->flags) & (JSCLASS_HAS_PRIVATE |
                            JSCLASS_PRIVATE_IS_NSISUPPORTS))) {
    return nsnull;
  }

  nsCOMPtr<nsIScriptGlobalObject> sgo;
  nsISupports *priv =
    (nsISupports *)::JS_GetPrivate(mContext, global);

  nsCOMPtr<nsIXPConnectWrappedNative> wrapped_native =
    do_QueryInterface(priv);

  if (wrapped_native) {
    
    

    sgo = do_QueryWrappedNative(wrapped_native);
  } else {
    sgo = do_QueryInterface(priv);
  }

  
  
  return sgo;
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
                                void **aNativeGlobal, nsISupports **aHolder)
{
  nsIXPConnect *xpc = nsContentUtils::XPConnect();
  PRUint32 flags = aIsChrome? nsIXPConnect::FLAG_SYSTEM_GLOBAL_OBJECT : 0;
  nsCOMPtr<nsIXPConnectJSObjectHolder> jsholder;
  nsresult rv = xpc->
          InitClassesWithNewWrappedGlobal(mContext,
                                          aNewInner, NS_GET_IID(nsISupports),
                                          flags,
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
  JSObject *myobject = (JSObject *)aOuterGlobal;

  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  JSObject *proto = ::JS_GetPrototype(mContext, myobject);
  JSObject *innerProto = ::JS_GetPrototype(mContext, newInnerJSObject);
  JSObject *innerProtoProto = ::JS_GetPrototype(mContext, innerProto);

  ::JS_SetPrototype(mContext, newInnerJSObject, proto);
  ::JS_SetPrototype(mContext, proto, innerProtoProto);
  return NS_OK;
}

void *
nsJSContext::GetNativeContext()
{
  return mContext;
}

nsresult
nsJSContext::InitContext(nsIScriptGlobalObject *aGlobalObject)
{
  
  
  NS_ENSURE_TRUE(!mIsInitialized, NS_ERROR_ALREADY_INITIALIZED);

  if (!mContext)
    return NS_ERROR_OUT_OF_MEMORY;

  ::JS_SetErrorReporter(mContext, NS_ScriptErrorReporter);

  if (!aGlobalObject) {
    

    return NS_OK;
  }

  nsCxPusher cxPusher;
  if (!cxPusher.Push(mContext)) {
    return NS_ERROR_FAILURE;
  }

  nsIXPConnect *xpc = nsContentUtils::XPConnect();

  JSObject *global = ::JS_GetGlobalObject(mContext);

  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;

  
  

  nsresult rv;

  if (!global) {
    nsCOMPtr<nsIDOMChromeWindow> chromeWindow(do_QueryInterface(aGlobalObject));
    PRUint32 flags = 0;

    if (chromeWindow) {
      
      
      
      flags = nsIXPConnect::FLAG_SYSTEM_GLOBAL_OBJECT;

      
      
      
      
      ::JS_SetOptions(mContext, ::JS_GetOptions(mContext) | JSOPTION_XML);
    }

    rv = xpc->InitClassesWithNewWrappedGlobal(mContext, aGlobalObject,
                                              NS_GET_IID(nsISupports),
                                              flags,
                                              getter_AddRefs(holder));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (!nsDOMClassInfo::GetXPCNativeWrapperClass()) {
      JSAutoRequest ar(mContext);
      rv = FindXPCNativeWrapperClass(holder);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else {
    
    
    

    
    
    
    ::JS_ClearScope(mContext, global);

    
    
    rv = xpc->InitClassesForOuterObject(mContext, global);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIClassInfo> ci(do_QueryInterface(aGlobalObject));

    if (ci) {
      jsval v;
      rv = nsContentUtils::WrapNative(mContext, global, aGlobalObject, &v,
                                      getter_AddRefs(holder));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIXPConnectWrappedNative> wrapper(do_QueryInterface(holder));
      NS_ENSURE_TRUE(wrapper, NS_ERROR_FAILURE);

      rv = wrapper->RefreshPrototype();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  
  
  mGlobalWrapperRef = holder;

  holder->GetJSObject(&global);

  rv = InitClasses(global); 
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
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
  void *mark;

  JSAutoRequest ar(mContext);

  nsresult rv;
  rv = ConvertSupportsTojsvals(aArgs, GetNativeGlobal(), &argc,
                               reinterpret_cast<void **>(&argv), &mark);
  NS_ENSURE_SUCCESS(rv, rv);
  AutoFreeJSStack stackGuard(mContext, mark); 

  jsval vargs;

  

  
  
  if (strcmp(aPropName, "dialogArguments") == 0 && argc <= 1) {
    vargs = argc ? argv[0] : JSVAL_VOID;
  } else {
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
                                     PRUint32 *aArgc, void **aArgv,
                                     void **aMarkp)
{
  nsresult rv = NS_OK;

  js_LeaveTrace(mContext);

  
  nsCOMPtr<nsIJSArgArray> fastArray = do_QueryInterface(aArgs);
  if (fastArray != nsnull) {
    *aMarkp = nsnull;
    return fastArray->GetArgs(aArgc, aArgv);
  }
  
  
  

  *aArgv = nsnull;
  *aArgc = 0;
  *aMarkp = nsnull;

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

  jsval *argv = js_AllocStack(mContext, argCount, aMarkp);
  NS_ENSURE_TRUE(argv, NS_ERROR_OUT_OF_MEMORY);

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
  if (NS_FAILED(rv)) {
    js_FreeStack(mContext, *aMarkp);
    return rv;
  }
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

nsresult
nsJSContext::FindXPCNativeWrapperClass(nsIXPConnectJSObjectHolder *aHolder)
{
  NS_ASSERTION(!nsDOMClassInfo::GetXPCNativeWrapperClass(),
               "Why was this called?");

  JSObject *globalObj;
  aHolder->GetJSObject(&globalObj);
  NS_ASSERTION(globalObj, "Must have global by now!");

  const char* arg = "arg";
  NS_NAMED_LITERAL_STRING(body, "return new XPCNativeWrapper(arg);");

  
  
  JSFunction *fun =
    ::JS_CompileUCFunction(mContext,
                           globalObj,
                           "_XPCNativeWrapperCtor",
                           1, &arg,
                           (jschar*)body.get(),
                           body.Length(),
                           "javascript:return new XPCNativeWrapper(arg);",
                           1 
                           );
  NS_ENSURE_TRUE(fun, NS_ERROR_FAILURE);

  jsval globalVal = OBJECT_TO_JSVAL(globalObj);
  jsval wrapper;

  JSBool ok = ::JS_CallFunction(mContext, globalObj, fun,
                                1, &globalVal, &wrapper);
  if (!ok) {
    
    
    return NS_ERROR_FAILURE;
  }

  NS_ASSERTION(JSVAL_IS_OBJECT(wrapper), "This should be an object!");

  nsDOMClassInfo::SetXPCNativeWrapperClass(
    ::JS_GET_CLASS(mContext, JSVAL_TO_OBJECT(wrapper)));

  return NS_OK;
}

static JSPropertySpec OptionsProperties[] = {
  {"strict",    (int8)JSOPTION_STRICT,   JSPROP_ENUMERATE | JSPROP_PERMANENT},
  {"werror",    (int8)JSOPTION_WERROR,   JSPROP_ENUMERATE | JSPROP_PERMANENT},
  {"relimit",   (int8)JSOPTION_RELIMIT,  JSPROP_ENUMERATE | JSPROP_PERMANENT},
  {0}
};

static JSBool
GetOptionsProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  if (JSVAL_IS_INT(id)) {
    uint32 optbit = (uint32) JSVAL_TO_INT(id);
    if ((optbit & (optbit - 1)) == 0 && optbit <= JSOPTION_WERROR)
      *vp = (JS_GetOptions(cx) & optbit) ? JSVAL_TRUE : JSVAL_FALSE;
  }
  return JS_TRUE;
}

static JSBool
SetOptionsProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  if (JSVAL_IS_INT(id)) {
    uint32 optbit = (uint32) JSVAL_TO_INT(id);

    
    
    
    if (((optbit & (optbit - 1)) == 0 && optbit <= JSOPTION_WERROR) ||
        optbit == JSOPTION_RELIMIT) {
      JSBool optval;
      JS_ValueToBoolean(cx, *vp, &optval);

      uint32 optset = ::JS_GetOptions(cx);
      if (optval)
        optset |= optbit;
      else
        optset &= ~optbit;
      ::JS_SetOptions(cx, optset);
    }
  }
  return JS_TRUE;
}

static JSClass OptionsClass = {
  "JSOptions",
  0,
  JS_PropertyStub, JS_PropertyStub, GetOptionsProperty, SetOptionsProperty,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, nsnull
};

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
TraceMallocDisable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    NS_TraceMallocDisable();
    return JS_TRUE;
}

static JSBool
TraceMallocEnable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    NS_TraceMallocEnable();
    return JS_TRUE;
}

static JSBool
TraceMallocOpenLogFile(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int fd;
    JSString *str;
    char *filename;

    if (argc == 0) {
        fd = -1;
    } else {
        str = JS_ValueToString(cx, argv[0]);
        if (!str)
            return JS_FALSE;
        filename = JS_GetStringBytes(str);
        fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) {
            JS_ReportError(cx, "can't open %s: %s", filename, strerror(errno));
            return JS_FALSE;
        }
    }
    *rval = INT_TO_JSVAL(fd);
    return JS_TRUE;
}

static JSBool
TraceMallocChangeLogFD(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int32 fd, oldfd;

    if (argc == 0) {
        oldfd = -1;
    } else {
        if (!JS_ValueToECMAInt32(cx, argv[0], &fd))
            return JS_FALSE;
        oldfd = NS_TraceMallocChangeLogFD(fd);
        if (oldfd == -2) {
            JS_ReportOutOfMemory(cx);
            return JS_FALSE;
        }
    }
    *rval = INT_TO_JSVAL(oldfd);
    return JS_TRUE;
}

static JSBool
TraceMallocCloseLogFD(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int32 fd;

    if (argc == 0)
        return JS_TRUE;
    if (!JS_ValueToECMAInt32(cx, argv[0], &fd))
        return JS_FALSE;
    NS_TraceMallocCloseLogFD((int) fd);
    return JS_TRUE;
}

static JSBool
TraceMallocLogTimestamp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    const char *caption;

    str = JS_ValueToString(cx, argv[0]);
    if (!str)
        return JS_FALSE;
    caption = JS_GetStringBytes(str);
    NS_TraceMallocLogTimestamp(caption);
    return JS_TRUE;
}

static JSBool
TraceMallocDumpAllocations(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    const char *pathname;

    str = JS_ValueToString(cx, argv[0]);
    if (!str)
        return JS_FALSE;
    pathname = JS_GetStringBytes(str);
    if (NS_TraceMallocDumpAllocations(pathname) < 0) {
        JS_ReportError(cx, "can't dump to %s: %s", pathname, strerror(errno));
        return JS_FALSE;
    }
    return JS_TRUE;
}

static JSFunctionSpec TraceMallocFunctions[] = {
    {"TraceMallocDisable",         TraceMallocDisable,         0, 0, 0},
    {"TraceMallocEnable",          TraceMallocEnable,          0, 0, 0},
    {"TraceMallocOpenLogFile",     TraceMallocOpenLogFile,     1, 0, 0},
    {"TraceMallocChangeLogFD",     TraceMallocChangeLogFD,     1, 0, 0},
    {"TraceMallocCloseLogFD",      TraceMallocCloseLogFD,      1, 0, 0},
    {"TraceMallocLogTimestamp",    TraceMallocLogTimestamp,    1, 0, 0},
    {"TraceMallocDumpAllocations", TraceMallocDumpAllocations, 1, 0, 0},
    {nsnull,                       nsnull,                     0, 0, 0}
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
JProfStartProfilingJS(JSContext *cx, JSObject *obj,
                      uintN argc, jsval *argv, jsval *rval)
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
JProfStopProfilingJS(JSContext *cx, JSObject *obj,
                     uintN argc, jsval *argv, jsval *rval)
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
    {"JProfStartProfiling",        JProfStartProfilingJS,      0, 0, 0},
    {"JProfStopProfiling",         JProfStopProfilingJS,       0, 0, 0},
    {nsnull,                       nsnull,                     0, 0, 0}
};

#endif 

#ifdef MOZ_SHARK
static JSFunctionSpec SharkFunctions[] = {
    {"startShark",                 js_StartShark,              0, 0, 0},
    {"stopShark",                  js_StopShark,               0, 0, 0},
    {"connectShark",               js_ConnectShark,            0, 0, 0},
    {"disconnectShark",            js_DisconnectShark,         0, 0, 0},
    {nsnull,                       nsnull,                     0, 0, 0}
};
#endif

#ifdef MOZ_CALLGRIND
static JSFunctionSpec CallgrindFunctions[] = {
    {"startCallgrind",             js_StartCallgrind,          0, 0, 0},
    {"stopCallgrind",              js_StopCallgrind,           0, 0, 0},
    {"dumpCallgrind",              js_DumpCallgrind,           1, 0, 0},
    {nsnull,                       nsnull,                     0, 0, 0}
};
#endif

#ifdef MOZ_VTUNE
static JSFunctionSpec VtuneFunctions[] = {
    {"startVtune",                 js_StartVtune,              1, 0, 0},
    {"stopVtune",                  js_StopVtune,               0, 0, 0},
    {"pauseVtune",                 js_PauseVtune,              0, 0, 0},
    {"resumeVtune",                js_ResumeVtune,             0, 0, 0},
    {nsnull,                       nsnull,                     0, 0, 0}
};
#endif

#ifdef MOZ_TRACEVIS
static JSFunctionSpec EthogramFunctions[] = {
    {"initEthogram",               js_InitEthogram,            0, 0, 0},
    {"shutdownEthogram",           js_ShutdownEthogram,        0, 0, 0},
    {nsnull,                       nsnull,                     0, 0, 0}
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

  
  JSObject *optionsObj = ::JS_DefineObject(mContext, globalObj, "_options",
                                           &OptionsClass, nsnull, 0);
  if (optionsObj &&
      ::JS_DefineProperties(mContext, optionsObj, OptionsProperties)) {
    ::JS_SetOptions(mContext, mDefaultJSOptions);
  } else {
    rv = NS_ERROR_FAILURE;
  }

#ifdef NS_TRACE_MALLOC
  
  ::JS_DefineFunctions(mContext, globalObj, TraceMallocFunctions);
#endif

#ifdef MOZ_JPROF
  
  ::JS_DefineFunctions(mContext, globalObj, JProfFunctions);
#endif

#ifdef MOZ_SHARK
  
  ::JS_DefineFunctions(mContext, globalObj, SharkFunctions);
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
    ::JS_ClearScope(mContext, obj);

    
    
    
    
    
    
    
    
    
    
    ::JS_ClearWatchPointsForObject(mContext, obj);

    
    
    
    
    if (aClearFromProtoChain) {
      nsWindowSH::InvalidateGlobalScopePolluter(mContext, obj);

      
      for (JSObject *o = ::JS_GetPrototype(mContext, obj), *next;
           o && (next = ::JS_GetPrototype(mContext, o)); o = next)
        ::JS_ClearScope(mContext, o);
    }
  }

  ::JS_ClearRegExpStatics(mContext);

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
nsJSContext::GC()
{
  FireGCTimer(PR_FALSE);
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

  mNumEvaluations++;

#ifdef JS_GC_ZEAL
  if (mContext->runtime->gcZeal >= 2) {
    MaybeGC(mContext);
  } else
#endif
  if (mNumEvaluations > 20) {
    mNumEvaluations = 0;
    MaybeGC(mContext);
  }

  if (aTerminated) {
    mOperationCallbackTime = 0;
    mModalStateTime = 0;
  }
}

nsresult
nsJSContext::SetTerminationFunction(nsScriptTerminationFunc aFunc,
                                    nsISupports* aRef)
{
  NS_PRECONDITION(JS_IsRunning(mContext), "should be executing script");

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

NS_IMETHODIMP
nsJSContext::PreserveWrapper(nsIXPConnectWrappedNative *aWrapper)
{
  nsDOMClassInfo::PreserveNodeWrapper(aWrapper);
  return NS_OK;
}


void
nsJSContext::CC()
{
  ++sCCollectCount;
#ifdef DEBUG_smaug
  printf("Will run cycle collector (%i), %lldms since previous.\n",
         sCCollectCount, (PR_Now() - sPreviousCCTime) / PR_USEC_PER_MSEC);
#endif
  sPreviousCCTime = PR_Now();
  sDelayedCCollectCount = 0;
  sCCSuspectChanges = 0;
  
  
  sCollectedObjectsCounts = nsCycleCollector_collect();
  sCCSuspectedCount = nsCycleCollector_suspectedCount();
  sSavedGCCount = JS_GetGCParameter(nsJSRuntime::sRuntime, JSGC_NUMBER);
#ifdef DEBUG_smaug
  printf("Collected %u objects, %u suspected objects, took %lldms\n",
         sCollectedObjectsCounts, sCCSuspectedCount,
         (PR_Now() - sPreviousCCTime) / PR_USEC_PER_MSEC);
#endif
}

static inline uint32
GetGCRunsSinceLastCC()
{
    
    
    if (!nsJSRuntime::sRuntime)
        return 0;

    
    
    
    return JS_GetGCParameter(nsJSRuntime::sRuntime, JSGC_NUMBER) -
           sSavedGCCount;
}


PRBool
nsJSContext::MaybeCC(PRBool aHigherProbability)
{
  ++sDelayedCCollectCount;

  
  if (sCCSuspectChanges <= NS_MIN_SUSPECT_CHANGES ||
      GetGCRunsSinceLastCC() <= NS_MAX_GC_COUNT) {
#ifdef DEBUG_smaug
    PRTime now = PR_Now();
#endif
    PRUint32 suspected = nsCycleCollector_suspectedCount();
#ifdef DEBUG_smaug
    printf("%u suspected objects (%lldms), sCCSuspectedCount %u\n",
            suspected, (PR_Now() - now) / PR_USEC_PER_MSEC,
            sCCSuspectedCount);
#endif
    
    if (suspected > sCCSuspectedCount) {
      sCCSuspectChanges += (suspected - sCCSuspectedCount);
      sCCSuspectedCount = suspected;
    }
  }
#ifdef DEBUG_smaug
  printf("sCCSuspectChanges %u, GC runs %u\n",
         sCCSuspectChanges, GetGCRunsSinceLastCC());
#endif

  
  
  if (aHigherProbability ||
      sCollectedObjectsCounts > NS_COLLECTED_OBJECTS_LIMIT) {
    sDelayedCCollectCount *= NS_PROBABILITY_MULTIPLIER;
  }

  if (!sGCTimer &&
      (sDelayedCCollectCount > NS_MAX_DELAYED_CCOLLECT) &&
      ((sCCSuspectChanges > NS_MIN_SUSPECT_CHANGES &&
        GetGCRunsSinceLastCC() > NS_MAX_GC_COUNT) ||
       (sCCSuspectChanges > NS_MAX_SUSPECT_CHANGES))) {
    return IntervalCC();
  }
  return PR_FALSE;
}


void
nsJSContext::CCIfUserInactive()
{
  if (sUserIsActive) {
    MaybeCC(PR_TRUE);
  } else {
    IntervalCC();
  }
}


PRBool
nsJSContext::IntervalCC()
{
  if ((PR_Now() - sPreviousCCTime) >=
      PRTime(NS_MIN_CC_INTERVAL * PR_USEC_PER_MSEC)) {
    nsJSContext::CC();
    return PR_TRUE;
  }
#ifdef DEBUG_smaug
  printf("Running CC was delayed because of NS_MIN_CC_INTERVAL.\n");
#endif
  return PR_FALSE;
}


void
GCTimerFired(nsITimer *aTimer, void *aClosure)
{
  NS_RELEASE(sGCTimer);

  if (sPendingLoadCount == 0 || sLoadInProgressGCTimer) {
    sLoadInProgressGCTimer = PR_FALSE;

    
    
    
    
    
    
    sPendingLoadCount = 0;

    nsJSContext::CCIfUserInactive();
  } else {
    nsJSContext::FireGCTimer(PR_TRUE);
  }

  sReadyForGC = PR_TRUE;
}


void
nsJSContext::LoadStart()
{
  ++sPendingLoadCount;
}


void
nsJSContext::LoadEnd()
{
  
  
  if (sPendingLoadCount > 0) {
    --sPendingLoadCount;
  }

  if (!sPendingLoadCount && sLoadInProgressGCTimer) {
    sGCTimer->Cancel();
    NS_RELEASE(sGCTimer);
    sLoadInProgressGCTimer = PR_FALSE;

    CCIfUserInactive();
  }
}


void
nsJSContext::FireGCTimer(PRBool aLoadInProgress)
{
  if (sGCTimer) {
    
    return;
  }

  CallCreateInstance("@mozilla.org/timer;1", &sGCTimer);

  if (!sGCTimer) {
    NS_WARNING("Failed to create timer");

    
    
    sLoadInProgressGCTimer = PR_FALSE;

    CCIfUserInactive();
    return;
  }

  static PRBool first = PR_TRUE;

  sGCTimer->InitWithFuncCallback(GCTimerFired, nsnull,
                                 first ? NS_FIRST_GC_DELAY :
                                 aLoadInProgress ? NS_LOAD_IN_PROCESS_GC_DELAY :
                                                   NS_GC_DELAY,
                                 nsITimer::TYPE_ONE_SHOT);

  sLoadInProgressGCTimer = aLoadInProgress;

  first = PR_FALSE;
}

static JSBool
DOMGCCallback(JSContext *cx, JSGCStatus status)
{
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
  
  sDelayedCCollectCount = 0;
  sCCollectCount = 0;
  sUserIsActive = PR_FALSE;
  sPreviousCCTime = 0;
  sCollectedObjectsCounts = 0;
  sSavedGCCount = 0;
  sCCSuspectChanges = 0;
  sCCSuspectedCount = 0;
  sGCTimer = nsnull;
  sReadyForGC = PR_FALSE;
  sLoadInProgressGCTimer = PR_FALSE;
  sPendingLoadCount = 0;
  gNameSpaceManager = nsnull;
  sRuntimeService = nsnull;
  sRuntime = nsnull;
  gOldJSGCCallback = nsnull;
  sIsInitialized = PR_FALSE;
  sDidShutdown = PR_FALSE;
  sContextCount = 0;
  sSecurityManager = nsnull;
  gCollation = nsnull;
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
  PRInt32 highwatermark = nsContentUtils::GetIntPref(aPrefName, 32);

  if (highwatermark >= 32) {
    
    
    
    
    
    JS_SetGCParameter(nsJSRuntime::sRuntime, JSGC_MAX_MALLOC_BYTES,
                      32L * 1024L * 1024L);
    JS_SetGCParameter(nsJSRuntime::sRuntime, JSGC_MAX_BYTES,
                      0xffffffff);
  } else {
    JS_SetGCParameter(nsJSRuntime::sRuntime, JSGC_MAX_MALLOC_BYTES,
                      highwatermark * 1024L * 1024L);
    JS_SetGCParameter(nsJSRuntime::sRuntime, JSGC_MAX_BYTES,
                      highwatermark * 1024L * 1024L);
  }
  return 0;
}

static int
SetMemoryGCFrequencyPrefChangedCallback(const char* aPrefName, void* aClosure)
{
  PRInt32 triggerFactor = nsContentUtils::GetIntPref(aPrefName, 1600);
  JS_SetGCParameter(nsJSRuntime::sRuntime, JSGC_TRIGGER_FACTOR, triggerFactor);
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

  sSavedGCCount = JS_GetGCParameter(nsJSRuntime::sRuntime, JSGC_NUMBER);

  
  gOldJSGCCallback = ::JS_SetGCCallbackRT(sRuntime, DOMGCCallback);

  JSSecurityCallbacks *callbacks = JS_GetRuntimeSecurityCallbacks(sRuntime);
  NS_ASSERTION(callbacks, "SecMan should have set security callbacks!");

  callbacks->findObjectPrincipals = ObjectPrincipalFinder;

  
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

  nsContentUtils::RegisterPrefCallback("javascript.options.mem.gc_frequency",
                                       SetMemoryGCFrequencyPrefChangedCallback,
                                       nsnull);
  SetMemoryGCFrequencyPrefChangedCallback("javascript.options.mem.gc_frequency",
                                          nsnull);

  nsCOMPtr<nsIObserverService> obs =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsIObserver* activityObserver = new nsUserActivityObserver();
  NS_ENSURE_TRUE(activityObserver, NS_ERROR_OUT_OF_MEMORY);
  obs->AddObserver(activityObserver, "user-interaction-inactive", PR_FALSE);
  obs->AddObserver(activityObserver, "user-interaction-active", PR_FALSE);
  obs->AddObserver(activityObserver, "xpcom-shutdown", PR_FALSE);

  nsIObserver* ccMemPressureObserver = new nsCCMemoryPressureObserver();
  NS_ENSURE_TRUE(ccMemPressureObserver, NS_ERROR_OUT_OF_MEMORY);
  obs->AddObserver(ccMemPressureObserver, "memory-pressure", PR_FALSE);

  sIsInitialized = PR_TRUE;

  return rv;
}


nsScriptNameSpaceManager*
nsJSRuntime::GetNameSpaceManager()
{
  if (sDidShutdown)
    return nsnull;

  if (!gNameSpaceManager) {
    gNameSpaceManager = new nsScriptNameSpaceManager;
    NS_ENSURE_TRUE(gNameSpaceManager, nsnull);

    nsresult rv = gNameSpaceManager->Init();
    NS_ENSURE_SUCCESS(rv, nsnull);
  }

  return gNameSpaceManager;
}


void
nsJSRuntime::Shutdown()
{
  if (sGCTimer) {
    
    

    sGCTimer->Cancel();

    NS_RELEASE(sGCTimer);

    sLoadInProgressGCTimer = PR_FALSE;
  }

  delete gNameSpaceManager;
  gNameSpaceManager = nsnull;

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
    NS_IF_RELEASE(gCollation);
    NS_IF_RELEASE(gDecoder);
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
    mArgv(argv),
    mArgc(argc)
{
  
  
  mArgv = (jsval *) PR_CALLOC(argc * sizeof(jsval));
  if (!mArgv) {
    *prv = NS_ERROR_OUT_OF_MEMORY;
    return;
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
NS_IMPL_CYCLE_COLLECTION_ROOT_BEGIN(nsJSArgArray)
  tmp->ReleaseJSObjects();
NS_IMPL_CYCLE_COLLECTION_ROOT_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_0(nsJSArgArray)
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

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsJSArgArray, nsIJSArgArray)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsJSArgArray, nsIJSArgArray)

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
