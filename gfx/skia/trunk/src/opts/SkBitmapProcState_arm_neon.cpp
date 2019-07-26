






#include "SkBitmapProcState.h"
#include "SkBitmapProcState_filter.h"
#include "SkColorPriv.h"
#include "SkFilterProc.h"
#include "SkPaint.h"
#include "SkShader.h"   
#include "SkUtilsArm.h"


extern const SkBitmapProcState::SampleProc32 gSkBitmapProcStateSample32_neon[];
extern const SkBitmapProcState::SampleProc16 gSkBitmapProcStateSample16_neon[];

#define   NAME_WRAP(x)  x ## _neon
#include "SkBitmapProcState_filter_neon.h"
#include "SkBitmapProcState_procs.h"

const SkBitmapProcState::SampleProc32 gSkBitmapProcStateSample32_neon[] = {
    S32_opaque_D32_nofilter_DXDY_neon,
    S32_alpha_D32_nofilter_DXDY_neon,
    S32_opaque_D32_nofilter_DX_neon,
    S32_alpha_D32_nofilter_DX_neon,
    S32_opaque_D32_filter_DXDY_neon,
    S32_alpha_D32_filter_DXDY_neon,
    S32_opaque_D32_filter_DX_neon,
    S32_alpha_D32_filter_DX_neon,

    S16_opaque_D32_nofilter_DXDY_neon,
    S16_alpha_D32_nofilter_DXDY_neon,
    S16_opaque_D32_nofilter_DX_neon,
    S16_alpha_D32_nofilter_DX_neon,
    S16_opaque_D32_filter_DXDY_neon,
    S16_alpha_D32_filter_DXDY_neon,
    S16_opaque_D32_filter_DX_neon,
    S16_alpha_D32_filter_DX_neon,

    SI8_opaque_D32_nofilter_DXDY_neon,
    SI8_alpha_D32_nofilter_DXDY_neon,
    SI8_opaque_D32_nofilter_DX_neon,
    SI8_alpha_D32_nofilter_DX_neon,
    SI8_opaque_D32_filter_DXDY_neon,
    SI8_alpha_D32_filter_DXDY_neon,
    SI8_opaque_D32_filter_DX_neon,
    SI8_alpha_D32_filter_DX_neon,

    S4444_opaque_D32_nofilter_DXDY_neon,
    S4444_alpha_D32_nofilter_DXDY_neon,
    S4444_opaque_D32_nofilter_DX_neon,
    S4444_alpha_D32_nofilter_DX_neon,
    S4444_opaque_D32_filter_DXDY_neon,
    S4444_alpha_D32_filter_DXDY_neon,
    S4444_opaque_D32_filter_DX_neon,
    S4444_alpha_D32_filter_DX_neon,

    
    SA8_alpha_D32_nofilter_DXDY_neon,
    SA8_alpha_D32_nofilter_DXDY_neon,
    SA8_alpha_D32_nofilter_DX_neon,
    SA8_alpha_D32_nofilter_DX_neon,
    SA8_alpha_D32_filter_DXDY_neon,
    SA8_alpha_D32_filter_DXDY_neon,
    SA8_alpha_D32_filter_DX_neon,
    SA8_alpha_D32_filter_DX_neon
};

const SkBitmapProcState::SampleProc16 gSkBitmapProcStateSample16_neon[] = {
    S32_D16_nofilter_DXDY_neon,
    S32_D16_nofilter_DX_neon,
    S32_D16_filter_DXDY_neon,
    S32_D16_filter_DX_neon,

    S16_D16_nofilter_DXDY_neon,
    S16_D16_nofilter_DX_neon,
    S16_D16_filter_DXDY_neon,
    S16_D16_filter_DX_neon,

    SI8_D16_nofilter_DXDY_neon,
    SI8_D16_nofilter_DX_neon,
    SI8_D16_filter_DXDY_neon,
    SI8_D16_filter_DX_neon,

    
    NULL, NULL, NULL, NULL,
    
    NULL, NULL, NULL, NULL
};



#include <arm_neon.h>
#include "SkConvolver.h"



