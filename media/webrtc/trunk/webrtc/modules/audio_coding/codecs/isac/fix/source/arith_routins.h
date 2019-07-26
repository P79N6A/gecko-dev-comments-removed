
















#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_ARITH_ROUTINS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_ISAC_FIX_SOURCE_ARITH_ROUTINS_H_

#include "structs.h"

















int WebRtcIsacfix_EncLogisticMulti2(
    Bitstr_enc *streamData,
    WebRtc_Word16 *dataQ7,
    const WebRtc_UWord16 *env,
    const WebRtc_Word16 lenData);













WebRtc_Word16 WebRtcIsacfix_EncTerminate(Bitstr_enc *streamData);



















WebRtc_Word16 WebRtcIsacfix_DecLogisticMulti2(
    WebRtc_Word16 *data,
    Bitstr_dec *streamData,
    const WebRtc_Word32 *env,
    const WebRtc_Word16 lenData);
















int WebRtcIsacfix_EncHistMulti(
    Bitstr_enc *streamData,
    const WebRtc_Word16 *data,
    const WebRtc_UWord16 **cdf,
    const WebRtc_Word16 lenData);






















WebRtc_Word16 WebRtcIsacfix_DecHistBisectMulti(
    WebRtc_Word16 *data,
    Bitstr_dec *streamData,
    const WebRtc_UWord16 **cdf,
    const WebRtc_UWord16 *cdfSize,
    const WebRtc_Word16 lenData);






















WebRtc_Word16 WebRtcIsacfix_DecHistOneStepMulti(
    WebRtc_Word16 *data,
    Bitstr_dec *streamData,
    const WebRtc_UWord16 **cdf,
    const WebRtc_UWord16 *initIndex,
    const WebRtc_Word16 lenData);

#endif 
