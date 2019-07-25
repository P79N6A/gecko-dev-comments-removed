





































#ifndef nsScreenCocoa_h_
#define nsScreenCocoa_h_

#import <Cocoa/Cocoa.h>

#include "nsIScreen.h"

class nsScreenCocoa : public nsIScreen
{
public:
    nsScreenCocoa (NSScreen *screen);
    ~nsScreenCocoa ();

    NS_DECL_ISUPPORTS

    NS_DECL_NSISCREEN

    NSScreen *CocoaScreen() { return mScreen; }

private:
    NSScreen *mScreen;
};

#endif 
