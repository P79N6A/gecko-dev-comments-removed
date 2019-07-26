



































#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include "modes.h"
#include "cwrs.h"
#include "arch.h"
#include "os_support.h"

#include "entcode.h"
#include "rate.h"

static const unsigned char LOG2_FRAC_TABLE[24]={
   0,
   8,13,
  16,19,21,23,
  24,26,27,28,29,30,31,32,
  32,33,34,34,35,36,36,37,37
};

#ifdef CUSTOM_MODES



static int fits_in32(int _n, int _k)
{
   static const opus_int16 maxN[15] = {
      32767, 32767, 32767, 1476, 283, 109,  60,  40,
       29,  24,  20,  18,  16,  14,  13};
   static const opus_int16 maxK[15] = {
      32767, 32767, 32767, 32767, 1172, 238,  95,  53,
       36,  27,  22,  18,  16,  15,  13};
   if (_n>=14)
   {
      if (_k>=14)
         return 0;
      else
         return _n <= maxN[_k];
   } else {
      return _k <= maxK[_n];
   }
}

void compute_pulse_cache(CELTMode *m, int LM)
{
   int C;
   int i;
   int j;
   int curr=0;
   int nbEntries=0;
   int entryN[100], entryK[100], entryI[100];
   const opus_int16 *eBands = m->eBands;
   PulseCache *cache = &m->cache;
   opus_int16 *cindex;
   unsigned char *bits;
   unsigned char *cap;

   cindex = opus_alloc(sizeof(cache->index[0])*m->nbEBands*(LM+2));
   cache->index = cindex;

   
   for (i=0;i<=LM+1;i++)
   {
      for (j=0;j<m->nbEBands;j++)
      {
         int k;
         int N = (eBands[j+1]-eBands[j])<<i>>1;
         cindex[i*m->nbEBands+j] = -1;
         
         for (k=0;k<=i;k++)
         {
            int n;
            for (n=0;n<m->nbEBands && (k!=i || n<j);n++)
            {
               if (N == (eBands[n+1]-eBands[n])<<k>>1)
               {
                  cindex[i*m->nbEBands+j] = cindex[k*m->nbEBands+n];
                  break;
               }
            }
         }
         if (cache->index[i*m->nbEBands+j] == -1 && N!=0)
         {
            int K;
            entryN[nbEntries] = N;
            K = 0;
            while (fits_in32(N,get_pulses(K+1)) && K<MAX_PSEUDO)
               K++;
            entryK[nbEntries] = K;
            cindex[i*m->nbEBands+j] = curr;
            entryI[nbEntries] = curr;

            curr += K+1;
            nbEntries++;
         }
      }
   }
   bits = opus_alloc(sizeof(unsigned char)*curr);
   cache->bits = bits;
   cache->size = curr;
   
   for (i=0;i<nbEntries;i++)
   {
      unsigned char *ptr = bits+entryI[i];
      opus_int16 tmp[MAX_PULSES+1];
      get_required_bits(tmp, entryN[i], get_pulses(entryK[i]), BITRES);
      for (j=1;j<=entryK[i];j++)
         ptr[j] = tmp[get_pulses(j)]-1;
      ptr[0] = entryK[i];
   }

   

   cache->caps = cap = opus_alloc(sizeof(cache->caps[0])*(LM+1)*2*m->nbEBands);
   for (i=0;i<=LM;i++)
   {
      for (C=1;C<=2;C++)
      {
         for (j=0;j<m->nbEBands;j++)
         {
            int N0;
            int max_bits;
            N0 = m->eBands[j+1]-m->eBands[j];
            
            if (N0<<i == 1)
               max_bits = C*(1+MAX_FINE_BITS)<<BITRES;
            else
            {
               const unsigned char *pcache;
               opus_int32           num;
               opus_int32           den;
               int                  LM0;
               int                  N;
               int                  offset;
               int                  ndof;
               int                  qb;
               int                  k;
               LM0 = 0;
               

               if (N0 > 2)
               {
                  N0>>=1;
                  LM0--;
               }
               
               else if (N0 <= 1)
               {
                  LM0=IMIN(i,1);
                  N0<<=LM0;
               }
               

               pcache = bits + cindex[(LM0+1)*m->nbEBands+j];
               max_bits = pcache[pcache[0]]+1;
               
               N = N0;
               for(k=0;k<i-LM0;k++){
                  max_bits <<= 1;
                  


                  offset = ((m->logN[j]+((LM0+k)<<BITRES))>>1)-QTHETA_OFFSET;
                  



                  num=459*(opus_int32)((2*N-1)*offset+max_bits);
                  den=((opus_int32)(2*N-1)<<9)-459;
                  qb = IMIN((num+(den>>1))/den, 57);
                  celt_assert(qb >= 0);
                  max_bits += qb;
                  N <<= 1;
               }
               
               if (C==2)
               {
                  max_bits <<= 1;
                  offset = ((m->logN[j]+(i<<BITRES))>>1)-(N==2?QTHETA_OFFSET_TWOPHASE:QTHETA_OFFSET);
                  ndof = 2*N-1-(N==2);
                  

                  num = (N==2?512:487)*(opus_int32)(max_bits+ndof*offset);
                  den = ((opus_int32)ndof<<9)-(N==2?512:487);
                  qb = IMIN((num+(den>>1))/den, (N==2?64:61));
                  celt_assert(qb >= 0);
                  max_bits += qb;
               }
               
               
               ndof = C*N + ((C==2 && N>2) ? 1 : 0);
               

               offset = ((m->logN[j] + (i<<BITRES))>>1)-FINE_OFFSET;
               
               if (N==2)
                  offset += 1<<BITRES>>2;
               

               num = max_bits+ndof*offset;
               den = (ndof-1)<<BITRES;
               qb = IMIN((num+(den>>1))/den, MAX_FINE_BITS);
               celt_assert(qb >= 0);
               max_bits += C*qb<<BITRES;
            }
            max_bits = (4*max_bits/(C*((m->eBands[j+1]-m->eBands[j])<<i)))-64;
            celt_assert(max_bits >= 0);
            celt_assert(max_bits < 256);
            *cap++ = (unsigned char)max_bits;
         }
      }
   }
}

