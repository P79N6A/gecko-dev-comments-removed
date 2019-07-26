









#include <assert.h>

#include "webrtc/modules/audio_coding/codecs/isac/fix/source/codec.h"



int WebRtcIsacfix_AutocorrC(WebRtc_Word32* __restrict r,
                            const WebRtc_Word16* __restrict x,
                            WebRtc_Word16 N,
                            WebRtc_Word16 order,
                            WebRtc_Word16* __restrict scale) {
  int i = 0;
  int j = 0;
  int16_t scaling = 0;
  int32_t sum = 0;
  uint32_t temp = 0;
  int64_t prod = 0;

  
  assert(N % 4 == 0);
  assert(N >= 8);

  
  for (i = 0; i < N; i++) {
    prod += WEBRTC_SPL_MUL_16_16(x[i], x[i]);
  }

  
  temp = (uint32_t)(prod >> 31);
  if(temp == 0) {
    scaling = 0;
  } else {
    scaling = 32 - WebRtcSpl_NormU32(temp);
  }
  r[0] = (int32_t)(prod >> scaling);

  
  for (i = 1; i < order + 1; i++) {
    prod = 0;
    for (j = 0; j < N - i; j++) {
      prod += WEBRTC_SPL_MUL_16_16(x[j], x[i + j]);
    }
    sum = (int32_t)(prod >> scaling);
    r[i] = sum;
  }

  *scale = scaling;

  return(order + 1);
}

static const WebRtc_Word32 kApUpperQ15[ALLPASSSECTIONS] = { 1137, 12537 };
static const WebRtc_Word32 kApLowerQ15[ALLPASSSECTIONS] = { 5059, 24379 };


static void AllpassFilterForDec32(WebRtc_Word16         *InOut16, 
                                  const WebRtc_Word32   *APSectionFactors, 
                                  WebRtc_Word16         lengthInOut,
                                  WebRtc_Word32          *FilterState) 
{
  int n, j;
  WebRtc_Word32 a, b;

  for (j=0; j<ALLPASSSECTIONS; j++) {
    for (n=0;n<lengthInOut;n+=2){
      a = WEBRTC_SPL_MUL_16_32_RSFT16(InOut16[n], APSectionFactors[j]); 
      a = WEBRTC_SPL_LSHIFT_W32(a, 1); 
      b = WEBRTC_SPL_ADD_SAT_W32(a, FilterState[j]); 
      a = WEBRTC_SPL_MUL_16_32_RSFT16(
          (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(b, 16),
          -APSectionFactors[j]); 
      FilterState[j] = WEBRTC_SPL_ADD_SAT_W32(
          WEBRTC_SPL_LSHIFT_W32(a,1),
          WEBRTC_SPL_LSHIFT_W32((WebRtc_UWord32)InOut16[n], 16)); 
      InOut16[n] = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(b, 16); 
    }
  }
}




void WebRtcIsacfix_DecimateAllpass32(const WebRtc_Word16 *in,
                                     WebRtc_Word32 *state_in,        
                                     WebRtc_Word16 N,                
                                     WebRtc_Word16 *out)             
{
  int n;
  WebRtc_Word16 data_vec[PITCH_FRAME_LEN];

  
  memcpy(data_vec+1, in, WEBRTC_SPL_MUL_16_16(sizeof(WebRtc_Word16), (N-1)));


  data_vec[0] = (WebRtc_Word16) WEBRTC_SPL_RSHIFT_W32(state_in[WEBRTC_SPL_MUL_16_16(2, ALLPASSSECTIONS)],16);   
  state_in[WEBRTC_SPL_MUL_16_16(2, ALLPASSSECTIONS)] = WEBRTC_SPL_LSHIFT_W32((WebRtc_UWord32)in[N-1],16);



  AllpassFilterForDec32(data_vec+1, kApUpperQ15, N, state_in);
  AllpassFilterForDec32(data_vec, kApLowerQ15, N, state_in+ALLPASSSECTIONS);

  for (n=0;n<N/2;n++) {
    out[n]=WEBRTC_SPL_ADD_SAT_W16(data_vec[WEBRTC_SPL_MUL_16_16(2, n)], data_vec[WEBRTC_SPL_MUL_16_16(2, n)+1]);
  }
}
