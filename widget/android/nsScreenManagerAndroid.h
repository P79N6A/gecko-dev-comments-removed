





































#ifndef nsScreenManagerAndroid_h___
#define nsScreenManagerAndroid_h___

#include "nsCOMPtr.h"

#include "nsIScreenManager.h"
#include "nsIScreen.h"
#include "WidgetUtils.h"

class nsScreenAndroid
  : public nsIScreen
  , public mozilla::widget::BrightnessLockingWidget
{
public:
    nsScreenAndroid(void *nativeScreen);
    ~nsScreenAndroid();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISCREEN

protected:
    void ApplyMinimumBrightness(PRUint32 aBrightness);
};

class nsScreenManagerAndroid :
    public nsIScreenManager
{
public:
    nsScreenManagerAndroid();
    ~nsScreenManagerAndroid();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISCREENMANAGER

protected:
    nsCOMPtr<nsIScreen> mOneScreen;
};

#endif 
