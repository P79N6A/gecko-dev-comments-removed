
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_LPC_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_LPC_TABLES_H_

#include "webrtc/typedefs.h"


extern const uint16_t WebRtcIsacfix_kSelIndGain[12];

extern const uint16_t WebRtcIsacfix_kSelIndShape[108];


extern const uint16_t WebRtcIsacfix_kModelCdf[KLT_NUM_MODELS+1];


extern const uint16_t *WebRtcIsacfix_kModelCdfPtr[1];


extern const uint16_t WebRtcIsacfix_kModelInitIndex[1];


extern const int16_t WebRtcIsacfix_kQuantMinGain[12];

extern const int16_t WebRtcIsacfix_kQuantMinShape[108];


extern const uint16_t WebRtcIsacfix_kMaxIndGain[12];

extern const uint16_t WebRtcIsacfix_kMaxIndShape[108];


extern const uint16_t WebRtcIsacfix_kOffsetGain[KLT_NUM_MODELS][12];

extern const uint16_t WebRtcIsacfix_kOffsetShape[KLT_NUM_MODELS][108];


extern const uint16_t WebRtcIsacfix_kInitIndexGain[KLT_NUM_MODELS][12];

extern const uint16_t WebRtcIsacfix_kInitIndexShape[KLT_NUM_MODELS][108];


extern const uint16_t WebRtcIsacfix_kOfLevelsGain[3];

extern const uint16_t WebRtcIsacfix_kOfLevelsShape[3];


extern const int32_t WebRtcIsacfix_kLevelsGainQ17[1176];

extern const int16_t WebRtcIsacfix_kLevelsShapeQ10[1735];


extern const uint16_t WebRtcIsacfix_kCdfGain[1212];

extern const uint16_t WebRtcIsacfix_kCdfShape[2059];


extern const uint16_t *WebRtcIsacfix_kCdfGainPtr[KLT_NUM_MODELS][12];

extern const uint16_t *WebRtcIsacfix_kCdfShapePtr[KLT_NUM_MODELS][108];


extern const int16_t WebRtcIsacfix_kCodeLenGainQ11[392];

extern const int16_t WebRtcIsacfix_kCodeLenShapeQ11[577];


extern const int16_t WebRtcIsacfix_kT1GainQ15[KLT_NUM_MODELS][4];

extern const int16_t WebRtcIsacfix_kT1ShapeQ15[KLT_NUM_MODELS][324];


extern const int16_t WebRtcIsacfix_kT2GainQ15[KLT_NUM_MODELS][36];

extern const int16_t WebRtcIsacfix_kT2ShapeQ15[KLT_NUM_MODELS][36];


extern const int16_t WebRtcIsacfix_kMeansGainQ8[KLT_NUM_MODELS][12];

extern const int32_t WebRtcIsacfix_kMeansShapeQ17[3][108];

#endif 
