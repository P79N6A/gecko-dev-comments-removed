










#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_CNG_MAIN_SOURCE_CNG_HELPFUNS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_CNG_MAIN_SOURCE_CNG_HELPFUNS_H_

extern WebRtc_Word32 lpc_lagwinTbl_fixw32[WEBRTC_CNG_MAX_LPC_ORDER + 1];

#ifdef __cplusplus
extern "C" {
#endif


void WebRtcCng_K2a16(WebRtc_Word16 *k, int useOrder, WebRtc_Word16 *a);

#ifdef __cplusplus
}
#endif

#endif
