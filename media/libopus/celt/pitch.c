








































#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pitch.h"
#include "os_support.h"
#include "modes.h"
#include "stack_alloc.h"
#include "mathops.h"
#include "celt_lpc.h"

static void find_best_pitch(opus_val32 *xcorr, opus_val16 *y, int len,
                            int max_pitch, int *best_pitch
#ifdef FIXED_POINT
                            , int yshift, opus_val32 maxcorr
#endif
                            )
{
   int i, j;
   opus_val32 Syy=1;
   opus_val16 best_num[2];
   opus_val32 best_den[2];
#ifdef FIXED_POINT
   int xshift;

   xshift = celt_ilog2(maxcorr)-14;
#endif

   best_num[0] = -1;
   best_num[1] = -1;
   best_den[0] = 0;
   best_den[1] = 0;
   best_pitch[0] = 0;
   best_pitch[1] = 1;
   for (j=0;j<len;j++)
      Syy = MAC16_16(Syy, y[j],y[j]);
   for (i=0;i<max_pitch;i++)
   {
      if (xcorr[i]>0)
      {
         opus_val16 num;
         opus_val32 xcorr16;
         xcorr16 = EXTRACT16(VSHR32(xcorr[i], xshift));
         num = MULT16_16_Q15(xcorr16,xcorr16);
         if (MULT16_32_Q15(num,best_den[1]) > MULT16_32_Q15(best_num[1],Syy))
         {
            if (MULT16_32_Q15(num,best_den[0]) > MULT16_32_Q15(best_num[0],Syy))
            {
               best_num[1] = best_num[0];
               best_den[1] = best_den[0];
               best_pitch[1] = best_pitch[0];
               best_num[0] = num;
               best_den[0] = Syy;
               best_pitch[0] = i;
            } else {
               best_num[1] = num;
               best_den[1] = Syy;
               best_pitch[1] = i;
            }
         }
      }
      Syy += SHR32(MULT16_16(y[i+len],y[i+len]),yshift) - SHR32(MULT16_16(y[i],y[i]),yshift);
      Syy = MAX32(1, Syy);
   }
}

void pitch_downsample(celt_sig * restrict x[], opus_val16 * restrict x_lp,
      int len, int C)
{
   int i;
   opus_val32 ac[5];
   opus_val16 tmp=Q15ONE;
   opus_val16 lpc[4], mem[4]={0,0,0,0};
   for (i=1;i<len>>1;i++)
      x_lp[i] = SHR32(HALF32(HALF32(x[0][(2*i-1)]+x[0][(2*i+1)])+x[0][2*i]), SIG_SHIFT+3);
   x_lp[0] = SHR32(HALF32(HALF32(x[0][1])+x[0][0]), SIG_SHIFT+3);
   if (C==2)
   {
      for (i=1;i<len>>1;i++)
         x_lp[i] += SHR32(HALF32(HALF32(x[1][(2*i-1)]+x[1][(2*i+1)])+x[1][2*i]), SIG_SHIFT+3);
      x_lp[0] += SHR32(HALF32(HALF32(x[1][1])+x[1][0]), SIG_SHIFT+3);
   }

   _celt_autocorr(x_lp, ac, NULL, 0,
                  4, len>>1);

   
#ifdef FIXED_POINT
   ac[0] += SHR32(ac[0],13);
#else
   ac[0] *= 1.0001f;
#endif
   
   for (i=1;i<=4;i++)
   {
      
#ifdef FIXED_POINT
      ac[i] -= MULT16_32_Q15(2*i*i, ac[i]);
#else
      ac[i] -= ac[i]*(.008f*i)*(.008f*i);
#endif
   }

   _celt_lpc(lpc, ac, 4);
   for (i=0;i<4;i++)
   {
      tmp = MULT16_16_Q15(QCONST16(.9f,15), tmp);
      lpc[i] = MULT16_16_Q15(lpc[i], tmp);
   }
   celt_fir(x_lp, lpc, x_lp, len>>1, 4, mem);

   mem[0]=0;
   lpc[0]=QCONST16(.8f,12);
   celt_fir(x_lp, lpc, x_lp, len>>1, 1, mem);

}

