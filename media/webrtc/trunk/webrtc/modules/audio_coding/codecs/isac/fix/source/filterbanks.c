



















#include "filterbank_internal.h"

#include <assert.h>

#include "codec.h"
#include "filterbank_tables.h"
#include "settings.h"


AllpassFilter2FixDec16 WebRtcIsacfix_AllpassFilter2FixDec16;

void WebRtcIsacfix_AllpassFilter2FixDec16C(
    int16_t *data_ch1,  
    int16_t *data_ch2,  
    const int16_t *factor_ch1,  
    const int16_t *factor_ch2,  
    const int length,  
    int32_t *filter_state_ch1,  
    int32_t *filter_state_ch2) {  
  int n = 0;
  int32_t state0_ch1 = filter_state_ch1[0], state1_ch1 = filter_state_ch1[1];
  int32_t state0_ch2 = filter_state_ch2[0], state1_ch2 = filter_state_ch2[1];
  int16_t in_out = 0;
  int32_t a = 0, b = 0;

  
  assert(length % 2 == 0);

  for (n = 0; n < length; n++) {
    
    in_out = data_ch1[n];
    a = WEBRTC_SPL_MUL_16_16(factor_ch1[0], in_out);  
    a <<= 1;  
    b = WEBRTC_SPL_ADD_SAT_W32(a, state0_ch1);
    a = WEBRTC_SPL_MUL_16_16(-factor_ch1[0], (int16_t) (b >> 16));  
    state0_ch1 = WEBRTC_SPL_ADD_SAT_W32(a << 1, (uint32_t)in_out << 16);  
    in_out = (int16_t) (b >> 16);  

    a = WEBRTC_SPL_MUL_16_16(factor_ch1[1], in_out);  
    a <<= 1; 
    b = WEBRTC_SPL_ADD_SAT_W32(a, state1_ch1);  
    a = WEBRTC_SPL_MUL_16_16(-factor_ch1[1], (int16_t) (b >> 16));  
    state1_ch1 = WEBRTC_SPL_ADD_SAT_W32(a << 1, (uint32_t)in_out << 16);  
    data_ch1[n] = (int16_t) (b >> 16);  

    
    in_out = data_ch2[n];
    a = WEBRTC_SPL_MUL_16_16(factor_ch2[0], in_out);  
    a <<= 1;  
    b = WEBRTC_SPL_ADD_SAT_W32(a, state0_ch2);  
    a = WEBRTC_SPL_MUL_16_16(-factor_ch2[0], (int16_t) (b >> 16));  
    state0_ch2 = WEBRTC_SPL_ADD_SAT_W32(a << 1, (uint32_t)in_out << 16);  
    in_out = (int16_t) (b >> 16);  

    a = WEBRTC_SPL_MUL_16_16(factor_ch2[1], in_out);  
    a <<= 1;  
    b = WEBRTC_SPL_ADD_SAT_W32(a, state1_ch2);  
    a = WEBRTC_SPL_MUL_16_16(-factor_ch2[1], (int16_t) (b >> 16));  
    state1_ch2 = WEBRTC_SPL_ADD_SAT_W32(a << 1, (uint32_t)in_out << 16);  
    data_ch2[n] = (int16_t) (b >> 16);  
  }

  filter_state_ch1[0] = state0_ch1;
  filter_state_ch1[1] = state1_ch1;
  filter_state_ch2[0] = state0_ch2;
  filter_state_ch2[1] = state1_ch2;
}

