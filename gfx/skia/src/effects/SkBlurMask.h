








#ifndef SkBlurMask_DEFINED
#define SkBlurMask_DEFINED

#include "SkShader.h"

class SkBlurMask {
public:
    enum Style {
        kNormal_Style,  
        kSolid_Style,   
        kOuter_Style,   
        kInner_Style,   

        kStyleCount
    };

    enum Quality {
        kLow_Quality,   
        kHigh_Quality   
    };

    static bool Blur(SkMask* dst, const SkMask& src,
                     SkScalar radius, Style style, Quality quality,
                     SkIPoint* margin = NULL);
};

#endif



