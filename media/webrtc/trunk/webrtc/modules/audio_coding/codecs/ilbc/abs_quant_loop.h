

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ABS_QUANT_LOOP_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ABS_QUANT_LOOP_H_

#include "defines.h"






void WebRtcIlbcfix_AbsQuantLoop(
    WebRtc_Word16 *syntOutIN,
    WebRtc_Word16 *in_weightedIN,
    WebRtc_Word16 *weightDenumIN,
    WebRtc_Word16 *quantLenIN,
    WebRtc_Word16 *idxVecIN
                                );

#endif
