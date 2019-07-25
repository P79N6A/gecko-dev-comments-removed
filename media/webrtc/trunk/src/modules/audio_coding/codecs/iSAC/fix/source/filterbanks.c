



















#include "codec.h"
#include "filterbank_tables.h"
#include "settings.h"


static void AllpassFilter2FixDec16(WebRtc_Word16 *InOut16, 
                                   const WebRtc_Word16 *APSectionFactors, 
                                   WebRtc_Word16 lengthInOut,
                                   WebRtc_Word16 NumberOfSections,
                                   WebRtc_Word32 *FilterState) 
{
  int n, j;
  WebRtc_Word32 a, b;

  for (j=0; j<NumberOfSections; j++) {
    for (n=0;n<lengthInOut;n++) {


      a = WEBRTC_SPL_MUL_16_16(APSectionFactors[j], InOut16[n]); 
      a = WEBRTC_SPL_LSHIFT_W32(a, 1); 
      b = WEBRTC_SPL_ADD_SAT_W32(a, FilterState[j]); 
      a = WEBRTC_SPL_MUL_16_16_RSFT(-APSectionFactors[j], (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(b, 16), 0); 
      FilterState[j] = WEBRTC_SPL_ADD_SAT_W32(WEBRTC_SPL_LSHIFT_W32(a,1), WEBRTC_SPL_LSHIFT_W32((WebRtc_UWord32)InOut16[n],16)); 
      InOut16[n] = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(b, 16); 

    }
  }

}


static void HighpassFilterFixDec32(
    WebRtc_Word16 *io,   
    WebRtc_Word16 len, 
    const WebRtc_Word16 *coeff, 
    WebRtc_Word32 *state) 
{
  int k;
  WebRtc_Word32 a, b, c, in;



  for (k=0; k<len; k++) {
    in = (WebRtc_Word32)io[k];
    
    a = WEBRTC_SPL_MUL_32_32_RSFT32(coeff[2*2], coeff[2*2+1], state[0]);
    b = WEBRTC_SPL_MUL_32_32_RSFT32(coeff[2*3], coeff[2*3+1], state[1]);

    c = ((WebRtc_Word32)in) + WEBRTC_SPL_RSHIFT_W32(a+b, 7); 
    
    io[k] = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(c);  

    
    a = WEBRTC_SPL_MUL_32_32_RSFT32(coeff[2*0], coeff[2*0+1], state[0]);
    b = WEBRTC_SPL_MUL_32_32_RSFT32(coeff[2*1], coeff[2*1+1], state[1]);

    c = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)in, 2) - a - b; 
    c= (WebRtc_Word32)WEBRTC_SPL_SAT((WebRtc_Word32)536870911, c, (WebRtc_Word32)-536870912); 

    state[1] = state[0];
    state[0] = WEBRTC_SPL_LSHIFT_W32(c, 2); 

  }
}


