






































#ifndef nsCocoaUtils_h_
#define nsCocoaUtils_h_

#import <Cocoa/Cocoa.h>

#include "nsRect.h"



float CocoaScreenCoordsHeight();








NSRect geckoRectToCocoaRect(const nsRect &geckoRect);




nsRect cocoaRectToGeckoRect(const NSRect &cocoaRect);

#endif 
