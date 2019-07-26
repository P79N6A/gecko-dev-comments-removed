



#ifndef nsAlertsImageLoadListener_h_
#define nsAlertsImageLoadListener_h_

#import "mozGrowlDelegate.h"

#include "nsIStreamLoader.h"
#include "nsStringAPI.h"
#include "mozilla/Attributes.h"

class nsAlertsImageLoadListener MOZ_FINAL : public nsIStreamLoaderObserver
{
public:
  nsAlertsImageLoadListener(const nsAString &aName,
                            const nsAString& aAlertTitle,
                            const nsAString& aAlertText,
                            const nsAString& aAlertCookie,
                            uint32_t aAlertListenerKey);

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLOADEROBSERVER
private:
  nsString mName;
  nsString mAlertTitle;
  nsString mAlertText;
  nsString mAlertCookie;
  uint32_t mAlertListenerKey;
};

#endif 
