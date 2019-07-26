
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_PITCH_ESTIMATOR_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_PITCH_ESTIMATOR_H_

#include "structs.h"

void WebRtcIsacfix_PitchAnalysis(const int16_t *in,               
                                 int16_t *outQ0,                  
                                 PitchAnalysisStruct *State,
                                 int16_t *lagsQ7,
                                 int16_t *PitchGains_Q12);

void WebRtcIsacfix_InitialPitch(const int16_t *in,
                                PitchAnalysisStruct *State,
                                int16_t *qlags);

void WebRtcIsacfix_PitchFilter(int16_t *indatFix,
                               int16_t *outdatQQ,
                               PitchFiltstr *pfp,
                               int16_t *lagsQ7,
                               int16_t *gainsQ12,
                               int16_t type);

void WebRtcIsacfix_PitchFilterCore(int loopNumber,
                                   int16_t gain,
                                   int index,
                                   int16_t sign,
                                   int16_t* inputState,
                                   int16_t* outputBuff2,
                                   const int16_t* coefficient,
                                   int16_t* inputBuf,
                                   int16_t* outputBuf,
                                   int* index2);

void WebRtcIsacfix_PitchFilterGains(const int16_t *indatQ0,
                                    PitchFiltstr *pfp,
                                    int16_t *lagsQ7,
                                    int16_t *gainsQ12);

void WebRtcIsacfix_DecimateAllpass32(const int16_t *in,
                                     int32_t *state_in,        
                                     int16_t N,                   
                                     int16_t *out);             

#endif 
