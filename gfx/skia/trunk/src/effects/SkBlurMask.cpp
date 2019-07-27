








#include "SkBlurMask.h"
#include "SkMath.h"
#include "SkTemplates.h"
#include "SkEndian.h"









static const SkScalar kBLUR_SIGMA_SCALE = 0.57735f;

SkScalar SkBlurMask::ConvertRadiusToSigma(SkScalar radius) {
    return radius > 0 ? kBLUR_SIGMA_SCALE * radius + 0.5f : 0.0f;
}

SkScalar SkBlurMask::ConvertSigmaToRadius(SkScalar sigma) {
    return sigma > 0.5f ? (sigma - 0.5f) / kBLUR_SIGMA_SCALE : 0.0f;
}

#define UNROLL_SEPARABLE_LOOPS













































static int boxBlur(const uint8_t* src, int src_y_stride, uint8_t* dst,
                   int leftRadius, int rightRadius, int width, int height,
                   bool transpose)
{
    int diameter = leftRadius + rightRadius;
    int kernelSize = diameter + 1;
    int border = SkMin32(width, diameter);
    uint32_t scale = (1 << 24) / kernelSize;
    int new_width = width + SkMax32(leftRadius, rightRadius) * 2;
    int dst_x_stride = transpose ? height : 1;
    int dst_y_stride = transpose ? 1 : new_width;
    uint32_t half = 1 << 23;
    for (int y = 0; y < height; ++y) {
        uint32_t sum = 0;
        uint8_t* dptr = dst + y * dst_y_stride;
        const uint8_t* right = src + y * src_y_stride;
        const uint8_t* left = right;
        for (int x = 0; x < rightRadius - leftRadius; x++) {
            *dptr = 0;
            dptr += dst_x_stride;
        }
#define LEFT_BORDER_ITER \
            sum += *right++; \
            *dptr = (sum * scale + half) >> 24; \
            dptr += dst_x_stride;

        int x = 0;
#ifdef UNROLL_SEPARABLE_LOOPS
        for (; x < border - 16; x += 16) {
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
        }
#endif
        for (; x < border; ++x) {
            LEFT_BORDER_ITER
        }
#undef LEFT_BORDER_ITER
#define TRIVIAL_ITER \
            *dptr = (sum * scale + half) >> 24; \
            dptr += dst_x_stride;
        x = width;
#ifdef UNROLL_SEPARABLE_LOOPS
        for (; x < diameter - 16; x += 16) {
            TRIVIAL_ITER
            TRIVIAL_ITER
            TRIVIAL_ITER
            TRIVIAL_ITER
            TRIVIAL_ITER
            TRIVIAL_ITER
            TRIVIAL_ITER
            TRIVIAL_ITER
            TRIVIAL_ITER
            TRIVIAL_ITER
            TRIVIAL_ITER
            TRIVIAL_ITER
            TRIVIAL_ITER
            TRIVIAL_ITER
            TRIVIAL_ITER
            TRIVIAL_ITER
        }
#endif
        for (; x < diameter; ++x) {
            TRIVIAL_ITER
        }
#undef TRIVIAL_ITER
#define CENTER_ITER \
            sum += *right++; \
            *dptr = (sum * scale + half) >> 24; \
            sum -= *left++; \
            dptr += dst_x_stride;

        x = diameter;
#ifdef UNROLL_SEPARABLE_LOOPS
        for (; x < width - 16; x += 16) {
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
        }
#endif
        for (; x < width; ++x) {
            CENTER_ITER
        }
#undef CENTER_ITER
#define RIGHT_BORDER_ITER \
            *dptr = (sum * scale + half) >> 24; \
            sum -= *left++; \
            dptr += dst_x_stride;

        x = 0;
#ifdef UNROLL_SEPARABLE_LOOPS
        for (; x < border - 16; x += 16) {
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
        }
#endif
        for (; x < border; ++x) {
            RIGHT_BORDER_ITER
        }
#undef RIGHT_BORDER_ITER
        for (int x = 0; x < leftRadius - rightRadius; ++x) {
            *dptr = 0;
            dptr += dst_x_stride;
        }
        SkASSERT(sum == 0);
    }
    return new_width;
}






















































