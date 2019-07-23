






































#include "nsCOMPtr.h"
#include "nsIScriptContext.h"
#include "nsIArray.h"
#include "nsIScriptTimeoutHandler.h"
#include "nsIXPConnect.h"
#include "nsIJSRuntimeService.h"
#include "nsJSUtils.h"
#include "nsDOMJSUtils.h"
#include "nsContentUtils.h"
#include "nsJSEnvironment.h"
#include "nsServiceManagerUtils.h"
#include "nsDOMError.h"

static const char kSetIntervalStr[] = "setInterval";
static const char kSetTimeoutStr[] = "setTimeout";


class nsJSScriptTimeoutHandler: public nsIScriptTimeoutHandler
{
public:
  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsJSScriptTimeoutHandler)

  nsJSScriptTimeoutHandler();
  ~nsJSScriptTimeoutHandler();

  virtual const PRUnichar *GetHandlerText();
  virtual void *GetScriptObject() {
    return mFunObj;
  }
  virtual void GetLocation(const char **aFileName, PRUint32 *aLineNo) {
    *aFileName = mFileName.get();
    *aLineNo = mLineNo;
  }

  virtual PRUint32 GetScriptTypeID() {
        return nsIProgrammingLanguage::JAVASCRIPT;
  }
  virtual PRUint32 GetScriptVersion() {
        return mVersion;
  }

  virtual nsIArray *GetArgv() {
    return mArgv;
  }
  
  
  virtual void SetLateness(PRIntervalTime aHowLate);

  nsresult Init(nsIScriptContext *aContext, PRBool *aIsInterval,
                PRInt32 *aInterval);

  void ReleaseJSObjects();

private:

  nsCOMPtr<nsIScriptContext> mContext;

  
  
  nsCAutoString mFileName;
  PRUint32 mLineNo;
  PRUint32 mVersion;
  nsCOMPtr<nsIArray> mArgv;

  
  JSString *mExpr;
  JSObject *mFunObj;
};




NS_IMPL_CYCLE_COLLECTION_CLASS(nsJSScriptTimeoutHandler)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsJSScriptTimeoutHandler)
  tmp->ReleaseJSObjects();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsJSScriptTimeoutHandler)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mArgv)
  cb.NoteScriptChild(nsIProgrammingLanguage::JAVASCRIPT, tmp->mFunObj);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsJSScriptTimeoutHandler)
  NS_INTERFACE_MAP_ENTRY(nsIScriptTimeoutHandler)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsJSScriptTimeoutHandler)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsJSScriptTimeoutHandler)

nsJSScriptTimeoutHandler::nsJSScriptTimeoutHandler() :
  mLineNo(0),
  mVersion(nsnull),
  mExpr(nsnull),
  mFunObj(nsnull)
{
}

nsJSScriptTimeoutHandler::~nsJSScriptTimeoutHandler()
{
  ReleaseJSObjects();
}

void
nsJSScriptTimeoutHandler::ReleaseJSObjects()
{
  if (mExpr || mFunObj) {
    nsCOMPtr<nsIScriptContext> scx = mContext;
    JSRuntime *rt = nsnull;

    if (scx) {
      JSContext *cx;
      cx = (JSContext *)scx->GetNativeContext();
      rt = ::JS_GetRuntime(cx);
      mContext = nsnull;
    } else {
      
      
      
      
      
      
      
      
      
      
      
      

      nsCOMPtr<nsIJSRuntimeService> rtsvc =
        do_GetService("@mozilla.org/js/xpc/RuntimeService;1");

      if (rtsvc) {
        rtsvc->GetRuntime(&rt);
      }
    }

    if (!rt) {
      

      NS_ERROR("nsTimeout::Release() with no JSRuntime. eek!");

      return;
    }

    if (mExpr) {
      ::JS_RemoveRootRT(rt, &mExpr);
      mExpr = nsnull;
    } else if (mFunObj) {
      ::JS_RemoveRootRT(rt, &mFunObj);
      mFunObj = nsnull;
    } else {
      NS_WARNING("No func and no expr - roots may not have been removed");
    }
  }
}

