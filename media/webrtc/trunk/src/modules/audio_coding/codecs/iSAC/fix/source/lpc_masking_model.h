
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_LPC_MASKING_MODEL_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_LPC_MASKING_MODEL_H_

#include "structs.h"

void WebRtcIsacfix_GetVars(const WebRtc_Word16 *input,
                           const WebRtc_Word16 *pitchGains_Q12,
                           WebRtc_UWord32 *oldEnergy,
                           WebRtc_Word16 *varscale);

void WebRtcIsacfix_GetLpcCoef(WebRtc_Word16 *inLoQ0,
                              WebRtc_Word16 *inHiQ0,
                              MaskFiltstr_enc *maskdata,
                              WebRtc_Word16 snrQ10,
                              const WebRtc_Word16 *pitchGains_Q12,
                              WebRtc_Word32 *gain_lo_hiQ17,
                              WebRtc_Word16 *lo_coeffQ15,
                              WebRtc_Word16 *hi_coeffQ15);

#endif 