static int boxBlurInterp(const uint8_t* src, int src_y_stride, uint8_t* dst,
                         int radius, int width, int height,
                         bool transpose, uint8_t outer_weight)
{
    int diameter = radius * 2;
    int kernelSize = diameter + 1;
    int border = SkMin32(width, diameter);
    int inner_weight = 255 - outer_weight;
    outer_weight += outer_weight >> 7;
    inner_weight += inner_weight >> 7;
    uint32_t outer_scale = (outer_weight << 16) / kernelSize;
    uint32_t inner_scale = (inner_weight << 16) / (kernelSize - 2);
    uint32_t half = 1 << 23;
    int new_width = width + diameter;
    int dst_x_stride = transpose ? height : 1;
    int dst_y_stride = transpose ? 1 : new_width;
    for (int y = 0; y < height; ++y) {
        uint32_t outer_sum = 0, inner_sum = 0;
        uint8_t* dptr = dst + y * dst_y_stride;
        const uint8_t* right = src + y * src_y_stride;
        const uint8_t* left = right;
        int x = 0;

#define LEFT_BORDER_ITER \
            inner_sum = outer_sum; \
            outer_sum += *right++; \
            *dptr = (outer_sum * outer_scale + inner_sum * inner_scale + half) >> 24; \
            dptr += dst_x_stride;

#ifdef UNROLL_SEPARABLE_LOOPS
        for (;x < border - 16; x += 16) {
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
            LEFT_BORDER_ITER
        }
#endif

        for (;x < border; ++x) {
            LEFT_BORDER_ITER
        }
#undef LEFT_BORDER_ITER
        for (int x = width; x < diameter; ++x) {
            *dptr = (outer_sum * outer_scale + inner_sum * inner_scale + half) >> 24;
            dptr += dst_x_stride;
        }
        x = diameter;

#define CENTER_ITER \
            inner_sum = outer_sum - *left; \
            outer_sum += *right++; \
            *dptr = (outer_sum * outer_scale + inner_sum * inner_scale + half) >> 24; \
            dptr += dst_x_stride; \
            outer_sum -= *left++;

#ifdef UNROLL_SEPARABLE_LOOPS
        for (; x < width - 16; x += 16) {
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
            CENTER_ITER
        }
#endif
        for (; x < width; ++x) {
            CENTER_ITER
        }
#undef CENTER_ITER

        #define RIGHT_BORDER_ITER \
            inner_sum = outer_sum - *left++; \
            *dptr = (outer_sum * outer_scale + inner_sum * inner_scale + half) >> 24; \
            dptr += dst_x_stride; \
            outer_sum = inner_sum;

        x = 0;
#ifdef UNROLL_SEPARABLE_LOOPS
        for (; x < border - 16; x += 16) {
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
            RIGHT_BORDER_ITER
        }
#endif
        for (; x < border; ++x) {
            RIGHT_BORDER_ITER
        }
#undef RIGHT_BORDER_ITER
        SkASSERT(outer_sum == 0 && inner_sum == 0);
    }
    return new_width;
}

static void get_adjusted_radii(SkScalar passRadius, int *loRadius, int *hiRadius)
{
    *loRadius = *hiRadius = SkScalarCeilToInt(passRadius);
    if (SkIntToScalar(*hiRadius) - passRadius > 0.5f) {
        *loRadius = *hiRadius - 1;
    }
}

#include "SkColorPriv.h"

static void merge_src_with_blur(uint8_t dst[], int dstRB,
                                const uint8_t src[], int srcRB,
                                const uint8_t blur[], int blurRB,
                                int sw, int sh) {
    dstRB -= sw;
    srcRB -= sw;
    blurRB -= sw;
    while (--sh >= 0) {
        for (int x = sw - 1; x >= 0; --x) {
            *dst = SkToU8(SkAlphaMul(*blur, SkAlpha255To256(*src)));
            dst += 1;
            src += 1;
            blur += 1;
        }
        dst += dstRB;
        src += srcRB;
        blur += blurRB;
    }
}

