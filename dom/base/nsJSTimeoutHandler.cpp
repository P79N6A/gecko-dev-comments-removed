





#include "nsCOMPtr.h"
#include "nsIScriptContext.h"
#include "nsIDocument.h"
#include "nsIArray.h"
#include "nsIScriptTimeoutHandler.h"
#include "nsIXPConnect.h"
#include "nsIJSRuntimeService.h"
#include "nsJSUtils.h"
#include "nsDOMJSUtils.h"
#include "nsContentUtils.h"
#include "nsJSEnvironment.h"
#include "nsServiceManagerUtils.h"
#include "nsError.h"
#include "nsGlobalWindow.h"
#include "nsIContentSecurityPolicy.h"
#include "nsAlgorithm.h"
#include "mozilla/Attributes.h"
#include "mozilla/Likely.h"
#include <algorithm>

static const char kSetIntervalStr[] = "setInterval";
static const char kSetTimeoutStr[] = "setTimeout";


class nsJSScriptTimeoutHandler MOZ_FINAL : public nsIScriptTimeoutHandler
{
public:
  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsJSScriptTimeoutHandler)

  nsJSScriptTimeoutHandler();
  ~nsJSScriptTimeoutHandler();

  virtual const PRUnichar *GetHandlerText();
  virtual JSObject *GetScriptObject() {
    return mFunObj;
  }
  virtual void GetLocation(const char **aFileName, uint32_t *aLineNo) {
    *aFileName = mFileName.get();
    *aLineNo = mLineNo;
  }

  virtual nsIArray *GetArgv() {
    return mArgv;
  }

  nsresult Init(nsGlobalWindow *aWindow, bool *aIsInterval,
                int32_t *aInterval);

  void ReleaseJSObjects();

private:

  nsCOMPtr<nsIScriptContext> mContext;

  
  
  nsCString mFileName;
  uint32_t mLineNo;
  nsCOMPtr<nsIJSArgArray> mArgv;

  
  JSFlatString *mExpr;
  JSObject *mFunObj;
};




NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsJSScriptTimeoutHandler)
  tmp->ReleaseJSObjects();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INTERNAL(nsJSScriptTimeoutHandler)
  if (MOZ_UNLIKELY(cb.WantDebugInfo())) {
    nsAutoCString name("nsJSScriptTimeoutHandler");
    if (tmp->mExpr) {
      name.AppendLiteral(" [");
      name.Append(tmp->mFileName);
      name.AppendLiteral(":");
      name.AppendInt(tmp->mLineNo);
      name.AppendLiteral("]");
    }
    else if (tmp->mFunObj) {
      JSFunction* fun = JS_GetObjectFunction(tmp->mFunObj);
      if (fun && JS_GetFunctionId(fun)) {
        JSFlatString *funId = JS_ASSERT_STRING_IS_FLAT(JS_GetFunctionId(fun));
        size_t size = 1 + JS_PutEscapedFlatString(NULL, 0, funId, 0);
        char *funIdName = new char[size];
        if (funIdName) {
          JS_PutEscapedFlatString(funIdName, size, funId, 0);
          name.AppendLiteral(" [");
          name.Append(funIdName);
          delete[] funIdName;
          name.AppendLiteral("]");
        }
      }
    }
    cb.DescribeRefCountedNode(tmp->mRefCnt.get(), name.get());
  }
  else {
    NS_IMPL_CYCLE_COLLECTION_DESCRIBE(nsJSScriptTimeoutHandler,
                                      tmp->mRefCnt.get())
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mArgv)
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
  mExpr(nullptr),
  mFunObj(nullptr)
{
}

nsJSScriptTimeoutHandler::~nsJSScriptTimeoutHandler()
{
  ReleaseJSObjects();
}

