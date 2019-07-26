
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_PITCH_GAIN_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_PITCH_GAIN_TABLES_H_

#include "typedefs.h"




extern const WebRtc_UWord16 WebRtcIsacfix_kPitchGainCdf[255];


extern const WebRtc_Word16 WebRtcIsacfix_kLowerlimiGain[3];
extern const WebRtc_Word16 WebRtcIsacfix_kUpperlimitGain[3];
extern const WebRtc_UWord16 WebRtcIsacfix_kMultsGain[2];


extern const WebRtc_Word16 WebRtcIsacfix_kPitchGain1[144];
extern const WebRtc_Word16 WebRtcIsacfix_kPitchGain2[144];
extern const WebRtc_Word16 WebRtcIsacfix_kPitchGain3[144];
extern const WebRtc_Word16 WebRtcIsacfix_kPitchGain4[144];


extern const WebRtc_UWord16 WebRtcIsacfix_kCdfTableSizeGain[1];


extern const WebRtc_Word16 WebRtcIsacfix_kTransform[4][4];

#endif 
