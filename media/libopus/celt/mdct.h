
















































#ifndef MDCT_H
#define MDCT_H

#include "kiss_fft.h"
#include "arch.h"

typedef struct {
   int n;
   int maxshift;
   const kiss_fft_state *kfft[4];
   const kiss_twiddle_scalar * restrict trig;
} mdct_lookup;

int clt_mdct_init(mdct_lookup *l,int N, int maxshift);
void clt_mdct_clear(mdct_lookup *l);


void clt_mdct_forward(const mdct_lookup *l, kiss_fft_scalar *in, kiss_fft_scalar *out,
      const opus_val16 *window, int overlap, int shift, int stride);



void clt_mdct_backward(const mdct_lookup *l, kiss_fft_scalar *in, kiss_fft_scalar *out,
      const opus_val16 * restrict window, int overlap, int shift, int stride);

#endif