void convolveHorizontally_neon(const unsigned char* srcData,
                               const SkConvolutionFilter1D& filter,
                               unsigned char* outRow,
                               bool hasAlpha) {
    
    int numValues = filter.numValues();
    for (int outX = 0; outX < numValues; outX++) {
        uint8x8_t coeff_mask0 = vcreate_u8(0x0100010001000100);
        uint8x8_t coeff_mask1 = vcreate_u8(0x0302030203020302);
        uint8x8_t coeff_mask2 = vcreate_u8(0x0504050405040504);
        uint8x8_t coeff_mask3 = vcreate_u8(0x0706070607060706);
        
        int filterOffset, filterLength;
        const SkConvolutionFilter1D::ConvolutionFixed* filterValues =
            filter.FilterForValue(outX, &filterOffset, &filterLength);

        
        
        const unsigned char* rowToFilter = &srcData[filterOffset * 4];

        
        int32x4_t accum = vdupq_n_s32(0);
        for (int filterX = 0; filterX < filterLength >> 2; filterX++) {
            
            int16x4_t coeffs, coeff0, coeff1, coeff2, coeff3;
            coeffs = vld1_s16(filterValues);
            coeff0 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_s16(coeffs), coeff_mask0));
            coeff1 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_s16(coeffs), coeff_mask1));
            coeff2 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_s16(coeffs), coeff_mask2));
            coeff3 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_s16(coeffs), coeff_mask3));

            
            uint8x16_t pixels = vld1q_u8(rowToFilter);
            int16x8_t p01_16 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(pixels)));
            int16x8_t p23_16 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(pixels)));

            int16x4_t p0_src = vget_low_s16(p01_16);
            int16x4_t p1_src = vget_high_s16(p01_16);
            int16x4_t p2_src = vget_low_s16(p23_16);
            int16x4_t p3_src = vget_high_s16(p23_16);

            int32x4_t p0 = vmull_s16(p0_src, coeff0);
            int32x4_t p1 = vmull_s16(p1_src, coeff1);
            int32x4_t p2 = vmull_s16(p2_src, coeff2);
            int32x4_t p3 = vmull_s16(p3_src, coeff3);

            accum += p0;
            accum += p1;
            accum += p2;
            accum += p3;

            
            rowToFilter += 16;
            filterValues += 4;
        }
        int r = filterLength & 3;
        if (r) {
            const uint16_t mask[4][4] = {
                {0, 0, 0, 0},
                {0xFFFF, 0, 0, 0},
                {0xFFFF, 0xFFFF, 0, 0},
                {0xFFFF, 0xFFFF, 0xFFFF, 0}
            };
            uint16x4_t coeffs;
            int16x4_t coeff0, coeff1, coeff2;
            coeffs = vld1_u16(reinterpret_cast<const uint16_t*>(filterValues));
            coeffs &= vld1_u16(&mask[r][0]);
            coeff0 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_u16(coeffs), coeff_mask0));
            coeff1 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_u16(coeffs), coeff_mask1));
            coeff2 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_u16(coeffs), coeff_mask2));

            
            uint8x16_t pixels = vld1q_u8(rowToFilter);
            int16x8_t p01_16 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(pixels)));
            int16x8_t p23_16 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(pixels)));
            int32x4_t p0 = vmull_s16(vget_low_s16(p01_16), coeff0);
            int32x4_t p1 = vmull_s16(vget_high_s16(p01_16), coeff1);
            int32x4_t p2 = vmull_s16(vget_low_s16(p23_16), coeff2);

            accum += p0;
            accum += p1;
            accum += p2;
        }

        
        
        accum = vshrq_n_s32(accum, SkConvolutionFilter1D::kShiftBits);

        
        int16x4_t accum16 = vqmovn_s32(accum);
        uint8x8_t accum8 = vqmovun_s16(vcombine_s16(accum16, accum16));
        vst1_lane_u32(reinterpret_cast<uint32_t*>(outRow), vreinterpret_u32_u8(accum8), 0);
        outRow += 4;
    }
}







