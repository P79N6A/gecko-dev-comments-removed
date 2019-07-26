




























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define CELT_ENCODER_C

#include "cpu_support.h"
#include "os_support.h"
#include "mdct.h"
#include <math.h>
#include "celt.h"
#include "pitch.h"
#include "bands.h"
#include "modes.h"
#include "entcode.h"
#include "quant_bands.h"
#include "rate.h"
#include "stack_alloc.h"
#include "mathops.h"
#include "float_cast.h"
#include <stdarg.h>
#include "celt_lpc.h"
#include "vq.h"





struct OpusCustomEncoder {
   const OpusCustomMode *mode;     
   int overlap;
   int channels;
   int stream_channels;

   int force_intra;
   int clip;
   int disable_pf;
   int complexity;
   int upsample;
   int start, end;

   opus_int32 bitrate;
   int vbr;
   int signalling;
   int constrained_vbr;      
   int loss_rate;
   int lsb_depth;
   int variable_duration;
   int lfe;
   int arch;

   
#define ENCODER_RESET_START rng

   opus_uint32 rng;
   int spread_decision;
   opus_val32 delayedIntra;
   int tonal_average;
   int lastCodedBands;
   int hf_average;
   int tapset_decision;

   int prefilter_period;
   opus_val16 prefilter_gain;
   int prefilter_tapset;
#ifdef RESYNTH
   int prefilter_period_old;
   opus_val16 prefilter_gain_old;
   int prefilter_tapset_old;
#endif
   int consec_transient;
   AnalysisInfo analysis;

   opus_val32 preemph_memE[2];
   opus_val32 preemph_memD[2];

   
   opus_int32 vbr_reservoir;
   opus_int32 vbr_drift;
   opus_int32 vbr_offset;
   opus_int32 vbr_count;
   opus_val32 overlap_max;
   opus_val16 stereo_saving;
   int intensity;
   opus_val16 *energy_save;
   opus_val16 *energy_mask;
   opus_val16 spec_avg;

#ifdef RESYNTH
   
   celt_sig syn_mem[2][2*MAX_PERIOD+MAX_PERIOD/2];
#endif

   celt_sig in_mem[1]; 
   
   
   
   
};

int celt_encoder_get_size(int channels)
{
   CELTMode *mode = opus_custom_mode_create(48000, 960, NULL);
   return opus_custom_encoder_get_size(mode, channels);
}

OPUS_CUSTOM_NOSTATIC int opus_custom_encoder_get_size(const CELTMode *mode, int channels)
{
   int size = sizeof(struct CELTEncoder)
         + (channels*mode->overlap-1)*sizeof(celt_sig)    
         + channels*COMBFILTER_MAXPERIOD*sizeof(celt_sig) 
         + 3*channels*mode->nbEBands*sizeof(opus_val16);  
                                                          
                                                          
   return size;
}

#ifdef CUSTOM_MODES
CELTEncoder *opus_custom_encoder_create(const CELTMode *mode, int channels, int *error)
{
   int ret;
   CELTEncoder *st = (CELTEncoder *)opus_alloc(opus_custom_encoder_get_size(mode, channels));
   
   ret = opus_custom_encoder_init(st, mode, channels);
   if (ret != OPUS_OK)
   {
      opus_custom_encoder_destroy(st);
      st = NULL;
   }
   if (error)
      *error = ret;
   return st;
}
#endif 

int celt_encoder_init(CELTEncoder *st, opus_int32 sampling_rate, int channels)
{
   int ret;
   ret = opus_custom_encoder_init(st, opus_custom_mode_create(48000, 960, NULL), channels);
   if (ret != OPUS_OK)
      return ret;
   st->upsample = resampling_factor(sampling_rate);
   return OPUS_OK;
}

OPUS_CUSTOM_NOSTATIC int opus_custom_encoder_init(CELTEncoder *st, const CELTMode *mode, int channels)
{
   if (channels < 0 || channels > 2)
      return OPUS_BAD_ARG;

   if (st==NULL || mode==NULL)
      return OPUS_ALLOC_FAIL;

   OPUS_CLEAR((char*)st, opus_custom_encoder_get_size(mode, channels));

   st->mode = mode;
   st->overlap = mode->overlap;
   st->stream_channels = st->channels = channels;

   st->upsample = 1;
   st->start = 0;
   st->end = st->mode->effEBands;
   st->signalling = 1;

   st->arch = opus_select_arch();

   st->constrained_vbr = 1;
   st->clip = 1;

   st->bitrate = OPUS_BITRATE_MAX;
   st->vbr = 0;
   st->force_intra  = 0;
   st->complexity = 5;
   st->lsb_depth=24;

   opus_custom_encoder_ctl(st, OPUS_RESET_STATE);

   return OPUS_OK;
}

#ifdef CUSTOM_MODES
void opus_custom_encoder_destroy(CELTEncoder *st)
{
   opus_free(st);
}
#endif 


static int transient_analysis(const opus_val32 * OPUS_RESTRICT in, int len, int C,
                              opus_val16 *tf_estimate, int *tf_chan)
{
   int i;
   VARDECL(opus_val16, tmp);
   opus_val32 mem0,mem1;
   int is_transient = 0;
   opus_int32 mask_metric = 0;
   int c;
   opus_val16 tf_max;
   int len2;
   
   static const unsigned char inv_table[128] = {
         255,255,156,110, 86, 70, 59, 51, 45, 40, 37, 33, 31, 28, 26, 25,
          23, 22, 21, 20, 19, 18, 17, 16, 16, 15, 15, 14, 13, 13, 12, 12,
          12, 12, 11, 11, 11, 10, 10, 10,  9,  9,  9,  9,  9,  9,  8,  8,
           8,  8,  8,  7,  7,  7,  7,  7,  7,  6,  6,  6,  6,  6,  6,  6,
           6,  6,  6,  6,  6,  6,  6,  6,  6,  5,  5,  5,  5,  5,  5,  5,
           5,  5,  5,  5,  5,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,
           4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  3,  3,
           3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  2,
   };
   SAVE_STACK;
   ALLOC(tmp, len, opus_val16);

   len2=len/2;
   tf_max = 0;
   for (c=0;c<C;c++)
   {
      opus_val32 mean;
      opus_int32 unmask=0;
      opus_val32 norm;
      opus_val16 maxE;
      mem0=0;
      mem1=0;
      
      for (i=0;i<len;i++)
      {
         opus_val32 x,y;
         x = SHR32(in[i+c*len],SIG_SHIFT);
         y = ADD32(mem0, x);
#ifdef FIXED_POINT
         mem0 = mem1 + y - SHL32(x,1);
         mem1 = x - SHR32(y,1);
#else
         mem0 = mem1 + y - 2*x;
         mem1 = x - .5f*y;
#endif
         tmp[i] = EXTRACT16(SHR32(y,2));
         
      }
      
      
      for (i=0;i<12;i++)
         tmp[i] = 0;

#ifdef FIXED_POINT
      
      {
         int shift=0;
         shift = 14-celt_ilog2(1+celt_maxabs16(tmp, len));
         if (shift!=0)
         {
            for (i=0;i<len;i++)
               tmp[i] = SHL16(tmp[i], shift);
         }
      }
#endif

      mean=0;
      mem0=0;
      
      
      for (i=0;i<len2;i++)
      {
         opus_val16 x2 = PSHR32(MULT16_16(tmp[2*i],tmp[2*i]) + MULT16_16(tmp[2*i+1],tmp[2*i+1]),16);
         mean += x2;
#ifdef FIXED_POINT
         
         tmp[i] = mem0 + PSHR32(x2-mem0,4);
#else
         tmp[i] = mem0 + MULT16_16_P15(QCONST16(.0625f,15),x2-mem0);
#endif
         mem0 = tmp[i];
      }

      mem0=0;
      maxE=0;
      
      for (i=len2-1;i>=0;i--)
      {
#ifdef FIXED_POINT
         
         tmp[i] = mem0 + PSHR32(tmp[i]-mem0,3);
#else
         tmp[i] = mem0 + MULT16_16_P15(QCONST16(0.125f,15),tmp[i]-mem0);
#endif
         mem0 = tmp[i];
         maxE = MAX16(maxE, mem0);
      }
      

      



      

#ifdef FIXED_POINT
      
      mean = MULT16_16(celt_sqrt(mean), celt_sqrt(MULT16_16(maxE,len2>>1)));
#else
      mean = celt_sqrt(mean * maxE*.5*len2);
#endif
      
      norm = SHL32(EXTEND32(len2),6+14)/ADD32(EPSILON,SHR32(mean,1));
      

      unmask=0;
      for (i=12;i<len2-5;i+=4)
      {
         int id;
#ifdef FIXED_POINT
         id = IMAX(0,IMIN(127,MULT16_32_Q15(tmp[i],norm))); 
#else
         id = IMAX(0,IMIN(127,(int)floor(64*norm*tmp[i]))); 
#endif
         unmask += inv_table[id];
      }
      
      
      unmask = 64*unmask*4/(6*(len2-17));
      if (unmask>mask_metric)
      {
         *tf_chan = c;
         mask_metric = unmask;
      }
   }
   is_transient = mask_metric>200;

   
   tf_max = MAX16(0,celt_sqrt(27*mask_metric)-42);
   
   *tf_estimate = celt_sqrt(MAX16(0, SHL32(MULT16_16(QCONST16(0.0069,14),MIN16(163,tf_max)),14)-QCONST32(0.139,28)));
   
   RESTORE_STACK;
#ifdef FUZZING
   is_transient = rand()&0x1;
#endif
   
   return is_transient;
}



