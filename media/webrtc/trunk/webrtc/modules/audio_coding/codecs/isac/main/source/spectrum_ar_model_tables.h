

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_SPECTRUM_AR_MODEL_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_SPECTRUM_AR_MODEL_TABLES_H_

#include "structs.h"



extern const WebRtc_UWord16 WebRtcIsac_kQArRc1Cdf[12];


extern const WebRtc_UWord16 WebRtcIsac_kQArRc2Cdf[12];


extern const WebRtc_UWord16 WebRtcIsac_kQArRc3Cdf[12];


extern const WebRtc_UWord16 WebRtcIsac_kQArRc4Cdf[12];


extern const WebRtc_UWord16 WebRtcIsac_kQArRc5Cdf[12];


extern const WebRtc_UWord16 WebRtcIsac_kQArRc6Cdf[12];


extern const WebRtc_Word16 WebRtcIsac_kQArBoundaryLevels[12];


extern const WebRtc_UWord16 WebRtcIsac_kQArRcInitIndex[AR_ORDER];


extern const WebRtc_UWord16 *WebRtcIsac_kQArRcCdfPtr[AR_ORDER];


extern const WebRtc_Word16 *WebRtcIsac_kQArRcLevelsPtr[AR_ORDER];




extern const WebRtc_UWord16 WebRtcIsac_kQGainCdf[19];


extern const WebRtc_Word32 WebRtcIsac_kQGain2Levels[18];


extern const WebRtc_Word32 WebRtcIsac_kQGain2BoundaryLevels[19];


extern const WebRtc_UWord16 *WebRtcIsac_kQGainCdf_ptr[1];


extern const WebRtc_UWord16 WebRtcIsac_kQGainInitIndex[1];



extern const WebRtc_Word16 WebRtcIsac_kCos[6][60];

#endif 
