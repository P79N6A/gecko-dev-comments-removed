
















#include <string.h>

#include "settings.h"
#include "entropy_coding.h"
#include "pitch_estimator.h"
#include "bandwidth_estimator.h"
#include "structs.h"
#include "codec.h"


#define NO_OF_PRIMES 8
#define NOISE_FILTER_LEN 30






static WebRtc_Word16 plc_filterma_Fast(
    WebRtc_Word16 *In,  

    WebRtc_Word16 *Out,  
    WebRtc_Word16 *B,   
    WebRtc_Word16 Blen,  
    WebRtc_Word16 len,   
    WebRtc_Word16 reduceDecay,
    WebRtc_Word16 decay,
    WebRtc_Word16 rshift )
{
  int i, j;
  WebRtc_Word32 o;
  WebRtc_Word32 lim;

  lim = WEBRTC_SPL_LSHIFT_W32( (WebRtc_Word32)1, 15 + rshift )-1;

  for (i = 0; i < len; i++)
  {
    G_CONST WebRtc_Word16 *b_ptr = &B[0];
    G_CONST WebRtc_Word16 *x_ptr = &In[i];

    o = (WebRtc_Word32)0;

    for (j = 0;j < Blen; j++)
    {
      o = WEBRTC_SPL_ADD_SAT_W32( o, WEBRTC_SPL_MUL_16_16( *b_ptr, *x_ptr) );
      b_ptr++;
      x_ptr--;
    }

    
    o = WEBRTC_SPL_ADD_SAT_W32( o, WEBRTC_SPL_LSHIFT_W32( 1, (rshift-1) ) );

    
    o = WEBRTC_SPL_SAT((WebRtc_Word32)lim, o, (WebRtc_Word32)-lim);

    
    o = WEBRTC_SPL_RSHIFT_W32( o, rshift );

    
    *Out++ = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT( (WebRtc_Word16)o, decay, 15); 

    
    decay -= reduceDecay;
    if( decay < 0 )
      decay = 0;
  }
  return( decay );
}








