





































#ifndef mozilla_dom_sms_SmsRequestManager_h
#define mozilla_dom_sms_SmsRequestManager_h

#include "nsCOMArray.h"
#include "SmsRequest.h"
#include "nsISmsRequestManager.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsRequestManager : nsISmsRequestManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISMSREQUESTMANAGER

private:
  nsresult DispatchTrustedEventToRequest(const nsAString& aEventName,
                                         nsIDOMMozSmsRequest* aRequest);
  SmsRequest* GetRequest(PRInt32 aRequestId);

  template <class T>
  nsresult NotifySuccess(PRInt32 aRequestId, T aParam);
  nsresult NotifyError(PRInt32 aRequestId, PRInt32 aError);

  nsCOMArray<nsIDOMMozSmsRequest> mRequests;
};

} 
} 
} 

#endif 
