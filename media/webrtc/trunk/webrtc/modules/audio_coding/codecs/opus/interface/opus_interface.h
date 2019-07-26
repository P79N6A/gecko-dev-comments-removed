









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_OPUS_INTERFACE_OPUS_INTERFACE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_OPUS_INTERFACE_OPUS_INTERFACE_H_

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct WebRtcOpusEncInst OpusEncInst;
typedef struct WebRtcOpusDecInst OpusDecInst;

int16_t WebRtcOpus_EncoderCreate(OpusEncInst** inst, int32_t channels);
int16_t WebRtcOpus_EncoderFree(OpusEncInst* inst);



















int16_t WebRtcOpus_Encode(OpusEncInst* inst, int16_t* audio_in, int16_t samples,
                          int16_t length_encoded_buffer, uint8_t* encoded);













int16_t WebRtcOpus_SetBitRate(OpusEncInst* inst, int32_t rate);

int16_t WebRtcOpus_DecoderCreate(OpusDecInst** inst, int channels);
int16_t WebRtcOpus_DecoderFree(OpusDecInst* inst);






int WebRtcOpus_DecoderChannels(OpusDecInst* inst);












int16_t WebRtcOpus_DecoderInit(OpusDecInst* inst);
int16_t WebRtcOpus_DecoderInitSlave(OpusDecInst* inst);





















int16_t WebRtcOpus_Decode(OpusDecInst* inst, int16_t* encoded,
                          int16_t encoded_bytes, int16_t* decoded,
                          int16_t* audio_type);
int16_t WebRtcOpus_DecodeSlave(OpusDecInst* inst, int16_t* encoded,
                               int16_t encoded_bytes, int16_t* decoded,
                               int16_t* audio_type);














int16_t WebRtcOpus_DecodePlc(OpusDecInst* inst, int16_t* decoded,
                             int16_t number_of_lost_frames);












int WebRtcOpus_DurationEst(OpusDecInst* inst,
                           const uint8_t* payload,
                           int payload_length_bytes);

#ifdef __cplusplus
}  
#endif

#endif
