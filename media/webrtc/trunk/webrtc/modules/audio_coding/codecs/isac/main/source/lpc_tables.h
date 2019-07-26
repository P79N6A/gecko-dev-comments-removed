
















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


extern const uint16_t WebRtcIsac_kQKltModelCdf[KLT_NUM_MODELS+1];


extern const uint16_t *WebRtcIsac_kQKltModelCdfPtr[1];


extern const uint16_t WebRtcIsac_kQKltModelInitIndex[1];


extern const short WebRtcIsac_kQKltQuantMinGain[12];

extern const short WebRtcIsac_kQKltQuantMinShape[108];


extern const uint16_t WebRtcIsac_kQKltMaxIndGain[12];

extern const uint16_t WebRtcIsac_kQKltMaxIndShape[108];


extern const uint16_t WebRtcIsac_kQKltOffsetGain[12];

extern const uint16_t WebRtcIsac_kQKltOffsetShape[108];


extern const uint16_t WebRtcIsac_kQKltInitIndexGain[12];

extern const uint16_t WebRtcIsac_kQKltInitIndexShape[108];


extern const double WebRtcIsac_kQKltLevelsGain[392];

extern const double WebRtcIsac_kQKltLevelsShape[578];


extern const uint16_t WebRtcIsac_kQKltCdfGain[404];

extern const uint16_t WebRtcIsac_kQKltCdfShape[686];


extern const uint16_t *WebRtcIsac_kQKltCdfPtrGain[12];

extern const uint16_t *WebRtcIsac_kQKltCdfPtrShape[108];


extern const double WebRtcIsac_kKltT1Gain[4];

extern const double WebRtcIsac_kKltT1Shape[324];


extern const double WebRtcIsac_kKltT2Gain[36];

extern const double WebRtcIsac_kKltT2Shape[36];


extern const double WebRtcIsac_kLpcMeansGain[12];

extern const double WebRtcIsac_kLpcMeansShape[108];

#endif 
