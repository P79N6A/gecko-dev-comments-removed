

















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ABS_QUANT_LOOP_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ILBC_MAIN_SOURCE_ABS_QUANT_LOOP_H_

#include "defines.h"






void WebRtcIlbcfix_AbsQuantLoop(int16_t *syntOutIN, int16_t *in_weightedIN,
                                int16_t *weightDenumIN, int16_t *quantLenIN,
                                int16_t *idxVecIN);

#endif