void WebRtcIsacfix_HighpassFilterFixDec32(int16_t *io,
                                          int16_t len,
                                          const int16_t *coefficient,
                                          int32_t *state)
{
  int k;
  WebRtc_Word32 a1 = 0, b1 = 0, c = 0, in = 0;
  WebRtc_Word32 a2 = 0, b2 = 0;
  WebRtc_Word32 state0 = state[0];
  WebRtc_Word32 state1 = state[1];

  for (k=0; k<len; k++) {
    in = (WebRtc_Word32)io[k];

#ifdef WEBRTC_ARCH_ARM_V7
    {
      int tmp_coeff0 = 0;
      int tmp_coeff1 = 0;
      __asm __volatile(
        "ldr %[tmp_coeff0], [%[coeff]]\n\t"
        "ldr %[tmp_coeff1], [%[coeff], #4]\n\t"
        "smmulr %[a2], %[tmp_coeff0], %[state0]\n\t"
        "smmulr %[b2], %[tmp_coeff1], %[state1]\n\t"
        "ldr %[tmp_coeff0], [%[coeff], #8]\n\t"
        "ldr %[tmp_coeff1], [%[coeff], #12]\n\t"
        "smmulr %[a1], %[tmp_coeff0], %[state0]\n\t"
        "smmulr %[b1], %[tmp_coeff1], %[state1]\n\t"
        :[a2]"+r"(a2),
         [b2]"+r"(b2),
         [a1]"+r"(a1),
         [b1]"+r"(b1),
         [tmp_coeff0]"+r"(tmp_coeff0),
         [tmp_coeff1]"+r"(tmp_coeff1)
        :[coeff]"r"(coefficient),
         [state0]"r"(state0),
         [state1]"r"(state1)
      );
    }
#else
    
    a1 = WEBRTC_SPL_MUL_32_32_RSFT32(coefficient[5], coefficient[4], state0);
    b1 = WEBRTC_SPL_MUL_32_32_RSFT32(coefficient[7], coefficient[6], state1);

    
    a2 = WEBRTC_SPL_MUL_32_32_RSFT32(coefficient[1], coefficient[0], state0);
    b2 = WEBRTC_SPL_MUL_32_32_RSFT32(coefficient[3], coefficient[2], state1);
#endif

    c = ((WebRtc_Word32)in) + WEBRTC_SPL_RSHIFT_W32(a1+b1, 7);  
    io[k] = (WebRtc_Word16)WebRtcSpl_SatW32ToW16(c);  

    c = WEBRTC_SPL_LSHIFT_W32((WebRtc_Word32)in, 2) - a2 - b2;  
    c = (WebRtc_Word32)WEBRTC_SPL_SAT(536870911, c, -536870912);

    state1 = state0;
    state0 = WEBRTC_SPL_LSHIFT_W32(c, 2);  
  }
  state[0] = state0;
  state[1] = state1;
}


