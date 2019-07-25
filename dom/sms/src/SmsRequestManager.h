





#ifndef mozilla_dom_sms_SmsRequestManager_h
#define mozilla_dom_sms_SmsRequestManager_h

#include "nsCOMArray.h"
#include "SmsRequest.h"
#include "nsISmsRequestManager.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace dom {
namespace sms {

class SmsRequestManager MOZ_FINAL : nsISmsRequestManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISMSREQUESTMANAGER

private:
  nsresult DispatchTrustedEventToRequest(const nsAString& aEventName,
                                         nsIDOMMozSmsRequest* aRequest);
  SmsRequest* GetRequest(int32_t aRequestId);

  template <class T>
  nsresult NotifySuccess(int32_t aRequestId, T aParam);
  nsresult NotifyError(int32_t aRequestId, int32_t aError);

  nsCOMArray<nsIDOMMozSmsRequest> mRequests;
};

} 
} 
} 

#endif 