nsresult
nsJSScriptTimeoutHandler::Init(nsIScriptContext *aContext, PRBool *aIsInterval,
                               PRInt32 *aInterval)
{
  if (!aContext) {
    
    

    return NS_ERROR_NOT_INITIALIZED;
  }

  mContext = aContext;

  nsCOMPtr<nsIXPCNativeCallContext> ncc;
  nsresult rv = nsContentUtils::XPConnect()->
    GetCurrentNativeCallContext(getter_AddRefs(ncc));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ncc)
    return NS_ERROR_NOT_AVAILABLE;

  JSContext *cx = nsnull;

  rv = ncc->GetJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 argc;
  jsval *argv = nsnull;

  ncc->GetArgc(&argc);
  ncc->GetArgvPtr(&argv);

  JSString *expr = nsnull;
  JSObject *funobj = nsnull;
  int32 interval = 0;

  JSAutoRequest ar(cx);

  if (argc < 1) {
    ::JS_ReportError(cx, "Function %s requires at least 1 parameter",
                     *aIsInterval ? kSetIntervalStr : kSetTimeoutStr);

    ncc->SetExceptionWasThrown(PR_TRUE);
    return NS_ERROR_DOM_TYPE_ERR;
  }

  if (argc > 1 && !::JS_ValueToECMAInt32(cx, argv[1], &interval)) {
    ::JS_ReportError(cx,
                     "Second argument to %s must be a millisecond interval",
                     aIsInterval ? kSetIntervalStr : kSetTimeoutStr);

    ncc->SetExceptionWasThrown(PR_TRUE);
    return NS_ERROR_DOM_TYPE_ERR;
  }

  if (argc == 1) {
    
    
    *aIsInterval = PR_FALSE;
  }

  switch (::JS_TypeOfValue(cx, argv[0])) {
  case JSTYPE_FUNCTION:
    funobj = JSVAL_TO_OBJECT(argv[0]);
    break;

  case JSTYPE_STRING:
  case JSTYPE_OBJECT:
    expr = ::JS_ValueToString(cx, argv[0]);
    if (!expr)
      return NS_ERROR_OUT_OF_MEMORY;
    argv[0] = STRING_TO_JSVAL(expr);
    break;

  default:
    ::JS_ReportError(cx, "useless %s call (missing quotes around argument?)",
                     *aIsInterval ? kSetIntervalStr : kSetTimeoutStr);

    
    ncc->SetExceptionWasThrown(PR_TRUE);
    return NS_ERROR_DOM_TYPE_ERR;
  }

  if (expr) {
    if (!::JS_AddNamedRoot(cx, &mExpr, "timeout.mExpr")) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    mExpr = expr;
  } else if (funobj) {
    if (!::JS_AddNamedRoot(cx, &mFunObj, "timeout.mFunObj")) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    mFunObj = funobj;

    
    
    
    nsCOMPtr<nsIArray> array;
    rv = NS_CreateJSArgv(cx, (argc > 1) ? argc - 1 : argc, nsnull,
                         getter_AddRefs(array));
    if (NS_FAILED(rv)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    PRUint32 dummy;
    jsval *jsargv = nsnull;
    nsCOMPtr<nsIJSArgArray> jsarray(do_QueryInterface(array));
    jsarray->GetArgs(&dummy, reinterpret_cast<void **>(&jsargv));
    
    NS_ASSERTION(jsargv, "No argv!");
    for (PRInt32 i = 2; (PRUint32)i < argc; ++i) {
      jsargv[i - 2] = argv[i];
    }
    
    mArgv = array;

    
    const char *filename;
    if (nsJSUtils::GetCallingLocation(cx, &filename, &mLineNo)) {
      mFileName.Assign(filename);

      if (mFileName.IsEmpty()) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }

  } else {
    NS_WARNING("No func and no expr - why are we here?");
  }
  *aInterval = interval;
  return NS_OK;
}

void nsJSScriptTimeoutHandler::SetLateness(PRIntervalTime aHowLate)
{
  nsCOMPtr<nsIJSArgArray> jsarray(do_QueryInterface(mArgv));
  if (jsarray) {
    PRUint32 argc;
    jsval *jsargv;
    jsarray->GetArgs(&argc, reinterpret_cast<void **>(&jsargv));
    if (jsargv && argc)
      jsargv[argc-1] = INT_TO_JSVAL((jsint) aHowLate);
  } else {
    NS_ERROR("How can our argv not handle this?");
  }
}

const PRUnichar *
nsJSScriptTimeoutHandler::GetHandlerText()
{
  NS_ASSERTION(mExpr, "No expression, so no handler text!");
  return reinterpret_cast<const PRUnichar *>
                         (::JS_GetStringChars(mExpr));
}

nsresult NS_CreateJSTimeoutHandler(nsIScriptContext *aContext,
                                   PRBool *aIsInterval,
                                   PRInt32 *aInterval,
                                   nsIScriptTimeoutHandler **aRet)
{
  *aRet = nsnull;
  nsJSScriptTimeoutHandler *handler = new nsJSScriptTimeoutHandler();
  if (!handler)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = handler->Init(aContext, aIsInterval, aInterval);
  if (NS_FAILED(rv)) {
    delete handler;
    return rv;
  }
  return handler->QueryInterface(NS_GET_IID(nsIScriptTimeoutHandler),
                                 reinterpret_cast<void **>(aRet));
}
