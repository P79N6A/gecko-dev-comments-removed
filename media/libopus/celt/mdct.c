








































#ifndef SKIP_CONFIG_H
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#endif

#include "mdct.h"
#include "kiss_fft.h"
#include "_kiss_fft_guts.h"
#include <math.h>
#include "os_support.h"
#include "mathops.h"
#include "stack_alloc.h"

#ifdef CUSTOM_MODES

int clt_mdct_init(mdct_lookup *l,int N, int maxshift)
{
   int i;
   int N4;
   kiss_twiddle_scalar *trig;
#if defined(FIXED_POINT)
   int N2=N>>1;
#endif
   l->n = N;
   N4 = N>>2;
   l->maxshift = maxshift;
   for (i=0;i<=maxshift;i++)
   {
      if (i==0)
         l->kfft[i] = opus_fft_alloc(N>>2>>i, 0, 0);
      else
         l->kfft[i] = opus_fft_alloc_twiddles(N>>2>>i, 0, 0, l->kfft[0]);
#ifndef ENABLE_TI_DSPLIB55
      if (l->kfft[i]==NULL)
         return 0;
#endif
   }
   l->trig = trig = (kiss_twiddle_scalar*)opus_alloc((N4+1)*sizeof(kiss_twiddle_scalar));
   if (l->trig==NULL)
     return 0;
   
#if defined(FIXED_POINT)
   for (i=0;i<=N4;i++)
      trig[i] = TRIG_UPSCALE*celt_cos_norm(DIV32(ADD32(SHL32(EXTEND32(i),17),N2),N));
#else
   for (i=0;i<=N4;i++)
      trig[i] = (kiss_twiddle_scalar)cos(2*PI*i/N);
#endif
   return 1;
}

void clt_mdct_clear(mdct_lookup *l)
{
   int i;
   for (i=0;i<=l->maxshift;i++)
      opus_fft_free(l->kfft[i]);
   opus_free((kiss_twiddle_scalar*)l->trig);
}

#endif 


void clt_mdct_forward(const mdct_lookup *l, kiss_fft_scalar *in, kiss_fft_scalar * OPUS_RESTRICT out,
      const opus_val16 *window, int overlap, int shift, int stride)
{
   int i;
   int N, N2, N4;
   kiss_twiddle_scalar sine;
   VARDECL(kiss_fft_scalar, f);
   VARDECL(kiss_fft_scalar, f2);
   SAVE_STACK;
   N = l->n;
   N >>= shift;
   N2 = N>>1;
   N4 = N>>2;
   ALLOC(f, N2, kiss_fft_scalar);
   ALLOC(f2, N2, kiss_fft_scalar);
   
#ifdef FIXED_POINT
   sine = TRIG_UPSCALE*(QCONST16(0.7853981f, 15)+N2)/N;
#else
   sine = (kiss_twiddle_scalar)2*PI*(.125f)/N;
#endif

   
   
   {
      
      const kiss_fft_scalar * OPUS_RESTRICT xp1 = in+(overlap>>1);
      const kiss_fft_scalar * OPUS_RESTRICT xp2 = in+N2-1+(overlap>>1);
      kiss_fft_scalar * OPUS_RESTRICT yp = f;
      const opus_val16 * OPUS_RESTRICT wp1 = window+(overlap>>1);
      const opus_val16 * OPUS_RESTRICT wp2 = window+(overlap>>1)-1;
      for(i=0;i<((overlap+3)>>2);i++)
      {
         
         *yp++ = MULT16_32_Q15(*wp2, xp1[N2]) + MULT16_32_Q15(*wp1,*xp2);
         *yp++ = MULT16_32_Q15(*wp1, *xp1)    - MULT16_32_Q15(*wp2, xp2[-N2]);
         xp1+=2;
         xp2-=2;
         wp1+=2;
         wp2-=2;
      }
      wp1 = window;
      wp2 = window+overlap-1;
      for(;i<N4-((overlap+3)>>2);i++)
      {
         
         *yp++ = *xp2;
         *yp++ = *xp1;
         xp1+=2;
         xp2-=2;
      }
      for(;i<N4;i++)
      {
         
         *yp++ =  -MULT16_32_Q15(*wp1, xp1[-N2]) + MULT16_32_Q15(*wp2, *xp2);
         *yp++ = MULT16_32_Q15(*wp2, *xp1)     + MULT16_32_Q15(*wp1, xp2[N2]);
         xp1+=2;
         xp2-=2;
         wp1+=2;
         wp2-=2;
      }
   }
   
   {
      kiss_fft_scalar * OPUS_RESTRICT yp = f;
      const kiss_twiddle_scalar *t = &l->trig[0];
      for(i=0;i<N4;i++)
      {
         kiss_fft_scalar re, im, yr, yi;
         re = yp[0];
         im = yp[1];
         yr = -S_MUL(re,t[i<<shift])  -  S_MUL(im,t[(N4-i)<<shift]);
         yi = -S_MUL(im,t[i<<shift])  +  S_MUL(re,t[(N4-i)<<shift]);
         
         *yp++ = yr + S_MUL(yi,sine);
         *yp++ = yi - S_MUL(yr,sine);
      }
   }

   
   opus_fft(l->kfft[shift], (kiss_fft_cpx *)f, (kiss_fft_cpx *)f2);

   
   {
      
      const kiss_fft_scalar * OPUS_RESTRICT fp = f2;
      kiss_fft_scalar * OPUS_RESTRICT yp1 = out;
      kiss_fft_scalar * OPUS_RESTRICT yp2 = out+stride*(N2-1);
      const kiss_twiddle_scalar *t = &l->trig[0];
      
      for(i=0;i<N4;i++)
      {
         kiss_fft_scalar yr, yi;
         yr = S_MUL(fp[1],t[(N4-i)<<shift]) + S_MUL(fp[0],t[i<<shift]);
         yi = S_MUL(fp[0],t[(N4-i)<<shift]) - S_MUL(fp[1],t[i<<shift]);
         
         *yp1 = yr - S_MUL(yi,sine);
         *yp2 = yi + S_MUL(yr,sine);;
         fp += 2;
         yp1 += 2*stride;
         yp2 -= 2*stride;
      }
   }
   RESTORE_STACK;
}