void pitch_search(const opus_val16 * restrict x_lp, opus_val16 * restrict y,
                  int len, int max_pitch, int *pitch)
{
   int i, j;
   int lag;
   int best_pitch[2]={0,0};
   VARDECL(opus_val16, x_lp4);
   VARDECL(opus_val16, y_lp4);
   VARDECL(opus_val32, xcorr);
#ifdef FIXED_POINT
   opus_val32 maxcorr=1;
   int shift=0;
#endif
   int offset;

   SAVE_STACK;

   celt_assert(len>0);
   celt_assert(max_pitch>0);
   lag = len+max_pitch;

   ALLOC(x_lp4, len>>2, opus_val16);
   ALLOC(y_lp4, lag>>2, opus_val16);
   ALLOC(xcorr, max_pitch>>1, opus_val32);

   
   for (j=0;j<len>>2;j++)
      x_lp4[j] = x_lp[2*j];
   for (j=0;j<lag>>2;j++)
      y_lp4[j] = y[2*j];

#ifdef FIXED_POINT
   shift = celt_ilog2(MAX16(1, MAX16(celt_maxabs16(x_lp4, len>>2), celt_maxabs16(y_lp4, lag>>2))))-11;
   if (shift>0)
   {
      for (j=0;j<len>>2;j++)
         x_lp4[j] = SHR16(x_lp4[j], shift);
      for (j=0;j<lag>>2;j++)
         y_lp4[j] = SHR16(y_lp4[j], shift);
      
      shift *= 2;
   } else {
      shift = 0;
   }
#endif

   

   for (i=0;i<max_pitch>>2;i++)
   {
      opus_val32 sum = 0;
      for (j=0;j<len>>2;j++)
         sum = MAC16_16(sum, x_lp4[j],y_lp4[i+j]);
      xcorr[i] = MAX32(-1, sum);
#ifdef FIXED_POINT
      maxcorr = MAX32(maxcorr, sum);
#endif
   }
   find_best_pitch(xcorr, y_lp4, len>>2, max_pitch>>2, best_pitch
#ifdef FIXED_POINT
                   , 0, maxcorr
#endif
                   );

   
#ifdef FIXED_POINT
   maxcorr=1;
#endif
   for (i=0;i<max_pitch>>1;i++)
   {
      opus_val32 sum=0;
      xcorr[i] = 0;
      if (abs(i-2*best_pitch[0])>2 && abs(i-2*best_pitch[1])>2)
         continue;
      for (j=0;j<len>>1;j++)
         sum += SHR32(MULT16_16(x_lp[j],y[i+j]), shift);
      xcorr[i] = MAX32(-1, sum);
#ifdef FIXED_POINT
      maxcorr = MAX32(maxcorr, sum);
#endif
   }
   find_best_pitch(xcorr, y, len>>1, max_pitch>>1, best_pitch
#ifdef FIXED_POINT
                   , shift, maxcorr
#endif
                   );

   
   if (best_pitch[0]>0 && best_pitch[0]<(max_pitch>>1)-1)
   {
      opus_val32 a, b, c;
      a = xcorr[best_pitch[0]-1];
      b = xcorr[best_pitch[0]];
      c = xcorr[best_pitch[0]+1];
      if ((c-a) > MULT16_32_Q15(QCONST16(.7f,15),b-a))
         offset = 1;
      else if ((a-c) > MULT16_32_Q15(QCONST16(.7f,15),b-c))
         offset = -1;
      else
         offset = 0;
   } else {
      offset = 0;
   }
   *pitch = 2*best_pitch[0]-offset;

   RESTORE_STACK;
}

