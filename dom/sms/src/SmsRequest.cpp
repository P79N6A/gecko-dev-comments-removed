




































#include "SmsRequest.h"
#include "nsIDOMClassInfo.h"
#include "nsDOMString.h"
#include "nsContentUtils.h"
#include "nsIDOMSmsMessage.h"
#include "nsIDOMSmsCursor.h"

DOMCI_DATA(MozSmsRequest, mozilla::dom::sms::SmsRequest)

namespace mozilla {
namespace dom {
namespace sms {

NS_IMPL_CYCLE_COLLECTION_CLASS(SmsRequest)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(SmsRequest,
                                                  nsDOMEventTargetWrapperCache)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(success)
  NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(error)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(SmsRequest,
                                                nsDOMEventTargetWrapperCache)
  if (tmp->mResultRooted) {
    tmp->mResult = JSVAL_VOID;
    tmp->UnrootResult();
  }
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(success)
  NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(error)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(SmsRequest,
                                               nsDOMEventTargetWrapperCache)
  if (JSVAL_IS_GCTHING(tmp->mResult)) {
    void *gcThing = JSVAL_TO_GCTHING(tmp->mResult);
    NS_IMPL_CYCLE_COLLECTION_TRACE_JS_CALLBACK(gcThing, "mResult")
  }
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(SmsRequest)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozSmsRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMozSmsRequest)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozSmsRequest)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetWrapperCache)

NS_IMPL_ADDREF_INHERITED(SmsRequest, nsDOMEventTargetWrapperCache)
NS_IMPL_RELEASE_INHERITED(SmsRequest, nsDOMEventTargetWrapperCache)

NS_IMPL_EVENT_HANDLER(SmsRequest, success)
NS_IMPL_EVENT_HANDLER(SmsRequest, error)

SmsRequest::SmsRequest(nsPIDOMWindow* aWindow, nsIScriptContext* aScriptContext)
  : mResult(JSVAL_VOID)
  , mResultRooted(false)
  , mError(eNoError)
  , mDone(false)
{
  
  mOwner = aWindow;
  mScriptContext = aScriptContext;
}

SmsRequest::~SmsRequest()
{
  if (mResultRooted) {
    UnrootResult();
  }
}

void
SmsRequest::Reset()
{
  NS_ASSERTION(mDone, "mDone should be true if we try to reset!");
  NS_ASSERTION(mResult != JSVAL_VOID, "mResult should be set if we try to reset!");
  NS_ASSERTION(mError == eNoError, "There should be no error if we try to reset!");

  if (mResultRooted) {
    UnrootResult();
  }

  mResult = JSVAL_VOID;
  mDone = false;
}

void
SmsRequest::RootResult()
{
  NS_ASSERTION(!mResultRooted, "Don't call RootResult() if already rooted!");
  NS_HOLD_JS_OBJECTS(this, SmsRequest);
  mResultRooted = true;
}

void
SmsRequest::UnrootResult()
{
  NS_ASSERTION(mResultRooted, "Don't call UnrotResult() if not rooted!");
  NS_DROP_JS_OBJECTS(this, SmsRequest);
  mResultRooted = false;
}

void
SmsRequest::SetSuccess(nsIDOMMozSmsMessage* aMessage)
{
  SetSuccessInternal(aMessage);
}

void
SmsRequest::SetSuccess(bool aResult)
{
  NS_PRECONDITION(!mDone, "mDone shouldn't have been set to true already!");
  NS_PRECONDITION(mError == eNoError, "mError shouldn't have been set!");
  NS_PRECONDITION(mResult == JSVAL_NULL, "mResult shouldn't have been set!");

  mResult.setBoolean(aResult);
  mDone = true;
}

void
SmsRequest::SetSuccess(nsIDOMMozSmsCursor* aCursor)
{
  SetSuccessInternal(aCursor);
}

bool
SmsRequest::SetSuccessInternal(nsISupports* aObject)
{
  NS_PRECONDITION(!mDone, "mDone shouldn't have been set to true already!");
  NS_PRECONDITION(mError == eNoError, "mError shouldn't have been set!");
  NS_PRECONDITION(mResult == JSVAL_VOID, "mResult shouldn't have been set!");

  JSContext* cx = mScriptContext->GetNativeContext();
  NS_ASSERTION(cx, "Failed to get a context!");

  JSObject* global = mScriptContext->GetNativeGlobal();
  NS_ASSERTION(global, "Failed to get global object!");

  JSAutoRequest ar(cx);
  JSAutoEnterCompartment ac;
  if (!ac.enter(cx, global)) {
    SetError(eInternalError);
    return false;
  }

  RootResult();

  if (NS_FAILED(nsContentUtils::WrapNative(cx, global, aObject, &mResult))) {
    UnrootResult();
    mResult = JSVAL_VOID;
    SetError(eInternalError);
    return false;
  }

  mDone = true;
  return true;
}

void
SmsRequest::SetError(ErrorType aError)
{
  NS_PRECONDITION(!mDone, "mDone shouldn't have been set to true already!");
  NS_PRECONDITION(mError == eNoError, "mError shouldn't have been set!");
  NS_PRECONDITION(mResult == JSVAL_VOID, "mResult shouldn't have been set!");

  mDone = true;
  mError = aError;
}

NS_IMETHODIMP
SmsRequest::GetReadyState(nsAString& aReadyState)
{
  if (mDone) {
    aReadyState.AssignLiteral("done");
  } else {
    aReadyState.AssignLiteral("processing");
  }

  return NS_OK;
}

NS_IMETHODIMP
SmsRequest::GetError(nsAString& aError)
{
  if (!mDone) {
    NS_ASSERTION(mError == eNoError,
                 "There should be no error if the request is still processing!");

    SetDOMStringToNull(aError);
    return NS_OK;
  }

  NS_ASSERTION(mError == eNoError || mResult == JSVAL_VOID,
               "mResult should be void when there is an error!");

  switch (mError) {
    case eNoError:
      SetDOMStringToNull(aError);
      break;
    case eNoSignalError:
      aError.AssignLiteral("NoSignalError");
      break;
    case eNotFoundError:
      aError.AssignLiteral("NotFoundError");
      break;
    case eUnknownError:
      aError.AssignLiteral("UnknownError");
      break;
    case eInternalError:
      aError.AssignLiteral("InternalError");
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
SmsRequest::GetResult(jsval* aResult)
{
  if (!mDone) {
    NS_ASSERTION(mResult == JSVAL_VOID,
                 "When not done, result should be null!");

    *aResult = JSVAL_VOID;
    return NS_OK;
  }

  *aResult = mResult;
  return NS_OK;
}

} 
} 
} 
