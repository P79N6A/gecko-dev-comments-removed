










#include <smmintrin.h> 

#include "./vp8_rtcd.h"
#include "vp8/encoder/block.h"
#include "vp8/common/entropy.h" 

#define SELECT_EOB(i, z, x, y, q) \
    do { \
        short boost = *zbin_boost_ptr; \
        short x_z = _mm_extract_epi16(x, z); \
        short y_z = _mm_extract_epi16(y, z); \
        int cmp = (x_z < boost) | (y_z == 0); \
        zbin_boost_ptr++; \
        if (cmp) \
            break; \
        q = _mm_insert_epi16(q, y_z, z); \
        eob = i; \
        zbin_boost_ptr = b->zrun_zbin_boost; \
    } while (0)

void vp8_regular_quantize_b_sse4_1(BLOCK *b, BLOCKD *d) {
    char eob = 0;
    short *zbin_boost_ptr  = b->zrun_zbin_boost;

    __m128i sz0, x0, sz1, x1, y0, y1, x_minus_zbin0, x_minus_zbin1,
            dqcoeff0, dqcoeff1;
    __m128i quant_shift0 = _mm_load_si128((__m128i *)(b->quant_shift));
    __m128i quant_shift1 = _mm_load_si128((__m128i *)(b->quant_shift + 8));
    __m128i z0 = _mm_load_si128((__m128i *)(b->coeff));
    __m128i z1 = _mm_load_si128((__m128i *)(b->coeff+8));
    __m128i zbin_extra = _mm_cvtsi32_si128(b->zbin_extra);
    __m128i zbin0 = _mm_load_si128((__m128i *)(b->zbin));
    __m128i zbin1 = _mm_load_si128((__m128i *)(b->zbin + 8));
    __m128i round0 = _mm_load_si128((__m128i *)(b->round));
    __m128i round1 = _mm_load_si128((__m128i *)(b->round + 8));
    __m128i quant0 = _mm_load_si128((__m128i *)(b->quant));
    __m128i quant1 = _mm_load_si128((__m128i *)(b->quant + 8));
    __m128i dequant0 = _mm_load_si128((__m128i *)(d->dequant));
    __m128i dequant1 = _mm_load_si128((__m128i *)(d->dequant + 8));
    __m128i qcoeff0 = _mm_setzero_si128();
    __m128i qcoeff1 = _mm_setzero_si128();

    
    zbin_extra = _mm_shufflelo_epi16(zbin_extra, 0);
    zbin_extra = _mm_unpacklo_epi16(zbin_extra, zbin_extra);

    
    sz0 = _mm_srai_epi16(z0, 15);
    sz1 = _mm_srai_epi16(z1, 15);

    
    x0 = _mm_xor_si128(z0, sz0);
    x1 = _mm_xor_si128(z1, sz1);
    x0 = _mm_sub_epi16(x0, sz0);
    x1 = _mm_sub_epi16(x1, sz1);

    
    zbin0 = _mm_add_epi16(zbin0, zbin_extra);
    zbin1 = _mm_add_epi16(zbin1, zbin_extra);

    


    x_minus_zbin0 = _mm_sub_epi16(x0, zbin0);
    x_minus_zbin1 = _mm_sub_epi16(x1, zbin1);

    

    x0 = _mm_add_epi16(x0, round0);
    x1 = _mm_add_epi16(x1, round1);

    y0 = _mm_mulhi_epi16(x0, quant0);
    y1 = _mm_mulhi_epi16(x1, quant1);

    y0 = _mm_add_epi16(y0, x0);
    y1 = _mm_add_epi16(y1, x1);

    

    y0 = _mm_mulhi_epi16(y0, quant_shift0);
    y1 = _mm_mulhi_epi16(y1, quant_shift1);

    
    y0 = _mm_xor_si128(y0, sz0);
    y1 = _mm_xor_si128(y1, sz1);
    y0 = _mm_sub_epi16(y0, sz0);
    y1 = _mm_sub_epi16(y1, sz1);

    
    SELECT_EOB(1, 0, x_minus_zbin0, y0, qcoeff0);
    SELECT_EOB(2, 1, x_minus_zbin0, y0, qcoeff0);
    SELECT_EOB(3, 4, x_minus_zbin0, y0, qcoeff0);
    SELECT_EOB(4, 0, x_minus_zbin1, y1, qcoeff1);
    SELECT_EOB(5, 5, x_minus_zbin0, y0, qcoeff0);
    SELECT_EOB(6, 2, x_minus_zbin0, y0, qcoeff0);
    SELECT_EOB(7, 3, x_minus_zbin0, y0, qcoeff0);
    SELECT_EOB(8, 6, x_minus_zbin0, y0, qcoeff0);
    SELECT_EOB(9, 1, x_minus_zbin1, y1, qcoeff1);
    SELECT_EOB(10, 4, x_minus_zbin1, y1, qcoeff1);
    SELECT_EOB(11, 5, x_minus_zbin1, y1, qcoeff1);
    SELECT_EOB(12, 2, x_minus_zbin1, y1, qcoeff1);
    SELECT_EOB(13, 7, x_minus_zbin0, y0, qcoeff0);
    SELECT_EOB(14, 3, x_minus_zbin1, y1, qcoeff1);
    SELECT_EOB(15, 6, x_minus_zbin1, y1, qcoeff1);
    SELECT_EOB(16, 7, x_minus_zbin1, y1, qcoeff1);

    _mm_store_si128((__m128i *)(d->qcoeff), qcoeff0);
    _mm_store_si128((__m128i *)(d->qcoeff + 8), qcoeff1);

    dqcoeff0 = _mm_mullo_epi16(qcoeff0, dequant0);
    dqcoeff1 = _mm_mullo_epi16(qcoeff1, dequant1);

    _mm_store_si128((__m128i *)(d->dqcoeff), dqcoeff0);
    _mm_store_si128((__m128i *)(d->dqcoeff + 8), dqcoeff1);

    *d->eob = eob;
}
