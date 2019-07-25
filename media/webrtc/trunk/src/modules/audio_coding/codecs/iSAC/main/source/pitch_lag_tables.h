
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_PITCH_LAG_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_MAIN_SOURCE_PITCH_LAG_TABLES_H_

#include "typedefs.h"






extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdf1Lo[127];
extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdf2Lo[20];
extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdf3Lo[2];
extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdf4Lo[10];

extern const WebRtc_UWord16 *WebRtcIsac_kQPitchLagCdfPtrLo[4];


extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdfSizeLo[1];


extern const WebRtc_Word16 WebRtcIsac_kQIndexLowerLimitLagLo[4];
extern const WebRtc_Word16 WebRtcIsac_kQIndexUpperLimitLagLo[4];


extern const WebRtc_UWord16 WebRtcIsac_kQInitIndexLagLo[3];


extern const double WebRtcIsac_kQMeanLag2Lo[19];
extern const double WebRtcIsac_kQMeanLag3Lo[1];
extern const double WebRtcIsac_kQMeanLag4Lo[9];

extern const double WebRtcIsac_kQPitchLagStepsizeLo;





extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdf1Mid[255];
extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdf2Mid[36];
extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdf3Mid[2];
extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdf4Mid[20];

extern const WebRtc_UWord16 *WebRtcIsac_kQPitchLagCdfPtrMid[4];


extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdfSizeMid[1];


extern const WebRtc_Word16 WebRtcIsac_kQIndexLowerLimitLagMid[4];
extern const WebRtc_Word16 WebRtcIsac_kQIndexUpperLimitLagMid[4];


extern const WebRtc_UWord16 WebRtcIsac_kQInitIndexLagMid[3];


extern const double WebRtcIsac_kQMeanLag2Mid[35];
extern const double WebRtcIsac_kQMeanLag3Mid[1];
extern const double WebRtcIsac_kQMeanLag4Mid[19];

extern const double WebRtcIsac_kQPitchLagStepsizeMid;





extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdf1Hi[511];
extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdf2Hi[68];
extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdf3Hi[2];
extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdf4Hi[35];

extern const WebRtc_UWord16 *WebRtcIsac_kQPitchLagCdfPtrHi[4];


extern const WebRtc_UWord16 WebRtcIsac_kQPitchLagCdfSizeHi[1];


extern const WebRtc_Word16 WebRtcIsac_kQindexLowerLimitLagHi[4];
extern const WebRtc_Word16 WebRtcIsac_kQindexUpperLimitLagHi[4];


extern const WebRtc_UWord16 WebRtcIsac_kQInitIndexLagHi[3];


extern const double WebRtcIsac_kQMeanLag2Hi[67];
extern const double WebRtcIsac_kQMeanLag3Hi[1];
extern const double WebRtcIsac_kQMeanLag4Hi[34];

extern const double WebRtcIsac_kQPitchLagStepsizeHi;


extern const double WebRtcIsac_kTransform[4][4];


extern const double WebRtcIsac_kTransformTranspose[4][4];

#endif 
