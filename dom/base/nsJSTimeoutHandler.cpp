





#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsIScriptTimeoutHandler.h"
#include "nsIXPConnect.h"
#include "nsJSUtils.h"
#include "nsContentUtils.h"
#include "nsError.h"
#include "nsGlobalWindow.h"
#include "nsIContentSecurityPolicy.h"
#include "mozilla/Attributes.h"
#include "mozilla/Likely.h"
#include <algorithm>
#include "mozilla/dom/FunctionBinding.h"
#include "nsAXPCNativeCallContext.h"

static const char kSetIntervalStr[] = "setInterval";
static const char kSetTimeoutStr[] = "setTimeout";

using namespace mozilla;
using namespace mozilla::dom;


class nsJSScriptTimeoutHandler final : public nsIScriptTimeoutHandler
{
public:
  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsJSScriptTimeoutHandler)

  nsJSScriptTimeoutHandler();
  
  nsJSScriptTimeoutHandler(nsGlobalWindow *aWindow, Function& aFunction,
                           FallibleTArray<JS::Heap<JS::Value> >& aArguments,
                           ErrorResult& aError);
  nsJSScriptTimeoutHandler(JSContext* aCx, nsGlobalWindow *aWindow,
                           const nsAString& aExpression, bool* aAllowEval,
                           ErrorResult& aError);

  virtual const char16_t* GetHandlerText() override;
  virtual Function* GetCallback() override
  {
    return mFunction;
  }
  virtual void GetLocation(const char** aFileName, uint32_t* aLineNo) override
  {
    *aFileName = mFileName.get();
    *aLineNo = mLineNo;
  }

  virtual const nsTArray<JS::Value>& GetArgs() override
  {
    return mArgs;
  }

  void ReleaseJSObjects();

private:
  ~nsJSScriptTimeoutHandler();

  
  
  nsCString mFileName;
  uint32_t mLineNo;
  nsTArray<JS::Heap<JS::Value> > mArgs;

  
  
  nsString mExpr;
  nsRefPtr<Function> mFunction;
};




NS_IMPL_CYCLE_COLLECTION_CLASS(nsJSScriptTimeoutHandler)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsJSScriptTimeoutHandler)
  tmp->ReleaseJSObjects();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INTERNAL(nsJSScriptTimeoutHandler)
  if (MOZ_UNLIKELY(cb.WantDebugInfo())) {
    nsAutoCString name("nsJSScriptTimeoutHandler");
    if (tmp->mFunction) {
      JSFunction* fun =
        JS_GetObjectFunction(js::UncheckedUnwrap(tmp->mFunction->Callable()));
      if (fun && JS_GetFunctionId(fun)) {
        JSFlatString *funId = JS_ASSERT_STRING_IS_FLAT(JS_GetFunctionId(fun));
        size_t size = 1 + JS_PutEscapedFlatString(nullptr, 0, funId, 0);
        char *funIdName = new char[size];
        if (funIdName) {
          JS_PutEscapedFlatString(funIdName, size, funId, 0);
          name.AppendLiteral(" [");
          name.Append(funIdName);
          delete[] funIdName;
          name.Append(']');
        }
      }
    } else {
      name.AppendLiteral(" [");
      name.Append(tmp->mFileName);
      name.Append(':');
      name.AppendInt(tmp->mLineNo);
      name.Append(']');
    }
    cb.DescribeRefCountedNode(tmp->mRefCnt.get(), name.get());
  }
  else {
    NS_IMPL_CYCLE_COLLECTION_DESCRIBE(nsJSScriptTimeoutHandler,
                                      tmp->mRefCnt.get())
  }

  if (tmp->mFunction) {
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mFunction)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsJSScriptTimeoutHandler)
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