int patch_transient_decision(opus_val16 *new, opus_val16 *old, int nbEBands,
      int end, int C)
{
   int i, c;
   opus_val32 mean_diff=0;
   opus_val16 spread_old[26];
   

   if (C==1)
   {
      spread_old[0] = old[0];
      for (i=1;i<end;i++)
         spread_old[i] = MAX16(spread_old[i-1]-QCONST16(1.0f, DB_SHIFT), old[i]);
   } else {
      spread_old[0] = MAX16(old[0],old[nbEBands]);
      for (i=1;i<end;i++)
         spread_old[i] = MAX16(spread_old[i-1]-QCONST16(1.0f, DB_SHIFT),
                               MAX16(old[i],old[i+nbEBands]));
   }
   for (i=end-2;i>=0;i--)
      spread_old[i] = MAX16(spread_old[i], spread_old[i+1]-QCONST16(1.0f, DB_SHIFT));
   
   c=0; do {
      for (i=2;i<end-1;i++)
      {
         opus_val16 x1, x2;
         x1 = MAX16(0, new[i]);
         x2 = MAX16(0, spread_old[i]);
         mean_diff = ADD32(mean_diff, EXTEND32(MAX16(0, SUB16(x1, x2))));
      }
   } while (++c<C);
   mean_diff = DIV32(mean_diff, C*(end-3));
   
   return mean_diff > QCONST16(1.f, DB_SHIFT);
}



static void compute_mdcts(const CELTMode *mode, int shortBlocks, celt_sig * OPUS_RESTRICT in,
                          celt_sig * OPUS_RESTRICT out, int C, int CC, int LM, int upsample)
{
   const int overlap = OVERLAP(mode);
   int N;
   int B;
   int shift;
   int i, b, c;
   if (shortBlocks)
   {
      B = shortBlocks;
      N = mode->shortMdctSize;
      shift = mode->maxLM;
   } else {
      B = 1;
      N = mode->shortMdctSize<<LM;
      shift = mode->maxLM-LM;
   }
   c=0; do {
      for (b=0;b<B;b++)
      {
         
         clt_mdct_forward(&mode->mdct, in+c*(B*N+overlap)+b*N, &out[b+c*N*B], mode->window, overlap, shift, B);
      }
   } while (++c<CC);
   if (CC==2&&C==1)
   {
      for (i=0;i<B*N;i++)
         out[i] = ADD32(HALF32(out[i]), HALF32(out[B*N+i]));
   }
   if (upsample != 1)
   {
      c=0; do
      {
         int bound = B*N/upsample;
         for (i=0;i<bound;i++)
            out[c*B*N+i] *= upsample;
         for (;i<B*N;i++)
            out[c*B*N+i] = 0;
      } while (++c<C);
   }
}


static void preemphasis(const opus_val16 * OPUS_RESTRICT pcmp, celt_sig * OPUS_RESTRICT inp,
                        int N, int CC, int upsample, const opus_val16 *coef, celt_sig *mem, int clip)
{
   int i;
   opus_val16 coef0;
   celt_sig m;
   int Nu;

   coef0 = coef[0];


   Nu = N/upsample;
   if (upsample!=1)
   {
      for (i=0;i<N;i++)
         inp[i] = 0;
   }
   for (i=0;i<Nu;i++)
   {
      celt_sig x;

      x = SCALEIN(pcmp[CC*i]);
#ifndef FIXED_POINT
      
      if (!(x==x))
         x = 0;
#endif
      inp[i*upsample] = x;
   }

#ifndef FIXED_POINT
   if (clip)
   {
      
      for (i=0;i<Nu;i++)
         inp[i*upsample] = MAX32(-65536.f, MIN32(65536.f,inp[i*upsample]));
   }
#endif
   m = *mem;
#ifdef CUSTOM_MODES
   if (coef[1] != 0)
   {
      opus_val16 coef1 = coef[1];
      opus_val16 coef2 = coef[2];
      for (i=0;i<N;i++)
      {
         opus_val16 x, tmp;
         x = inp[i];
         
         tmp = MULT16_16(coef2, x);
         inp[i] = tmp + m;
         m = MULT16_32_Q15(coef1, inp[i]) - MULT16_32_Q15(coef0, tmp);
      }
   } else
#endif
   {
      for (i=0;i<N;i++)
      {
         celt_sig x;
         x = SHL32(inp[i], SIG_SHIFT);
         
         inp[i] = x + m;
         m = - MULT16_32_Q15(coef0, x);
      }
   }
   *mem = m;
}



static opus_val32 l1_metric(const celt_norm *tmp, int N, int LM, opus_val16 bias)
{
   int i;
   opus_val32 L1;
   L1 = 0;
   for (i=0;i<N;i++)
      L1 += EXTEND32(ABS16(tmp[i]));
   
   L1 = MAC16_32_Q15(L1, LM*bias, L1);
   return L1;

}

static int tf_analysis(const CELTMode *m, int len, int isTransient,
      int *tf_res, int lambda, celt_norm *X, int N0, int LM,
      int *tf_sum, opus_val16 tf_estimate, int tf_chan)
{
   int i;
   VARDECL(int, metric);
   int cost0;
   int cost1;
   VARDECL(int, path0);
   VARDECL(int, path1);
   VARDECL(celt_norm, tmp);
   VARDECL(celt_norm, tmp_1);
   int sel;
   int selcost[2];
   int tf_select=0;
   opus_val16 bias;

   SAVE_STACK;
   bias = MULT16_16_Q14(QCONST16(.04f,15), MAX16(-QCONST16(.25f,14), QCONST16(.5f,14)-tf_estimate));
   

   ALLOC(metric, len, int);
   ALLOC(tmp, (m->eBands[len]-m->eBands[len-1])<<LM, celt_norm);
   ALLOC(tmp_1, (m->eBands[len]-m->eBands[len-1])<<LM, celt_norm);
   ALLOC(path0, len, int);
   ALLOC(path1, len, int);

   *tf_sum = 0;
   for (i=0;i<len;i++)
   {
      int j, k, N;
      int narrow;
      opus_val32 L1, best_L1;
      int best_level=0;
      N = (m->eBands[i+1]-m->eBands[i])<<LM;
      
      narrow = (m->eBands[i+1]-m->eBands[i])==1;
      for (j=0;j<N;j++)
         tmp[j] = X[tf_chan*N0 + j+(m->eBands[i]<<LM)];
      
      


      L1 = l1_metric(tmp, N, isTransient ? LM : 0, bias);
      best_L1 = L1;
      
      if (isTransient && !narrow)
      {
         for (j=0;j<N;j++)
            tmp_1[j] = tmp[j];
         haar1(tmp_1, N>>LM, 1<<LM);
         L1 = l1_metric(tmp_1, N, LM+1, bias);
         if (L1<best_L1)
         {
            best_L1 = L1;
            best_level = -1;
         }
      }
      
      for (k=0;k<LM+!(isTransient||narrow);k++)
      {
         int B;

         if (isTransient)
            B = (LM-k-1);
         else
            B = k+1;

         haar1(tmp, N>>k, 1<<k);

         L1 = l1_metric(tmp, N, B, bias);

         if (L1 < best_L1)
         {
            best_L1 = L1;
            best_level = k+1;
         }
      }
      
      
      if (isTransient)
         metric[i] = 2*best_level;
      else
         metric[i] = -2*best_level;
      *tf_sum += (isTransient ? LM : 0) - metric[i]/2;
      

      if (narrow && (metric[i]==0 || metric[i]==-2*LM))
         metric[i]-=1;
      
   }
   
   
   tf_select = 0;
   for (sel=0;sel<2;sel++)
   {
      cost0 = 0;
      cost1 = isTransient ? 0 : lambda;
      for (i=1;i<len;i++)
      {
         int curr0, curr1;
         curr0 = IMIN(cost0, cost1 + lambda);
         curr1 = IMIN(cost0 + lambda, cost1);
         cost0 = curr0 + abs(metric[i]-2*tf_select_table[LM][4*isTransient+2*sel+0]);
         cost1 = curr1 + abs(metric[i]-2*tf_select_table[LM][4*isTransient+2*sel+1]);
      }
      cost0 = IMIN(cost0, cost1);
      selcost[sel]=cost0;
   }
   

   if (selcost[1]<selcost[0] && isTransient)
      tf_select=1;
   cost0 = 0;
   cost1 = isTransient ? 0 : lambda;
   
   for (i=1;i<len;i++)
   {
      int curr0, curr1;
      int from0, from1;

      from0 = cost0;
      from1 = cost1 + lambda;
      if (from0 < from1)
      {
         curr0 = from0;
         path0[i]= 0;
      } else {
         curr0 = from1;
         path0[i]= 1;
      }

      from0 = cost0 + lambda;
      from1 = cost1;
      if (from0 < from1)
      {
         curr1 = from0;
         path1[i]= 0;
      } else {
         curr1 = from1;
         path1[i]= 1;
      }
      cost0 = curr0 + abs(metric[i]-2*tf_select_table[LM][4*isTransient+2*tf_select+0]);
      cost1 = curr1 + abs(metric[i]-2*tf_select_table[LM][4*isTransient+2*tf_select+1]);
   }
   tf_res[len-1] = cost0 < cost1 ? 0 : 1;
   
   for (i=len-2;i>=0;i--)
   {
      if (tf_res[i+1] == 1)
         tf_res[i] = path1[i+1];
      else
         tf_res[i] = path0[i+1];
   }
   
   RESTORE_STACK;
#ifdef FUZZING
   tf_select = rand()&0x1;
   tf_res[0] = rand()&0x1;
   for (i=1;i<len;i++)
      tf_res[i] = tf_res[i-1] ^ ((rand()&0xF) == 0);
#endif
   return tf_select;
}

