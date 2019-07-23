





































#ifndef nsScreenManagerCocoa_h_
#define nsScreenManagerCocoa_h_

#import <Cocoa/Cocoa.h>

#include "nsCOMArray.h"

#include "nsIScreenManager.h"

#include "nsScreenCocoa.h"

class nsScreenManagerCocoa : public nsIScreenManager
{
public:
    nsScreenManagerCocoa ();
    ~nsScreenManagerCocoa ();

    NS_DECL_ISUPPORTS

    NS_DECL_NSISCREENMANAGER

private:

    nsScreenCocoa *ScreenForCocoaScreen (NSScreen *screen);

    nsCOMArray<nsScreenCocoa> mScreenList;
};

#endif 
