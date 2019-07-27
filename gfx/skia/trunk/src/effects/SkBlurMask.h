






#ifndef SkBlurMask_DEFINED
#define SkBlurMask_DEFINED

#include "SkBlurTypes.h"
#include "SkShader.h"
#include "SkMask.h"
#include "SkRRect.h"

class SkBlurMask {
public:
    static bool BlurRect(SkScalar sigma, SkMask *dst, const SkRect &src, SkBlurStyle,
                         SkIPoint *margin = NULL,
                         SkMask::CreateMode createMode =
                                                SkMask::kComputeBoundsAndRenderImage_CreateMode);
    static bool BlurRRect(SkScalar sigma, SkMask *dst, const SkRRect &src, SkBlurStyle,
                         SkIPoint *margin = NULL,
                         SkMask::CreateMode createMode =
                                                SkMask::kComputeBoundsAndRenderImage_CreateMode);

    
    
    
    
    

    static bool BoxBlur(SkMask* dst, const SkMask& src,
                        SkScalar sigma, SkBlurStyle style, SkBlurQuality quality,
                        SkIPoint* margin = NULL, bool force_quality=false);

    
    
    static bool BlurGroundTruth(SkScalar sigma, SkMask* dst, const SkMask& src, SkBlurStyle,
                                SkIPoint* margin = NULL);

    
    static SkScalar ConvertRadiusToSigma(SkScalar radius);
    
    static SkScalar ConvertSigmaToRadius(SkScalar sigma);

    

    







    static uint8_t ProfileLookup(const uint8_t* profile, int loc, int blurred_width, int sharp_width);

    





    static void ComputeBlurProfile(SkScalar sigma, uint8_t** profile_out);

    








    static void ComputeBlurredScanline(uint8_t* pixels, const uint8_t* profile,
                                       unsigned int width, SkScalar sigma);



};

#endif