static void clamp_with_orig(uint8_t dst[], int dstRowBytes,
                            const uint8_t src[], int srcRowBytes,
                            int sw, int sh,
                            SkBlurStyle style) {
    int x;
    while (--sh >= 0) {
        switch (style) {
        case kSolid_SkBlurStyle:
            for (x = sw - 1; x >= 0; --x) {
                int s = *src;
                int d = *dst;
                *dst = SkToU8(s + d - SkMulDiv255Round(s, d));
                dst += 1;
                src += 1;
            }
            break;
        case kOuter_SkBlurStyle:
            for (x = sw - 1; x >= 0; --x) {
                if (*src) {
                    *dst = SkToU8(SkAlphaMul(*dst, SkAlpha255To256(255 - *src)));
                }
                dst += 1;
                src += 1;
            }
            break;
        default:
            SkDEBUGFAIL("Unexpected blur style here");
            break;
        }
        dst += dstRowBytes - sw;
        src += srcRowBytes - sw;
    }
}





void SkMask_FreeImage(uint8_t* image);
void SkMask_FreeImage(uint8_t* image) {
    SkMask::FreeImage(image);
}

bool SkBlurMask::BoxBlur(SkMask* dst, const SkMask& src,
                         SkScalar sigma, SkBlurStyle style, SkBlurQuality quality,
                         SkIPoint* margin, bool force_quality) {

    if (src.fFormat != SkMask::kA8_Format) {
        return false;
    }

    
    if (!force_quality && sigma <= SkIntToScalar(2)) {
        quality = kLow_SkBlurQuality;
    }

    SkScalar passRadius;
    if (kHigh_SkBlurQuality == quality) {
        
        
        passRadius = sigma - (1/6.0f);
    } else {
        
        
        
        passRadius = 1.5f*sigma - 0.5f;
    }

    
    
    int passCount = (kHigh_SkBlurQuality == quality) ? 3 : 1;

    int rx = SkScalarCeilToInt(passRadius);
    int outerWeight = 255 - SkScalarRoundToInt((SkIntToScalar(rx) - passRadius) * 255);

    SkASSERT(rx >= 0);
    SkASSERT((unsigned)outerWeight <= 255);
    if (rx <= 0) {
        return false;
    }

    int ry = rx;    

    int padx = passCount * rx;
    int pady = passCount * ry;

    if (margin) {
        margin->set(padx, pady);
    }
    dst->fBounds.set(src.fBounds.fLeft - padx, src.fBounds.fTop - pady,
                     src.fBounds.fRight + padx, src.fBounds.fBottom + pady);

    dst->fRowBytes = dst->fBounds.width();
    dst->fFormat = SkMask::kA8_Format;
    dst->fImage = NULL;

    if (src.fImage) {
        size_t dstSize = dst->computeImageSize();
        if (0 == dstSize) {
            return false;   
        }

        int             sw = src.fBounds.width();
        int             sh = src.fBounds.height();
        const uint8_t*  sp = src.fImage;
        uint8_t*        dp = SkMask::AllocImage(dstSize);
        SkAutoTCallVProc<uint8_t, SkMask_FreeImage> autoCall(dp);

        
        SkAutoTMalloc<uint8_t>  tmpBuffer(dstSize);
        uint8_t*                tp = tmpBuffer.get();
        int w = sw, h = sh;

        if (outerWeight == 255) {
            int loRadius, hiRadius;
            get_adjusted_radii(passRadius, &loRadius, &hiRadius);
            if (kHigh_SkBlurQuality == quality) {
                
                w = boxBlur(sp, src.fRowBytes, tp, loRadius, hiRadius, w, h, false);
                w = boxBlur(tp, w,             dp, hiRadius, loRadius, w, h, false);
                w = boxBlur(dp, w,             tp, hiRadius, hiRadius, w, h, true);
                
                h = boxBlur(tp, h,             dp, loRadius, hiRadius, h, w, false);
                h = boxBlur(dp, h,             tp, hiRadius, loRadius, h, w, false);
                h = boxBlur(tp, h,             dp, hiRadius, hiRadius, h, w, true);
            } else {
                w = boxBlur(sp, src.fRowBytes, tp, rx, rx, w, h, true);
                h = boxBlur(tp, h,             dp, ry, ry, h, w, true);
            }
        } else {
            if (kHigh_SkBlurQuality == quality) {
                
                w = boxBlurInterp(sp, src.fRowBytes, tp, rx, w, h, false, outerWeight);
                w = boxBlurInterp(tp, w,             dp, rx, w, h, false, outerWeight);
                w = boxBlurInterp(dp, w,             tp, rx, w, h, true, outerWeight);
                
                h = boxBlurInterp(tp, h,             dp, ry, h, w, false, outerWeight);
                h = boxBlurInterp(dp, h,             tp, ry, h, w, false, outerWeight);
                h = boxBlurInterp(tp, h,             dp, ry, h, w, true, outerWeight);
            } else {
                w = boxBlurInterp(sp, src.fRowBytes, tp, rx, w, h, true, outerWeight);
                h = boxBlurInterp(tp, h,             dp, ry, h, w, true, outerWeight);
            }
        }

        dst->fImage = dp;
        
        
        if (style == kInner_SkBlurStyle) {
            
            size_t srcSize = src.computeImageSize();
            if (0 == srcSize) {
                return false;   
            }
            dst->fImage = SkMask::AllocImage(srcSize);
            merge_src_with_blur(dst->fImage, src.fRowBytes,
                                sp, src.fRowBytes,
                                dp + passCount * (rx + ry * dst->fRowBytes),
                                dst->fRowBytes, sw, sh);
            SkMask::FreeImage(dp);
        } else if (style != kNormal_SkBlurStyle) {
            clamp_with_orig(dp + passCount * (rx + ry * dst->fRowBytes),
                            dst->fRowBytes, sp, src.fRowBytes, sw, sh, style);
        }
        (void)autoCall.detach();
    }

    if (style == kInner_SkBlurStyle) {
        dst->fBounds = src.fBounds; 
        dst->fRowBytes = src.fRowBytes;
    }

    return true;
}





