template<bool hasAlpha>
void convolveVertically_neon(const SkConvolutionFilter1D::ConvolutionFixed* filterValues,
                             int filterLength,
                             unsigned char* const* sourceDataRows,
                             int pixelWidth,
                             unsigned char* outRow) {
    int width = pixelWidth & ~3;

    int32x4_t accum0, accum1, accum2, accum3;
    int16x4_t coeff16;

    
    for (int outX = 0; outX < width; outX += 4) {

        
        accum0 = accum1 = accum2 = accum3 = vdupq_n_s32(0);

        
        for (int filterY = 0; filterY < filterLength; filterY++) {

            
            
            coeff16 = vdup_n_s16(filterValues[filterY]);

            
            
            uint8x16_t src8 = vld1q_u8(&sourceDataRows[filterY][outX << 2]);

            int16x8_t src16_01 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(src8)));
            int16x8_t src16_23 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(src8)));
            int16x4_t src16_0 = vget_low_s16(src16_01);
            int16x4_t src16_1 = vget_high_s16(src16_01);
            int16x4_t src16_2 = vget_low_s16(src16_23);
            int16x4_t src16_3 = vget_high_s16(src16_23);

            accum0 += vmull_s16(src16_0, coeff16);
            accum1 += vmull_s16(src16_1, coeff16);
            accum2 += vmull_s16(src16_2, coeff16);
            accum3 += vmull_s16(src16_3, coeff16);
        }

        
        accum0 = vshrq_n_s32(accum0, SkConvolutionFilter1D::kShiftBits);
        accum1 = vshrq_n_s32(accum1, SkConvolutionFilter1D::kShiftBits);
        accum2 = vshrq_n_s32(accum2, SkConvolutionFilter1D::kShiftBits);
        accum3 = vshrq_n_s32(accum3, SkConvolutionFilter1D::kShiftBits);

        
        
        int16x8_t accum16_0 = vcombine_s16(vqmovn_s32(accum0), vqmovn_s32(accum1));
        
        int16x8_t accum16_1 = vcombine_s16(vqmovn_s32(accum2), vqmovn_s32(accum3));

        
        
        uint8x16_t accum8 = vcombine_u8(vqmovun_s16(accum16_0), vqmovun_s16(accum16_1));

        if (hasAlpha) {
            
            
            uint8x16_t a = vreinterpretq_u8_u32(vshrq_n_u32(vreinterpretq_u32_u8(accum8), 8));
            
            uint8x16_t b = vmaxq_u8(a, accum8); 
            
            a = vreinterpretq_u8_u32(vshrq_n_u32(vreinterpretq_u32_u8(accum8), 16));
            
            b = vmaxq_u8(a, b); 
            
            b = vreinterpretq_u8_u32(vshlq_n_u32(vreinterpretq_u32_u8(b), 24));

            
            
            accum8 = vmaxq_u8(b, accum8);
        } else {
            
            accum8 = vreinterpretq_u8_u32(vreinterpretq_u32_u8(accum8) | vdupq_n_u32(0xFF000000));
        }

        
        vst1q_u8(outRow, accum8);
        outRow += 16;
    }

    
    
    int r = pixelWidth & 3;
    if (r) {

        accum0 = accum1 = accum2 = vdupq_n_s32(0);

        for (int filterY = 0; filterY < filterLength; ++filterY) {
            coeff16 = vdup_n_s16(filterValues[filterY]);

            
            uint8x16_t src8 = vld1q_u8(&sourceDataRows[filterY][width << 2]);

            int16x8_t src16_01 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(src8)));
            int16x8_t src16_23 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(src8)));
            int16x4_t src16_0 = vget_low_s16(src16_01);
            int16x4_t src16_1 = vget_high_s16(src16_01);
            int16x4_t src16_2 = vget_low_s16(src16_23);

            accum0 += vmull_s16(src16_0, coeff16);
            accum1 += vmull_s16(src16_1, coeff16);
            accum2 += vmull_s16(src16_2, coeff16);
        }

        accum0 = vshrq_n_s32(accum0, SkConvolutionFilter1D::kShiftBits);
        accum1 = vshrq_n_s32(accum1, SkConvolutionFilter1D::kShiftBits);
        accum2 = vshrq_n_s32(accum2, SkConvolutionFilter1D::kShiftBits);

        int16x8_t accum16_0 = vcombine_s16(vqmovn_s32(accum0), vqmovn_s32(accum1));
        int16x8_t accum16_1 = vcombine_s16(vqmovn_s32(accum2), vqmovn_s32(accum2));

        uint8x16_t accum8 = vcombine_u8(vqmovun_s16(accum16_0), vqmovun_s16(accum16_1));

        if (hasAlpha) {
            
            
            uint8x16_t a = vreinterpretq_u8_u32(vshrq_n_u32(vreinterpretq_u32_u8(accum8), 8));
            
            uint8x16_t b = vmaxq_u8(a, accum8); 
            
            a = vreinterpretq_u8_u32(vshrq_n_u32(vreinterpretq_u32_u8(accum8), 16));
            
            b = vmaxq_u8(a, b); 
            
            b = vreinterpretq_u8_u32(vshlq_n_u32(vreinterpretq_u32_u8(b), 24));

            
            
            accum8 = vmaxq_u8(b, accum8);
        } else {
            
            accum8 = vreinterpretq_u8_u32(vreinterpretq_u32_u8(accum8) | vdupq_n_u32(0xFF000000));
        }

        switch(r) {
        case 1:
            vst1q_lane_u32(reinterpret_cast<uint32_t*>(outRow), vreinterpretq_u32_u8(accum8), 0);
            break;
        case 2:
            vst1_u32(reinterpret_cast<uint32_t*>(outRow),
                     vreinterpret_u32_u8(vget_low_u8(accum8)));
            break;
        case 3:
            vst1_u32(reinterpret_cast<uint32_t*>(outRow),
                     vreinterpret_u32_u8(vget_low_u8(accum8)));
            vst1q_lane_u32(reinterpret_cast<uint32_t*>(outRow+8), vreinterpretq_u32_u8(accum8), 2);
            break;
        }
    }
}