static void tf_encode(int start, int end, int isTransient, int *tf_res, int LM, int tf_select, ec_enc *enc)
{
   int curr, i;
   int tf_select_rsv;
   int tf_changed;
   int logp;
   opus_uint32 budget;
   opus_uint32 tell;
   budget = enc->storage*8;
   tell = ec_tell(enc);
   logp = isTransient ? 2 : 4;
   
   tf_select_rsv = LM>0 && tell+logp+1 <= budget;
   budget -= tf_select_rsv;
   curr = tf_changed = 0;
   for (i=start;i<end;i++)
   {
      if (tell+logp<=budget)
      {
         ec_enc_bit_logp(enc, tf_res[i] ^ curr, logp);
         tell = ec_tell(enc);
         curr = tf_res[i];
         tf_changed |= curr;
      }
      else
         tf_res[i] = curr;
      logp = isTransient ? 4 : 5;
   }
   
   if (tf_select_rsv &&
         tf_select_table[LM][4*isTransient+0+tf_changed]!=
         tf_select_table[LM][4*isTransient+2+tf_changed])
      ec_enc_bit_logp(enc, tf_select, 1);
   else
      tf_select = 0;
   for (i=start;i<end;i++)
      tf_res[i] = tf_select_table[LM][4*isTransient+2*tf_select+tf_res[i]];
   
}


static int alloc_trim_analysis(const CELTMode *m, const celt_norm *X,
      const opus_val16 *bandLogE, int end, int LM, int C, int N0,
      AnalysisInfo *analysis, opus_val16 *stereo_saving, opus_val16 tf_estimate,
      int intensity)
{
   int i;
   opus_val32 diff=0;
   int c;
   int trim_index = 5;
   opus_val16 trim = QCONST16(5.f, 8);
   opus_val16 logXC, logXC2;
   if (C==2)
   {
      opus_val16 sum = 0; 
      opus_val16 minXC; 
      
      for (i=0;i<8;i++)
      {
         int j;
         opus_val32 partial = 0;
         for (j=m->eBands[i]<<LM;j<m->eBands[i+1]<<LM;j++)
            partial = MAC16_16(partial, X[j], X[N0+j]);
         sum = ADD16(sum, EXTRACT16(SHR32(partial, 18)));
      }
      sum = MULT16_16_Q15(QCONST16(1.f/8, 15), sum);
      sum = MIN16(QCONST16(1.f, 10), ABS16(sum));
      minXC = sum;
      for (i=8;i<intensity;i++)
      {
         int j;
         opus_val32 partial = 0;
         for (j=m->eBands[i]<<LM;j<m->eBands[i+1]<<LM;j++)
            partial = MAC16_16(partial, X[j], X[N0+j]);
         minXC = MIN16(minXC, ABS16(EXTRACT16(SHR32(partial, 18))));
      }
      minXC = MIN16(QCONST16(1.f, 10), ABS16(minXC));
      
      if (sum > QCONST16(.995f,10))
         trim_index-=4;
      else if (sum > QCONST16(.92f,10))
         trim_index-=3;
      else if (sum > QCONST16(.85f,10))
         trim_index-=2;
      else if (sum > QCONST16(.8f,10))
         trim_index-=1;
      
      logXC = celt_log2(QCONST32(1.001f, 20)-MULT16_16(sum, sum));
      
      logXC2 = MAX16(HALF16(logXC), celt_log2(QCONST32(1.001f, 20)-MULT16_16(minXC, minXC)));
#ifdef FIXED_POINT
      
      logXC = PSHR32(logXC-QCONST16(6.f, DB_SHIFT),DB_SHIFT-8);
      logXC2 = PSHR32(logXC2-QCONST16(6.f, DB_SHIFT),DB_SHIFT-8);
#endif

      trim += MAX16(-QCONST16(4.f, 8), MULT16_16_Q15(QCONST16(.75f,15),logXC));
      *stereo_saving = MIN16(*stereo_saving + QCONST16(0.25f, 8), -HALF16(logXC2));
   }

   
   c=0; do {
      for (i=0;i<end-1;i++)
      {
         diff += bandLogE[i+c*m->nbEBands]*(opus_int32)(2+2*i-end);
      }
   } while (++c<C);
   diff /= C*(end-1);
   
   if (diff > QCONST16(2.f, DB_SHIFT))
      trim_index--;
   if (diff > QCONST16(8.f, DB_SHIFT))
      trim_index--;
   if (diff < -QCONST16(4.f, DB_SHIFT))
      trim_index++;
   if (diff < -QCONST16(10.f, DB_SHIFT))
      trim_index++;
   trim -= MAX16(-QCONST16(2.f, 8), MIN16(QCONST16(2.f, 8), SHR16(diff+QCONST16(1.f, DB_SHIFT),DB_SHIFT-8)/6 ));
   trim -= 2*SHR16(tf_estimate, 14-8);
#ifndef FIXED_POINT
   if (analysis->valid)
   {
      trim -= MAX16(-QCONST16(2.f, 8), MIN16(QCONST16(2.f, 8), 2*(analysis->tonality_slope+.05f)));
   }
#endif

#ifdef FIXED_POINT
   trim_index = PSHR32(trim, 8);
#else
   trim_index = (int)floor(.5f+trim);
#endif
   if (trim_index<0)
      trim_index = 0;
   if (trim_index>10)
      trim_index = 10;
   
#ifdef FUZZING
   trim_index = rand()%11;
#endif
   return trim_index;
}

static int stereo_analysis(const CELTMode *m, const celt_norm *X,
      int LM, int N0)
{
   int i;
   int thetas;
   opus_val32 sumLR = EPSILON, sumMS = EPSILON;

   
   for (i=0;i<13;i++)
   {
      int j;
      for (j=m->eBands[i]<<LM;j<m->eBands[i+1]<<LM;j++)
      {
         opus_val32 L, R, M, S;
         
         L = EXTEND32(X[j]);
         R = EXTEND32(X[N0+j]);
         M = ADD32(L, R);
         S = SUB32(L, R);
         sumLR = ADD32(sumLR, ADD32(ABS32(L), ABS32(R)));
         sumMS = ADD32(sumMS, ADD32(ABS32(M), ABS32(S)));
      }
   }
   sumMS = MULT16_32_Q15(QCONST16(0.707107f, 15), sumMS);
   thetas = 13;
   
   if (LM<=1)
      thetas -= 8;
   return MULT16_32_Q15((m->eBands[13]<<(LM+1))+thetas, sumMS)
         > MULT16_32_Q15(m->eBands[13]<<(LM+1), sumLR);
}

static opus_val16 dynalloc_analysis(const opus_val16 *bandLogE, const opus_val16 *bandLogE2,
      int nbEBands, int start, int end, int C, int *offsets, int lsb_depth, const opus_int16 *logN,
      int isTransient, int vbr, int constrained_vbr, const opus_int16 *eBands, int LM,
      int effectiveBytes, opus_int32 *tot_boost_, int lfe)
{
   int i, c;
   opus_int32 tot_boost=0;
   opus_val16 maxDepth;
   VARDECL(opus_val16, follower);
   VARDECL(opus_val16, noise_floor);
   SAVE_STACK;
   ALLOC(follower, C*nbEBands, opus_val16);
   ALLOC(noise_floor, C*nbEBands, opus_val16);
   for (i=0;i<nbEBands;i++)
      offsets[i] = 0;
   
   maxDepth=-QCONST16(31.9f, DB_SHIFT);
   for (i=0;i<end;i++)
   {
      

      noise_floor[i] = MULT16_16(QCONST16(0.0625f, DB_SHIFT),logN[i])
            +QCONST16(.5f,DB_SHIFT)+SHL16(9-lsb_depth,DB_SHIFT)-SHL16(eMeans[i],6)
            +MULT16_16(QCONST16(.0062,DB_SHIFT),(i+5)*(i+5));
   }
   c=0;do
   {
      for (i=0;i<end;i++)
         maxDepth = MAX16(maxDepth, bandLogE[c*nbEBands+i]-noise_floor[i]);
   } while (++c<C);
   
   if (effectiveBytes > 50 && LM>=1 && !lfe)
   {
      int last=0;
      c=0;do
      {
         follower[c*nbEBands] = bandLogE2[c*nbEBands];
         for (i=1;i<end;i++)
         {
            


            if (bandLogE2[c*nbEBands+i] > bandLogE2[c*nbEBands+i-1]+QCONST16(.5f,DB_SHIFT))
               last=i;
            follower[c*nbEBands+i] = MIN16(follower[c*nbEBands+i-1]+QCONST16(1.5f,DB_SHIFT), bandLogE2[c*nbEBands+i]);
         }
         for (i=last-1;i>=0;i--)
            follower[c*nbEBands+i] = MIN16(follower[c*nbEBands+i], MIN16(follower[c*nbEBands+i+1]+QCONST16(2.f,DB_SHIFT), bandLogE2[c*nbEBands+i]));
         for (i=0;i<end;i++)
            follower[c*nbEBands+i] = MAX16(follower[c*nbEBands+i], noise_floor[i]);
      } while (++c<C);
      if (C==2)
      {
         for (i=start;i<end;i++)
         {
            
            follower[nbEBands+i] = MAX16(follower[nbEBands+i], follower[         i]-QCONST16(4.f,DB_SHIFT));
            follower[         i] = MAX16(follower[         i], follower[nbEBands+i]-QCONST16(4.f,DB_SHIFT));
            follower[i] = HALF16(MAX16(0, bandLogE[i]-follower[i]) + MAX16(0, bandLogE[nbEBands+i]-follower[nbEBands+i]));
         }
      } else {
         for (i=start;i<end;i++)
         {
            follower[i] = MAX16(0, bandLogE[i]-follower[i]);
         }
      }
      
      if ((!vbr || constrained_vbr)&&!isTransient)
      {
         for (i=start;i<end;i++)
            follower[i] = HALF16(follower[i]);
      }
      for (i=start;i<end;i++)
      {
         int width;
         int boost;
         int boost_bits;

         if (i<8)
            follower[i] *= 2;
         if (i>=12)
            follower[i] = HALF16(follower[i]);
         follower[i] = MIN16(follower[i], QCONST16(4, DB_SHIFT));

         width = C*(eBands[i+1]-eBands[i])<<LM;
         if (width<6)
         {
            boost = (int)SHR32(EXTEND32(follower[i]),DB_SHIFT);
            boost_bits = boost*width<<BITRES;
         } else if (width > 48) {
            boost = (int)SHR32(EXTEND32(follower[i])*8,DB_SHIFT);
            boost_bits = (boost*width<<BITRES)/8;
         } else {
            boost = (int)SHR32(EXTEND32(follower[i])*width/6,DB_SHIFT);
            boost_bits = boost*6<<BITRES;
         }
         
         if ((!vbr || (constrained_vbr&&!isTransient))
               && (tot_boost+boost_bits)>>BITRES>>3 > effectiveBytes/4)
         {
            opus_int32 cap = ((effectiveBytes/4)<<BITRES<<3);
            offsets[i] = cap-tot_boost;
            tot_boost = cap;
            break;
         } else {
            offsets[i] = boost;
            tot_boost += boost_bits;
         }
      }
   }
   *tot_boost_ = tot_boost;
   RESTORE_STACK;
   return maxDepth;
}