static float gaussianIntegral(float x) {
    if (x > 1.5f) {
        return 0.0f;
    }
    if (x < -1.5f) {
        return 1.0f;
    }

    float x2 = x*x;
    float x3 = x2*x;

    if ( x > 0.5f ) {
        return 0.5625f - (x3 / 6.0f - 3.0f * x2 * 0.25f + 1.125f * x);
    }
    if ( x > -0.5f ) {
        return 0.5f - (0.75f * x - x3 / 3.0f);
    }
    return 0.4375f + (-x3 / 6.0f - 3.0f * x2 * 0.25f - 1.125f * x);
}












void SkBlurMask::ComputeBlurProfile(SkScalar sigma, uint8_t **profile_out) {
    int size = SkScalarCeilToInt(6*sigma);

    int center = size >> 1;
    uint8_t *profile = SkNEW_ARRAY(uint8_t, size);

    float invr = 1.f/(2*sigma);

    profile[0] = 255;
    for (int x = 1 ; x < size ; ++x) {
        float scaled_x = (center - x - .5f) * invr;
        float gi = gaussianIntegral(scaled_x);
        profile[x] = 255 - (uint8_t) (255.f * gi);
    }

    *profile_out = profile;
}








uint8_t SkBlurMask::ProfileLookup(const uint8_t *profile, int loc, int blurred_width, int sharp_width) {
    int dx = SkAbs32(((loc << 1) + 1) - blurred_width) - sharp_width; 
    int ox = dx >> 1;
    if (ox < 0) {
        ox = 0;
    }

    return profile[ox];
}

void SkBlurMask::ComputeBlurredScanline(uint8_t *pixels, const uint8_t *profile,
                                        unsigned int width, SkScalar sigma) {

    unsigned int profile_size = SkScalarCeilToInt(6*sigma);
    SkAutoTMalloc<uint8_t> horizontalScanline(width);

    unsigned int sw = width - profile_size;
    
    
    int center = ( profile_size & ~1 ) - 1;

    int w = sw - center;

    for (unsigned int x = 0 ; x < width ; ++x) {
       if (profile_size <= sw) {
           pixels[x] = ProfileLookup(profile, x, width, w);
       } else {
           float span = float(sw)/(2*sigma);
           float giX = 1.5f - (x+.5f)/(2*sigma);
           pixels[x] = (uint8_t) (255 * (gaussianIntegral(giX) - gaussianIntegral(giX + span)));
       }
    }
}

