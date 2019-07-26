















#ifndef WEBRTC_SPL_RESAMPLE_BY_2_INTERNAL_H_
#define WEBRTC_SPL_RESAMPLE_BY_2_INTERNAL_H_

#include "typedefs.h"





void WebRtcSpl_DownBy2IntToShort(int32_t *in, int32_t len, int16_t *out,
                                 int32_t *state);

void WebRtcSpl_DownBy2ShortToInt(const int16_t *in, int32_t len,
                                 int32_t *out, int32_t *state);

void WebRtcSpl_UpBy2ShortToInt(const int16_t *in, int32_t len,
                               int32_t *out, int32_t *state);

void WebRtcSpl_UpBy2IntToInt(const int32_t *in, int32_t len, int32_t *out,
                             int32_t *state);

void WebRtcSpl_UpBy2IntToShort(const int32_t *in, int32_t len,
                               int16_t *out, int32_t *state);

void WebRtcSpl_LPBy2ShortToInt(const int16_t* in, int32_t len,
                               int32_t* out, int32_t* state);

void WebRtcSpl_LPBy2IntToInt(const int32_t* in, int32_t len, int32_t* out,
                             int32_t* state);

#endif 
