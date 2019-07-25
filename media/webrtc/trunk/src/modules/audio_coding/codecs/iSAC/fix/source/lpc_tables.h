
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_LPC_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_LPC_TABLES_H_

#include "typedefs.h"



extern const WebRtc_UWord16 WebRtcIsacfix_kSelIndGain[12];

extern const WebRtc_UWord16 WebRtcIsacfix_kSelIndShape[108];


extern const WebRtc_UWord16 WebRtcIsacfix_kModelCdf[KLT_NUM_MODELS+1];


extern const WebRtc_UWord16 *WebRtcIsacfix_kModelCdfPtr[1];


extern const WebRtc_UWord16 WebRtcIsacfix_kModelInitIndex[1];


extern const WebRtc_Word16 WebRtcIsacfix_kQuantMinGain[12];

extern const WebRtc_Word16 WebRtcIsacfix_kQuantMinShape[108];


extern const WebRtc_UWord16 WebRtcIsacfix_kMaxIndGain[12];

extern const WebRtc_UWord16 WebRtcIsacfix_kMaxIndShape[108];


extern const WebRtc_UWord16 WebRtcIsacfix_kOffsetGain[KLT_NUM_MODELS][12];

extern const WebRtc_UWord16 WebRtcIsacfix_kOffsetShape[KLT_NUM_MODELS][108];


extern const WebRtc_UWord16 WebRtcIsacfix_kInitIndexGain[KLT_NUM_MODELS][12];

extern const WebRtc_UWord16 WebRtcIsacfix_kInitIndexShape[KLT_NUM_MODELS][108];


extern const WebRtc_UWord16 WebRtcIsacfix_kOfLevelsGain[3];

extern const WebRtc_UWord16 WebRtcIsacfix_kOfLevelsShape[3];


extern const WebRtc_Word32 WebRtcIsacfix_kLevelsGainQ17[1176];

extern const WebRtc_Word16 WebRtcIsacfix_kLevelsShapeQ10[1735];


extern const WebRtc_UWord16 WebRtcIsacfix_kCdfGain[1212];

extern const WebRtc_UWord16 WebRtcIsacfix_kCdfShape[2059];


extern const WebRtc_UWord16 *WebRtcIsacfix_kCdfGainPtr[KLT_NUM_MODELS][12];

extern const WebRtc_UWord16 *WebRtcIsacfix_kCdfShapePtr[KLT_NUM_MODELS][108];


extern const WebRtc_Word16 WebRtcIsacfix_kCodeLenGainQ11[392];

extern const WebRtc_Word16 WebRtcIsacfix_kCodeLenShapeQ11[577];


extern const WebRtc_Word16 WebRtcIsacfix_kT1GainQ15[KLT_NUM_MODELS][4];

extern const WebRtc_Word16 WebRtcIsacfix_kT1ShapeQ15[KLT_NUM_MODELS][324];


extern const WebRtc_Word16 WebRtcIsacfix_kT2GainQ15[KLT_NUM_MODELS][36];

extern const WebRtc_Word16 WebRtcIsacfix_kT2ShapeQ15[KLT_NUM_MODELS][36];


extern const WebRtc_Word16 WebRtcIsacfix_kMeansGainQ8[KLT_NUM_MODELS][12];

extern const WebRtc_Word32 WebRtcIsacfix_kMeansShapeQ17[3][108];

#endif 
