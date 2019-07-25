





































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

  void NotifySmsSent(PRInt32 aRequestId, nsIDOMMozSmsMessage* aMessage);
  void NotifySmsSendFailed(PRInt32 aRequestId, SmsRequest::ErrorType aError);

private:
  static SmsRequestManager* sInstance;

  nsresult DispatchTrustedEventToRequest(const nsAString& aEventName,
                                         nsIDOMMozSmsRequest* aRequest);

  nsCOMArray<nsIDOMMozSmsRequest> mRequests;
};

} 
} 
} 

#endif 
