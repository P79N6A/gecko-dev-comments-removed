



#ifndef _MP_GF2M_PRIV_H_
#define _MP_GF2M_PRIV_H_

#include "mpi-priv.h"

extern const mp_digit mp_gf2m_sqr_tb[16];

#if defined(MP_USE_UINT_DIGIT)
#define MP_DIGIT_BITS 32

#define MP_DIGIT_BITS_LOG_2 5
#define MP_DIGIT_BITS_MASK 0x1f
#else
#define MP_DIGIT_BITS 64

#define MP_DIGIT_BITS_LOG_2 6
#define MP_DIGIT_BITS_MASK 0x3f
#endif


#if MP_DIGIT_BITS == 32
#define gf2m_SQR1(w) \
    mp_gf2m_sqr_tb[(w) >> 28 & 0xF] << 24 | mp_gf2m_sqr_tb[(w) >> 24 & 0xF] << 16 | \
    mp_gf2m_sqr_tb[(w) >> 20 & 0xF] <<  8 | mp_gf2m_sqr_tb[(w) >> 16 & 0xF]
#define gf2m_SQR0(w) \
    mp_gf2m_sqr_tb[(w) >> 12 & 0xF] << 24 | mp_gf2m_sqr_tb[(w) >>  8 & 0xF] << 16 | \
    mp_gf2m_sqr_tb[(w) >>  4 & 0xF] <<  8 | mp_gf2m_sqr_tb[(w)       & 0xF]
#else
#define gf2m_SQR1(w) \
    mp_gf2m_sqr_tb[(w) >> 60 & 0xF] << 56 | mp_gf2m_sqr_tb[(w) >> 56 & 0xF] << 48 | \
    mp_gf2m_sqr_tb[(w) >> 52 & 0xF] << 40 | mp_gf2m_sqr_tb[(w) >> 48 & 0xF] << 32 | \
    mp_gf2m_sqr_tb[(w) >> 44 & 0xF] << 24 | mp_gf2m_sqr_tb[(w) >> 40 & 0xF] << 16 | \
    mp_gf2m_sqr_tb[(w) >> 36 & 0xF] <<  8 | mp_gf2m_sqr_tb[(w) >> 32 & 0xF]
#define gf2m_SQR0(w) \
    mp_gf2m_sqr_tb[(w) >> 28 & 0xF] << 56 | mp_gf2m_sqr_tb[(w) >> 24 & 0xF] << 48 | \
    mp_gf2m_sqr_tb[(w) >> 20 & 0xF] << 40 | mp_gf2m_sqr_tb[(w) >> 16 & 0xF] << 32 | \
    mp_gf2m_sqr_tb[(w) >> 12 & 0xF] << 24 | mp_gf2m_sqr_tb[(w) >>  8 & 0xF] << 16 | \
    mp_gf2m_sqr_tb[(w) >>  4 & 0xF] <<  8 | mp_gf2m_sqr_tb[(w)       & 0xF]
#endif





void s_bmul_1x1(mp_digit *rh, mp_digit *rl, const mp_digit a, const mp_digit b);





void s_bmul_2x2(mp_digit *r, const mp_digit a1, const mp_digit a0, const mp_digit b1,
	const mp_digit b0);





void s_bmul_3x3(mp_digit *r, const mp_digit a2, const mp_digit a1, const mp_digit a0, 
	const mp_digit b2, const mp_digit b1, const mp_digit b0);





void s_bmul_4x4(mp_digit *r, const mp_digit a3, const mp_digit a2, const mp_digit a1, 
	const mp_digit a0, const mp_digit b3, const mp_digit b2, const mp_digit b1, 
	const mp_digit b0);

#endif 
