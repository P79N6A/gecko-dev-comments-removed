


















#include "arith_routines.h"



static const WebRtc_Word32 kHistEdgesQ15[51] = {
  -327680, -314573, -301466, -288359, -275252, -262144, -249037, -235930, -222823, -209716,
  -196608, -183501, -170394, -157287, -144180, -131072, -117965, -104858, -91751, -78644,
  -65536, -52429, -39322, -26215, -13108,  0,  13107,  26214,  39321,  52428,
  65536,  78643,  91750,  104857,  117964,  131072,  144179,  157286,  170393,  183500,
  196608,  209715,  222822,  235929,  249036,  262144,  275251,  288358,  301465,  314572,
  327680};


static const int kCdfSlopeQ0[51] = {  
  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,
  5,  5,  13,  23,  47,  87,  154,  315,  700,  1088,
  2471,  6064,  14221,  21463,  36634,  36924,  19750,  13270,  5806,  2312,
  1095,  660,  316,  145,  86,  41,  32,  5,  5,  5,
  5,  5,  5,  5,  5,  5,  5,  5,  5,  2, 0};


static const int kCdfQ16[51] = {  
  0,  2,  4,  6,  8,  10,  12,  14,  16,  18,
  20,  22,  24,  29,  38,  57,  92,  153,  279,  559,
  994,  1983,  4408,  10097,  18682,  33336,  48105,  56005,  61313,  63636,
  64560,  64998,  65262,  65389,  65447,  65481,  65497,  65510,  65512,  65514,
  65516,  65518,  65520,  65522,  65524,  65526,  65528,  65530,  65532,  65534,
  65535};




static __inline WebRtc_UWord32 piecewise(WebRtc_Word32 xinQ15) {

  WebRtc_Word32 ind, qtmp1, qtmp2, qtmp3;
  WebRtc_UWord32 tmpUW32;


  qtmp2 = xinQ15;

  if (qtmp2 < kHistEdgesQ15[0]) {
    qtmp2 = kHistEdgesQ15[0];
  }
  if (qtmp2 > kHistEdgesQ15[50]) {
    qtmp2 = kHistEdgesQ15[50];
  }

  qtmp1 = qtmp2 - kHistEdgesQ15[0];       
  ind = (qtmp1 * 5) >> 16;              
  
  qtmp1 = qtmp2 - kHistEdgesQ15[ind];     
  qtmp2 = kCdfSlopeQ0[ind] * qtmp1;      
  qtmp3 = qtmp2>>15;                    

  tmpUW32 = kCdfQ16[ind] + qtmp3;    
  return tmpUW32;
}



int WebRtcIsac_EncLogisticMulti2(
    Bitstr *streamdata,      
    WebRtc_Word16 *dataQ7,    
    const WebRtc_UWord16 *envQ8, 
    const int N,       
    const WebRtc_Word16 isSWB12kHz)
{
  WebRtc_UWord32 W_lower, W_upper;
  WebRtc_UWord32 W_upper_LSB, W_upper_MSB;
  WebRtc_UWord8 *stream_ptr;
  WebRtc_UWord8 *maxStreamPtr;
  WebRtc_UWord8 *stream_ptr_carry;
  WebRtc_UWord32 cdf_lo, cdf_hi;
  int k;

  
  stream_ptr = streamdata->stream + streamdata->stream_index;
  W_upper = streamdata->W_upper;

  maxStreamPtr = streamdata->stream + STREAM_SIZE_MAX_60 - 1;
  for (k = 0; k < N; k++)
  {
    
    cdf_lo = piecewise((*dataQ7 - 64) * *envQ8);
    cdf_hi = piecewise((*dataQ7 + 64) * *envQ8);

    
    while (cdf_lo+1 >= cdf_hi) {
      
      if (*dataQ7 > 0) {
        *dataQ7 -= 128;
        cdf_hi = cdf_lo;
        cdf_lo = piecewise((*dataQ7 - 64) * *envQ8);
      } else {
        *dataQ7 += 128;
        cdf_lo = cdf_hi;
        cdf_hi = piecewise((*dataQ7 + 64) * *envQ8);
      }
    }

    dataQ7++;
    
    
    envQ8 += (isSWB12kHz)? (k & 1):((k & 1) & (k >> 1));


    
    W_upper_LSB = W_upper & 0x0000FFFF;
    W_upper_MSB = W_upper >> 16;
    W_lower = W_upper_MSB * cdf_lo;
    W_lower += (W_upper_LSB * cdf_lo) >> 16;
    W_upper = W_upper_MSB * cdf_hi;
    W_upper += (W_upper_LSB * cdf_hi) >> 16;

    
    W_upper -= ++W_lower;

    
    streamdata->streamval += W_lower;

    
    if (streamdata->streamval < W_lower)
    {
      
      stream_ptr_carry = stream_ptr;
      while (!(++(*--stream_ptr_carry)));
    }

    
    while ( !(W_upper & 0xFF000000) )      
    {
      W_upper <<= 8;
      *stream_ptr++ = (WebRtc_UWord8) (streamdata->streamval >> 24);

      if(stream_ptr > maxStreamPtr)
      {
        return -ISAC_DISALLOWED_BITSTREAM_LENGTH;
      }
      streamdata->streamval <<= 8;
    }
  }

  
  streamdata->stream_index = (int)(stream_ptr - streamdata->stream);
  streamdata->W_upper = W_upper;

  return 0;
}