static bool
CheckCSPForEval(JSContext* aCx, nsGlobalWindow* aWindow, ErrorResult& aError)
{
  
  
  nsCOMPtr<nsIDocument> doc = aWindow->GetExtantDoc();
  if (!doc) {
    
    return true;
  }

  nsCOMPtr<nsIContentSecurityPolicy> csp;
  aError = doc->NodePrincipal()->GetCsp(getter_AddRefs(csp));
  if (aError.Failed()) {
    return false;
  }

  if (!csp) {
    return true;
  }

  bool allowsEval = true;
  bool reportViolation = false;
  aError = csp->GetAllowsEval(&reportViolation, &allowsEval);
  if (aError.Failed()) {
    return false;
  }

  if (reportViolation) {
    
    NS_NAMED_LITERAL_STRING(scriptSample,
                            "call to eval() or related function blocked by CSP");

    
    uint32_t lineNum = 0;
    nsAutoString fileNameString;
    if (!nsJSUtils::GetCallingLocation(aCx, fileNameString, &lineNum)) {
      fileNameString.AssignLiteral("unknown");
    }

    csp->LogViolationDetails(nsIContentSecurityPolicy::VIOLATION_TYPE_EVAL,
                             fileNameString, scriptSample, lineNum,
                             EmptyString(), EmptyString());
  }

  return allowsEval;
}

nsJSScriptTimeoutHandler::nsJSScriptTimeoutHandler() :
  mLineNo(0)
{
}

nsJSScriptTimeoutHandler::nsJSScriptTimeoutHandler(nsGlobalWindow *aWindow,
                                                   Function& aFunction,
                                                   FallibleTArray<JS::Heap<JS::Value> >& aArguments,
                                                   ErrorResult& aError) :
  mLineNo(0),
  mFunction(&aFunction)
{
  if (!aWindow->GetContextInternal() || !aWindow->FastGetGlobalJSObject()) {
    
    
    aError.Throw(NS_ERROR_NOT_INITIALIZED);
    return;
  }

  mozilla::HoldJSObjects(this);
  mArgs.SwapElements(aArguments);
}

nsJSScriptTimeoutHandler::nsJSScriptTimeoutHandler(JSContext* aCx,
                                                   nsGlobalWindow *aWindow,
                                                   const nsAString& aExpression,
                                                   bool* aAllowEval,
                                                   ErrorResult& aError) :
  mLineNo(0),
  mExpr(aExpression)
{
  if (!aWindow->GetContextInternal() || !aWindow->FastGetGlobalJSObject()) {
    
    
    aError.Throw(NS_ERROR_NOT_INITIALIZED);
    return;
  }

  *aAllowEval = CheckCSPForEval(aCx, aWindow, aError);
  if (aError.Failed() || !*aAllowEval) {
    return;
  }

  
  nsJSUtils::GetCallingLocation(aCx, mFileName, &mLineNo);
}

nsJSScriptTimeoutHandler::~nsJSScriptTimeoutHandler()
{
  ReleaseJSObjects();
}

void
nsJSScriptTimeoutHandler::ReleaseJSObjects()
{
  if (mFunction) {
    mFunction = nullptr;
    mArgs.Clear();
    mozilla::DropJSObjects(this);
  }
}

const char16_t *
nsJSScriptTimeoutHandler::GetHandlerText()
{
  NS_ASSERTION(!mFunction, "No expression, so no handler text!");
  return mExpr.get();
}

already_AddRefed<nsIScriptTimeoutHandler>
NS_CreateJSTimeoutHandler(nsGlobalWindow *aWindow, Function& aFunction,
                          const Sequence<JS::Value>& aArguments,
                          ErrorResult& aError)
{
  FallibleTArray<JS::Heap<JS::Value> > args;
  if (!args.AppendElements(aArguments, fallible)) {
    aError.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  nsRefPtr<nsJSScriptTimeoutHandler> handler =
    new nsJSScriptTimeoutHandler(aWindow, aFunction, args, aError);
  return aError.Failed() ? nullptr : handler.forget();
}

already_AddRefed<nsIScriptTimeoutHandler>
NS_CreateJSTimeoutHandler(JSContext* aCx, nsGlobalWindow *aWindow,
                          const nsAString& aExpression, ErrorResult& aError)
{
  ErrorResult rv;
  bool allowEval = false;
  nsRefPtr<nsJSScriptTimeoutHandler> handler =
    new nsJSScriptTimeoutHandler(aCx, aWindow, aExpression, &allowEval, rv);
  if (rv.Failed() || !allowEval) {
    return nullptr;
  }

  return handler.forget();
}