static __inline WebRtc_Word32 log2_Q8_T( WebRtc_UWord32 x ) {

  WebRtc_Word32 zeros, lg2;
  WebRtc_Word16 frac;

  zeros=WebRtcSpl_NormU32(x);
  frac=(WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(((WebRtc_UWord32)WEBRTC_SPL_LSHIFT_W32(x, zeros)&0x7FFFFFFF), 23);
  

  lg2= (WEBRTC_SPL_LSHIFT_W16((31-zeros), 8)+frac);
  return lg2;

}

static __inline WebRtc_Word16  exp2_Q10_T(WebRtc_Word16 x) { 

  WebRtc_Word16 tmp16_1, tmp16_2;

  tmp16_2=(WebRtc_Word16)(0x0400|(x&0x03FF));
  tmp16_1=-(WebRtc_Word16)WEBRTC_SPL_RSHIFT_W16(x,10);
  if(tmp16_1>0)
    return (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W16(tmp16_2, tmp16_1);
  else
    return (WebRtc_Word16) WEBRTC_SPL_LSHIFT_W16(tmp16_2, -tmp16_1);

}









static void MemshipValQ15( WebRtc_Word16 in, WebRtc_Word16 *A, WebRtc_Word16 *B )
{
  WebRtc_Word16 x;

  in -= 700;    

  if( in <= 2150 )
  {
    if( in > 0 )
    {
      


      


      
      x = (WebRtc_Word16)( WEBRTC_SPL_MUL_16_16( in, 15 ) +
                           WEBRTC_SPL_MUL_16_16_RSFT( in, 983, 12) );
      

      *B = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32( WEBRTC_SPL_MUL_16_16( x, x ) + 0x00010000, 17 );
      *A = WEBRTC_SPL_WORD16_MAX - *B;
    }
    else
    {
      *B = 0;
      *A = WEBRTC_SPL_WORD16_MAX;
    }
  }
  else
  {
    if( in < 4300 )
    {
      
      in = 4300 - in;
      x = (WebRtc_Word16)( WEBRTC_SPL_MUL_16_16( in, 15 ) +
                           WEBRTC_SPL_MUL_16_16_RSFT( in, 983, 12) );
      

      *A = (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32( WEBRTC_SPL_MUL_16_16( x, x ) + 0x00010000, 17 );
      *B = WEBRTC_SPL_WORD16_MAX - *A;

    }
    else
    {
      *A = 0;
      *B = WEBRTC_SPL_WORD16_MAX;
    }
  }
}




static void LinearResampler( WebRtc_Word16 *in, WebRtc_Word16 *out, WebRtc_Word16 lenIn, WebRtc_Word16 lenOut )
{
  WebRtc_Word32 n;
  WebRtc_Word16 resOut, i, j, relativePos, diff; 
  WebRtc_UWord16 udiff;

  if( lenIn == lenOut )
  {
    WEBRTC_SPL_MEMCPY_W16( out, in, lenIn );
    return;
  }

  n = WEBRTC_SPL_MUL_16_16( (WebRtc_Word16)(lenIn-1), RESAMP_RES );
  resOut = WebRtcSpl_DivW32W16ResW16( n, (WebRtc_Word16)(lenOut-1) );

  out[0] = in[0];
  for( i = 1, j = 0, relativePos = 0; i < lenOut; i++ )
  {

    relativePos += resOut;
    while( relativePos > RESAMP_RES )
    {
      j++;
      relativePos -= RESAMP_RES;
    }


    



    if( (in[ j ] > 0) && (in[j + 1] < 0) )
    {
      udiff = (WebRtc_UWord16)(in[ j ] - in[j + 1]);
      out[ i ] = in[ j ] - (WebRtc_UWord16)( ((WebRtc_Word32)( udiff * relativePos )) >> RESAMP_RES_BIT);
    }
    else
    {
      if( (in[j] < 0) && (in[j+1] > 0) )
      {
        udiff = (WebRtc_UWord16)( in[j + 1] - in[ j ] );
        out[ i ] = in[ j ] + (WebRtc_UWord16)( ((WebRtc_Word32)( udiff * relativePos )) >> RESAMP_RES_BIT);
      }
      else
      {
        diff = in[ j + 1 ] - in[ j ];
        out[ i ] = in[ j ] + (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT( diff, relativePos, RESAMP_RES_BIT );
      }
    }
  }
}





WebRtc_Word16 WebRtcIsacfix_DecodePlcImpl(WebRtc_Word16 *signal_out16,
                                          ISACFIX_DecInst_t *ISACdec_obj,
                                          WebRtc_Word16 *current_framesamples )
{
  int subframecnt;
  WebRtc_Word16 len = 0;

  WebRtc_Word16* Vector_Word16_1;
  WebRtc_Word16  Vector_Word16_Extended_1[FRAMESAMPLES_HALF + NOISE_FILTER_LEN];
  WebRtc_Word16* Vector_Word16_2;
  WebRtc_Word16  Vector_Word16_Extended_2[FRAMESAMPLES_HALF + NOISE_FILTER_LEN];

  WebRtc_Word32 Vector_Word32_1[FRAMESAMPLES_HALF];
  WebRtc_Word32 Vector_Word32_2[FRAMESAMPLES_HALF];

  WebRtc_Word16 lofilt_coefQ15[ORDERLO*SUBFRAMES]; 
  WebRtc_Word16 hifilt_coefQ15[ORDERHI*SUBFRAMES]; 

  WebRtc_Word16 pitchLags_Q7[PITCH_SUBFRAMES];
  WebRtc_Word16 pitchGains_Q12[PITCH_SUBFRAMES];

  WebRtc_Word16 tmp_1, tmp_2;
  WebRtc_Word32 tmp32a, tmp32b;
  WebRtc_Word16 gainQ13;

  WebRtc_Word16 myDecayRate;

  
  WebRtc_Word16 lag0, i, k, noiseIndex;
  WebRtc_Word16 stretchPitchLP[PITCH_MAX_LAG + 10], stretchPitchLP1[PITCH_MAX_LAG + 10];

  WebRtc_Word32 gain_lo_hiQ17[2*SUBFRAMES];

  WebRtc_Word16 nLP, pLP, wNoisyLP, wPriodicLP, tmp16, minIdx;
  WebRtc_Word32 nHP, pHP, wNoisyHP, wPriodicHP, corr, minCorr, maxCoeff;
  WebRtc_Word16 noise1, rshift;


  WebRtc_Word16 ltpGain, pitchGain, myVoiceIndicator, myAbs, maxAbs;
  WebRtc_Word32 varIn, varOut, logVarIn, logVarOut, Q, logMaxAbs;
  int rightShiftIn, rightShiftOut;


  


  myDecayRate = (DECAY_RATE);
  Vector_Word16_1 = &Vector_Word16_Extended_1[NOISE_FILTER_LEN];
  Vector_Word16_2 = &Vector_Word16_Extended_2[NOISE_FILTER_LEN];


  
  for( subframecnt = 0; subframecnt < SUBFRAMES; subframecnt++ )
  {
    
    WEBRTC_SPL_MEMCPY_W16(&lofilt_coefQ15[ subframecnt * ORDERLO ],
                          (ISACdec_obj->plcstr_obj).lofilt_coefQ15, ORDERLO);
    gain_lo_hiQ17[2*subframecnt] = (ISACdec_obj->plcstr_obj).gain_lo_hiQ17[0];

    
    WEBRTC_SPL_MEMCPY_W16(&hifilt_coefQ15[ subframecnt * ORDERHI ],
                          (ISACdec_obj->plcstr_obj).hifilt_coefQ15, ORDERHI);
    gain_lo_hiQ17[2*subframecnt + 1] = (ISACdec_obj->plcstr_obj).gain_lo_hiQ17[1];
  }




  lag0 = WEBRTC_SPL_RSHIFT_W16(
      (ISACdec_obj->plcstr_obj).lastPitchLag_Q7 + 64, 7 ) + 1;


  if( (ISACdec_obj->plcstr_obj).used != PLC_WAS_USED )
  {
    (ISACdec_obj->plcstr_obj).pitchCycles = 0;

    (ISACdec_obj->plcstr_obj).lastPitchLP =
        &((ISACdec_obj->plcstr_obj).prevPitchInvIn[FRAMESAMPLES_HALF - lag0]);
    minCorr = WEBRTC_SPL_WORD32_MAX;

    if ( (FRAMESAMPLES_HALF - 2*lag0 - 10) > 0 )
    {
      minIdx = 11;
      for( i = 0; i < 21; i++ )
      {
        corr = 0;
        for( k = 0; k < lag0; k++ )
        {
          corr = WEBRTC_SPL_ADD_SAT_W32( corr, WEBRTC_SPL_ABS_W32(
              WEBRTC_SPL_SUB_SAT_W16(
                  (ISACdec_obj->plcstr_obj).lastPitchLP[k],
                  (ISACdec_obj->plcstr_obj).prevPitchInvIn[
                      FRAMESAMPLES_HALF - 2*lag0 - 10 + i + k ] ) ) );
        }
        if( corr < minCorr )
        {
          minCorr = corr;
          minIdx = i;
        }
      }
      (ISACdec_obj->plcstr_obj).prevPitchLP =
          &( (ISACdec_obj->plcstr_obj).prevPitchInvIn[
              FRAMESAMPLES_HALF - lag0*2 - 10 + minIdx] );
    }
    else
    {
      (ISACdec_obj->plcstr_obj).prevPitchLP =
          (ISACdec_obj->plcstr_obj).lastPitchLP;
    }
    pitchGain = (ISACdec_obj->plcstr_obj).lastPitchGain_Q12;

    WebRtcSpl_AutoCorrelation(
        &(ISACdec_obj->plcstr_obj).prevPitchInvIn[FRAMESAMPLES_HALF - lag0],
        lag0, 0, &varIn, &rightShiftIn);
    WebRtcSpl_AutoCorrelation(
        &(ISACdec_obj->plcstr_obj).prevPitchInvOut[PITCH_MAX_LAG + 10 - lag0],
        lag0, 0, &varOut, &rightShiftOut);

    maxAbs = 0;
    for( i = 0; i< lag0; i++)
    {
      myAbs = WEBRTC_SPL_ABS_W16(
          (ISACdec_obj->plcstr_obj).prevPitchInvOut[
              PITCH_MAX_LAG + 10 - lag0 + i] );
      maxAbs = (myAbs > maxAbs)? myAbs:maxAbs;
    }
    logVarIn = log2_Q8_T( (WebRtc_UWord32)( varIn ) ) +
        (WebRtc_Word32)(rightShiftIn << 8);
    logVarOut = log2_Q8_T( (WebRtc_UWord32)( varOut ) ) +
        (WebRtc_Word32)(rightShiftOut << 8);
    logMaxAbs = log2_Q8_T( (WebRtc_UWord32)( maxAbs ) );

    ltpGain = (WebRtc_Word16)(logVarOut - logVarIn);
    Q = 2 * logMaxAbs - ( logVarOut - 1512 );

    








    logVarIn -= log2_Q8_T( (WebRtc_UWord32)( lag0 ) );
    tmp16 = (WebRtc_Word16)((logVarIn<<1) - (4<<10) );
    rightShiftIn = 0;
    if( tmp16 > 4096 )
    {
      tmp16 -= 4096;
      tmp16 = exp2_Q10_T( tmp16 );
      tmp16 >>= 6;
    }
    else
      tmp16 = exp2_Q10_T( tmp16 )>>10;

    (ISACdec_obj->plcstr_obj).std = tmp16 - 4;

    if( (ltpGain < 110) || (ltpGain > 230) )
    {
      if( ltpGain < 100 && (pitchGain < 1800) )
      {
        (ISACdec_obj->plcstr_obj).A = WEBRTC_SPL_WORD16_MAX;
      }
      else
      {
        (ISACdec_obj->plcstr_obj).A = ((ltpGain < 110) && (Q < 800)
                                       )? WEBRTC_SPL_WORD16_MAX:0;
      }
      (ISACdec_obj->plcstr_obj).B = WEBRTC_SPL_WORD16_MAX -
          (ISACdec_obj->plcstr_obj).A;
    }
    else
    {
      if( (pitchGain < 450) || (pitchGain > 1600) )
      {
        (ISACdec_obj->plcstr_obj).A = ((pitchGain < 450)
                                       )? WEBRTC_SPL_WORD16_MAX:0;
        (ISACdec_obj->plcstr_obj).B = WEBRTC_SPL_WORD16_MAX -
            (ISACdec_obj->plcstr_obj).A;
      }
      else
      {
        myVoiceIndicator = ltpGain * 2 + pitchGain;
        MemshipValQ15( myVoiceIndicator,
                       &(ISACdec_obj->plcstr_obj).A, &(ISACdec_obj->plcstr_obj).B );
      }
    }



    myVoiceIndicator = ltpGain * 16 + pitchGain * 2 + (pitchGain >> 8);
    MemshipValQ15( myVoiceIndicator,
                   &(ISACdec_obj->plcstr_obj).A, &(ISACdec_obj->plcstr_obj).B );



    (ISACdec_obj->plcstr_obj).stretchLag = lag0;
    (ISACdec_obj->plcstr_obj).pitchIndex = 0;

  }
  else
  {
    myDecayRate = (DECAY_RATE<<2);
  }

  if( (ISACdec_obj->plcstr_obj).B < 1000 )
  {
    myDecayRate += (DECAY_RATE<<3);
  }

  

  LinearResampler( (ISACdec_obj->plcstr_obj).lastPitchLP,
                   stretchPitchLP, lag0, (ISACdec_obj->plcstr_obj).stretchLag );
  

  pitchLags_Q7[0] = pitchLags_Q7[1] = pitchLags_Q7[2] = pitchLags_Q7[3] =
      ((ISACdec_obj->plcstr_obj).stretchLag<<7);
  pitchGains_Q12[3] = ( (ISACdec_obj->plcstr_obj).lastPitchGain_Q12);
  pitchGains_Q12[2] = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(
      pitchGains_Q12[3], 1010, 10 );
  pitchGains_Q12[1] = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(
      pitchGains_Q12[2], 1010, 10 );
  pitchGains_Q12[0] = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(
      pitchGains_Q12[1], 1010, 10 );


  
  if( (ISACdec_obj->plcstr_obj).B == 0 )
  {
    for( i = 0; i < FRAMESAMPLES_HALF; i++ )
    {
      
      (ISACdec_obj->plcstr_obj).seed = WEBRTC_SPL_RAND(
          (ISACdec_obj->plcstr_obj).seed );
      Vector_Word16_1[i] = WEBRTC_SPL_RSHIFT_W16(
          (ISACdec_obj->plcstr_obj).seed, 10 ) - 16;

      
      (ISACdec_obj->plcstr_obj).seed = WEBRTC_SPL_RAND(
          (ISACdec_obj->plcstr_obj).seed );
      Vector_Word16_2[i] = WEBRTC_SPL_RSHIFT_W16(
          (ISACdec_obj->plcstr_obj).seed, 10 ) - 16;

    }
    for( i = 1; i < NOISE_FILTER_LEN; i++ )
    {
      (ISACdec_obj->plcstr_obj).seed = WEBRTC_SPL_RAND(
          (ISACdec_obj->plcstr_obj).seed );
      Vector_Word16_Extended_1[ i ] = WEBRTC_SPL_RSHIFT_W16(
          (ISACdec_obj->plcstr_obj).seed, 10 ) - 16;

      (ISACdec_obj->plcstr_obj).seed = WEBRTC_SPL_RAND(
          (ISACdec_obj->plcstr_obj).seed );
      Vector_Word16_Extended_2[ i ] = WEBRTC_SPL_RSHIFT_W16(
          (ISACdec_obj->plcstr_obj).seed, 10 ) - 16;
    }
    plc_filterma_Fast(Vector_Word16_1, Vector_Word16_Extended_1,
                      &(ISACdec_obj->plcstr_obj).prevPitchInvIn[FRAMESAMPLES_HALF -
                                                                NOISE_FILTER_LEN], (WebRtc_Word16) NOISE_FILTER_LEN,
                      (WebRtc_Word16) FRAMESAMPLES_HALF, (WebRtc_Word16)(5),
                      (ISACdec_obj->plcstr_obj).decayCoeffNoise, (WebRtc_Word16)(6));

    maxCoeff = WebRtcSpl_MaxAbsValueW32(
        &(ISACdec_obj->plcstr_obj).prevHP[
            PITCH_MAX_LAG + 10 - NOISE_FILTER_LEN], NOISE_FILTER_LEN );

    rshift = 0;
    while( maxCoeff > WEBRTC_SPL_WORD16_MAX )
    {
      maxCoeff = WEBRTC_SPL_RSHIFT_W32(maxCoeff, 1);
      rshift++;
    }
    for( i = 0; i < NOISE_FILTER_LEN; i++ ) {
      Vector_Word16_1[ FRAMESAMPLES_HALF - NOISE_FILTER_LEN + i] =
          (WebRtc_Word16)WEBRTC_SPL_RSHIFT_W32(
              (ISACdec_obj->plcstr_obj).prevHP[
                  PITCH_MAX_LAG + 10 - NOISE_FILTER_LEN + i], rshift);
    }
    (ISACdec_obj->plcstr_obj).decayCoeffNoise = plc_filterma_Fast(
        Vector_Word16_2,
        Vector_Word16_Extended_2,
        &Vector_Word16_1[FRAMESAMPLES_HALF - NOISE_FILTER_LEN],
        (WebRtc_Word16) NOISE_FILTER_LEN,
        (WebRtc_Word16) FRAMESAMPLES_HALF,
        (WebRtc_Word16) (5),
        (ISACdec_obj->plcstr_obj).decayCoeffNoise,
        (WebRtc_Word16) (7) );

    for( i = 0; i < FRAMESAMPLES_HALF; i++ )
      Vector_Word32_2[i] = WEBRTC_SPL_LSHIFT_W32(
          (WebRtc_Word32)Vector_Word16_Extended_2[i], rshift );

    Vector_Word16_1 = Vector_Word16_Extended_1;
  }
  else
  {
    if( (ISACdec_obj->plcstr_obj).A == 0 )
    {
      
      for( i = 0, noiseIndex = 0; i < FRAMESAMPLES_HALF; i++, noiseIndex++ )
      {
        
        pLP = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(
            stretchPitchLP[(ISACdec_obj->plcstr_obj).pitchIndex],
            (ISACdec_obj->plcstr_obj).decayCoeffPriodic, 15 );

        
        pHP = (WebRtc_Word32)WEBRTC_SPL_MUL_16_32_RSFT15(
            (ISACdec_obj->plcstr_obj).decayCoeffPriodic,
            (ISACdec_obj->plcstr_obj).prevHP[PITCH_MAX_LAG + 10 -
                                             (ISACdec_obj->plcstr_obj).stretchLag +
                                             (ISACdec_obj->plcstr_obj).pitchIndex] );

        
        (ISACdec_obj->plcstr_obj).decayCoeffPriodic -= (myDecayRate);
        if( (ISACdec_obj->plcstr_obj).decayCoeffPriodic < 0 )
          (ISACdec_obj->plcstr_obj).decayCoeffPriodic = 0;

        (ISACdec_obj->plcstr_obj).pitchIndex++;

        if( (ISACdec_obj->plcstr_obj).pitchIndex ==
            (ISACdec_obj->plcstr_obj).stretchLag )
        {
          (ISACdec_obj->plcstr_obj).pitchIndex = 0;
          (ISACdec_obj->plcstr_obj).pitchCycles++;

          if( (ISACdec_obj->plcstr_obj).stretchLag != (lag0 + 1) )
          {
            (ISACdec_obj->plcstr_obj).stretchLag = lag0 + 1;
          }
          else
          {
            (ISACdec_obj->plcstr_obj).stretchLag = lag0;
          }

          (ISACdec_obj->plcstr_obj).stretchLag = (
              (ISACdec_obj->plcstr_obj).stretchLag > PITCH_MAX_LAG
                                                  )? (PITCH_MAX_LAG):(ISACdec_obj->plcstr_obj).stretchLag;

          LinearResampler( (ISACdec_obj->plcstr_obj).lastPitchLP,
                           stretchPitchLP, lag0, (ISACdec_obj->plcstr_obj).stretchLag );

          LinearResampler( (ISACdec_obj->plcstr_obj).prevPitchLP,
                           stretchPitchLP1, lag0, (ISACdec_obj->plcstr_obj).stretchLag );

          switch( (ISACdec_obj->plcstr_obj).pitchCycles )
          {
            case 1:
              {
                for( k=0; k<(ISACdec_obj->plcstr_obj).stretchLag; k++ )
                {
                  stretchPitchLP[k] = (WebRtc_Word16)((
                      (WebRtc_Word32)stretchPitchLP[k]* 3 +
                      (WebRtc_Word32)stretchPitchLP1[k])>>2);
                }
                break;
              }
            case 2:
              {
                for( k=0; k<(ISACdec_obj->plcstr_obj).stretchLag; k++ )
                {
                  stretchPitchLP[k] = (WebRtc_Word16)((
                      (WebRtc_Word32)stretchPitchLP[k] +
                      (WebRtc_Word32)stretchPitchLP1[k] )>>1);
                }
                break;
              }
            case 3:
              {
                for( k=0; k<(ISACdec_obj->plcstr_obj).stretchLag; k++ )
                {
                  stretchPitchLP[k] = (WebRtc_Word16)((stretchPitchLP[k] +
                                                       (WebRtc_Word32)stretchPitchLP1[k]*3 )>>2);
                }
                break;
              }
          }

          if( (ISACdec_obj->plcstr_obj).pitchCycles == 3 )
          {
            myDecayRate += 35; 
            (ISACdec_obj->plcstr_obj).pitchCycles = 0;
          }

        }

        
        Vector_Word16_1[i] = pLP;
        Vector_Word32_2[i] = pHP;
      }
    }
    else
    {
      for( i = 0, noiseIndex = 0; i < FRAMESAMPLES_HALF; i++, noiseIndex++ )
      {

        (ISACdec_obj->plcstr_obj).seed = WEBRTC_SPL_RAND(
            (ISACdec_obj->plcstr_obj).seed );

        noise1 = WEBRTC_SPL_RSHIFT_W16(
            (ISACdec_obj->plcstr_obj).seed, 10 ) - 16;

        nLP = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(
            (WebRtc_Word16)((noise1)*(ISACdec_obj->plcstr_obj).std),
            (ISACdec_obj->plcstr_obj).decayCoeffNoise, 15 );

        
        (ISACdec_obj->plcstr_obj).seed = WEBRTC_SPL_RAND(
            (ISACdec_obj->plcstr_obj).seed );
        noise1 = WEBRTC_SPL_RSHIFT_W16(
            (ISACdec_obj->plcstr_obj).seed, 11 ) - 8;

        nHP = (WebRtc_Word32)WEBRTC_SPL_MUL_16_32_RSFT15(
            (ISACdec_obj->plcstr_obj).decayCoeffNoise,
            (WebRtc_Word32)(noise1*(ISACdec_obj->plcstr_obj).std) );

        
        (ISACdec_obj->plcstr_obj).decayCoeffNoise -= (myDecayRate);
        if( (ISACdec_obj->plcstr_obj).decayCoeffNoise < 0 )
          (ISACdec_obj->plcstr_obj).decayCoeffNoise = 0;

        
        
        pLP = (WebRtc_Word16)WEBRTC_SPL_MUL_16_16_RSFT(
            stretchPitchLP[(ISACdec_obj->plcstr_obj).pitchIndex],
            (ISACdec_obj->plcstr_obj).decayCoeffPriodic, 15 );

        
        pHP = (WebRtc_Word32)WEBRTC_SPL_MUL_16_32_RSFT15(
            (ISACdec_obj->plcstr_obj).decayCoeffPriodic,
            (ISACdec_obj->plcstr_obj).prevHP[PITCH_MAX_LAG + 10 -
                                             (ISACdec_obj->plcstr_obj).stretchLag +
                                             (ISACdec_obj->plcstr_obj).pitchIndex] );

        
        (ISACdec_obj->plcstr_obj).decayCoeffPriodic -= (myDecayRate);
        if( (ISACdec_obj->plcstr_obj).decayCoeffPriodic < 0 )
        {
          (ISACdec_obj->plcstr_obj).decayCoeffPriodic = 0;
        }

        
        wNoisyLP = (WebRtc_Word16)(WEBRTC_SPL_MUL_16_16_RSFT(
            (ISACdec_obj->plcstr_obj).A, nLP, 15 ) );
        wNoisyHP = (WebRtc_Word32)(WEBRTC_SPL_MUL_16_32_RSFT15(
            (ISACdec_obj->plcstr_obj).A, (nHP) ) );

        wPriodicLP = (WebRtc_Word16)(WEBRTC_SPL_MUL_16_16_RSFT(
            (ISACdec_obj->plcstr_obj).B, pLP, 15));
        wPriodicHP = (WebRtc_Word32)(WEBRTC_SPL_MUL_16_32_RSFT15(
            (ISACdec_obj->plcstr_obj).B, pHP));

        (ISACdec_obj->plcstr_obj).pitchIndex++;

        if((ISACdec_obj->plcstr_obj).pitchIndex ==
           (ISACdec_obj->plcstr_obj).stretchLag)
        {
          (ISACdec_obj->plcstr_obj).pitchIndex = 0;
          (ISACdec_obj->plcstr_obj).pitchCycles++;

          if( (ISACdec_obj->plcstr_obj).stretchLag != (lag0 + 1) )
            (ISACdec_obj->plcstr_obj).stretchLag = lag0 + 1;
          else
            (ISACdec_obj->plcstr_obj).stretchLag = lag0;

          (ISACdec_obj->plcstr_obj).stretchLag = (
              (ISACdec_obj->plcstr_obj).stretchLag > PITCH_MAX_LAG
                                                  )? (PITCH_MAX_LAG):(ISACdec_obj->plcstr_obj).stretchLag;
          LinearResampler(
              (ISACdec_obj->plcstr_obj).lastPitchLP,
              stretchPitchLP, lag0, (ISACdec_obj->plcstr_obj).stretchLag );

          LinearResampler((ISACdec_obj->plcstr_obj).prevPitchLP,
                          stretchPitchLP1, lag0, (ISACdec_obj->plcstr_obj).stretchLag );

          switch((ISACdec_obj->plcstr_obj).pitchCycles)
          {
            case 1:
              {
                for( k=0; k<(ISACdec_obj->plcstr_obj).stretchLag; k++ )
                {
                  stretchPitchLP[k] = (WebRtc_Word16)((
                      (WebRtc_Word32)stretchPitchLP[k]* 3 +
                      (WebRtc_Word32)stretchPitchLP1[k] )>>2);
                }
                break;
              }
            case 2:
              {
                for( k=0; k<(ISACdec_obj->plcstr_obj).stretchLag; k++ )
                {
                  stretchPitchLP[k] = (WebRtc_Word16)((
                      (WebRtc_Word32)stretchPitchLP[k] +
                      (WebRtc_Word32)stretchPitchLP1[k])>>1);
                }
                break;
              }
            case 3:
              {
                for( k=0; k<(ISACdec_obj->plcstr_obj).stretchLag; k++ )
                {
                  stretchPitchLP[k] = (WebRtc_Word16)(
                      (stretchPitchLP[k] +
                       (WebRtc_Word32)stretchPitchLP1[k]*3 )>>2);
                }
                break;
              }
          }

          if( (ISACdec_obj->plcstr_obj).pitchCycles == 3 )
          {
            myDecayRate += 55; 
            (ISACdec_obj->plcstr_obj).pitchCycles = 0;
          }
        }

        
        Vector_Word16_1[i] = (WebRtc_Word16)WEBRTC_SPL_ADD_SAT_W16(
            wNoisyLP, wPriodicLP );
        Vector_Word32_2[i] = (WebRtc_Word32)WEBRTC_SPL_ADD_SAT_W32(
            wNoisyHP, wPriodicHP );
      }
    }
  }
  

  k = (ISACdec_obj->plcstr_obj).pitchIndex;
  

  for( i = 0; i < RECOVERY_OVERLAP; i++ )
  {
    (ISACdec_obj->plcstr_obj).overlapLP[i] = (WebRtc_Word16)(
        WEBRTC_SPL_MUL_16_16_RSFT(stretchPitchLP[k],
                                  (ISACdec_obj->plcstr_obj).decayCoeffPriodic, 15) );
    k = ( k < ((ISACdec_obj->plcstr_obj).stretchLag - 1) )? (k+1):0;
  }

  (ISACdec_obj->plcstr_obj).lastPitchLag_Q7 = (ISACdec_obj->plcstr_obj).stretchLag << 7;


  
  WebRtcIsacfix_PitchFilter(Vector_Word16_1, Vector_Word16_2,
                            &ISACdec_obj->pitchfiltstr_obj, pitchLags_Q7, pitchGains_Q12, 4);

  
  
  tmp32a = WEBRTC_SPL_MUL_16_16_RSFT((ISACdec_obj->plcstr_obj).AvgPitchGain_Q12,
                                     29, 0); 
  tmp32b = 262144 - tmp32a;  
  gainQ13 = (WebRtc_Word16) (tmp32b >> 5); 

  
  for (k = 0; k < FRAMESAMPLES_HALF; k++)
    Vector_Word32_1[k] = (WebRtc_Word32) WEBRTC_SPL_MUL_16_16(
        Vector_Word16_2[k], gainQ13) << 3; 


  WebRtcIsacfix_NormLatticeFilterAr(ORDERLO,
                                    (ISACdec_obj->maskfiltstr_obj).PostStateLoGQ0,
                                    Vector_Word32_1, lofilt_coefQ15, gain_lo_hiQ17, 0, Vector_Word16_1);

  WebRtcIsacfix_NormLatticeFilterAr(ORDERHI,
                                    (ISACdec_obj->maskfiltstr_obj).PostStateHiGQ0,
                                    Vector_Word32_2, hifilt_coefQ15, gain_lo_hiQ17, 1, Vector_Word16_2);

  

  
  for (k=0;k<FRAMESAMPLES_HALF;k++)
  {
    
    tmp_1 = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(
                                           ((WebRtc_Word32)Vector_Word16_1[k]+Vector_Word16_2[k] + 1));
    
    tmp_2 = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(
                                           ((WebRtc_Word32)Vector_Word16_1[k]-Vector_Word16_2[k]));
    Vector_Word16_1[k] = tmp_1;
    Vector_Word16_2[k] = tmp_2;
  }


  WebRtcIsacfix_FilterAndCombine1(Vector_Word16_1,
                                  Vector_Word16_2, signal_out16, &ISACdec_obj->postfiltbankstr_obj);

  (ISACdec_obj->plcstr_obj).used = PLC_WAS_USED;
  *current_framesamples = 480;

  return len;
}