static const int second_check[16] = {0, 0, 3, 2, 3, 2, 5, 2, 3, 2, 3, 2, 5, 2, 3, 2};
opus_val16 remove_doubling(opus_val16 *x, int maxperiod, int minperiod,
      int N, int *T0_, int prev_period, opus_val16 prev_gain)
{
   int k, i, T, T0;
   opus_val16 g, g0;
   opus_val16 pg;
   opus_val32 xy,xx,yy;
   opus_val32 xcorr[3];
   opus_val32 best_xy, best_yy;
   int offset;
   int minperiod0;

   minperiod0 = minperiod;
   maxperiod /= 2;
   minperiod /= 2;
   *T0_ /= 2;
   prev_period /= 2;
   N /= 2;
   x += maxperiod;
   if (*T0_>=maxperiod)
      *T0_=maxperiod-1;

   T = T0 = *T0_;
   xx=xy=yy=0;
   for (i=0;i<N;i++)
   {
      xy = MAC16_16(xy, x[i], x[i-T0]);
      xx = MAC16_16(xx, x[i], x[i]);
      yy = MAC16_16(yy, x[i-T0],x[i-T0]);
   }
   best_xy = xy;
   best_yy = yy;
#ifdef FIXED_POINT
      {
         opus_val32 x2y2;
         int sh, t;
         x2y2 = 1+HALF32(MULT32_32_Q31(xx,yy));
         sh = celt_ilog2(x2y2)>>1;
         t = VSHR32(x2y2, 2*(sh-7));
         g = g0 = VSHR32(MULT16_32_Q15(celt_rsqrt_norm(t), xy),sh+1);
      }
#else
      g = g0 = xy/celt_sqrt(1+xx*yy);
#endif
   
   for (k=2;k<=15;k++)
   {
      int T1, T1b;
      opus_val16 g1;
      opus_val16 cont=0;
      T1 = (2*T0+k)/(2*k);
      if (T1 < minperiod)
         break;
      
      if (k==2)
      {
         if (T1+T0>maxperiod)
            T1b = T0;
         else
            T1b = T0+T1;
      } else
      {
         T1b = (2*second_check[k]*T0+k)/(2*k);
      }
      xy=yy=0;
      for (i=0;i<N;i++)
      {
         xy = MAC16_16(xy, x[i], x[i-T1]);
         yy = MAC16_16(yy, x[i-T1], x[i-T1]);

         xy = MAC16_16(xy, x[i], x[i-T1b]);
         yy = MAC16_16(yy, x[i-T1b], x[i-T1b]);
      }
#ifdef FIXED_POINT
      {
         opus_val32 x2y2;
         int sh, t;
         x2y2 = 1+MULT32_32_Q31(xx,yy);
         sh = celt_ilog2(x2y2)>>1;
         t = VSHR32(x2y2, 2*(sh-7));
         g1 = VSHR32(MULT16_32_Q15(celt_rsqrt_norm(t), xy),sh+1);
      }
#else
      g1 = xy/celt_sqrt(1+2.f*xx*1.f*yy);
#endif
      if (abs(T1-prev_period)<=1)
         cont = prev_gain;
      else if (abs(T1-prev_period)<=2 && 5*k*k < T0)
         cont = HALF32(prev_gain);
      else
         cont = 0;
      if (g1 > QCONST16(.3f,15) + MULT16_16_Q15(QCONST16(.4f,15),g0)-cont)
      {
         best_xy = xy;
         best_yy = yy;
         T = T1;
         g = g1;
      }
   }
   if (best_yy <= best_xy)
      pg = Q15ONE;
   else
      pg = SHR32(frac_div32(best_xy,best_yy+1),16);

   for (k=0;k<3;k++)
   {
      int T1 = T+k-1;
      xy = 0;
      for (i=0;i<N;i++)
         xy = MAC16_16(xy, x[i], x[i-T1]);
      xcorr[k] = xy;
   }
   if ((xcorr[2]-xcorr[0]) > MULT16_32_Q15(QCONST16(.7f,15),xcorr[1]-xcorr[0]))
      offset = 1;
   else if ((xcorr[0]-xcorr[2]) > MULT16_32_Q15(QCONST16(.7f,15),xcorr[1]-xcorr[2]))
      offset = -1;
   else
      offset = 0;
   if (pg > g)
      pg = g;
   *T0_ = 2*T+offset;

   if (*T0_<minperiod0)
      *T0_=minperiod0;
   return pg;
}