static int run_prefilter(CELTEncoder *st, celt_sig *in, celt_sig *prefilter_mem, int CC, int N,
      int prefilter_tapset, int *pitch, opus_val16 *gain, int *qgain, int enabled, int nbAvailableBytes)
{
   int c;
   VARDECL(celt_sig, _pre);
   celt_sig *pre[2];
   const CELTMode *mode;
   int pitch_index;
   opus_val16 gain1;
   opus_val16 pf_threshold;
   int pf_on;
   int qg;
   SAVE_STACK;

   mode = st->mode;
   ALLOC(_pre, CC*(N+COMBFILTER_MAXPERIOD), celt_sig);

   pre[0] = _pre;
   pre[1] = _pre + (N+COMBFILTER_MAXPERIOD);


   c=0; do {
      OPUS_COPY(pre[c], prefilter_mem+c*COMBFILTER_MAXPERIOD, COMBFILTER_MAXPERIOD);
      OPUS_COPY(pre[c]+COMBFILTER_MAXPERIOD, in+c*(N+st->overlap)+st->overlap, N);
   } while (++c<CC);

   if (enabled)
   {
      VARDECL(opus_val16, pitch_buf);
      ALLOC(pitch_buf, (COMBFILTER_MAXPERIOD+N)>>1, opus_val16);

      pitch_downsample(pre, pitch_buf, COMBFILTER_MAXPERIOD+N, CC);
      

      pitch_search(pitch_buf+(COMBFILTER_MAXPERIOD>>1), pitch_buf, N,
            COMBFILTER_MAXPERIOD-3*COMBFILTER_MINPERIOD, &pitch_index);
      pitch_index = COMBFILTER_MAXPERIOD-pitch_index;

      gain1 = remove_doubling(pitch_buf, COMBFILTER_MAXPERIOD, COMBFILTER_MINPERIOD,
            N, &pitch_index, st->prefilter_period, st->prefilter_gain);
      if (pitch_index > COMBFILTER_MAXPERIOD-2)
         pitch_index = COMBFILTER_MAXPERIOD-2;
      gain1 = MULT16_16_Q15(QCONST16(.7f,15),gain1);
      
      if (st->loss_rate>2)
         gain1 = HALF32(gain1);
      if (st->loss_rate>4)
         gain1 = HALF32(gain1);
      if (st->loss_rate>8)
         gain1 = 0;
   } else {
      gain1 = 0;
      pitch_index = COMBFILTER_MINPERIOD;
   }

   
   pf_threshold = QCONST16(.2f,15);

   
   if (abs(pitch_index-st->prefilter_period)*10>pitch_index)
      pf_threshold += QCONST16(.2f,15);
   if (nbAvailableBytes<25)
      pf_threshold += QCONST16(.1f,15);
   if (nbAvailableBytes<35)
      pf_threshold += QCONST16(.1f,15);
   if (st->prefilter_gain > QCONST16(.4f,15))
      pf_threshold -= QCONST16(.1f,15);
   if (st->prefilter_gain > QCONST16(.55f,15))
      pf_threshold -= QCONST16(.1f,15);

   
   pf_threshold = MAX16(pf_threshold, QCONST16(.2f,15));
   if (gain1<pf_threshold)
   {
      gain1 = 0;
      pf_on = 0;
      qg = 0;
   } else {
      

      if (ABS16(gain1-st->prefilter_gain)<QCONST16(.1f,15))
         gain1=st->prefilter_gain;

#ifdef FIXED_POINT
      qg = ((gain1+1536)>>10)/3-1;
#else
      qg = (int)floor(.5f+gain1*32/3)-1;
#endif
      qg = IMAX(0, IMIN(7, qg));
      gain1 = QCONST16(0.09375f,15)*(qg+1);
      pf_on = 1;
   }
   

   c=0; do {
      int offset = mode->shortMdctSize-st->overlap;
      st->prefilter_period=IMAX(st->prefilter_period, COMBFILTER_MINPERIOD);
      OPUS_COPY(in+c*(N+st->overlap), st->in_mem+c*(st->overlap), st->overlap);
      if (offset)
         comb_filter(in+c*(N+st->overlap)+st->overlap, pre[c]+COMBFILTER_MAXPERIOD,
               st->prefilter_period, st->prefilter_period, offset, -st->prefilter_gain, -st->prefilter_gain,
               st->prefilter_tapset, st->prefilter_tapset, NULL, 0);

      comb_filter(in+c*(N+st->overlap)+st->overlap+offset, pre[c]+COMBFILTER_MAXPERIOD+offset,
            st->prefilter_period, pitch_index, N-offset, -st->prefilter_gain, -gain1,
            st->prefilter_tapset, prefilter_tapset, mode->window, st->overlap);
      OPUS_COPY(st->in_mem+c*(st->overlap), in+c*(N+st->overlap)+N, st->overlap);

      if (N>COMBFILTER_MAXPERIOD)
      {
         OPUS_MOVE(prefilter_mem+c*COMBFILTER_MAXPERIOD, pre[c]+N, COMBFILTER_MAXPERIOD);
      } else {
         OPUS_MOVE(prefilter_mem+c*COMBFILTER_MAXPERIOD, prefilter_mem+c*COMBFILTER_MAXPERIOD+N, COMBFILTER_MAXPERIOD-N);
         OPUS_MOVE(prefilter_mem+c*COMBFILTER_MAXPERIOD+COMBFILTER_MAXPERIOD-N, pre[c]+COMBFILTER_MAXPERIOD, N);
      }
   } while (++c<CC);

   RESTORE_STACK;
   *gain = gain1;
   *pitch = pitch_index;
   *qgain = qg;
   return pf_on;
}

static int compute_vbr(const CELTMode *mode, AnalysisInfo *analysis, opus_int32 base_target,
      int LM, opus_int32 bitrate, int lastCodedBands, int C, int intensity,
      int constrained_vbr, opus_val16 stereo_saving, int tot_boost,
      opus_val16 tf_estimate, int pitch_change, opus_val16 maxDepth,
      int variable_duration, int lfe, int has_surround_mask, opus_val16 surround_masking,
      opus_val16 temporal_vbr)
{
   
   opus_int32 target;
   int coded_bins;
   int coded_bands;
   opus_val16 tf_calibration;
   int nbEBands;
   const opus_int16 *eBands;

   nbEBands = mode->nbEBands;
   eBands = mode->eBands;

   coded_bands = lastCodedBands ? lastCodedBands : nbEBands;
   coded_bins = eBands[coded_bands]<<LM;
   if (C==2)
      coded_bins += eBands[IMIN(intensity, coded_bands)]<<LM;

   target = base_target;

   
#ifndef FIXED_POINT
   if (analysis->valid && analysis->activity<.4)
      target -= (opus_int32)((coded_bins<<BITRES)*(.4f-analysis->activity));
#endif
   
   if (C==2)
   {
      int coded_stereo_bands;
      int coded_stereo_dof;
      opus_val16 max_frac;
      coded_stereo_bands = IMIN(intensity, coded_bands);
      coded_stereo_dof = (eBands[coded_stereo_bands]<<LM)-coded_stereo_bands;
      
      max_frac = DIV32_16(MULT16_16(QCONST16(0.8f, 15), coded_stereo_dof), coded_bins);
      
      target -= (opus_int32)MIN32(MULT16_32_Q15(max_frac,target),
                      SHR32(MULT16_16(stereo_saving-QCONST16(0.1f,8),(coded_stereo_dof<<BITRES)),8));
   }
   
   target += tot_boost-(16<<LM);
   
   tf_calibration = variable_duration==OPUS_FRAMESIZE_VARIABLE ?
                    QCONST16(0.02f,14) : QCONST16(0.04f,14);
   target += (opus_int32)SHL32(MULT16_32_Q15(tf_estimate-tf_calibration, target),1);

#ifndef FIXED_POINT
   
   if (analysis->valid && !lfe)
   {
      opus_int32 tonal_target;
      float tonal;

      
      tonal = MAX16(0.f,analysis->tonality-.15f)-0.09f;
      tonal_target = target + (opus_int32)((coded_bins<<BITRES)*1.2f*tonal);
      if (pitch_change)
         tonal_target +=  (opus_int32)((coded_bins<<BITRES)*.8f);
      
      target = tonal_target;
   }
#endif

   if (has_surround_mask&&!lfe)
   {
      opus_int32 surround_target = target + (opus_int32)SHR32(MULT16_16(surround_masking,coded_bins<<BITRES), DB_SHIFT);
      
      target = IMAX(target/4, surround_target);
   }

   {
      opus_int32 floor_depth;
      int bins;
      bins = eBands[nbEBands-2]<<LM;
      
      floor_depth = (opus_int32)SHR32(MULT16_16((C*bins<<BITRES),maxDepth), DB_SHIFT);
      floor_depth = IMAX(floor_depth, target>>2);
      target = IMIN(target, floor_depth);
      
   }

   if ((!has_surround_mask||lfe) && (constrained_vbr || bitrate<64000))
   {
      opus_val16 rate_factor;
#ifdef FIXED_POINT
      rate_factor = MAX16(0,(bitrate-32000));
#else
      rate_factor = MAX16(0,(1.f/32768)*(bitrate-32000));
#endif
      if (constrained_vbr)
         rate_factor = MIN16(rate_factor, QCONST16(0.67f, 15));
      target = base_target + (opus_int32)MULT16_32_Q15(rate_factor, target-base_target);

   }

   if (!has_surround_mask && tf_estimate < QCONST16(.2f, 14))
   {
      opus_val16 amount;
      opus_val16 tvbr_factor;
      amount = MULT16_16_Q15(QCONST16(.0000031f, 30), IMAX(0, IMIN(32000, 96000-bitrate)));
      tvbr_factor = SHR32(MULT16_16(temporal_vbr, amount), DB_SHIFT);
      target += (opus_int32)MULT16_32_Q15(tvbr_factor, target);
   }

   
   target = IMIN(2*base_target, target);

   return target;
}