bool SkBlurMask::BlurRect(SkScalar sigma, SkMask *dst,
                          const SkRect &src, SkBlurStyle style,
                          SkIPoint *margin, SkMask::CreateMode createMode) {
    int profile_size = SkScalarCeilToInt(6*sigma);

    int pad = profile_size/2;
    if (margin) {
        margin->set( pad, pad );
    }

    dst->fBounds.set(SkScalarRoundToInt(src.fLeft - pad),
                     SkScalarRoundToInt(src.fTop - pad),
                     SkScalarRoundToInt(src.fRight + pad),
                     SkScalarRoundToInt(src.fBottom + pad));

    dst->fRowBytes = dst->fBounds.width();
    dst->fFormat = SkMask::kA8_Format;
    dst->fImage = NULL;

    int             sw = SkScalarFloorToInt(src.width());
    int             sh = SkScalarFloorToInt(src.height());

    if (createMode == SkMask::kJustComputeBounds_CreateMode) {
        if (style == kInner_SkBlurStyle) {
            dst->fBounds.set(SkScalarRoundToInt(src.fLeft),
                             SkScalarRoundToInt(src.fTop),
                             SkScalarRoundToInt(src.fRight),
                             SkScalarRoundToInt(src.fBottom)); 
            dst->fRowBytes = sw;
        }
        return true;
    }
    uint8_t *profile = NULL;

    ComputeBlurProfile(sigma, &profile);
    SkAutoTDeleteArray<uint8_t> ada(profile);

    size_t dstSize = dst->computeImageSize();
    if (0 == dstSize) {
        return false;   
    }

    uint8_t*        dp = SkMask::AllocImage(dstSize);

    dst->fImage = dp;

    int dstHeight = dst->fBounds.height();
    int dstWidth = dst->fBounds.width();

    uint8_t *outptr = dp;

    SkAutoTMalloc<uint8_t> horizontalScanline(dstWidth);
    SkAutoTMalloc<uint8_t> verticalScanline(dstHeight);

    ComputeBlurredScanline(horizontalScanline, profile, dstWidth, sigma);
    ComputeBlurredScanline(verticalScanline, profile, dstHeight, sigma);

    for (int y = 0 ; y < dstHeight ; ++y) {
        for (int x = 0 ; x < dstWidth ; x++) {
            unsigned int maskval = SkMulDiv255Round(horizontalScanline[x], verticalScanline[y]);
            *(outptr++) = maskval;
        }
    }

    if (style == kInner_SkBlurStyle) {
        
        size_t srcSize = (size_t)(src.width() * src.height());
        if (0 == srcSize) {
            return false;   
        }
        dst->fImage = SkMask::AllocImage(srcSize);
        for (int y = 0 ; y < sh ; y++) {
            uint8_t *blur_scanline = dp + (y+pad)*dstWidth + pad;
            uint8_t *inner_scanline = dst->fImage + y*sw;
            memcpy(inner_scanline, blur_scanline, sw);
        }
        SkMask::FreeImage(dp);

        dst->fBounds.set(SkScalarRoundToInt(src.fLeft),
                         SkScalarRoundToInt(src.fTop),
                         SkScalarRoundToInt(src.fRight),
                         SkScalarRoundToInt(src.fBottom)); 
        dst->fRowBytes = sw;

    } else if (style == kOuter_SkBlurStyle) {
        for (int y = pad ; y < dstHeight-pad ; y++) {
            uint8_t *dst_scanline = dp + y*dstWidth + pad;
            memset(dst_scanline, 0, sw);
        }
    } else if (style == kSolid_SkBlurStyle) {
        for (int y = pad ; y < dstHeight-pad ; y++) {
            uint8_t *dst_scanline = dp + y*dstWidth + pad;
            memset(dst_scanline, 0xff, sw);
        }
    }
    
    

    return true;
}

bool SkBlurMask::BlurRRect(SkScalar sigma, SkMask *dst,
                           const SkRRect &src, SkBlurStyle style,
                           SkIPoint *margin, SkMask::CreateMode createMode) {
    
    

    return false;
}





