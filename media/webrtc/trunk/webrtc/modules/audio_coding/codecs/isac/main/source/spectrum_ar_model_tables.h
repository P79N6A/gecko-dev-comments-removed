

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_SPECTRUM_AR_MODEL_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_SPECTRUM_AR_MODEL_TABLES_H_

#include "structs.h"

#define NUM_AR_RC_QUANT_BAUNDARY 12



extern const uint16_t WebRtcIsac_kQArRc1Cdf[NUM_AR_RC_QUANT_BAUNDARY];


extern const uint16_t WebRtcIsac_kQArRc2Cdf[NUM_AR_RC_QUANT_BAUNDARY];


extern const uint16_t WebRtcIsac_kQArRc3Cdf[NUM_AR_RC_QUANT_BAUNDARY];


extern const uint16_t WebRtcIsac_kQArRc4Cdf[NUM_AR_RC_QUANT_BAUNDARY];


extern const uint16_t WebRtcIsac_kQArRc5Cdf[NUM_AR_RC_QUANT_BAUNDARY];


extern const uint16_t WebRtcIsac_kQArRc6Cdf[NUM_AR_RC_QUANT_BAUNDARY];


extern const int16_t WebRtcIsac_kQArBoundaryLevels[NUM_AR_RC_QUANT_BAUNDARY];


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