void WebRtcIsacfix_SplitAndFilter1(WebRtc_Word16 *pin,
                                   WebRtc_Word16 *LP16,
                                   WebRtc_Word16 *HP16,
                                   PreFiltBankstr *prefiltdata)
{
  
  


  int k;

  WebRtc_Word16 tempin_ch1[FRAMESAMPLES/2 + QLOOKAHEAD];
  WebRtc_Word16 tempin_ch2[FRAMESAMPLES/2 + QLOOKAHEAD];
  WebRtc_Word32 tmpState[WEBRTC_SPL_MUL_16_16(2,(QORDER-1))]; 


  
  HighpassFilterFixDec32(pin, FRAMESAMPLES, WebRtcIsacfix_kHpStCoeffInQ30, prefiltdata->HPstates_fix);


  
  for (k=0;k<FRAMESAMPLES/2;k++) {
    tempin_ch1[QLOOKAHEAD + k] = pin[1+WEBRTC_SPL_MUL_16_16(2, k)];
  }
  for (k=0;k<QLOOKAHEAD;k++) {
    tempin_ch1[k]=prefiltdata->INLABUF1_fix[k];
    prefiltdata->INLABUF1_fix[k]=pin[FRAMESAMPLES+1-WEBRTC_SPL_MUL_16_16(2, QLOOKAHEAD)+WEBRTC_SPL_MUL_16_16(2, k)];
  }

  

  for (k=0;k<FRAMESAMPLES/2;k++) {
    tempin_ch2[QLOOKAHEAD+k] = pin[WEBRTC_SPL_MUL_16_16(2, k)];
  }
  for (k=0;k<QLOOKAHEAD;k++) {
    tempin_ch2[k]=prefiltdata->INLABUF2_fix[k];
    prefiltdata->INLABUF2_fix[k]=pin[FRAMESAMPLES-WEBRTC_SPL_MUL_16_16(2, QLOOKAHEAD)+WEBRTC_SPL_MUL_16_16(2, k)];
  }


  
  

  AllpassFilter2FixDec16(tempin_ch1,WebRtcIsacfix_kUpperApFactorsQ15, FRAMESAMPLES/2 , NUMBEROFCHANNELAPSECTIONS, prefiltdata->INSTAT1_fix);
  AllpassFilter2FixDec16(tempin_ch2,WebRtcIsacfix_kLowerApFactorsQ15, FRAMESAMPLES/2 , NUMBEROFCHANNELAPSECTIONS, prefiltdata->INSTAT2_fix);

  for (k=0;k<WEBRTC_SPL_MUL_16_16(2, (QORDER-1));k++)
    tmpState[k] = prefiltdata->INSTAT1_fix[k];
  AllpassFilter2FixDec16(tempin_ch1 + FRAMESAMPLES/2,WebRtcIsacfix_kUpperApFactorsQ15, QLOOKAHEAD , NUMBEROFCHANNELAPSECTIONS, tmpState);
  for (k=0;k<WEBRTC_SPL_MUL_16_16(2, (QORDER-1));k++)
    tmpState[k] = prefiltdata->INSTAT2_fix[k];
  AllpassFilter2FixDec16(tempin_ch2 + FRAMESAMPLES/2,WebRtcIsacfix_kLowerApFactorsQ15, QLOOKAHEAD , NUMBEROFCHANNELAPSECTIONS, tmpState);


  
  for (k=0; k<FRAMESAMPLES/2 + QLOOKAHEAD; k++) {
    WebRtc_Word32 tmp1, tmp2, tmp3;
    tmp1 = (WebRtc_Word32)tempin_ch1[k]; 
    tmp2 = (WebRtc_Word32)tempin_ch2[k]; 
    tmp3 = (WebRtc_Word32)WEBRTC_SPL_RSHIFT_W32((tmp1 + tmp2), 1);
    LP16[k] = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(tmp3); 
    tmp3 = (WebRtc_Word32)WEBRTC_SPL_RSHIFT_W32((tmp1 - tmp2), 1);
    HP16[k] = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(tmp3); 
  }

}


#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED


void WebRtcIsacfix_SplitAndFilter2(WebRtc_Word16 *pin,
                                   WebRtc_Word16 *LP16,
                                   WebRtc_Word16 *HP16,
                                   PreFiltBankstr *prefiltdata)
{
  
  


  int k;

  WebRtc_Word16 tempin_ch1[FRAMESAMPLES/2];
  WebRtc_Word16 tempin_ch2[FRAMESAMPLES/2];


  
  HighpassFilterFixDec32(pin, FRAMESAMPLES, WebRtcIsacfix_kHpStCoeffInQ30, prefiltdata->HPstates_fix);


  
  for (k=0;k<FRAMESAMPLES/2;k++) {
    tempin_ch1[k] = pin[1+WEBRTC_SPL_MUL_16_16(2, k)];
  }

  

  for (k=0;k<FRAMESAMPLES/2;k++) {
    tempin_ch2[k] = pin[WEBRTC_SPL_MUL_16_16(2, k)];
  }


  
  

  AllpassFilter2FixDec16(tempin_ch1,WebRtcIsacfix_kUpperApFactorsQ15, FRAMESAMPLES/2 , NUMBEROFCHANNELAPSECTIONS, prefiltdata->INSTAT1_fix);
  AllpassFilter2FixDec16(tempin_ch2,WebRtcIsacfix_kLowerApFactorsQ15, FRAMESAMPLES/2 , NUMBEROFCHANNELAPSECTIONS, prefiltdata->INSTAT2_fix);


  
  for (k=0; k<FRAMESAMPLES/2; k++) {
    WebRtc_Word32 tmp1, tmp2, tmp3;
    tmp1 = (WebRtc_Word32)tempin_ch1[k]; 
    tmp2 = (WebRtc_Word32)tempin_ch2[k]; 
    tmp3 = (WebRtc_Word32)WEBRTC_SPL_RSHIFT_W32((tmp1 + tmp2), 1);
    LP16[k] = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(tmp3); 
    tmp3 = (WebRtc_Word32)WEBRTC_SPL_RSHIFT_W32((tmp1 - tmp2), 1);
    HP16[k] = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(tmp3); 
  }

}

