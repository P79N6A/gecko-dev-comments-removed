






#ifndef SkFontLCDConfig_DEFINED
#define SkFontLCDConfig_DEFINED

#include "SkTypes.h"

class SkFontLCDConfig {
public:
    








    enum LCDOrientation {
        kHorizontal_LCDOrientation = 0,    
        kVertical_LCDOrientation   = 1
    };

    
    static void SetSubpixelOrientation(LCDOrientation orientation);
    
    static LCDOrientation GetSubpixelOrientation();

    











    enum LCDOrder {
        kRGB_LCDOrder = 0,    
        kBGR_LCDOrder = 1,
        kNONE_LCDOrder = 2
    };

    
    static void SetSubpixelOrder(LCDOrder order);
    
    static LCDOrder GetSubpixelOrder();
};

#endif
