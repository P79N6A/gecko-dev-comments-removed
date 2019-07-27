









#include <arm_neon.h>

void vp8_copy_mem8x4_neon(
        unsigned char *src,
        int src_stride,
        unsigned char *dst,
        int dst_stride) {
    uint8x8_t vtmp;
    int r;

    for (r = 0; r < 4; r++) {
        vtmp = vld1_u8(src);
        vst1_u8(dst, vtmp);
        src += src_stride;
        dst += dst_stride;
    }
}

void vp8_copy_mem8x8_neon(
        unsigned char *src,
        int src_stride,
        unsigned char *dst,
        int dst_stride) {
    uint8x8_t vtmp;
    int r;

    for (r = 0; r < 8; r++) {
        vtmp = vld1_u8(src);
        vst1_u8(dst, vtmp);
        src += src_stride;
        dst += dst_stride;
    }
}

void vp8_copy_mem16x16_neon(
        unsigned char *src,
        int src_stride,
        unsigned char *dst,
        int dst_stride) {
    int r;
    uint8x16_t qtmp;

    for (r = 0; r < 16; r++) {
        qtmp = vld1q_u8(src);
        vst1q_u8(dst, qtmp);
        src += src_stride;
        dst += dst_stride;
    }
}
