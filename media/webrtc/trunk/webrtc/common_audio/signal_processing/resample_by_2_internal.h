















#ifndef WEBRTC_SPL_RESAMPLE_BY_2_INTERNAL_H_
#define WEBRTC_SPL_RESAMPLE_BY_2_INTERNAL_H_

#include "typedefs.h"





void WebRtcSpl_DownBy2IntToShort(WebRtc_Word32 *in, WebRtc_Word32 len, WebRtc_Word16 *out,
                                 WebRtc_Word32 *state);

void WebRtcSpl_DownBy2ShortToInt(const WebRtc_Word16 *in, WebRtc_Word32 len,
                                 WebRtc_Word32 *out, WebRtc_Word32 *state);

void WebRtcSpl_UpBy2ShortToInt(const WebRtc_Word16 *in, WebRtc_Word32 len,
                               WebRtc_Word32 *out, WebRtc_Word32 *state);

void WebRtcSpl_UpBy2IntToInt(const WebRtc_Word32 *in, WebRtc_Word32 len, WebRtc_Word32 *out,
                             WebRtc_Word32 *state);

void WebRtcSpl_UpBy2IntToShort(const WebRtc_Word32 *in, WebRtc_Word32 len,
                               WebRtc_Word16 *out, WebRtc_Word32 *state);

void WebRtcSpl_LPBy2ShortToInt(const WebRtc_Word16* in, WebRtc_Word32 len,
                               WebRtc_Word32* out, WebRtc_Word32* state);

void WebRtcSpl_LPBy2IntToInt(const WebRtc_Word32* in, WebRtc_Word32 len, WebRtc_Word32* out,
                             WebRtc_Word32* state);

#endif 