void clt_mdct_backward(const mdct_lookup *l, kiss_fft_scalar *in, kiss_fft_scalar * OPUS_RESTRICT out,
      const opus_val16 * OPUS_RESTRICT window, int overlap, int shift, int stride)
{
   int i;
   int N, N2, N4;
   kiss_twiddle_scalar sine;
   VARDECL(kiss_fft_scalar, f2);
   SAVE_STACK;
   N = l->n;
   N >>= shift;
   N2 = N>>1;
   N4 = N>>2;
   ALLOC(f2, N2, kiss_fft_scalar);
   
#ifdef FIXED_POINT
   sine = TRIG_UPSCALE*(QCONST16(0.7853981f, 15)+N2)/N;
#else
   sine = (kiss_twiddle_scalar)2*PI*(.125f)/N;
#endif

   
   {
      
      const kiss_fft_scalar * OPUS_RESTRICT xp1 = in;
      const kiss_fft_scalar * OPUS_RESTRICT xp2 = in+stride*(N2-1);
      kiss_fft_scalar * OPUS_RESTRICT yp = f2;
      const kiss_twiddle_scalar *t = &l->trig[0];
      for(i=0;i<N4;i++)
      {
         kiss_fft_scalar yr, yi;
         yr = -S_MUL(*xp2, t[i<<shift]) + S_MUL(*xp1,t[(N4-i)<<shift]);
         yi =  -S_MUL(*xp2, t[(N4-i)<<shift]) - S_MUL(*xp1,t[i<<shift]);
         
         *yp++ = yr - S_MUL(yi,sine);
         *yp++ = yi + S_MUL(yr,sine);
         xp1+=2*stride;
         xp2-=2*stride;
      }
   }

   
   opus_ifft(l->kfft[shift], (kiss_fft_cpx *)f2, (kiss_fft_cpx *)(out+(overlap>>1)));

   

   {
      kiss_fft_scalar * OPUS_RESTRICT yp0 = out+(overlap>>1);
      kiss_fft_scalar * OPUS_RESTRICT yp1 = out+(overlap>>1)+N2-2;
      const kiss_twiddle_scalar *t = &l->trig[0];
      

      for(i=0;i<(N4+1)>>1;i++)
      {
         kiss_fft_scalar re, im, yr, yi;
         kiss_twiddle_scalar t0, t1;
         re = yp0[0];
         im = yp0[1];
         t0 = t[i<<shift];
         t1 = t[(N4-i)<<shift];
         
         yr = S_MUL(re,t0) - S_MUL(im,t1);
         yi = S_MUL(im,t0) + S_MUL(re,t1);
         re = yp1[0];
         im = yp1[1];
         
         yp0[0] = -(yr - S_MUL(yi,sine));
         yp1[1] = yi + S_MUL(yr,sine);

         t0 = t[(N4-i-1)<<shift];
         t1 = t[(i+1)<<shift];
         
         yr = S_MUL(re,t0) - S_MUL(im,t1);
         yi = S_MUL(im,t0) + S_MUL(re,t1);
         
         yp1[0] = -(yr - S_MUL(yi,sine));
         yp0[1] = yi + S_MUL(yr,sine);
         yp0 += 2;
         yp1 -= 2;
      }
   }

   
   {
      kiss_fft_scalar * OPUS_RESTRICT xp1 = out+overlap-1;
      kiss_fft_scalar * OPUS_RESTRICT yp1 = out;
      const opus_val16 * OPUS_RESTRICT wp1 = window;
      const opus_val16 * OPUS_RESTRICT wp2 = window+overlap-1;

      for(i = 0; i < overlap/2; i++)
      {
         kiss_fft_scalar x1, x2;
         x1 = *xp1;
         x2 = *yp1;
         *yp1++ = MULT16_32_Q15(*wp2, x2) - MULT16_32_Q15(*wp1, x1);
         *xp1-- = MULT16_32_Q15(*wp1, x2) + MULT16_32_Q15(*wp2, x1);
         wp1++;
         wp2--;
      }
   }
   RESTORE_STACK;
}
