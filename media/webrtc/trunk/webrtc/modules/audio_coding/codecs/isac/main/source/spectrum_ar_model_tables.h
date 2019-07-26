

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_SPECTRUM_AR_MODEL_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_SPECTRUM_AR_MODEL_TABLES_H_

#include "structs.h"



extern const uint16_t WebRtcIsac_kQArRc1Cdf[12];


extern const uint16_t WebRtcIsac_kQArRc2Cdf[12];


extern const uint16_t WebRtcIsac_kQArRc3Cdf[12];


extern const uint16_t WebRtcIsac_kQArRc4Cdf[12];


extern const uint16_t WebRtcIsac_kQArRc5Cdf[12];


extern const uint16_t WebRtcIsac_kQArRc6Cdf[12];


extern const int16_t WebRtcIsac_kQArBoundaryLevels[12];


extern const uint16_t WebRtcIsac_kQArRcInitIndex[AR_ORDER];


extern const uint16_t *WebRtcIsac_kQArRcCdfPtr[AR_ORDER];


extern const int16_t *WebRtcIsac_kQArRcLevelsPtr[AR_ORDER];




extern const uint16_t WebRtcIsac_kQGainCdf[19];


extern const int32_t WebRtcIsac_kQGain2Levels[18];


extern const int32_t WebRtcIsac_kQGain2BoundaryLevels[19];


extern const uint16_t *WebRtcIsac_kQGainCdf_ptr[1];


extern const uint16_t WebRtcIsac_kQGainInitIndex[1];



extern const int16_t WebRtcIsac_kCos[6][60];

#endif 
