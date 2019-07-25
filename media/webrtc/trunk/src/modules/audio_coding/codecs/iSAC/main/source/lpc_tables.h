
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_LPC_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_LPC_TABLES_H_

#include "structs.h"

#include "settings.h"

#define KLT_STEPSIZE         1.00000000
#define KLT_NUM_AVG_GAIN     0
#define KLT_NUM_AVG_SHAPE    0
#define KLT_NUM_MODELS  3
#define LPC_GAIN_SCALE     4.000f
#define LPC_LOBAND_SCALE   2.100f
#define LPC_LOBAND_ORDER   ORDERLO
#define LPC_HIBAND_SCALE   0.450f
#define LPC_HIBAND_ORDER   ORDERHI
#define LPC_GAIN_ORDER     2

#define LPC_SHAPE_ORDER    (LPC_LOBAND_ORDER + LPC_HIBAND_ORDER)

#define KLT_ORDER_GAIN     (LPC_GAIN_ORDER * SUBFRAMES)
#define KLT_ORDER_SHAPE    (LPC_SHAPE_ORDER * SUBFRAMES)

extern const WebRtc_UWord16 WebRtcIsac_kQKltSelIndGain[12];

extern const WebRtc_UWord16 WebRtcIsac_kQKltSelIndShape[108];


extern const WebRtc_UWord16 WebRtcIsac_kQKltModelCdf[KLT_NUM_MODELS+1];


extern const WebRtc_UWord16 *WebRtcIsac_kQKltModelCdfPtr[1];


extern const WebRtc_UWord16 WebRtcIsac_kQKltModelInitIndex[1];


extern const short WebRtcIsac_kQKltQuantMinGain[12];

extern const short WebRtcIsac_kQKltQuantMinShape[108];


extern const WebRtc_UWord16 WebRtcIsac_kQKltMaxIndGain[12];

extern const WebRtc_UWord16 WebRtcIsac_kQKltMaxIndShape[108];


extern const WebRtc_UWord16 WebRtcIsac_kQKltOffsetGain[KLT_NUM_MODELS][12];

extern const WebRtc_UWord16 WebRtcIsac_kQKltOffsetShape[KLT_NUM_MODELS][108];


extern const WebRtc_UWord16 WebRtcIsac_kQKltInitIndexGain[KLT_NUM_MODELS][12];

extern const WebRtc_UWord16 WebRtcIsac_kQKltInitIndexShape[KLT_NUM_MODELS][108];


extern const WebRtc_UWord16 WebRtcIsac_kQKltOfLevelsGain[3];

extern const WebRtc_UWord16 WebRtcIsac_kQKltOfLevelsShape[3];


extern const double WebRtcIsac_kQKltLevelsGain[1176];

extern const double WebRtcIsac_kQKltLevelsShape[1735];


extern const WebRtc_UWord16 WebRtcIsac_kQKltCdfGain[1212];

extern const WebRtc_UWord16 WebRtcIsac_kQKltCdfShape[2059];


extern const WebRtc_UWord16 *WebRtcIsac_kQKltCdfPtrGain[KLT_NUM_MODELS][12];

extern const WebRtc_UWord16 *WebRtcIsac_kQKltCdfPtrShape[KLT_NUM_MODELS][108];


extern const double WebRtcIsac_kQKltCodeLenGain[392];

extern const double WebRtcIsac_kQKltCodeLenShape[578];


extern const double WebRtcIsac_kKltT1Gain[KLT_NUM_MODELS][4];

extern const double WebRtcIsac_kKltT1Shape[KLT_NUM_MODELS][324];


extern const double WebRtcIsac_kKltT2Gain[KLT_NUM_MODELS][36];

extern const double WebRtcIsac_kKltT2Shape[KLT_NUM_MODELS][36];


extern const double WebRtcIsac_kLpcMeansGain[KLT_NUM_MODELS][12];

extern const double WebRtcIsac_kLpcMeansShape[KLT_NUM_MODELS][108];

#endif 
