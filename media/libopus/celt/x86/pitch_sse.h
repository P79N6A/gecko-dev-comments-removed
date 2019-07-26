






























#ifndef PITCH_SSE_H
#define PITCH_SSE_H

#include <xmmintrin.h>
#include "arch.h"

#define OVERRIDE_XCORR_KERNEL
static inline void xcorr_kernel(const opus_val16 *x, const opus_val16 *y, opus_val32 sum[4], int len)
{
   int j;
   __m128 xsum1, xsum2;
   xsum1 = _mm_loadu_ps(sum);
   xsum2 = _mm_setzero_ps();

   for (j = 0; j < len-3; j += 4)
   {
      __m128 x0 = _mm_loadu_ps(x+j);
      __m128 yj = _mm_loadu_ps(y+j);
      __m128 y3 = _mm_loadu_ps(y+j+3);

      xsum1 = _mm_add_ps(xsum1,_mm_mul_ps(_mm_shuffle_ps(x0,x0,0x00),yj));
      xsum2 = _mm_add_ps(xsum2,_mm_mul_ps(_mm_shuffle_ps(x0,x0,0x55),
                                          _mm_shuffle_ps(yj,y3,0x49)));
      xsum1 = _mm_add_ps(xsum1,_mm_mul_ps(_mm_shuffle_ps(x0,x0,0xaa),
                                          _mm_shuffle_ps(yj,y3,0x9e)));
      xsum2 = _mm_add_ps(xsum2,_mm_mul_ps(_mm_shuffle_ps(x0,x0,0xff),y3));
   }
   if (j < len)
   {
      xsum1 = _mm_add_ps(xsum1,_mm_mul_ps(_mm_load1_ps(x+j),_mm_loadu_ps(y+j)));
      if (++j < len)
      {
         xsum2 = _mm_add_ps(xsum2,_mm_mul_ps(_mm_load1_ps(x+j),_mm_loadu_ps(y+j)));
         if (++j < len)
         {
            xsum1 = _mm_add_ps(xsum1,_mm_mul_ps(_mm_load1_ps(x+j),_mm_loadu_ps(y+j)));
         }
      }
   }
   _mm_storeu_ps(sum,_mm_add_ps(xsum1,xsum2));
}

#define OVERRIDE_DUAL_INNER_PROD
static inline void dual_inner_prod(const opus_val16 *x, const opus_val16 *y01, const opus_val16 *y02,
      int N, opus_val32 *xy1, opus_val32 *xy2)
{
   int i;
   __m128 xsum1, xsum2;
   xsum1 = _mm_setzero_ps();
   xsum2 = _mm_setzero_ps();
   for (i=0;i<N-3;i+=4)
   {
      __m128 xi = _mm_loadu_ps(x+i);
      __m128 y1i = _mm_loadu_ps(y01+i);
      __m128 y2i = _mm_loadu_ps(y02+i);
      xsum1 = _mm_add_ps(xsum1,_mm_mul_ps(xi, y1i));
      xsum2 = _mm_add_ps(xsum2,_mm_mul_ps(xi, y2i));
   }
   
   xsum1 = _mm_add_ps(xsum1, _mm_movehl_ps(xsum1, xsum1));
   xsum1 = _mm_add_ss(xsum1, _mm_shuffle_ps(xsum1, xsum1, 0x55));
   _mm_store_ss(xy1, xsum1);
   xsum2 = _mm_add_ps(xsum2, _mm_movehl_ps(xsum2, xsum2));
   xsum2 = _mm_add_ss(xsum2, _mm_shuffle_ps(xsum2, xsum2, 0x55));
   _mm_store_ss(xy2, xsum2);
   for (;i<N;i++)
   {
      *xy1 = MAC16_16(*xy1, x[i], y01[i]);
      *xy2 = MAC16_16(*xy2, x[i], y02[i]);
   }
}

#define OVERRIDE_COMB_FILTER_CONST
static inline void comb_filter_const(opus_val32 *y, opus_val32 *x, int T, int N,
      opus_val16 g10, opus_val16 g11, opus_val16 g12)
{
   int i;
   __m128 x0v;
   __m128 g10v, g11v, g12v;
   g10v = _mm_load1_ps(&g10);
   g11v = _mm_load1_ps(&g11);
   g12v = _mm_load1_ps(&g12);
   x0v = _mm_loadu_ps(&x[-T-2]);
   for (i=0;i<N-3;i+=4)
   {
      __m128 yi, yi2, x1v, x2v, x3v, x4v;
      const opus_val32 *xp = &x[i-T-2];
      yi = _mm_loadu_ps(x+i);
      x4v = _mm_loadu_ps(xp+4);
#if 0
      
      x1v = _mm_loadu_ps(xp+1);
      x2v = _mm_loadu_ps(xp+2);
      x3v = _mm_loadu_ps(xp+3);
#else
      x2v = _mm_shuffle_ps(x0v, x4v, 0x4e);
      x1v = _mm_shuffle_ps(x0v, x2v, 0x99);
      x3v = _mm_shuffle_ps(x2v, x4v, 0x99);
#endif

      yi = _mm_add_ps(yi, _mm_mul_ps(g10v,x2v));
#if 0 
      yi = _mm_add_ps(yi, _mm_mul_ps(g11v,_mm_add_ps(x3v,x1v)));
      yi = _mm_add_ps(yi, _mm_mul_ps(g12v,_mm_add_ps(x4v,x0v)));
#else
      
      yi2 = _mm_add_ps(_mm_mul_ps(g11v,_mm_add_ps(x3v,x1v)),
                       _mm_mul_ps(g12v,_mm_add_ps(x4v,x0v)));
      yi = _mm_add_ps(yi, yi2);
#endif
      x0v=x4v;
      _mm_storeu_ps(y+i, yi);
   }
#ifdef CUSTOM_MODES
   for (;i<N;i++)
   {
      y[i] = x[i]
               + MULT16_32_Q15(g10,x[i-T])
               + MULT16_32_Q15(g11,ADD32(x[i-T+1],x[i-T-1]))
               + MULT16_32_Q15(g12,ADD32(x[i-T+2],x[i-T-2]));
   }
#endif
}

#endif
