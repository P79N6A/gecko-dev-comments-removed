





































#ifndef nsScreenCocoa_h_
#define nsScreenCocoa_h_

#import <Cocoa/Cocoa.h>

#include "nsBaseScreen.h"

class nsScreenCocoa : public nsBaseScreen
{
public:
    nsScreenCocoa (NSScreen *screen);
    ~nsScreenCocoa ();

    NS_IMETHOD GetRect(PRInt32* aLeft, PRInt32* aTop, PRInt32* aWidth, PRInt32* aHeight);
    NS_IMETHOD GetAvailRect(PRInt32* aLeft, PRInt32* aTop, PRInt32* aWidth, PRInt32* aHeight);
    NS_IMETHOD GetPixelDepth(PRInt32* aPixelDepth);
    NS_IMETHOD GetColorDepth(PRInt32* aColorDepth);

    NSScreen *CocoaScreen() { return mScreen; }

private:
    NSScreen *mScreen;
};

#endif 
