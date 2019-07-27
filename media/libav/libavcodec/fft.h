




















#ifndef AVCODEC_FFT_H
#define AVCODEC_FFT_H

#ifndef FFT_FLOAT
#define FFT_FLOAT 1
#endif

#include <stdint.h>
#include "config.h"
#include "libavutil/mem.h"

#if FFT_FLOAT

#include "avfft.h"

#define FFT_NAME(x) x

typedef float FFTDouble;

#else

#define FFT_NAME(x) x ## _fixed

typedef int16_t FFTSample;
typedef int     FFTDouble;

typedef struct FFTComplex {
    int16_t re, im;
} FFTComplex;

typedef struct FFTContext FFTContext;

#endif 

typedef struct FFTDComplex {
    FFTDouble re, im;
} FFTDComplex;



enum fft_permutation_type {
    FF_FFT_PERM_DEFAULT,
    FF_FFT_PERM_SWAP_LSBS,
    FF_FFT_PERM_AVX,
};

enum mdct_permutation_type {
    FF_MDCT_PERM_NONE,
    FF_MDCT_PERM_INTERLEAVE,
};

struct FFTContext {
    int nbits;
    int inverse;
    uint16_t *revtab;
    FFTComplex *tmp_buf;
    int mdct_size; 
    int mdct_bits; 
    
    FFTSample *tcos;
    FFTSample *tsin;
    


    void (*fft_permute)(struct FFTContext *s, FFTComplex *z);
    



    void (*fft_calc)(struct FFTContext *s, FFTComplex *z);
    void (*imdct_calc)(struct FFTContext *s, FFTSample *output, const FFTSample *input);
    void (*imdct_half)(struct FFTContext *s, FFTSample *output, const FFTSample *input);
    void (*mdct_calc)(struct FFTContext *s, FFTSample *output, const FFTSample *input);
    void (*mdct_calcw)(struct FFTContext *s, FFTDouble *output, const FFTSample *input);
    enum fft_permutation_type fft_permutation;
    enum mdct_permutation_type mdct_permutation;
};

#if CONFIG_HARDCODED_TABLES
#define COSTABLE_CONST const
#else
#define COSTABLE_CONST
#endif

#define COSTABLE(size) \
    COSTABLE_CONST DECLARE_ALIGNED(32, FFTSample, FFT_NAME(ff_cos_##size))[size/2]

extern COSTABLE(16);
extern COSTABLE(32);
extern COSTABLE(64);
extern COSTABLE(128);
extern COSTABLE(256);
extern COSTABLE(512);
extern COSTABLE(1024);
extern COSTABLE(2048);
extern COSTABLE(4096);
extern COSTABLE(8192);
extern COSTABLE(16384);
extern COSTABLE(32768);
extern COSTABLE(65536);
extern COSTABLE_CONST FFTSample* const FFT_NAME(ff_cos_tabs)[17];

#define ff_init_ff_cos_tabs FFT_NAME(ff_init_ff_cos_tabs)





void ff_init_ff_cos_tabs(int index);

#define ff_fft_init FFT_NAME(ff_fft_init)
#define ff_fft_end  FFT_NAME(ff_fft_end)






int ff_fft_init(FFTContext *s, int nbits, int inverse);

void ff_fft_init_aarch64(FFTContext *s);
void ff_fft_init_x86(FFTContext *s);
void ff_fft_init_arm(FFTContext *s);
void ff_fft_init_ppc(FFTContext *s);

void ff_fft_fixed_init_arm(FFTContext *s);

void ff_fft_end(FFTContext *s);

#define ff_mdct_init FFT_NAME(ff_mdct_init)
#define ff_mdct_end  FFT_NAME(ff_mdct_end)

int ff_mdct_init(FFTContext *s, int nbits, int inverse, double scale);
void ff_mdct_end(FFTContext *s);

#endif 