void WebRtcIsacfix_SplitAndFilter1(WebRtc_Word16 *pin,
                                   WebRtc_Word16 *LP16,
                                   WebRtc_Word16 *HP16,
                                   PreFiltBankstr *prefiltdata)
{
  
  


  int k;

  WebRtc_Word16 tempin_ch1[FRAMESAMPLES/2 + QLOOKAHEAD];
  WebRtc_Word16 tempin_ch2[FRAMESAMPLES/2 + QLOOKAHEAD];
  WebRtc_Word32 tmpState_ch1[2 * (QORDER-1)]; 
  WebRtc_Word32 tmpState_ch2[2 * (QORDER-1)]; 

  
  WebRtcIsacfix_HighpassFilterFixDec32(pin, FRAMESAMPLES, WebRtcIsacfix_kHpStCoeffInQ30, prefiltdata->HPstates_fix);


  
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


  
  

  WebRtcIsacfix_AllpassFilter2FixDec16(tempin_ch1,
                                       tempin_ch2,
                                       WebRtcIsacfix_kUpperApFactorsQ15,
                                       WebRtcIsacfix_kLowerApFactorsQ15,
                                       FRAMESAMPLES/2,
                                       prefiltdata->INSTAT1_fix,
                                       prefiltdata->INSTAT2_fix);

  for (k=0;k<WEBRTC_SPL_MUL_16_16(2, (QORDER-1));k++) {
    tmpState_ch1[k] = prefiltdata->INSTAT1_fix[k];
    tmpState_ch2[k] = prefiltdata->INSTAT2_fix[k];
  }
  WebRtcIsacfix_AllpassFilter2FixDec16(tempin_ch1 + FRAMESAMPLES/2,
                                       tempin_ch2 + FRAMESAMPLES/2,
                                       WebRtcIsacfix_kUpperApFactorsQ15,
                                       WebRtcIsacfix_kLowerApFactorsQ15,
                                       QLOOKAHEAD,
                                       tmpState_ch1,
                                       tmpState_ch2);

  
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


  
  WebRtcIsacfix_HighpassFilterFixDec32(pin, FRAMESAMPLES, WebRtcIsacfix_kHpStCoeffInQ30, prefiltdata->HPstates_fix);


  
  for (k=0;k<FRAMESAMPLES/2;k++) {
    tempin_ch1[k] = pin[1+WEBRTC_SPL_MUL_16_16(2, k)];
  }

  

  for (k=0;k<FRAMESAMPLES/2;k++) {
    tempin_ch2[k] = pin[WEBRTC_SPL_MUL_16_16(2, k)];
  }


  
  

  WebRtcIsacfix_AllpassFilter2FixDec16(tempin_ch1,
                                       tempin_ch2,
                                       WebRtcIsacfix_kUpperApFactorsQ15,
                                       WebRtcIsacfix_kLowerApFactorsQ15,
                                       FRAMESAMPLES/2,
                                       prefiltdata->INSTAT1_fix,
                                       prefiltdata->INSTAT2_fix);

  
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

  







  WebRtcIsacfix_AllpassFilter2FixDec16(tempin_ch1,
                                       tempin_ch2,
                                       WebRtcIsacfix_kLowerApFactorsQ15,
                                       WebRtcIsacfix_kUpperApFactorsQ15,
                                       FRAMESAMPLES/2,
                                       postfiltdata->STATE_0_UPPER_fix,
                                       postfiltdata->STATE_0_LOWER_fix);

  
  for (k=0;k<FRAMESAMPLES/2;k++) {
    in[WEBRTC_SPL_MUL_16_16(2, k)]=tempin_ch2[k];
    in[WEBRTC_SPL_MUL_16_16(2, k)+1]=tempin_ch1[k];
  }

  
  WebRtcIsacfix_HighpassFilterFixDec32(in, FRAMESAMPLES, WebRtcIsacfix_kHPStCoeffOut1Q30, postfiltdata->HPstates1_fix);
  WebRtcIsacfix_HighpassFilterFixDec32(in, FRAMESAMPLES, WebRtcIsacfix_kHPStCoeffOut2Q30, postfiltdata->HPstates2_fix);

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

  







  WebRtcIsacfix_AllpassFilter2FixDec16(tempin_ch1,
                                       tempin_ch2,
                                       WebRtcIsacfix_kLowerApFactorsQ15,
                                       WebRtcIsacfix_kUpperApFactorsQ15,
                                       len / 2,
                                       postfiltdata->STATE_0_UPPER_fix,
                                       postfiltdata->STATE_0_LOWER_fix);

  
  for (k=0;k<len/2;k++) {
    in[WEBRTC_SPL_MUL_16_16(2, k)]=tempin_ch2[k];
    in[WEBRTC_SPL_MUL_16_16(2, k)+1]=tempin_ch1[k];
  }

  
  WebRtcIsacfix_HighpassFilterFixDec32(in, len, WebRtcIsacfix_kHPStCoeffOut1Q30, postfiltdata->HPstates1_fix);
  WebRtcIsacfix_HighpassFilterFixDec32(in, len, WebRtcIsacfix_kHPStCoeffOut2Q30, postfiltdata->HPstates2_fix);

  for (k=0;k<len;k++) {
    out16[k] = in[k];
  }
}

#endif
