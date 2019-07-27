



#ifndef mozilla_dom_MobileConnectionCallback_h
#define mozilla_dom_MobileConnectionCallback_h

#include "mozilla/dom/DOMRequest.h"
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