int WebRtcIsac_DecLogisticMulti2(
    WebRtc_Word16 *dataQ7,       
    Bitstr *streamdata,      
    const WebRtc_UWord16 *envQ8, 
    const WebRtc_Word16 *ditherQ7,
    const int N,         
    const WebRtc_Word16 isSWB12kHz)
{
  WebRtc_UWord32    W_lower, W_upper;
  WebRtc_UWord32    W_tmp;
  WebRtc_UWord32    W_upper_LSB, W_upper_MSB;
  WebRtc_UWord32    streamval;
  const WebRtc_UWord8 *stream_ptr;
  WebRtc_UWord32    cdf_tmp;
  WebRtc_Word16     candQ7;
  int             k;

  stream_ptr = streamdata->stream + streamdata->stream_index;
  W_upper = streamdata->W_upper;
  if (streamdata->stream_index == 0)   
  {
    
    streamval = *stream_ptr << 24;
    streamval |= *++stream_ptr << 16;
    streamval |= *++stream_ptr << 8;
    streamval |= *++stream_ptr;
  } else {
    streamval = streamdata->streamval;
  }


  for (k = 0; k < N; k++)
  {
    
    W_upper_LSB = W_upper & 0x0000FFFF;
    W_upper_MSB = W_upper >> 16;

    
    candQ7 = - *ditherQ7 + 64;
    cdf_tmp = piecewise(candQ7 * *envQ8);

    W_tmp = W_upper_MSB * cdf_tmp;
    W_tmp += (W_upper_LSB * cdf_tmp) >> 16;
    if (streamval > W_tmp)
    {
      W_lower = W_tmp;
      candQ7 += 128;
      cdf_tmp = piecewise(candQ7 * *envQ8);

      W_tmp = W_upper_MSB * cdf_tmp;
      W_tmp += (W_upper_LSB * cdf_tmp) >> 16;
      while (streamval > W_tmp)
      {
        W_lower = W_tmp;
        candQ7 += 128;
        cdf_tmp = piecewise(candQ7 * *envQ8);

        W_tmp = W_upper_MSB * cdf_tmp;
        W_tmp += (W_upper_LSB * cdf_tmp) >> 16;

        
        if (W_lower == W_tmp) return -1;
      }
      W_upper = W_tmp;

      
      *dataQ7 = candQ7 - 64;
    }
    else
    {
      W_upper = W_tmp;
      candQ7 -= 128;
      cdf_tmp = piecewise(candQ7 * *envQ8);

      W_tmp = W_upper_MSB * cdf_tmp;
      W_tmp += (W_upper_LSB * cdf_tmp) >> 16;
      while ( !(streamval > W_tmp) )
      {
        W_upper = W_tmp;
        candQ7 -= 128;
        cdf_tmp = piecewise(candQ7 * *envQ8);

        W_tmp = W_upper_MSB * cdf_tmp;
        W_tmp += (W_upper_LSB * cdf_tmp) >> 16;

        
        if (W_upper == W_tmp) return -1;
      }
      W_lower = W_tmp;

      
      *dataQ7 = candQ7 + 64;
    }
    ditherQ7++;
    dataQ7++;
    
    
    envQ8 += (isSWB12kHz)? (k & 1):((k & 1) & (k >> 1));

    
    W_upper -= ++W_lower;

    
    streamval -= W_lower;

    
    while ( !(W_upper & 0xFF000000) )    
    {
      
      streamval = (streamval << 8) | *++stream_ptr;
      W_upper <<= 8;
    }
  }

  streamdata->stream_index = (int)(stream_ptr - streamdata->stream);
  streamdata->W_upper = W_upper;
  streamdata->streamval = streamval;

  
  if ( W_upper > 0x01FFFFFF )
    return streamdata->stream_index - 2;
  else
    return streamdata->stream_index - 1;
}