int celt_encode_with_ec(CELTEncoder * OPUS_RESTRICT st, const opus_val16 * pcm, int frame_size, unsigned char *compressed, int nbCompressedBytes, ec_enc *enc)
{
   int i, c, N;
   opus_int32 bits;
   ec_enc _enc;
   VARDECL(celt_sig, in);
   VARDECL(celt_sig, freq);
   VARDECL(celt_norm, X);
   VARDECL(celt_ener, bandE);
   VARDECL(opus_val16, bandLogE);
   VARDECL(opus_val16, bandLogE2);
   VARDECL(int, fine_quant);
   VARDECL(opus_val16, error);
   VARDECL(int, pulses);
   VARDECL(int, cap);
   VARDECL(int, offsets);
   VARDECL(int, fine_priority);
   VARDECL(int, tf_res);
   VARDECL(unsigned char, collapse_masks);
   celt_sig *prefilter_mem;
   opus_val16 *oldBandE, *oldLogE, *oldLogE2;
   int shortBlocks=0;
   int isTransient=0;
   const int CC = st->channels;
   const int C = st->stream_channels;
   int LM, M;
   int tf_select;
   int nbFilledBytes, nbAvailableBytes;
   int effEnd;
   int codedBands;
   int tf_sum;
   int alloc_trim;
   int pitch_index=COMBFILTER_MINPERIOD;
   opus_val16 gain1 = 0;
   int dual_stereo=0;
   int effectiveBytes;
   int dynalloc_logp;
   opus_int32 vbr_rate;
   opus_int32 total_bits;
   opus_int32 total_boost;
   opus_int32 balance;
   opus_int32 tell;
   int prefilter_tapset=0;
   int pf_on;
   int anti_collapse_rsv;
   int anti_collapse_on=0;
   int silence=0;
   int tf_chan = 0;
   opus_val16 tf_estimate;
   int pitch_change=0;
   opus_int32 tot_boost;
   opus_val32 sample_max;
   opus_val16 maxDepth;
   const OpusCustomMode *mode;
   int nbEBands;
   int overlap;
   const opus_int16 *eBands;
   int secondMdct;
   int signalBandwidth;
   int transient_got_disabled=0;
   opus_val16 surround_masking=0;
   opus_val16 temporal_vbr=0;
   ALLOC_STACK;

   mode = st->mode;
   nbEBands = mode->nbEBands;
   overlap = mode->overlap;
   eBands = mode->eBands;
   tf_estimate = 0;
   if (nbCompressedBytes<2 || pcm==NULL)
     return OPUS_BAD_ARG;

   frame_size *= st->upsample;
   for (LM=0;LM<=mode->maxLM;LM++)
      if (mode->shortMdctSize<<LM==frame_size)
         break;
   if (LM>mode->maxLM)
      return OPUS_BAD_ARG;
   M=1<<LM;
   N = M*mode->shortMdctSize;

   prefilter_mem = st->in_mem+CC*(st->overlap);
   oldBandE = (opus_val16*)(st->in_mem+CC*(st->overlap+COMBFILTER_MAXPERIOD));
   oldLogE = oldBandE + CC*nbEBands;
   oldLogE2 = oldLogE + CC*nbEBands;

   if (enc==NULL)
   {
      tell=1;
      nbFilledBytes=0;
   } else {
      tell=ec_tell(enc);
      nbFilledBytes=(tell+4)>>3;
   }

#ifdef CUSTOM_MODES
   if (st->signalling && enc==NULL)
   {
      int tmp = (mode->effEBands-st->end)>>1;
      st->end = IMAX(1, mode->effEBands-tmp);
      compressed[0] = tmp<<5;
      compressed[0] |= LM<<3;
      compressed[0] |= (C==2)<<2;
      
      if (mode->Fs==48000 && mode->shortMdctSize==120)
      {
         int c0 = toOpus(compressed[0]);
         if (c0<0)
            return OPUS_BAD_ARG;
         compressed[0] = c0;
      }
      compressed++;
      nbCompressedBytes--;
   }
#else
   celt_assert(st->signalling==0);
#endif

   
   nbCompressedBytes = IMIN(nbCompressedBytes,1275);
   nbAvailableBytes = nbCompressedBytes - nbFilledBytes;

   if (st->vbr && st->bitrate!=OPUS_BITRATE_MAX)
   {
      opus_int32 den=mode->Fs>>BITRES;
      vbr_rate=(st->bitrate*frame_size+(den>>1))/den;
#ifdef CUSTOM_MODES
      if (st->signalling)
         vbr_rate -= 8<<BITRES;
#endif
      effectiveBytes = vbr_rate>>(3+BITRES);
   } else {
      opus_int32 tmp;
      vbr_rate = 0;
      tmp = st->bitrate*frame_size;
      if (tell>1)
         tmp += tell;
      if (st->bitrate!=OPUS_BITRATE_MAX)
         nbCompressedBytes = IMAX(2, IMIN(nbCompressedBytes,
               (tmp+4*mode->Fs)/(8*mode->Fs)-!!st->signalling));
      effectiveBytes = nbCompressedBytes;
   }

   if (enc==NULL)
   {
      ec_enc_init(&_enc, compressed, nbCompressedBytes);
      enc = &_enc;
   }

   if (vbr_rate>0)
   {
      



      if (st->constrained_vbr)
      {
         opus_int32 vbr_bound;
         opus_int32 max_allowed;
         



         vbr_bound = vbr_rate;
         max_allowed = IMIN(IMAX(tell==1?2:0,
               (vbr_rate+vbr_bound-st->vbr_reservoir)>>(BITRES+3)),
               nbAvailableBytes);
         if(max_allowed < nbAvailableBytes)
         {
            nbCompressedBytes = nbFilledBytes+max_allowed;
            nbAvailableBytes = max_allowed;
            ec_enc_shrink(enc, nbCompressedBytes);
         }
      }
   }
   total_bits = nbCompressedBytes*8;

   effEnd = st->end;
   if (effEnd > mode->effEBands)
      effEnd = mode->effEBands;

   ALLOC(in, CC*(N+st->overlap), celt_sig);

   sample_max=MAX32(st->overlap_max, celt_maxabs16(pcm, C*(N-overlap)/st->upsample));
   st->overlap_max=celt_maxabs16(pcm+C*(N-overlap)/st->upsample, C*overlap/st->upsample);
   sample_max=MAX32(sample_max, st->overlap_max);
#ifdef FIXED_POINT
   silence = (sample_max==0);
#else
   silence = (sample_max <= (opus_val16)1/(1<<st->lsb_depth));
#endif
#ifdef FUZZING
   if ((rand()&0x3F)==0)
      silence = 1;
#endif
   if (tell==1)
      ec_enc_bit_logp(enc, silence, 15);
   else
      silence=0;
   if (silence)
   {
      
      if (vbr_rate>0)
      {
         effectiveBytes=nbCompressedBytes=IMIN(nbCompressedBytes, nbFilledBytes+2);
         total_bits=nbCompressedBytes*8;
         nbAvailableBytes=2;
         ec_enc_shrink(enc, nbCompressedBytes);
      }
      

      tell = nbCompressedBytes*8;
      enc->nbits_total+=tell-ec_tell(enc);
   }
   c=0; do {
      preemphasis(pcm+c, in+c*(N+st->overlap)+st->overlap, N, CC, st->upsample,
                  mode->preemph, st->preemph_memE+c, st->clip);
   } while (++c<CC);



   
   {
      int enabled;
      int qg;
      enabled = (st->lfe || nbAvailableBytes>12*C) && st->start==0 && !silence && !st->disable_pf
            && st->complexity >= 5 && !(st->consec_transient && LM!=3 && st->variable_duration==OPUS_FRAMESIZE_VARIABLE);

      prefilter_tapset = st->tapset_decision;
      pf_on = run_prefilter(st, in, prefilter_mem, CC, N, prefilter_tapset, &pitch_index, &gain1, &qg, enabled, nbAvailableBytes);
      if ((gain1 > QCONST16(.4f,15) || st->prefilter_gain > QCONST16(.4f,15)) && (!st->analysis.valid || st->analysis.tonality > .3)
            && (pitch_index > 1.26*st->prefilter_period || pitch_index < .79*st->prefilter_period))
         pitch_change = 1;
      if (pf_on==0)
      {
         if(st->start==0 && tell+16<=total_bits)
            ec_enc_bit_logp(enc, 0, 1);
      } else {
         

         int octave;
         ec_enc_bit_logp(enc, 1, 1);
         pitch_index += 1;
         octave = EC_ILOG(pitch_index)-5;
         ec_enc_uint(enc, octave, 6);
         ec_enc_bits(enc, pitch_index-(16<<octave), 4+octave);
         pitch_index -= 1;
         ec_enc_bits(enc, qg, 3);
         ec_enc_icdf(enc, prefilter_tapset, tapset_icdf, 2);
      }
   }

   isTransient = 0;
   shortBlocks = 0;
   if (st->complexity >= 1 && !st->lfe)
   {
      isTransient = transient_analysis(in, N+st->overlap, CC,
            &tf_estimate, &tf_chan);
   }
   if (LM>0 && ec_tell(enc)+3<=total_bits)
   {
      if (isTransient)
         shortBlocks = M;
   } else {
      isTransient = 0;
      transient_got_disabled=1;
   }

   ALLOC(freq, CC*N, celt_sig); 
   ALLOC(bandE,nbEBands*CC, celt_ener);
   ALLOC(bandLogE,nbEBands*CC, opus_val16);

   secondMdct = shortBlocks && st->complexity>=8;
   ALLOC(bandLogE2, C*nbEBands, opus_val16);
   if (secondMdct)
   {
      compute_mdcts(mode, 0, in, freq, C, CC, LM, st->upsample);
      compute_band_energies(mode, freq, bandE, effEnd, C, M);
      amp2Log2(mode, effEnd, st->end, bandE, bandLogE2, C);
      for (i=0;i<C*nbEBands;i++)
         bandLogE2[i] += HALF16(SHL16(LM, DB_SHIFT));
   }

   compute_mdcts(mode, shortBlocks, in, freq, C, CC, LM, st->upsample);
   if (CC==2&&C==1)
      tf_chan = 0;
   compute_band_energies(mode, freq, bandE, effEnd, C, M);

   if (st->lfe)
   {
      for (i=2;i<st->end;i++)
      {
         bandE[i] = IMIN(bandE[i], MULT16_32_Q15(QCONST16(1e-4f,15),bandE[0]));
         bandE[i] = MAX32(bandE[i], EPSILON);
      }
   }
   amp2Log2(mode, effEnd, st->end, bandE, bandLogE, C);
   if (st->energy_save)
   {
      opus_val16 offset = shortBlocks?HALF16(SHL16(LM, DB_SHIFT)):0;
#ifdef FIXED_POINT
      
      offset -= QCONST16(3.0f, DB_SHIFT);
#endif
      for(i=0;i<C*nbEBands;i++)
         st->energy_save[i]=bandLogE[i]-offset;
      st->energy_save=NULL;
   }
   
   if (st->energy_mask&&!st->lfe)
   {
      opus_val32 mask_avg=0;
      opus_val16 offset = shortBlocks?HALF16(SHL16(LM, DB_SHIFT)):0;
      for (c=0;c<C;c++)
      {
         opus_val16 followE, followMask;
         followE = followMask = -QCONST16(14.f, DB_SHIFT);
         for(i=0;i<st->end;i++)
         {
            
            followE = MAX16(followE-QCONST16(1.f, DB_SHIFT), bandLogE[nbEBands*c+i]-offset);
            followMask = MAX16(followMask-QCONST16(1.f, DB_SHIFT), st->energy_mask[nbEBands*c+i]);
            mask_avg += followE-followMask;
         }
      }
      surround_masking = DIV32_16(mask_avg,C*st->end) + QCONST16(.7f, DB_SHIFT);
      surround_masking = MIN16(MAX16(surround_masking, -QCONST16(2.f, DB_SHIFT)), QCONST16(.2f, DB_SHIFT));
      surround_masking -= HALF16(HALF16(surround_masking));
   }
   
   if (!st->lfe)
   {
      opus_val16 follow=-QCONST16(10.0f,DB_SHIFT);
      float frame_avg=0;
      opus_val16 offset = shortBlocks?HALF16(SHL16(LM, DB_SHIFT)):0;
      for(i=st->start;i<st->end;i++)
      {
         follow = MAX16(follow-QCONST16(1.f, DB_SHIFT), bandLogE[i]-offset);
         if (C==2)
            follow = MAX16(follow, bandLogE[i+nbEBands]-offset);
         frame_avg += follow;
      }
      frame_avg /= (st->end-st->start);
      temporal_vbr = SUB16(frame_avg,st->spec_avg);
      temporal_vbr = MIN16(QCONST16(3.f, DB_SHIFT), MAX16(-QCONST16(1.5f, DB_SHIFT), temporal_vbr));
      st->spec_avg += MULT16_16_Q15(QCONST16(.02f, 15), temporal_vbr);
   }
   



   if (!secondMdct)
   {
      for (i=0;i<C*nbEBands;i++)
         bandLogE2[i] = bandLogE[i];
   }

   

   if (LM>0 && ec_tell(enc)+3<=total_bits && !isTransient && st->complexity>=5 && !st->lfe)
   {
      if (patch_transient_decision(bandLogE, oldBandE, nbEBands, st->end, C))
      {
         isTransient = 1;
         shortBlocks = M;
         compute_mdcts(mode, shortBlocks, in, freq, C, CC, LM, st->upsample);
         compute_band_energies(mode, freq, bandE, effEnd, C, M);
         amp2Log2(mode, effEnd, st->end, bandE, bandLogE, C);
         
         for (i=0;i<C*nbEBands;i++)
            bandLogE2[i] += HALF16(SHL16(LM, DB_SHIFT));
         tf_estimate = QCONST16(.2f,14);
      }
   }

   if (LM>0 && ec_tell(enc)+3<=total_bits)
      ec_enc_bit_logp(enc, isTransient, 3);

   ALLOC(X, C*N, celt_norm);         

   
   normalise_bands(mode, freq, X, bandE, effEnd, C, M);

   ALLOC(tf_res, nbEBands, int);
   
   if (effectiveBytes>=15*C && st->start==0 && st->complexity>=2 && !st->lfe)
   {
      int lambda;
      if (effectiveBytes<40)
         lambda = 12;
      else if (effectiveBytes<60)
         lambda = 6;
      else if (effectiveBytes<100)
         lambda = 4;
      else
         lambda = 3;
      lambda*=2;
      tf_select = tf_analysis(mode, effEnd, isTransient, tf_res, lambda, X, N, LM, &tf_sum, tf_estimate, tf_chan);
      for (i=effEnd;i<st->end;i++)
         tf_res[i] = tf_res[effEnd-1];
   } else {
      tf_sum = 0;
      for (i=0;i<st->end;i++)
         tf_res[i] = isTransient;
      tf_select=0;
   }

   ALLOC(error, C*nbEBands, opus_val16);
   quant_coarse_energy(mode, st->start, st->end, effEnd, bandLogE,
         oldBandE, total_bits, error, enc,
         C, LM, nbAvailableBytes, st->force_intra,
         &st->delayedIntra, st->complexity >= 4, st->loss_rate, st->lfe);

   tf_encode(st->start, st->end, isTransient, tf_res, LM, tf_select, enc);

   if (ec_tell(enc)+4<=total_bits)
   {
      if (st->lfe)
      {
         st->tapset_decision = 0;
         st->spread_decision = SPREAD_NORMAL;
      } else if (shortBlocks || st->complexity < 3 || nbAvailableBytes < 10*C || st->start != 0)
      {
         if (st->complexity == 0)
            st->spread_decision = SPREAD_NONE;
         else
            st->spread_decision = SPREAD_NORMAL;
      } else {
         


         if (0&&st->analysis.valid)
         {
            static const opus_val16 spread_thresholds[3] = {-QCONST16(.6f, 15), -QCONST16(.2f, 15), -QCONST16(.07f, 15)};
            static const opus_val16 spread_histeresis[3] = {QCONST16(.15f, 15), QCONST16(.07f, 15), QCONST16(.02f, 15)};
            static const opus_val16 tapset_thresholds[2] = {QCONST16(.0f, 15), QCONST16(.15f, 15)};
            static const opus_val16 tapset_histeresis[2] = {QCONST16(.1f, 15), QCONST16(.05f, 15)};
            st->spread_decision = hysteresis_decision(-st->analysis.tonality, spread_thresholds, spread_histeresis, 3, st->spread_decision);
            st->tapset_decision = hysteresis_decision(st->analysis.tonality_slope, tapset_thresholds, tapset_histeresis, 2, st->tapset_decision);
         } else {
            st->spread_decision = spreading_decision(mode, X,
                  &st->tonal_average, st->spread_decision, &st->hf_average,
                  &st->tapset_decision, pf_on&&!shortBlocks, effEnd, C, M);
         }
         
         
      }
      ec_enc_icdf(enc, st->spread_decision, spread_icdf, 5);
   }

   ALLOC(offsets, nbEBands, int);

   maxDepth = dynalloc_analysis(bandLogE, bandLogE2, nbEBands, st->start, st->end, C, offsets,
         st->lsb_depth, mode->logN, isTransient, st->vbr, st->constrained_vbr,
         eBands, LM, effectiveBytes, &tot_boost, st->lfe);
   
   if (st->lfe)
      offsets[0] = IMIN(8, effectiveBytes/3);
   ALLOC(cap, nbEBands, int);
   init_caps(mode,cap,LM,C);

   dynalloc_logp = 6;
   total_bits<<=BITRES;
   total_boost = 0;
   tell = ec_tell_frac(enc);
   for (i=st->start;i<st->end;i++)
   {
      int width, quanta;
      int dynalloc_loop_logp;
      int boost;
      int j;
      width = C*(eBands[i+1]-eBands[i])<<LM;
      

      quanta = IMIN(width<<BITRES, IMAX(6<<BITRES, width));
      dynalloc_loop_logp = dynalloc_logp;
      boost = 0;
      for (j = 0; tell+(dynalloc_loop_logp<<BITRES) < total_bits-total_boost
            && boost < cap[i]; j++)
      {
         int flag;
         flag = j<offsets[i];
         ec_enc_bit_logp(enc, flag, dynalloc_loop_logp);
         tell = ec_tell_frac(enc);
         if (!flag)
            break;
         boost += quanta;
         total_boost += quanta;
         dynalloc_loop_logp = 1;
      }
      
      if (j)
         dynalloc_logp = IMAX(2, dynalloc_logp-1);
      offsets[i] = boost;
   }

   if (C==2)
   {
      int effectiveRate;

      static const opus_val16 intensity_thresholds[21]=
      
        { 16,21,23,25,27,29,31,33,35,38,42,46,50,54,58,63,68,75,84,102,130};
      static const opus_val16 intensity_histeresis[21]=
        {  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 4, 5, 6,  8, 12};

      
      if (LM!=0)
         dual_stereo = stereo_analysis(mode, X, LM, N);

      
      effectiveRate = (8*effectiveBytes - 80)>>LM;

      
      effectiveRate = 2*effectiveRate/5;

      st->intensity = hysteresis_decision((opus_val16)effectiveRate, intensity_thresholds, intensity_histeresis, 21, st->intensity);
      st->intensity = IMIN(st->end,IMAX(st->start, st->intensity));
   }

   alloc_trim = 5;
   if (tell+(6<<BITRES) <= total_bits - total_boost)
   {
      if (st->lfe)
         alloc_trim = 5;
      else
         alloc_trim = alloc_trim_analysis(mode, X, bandLogE,
            st->end, LM, C, N, &st->analysis, &st->stereo_saving, tf_estimate, st->intensity);
      ec_enc_icdf(enc, alloc_trim, trim_icdf, 7);
      tell = ec_tell_frac(enc);
   }

   
   if (vbr_rate>0)
   {
     opus_val16 alpha;
     opus_int32 delta;
     
     opus_int32 target, base_target;
     opus_int32 min_allowed;
     int lm_diff = mode->maxLM - LM;

     

     nbCompressedBytes = IMIN(nbCompressedBytes,1275>>(3-LM));
     base_target = vbr_rate - ((40*C+20)<<BITRES);

     if (st->constrained_vbr)
        base_target += (st->vbr_offset>>lm_diff);

     target = compute_vbr(mode, &st->analysis, base_target, LM, st->bitrate,
           st->lastCodedBands, C, st->intensity, st->constrained_vbr,
           st->stereo_saving, tot_boost, tf_estimate, pitch_change, maxDepth,
           st->variable_duration, st->lfe, st->energy_mask!=NULL, surround_masking,
           temporal_vbr);

     

     target=target+tell;
     



     min_allowed = ((tell+total_boost+(1<<(BITRES+3))-1)>>(BITRES+3)) + 2 - nbFilledBytes;

     nbAvailableBytes = (target+(1<<(BITRES+2)))>>(BITRES+3);
     nbAvailableBytes = IMAX(min_allowed,nbAvailableBytes);
     nbAvailableBytes = IMIN(nbCompressedBytes,nbAvailableBytes+nbFilledBytes) - nbFilledBytes;

     
     delta = target - vbr_rate;

     target=nbAvailableBytes<<(BITRES+3);

     




     if(silence)
     {
       nbAvailableBytes = 2;
       target = 2*8<<BITRES;
       delta = 0;
     }

     if (st->vbr_count < 970)
     {
        st->vbr_count++;
        alpha = celt_rcp(SHL32(EXTEND32(st->vbr_count+20),16));
     } else
        alpha = QCONST16(.001f,15);
     
     if (st->constrained_vbr)
        st->vbr_reservoir += target - vbr_rate;
     

     
     if (st->constrained_vbr)
     {
        st->vbr_drift += (opus_int32)MULT16_32_Q15(alpha,(delta*(1<<lm_diff))-st->vbr_offset-st->vbr_drift);
        st->vbr_offset = -st->vbr_drift;
     }
     

     if (st->constrained_vbr && st->vbr_reservoir < 0)
     {
        
        int adjust = (-st->vbr_reservoir)/(8<<BITRES);
        
        nbAvailableBytes += silence?0:adjust;
        st->vbr_reservoir = 0;
        
     }
     nbCompressedBytes = IMIN(nbCompressedBytes,nbAvailableBytes+nbFilledBytes);
     
     
     ec_enc_shrink(enc, nbCompressedBytes);
   }

   
   ALLOC(fine_quant, nbEBands, int);
   ALLOC(pulses, nbEBands, int);
   ALLOC(fine_priority, nbEBands, int);

   
   bits = (((opus_int32)nbCompressedBytes*8)<<BITRES) - ec_tell_frac(enc) - 1;
   anti_collapse_rsv = isTransient&&LM>=2&&bits>=((LM+2)<<BITRES) ? (1<<BITRES) : 0;
   bits -= anti_collapse_rsv;
   signalBandwidth = st->end-1;
#ifndef FIXED_POINT
   if (st->analysis.valid)
   {
      int min_bandwidth;
      if (st->bitrate < (opus_int32)32000*C)
         min_bandwidth = 13;
      else if (st->bitrate < (opus_int32)48000*C)
         min_bandwidth = 16;
      else if (st->bitrate < (opus_int32)60000*C)
         min_bandwidth = 18;
      else  if (st->bitrate < (opus_int32)80000*C)
         min_bandwidth = 19;
      else
         min_bandwidth = 20;
      signalBandwidth = IMAX(st->analysis.bandwidth, min_bandwidth);
   }
#endif
   if (st->lfe)
      signalBandwidth = 1;
   codedBands = compute_allocation(mode, st->start, st->end, offsets, cap,
         alloc_trim, &st->intensity, &dual_stereo, bits, &balance, pulses,
         fine_quant, fine_priority, C, LM, enc, 1, st->lastCodedBands, signalBandwidth);
   if (st->lastCodedBands)
      st->lastCodedBands = IMIN(st->lastCodedBands+1,IMAX(st->lastCodedBands-1,codedBands));
   else
      st->lastCodedBands = codedBands;

   quant_fine_energy(mode, st->start, st->end, oldBandE, error, fine_quant, enc, C);

   
   ALLOC(collapse_masks, C*nbEBands, unsigned char);
   quant_all_bands(1, mode, st->start, st->end, X, C==2 ? X+N : NULL, collapse_masks,
         bandE, pulses, shortBlocks, st->spread_decision, dual_stereo, st->intensity, tf_res,
         nbCompressedBytes*(8<<BITRES)-anti_collapse_rsv, balance, enc, LM, codedBands, &st->rng);

   if (anti_collapse_rsv > 0)
   {
      anti_collapse_on = st->consec_transient<2;
#ifdef FUZZING
      anti_collapse_on = rand()&0x1;
#endif
      ec_enc_bits(enc, anti_collapse_on, 1);
   }
   quant_energy_finalise(mode, st->start, st->end, oldBandE, error, fine_quant, fine_priority, nbCompressedBytes*8-ec_tell(enc), enc, C);

   if (silence)
   {
      for (i=0;i<C*nbEBands;i++)
         oldBandE[i] = -QCONST16(28.f,DB_SHIFT);
   }

#ifdef RESYNTH
   
   {
      celt_sig *out_mem[2];

      if (anti_collapse_on)
      {
         anti_collapse(mode, X, collapse_masks, LM, C, N,
               st->start, st->end, oldBandE, oldLogE, oldLogE2, pulses, st->rng);
      }

      if (silence)
      {
         for (i=0;i<C*N;i++)
            freq[i] = 0;
      } else {
         
         denormalise_bands(mode, X, freq, oldBandE, st->start, effEnd, C, M);
      }

      c=0; do {
         OPUS_MOVE(st->syn_mem[c], st->syn_mem[c]+N, 2*MAX_PERIOD-N+overlap/2);
      } while (++c<CC);

      if (CC==2&&C==1)
      {
         for (i=0;i<N;i++)
            freq[N+i] = freq[i];
      }

      c=0; do {
         out_mem[c] = st->syn_mem[c]+2*MAX_PERIOD-N;
      } while (++c<CC);

      compute_inv_mdcts(mode, shortBlocks, freq, out_mem, CC, LM);

      c=0; do {
         st->prefilter_period=IMAX(st->prefilter_period, COMBFILTER_MINPERIOD);
         st->prefilter_period_old=IMAX(st->prefilter_period_old, COMBFILTER_MINPERIOD);
         comb_filter(out_mem[c], out_mem[c], st->prefilter_period_old, st->prefilter_period, mode->shortMdctSize,
               st->prefilter_gain_old, st->prefilter_gain, st->prefilter_tapset_old, st->prefilter_tapset,
               mode->window, st->overlap);
         if (LM!=0)
            comb_filter(out_mem[c]+mode->shortMdctSize, out_mem[c]+mode->shortMdctSize, st->prefilter_period, pitch_index, N-mode->shortMdctSize,
                  st->prefilter_gain, gain1, st->prefilter_tapset, prefilter_tapset,
                  mode->window, overlap);
      } while (++c<CC);

      
      deemphasis(out_mem, (opus_val16*)pcm, N, CC, st->upsample, mode->preemph, st->preemph_memD, freq);
      st->prefilter_period_old = st->prefilter_period;
      st->prefilter_gain_old = st->prefilter_gain;
      st->prefilter_tapset_old = st->prefilter_tapset;
   }
#endif

   st->prefilter_period = pitch_index;
   st->prefilter_gain = gain1;
   st->prefilter_tapset = prefilter_tapset;
#ifdef RESYNTH
   if (LM!=0)
   {
      st->prefilter_period_old = st->prefilter_period;
      st->prefilter_gain_old = st->prefilter_gain;
      st->prefilter_tapset_old = st->prefilter_tapset;
   }
#endif

   if (CC==2&&C==1) {
      for (i=0;i<nbEBands;i++)
         oldBandE[nbEBands+i]=oldBandE[i];
   }

   if (!isTransient)
   {
      for (i=0;i<CC*nbEBands;i++)
         oldLogE2[i] = oldLogE[i];
      for (i=0;i<CC*nbEBands;i++)
         oldLogE[i] = oldBandE[i];
   } else {
      for (i=0;i<CC*nbEBands;i++)
         oldLogE[i] = MIN16(oldLogE[i], oldBandE[i]);
   }
   
   c=0; do
   {
      for (i=0;i<st->start;i++)
      {
         oldBandE[c*nbEBands+i]=0;
         oldLogE[c*nbEBands+i]=oldLogE2[c*nbEBands+i]=-QCONST16(28.f,DB_SHIFT);
      }
      for (i=st->end;i<nbEBands;i++)
      {
         oldBandE[c*nbEBands+i]=0;
         oldLogE[c*nbEBands+i]=oldLogE2[c*nbEBands+i]=-QCONST16(28.f,DB_SHIFT);
      }
   } while (++c<CC);

   if (isTransient || transient_got_disabled)
      st->consec_transient++;
   else
      st->consec_transient=0;
   st->rng = enc->rng;

   

   ec_enc_done(enc);

#ifdef CUSTOM_MODES
   if (st->signalling)
      nbCompressedBytes++;
#endif

   RESTORE_STACK;
   if (ec_get_error(enc))
      return OPUS_INTERNAL_ERROR;
   else
      return nbCompressedBytes;
}


