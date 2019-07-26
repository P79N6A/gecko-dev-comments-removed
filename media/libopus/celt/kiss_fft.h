



























#ifndef KISS_FFT_H
#define KISS_FFT_H

#include <stdlib.h>
#include <math.h>
#include "arch.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_SIMD
# include <xmmintrin.h>
# define kiss_fft_scalar __m128
#define KISS_FFT_MALLOC(nbytes) memalign(16,nbytes)
#else
#define KISS_FFT_MALLOC opus_alloc
#endif

#ifdef FIXED_POINT
#include "arch.h"

#  define kiss_fft_scalar opus_int32
#  define kiss_twiddle_scalar opus_int16


#else
# ifndef kiss_fft_scalar

#   define kiss_fft_scalar float
#   define kiss_twiddle_scalar float
#   define KF_SUFFIX _celt_single
# endif
#endif

typedef struct {
    kiss_fft_scalar r;
    kiss_fft_scalar i;
}kiss_fft_cpx;

typedef struct {
   kiss_twiddle_scalar r;
   kiss_twiddle_scalar i;
}kiss_twiddle_cpx;

#define MAXFACTORS 8





typedef struct kiss_fft_state{
    int nfft;
#ifndef FIXED_POINT
    kiss_fft_scalar scale;
#endif
    int shift;
    opus_int16 factors[2*MAXFACTORS];
    const opus_int16 *bitrev;
    const kiss_twiddle_cpx *twiddles;
} kiss_fft_state;


























kiss_fft_state *opus_fft_alloc_twiddles(int nfft,void * mem,size_t * lenmem, const kiss_fft_state *base);

kiss_fft_state *opus_fft_alloc(int nfft,void * mem,size_t * lenmem);











void opus_fft(const kiss_fft_state *cfg,const kiss_fft_cpx *fin,kiss_fft_cpx *fout);
void opus_ifft(const kiss_fft_state *cfg,const kiss_fft_cpx *fin,kiss_fft_cpx *fout);

void opus_fft_free(const kiss_fft_state *cfg);

#ifdef __cplusplus
}
#endif

#endif
