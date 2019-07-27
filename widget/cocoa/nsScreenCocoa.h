




#ifndef nsScreenCocoa_h_
#define nsScreenCocoa_h_

#import <Cocoa/Cocoa.h>

#include "nsBaseScreen.h"

class nsScreenCocoa : public nsBaseScreen
{
public:
    explicit nsScreenCocoa (NSScreen *screen);
    ~nsScreenCocoa ();

    NS_IMETHOD GetId(uint32_t* outId);
    NS_IMETHOD GetRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
    NS_IMETHOD GetAvailRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
    NS_IMETHOD GetRectDisplayPix(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
    NS_IMETHOD GetAvailRectDisplayPix(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
    NS_IMETHOD GetPixelDepth(int32_t* aPixelDepth);
    NS_IMETHOD GetColorDepth(int32_t* aColorDepth);
    NS_IMETHOD GetContentsScaleFactor(double* aContentsScaleFactor);

    NSScreen *CocoaScreen() { return mScreen; }

private:
    CGFloat BackingScaleFactor();

    NSScreen *mScreen;
    uint32_t mId;
};

#endif 
