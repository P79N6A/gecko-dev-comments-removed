




































#include "SmsRequestManager.h"
#include "nsIDOMSmsMessage.h"
#include "nsDOMEvent.h"
#include "SmsCursor.h"





#define SUCCESS_EVENT_NAME NS_LITERAL_STRING("success")
#define ERROR_EVENT_NAME   NS_LITERAL_STRING("error")

namespace mozilla {
namespace dom {
namespace sms {

SmsRequestManager* SmsRequestManager::sInstance = nsnull;

void
SmsRequestManager::Init()
{
  NS_PRECONDITION(!sInstance,
                  "sInstance shouldn't be set. Did you call Init() twice?");
  sInstance = new SmsRequestManager();
}

void
SmsRequestManager::Shutdown()
{
  NS_PRECONDITION(sInstance, "sInstance should be set. Did you call Init()?");

  delete sInstance;
  sInstance = nsnull;
}

 SmsRequestManager*
SmsRequestManager::GetInstance()
{
  return sInstance;
}

PRInt32
SmsRequestManager::AddRequest(nsIDOMMozSmsRequest* aRequest)
{
  
  PRInt32 size = mRequests.Count();

  
  for (PRInt32 i=0; i<size; ++i) {
    if (mRequests[i]) {
      continue;
    }

    mRequests.ReplaceObjectAt(aRequest, i);
    return i;
  }

  mRequests.AppendObject(aRequest);
  return size;
}

PRInt32
SmsRequestManager::CreateRequest(nsPIDOMWindow* aWindow,
                                 nsIScriptContext* aScriptContext,
                                 nsIDOMMozSmsRequest** aRequest)
{
  nsCOMPtr<nsIDOMMozSmsRequest> request =
    new SmsRequest(aWindow, aScriptContext);

  PRInt32 size = mRequests.Count();

  
  for (PRInt32 i=0; i<size; ++i) {
    if (mRequests[i]) {
      continue;
    }

    mRequests.ReplaceObjectAt(request, i);
    NS_ADDREF(*aRequest = request);
    return i;
  }

  mRequests.AppendObject(request);
  NS_ADDREF(*aRequest = request);
  return size;
}

nsresult
SmsRequestManager::DispatchTrustedEventToRequest(const nsAString& aEventName,
                                                 nsIDOMMozSmsRequest* aRequest)
{
  nsRefPtr<nsDOMEvent> event = new nsDOMEvent(nsnull, nsnull);
  nsresult rv = event->InitEvent(aEventName, false, false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = event->SetTrusted(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  bool dummy;
  return aRequest->DispatchEvent(event, &dummy);
}

SmsRequest*
SmsRequestManager::GetRequest(PRInt32 aRequestId)
{
  NS_ASSERTION(mRequests.Count() > aRequestId && mRequests[aRequestId],
               "Got an invalid request id or it has been already deleted!");

  
  
  return static_cast<SmsRequest*>(mRequests[aRequestId]);
}

template <class T>
void
SmsRequestManager::NotifySuccess(PRInt32 aRequestId, T aParam)
{
  SmsRequest* request = GetRequest(aRequestId);
  request->SetSuccess(aParam);

  DispatchTrustedEventToRequest(SUCCESS_EVENT_NAME, request);

  mRequests.ReplaceObjectAt(nsnull, aRequestId);
}

void
SmsRequestManager::NotifyError(PRInt32 aRequestId, SmsRequest::ErrorType aError)
{
  SmsRequest* request = GetRequest(aRequestId);
  request->SetError(aError);

  DispatchTrustedEventToRequest(ERROR_EVENT_NAME, request);

  mRequests.ReplaceObjectAt(nsnull, aRequestId);
}

void
SmsRequestManager::NotifySmsSent(PRInt32 aRequestId, nsIDOMMozSmsMessage* aMessage)
{
  NotifySuccess<nsIDOMMozSmsMessage*>(aRequestId, aMessage);
}

void
SmsRequestManager::NotifySmsSendFailed(PRInt32 aRequestId, SmsRequest::ErrorType aError)
{
  NotifyError(aRequestId, aError);
}

void
SmsRequestManager::NotifyGotSms(PRInt32 aRequestId, nsIDOMMozSmsMessage* aMessage)
{
  NotifySuccess<nsIDOMMozSmsMessage*>(aRequestId, aMessage);
}

void
SmsRequestManager::NotifyGetSmsFailed(PRInt32 aRequestId,
                                      SmsRequest::ErrorType aError)
{
  NotifyError(aRequestId, aError);
}

void
SmsRequestManager::NotifySmsDeleted(PRInt32 aRequestId, bool aDeleted)
{
  NotifySuccess<bool>(aRequestId, aDeleted);
}

void
SmsRequestManager::NotifySmsDeleteFailed(PRInt32 aRequestId, SmsRequest::ErrorType aError)
{
  NotifyError(aRequestId, aError);
}

void
SmsRequestManager::NotifyNoMessageInList(PRInt32 aRequestId)
{
  
  nsCOMPtr<nsIDOMMozSmsCursor> cursor = new SmsCursor(nsnull);

  NotifySuccess<nsIDOMMozSmsCursor*>(aRequestId, cursor);
}

void
SmsRequestManager::NotifyCreateMessageList(PRInt32 aRequestId, PRInt32 aListId,
                                           nsIDOMMozSmsMessage* aMessage)
{
  
  SmsRequest* request = GetRequest(aRequestId);

  nsCOMPtr<SmsCursor> cursor = new SmsCursor(aListId, nsnull, request);
  cursor->SetMessage(aMessage);

  NotifySuccess<nsIDOMMozSmsCursor*>(aRequestId, cursor);
}

} 
} 
} 