#ifdef CUSTOM_MODES

#ifdef FIXED_POINT
int opus_custom_encode(CELTEncoder * OPUS_RESTRICT st, const opus_int16 * pcm, int frame_size, unsigned char *compressed, int nbCompressedBytes)
{
   return celt_encode_with_ec(st, pcm, frame_size, compressed, nbCompressedBytes, NULL);
}

#ifndef DISABLE_FLOAT_API
int opus_custom_encode_float(CELTEncoder * OPUS_RESTRICT st, const float * pcm, int frame_size, unsigned char *compressed, int nbCompressedBytes)
{
   int j, ret, C, N;
   VARDECL(opus_int16, in);
   ALLOC_STACK;

   if (pcm==NULL)
      return OPUS_BAD_ARG;

   C = st->channels;
   N = frame_size;
   ALLOC(in, C*N, opus_int16);

   for (j=0;j<C*N;j++)
     in[j] = FLOAT2INT16(pcm[j]);

   ret=celt_encode_with_ec(st,in,frame_size,compressed,nbCompressedBytes, NULL);
#ifdef RESYNTH
   for (j=0;j<C*N;j++)
      ((float*)pcm)[j]=in[j]*(1.f/32768.f);
#endif
   RESTORE_STACK;
   return ret;
}
#endif 
#else