#endif 

#define ALLOC_STEPS 6

static inline int interp_bits2pulses(const CELTMode *m, int start, int end, int skip_start,
      const int *bits1, const int *bits2, const int *thresh, const int *cap, opus_int32 total, opus_int32 *_balance,
      int skip_rsv, int *intensity, int intensity_rsv, int *dual_stereo, int dual_stereo_rsv, int *bits,
      int *ebits, int *fine_priority, int C, int LM, ec_ctx *ec, int encode, int prev)
{
   opus_int32 psum;
   int lo, hi;
   int i, j;
   int logM;
   int stereo;
   int codedBands=-1;
   int alloc_floor;
   opus_int32 left, percoeff;
   int done;
   int balance;
   SAVE_STACK;

   alloc_floor = C<<BITRES;
   stereo = C>1;

   logM = LM<<BITRES;
   lo = 0;
   hi = 1<<ALLOC_STEPS;
   for (i=0;i<ALLOC_STEPS;i++)
   {
      int mid = (lo+hi)>>1;
      psum = 0;
      done = 0;
      for (j=end;j-->start;)
      {
         int tmp = bits1[j] + (mid*(opus_int32)bits2[j]>>ALLOC_STEPS);
         if (tmp >= thresh[j] || done)
         {
            done = 1;
            
            psum += IMIN(tmp, cap[j]);
         } else {
            if (tmp >= alloc_floor)
               psum += alloc_floor;
         }
      }
      if (psum > total)
         hi = mid;
      else
         lo = mid;
   }
   psum = 0;
   
   done = 0;
   for (j=end;j-->start;)
   {
      int tmp = bits1[j] + (lo*bits2[j]>>ALLOC_STEPS);
      if (tmp < thresh[j] && !done)
      {
         if (tmp >= alloc_floor)
            tmp = alloc_floor;
         else
            tmp = 0;
      } else
         done = 1;
      
      tmp = IMIN(tmp, cap[j]);
      bits[j] = tmp;
      psum += tmp;
   }

   
   for (codedBands=end;;codedBands--)
   {
      int band_width;
      int band_bits;
      int rem;
      j = codedBands-1;
      





      if (j<=skip_start)
      {
         
         total += skip_rsv;
         break;
      }
      

      left = total-psum;
      percoeff = left/(m->eBands[codedBands]-m->eBands[start]);
      left -= (m->eBands[codedBands]-m->eBands[start])*percoeff;
      rem = IMAX(left-(m->eBands[j]-m->eBands[start]),0);
      band_width = m->eBands[codedBands]-m->eBands[j];
      band_bits = (int)(bits[j] + percoeff*band_width + rem);
      


      if (band_bits >= IMAX(thresh[j], alloc_floor+(1<<BITRES)))
      {
         if (encode)
         {
            


            

#ifdef FUZZING
            if ((rand()&0x1) == 0)
#else
            if (band_bits > ((j<prev?7:9)*band_width<<LM<<BITRES)>>4)
#endif
            {
               ec_enc_bit_logp(ec, 1, 1);
               break;
            }
            ec_enc_bit_logp(ec, 0, 1);
         } else if (ec_dec_bit_logp(ec, 1)) {
            break;
         }
         
         psum += 1<<BITRES;
         band_bits -= 1<<BITRES;
      }
      
      psum -= bits[j]+intensity_rsv;
      if (intensity_rsv > 0)
         intensity_rsv = LOG2_FRAC_TABLE[j-start];
      psum += intensity_rsv;
      if (band_bits >= alloc_floor)
      {
         
         psum += alloc_floor;
         bits[j] = alloc_floor;
      } else {
         
         bits[j] = 0;
      }
   }

   celt_assert(codedBands > start);
   
   if (intensity_rsv > 0)
   {
      if (encode)
      {
         *intensity = IMIN(*intensity, codedBands);
         ec_enc_uint(ec, *intensity-start, codedBands+1-start);
      }
      else
         *intensity = start+ec_dec_uint(ec, codedBands+1-start);
   }
   else
      *intensity = 0;
   if (*intensity <= start)
   {
      total += dual_stereo_rsv;
      dual_stereo_rsv = 0;
   }
   if (dual_stereo_rsv > 0)
   {
      if (encode)
         ec_enc_bit_logp(ec, *dual_stereo, 1);
      else
         *dual_stereo = ec_dec_bit_logp(ec, 1);
   }
   else
      *dual_stereo = 0;

   
   left = total-psum;
   percoeff = left/(m->eBands[codedBands]-m->eBands[start]);
   left -= (m->eBands[codedBands]-m->eBands[start])*percoeff;
   for (j=start;j<codedBands;j++)
      bits[j] += ((int)percoeff*(m->eBands[j+1]-m->eBands[j]));
   for (j=start;j<codedBands;j++)
   {
      int tmp = (int)IMIN(left, m->eBands[j+1]-m->eBands[j]);
      bits[j] += tmp;
      left -= tmp;
   }
   

   balance = 0;
   for (j=start;j<codedBands;j++)
   {
      int N0, N, den;
      int offset;
      int NClogN;
      int excess;

      celt_assert(bits[j] >= 0);
      N0 = m->eBands[j+1]-m->eBands[j];
      N=N0<<LM;
      bits[j] += balance;

      if (N>1)
      {
         excess = IMAX(bits[j]-cap[j],0);
         bits[j] -= excess;

         
         den=(C*N+ ((C==2 && N>2 && !*dual_stereo && j<*intensity) ? 1 : 0));

         NClogN = den*(m->logN[j] + logM);

         

         offset = (NClogN>>1)-den*FINE_OFFSET;

         
         if (N==2)
            offset += den<<BITRES>>2;

         

         if (bits[j] + offset < den*2<<BITRES)
            offset += NClogN>>2;
         else if (bits[j] + offset < den*3<<BITRES)
            offset += NClogN>>3;

         
         ebits[j] = IMAX(0, (bits[j] + offset + (den<<(BITRES-1))) / (den<<BITRES));

         
         if (C*ebits[j] > (bits[j]>>BITRES))
            ebits[j] = bits[j] >> stereo >> BITRES;

         
         ebits[j] = IMIN(ebits[j], MAX_FINE_BITS);

         

         fine_priority[j] = ebits[j]*(den<<BITRES) >= bits[j]+offset;

         
         bits[j] -= C*ebits[j]<<BITRES;

      } else {
         
         excess = IMAX(0,bits[j]-(C<<BITRES));
         bits[j] -= excess;
         ebits[j] = 0;
         fine_priority[j] = 1;
      }

      


      if(excess > 0)
      {
         int extra_fine;
         int extra_bits;
         extra_fine = IMIN(excess>>(stereo+BITRES),MAX_FINE_BITS-ebits[j]);
         ebits[j] += extra_fine;
         extra_bits = extra_fine*C<<BITRES;
         fine_priority[j] = extra_bits >= excess-balance;
         excess -= extra_bits;
      }
      balance = excess;

      celt_assert(bits[j] >= 0);
      celt_assert(ebits[j] >= 0);
   }
   

   *_balance = balance;

   
   for (;j<end;j++)
   {
      ebits[j] = bits[j] >> stereo >> BITRES;
      celt_assert(C*ebits[j]<<BITRES == bits[j]);
      bits[j] = 0;
      fine_priority[j] = ebits[j]<1;
   }
   RESTORE_STACK;
   return codedBands;
}

