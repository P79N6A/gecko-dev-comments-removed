
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_PITCH_LAG_TABLES_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_PITCH_LAG_TABLES_H_

#include "webrtc/typedefs.h"






extern const uint16_t WebRtcIsacfix_kPitchLagCdf1Lo[127];
extern const uint16_t WebRtcIsacfix_kPitchLagCdf2Lo[20];
extern const uint16_t WebRtcIsacfix_kPitchLagCdf3Lo[2];
extern const uint16_t WebRtcIsacfix_kPitchLagCdf4Lo[10];

extern const uint16_t *WebRtcIsacfix_kPitchLagPtrLo[4];


extern const uint16_t WebRtcIsacfix_kPitchLagSizeLo[1];


extern const int16_t WebRtcIsacfix_kLowerLimitLo[4];
extern const int16_t WebRtcIsacfix_kUpperLimitLo[4];


extern const uint16_t WebRtcIsacfix_kInitIndLo[3];


extern const int16_t WebRtcIsacfix_kMeanLag2Lo[19];
extern const int16_t WebRtcIsacfix_kMeanLag4Lo[9];






extern const uint16_t WebRtcIsacfix_kPitchLagCdf1Mid[255];
extern const uint16_t WebRtcIsacfix_kPitchLagCdf2Mid[36];
extern const uint16_t WebRtcIsacfix_kPitchLagCdf3Mid[2];
extern const uint16_t WebRtcIsacfix_kPitchLagCdf4Mid[20];

extern const uint16_t *WebRtcIsacfix_kPitchLagPtrMid[4];


extern const uint16_t WebRtcIsacfix_kPitchLagSizeMid[1];


extern const int16_t WebRtcIsacfix_kLowerLimitMid[4];
extern const int16_t WebRtcIsacfix_kUpperLimitMid[4];


extern const uint16_t WebRtcIsacfix_kInitIndMid[3];


extern const int16_t WebRtcIsacfix_kMeanLag2Mid[35];
extern const int16_t WebRtcIsacfix_kMeanLag4Mid[19];





extern const uint16_t WebRtcIsacfix_kPitchLagCdf1Hi[511];
extern const uint16_t WebRtcIsacfix_kPitchLagCdf2Hi[68];
extern const uint16_t WebRtcIsacfix_kPitchLagCdf3Hi[2];
extern const uint16_t WebRtcIsacfix_kPitchLagCdf4Hi[35];

extern const uint16_t *WebRtcIsacfix_kPitchLagPtrHi[4];


extern const uint16_t WebRtcIsacfix_kPitchLagSizeHi[1];


extern const int16_t WebRtcIsacfix_kLowerLimitHi[4];
extern const int16_t WebRtcIsacfix_kUpperLimitHi[4];


extern const uint16_t WebRtcIsacfix_kInitIndHi[3];


extern const int16_t WebRtcIsacfix_kMeanLag2Hi[67];
extern const int16_t WebRtcIsacfix_kMeanLag4Hi[34];


#endif 
