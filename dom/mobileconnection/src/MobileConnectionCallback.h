



#ifndef mozilla_dom_MobileConnectionCallback_h
#define mozilla_dom_MobileConnectionCallback_h

#include "mozilla/dom/DOMRequest.h"
#include "mozilla/dom/MobileConnectionIPCSerializer.h"
#include "nsCOMPtr.h"
#include "nsIMobileConnectionService.h"

namespace mozilla {
namespace dom {











class MobileConnectionCallback MOZ_FINAL : public nsIMobileConnectionCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILECONNECTIONCALLBACK

  MobileConnectionCallback(nsPIDOMWindow* aWindow, DOMRequest* aRequest);

  


  nsresult
  NotifySendCancelMmiSuccess(const nsAString& aServiceCode,
                             const nsAString& aStatusMessage);
  nsresult
  NotifySendCancelMmiSuccess(const nsAString& aServiceCode,
                             const nsAString& aStatusMessage,
                             JS::Handle<JS::Value> aAdditionalInformation);
  nsresult
  NotifySendCancelMmiSuccess(const nsAString& aServiceCode,
                             const nsAString& aStatusMessage,
                             uint16_t aAdditionalInformation);
  nsresult
  NotifySendCancelMmiSuccess(const nsAString& aServiceCode,
                             const nsAString& aStatusMessage,
                             const nsTArray<nsString>& aAdditionalInformation);
  nsresult
  NotifySendCancelMmiSuccess(const nsAString& aServiceCode,
                             const nsAString& aStatusMessage,
                             const nsTArray<IPC::MozCallForwardingOptions>& aAdditionalInformation);
  nsresult
  NotifySendCancelMmiSuccess(const MozMMIResult& aResult);

  


  nsresult
  NotifyGetCallForwardingSuccess(const nsTArray<IPC::MozCallForwardingOptions>& aResults);

private:
  ~MobileConnectionCallback() {}

  nsresult
  NotifySuccess(JS::Handle<JS::Value> aResult);

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<DOMRequest> mRequest;
};

} 
} 

#endif 
