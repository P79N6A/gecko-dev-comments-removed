








#ifndef SkBlurMask_DEFINED
#define SkBlurMask_DEFINED

#include "SkShader.h"
#include "SkMask.h"
#include "SkRRect.h"

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

    static bool BlurRect(SkScalar sigma, SkMask *dst, const SkRect &src,
                         Style style,
                         SkIPoint *margin = NULL,
                         SkMask::CreateMode createMode =
                                                SkMask::kComputeBoundsAndRenderImage_CreateMode);
    static bool BlurRRect(SkScalar sigma, SkMask *dst, const SkRRect &src,
                         Style style,
                         SkIPoint *margin = NULL,
                         SkMask::CreateMode createMode =
                                                SkMask::kComputeBoundsAndRenderImage_CreateMode);
    static bool BoxBlur(SkMask* dst, const SkMask& src,
                        SkScalar sigma, Style style, Quality quality,
                        SkIPoint* margin = NULL);

    
    
    static bool BlurGroundTruth(SkScalar sigma, SkMask* dst, const SkMask& src,
                                Style style,
                                SkIPoint* margin = NULL);

    static SkScalar ConvertRadiusToSigma(SkScalar radius);

    

    







    static uint8_t ProfileLookup(const uint8_t* profile, int loc, int blurred_width, int sharp_width);

    





    static void ComputeBlurProfile(SkScalar sigma, uint8_t** profile_out);

    








    static void ComputeBlurredScanline(uint8_t* pixels, const uint8_t* profile,
                                       unsigned int width, SkScalar sigma);



};

#endif
