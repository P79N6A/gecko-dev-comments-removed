





































#ifndef mozilla_dom_sms_SmsRequestManager_h
#define mozilla_dom_sms_SmsRequestManager_h

#include "nsCOMArray.h"
#include "SmsRequest.h"

class nsIDOMMozSmsRequest;
class nsPIDOMWindow;
class nsIScriptContext;
class nsIDOMMozSmsMessage;

namespace mozilla {
namespace dom {
namespace sms {

class SmsRequestManager
{
public:
  static void Init();
  static void Shutdown();
  static SmsRequestManager* GetInstance();

  PRInt32 CreateRequest(nsPIDOMWindow* aWindow,
                        nsIScriptContext* aScriptContext,
                        nsIDOMMozSmsRequest** aRequest);

  PRInt32 AddRequest(nsIDOMMozSmsRequest* aRequest);

  void NotifySmsSent(PRInt32 aRequestId, nsIDOMMozSmsMessage* aMessage);
  void NotifySmsSendFailed(PRInt32 aRequestId, SmsRequest::ErrorType aError);
  void NotifyGotSms(PRInt32 aRequestId, nsIDOMMozSmsMessage* aMessage);
  void NotifyGetSmsFailed(PRInt32 aRequestId, SmsRequest::ErrorType aError);
  void NotifySmsDeleted(PRInt32 aRequestId, bool aDeleted);
  void NotifySmsDeleteFailed(PRInt32 aRequestId, SmsRequest::ErrorType aError);
  void NotifyNoMessageInList(PRInt32 aRequestId);
  void NotifyCreateMessageList(PRInt32 aRequestId, PRInt32 aListId, nsIDOMMozSmsMessage* aMessage);

private:
  static SmsRequestManager* sInstance;

  nsresult DispatchTrustedEventToRequest(const nsAString& aEventName,
                                         nsIDOMMozSmsRequest* aRequest);
  SmsRequest* GetRequest(PRInt32 aRequestId);

  template <class T>
  void NotifySuccess(PRInt32 aRequestId, T aParam);
  void NotifyError(PRInt32 aRequestId, SmsRequest::ErrorType aError);

  nsCOMArray<nsIDOMMozSmsRequest> mRequests;
};

} 
} 
} 

#endif 
