









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_FILTERBANK_INTERNAL_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_FILTERBANK_INTERNAL_H_

#include "typedefs.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif







void WebRtcIsacfix_HighpassFilterFixDec32(int16_t *io,
                                          int16_t len,
                                          const int16_t *coefficient,
                                          int32_t *state);

typedef void (*AllpassFilter2FixDec16)(
    int16_t *data_ch1,           
    int16_t *data_ch2,           
    const int16_t *factor_ch1,   
    const int16_t *factor_ch2,   
    const int length,            
    int32_t *filter_state_ch1,   
    int32_t *filter_state_ch2);  
extern AllpassFilter2FixDec16 WebRtcIsacfix_AllpassFilter2FixDec16;

void WebRtcIsacfix_AllpassFilter2FixDec16C(
   int16_t *data_ch1,
   int16_t *data_ch2,
   const int16_t *factor_ch1,
   const int16_t *factor_ch2,
   const int length,
   int32_t *filter_state_ch1,
   int32_t *filter_state_ch2);

#if (defined WEBRTC_DETECT_ARM_NEON) || (defined WEBRTC_ARCH_ARM_NEON)
void WebRtcIsacfix_AllpassFilter2FixDec16Neon(
   int16_t *data_ch1,
   int16_t *data_ch2,
   const int16_t *factor_ch1,
   const int16_t *factor_ch2,
   const int length,
   int32_t *filter_state_ch1,
   int32_t *filter_state_ch2);
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif

