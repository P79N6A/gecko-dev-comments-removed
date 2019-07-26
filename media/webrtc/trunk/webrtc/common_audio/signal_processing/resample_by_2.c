
















#include "signal_processing_library.h"

#ifdef WEBRTC_ARCH_ARM_V7


static const WebRtc_UWord32 kResampleAllpass1[3] = {3284, 24441, 49528 << 15};
static const WebRtc_UWord32 kResampleAllpass2[3] =
  {12199, 37471 << 15, 60255 << 15};




static __inline WebRtc_Word32 MUL_ACCUM_1(WebRtc_Word32 tbl_value,
                                          WebRtc_Word32 diff,
                                          WebRtc_Word32 state) {
  WebRtc_Word32 result;
  __asm __volatile ("smlawb %0, %1, %2, %3": "=r"(result): "r"(diff),
                                   "r"(tbl_value), "r"(state));
  return result;
}








static __inline WebRtc_Word32 MUL_ACCUM_2(WebRtc_Word32 tbl_value,
                                          WebRtc_Word32 diff,
                                          WebRtc_Word32 state) {
  WebRtc_Word32 result;
  __asm __volatile ("smmla %0, %1, %2, %3": "=r"(result): "r"(diff << 1),
                                  "r"(tbl_value), "r"(state));
  return result;
}

#else


static const WebRtc_UWord16 kResampleAllpass1[3] = {3284, 24441, 49528};
static const WebRtc_UWord16 kResampleAllpass2[3] = {12199, 37471, 60255};


#define MUL_ACCUM_1(a, b, c) WEBRTC_SPL_SCALEDIFF32(a, b, c)
#define MUL_ACCUM_2(a, b, c) WEBRTC_SPL_SCALEDIFF32(a, b, c)

#endif  



void WebRtcSpl_DownsampleBy2(const WebRtc_Word16* in, const WebRtc_Word16 len,
                             WebRtc_Word16* out, WebRtc_Word32* filtState) {
  WebRtc_Word32 tmp1, tmp2, diff, in32, out32;
  WebRtc_Word16 i;

  register WebRtc_Word32 state0 = filtState[0];
  register WebRtc_Word32 state1 = filtState[1];
  register WebRtc_Word32 state2 = filtState[2];
  register WebRtc_Word32 state3 = filtState[3];
  register WebRtc_Word32 state4 = filtState[4];
  register WebRtc_Word32 state5 = filtState[5];
  register WebRtc_Word32 state6 = filtState[6];
  register WebRtc_Word32 state7 = filtState[7];

  for (i = (len >> 1); i > 0; i--) {
    
    in32 = (WebRtc_Word32)(*in++) << 10;
    diff = in32 - state1;
    tmp1 = MUL_ACCUM_1(kResampleAllpass2[0], diff, state0);
    state0 = in32;
    diff = tmp1 - state2;
    tmp2 = MUL_ACCUM_2(kResampleAllpass2[1], diff, state1);
    state1 = tmp1;
    diff = tmp2 - state3;
    state3 = MUL_ACCUM_2(kResampleAllpass2[2], diff, state2);
    state2 = tmp2;

    
    in32 = (WebRtc_Word32)(*in++) << 10;
    diff = in32 - state5;
    tmp1 = MUL_ACCUM_1(kResampleAllpass1[0], diff, state4);
    state4 = in32;
    diff = tmp1 - state6;
    tmp2 = MUL_ACCUM_1(kResampleAllpass1[1], diff, state5);
    state5 = tmp1;
    diff = tmp2 - state7;
    state7 = MUL_ACCUM_2(kResampleAllpass1[2], diff, state6);
    state6 = tmp2;

    
    out32 = (state3 + state7 + 1024) >> 11;

    
    *out++ = WebRtcSpl_SatW32ToW16(out32);
  }

  filtState[0] = state0;
  filtState[1] = state1;
  filtState[2] = state2;
  filtState[3] = state3;
  filtState[4] = state4;
  filtState[5] = state5;
  filtState[6] = state6;
  filtState[7] = state7;
}


void WebRtcSpl_UpsampleBy2(const WebRtc_Word16* in, WebRtc_Word16 len,
                           WebRtc_Word16* out, WebRtc_Word32* filtState) {
  WebRtc_Word32 tmp1, tmp2, diff, in32, out32;
  WebRtc_Word16 i;

  register WebRtc_Word32 state0 = filtState[0];
  register WebRtc_Word32 state1 = filtState[1];
  register WebRtc_Word32 state2 = filtState[2];
  register WebRtc_Word32 state3 = filtState[3];
  register WebRtc_Word32 state4 = filtState[4];
  register WebRtc_Word32 state5 = filtState[5];
  register WebRtc_Word32 state6 = filtState[6];
  register WebRtc_Word32 state7 = filtState[7];

  for (i = len; i > 0; i--) {
    
    in32 = (WebRtc_Word32)(*in++) << 10;
    diff = in32 - state1;
    tmp1 = MUL_ACCUM_1(kResampleAllpass1[0], diff, state0);
    state0 = in32;
    diff = tmp1 - state2;
    tmp2 = MUL_ACCUM_1(kResampleAllpass1[1], diff, state1);
    state1 = tmp1;
    diff = tmp2 - state3;
    state3 = MUL_ACCUM_2(kResampleAllpass1[2], diff, state2);
    state2 = tmp2;

    
    out32 = (state3 + 512) >> 10;
    *out++ = WebRtcSpl_SatW32ToW16(out32);

    
    diff = in32 - state5;
    tmp1 = MUL_ACCUM_1(kResampleAllpass2[0], diff, state4);
    state4 = in32;
    diff = tmp1 - state6;
    tmp2 = MUL_ACCUM_2(kResampleAllpass2[1], diff, state5);
    state5 = tmp1;
    diff = tmp2 - state7;
    state7 = MUL_ACCUM_2(kResampleAllpass2[2], diff, state6);
    state6 = tmp2;

    
    out32 = (state7 + 512) >> 10;
    *out++ = WebRtcSpl_SatW32ToW16(out32);
  }

  filtState[0] = state0;
  filtState[1] = state1;
  filtState[2] = state2;
  filtState[3] = state3;
  filtState[4] = state4;
  filtState[5] = state5;
  filtState[6] = state6;
  filtState[7] = state7;
}