#endif
























void WebRtcIsacfix_FilterAndCombine1(WebRtc_Word16 *tempin_ch1,
                                     WebRtc_Word16 *tempin_ch2,
                                     WebRtc_Word16 *out16,
                                     PostFiltBankstr *postfiltdata)
{
  int k;
  WebRtc_Word16 in[FRAMESAMPLES];

  



  AllpassFilter2FixDec16(tempin_ch1, WebRtcIsacfix_kLowerApFactorsQ15, FRAMESAMPLES/2, NUMBEROFCHANNELAPSECTIONS,postfiltdata->STATE_0_UPPER_fix);

  



  AllpassFilter2FixDec16(tempin_ch2, WebRtcIsacfix_kUpperApFactorsQ15, FRAMESAMPLES/2, NUMBEROFCHANNELAPSECTIONS,postfiltdata->STATE_0_LOWER_fix);

  
  for (k=0;k<FRAMESAMPLES/2;k++) {
    in[WEBRTC_SPL_MUL_16_16(2, k)]=tempin_ch2[k];
    in[WEBRTC_SPL_MUL_16_16(2, k)+1]=tempin_ch1[k];
  }

  
  HighpassFilterFixDec32(in, FRAMESAMPLES, WebRtcIsacfix_kHPStCoeffOut1Q30, postfiltdata->HPstates1_fix);
  HighpassFilterFixDec32(in, FRAMESAMPLES, WebRtcIsacfix_kHPStCoeffOut2Q30, postfiltdata->HPstates2_fix);

  for (k=0;k<FRAMESAMPLES;k++) {
    out16[k] = in[k];
  }
}


#ifdef WEBRTC_ISAC_FIX_NB_CALLS_ENABLED



















void WebRtcIsacfix_FilterAndCombine2(WebRtc_Word16 *tempin_ch1,
                                     WebRtc_Word16 *tempin_ch2,
                                     WebRtc_Word16 *out16,
                                     PostFiltBankstr *postfiltdata,
                                     WebRtc_Word16 len)
{
  int k;
  WebRtc_Word16 in[FRAMESAMPLES];

  



  AllpassFilter2FixDec16(tempin_ch1, WebRtcIsacfix_kLowerApFactorsQ15,(WebRtc_Word16) (len/2), NUMBEROFCHANNELAPSECTIONS,postfiltdata->STATE_0_UPPER_fix);

  



  AllpassFilter2FixDec16(tempin_ch2, WebRtcIsacfix_kUpperApFactorsQ15, (WebRtc_Word16) (len/2), NUMBEROFCHANNELAPSECTIONS,postfiltdata->STATE_0_LOWER_fix);

  
  for (k=0;k<len/2;k++) {
    in[WEBRTC_SPL_MUL_16_16(2, k)]=tempin_ch2[k];
    in[WEBRTC_SPL_MUL_16_16(2, k)+1]=tempin_ch1[k];
  }

  
  HighpassFilterFixDec32(in, len, WebRtcIsacfix_kHPStCoeffOut1Q30, postfiltdata->HPstates1_fix);
  HighpassFilterFixDec32(in, len, WebRtcIsacfix_kHPStCoeffOut2Q30, postfiltdata->HPstates2_fix);

  for (k=0;k<len;k++) {
    out16[k] = in[k];
  }
}

#endif
