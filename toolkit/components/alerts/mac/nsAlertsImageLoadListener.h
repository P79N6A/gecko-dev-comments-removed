



































#ifndef nsAlertsImageLoadListener_h_
#define nsAlertsImageLoadListener_h_

#import "mozGrowlDelegate.h"

#include "nsIStreamLoader.h"
#include "nsStringAPI.h"

class nsAlertsImageLoadListener : public nsIStreamLoaderObserver
{
public:
  nsAlertsImageLoadListener(const nsAString &aName,
                            const nsAString& aAlertTitle,
                            const nsAString& aAlertText,
                            const nsAString& aAlertCookie,
                            PRUint32 aAlertListenerKey);

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLOADEROBSERVER
private:
  nsString mName;
  nsString mAlertTitle;
  nsString mAlertText;
  nsString mAlertCookie;
  PRUint32 mAlertListenerKey;
};

#endif 
