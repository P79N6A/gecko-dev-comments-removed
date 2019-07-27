






















#ifndef AVCODEC_DCT_H
#define AVCODEC_DCT_H

#include <stdint.h>

#include "rdft.h"

struct DCTContext {
    int nbits;
    int inverse;
    RDFTContext rdft;
    const float *costab;
    FFTSample *csc2;
    void (*dct_calc)(struct DCTContext *s, FFTSample *data);
    void (*dct32)(FFTSample *out, const FFTSample *in);
};









int  ff_dct_init(DCTContext *s, int nbits, enum DCTTransformType type);
void ff_dct_end (DCTContext *s);

void ff_dct_init_x86(DCTContext *s);

void ff_fdct_ifast(int16_t *data);
void ff_fdct_ifast248(int16_t *data);
void ff_jpeg_fdct_islow_8(int16_t *data);
void ff_jpeg_fdct_islow_10(int16_t *data);
void ff_fdct248_islow_8(int16_t *data);
void ff_fdct248_islow_10(int16_t *data);

void ff_j_rev_dct(int16_t *data);

#endif 
