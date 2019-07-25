

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_LPC_SHAPE_SWB16_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_LPC_SHAPE_SWB16_TABLES_H_

#include "settings.h"
#include "typedefs.h"


extern const double WebRtcIsac_kMeanLarUb16[UB_LPC_ORDER];

extern const double WebRtcIsac_kIintraVecDecorrMatUb16[UB_LPC_ORDER][UB_LPC_ORDER];

extern const double WebRtcIsac_kInterVecDecorrMatUb16
[UB16_LPC_VEC_PER_FRAME][UB16_LPC_VEC_PER_FRAME];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec01Ub16[14];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec1Ub16[16];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec2Ub16[18];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec3Ub16[30];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec4Ub16[16];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec5Ub16[17];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec6Ub16[21];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec7Ub16[36];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec8Ub16[21];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec01Ub160[21];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec01Ub161[28];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec01Ub162[55];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec01Ub163[26];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec01Ub164[28];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec01Ub165[34];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeCdfVec01Ub166[71];

extern const WebRtc_UWord16* WebRtcIsac_kLpcShapeCdfMatUb16
[UB_LPC_ORDER * UB16_LPC_VEC_PER_FRAME];

extern const double WebRtcIsac_kLpcShapeLeftRecPointUb16
[UB_LPC_ORDER * UB16_LPC_VEC_PER_FRAME];

extern const WebRtc_Word16 WebRtcIsac_kLpcShapeNumRecPointUb16
[UB_LPC_ORDER * UB16_LPC_VEC_PER_FRAME];

extern const WebRtc_UWord16 WebRtcIsac_kLpcShapeEntropySearchUb16
[UB_LPC_ORDER * UB16_LPC_VEC_PER_FRAME];

extern const double WebRtcIsac_kLpcShapeQStepSizeUb16;

#endif 
