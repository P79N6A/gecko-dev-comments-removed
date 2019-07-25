





































#ifndef nsScreenManagerGonk_h___
#define nsScreenManagerGonk_h___

#include "nsCOMPtr.h"

#include "nsIScreenManager.h"
#include "nsIScreen.h"
#include "WidgetUtils.h"

class nsScreenGonk : public nsIScreen
{
public:
    nsScreenGonk(void *nativeScreen);
    ~nsScreenGonk();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISCREEN
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