bool SkBlurMask::BlurGroundTruth(SkScalar sigma, SkMask* dst, const SkMask& src,
                                 SkBlurStyle style, SkIPoint* margin) {

    if (src.fFormat != SkMask::kA8_Format) {
        return false;
    }

    float variance = sigma * sigma;

    int windowSize = SkScalarCeilToInt(sigma*6);
    
    windowSize |= 1;

    SkAutoTMalloc<float> gaussWindow(windowSize);

    int halfWindow = windowSize >> 1;

    gaussWindow[halfWindow] = 1;

    float windowSum = 1;
    for (int x = 1 ; x <= halfWindow ; ++x) {
        float gaussian = expf(-x*x / (2*variance));
        gaussWindow[halfWindow + x] = gaussWindow[halfWindow-x] = gaussian;
        windowSum += 2*gaussian;
    }

    
    

    int pad = halfWindow;
    if (margin) {
        margin->set( pad, pad );
    }

    dst->fBounds = src.fBounds;
    dst->fBounds.outset(pad, pad);

    dst->fRowBytes = dst->fBounds.width();
    dst->fFormat = SkMask::kA8_Format;
    dst->fImage = NULL;

    if (src.fImage) {

        size_t dstSize = dst->computeImageSize();
        if (0 == dstSize) {
            return false;   
        }

        int             srcWidth = src.fBounds.width();
        int             srcHeight = src.fBounds.height();
        int             dstWidth = dst->fBounds.width();

        const uint8_t*  srcPixels = src.fImage;
        uint8_t*        dstPixels = SkMask::AllocImage(dstSize);
        SkAutoTCallVProc<uint8_t, SkMask_FreeImage> autoCall(dstPixels);

        
        

        int padWidth = srcWidth + 4*pad;
        int padHeight = srcHeight;
        int padSize = padWidth * padHeight;

        SkAutoTMalloc<uint8_t> padPixels(padSize);
        memset(padPixels, 0, padSize);

        for (int y = 0 ; y < srcHeight; ++y) {
            uint8_t* padptr = padPixels + y * padWidth + 2*pad;
            const uint8_t* srcptr = srcPixels + y * srcWidth;
            memcpy(padptr, srcptr, srcWidth);
        }

        
        
        

        int tmpWidth = padHeight + 4*pad;
        int tmpHeight = padWidth - 2*pad;
        int tmpSize = tmpWidth * tmpHeight;

        SkAutoTMalloc<float> tmpImage(tmpSize);
        memset(tmpImage, 0, tmpSize*sizeof(tmpImage[0]));

        for (int y = 0 ; y < padHeight ; ++y) {
            uint8_t *srcScanline = padPixels + y*padWidth;
            for (int x = pad ; x < padWidth - pad ; ++x) {
                float *outPixel = tmpImage + (x-pad)*tmpWidth + y + 2*pad; 
                uint8_t *windowCenter = srcScanline + x;
                for (int i = -pad ; i <= pad ; ++i) {
                    *outPixel += gaussWindow[pad+i]*windowCenter[i];
                }
                *outPixel /= windowSum;
            }
        }

        
        
        

        for (int y = 0 ; y < tmpHeight ; ++y) {
            float *srcScanline = tmpImage + y*tmpWidth;
            for (int x = pad ; x < tmpWidth - pad ; ++x) {
                float *windowCenter = srcScanline + x;
                float finalValue = 0;
                for (int i = -pad ; i <= pad ; ++i) {
                    finalValue += gaussWindow[pad+i]*windowCenter[i];
                }
                finalValue /= windowSum;
                uint8_t *outPixel = dstPixels + (x-pad)*dstWidth + y; 
                int integerPixel = int(finalValue + 0.5f);
                *outPixel = SkClampMax( SkClampPos(integerPixel), 255 );
            }
        }

        dst->fImage = dstPixels;
        
        
        if (style == kInner_SkBlurStyle) {
            
            size_t srcSize = src.computeImageSize();
            if (0 == srcSize) {
                return false;   
            }
            dst->fImage = SkMask::AllocImage(srcSize);
            merge_src_with_blur(dst->fImage, src.fRowBytes,
                srcPixels, src.fRowBytes,
                dstPixels + pad*dst->fRowBytes + pad,
                dst->fRowBytes, srcWidth, srcHeight);
            SkMask::FreeImage(dstPixels);
        } else if (style != kNormal_SkBlurStyle) {
            clamp_with_orig(dstPixels + pad*dst->fRowBytes + pad,
                dst->fRowBytes, srcPixels, src.fRowBytes, srcWidth, srcHeight, style);
        }
        (void)autoCall.detach();
    }

    if (style == kInner_SkBlurStyle) {
        dst->fBounds = src.fBounds; 
        dst->fRowBytes = src.fRowBytes;
    }

    return true;
}
