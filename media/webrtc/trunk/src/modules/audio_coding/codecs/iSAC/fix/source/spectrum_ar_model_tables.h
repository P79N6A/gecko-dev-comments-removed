

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_SPECTRUM_AR_MODEL_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_SPECTRUM_AR_MODEL_TABLES_H_

#include "typedefs.h"
#include "settings.h"




extern const WebRtc_UWord16 WebRtcIsacfix_kRc1Cdf[12];


extern const WebRtc_UWord16 WebRtcIsacfix_kRc2Cdf[12];


extern const WebRtc_UWord16 WebRtcIsacfix_kRc3Cdf[12];


extern const WebRtc_UWord16 WebRtcIsacfix_kRc4Cdf[12];


extern const WebRtc_UWord16 WebRtcIsacfix_kRc5Cdf[12];


extern const WebRtc_UWord16 WebRtcIsacfix_kRc6Cdf[12];


extern const WebRtc_Word16 WebRtcIsacfix_kRc1Levels[11];


extern const WebRtc_Word16 WebRtcIsacfix_kRc2Levels[11];


extern const WebRtc_Word16 WebRtcIsacfix_kRc3Levels[11];


extern const WebRtc_Word16 WebRtcIsacfix_kRc4Levels[11];


extern const WebRtc_Word16 WebRtcIsacfix_kRc5Levels[11];


extern const WebRtc_Word16 WebRtcIsacfix_kRc6Levels[11];


extern const WebRtc_Word16 WebRtcIsacfix_kRcBound[12];


extern const WebRtc_UWord16 WebRtcIsacfix_kRcInitInd[AR_ORDER];


extern const WebRtc_UWord16 *WebRtcIsacfix_kRcCdfPtr[AR_ORDER];


extern const WebRtc_Word16 *WebRtcIsacfix_kRcLevPtr[AR_ORDER];




extern const WebRtc_UWord16 WebRtcIsacfix_kGainCdf[19];


extern const WebRtc_Word32 WebRtcIsacfix_kGain2Lev[18];


extern const WebRtc_Word32 WebRtcIsacfix_kGain2Bound[19];


extern const WebRtc_UWord16 *WebRtcIsacfix_kGainPtr[1];


extern const WebRtc_UWord16 WebRtcIsacfix_kGainInitInd[1];



extern const WebRtc_Word16 WebRtcIsacfix_kCos[6][60];

#endif 
