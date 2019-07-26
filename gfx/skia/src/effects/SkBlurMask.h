








#ifndef SkBlurMask_DEFINED
#define SkBlurMask_DEFINED

#include "SkShader.h"
#include "SkMask.h"

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

    static bool BlurRect(SkMask *dst, const SkRect &src,
                         SkScalar radius, Style style,
                         SkIPoint *margin = NULL,
                         SkMask::CreateMode createMode=SkMask::kComputeBoundsAndRenderImage_CreateMode);
    static bool Blur(SkMask* dst, const SkMask& src,
                     SkScalar radius, Style style, Quality quality,
                     SkIPoint* margin = NULL);
    static bool BlurSeparable(SkMask* dst, const SkMask& src,
                              SkScalar radius, Style style, Quality quality,
                              SkIPoint* margin = NULL);


    
    

    static bool BlurGroundTruth(SkMask* dst, const SkMask& src,
                           SkScalar provided_radius, Style style,
                           SkIPoint* margin = NULL);

private:
    static bool Blur(SkMask* dst, const SkMask& src,
                     SkScalar radius, Style style, Quality quality,
                     SkIPoint* margin, bool separable);
};

#endif