void convolveVertically_neon(const SkConvolutionFilter1D::ConvolutionFixed* filterValues,
                             int filterLength,
                             unsigned char* const* sourceDataRows,
                             int pixelWidth,
                             unsigned char* outRow,
                             bool sourceHasAlpha) {
    if (sourceHasAlpha) {
        convolveVertically_neon<true>(filterValues, filterLength,
                                      sourceDataRows, pixelWidth,
                                      outRow);
    } else {
        convolveVertically_neon<false>(filterValues, filterLength,
                                       sourceDataRows, pixelWidth,
                                       outRow);
    }
}





void convolve4RowsHorizontally_neon(const unsigned char* srcData[4],
                                    const SkConvolutionFilter1D& filter,
                                    unsigned char* outRow[4]) {

    uint8x8_t coeff_mask0 = vcreate_u8(0x0100010001000100);
    uint8x8_t coeff_mask1 = vcreate_u8(0x0302030203020302);
    uint8x8_t coeff_mask2 = vcreate_u8(0x0504050405040504);
    uint8x8_t coeff_mask3 = vcreate_u8(0x0706070607060706);
    int num_values = filter.numValues();

    int filterOffset, filterLength;
    
    
    
    const uint16_t mask[4][4] = {
        {0, 0, 0, 0},
        {0xFFFF, 0, 0, 0},
        {0xFFFF, 0xFFFF, 0, 0},
        {0xFFFF, 0xFFFF, 0xFFFF, 0}
    };

    
    for (int outX = 0; outX < num_values; outX++) {

        const SkConvolutionFilter1D::ConvolutionFixed* filterValues =
        filter.FilterForValue(outX, &filterOffset, &filterLength);

        
        int32x4_t accum0 = vdupq_n_s32(0);
        int32x4_t accum1 = vdupq_n_s32(0);
        int32x4_t accum2 = vdupq_n_s32(0);
        int32x4_t accum3 = vdupq_n_s32(0);

        int start = (filterOffset<<2);

        
        for (int filter_x = 0; filter_x < (filterLength >> 2); filter_x++) {
            int16x4_t coeffs, coeff0, coeff1, coeff2, coeff3;

            coeffs = vld1_s16(filterValues);
            coeff0 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_s16(coeffs), coeff_mask0));
            coeff1 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_s16(coeffs), coeff_mask1));
            coeff2 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_s16(coeffs), coeff_mask2));
            coeff3 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_s16(coeffs), coeff_mask3));

            uint8x16_t pixels;
            int16x8_t p01_16, p23_16;
            int32x4_t p0, p1, p2, p3;


