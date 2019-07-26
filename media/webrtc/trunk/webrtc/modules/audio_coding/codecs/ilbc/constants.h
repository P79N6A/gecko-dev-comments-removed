

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CONSTANTS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_CONSTANTS_H_

#include "defines.h"
#include "typedefs.h"



extern const WebRtc_Word16 WebRtcIlbcfix_kHpInCoefs[];
extern const WebRtc_Word16 WebRtcIlbcfix_kHpOutCoefs[];


extern const WebRtc_Word16 WebRtcIlbcfix_kStartSequenceEnrgWin[];


extern const WebRtc_Word16 WebRtcIlbcfix_kLpFiltCoefs[];



extern const WebRtc_Word16 WebRtcIlbcfix_kLpcWin[];
extern const WebRtc_Word16 WebRtcIlbcfix_kLpcAsymWin[];
extern const WebRtc_Word32 WebRtcIlbcfix_kLpcLagWin[];
extern const WebRtc_Word16 WebRtcIlbcfix_kLpcChirpSyntDenum[];
extern const WebRtc_Word16 WebRtcIlbcfix_kLpcChirpWeightDenum[];
extern const WebRtc_Word16 WebRtcIlbcfix_kLsfDimCb[];
extern const WebRtc_Word16 WebRtcIlbcfix_kLsfSizeCb[];
extern const WebRtc_Word16 WebRtcIlbcfix_kLsfCb[];
extern const WebRtc_Word16 WebRtcIlbcfix_kLsfWeight20ms[];
extern const WebRtc_Word16 WebRtcIlbcfix_kLsfWeight30ms[];
extern const WebRtc_Word16 WebRtcIlbcfix_kLsfMean[];
extern const WebRtc_Word16 WebRtcIlbcfix_kLspMean[];
extern const WebRtc_Word16 WebRtcIlbcfix_kCos[];
extern const WebRtc_Word16 WebRtcIlbcfix_kCosDerivative[];
extern const WebRtc_Word16 WebRtcIlbcfix_kCosGrid[];
extern const WebRtc_Word16 WebRtcIlbcfix_kAcosDerivative[];



extern const WebRtc_Word16 WebRtcIlbcfix_kStateSq3[];
extern const WebRtc_Word32 WebRtcIlbcfix_kChooseFrgQuant[];
extern const WebRtc_Word16 WebRtcIlbcfix_kScale[];
extern const WebRtc_Word16 WebRtcIlbcfix_kFrgQuantMod[];



extern const WebRtc_Word16 WebRtcIlbcfix_kSearchRange[5][CB_NSTAGES];
extern const WebRtc_Word16 WebRtcIlbcfix_kFilterRange[];



extern const WebRtc_Word16 WebRtcIlbcfix_kGainSq3[];
extern const WebRtc_Word16 WebRtcIlbcfix_kGainSq4[];
extern const WebRtc_Word16 WebRtcIlbcfix_kGainSq5[];
extern const WebRtc_Word16 WebRtcIlbcfix_kGainSq5Sq[];
extern const WebRtc_Word16* const WebRtcIlbcfix_kGain[];



extern const WebRtc_Word16 WebRtcIlbcfix_kCbFiltersRev[];
extern const WebRtc_Word16 WebRtcIlbcfix_kAlpha[];



extern const WebRtc_Word16 WebRtcIlbcfix_kEnhPolyPhaser[ENH_UPS0][ENH_FLO_MULT2_PLUS1];
extern const WebRtc_Word16 WebRtcIlbcfix_kEnhWt[];
extern const WebRtc_Word16 WebRtcIlbcfix_kEnhPlocs[];



extern const WebRtc_Word16 WebRtcIlbcfix_kPlcPerSqr[];
extern const WebRtc_Word16 WebRtcIlbcfix_kPlcPitchFact[];
extern const WebRtc_Word16 WebRtcIlbcfix_kPlcPfSlope[];

#endif
