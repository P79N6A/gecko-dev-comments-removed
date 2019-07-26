




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

  MobileMessageCallback(DOMRequest* aDOMRequest);

private:
  ~MobileMessageCallback();

  nsRefPtr<DOMRequest> mDOMRequest;

  nsresult NotifySuccess(const jsval& aResult);
  nsresult NotifySuccess(nsISupports *aMessage);
  nsresult NotifyError(int32_t aError);
};

} 
} 
} 

#endif 
