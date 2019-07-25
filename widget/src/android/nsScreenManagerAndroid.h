





































#ifndef nsScreenManagerAndroid_h___
#define nsScreenManagerAndroid_h___

#include "nsCOMPtr.h"

#include "nsIScreenManager.h"
#include "nsIScreen.h"

class nsScreenAndroid :
    public nsIScreen
{
public:
    nsScreenAndroid(void *platformScreen);
    ~nsScreenAndroid();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISCREEN
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
