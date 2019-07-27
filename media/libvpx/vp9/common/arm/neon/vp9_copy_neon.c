









#include <stddef.h>
#include <arm_neon.h>

void vp9_convolve_copy_neon(
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
    uint8x8_t d0u8, d2u8;
    uint8x16_t q0u8, q1u8, q2u8, q3u8;
    (void)filter_x;  (void)filter_x_stride;
    (void)filter_y;  (void)filter_y_stride;

    if (w > 32) {  
        for (; h > 0; h--) {
            q0u8 = vld1q_u8(src);
            q1u8 = vld1q_u8(src + 16);
            q2u8 = vld1q_u8(src + 32);
            q3u8 = vld1q_u8(src + 48);
            src += src_stride;

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

            vst1q_u8(dst, q0u8);
            dst += dst_stride;
            vst1q_u8(dst, q1u8);
            dst += dst_stride;
        }
    } else if (w == 8) {  
        for (; h > 0; h -= 2) {
            d0u8 = vld1_u8(src);
            src += src_stride;
            d2u8 = vld1_u8(src);
            src += src_stride;

            vst1_u8(dst, d0u8);
            dst += dst_stride;
            vst1_u8(dst, d2u8);
            dst += dst_stride;
        }
    } else {  
        for (; h > 0; h--) {
            *(uint32_t *)dst = *(const uint32_t *)src;
            src += src_stride;
            dst += dst_stride;
        }
    }
    return;
}
