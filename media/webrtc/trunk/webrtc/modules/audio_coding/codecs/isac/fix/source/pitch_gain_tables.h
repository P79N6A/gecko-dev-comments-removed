
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_PITCH_GAIN_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_PITCH_GAIN_TABLES_H_

#include "typedefs.h"




extern const uint16_t WebRtcIsacfix_kPitchGainCdf[255];


extern const int16_t WebRtcIsacfix_kLowerlimiGain[3];
extern const int16_t WebRtcIsacfix_kUpperlimitGain[3];
extern const uint16_t WebRtcIsacfix_kMultsGain[2];


extern const int16_t WebRtcIsacfix_kPitchGain1[144];
extern const int16_t WebRtcIsacfix_kPitchGain2[144];
extern const int16_t WebRtcIsacfix_kPitchGain3[144];
extern const int16_t WebRtcIsacfix_kPitchGain4[144];


extern const uint16_t WebRtcIsacfix_kCdfTableSizeGain[1];


extern const int16_t WebRtcIsacfix_kTransform[4][4];

#endif 
