





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
#include "nsError.h"
#include "nsGlobalWindow.h"
#include "nsIContentSecurityPolicy.h"
#include "nsAlgorithm.h"
#include "mozilla/Attributes.h"
#include "mozilla/Likely.h"
#include "mozilla/dom/FunctionBinding.h"

static const char kSetIntervalStr[] = "setInterval";
static const char kSetTimeoutStr[] = "setTimeout";

using namespace mozilla::dom;


class nsJSScriptTimeoutHandler MOZ_FINAL : public nsIScriptTimeoutHandler
{
public:
  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsJSScriptTimeoutHandler)

  nsJSScriptTimeoutHandler();
  ~nsJSScriptTimeoutHandler();

  virtual const PRUnichar *GetHandlerText();
  virtual Function* GetCallback()
  {
    return mFunction;
  }
  virtual void GetLocation(const char **aFileName, uint32_t *aLineNo)
  {
    *aFileName = mFileName.get();
    *aLineNo = mLineNo;
  }

  virtual const nsTArray<JS::Value>& GetArgs()
  {
    return mArgs;
  }

  nsresult Init(nsGlobalWindow *aWindow, bool *aIsInterval,
                int32_t *aInterval);

  void ReleaseJSObjects();

private:
  
  
  nsCString mFileName;
  uint32_t mLineNo;
  nsTArray<JS::Value> mArgs;

  
  JSFlatString *mExpr;
  nsRefPtr<Function> mFunction;
};




NS_IMPL_CYCLE_COLLECTION_CLASS(nsJSScriptTimeoutHandler)
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
    else if (tmp->mFunction) {
      JSFunction* fun =
        JS_GetObjectFunction(js::UnwrapObject(tmp->mFunction->Callable()));
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

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mFunction)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsJSScriptTimeoutHandler)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mExpr)
  for (uint32_t i = 0; i < tmp->mArgs.Length(); ++i) {
    NS_IMPL_CYCLE_COLLECTION_TRACE_JSVAL_MEMBER_CALLBACK(mArgs[i])
  }
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsJSScriptTimeoutHandler)
  NS_INTERFACE_MAP_ENTRY(nsIScriptTimeoutHandler)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsJSScriptTimeoutHandler)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsJSScriptTimeoutHandler)

nsJSScriptTimeoutHandler::nsJSScriptTimeoutHandler() :
  mLineNo(0),
  mExpr(nullptr)
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
    mFunction = nullptr;
    mArgs.Clear();
  }
  NS_DROP_JS_OBJECTS(this, nsJSScriptTimeoutHandler);
}

nsresult
nsJSScriptTimeoutHandler::Init(nsGlobalWindow *aWindow, bool *aIsInterval,
                               int32_t *aInterval)
{
  if (!aWindow->GetContextInternal() || !aWindow->FastGetGlobalJSObject()) {
    
    

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

    bool ok;
    mFunction = new Function(cx, aWindow->FastGetGlobalJSObject(), funobj, &ok);
    if (!ok) {
      NS_DROP_JS_OBJECTS(this, nsJSScriptTimeoutHandler);
      return NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    
    
    
    uint32_t argCount = NS_MAX(argc, 2u) - 2;
    FallibleTArray<JS::Value> args;
    if (!args.SetCapacity(argCount)) {
      
      return NS_ERROR_OUT_OF_MEMORY;
    }
    for (uint32_t idx = 0; idx < argCount; ++idx) {
      *args.AppendElement() = argv[idx + 2];
    }
    args.SwapElements(mArgs);
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
  nsRefPtr<nsJSScriptTimeoutHandler> handler = new nsJSScriptTimeoutHandler();
  nsresult rv = handler->Init(aWindow, aIsInterval, aInterval);
  if (NS_FAILED(rv)) {
    return rv;
  }

  handler.forget(aRet);

  return NS_OK;
}
