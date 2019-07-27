




#ifndef mozilla_dom_mobilemessage_MobileMessageCallback_h
#define mozilla_dom_mobilemessage_MobileMessageCallback_h

#include "nsIMobileMessageCallback.h"
#include "nsCOMPtr.h"
#include "DOMRequest.h"

class nsIDOMMozMmsMessage;

namespace mozilla {
namespace dom {
namespace mobilemessage {

class MobileMessageCallback MOZ_FINAL : public nsIMobileMessageCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILEMESSAGECALLBACK

  explicit MobileMessageCallback(DOMRequest* aDOMRequest);

private:
  ~MobileMessageCallback();

  nsRefPtr<DOMRequest> mDOMRequest;

  nsresult NotifySuccess(JS::Handle<JS::Value> aResult, bool aAsync = false);
  nsresult NotifySuccess(nsISupports *aMessage, bool aAsync = false);
  nsresult NotifyError(int32_t aError, DOMError *aDetailedError = nullptr, bool aAsync = false);
};

} 
} 
} 

#endif 
