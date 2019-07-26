



































#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "laplace.h"
#include "mathops.h"


#define LAPLACE_LOG_MINP (0)
#define LAPLACE_MINP (1<<LAPLACE_LOG_MINP)


#define LAPLACE_NMIN (16)

static unsigned ec_laplace_get_freq1(unsigned fs0, int decay)
{
   unsigned ft;
   ft = 32768 - LAPLACE_MINP*(2*LAPLACE_NMIN) - fs0;
   return ft*(16384-decay)>>15;
}

void ec_laplace_encode(ec_enc *enc, int *value, unsigned fs, int decay)
{
   unsigned fl;
   int val = *value;
   fl = 0;
   if (val)
   {
      int s;
      int i;
      s = -(val<0);
      val = (val+s)^s;
      fl = fs;
      fs = ec_laplace_get_freq1(fs, decay);
      
      for (i=1; fs > 0 && i < val; i++)
      {
         fs *= 2;
         fl += fs+2*LAPLACE_MINP;
         fs = (fs*(opus_int32)decay)>>15;
      }
      
      if (!fs)
      {
         int di;
         int ndi_max;
         ndi_max = (32768-fl+LAPLACE_MINP-1)>>LAPLACE_LOG_MINP;
         ndi_max = (ndi_max-s)>>1;
         di = IMIN(val - i, ndi_max - 1);
         fl += (2*di+1+s)*LAPLACE_MINP;
         fs = IMIN(LAPLACE_MINP, 32768-fl);
         *value = (i+di+s)^s;
      }
      else
      {
         fs += LAPLACE_MINP;
         fl += fs&~s;
      }
      celt_assert(fl+fs<=32768);
      celt_assert(fs>0);
   }
   ec_encode_bin(enc, fl, fl+fs, 15);
}

int ec_laplace_decode(ec_dec *dec, unsigned fs, int decay)
{
   int val=0;
   unsigned fl;
   unsigned fm;
   fm = ec_decode_bin(dec, 15);
   fl = 0;
   if (fm >= fs)
   {
      val++;
      fl = fs;
      fs = ec_laplace_get_freq1(fs, decay)+LAPLACE_MINP;
      
      while(fs > LAPLACE_MINP && fm >= fl+2*fs)
      {
         fs *= 2;
         fl += fs;
         fs = ((fs-2*LAPLACE_MINP)*(opus_int32)decay)>>15;
         fs += LAPLACE_MINP;
         val++;
      }
      
      if (fs <= LAPLACE_MINP)
      {
         int di;
         di = (fm-fl)>>(LAPLACE_LOG_MINP+1);
         val += di;
         fl += 2*di*LAPLACE_MINP;
      }
      if (fm < fl+fs)
         val = -val;
      else
         fl += fs;
   }
   celt_assert(fl<32768);
   celt_assert(fs>0);
   celt_assert(fl<=fm);
   celt_assert(fm<IMIN(fl+fs,32768));
   ec_dec_update(dec, fl, IMIN(fl+fs,32768), 32768);
   return val;
}
