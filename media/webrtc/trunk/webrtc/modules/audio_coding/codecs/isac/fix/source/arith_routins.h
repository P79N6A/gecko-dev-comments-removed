
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_ARITH_ROUTINS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_ARITH_ROUTINS_H_

#include "structs.h"

















int WebRtcIsacfix_EncLogisticMulti2(
    Bitstr_enc *streamData,
    int16_t *dataQ7,
    const uint16_t *env,
    const int16_t lenData);













int16_t WebRtcIsacfix_EncTerminate(Bitstr_enc *streamData);



















int16_t WebRtcIsacfix_DecLogisticMulti2(
    int16_t *data,
    Bitstr_dec *streamData,
    const int32_t *env,
    const int16_t lenData);
















int WebRtcIsacfix_EncHistMulti(
    Bitstr_enc *streamData,
    const int16_t *data,
    const uint16_t **cdf,
    const int16_t lenData);






















int16_t WebRtcIsacfix_DecHistBisectMulti(
    int16_t *data,
    Bitstr_dec *streamData,
    const uint16_t **cdf,
    const uint16_t *cdfSize,
    const int16_t lenData);






















int16_t WebRtcIsacfix_DecHistOneStepMulti(
    int16_t *data,
    Bitstr_dec *streamData,
    const uint16_t **cdf,
    const uint16_t *initIndex,
    const int16_t lenData);

#endif 
