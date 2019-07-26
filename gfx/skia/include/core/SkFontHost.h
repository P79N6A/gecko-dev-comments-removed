








#ifndef SkFontHost_DEFINED
#define SkFontHost_DEFINED

#include "SkTypeface.h"

class SkDescriptor;
class SkScalerContext;
struct SkScalerContextRec;
class SkStream;
class SkWStream;






























class SK_API SkFontHost {
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

private:
    






    static SkTypeface* CreateTypeface(const SkTypeface* familyFace,
                                      const char familyName[],
                                      SkTypeface::Style style);

    










    static SkTypeface* CreateTypefaceFromStream(SkStream*);

    



    static SkTypeface* CreateTypefaceFromFile(const char path[]);

    

    friend class SkScalerContext;
    friend class SkTypeface;
};

#endif
