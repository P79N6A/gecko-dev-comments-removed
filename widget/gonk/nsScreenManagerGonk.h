





































#ifndef nsScreenManagerGonk_h___
#define nsScreenManagerGonk_h___

#include "nsCOMPtr.h"

#include "nsBaseScreen.h"
#include "nsIScreenManager.h"

class nsScreenGonk : public nsBaseScreen
{
public:
    nsScreenGonk(void* nativeScreen);
    ~nsScreenGonk();

    NS_IMETHOD GetRect(PRInt32* aLeft, PRInt32* aTop, PRInt32* aWidth, PRInt32* aHeight);
    NS_IMETHOD GetAvailRect(PRInt32* aLeft, PRInt32* aTop, PRInt32* aWidth, PRInt32* aHeight);
    NS_IMETHOD GetPixelDepth(PRInt32* aPixelDepth);
    NS_IMETHOD GetColorDepth(PRInt32* aColorDepth);
};

class nsScreenManagerGonk : public nsIScreenManager
{
public:
    nsScreenManagerGonk();
    ~nsScreenManagerGonk();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISCREENMANAGER

protected:
    nsCOMPtr<nsIScreen> mOneScreen;
};

#endif 