#define ITERATION(src, accum)                                       \
    pixels = vld1q_u8(src);                                         \
    p01_16 = vreinterpretq_s16_u16(vmovl_u8(vget_low_u8(pixels)));  \
    p23_16 = vreinterpretq_s16_u16(vmovl_u8(vget_high_u8(pixels))); \
    p0 = vmull_s16(vget_low_s16(p01_16), coeff0);                   \
    p1 = vmull_s16(vget_high_s16(p01_16), coeff1);                  \
    p2 = vmull_s16(vget_low_s16(p23_16), coeff2);                   \
    p3 = vmull_s16(vget_high_s16(p23_16), coeff3);                  \
    accum += p0;                                                    \
    accum += p1;                                                    \
    accum += p2;                                                    \
    accum += p3

            ITERATION(srcData[0] + start, accum0);
            ITERATION(srcData[1] + start, accum1);
            ITERATION(srcData[2] + start, accum2);
            ITERATION(srcData[3] + start, accum3);

            start += 16;
            filterValues += 4;
        }

        int r = filterLength & 3;
        if (r) {
            int16x4_t coeffs, coeff0, coeff1, coeff2, coeff3;
            coeffs = vld1_s16(filterValues);
            coeffs &= vreinterpret_s16_u16(vld1_u16(&mask[r][0]));
            coeff0 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_s16(coeffs), coeff_mask0));
            coeff1 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_s16(coeffs), coeff_mask1));
            coeff2 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_s16(coeffs), coeff_mask2));
            coeff3 = vreinterpret_s16_u8(vtbl1_u8(vreinterpret_u8_s16(coeffs), coeff_mask3));

            uint8x16_t pixels;
            int16x8_t p01_16, p23_16;
            int32x4_t p0, p1, p2, p3;

            ITERATION(srcData[0] + start, accum0);
            ITERATION(srcData[1] + start, accum1);
            ITERATION(srcData[2] + start, accum2);
            ITERATION(srcData[3] + start, accum3);
        }

        int16x4_t accum16;
        uint8x8_t res0, res1, res2, res3;

#define PACK_RESULT(accum, res)                                         \
        accum = vshrq_n_s32(accum, SkConvolutionFilter1D::kShiftBits);  \
        accum16 = vqmovn_s32(accum);                                    \
        res = vqmovun_s16(vcombine_s16(accum16, accum16));

        PACK_RESULT(accum0, res0);
        PACK_RESULT(accum1, res1);
        PACK_RESULT(accum2, res2);
        PACK_RESULT(accum3, res3);

        vst1_lane_u32(reinterpret_cast<uint32_t*>(outRow[0]), vreinterpret_u32_u8(res0), 0);
        vst1_lane_u32(reinterpret_cast<uint32_t*>(outRow[1]), vreinterpret_u32_u8(res1), 0);
        vst1_lane_u32(reinterpret_cast<uint32_t*>(outRow[2]), vreinterpret_u32_u8(res2), 0);
        vst1_lane_u32(reinterpret_cast<uint32_t*>(outRow[3]), vreinterpret_u32_u8(res3), 0);
        outRow[0] += 4;
        outRow[1] += 4;
        outRow[2] += 4;
        outRow[3] += 4;
    }
}

void applySIMDPadding_neon(SkConvolutionFilter1D *filter) {
    
    
    
    
    
    for (int i = 0; i < 8; ++i) {
        filter->addFilterValue(static_cast<SkConvolutionFilter1D::ConvolutionFixed>(0));
    }
}

void platformConvolutionProcs_arm_neon(SkConvolutionProcs* procs) {
    procs->fExtraHorizontalReads = 3;
    procs->fConvolveVertically = &convolveVertically_neon;
    procs->fConvolve4RowsHorizontally = &convolve4RowsHorizontally_neon;
    procs->fConvolveHorizontally = &convolveHorizontally_neon;
    procs->fApplySIMDPadding = &applySIMDPadding_neon;
}