void
nsJSScriptTimeoutHandler::ReleaseJSObjects()
{
  if (mExpr) {
    mExpr = nullptr;
  } else {
    mFunObj = nullptr;
  }
  NS_DROP_JS_OBJECTS(this, nsJSScriptTimeoutHandler);
}

nsresult
nsJSScriptTimeoutHandler::Init(nsGlobalWindow *aWindow, bool *aIsInterval,
                               int32_t *aInterval)
{
  mContext = aWindow->GetContextInternal();
  if (!mContext) {
    
    

    return NS_ERROR_NOT_INITIALIZED;
  }

  nsAXPCNativeCallContext *ncc = nullptr;
  nsresult rv = nsContentUtils::XPConnect()->
    GetCurrentNativeCallContext(&ncc);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ncc)
    return NS_ERROR_NOT_AVAILABLE;

  JSContext *cx = nullptr;

  rv = ncc->GetJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t argc;
  jsval *argv = nullptr;

  ncc->GetArgc(&argc);
  ncc->GetArgvPtr(&argv);

  JSFlatString *expr = nullptr;
  JSObject *funobj = nullptr;

  JSAutoRequest ar(cx);

  if (argc < 1) {
    ::JS_ReportError(cx, "Function %s requires at least 2 parameter",
                     *aIsInterval ? kSetIntervalStr : kSetTimeoutStr);
    return NS_ERROR_DOM_TYPE_ERR;
  }

  int32_t interval = 0;
  if (argc > 1 && !::JS_ValueToECMAInt32(cx, argv[1], &interval)) {
    ::JS_ReportError(cx,
                     "Second argument to %s must be a millisecond interval",
                     aIsInterval ? kSetIntervalStr : kSetTimeoutStr);
    return NS_ERROR_DOM_TYPE_ERR;
  }

  if (argc == 1) {
    
    
    *aIsInterval = false;
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
        bool allowsEval;
        
        
        rv = csp->GetAllowsEval(&allowsEval);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!allowsEval) {
          ::JS_ReportError(cx, "call to %s blocked by CSP",
                            *aIsInterval ? kSetIntervalStr : kSetTimeoutStr);

          
          return NS_ERROR_DOM_TYPE_ERR;
        }
      }
    } 

    NS_HOLD_JS_OBJECTS(this, nsJSScriptTimeoutHandler);

    mExpr = expr;

    
    const char *filename;
    if (nsJSUtils::GetCallingLocation(cx, &filename, &mLineNo)) {
      mFileName.Assign(filename);
    }
  } else if (funobj) {
    NS_HOLD_JS_OBJECTS(this, nsJSScriptTimeoutHandler);

    mFunObj = funobj;

    
    
    
    
    nsCOMPtr<nsIJSArgArray> array;
    
    rv = NS_CreateJSArgv(cx, std::max(argc, 2u) - 2, nullptr,
                         getter_AddRefs(array));
    if (NS_FAILED(rv)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    uint32_t dummy;
    jsval *jsargv = nullptr;
    array->GetArgs(&dummy, reinterpret_cast<void **>(&jsargv));

    
    if (jsargv) {
      for (int32_t i = 2; (uint32_t)i < argc; ++i) {
        jsargv[i - 2] = argv[i];
      }
    } else {
      NS_ASSERTION(argc <= 2, "Why do we have no jsargv when we have arguments?");
    }
    mArgv = array;
  } else {
    NS_WARNING("No func and no expr - why are we here?");
  }
  *aInterval = interval;
  return NS_OK;
}

const PRUnichar *
nsJSScriptTimeoutHandler::GetHandlerText()
{
  NS_ASSERTION(mExpr, "No expression, so no handler text!");
  return ::JS_GetFlatStringChars(mExpr);
}

nsresult NS_CreateJSTimeoutHandler(nsGlobalWindow *aWindow,
                                   bool *aIsInterval,
                                   int32_t *aInterval,
                                   nsIScriptTimeoutHandler **aRet)
{
  *aRet = nullptr;
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