int opus_custom_encode(CELTEncoder * OPUS_RESTRICT st, const opus_int16 * pcm, int frame_size, unsigned char *compressed, int nbCompressedBytes)
{
   int j, ret, C, N;
   VARDECL(celt_sig, in);
   ALLOC_STACK;

   if (pcm==NULL)
      return OPUS_BAD_ARG;

   C=st->channels;
   N=frame_size;
   ALLOC(in, C*N, celt_sig);
   for (j=0;j<C*N;j++) {
     in[j] = SCALEOUT(pcm[j]);
   }

   ret = celt_encode_with_ec(st,in,frame_size,compressed,nbCompressedBytes, NULL);
#ifdef RESYNTH
   for (j=0;j<C*N;j++)
      ((opus_int16*)pcm)[j] = FLOAT2INT16(in[j]);
#endif
   RESTORE_STACK;
   return ret;
}

int opus_custom_encode_float(CELTEncoder * OPUS_RESTRICT st, const float * pcm, int frame_size, unsigned char *compressed, int nbCompressedBytes)
{
   return celt_encode_with_ec(st, pcm, frame_size, compressed, nbCompressedBytes, NULL);
}

#endif

#endif 

int opus_custom_encoder_ctl(CELTEncoder * OPUS_RESTRICT st, int request, ...)
{
   va_list ap;

   va_start(ap, request);
   switch (request)
   {
      case OPUS_SET_COMPLEXITY_REQUEST:
      {
         int value = va_arg(ap, opus_int32);
         if (value<0 || value>10)
            goto bad_arg;
         st->complexity = value;
      }
      break;
      case CELT_SET_START_BAND_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         if (value<0 || value>=st->mode->nbEBands)
            goto bad_arg;
         st->start = value;
      }
      break;
      case CELT_SET_END_BAND_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         if (value<1 || value>st->mode->nbEBands)
            goto bad_arg;
         st->end = value;
      }
      break;
      case CELT_SET_PREDICTION_REQUEST:
      {
         int value = va_arg(ap, opus_int32);
         if (value<0 || value>2)
            goto bad_arg;
         st->disable_pf = value<=1;
         st->force_intra = value==0;
      }
      break;
      case OPUS_SET_PACKET_LOSS_PERC_REQUEST:
      {
         int value = va_arg(ap, opus_int32);
         if (value<0 || value>100)
            goto bad_arg;
         st->loss_rate = value;
      }
      break;
      case OPUS_SET_VBR_CONSTRAINT_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         st->constrained_vbr = value;
      }
      break;
      case OPUS_SET_VBR_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         st->vbr = value;
      }
      break;
      case OPUS_SET_BITRATE_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         if (value<=500 && value!=OPUS_BITRATE_MAX)
            goto bad_arg;
         value = IMIN(value, 260000*st->channels);
         st->bitrate = value;
      }
      break;
      case CELT_SET_CHANNELS_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         if (value<1 || value>2)
            goto bad_arg;
         st->stream_channels = value;
      }
      break;
      case OPUS_SET_LSB_DEPTH_REQUEST:
      {
          opus_int32 value = va_arg(ap, opus_int32);
          if (value<8 || value>24)
             goto bad_arg;
          st->lsb_depth=value;
      }
      break;
      case OPUS_GET_LSB_DEPTH_REQUEST:
      {
          opus_int32 *value = va_arg(ap, opus_int32*);
          *value=st->lsb_depth;
      }
      break;
      case OPUS_SET_EXPERT_FRAME_DURATION_REQUEST:
      {
          opus_int32 value = va_arg(ap, opus_int32);
          st->variable_duration = value;
      }
      break;
      case OPUS_RESET_STATE:
      {
         int i;
         opus_val16 *oldBandE, *oldLogE, *oldLogE2;
         oldBandE = (opus_val16*)(st->in_mem+st->channels*(st->overlap+COMBFILTER_MAXPERIOD));
         oldLogE = oldBandE + st->channels*st->mode->nbEBands;
         oldLogE2 = oldLogE + st->channels*st->mode->nbEBands;
         OPUS_CLEAR((char*)&st->ENCODER_RESET_START,
               opus_custom_encoder_get_size(st->mode, st->channels)-
               ((char*)&st->ENCODER_RESET_START - (char*)st));
         for (i=0;i<st->channels*st->mode->nbEBands;i++)
            oldLogE[i]=oldLogE2[i]=-QCONST16(28.f,DB_SHIFT);
         st->vbr_offset = 0;
         st->delayedIntra = 1;
         st->spread_decision = SPREAD_NORMAL;
         st->tonal_average = 256;
         st->hf_average = 0;
         st->tapset_decision = 0;
      }
      break;
