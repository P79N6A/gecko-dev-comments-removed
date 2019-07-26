







#include "SkBitmap.h"
#include "SkColorPriv.h"
#include "SkBlurImage_opts.h"
#include "SkRect.h"

#include <arm_neon.h>

namespace {

enum BlurDirection {
    kX, kY
};





template<BlurDirection srcDirection>
inline uint8x8_t load_2_pixels(const SkPMColor* src, int srcStride) {
    if (srcDirection == kX) {
        uint32x2_t temp = vdup_n_u32(0);
        
        SK_PREFETCH(src + 16);
        SK_PREFETCH(src + srcStride + 16);
        return vreinterpret_u8_u32(vld1_lane_u32(src + srcStride, vld1_lane_u32(src, temp, 0), 1));
     } else {
         return vld1_u8((uint8_t*)src);
     }
}




template<BlurDirection dstDirection>
inline void store_2_pixels(uint16x8_t result16x8, SkPMColor* dst, int dstStride) {
    if (dstDirection == kX) {
        uint32x2_t temp = vreinterpret_u32_u8(vmovn_u16(result16x8));
        vst1_lane_u32(dst, temp, 0);
        vst1_lane_u32(dst + dstStride, temp, 1);
    } else {
        uint8x8_t temp = vmovn_u16(result16x8);
        vst1_u8((uint8_t*)dst, temp);
    }
}




template<BlurDirection srcDirection, BlurDirection dstDirection>
void SkDoubleRowBoxBlur_NEON(const SkPMColor** src, int srcStride, SkPMColor** dst, int kernelSize,
                        int leftOffset, int rightOffset, int width, int* height)
{
    const int rightBorder = SkMin32(rightOffset + 1, width);
    const int srcStrideX = srcDirection == kX ? 1 : srcStride;
    const int dstStrideX = dstDirection == kX ? 1 : *height;
    const int srcStrideY = srcDirection == kX ? srcStride : 1;
    const int dstStrideY = dstDirection == kX ? width : 1;
    const uint16x8_t scale = vdupq_n_u16((1 << 15) / kernelSize);

    for (; *height >= 2; *height -= 2) {
        uint16x8_t sum = vdupq_n_u16(0);
        const SkPMColor* p = *src;
        for (int i = 0; i < rightBorder; i++) {
            sum = vaddw_u8(sum,
                load_2_pixels<srcDirection>(p, srcStride));
            p += srcStrideX;
        }

        const SkPMColor* sptr = *src;
        SkPMColor* dptr = *dst;
        for (int x = 0; x < width; x++) {
            
            uint16x8_t resultPixels = vreinterpretq_u16_s16(vqrdmulhq_s16(
                vreinterpretq_s16_u16(sum), vreinterpretq_s16_u16(scale)));
            store_2_pixels<dstDirection>(resultPixels, dptr, width);

            if (x >= leftOffset) {
                sum = vsubw_u8(sum,
                    load_2_pixels<srcDirection>(sptr - leftOffset * srcStrideX, srcStride));
            }
            if (x + rightOffset + 1 < width) {
                sum = vaddw_u8(sum,
                    load_2_pixels<srcDirection>(sptr + (rightOffset + 1) * srcStrideX, srcStride));
            }
            sptr += srcStrideX;
            dptr += dstStrideX;
        }
        *src += srcStrideY * 2;
        *dst += dstStrideY * 2;
    }
}







static inline uint16x4_t expand(uint32_t a) {
    
    uint8x8_t v8 = vreinterpret_u8_u32(vdup_n_u32(a));
    
    return vget_low_u16(vmovl_u8(v8));
}

template<BlurDirection srcDirection, BlurDirection dstDirection>
void SkBoxBlur_NEON(const SkPMColor* src, int srcStride, SkPMColor* dst, int kernelSize,
                    int leftOffset, int rightOffset, int width, int height)
{
    const int rightBorder = SkMin32(rightOffset + 1, width);
    const int srcStrideX = srcDirection == kX ? 1 : srcStride;
    const int dstStrideX = dstDirection == kX ? 1 : height;
    const int srcStrideY = srcDirection == kX ? srcStride : 1;
    const int dstStrideY = dstDirection == kX ? width : 1;
    const uint32x4_t scale = vdupq_n_u32((1 << 24) / kernelSize);
    const uint32x4_t half = vdupq_n_u32(1 << 23);

    if (kernelSize < 128)
    {
        SkDoubleRowBoxBlur_NEON<srcDirection, dstDirection>(&src, srcStride, &dst, kernelSize,
            leftOffset, rightOffset, width, &height);
    }

    for (; height > 0; height--) {
        uint32x4_t sum = vdupq_n_u32(0);
        const SkPMColor* p = src;
        for (int i = 0; i < rightBorder; ++i) {
            sum = vaddw_u16(sum, expand(*p));
            p += srcStrideX;
        }

        const SkPMColor* sptr = src;
        SkPMColor* dptr = dst;
        for (int x = 0; x < width; ++x) {
            
            uint32x4_t result = vmlaq_u32(half, sum, scale);

            
            
            uint16x4_t result16 = vqshrn_n_u32(result, 16);

            
            
            uint8x8_t result8 = vqshrn_n_u16(vcombine_u16(result16, result16), 8);

            
            
            vst1_lane_u32(dptr, vreinterpret_u32_u8(result8), 0);

            if (x >= leftOffset) {
                const SkPMColor* l = sptr - leftOffset * srcStrideX;
                sum = vsubw_u16(sum, expand(*l));
            }
            if (x + rightOffset + 1 < width) {
                const SkPMColor* r = sptr + (rightOffset + 1) * srcStrideX;
                sum = vaddw_u16(sum, expand(*r));
            }
            sptr += srcStrideX;
            if (srcDirection == kX) {
                SK_PREFETCH(sptr + (rightOffset + 16) * srcStrideX);
            }
            dptr += dstStrideX;
        }
        src += srcStrideY;
        dst += dstStrideY;
    }
}

} 

bool SkBoxBlurGetPlatformProcs_NEON(SkBoxBlurProc* boxBlurX,
                                    SkBoxBlurProc* boxBlurY,
                                    SkBoxBlurProc* boxBlurXY,
                                    SkBoxBlurProc* boxBlurYX) {
    *boxBlurX = SkBoxBlur_NEON<kX, kX>;
    *boxBlurY = SkBoxBlur_NEON<kY, kY>;
    *boxBlurXY = SkBoxBlur_NEON<kX, kY>;
    *boxBlurYX = SkBoxBlur_NEON<kY, kX>;
    return true;
}
