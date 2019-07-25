




#ifndef nsScreenManagerAndroid_h___
#define nsScreenManagerAndroid_h___

#include "nsCOMPtr.h"

#include "nsBaseScreen.h"
#include "nsIScreenManager.h"
#include "WidgetUtils.h"

class nsScreenAndroid : public nsBaseScreen
{
public:
    nsScreenAndroid(void *nativeScreen);
    ~nsScreenAndroid();

    NS_IMETHOD GetRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
    NS_IMETHOD GetAvailRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
    NS_IMETHOD GetPixelDepth(int32_t* aPixelDepth);
    NS_IMETHOD GetColorDepth(int32_t* aColorDepth);

protected:
    virtual void ApplyMinimumBrightness(uint32_t aBrightness) MOZ_OVERRIDE;
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
