
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_PITCH_ESTIMATOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_PITCH_ESTIMATOR_H_

#include "structs.h"

void WebRtcIsacfix_PitchAnalysis(const WebRtc_Word16 *in,               
                                 WebRtc_Word16 *outQ0,                  
                                 PitchAnalysisStruct *State,
                                 WebRtc_Word16 *lagsQ7,
                                 WebRtc_Word16 *PitchGains_Q12);

void WebRtcIsacfix_InitialPitch(const WebRtc_Word16 *in,
                                PitchAnalysisStruct *State,
                                WebRtc_Word16 *qlags);

void WebRtcIsacfix_PitchFilter(WebRtc_Word16 *indatFix,
                               WebRtc_Word16 *outdatQQ,
                               PitchFiltstr *pfp,
                               WebRtc_Word16 *lagsQ7,
                               WebRtc_Word16 *gainsQ12,
                               WebRtc_Word16 type);

void WebRtcIsacfix_PitchFilterCore(int loopNumber,
                                   WebRtc_Word16 gain,
                                   int index,
                                   WebRtc_Word16 sign,
                                   WebRtc_Word16* inputState,
                                   WebRtc_Word16* outputBuff2,
                                   const WebRtc_Word16* coefficient,
                                   WebRtc_Word16* inputBuf,
                                   WebRtc_Word16* outputBuf,
                                   int* index2);

void WebRtcIsacfix_PitchFilterGains(const WebRtc_Word16 *indatQ0,
                                    PitchFiltstr *pfp,
                                    WebRtc_Word16 *lagsQ7,
                                    WebRtc_Word16 *gainsQ12);

void WebRtcIsacfix_DecimateAllpass32(const WebRtc_Word16 *in,
                                     WebRtc_Word32 *state_in,        
                                     WebRtc_Word16 N,                   
                                     WebRtc_Word16 *out);             

#endif 
