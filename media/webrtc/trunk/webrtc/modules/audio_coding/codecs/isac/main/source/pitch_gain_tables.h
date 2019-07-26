
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_PITCH_GAIN_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_PITCH_GAIN_TABLES_H_

#include "typedefs.h"




extern const WebRtc_UWord16 WebRtcIsac_kQPitchGainCdf[255];


extern const WebRtc_Word16 WebRtcIsac_kIndexLowerLimitGain[3];

extern const WebRtc_Word16 WebRtcIsac_kIndexUpperLimitGain[3];
extern const WebRtc_UWord16 WebRtcIsac_kIndexMultsGain[2];



extern const WebRtc_Word16 WebRtcIsac_kQMeanGain1Q12[144];
extern const WebRtc_Word16 WebRtcIsac_kQMeanGain2Q12[144];
extern const WebRtc_Word16 WebRtcIsac_kQMeanGain3Q12[144];
extern const WebRtc_Word16 WebRtcIsac_kQMeanGain4Q12[144];



extern const WebRtc_UWord16 WebRtcIsac_kQCdfTableSizeGain[1];

#endif 