#ifdef CUSTOM_MODES
      case CELT_SET_INPUT_CLIPPING_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         st->clip = value;
      }
      break;
#endif
      case CELT_SET_SIGNALLING_REQUEST:
      {
         opus_int32 value = va_arg(ap, opus_int32);
         st->signalling = value;
      }
      break;
      case CELT_SET_ANALYSIS_REQUEST:
      {
         AnalysisInfo *info = va_arg(ap, AnalysisInfo *);
         if (info)
            OPUS_COPY(&st->analysis, info, 1);
      }
      break;
      case CELT_GET_MODE_REQUEST:
      {
         const CELTMode ** value = va_arg(ap, const CELTMode**);
         if (value==0)
            goto bad_arg;
         *value=st->mode;
      }
      break;
      case OPUS_GET_FINAL_RANGE_REQUEST:
      {
         opus_uint32 * value = va_arg(ap, opus_uint32 *);
         if (value==0)
            goto bad_arg;
         *value=st->rng;
      }
      break;
      case OPUS_SET_LFE_REQUEST:
      {
          opus_int32 value = va_arg(ap, opus_int32);
          st->lfe = value;
      }
      break;
      case OPUS_SET_ENERGY_SAVE_REQUEST:
      {
          opus_val16 *value = va_arg(ap, opus_val16*);
          st->energy_save=value;
      }
      break;
      case OPUS_SET_ENERGY_MASK_REQUEST:
      {
          opus_val16 *value = va_arg(ap, opus_val16*);
          st->energy_mask = value;
      }
      break;
      default:
         goto bad_request;
   }
   va_end(ap);
   return OPUS_OK;
bad_arg:
   va_end(ap);
   return OPUS_BAD_ARG;
bad_request:
   va_end(ap);
   return OPUS_UNIMPLEMENTED;
}
