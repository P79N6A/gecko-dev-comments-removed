









#include <stddef.h>
#include <arm_neon.h>

void vp9_convolve_avg_neon(
        const uint8_t *src,    
        ptrdiff_t src_stride,  
        uint8_t *dst,          
        ptrdiff_t dst_stride,  
        const int16_t *filter_x,
        int filter_x_stride,
        const int16_t *filter_y,
        int filter_y_stride,
        int w,
        int h) {
    uint8_t *d;
    uint8x8_t d0u8, d1u8, d2u8, d3u8;
    uint32x2_t d0u32, d2u32;
    uint8x16_t q0u8, q1u8, q2u8, q3u8, q8u8, q9u8, q10u8, q11u8;
    (void)filter_x;  (void)filter_x_stride;
    (void)filter_y;  (void)filter_y_stride;

    d = dst;
    if (w > 32) {  
        for (; h > 0; h -= 1) {
            q0u8  = vld1q_u8(src);
            q1u8  = vld1q_u8(src + 16);
            q2u8  = vld1q_u8(src + 32);
            q3u8  = vld1q_u8(src + 48);
            src += src_stride;
            q8u8  = vld1q_u8(d);
            q9u8  = vld1q_u8(d + 16);
            q10u8 = vld1q_u8(d + 32);
            q11u8 = vld1q_u8(d + 48);
            d += dst_stride;

            q0u8 = vrhaddq_u8(q0u8, q8u8);
            q1u8 = vrhaddq_u8(q1u8, q9u8);
            q2u8 = vrhaddq_u8(q2u8, q10u8);
            q3u8 = vrhaddq_u8(q3u8, q11u8);

            vst1q_u8(dst, q0u8);
            vst1q_u8(dst + 16, q1u8);
            vst1q_u8(dst + 32, q2u8);
            vst1q_u8(dst + 48, q3u8);
            dst += dst_stride;
        }
    } else if (w == 32) {  
        for (; h > 0; h -= 2) {
            q0u8 = vld1q_u8(src);
            q1u8 = vld1q_u8(src + 16);
            src += src_stride;
            q2u8 = vld1q_u8(src);
            q3u8 = vld1q_u8(src + 16);
            src += src_stride;
            q8u8 = vld1q_u8(d);
            q9u8 = vld1q_u8(d + 16);
            d += dst_stride;
            q10u8 = vld1q_u8(d);
            q11u8 = vld1q_u8(d + 16);
            d += dst_stride;

            q0u8 = vrhaddq_u8(q0u8, q8u8);
            q1u8 = vrhaddq_u8(q1u8, q9u8);
            q2u8 = vrhaddq_u8(q2u8, q10u8);
            q3u8 = vrhaddq_u8(q3u8, q11u8);

            vst1q_u8(dst, q0u8);
            vst1q_u8(dst + 16, q1u8);
            dst += dst_stride;
            vst1q_u8(dst, q2u8);
            vst1q_u8(dst + 16, q3u8);
            dst += dst_stride;
        }
    } else if (w > 8) {  
        for (; h > 0; h -= 2) {
            q0u8 = vld1q_u8(src);
            src += src_stride;
            q1u8 = vld1q_u8(src);
            src += src_stride;
            q2u8 = vld1q_u8(d);
            d += dst_stride;
            q3u8 = vld1q_u8(d);
            d += dst_stride;

            q0u8 = vrhaddq_u8(q0u8, q2u8);
            q1u8 = vrhaddq_u8(q1u8, q3u8);

            vst1q_u8(dst, q0u8);
            dst += dst_stride;
            vst1q_u8(dst, q1u8);
            dst += dst_stride;
        }
    } else if (w == 8) {  
        for (; h > 0; h -= 2) {
            d0u8 = vld1_u8(src);
            src += src_stride;
            d1u8 = vld1_u8(src);
            src += src_stride;
            d2u8 = vld1_u8(d);
            d += dst_stride;
            d3u8 = vld1_u8(d);
            d += dst_stride;

            q0u8 = vcombine_u8(d0u8, d1u8);
            q1u8 = vcombine_u8(d2u8, d3u8);
            q0u8 = vrhaddq_u8(q0u8, q1u8);

            vst1_u8(dst, vget_low_u8(q0u8));
            dst += dst_stride;
            vst1_u8(dst, vget_high_u8(q0u8));
            dst += dst_stride;
        }
    } else {  
        for (; h > 0; h -= 2) {
            d0u32 = vld1_lane_u32((const uint32_t *)src, d0u32, 0);
            src += src_stride;
            d0u32 = vld1_lane_u32((const uint32_t *)src, d0u32, 1);
            src += src_stride;
            d2u32 = vld1_lane_u32((const uint32_t *)d, d2u32, 0);
            d += dst_stride;
            d2u32 = vld1_lane_u32((const uint32_t *)d, d2u32, 1);
            d += dst_stride;

            d0u8 = vrhadd_u8(vreinterpret_u8_u32(d0u32),
                             vreinterpret_u8_u32(d2u32));

            d0u32 = vreinterpret_u32_u8(d0u8);
            vst1_lane_u32((uint32_t *)dst, d0u32, 0);
            dst += dst_stride;
            vst1_lane_u32((uint32_t *)dst, d0u32, 1);
            dst += dst_stride;
        }
    }
    return;
}
