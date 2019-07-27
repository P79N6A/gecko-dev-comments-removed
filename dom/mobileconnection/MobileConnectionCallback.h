



#ifndef mozilla_dom_mobileconnection_MobileConnectionCallback_h
#define mozilla_dom_mobileconnection_MobileConnectionCallback_h

#include "mozilla/dom/DOMRequest.h"
#include "mozilla/dom/mobileconnection/MobileConnectionIPCSerializer.h"
#include "nsCOMPtr.h"
#include "nsIMobileConnectionService.h"

namespace mozilla {
namespace dom {
namespace mobileconnection {











class MobileConnectionCallback MOZ_FINAL : public nsIMobileConnectionCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILECONNECTIONCALLBACK

  MobileConnectionCallback(nsPIDOMWindow* aWindow, DOMRequest* aRequest);

private:
  ~MobileConnectionCallback() {}

  nsresult
  NotifySuccess(JS::Handle<JS::Value> aResult);

  nsresult
  NotifySuccessWithString(const nsAString& aResult);

  nsresult
  NotifySendCancelMmiSuccess(const MozMMIResult& aResult);

  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<DOMRequest> mRequest;
};

} 
} 
} 

#endif 
