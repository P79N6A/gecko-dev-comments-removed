






































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
#include "nsGlobalWindow.h"
#include "nsIContentSecurityPolicy.h"

static const char kSetIntervalStr[] = "setInterval";
static const char kSetTimeoutStr[] = "setTimeout";


class nsJSScriptTimeoutHandler: public nsIScriptTimeoutHandler
{
public:
  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsJSScriptTimeoutHandler)

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

  nsresult Init(nsGlobalWindow *aWindow, PRBool *aIsInterval,
                PRInt32 *aInterval);

  void ReleaseJSObjects();

private:

  nsCOMPtr<nsIScriptContext> mContext;

  
  
  nsCString mFileName;
  PRUint32 mLineNo;
  PRUint32 mVersion;
  nsCOMPtr<nsIArray> mArgv;

  
  JSFlatString *mExpr;
  JSObject *mFunObj;
};




NS_IMPL_CYCLE_COLLECTION_CLASS(nsJSScriptTimeoutHandler)
NS_IMPL_CYCLE_COLLECTION_ROOT_BEGIN(nsJSScriptTimeoutHandler)
  tmp->ReleaseJSObjects();
NS_IMPL_CYCLE_COLLECTION_ROOT_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_0(nsJSScriptTimeoutHandler)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INTERNAL(nsJSScriptTimeoutHandler)
  if (NS_UNLIKELY(cb.WantDebugInfo())) {
    nsCAutoString foo("nsJSScriptTimeoutHandler");
    if (tmp->mExpr) {
      foo.AppendLiteral(" [");
      foo.Append(tmp->mFileName);
      foo.AppendLiteral(":");
      foo.AppendInt(tmp->mLineNo);
      foo.AppendLiteral("]");
    }
    else if (tmp->mFunObj) {
      JSFunction* fun = (JSFunction*)tmp->mFunObj->getPrivate();
      if (fun->atom) {
        JSFlatString *funId = JS_ASSERT_STRING_IS_FLAT(JS_GetFunctionId(fun));
        size_t size = 1 + JS_PutEscapedFlatString(NULL, 0, funId, 0);
        char *name = new char[size];
        if (name) {
          JS_PutEscapedFlatString(name, size, funId, 0);
          foo.AppendLiteral(" [");
          foo.Append(name);
          delete[] name;
          foo.AppendLiteral("]");
        }
      }
    }
    cb.DescribeNode(RefCounted, tmp->mRefCnt.get(),
                    sizeof(nsJSScriptTimeoutHandler), foo.get());
  }
  else {
    cb.DescribeNode(RefCounted, tmp->mRefCnt.get(),
                    sizeof(nsJSScriptTimeoutHandler),
                    "nsJSScriptTimeoutHandler");
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mArgv)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsJSScriptTimeoutHandler)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mExpr)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mFunObj)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

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
    if (mExpr) {
      NS_DROP_JS_OBJECTS(this, nsJSScriptTimeoutHandler);
      mExpr = nsnull;
    } else if (mFunObj) {
      NS_DROP_JS_OBJECTS(this, nsJSScriptTimeoutHandler);
      mFunObj = nsnull;
    } else {
      NS_WARNING("No func and no expr - roots may not have been removed");
    }
  }
}

nsresult
nsJSScriptTimeoutHandler::Init(nsGlobalWindow *aWindow, PRBool *aIsInterval,
                               PRInt32 *aInterval)
{
  mContext = aWindow->GetContextInternal();
  if (!mContext) {
    
    

    return NS_ERROR_NOT_INITIALIZED;
  }

  nsAXPCNativeCallContext *ncc = nsnull;
  nsresult rv = nsContentUtils::XPConnect()->
    GetCurrentNativeCallContext(&ncc);
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

  JSFlatString *expr = nsnull;
  JSObject *funobj = nsnull;
  int32 interval = 0;

  JSAutoRequest ar(cx);

  if (argc < 1) {
    ::JS_ReportError(cx, "Function %s requires at least 2 parameter",
                     *aIsInterval ? kSetIntervalStr : kSetTimeoutStr);
    return NS_ERROR_DOM_TYPE_ERR;
  }

  if (argc > 1 && !::JS_ValueToECMAInt32(cx, argv[1], &interval)) {
    ::JS_ReportError(cx,
                     "Second argument to %s must be a millisecond interval",
                     aIsInterval ? kSetIntervalStr : kSetTimeoutStr);
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
    {
      JSString *str = ::JS_ValueToString(cx, argv[0]);
      if (!str)
        return NS_ERROR_OUT_OF_MEMORY;

      expr = ::JS_FlattenString(cx, str);
      if (!expr)
          return NS_ERROR_OUT_OF_MEMORY;

      argv[0] = STRING_TO_JSVAL(str);
    }
    break;

  default:
    ::JS_ReportError(cx, "useless %s call (missing quotes around argument?)",
                     *aIsInterval ? kSetIntervalStr : kSetTimeoutStr);

    
    return NS_ERROR_DOM_TYPE_ERR;
  }

  if (expr) {
    
    
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(aWindow->GetExtantDocument());

    if (doc) {
      nsCOMPtr<nsIContentSecurityPolicy> csp;
      nsresult rv = doc->NodePrincipal()->GetCsp(getter_AddRefs(csp));
      NS_ENSURE_SUCCESS(rv, rv);

      if (csp) {
        PRBool allowsEval;
        
        
        rv = csp->GetAllowsEval(&allowsEval);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!allowsEval) {
          ::JS_ReportError(cx, "call to %s blocked by CSP",
                            *aIsInterval ? kSetIntervalStr : kSetTimeoutStr);

          
          return NS_ERROR_DOM_TYPE_ERR;
        }
      }
    } 

    rv = NS_HOLD_JS_OBJECTS(this, nsJSScriptTimeoutHandler);
    NS_ENSURE_SUCCESS(rv, rv);

    mExpr = expr;

    
    const char *filename;
    if (nsJSUtils::GetCallingLocation(cx, &filename, &mLineNo)) {
      mFileName.Assign(filename);
    }
  } else if (funobj) {
    rv = NS_HOLD_JS_OBJECTS(this, nsJSScriptTimeoutHandler);
    NS_ENSURE_SUCCESS(rv, rv);

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
    nsresult rv = jsarray->GetArgs(&argc, reinterpret_cast<void **>(&jsargv));
    if (NS_SUCCEEDED(rv) && jsargv && argc)
      jsargv[argc-1] = INT_TO_JSVAL((jsint) aHowLate);
  } else {
    NS_ERROR("How can our argv not handle this?");
  }
}

const PRUnichar *
nsJSScriptTimeoutHandler::GetHandlerText()
{
  NS_ASSERTION(mExpr, "No expression, so no handler text!");
  return ::JS_GetFlatStringChars(mExpr);
}

nsresult NS_CreateJSTimeoutHandler(nsGlobalWindow *aWindow,
                                   PRBool *aIsInterval,
                                   PRInt32 *aInterval,
                                   nsIScriptTimeoutHandler **aRet)
{
  *aRet = nsnull;
  nsJSScriptTimeoutHandler *handler = new nsJSScriptTimeoutHandler();
  if (!handler)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = handler->Init(aWindow, aIsInterval, aInterval);
  if (NS_FAILED(rv)) {
    delete handler;
    return rv;
  }

  NS_ADDREF(*aRet = handler);

  return NS_OK;
}