int compute_allocation(const CELTMode *m, int start, int end, const int *offsets, const int *cap, int alloc_trim, int *intensity, int *dual_stereo,
      opus_int32 total, opus_int32 *balance, int *pulses, int *ebits, int *fine_priority, int C, int LM, ec_ctx *ec, int encode, int prev)
{
   int lo, hi, len, j;
   int codedBands;
   int skip_start;
   int skip_rsv;
   int intensity_rsv;
   int dual_stereo_rsv;
   VARDECL(int, bits1);
   VARDECL(int, bits2);
   VARDECL(int, thresh);
   VARDECL(int, trim_offset);
   SAVE_STACK;

   total = IMAX(total, 0);
   len = m->nbEBands;
   skip_start = start;
   
   skip_rsv = total >= 1<<BITRES ? 1<<BITRES : 0;
   total -= skip_rsv;
   
   intensity_rsv = dual_stereo_rsv = 0;
   if (C==2)
   {
      intensity_rsv = LOG2_FRAC_TABLE[end-start];
      if (intensity_rsv>total)
         intensity_rsv = 0;
      else
      {
         total -= intensity_rsv;
         dual_stereo_rsv = total>=1<<BITRES ? 1<<BITRES : 0;
         total -= dual_stereo_rsv;
      }
   }
   ALLOC(bits1, len, int);
   ALLOC(bits2, len, int);
   ALLOC(thresh, len, int);
   ALLOC(trim_offset, len, int);

   for (j=start;j<end;j++)
   {
      
      thresh[j] = IMAX((C)<<BITRES, (3*(m->eBands[j+1]-m->eBands[j])<<LM<<BITRES)>>4);
      
      trim_offset[j] = C*(m->eBands[j+1]-m->eBands[j])*(alloc_trim-5-LM)*(end-j-1)
            *(1<<(LM+BITRES))>>6;
      

      if ((m->eBands[j+1]-m->eBands[j])<<LM==1)
         trim_offset[j] -= C<<BITRES;
   }
   lo = 1;
   hi = m->nbAllocVectors - 1;
   do
   {
      int done = 0;
      int psum = 0;
      int mid = (lo+hi) >> 1;
      for (j=end;j-->start;)
      {
         int bitsj;
         int N = m->eBands[j+1]-m->eBands[j];
         bitsj = C*N*m->allocVectors[mid*len+j]<<LM>>2;
         if (bitsj > 0)
            bitsj = IMAX(0, bitsj + trim_offset[j]);
         bitsj += offsets[j];
         if (bitsj >= thresh[j] || done)
         {
            done = 1;
            
            psum += IMIN(bitsj, cap[j]);
         } else {
            if (bitsj >= C<<BITRES)
               psum += C<<BITRES;
         }
      }
      if (psum > total)
         hi = mid - 1;
      else
         lo = mid + 1;
      
   }
   while (lo <= hi);
   hi = lo--;
   
   for (j=start;j<end;j++)
   {
      int bits1j, bits2j;
      int N = m->eBands[j+1]-m->eBands[j];
      bits1j = C*N*m->allocVectors[lo*len+j]<<LM>>2;
      bits2j = hi>=m->nbAllocVectors ?
            cap[j] : C*N*m->allocVectors[hi*len+j]<<LM>>2;
      if (bits1j > 0)
         bits1j = IMAX(0, bits1j + trim_offset[j]);
      if (bits2j > 0)
         bits2j = IMAX(0, bits2j + trim_offset[j]);
      if (lo > 0)
         bits1j += offsets[j];
      bits2j += offsets[j];
      if (offsets[j]>0)
         skip_start = j;
      bits2j = IMAX(0,bits2j-bits1j);
      bits1[j] = bits1j;
      bits2[j] = bits2j;
   }
   codedBands = interp_bits2pulses(m, start, end, skip_start, bits1, bits2, thresh, cap,
         total, balance, skip_rsv, intensity, intensity_rsv, dual_stereo, dual_stereo_rsv,
         pulses, ebits, fine_priority, C, LM, ec, encode, prev);
   RESTORE_STACK;
   return codedBands;
}

