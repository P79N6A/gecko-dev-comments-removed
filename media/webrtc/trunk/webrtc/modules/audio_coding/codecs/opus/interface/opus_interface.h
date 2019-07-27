









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_OPUS_INTERFACE_OPUS_INTERFACE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_OPUS_INTERFACE_OPUS_INTERFACE_H_

#include "webrtc/typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct WebRtcOpusEncInst OpusEncInst;
typedef struct WebRtcOpusDecInst OpusDecInst;

int16_t WebRtcOpus_EncoderCreate(OpusEncInst** inst, int32_t channels);
int16_t WebRtcOpus_EncoderFree(OpusEncInst* inst);



















int16_t WebRtcOpus_Encode(OpusEncInst* inst,
                          const int16_t* audio_in,
                          int16_t samples,
                          int16_t length_encoded_buffer,
                          uint8_t* encoded);













int16_t WebRtcOpus_SetBitRate(OpusEncInst* inst, int32_t rate);












int16_t WebRtcOpus_SetPacketLossRate(OpusEncInst* inst, int32_t loss_rate);
























int16_t WebRtcOpus_SetMaxPlaybackRate(OpusEncInst* inst, int32_t frequency_hz);
















int16_t WebRtcOpus_EnableFec(OpusEncInst* inst);












int16_t WebRtcOpus_DisableFec(OpusEncInst* inst);














int16_t WebRtcOpus_SetComplexity(OpusEncInst* inst, int32_t complexity);

int16_t WebRtcOpus_DecoderCreate(OpusDecInst** inst, int channels);
int16_t WebRtcOpus_DecoderFree(OpusDecInst* inst);






int WebRtcOpus_DecoderChannels(OpusDecInst* inst);












int16_t WebRtcOpus_DecoderInitNew(OpusDecInst* inst);
int16_t WebRtcOpus_DecoderInit(OpusDecInst* inst);
int16_t WebRtcOpus_DecoderInitSlave(OpusDecInst* inst);





















int16_t WebRtcOpus_DecodeNew(OpusDecInst* inst, const uint8_t* encoded,
                             int16_t encoded_bytes, int16_t* decoded,
                             int16_t* audio_type);
int16_t WebRtcOpus_Decode(OpusDecInst* inst, const uint8_t* encoded,
                          int16_t encoded_bytes, int16_t* decoded,
                          int16_t* audio_type);
int16_t WebRtcOpus_DecodeSlave(OpusDecInst* inst, const uint8_t* encoded,
                               int16_t encoded_bytes, int16_t* decoded,
                               int16_t* audio_type);


















int16_t WebRtcOpus_DecodePlc(OpusDecInst* inst, int16_t* decoded,
                             int16_t number_of_lost_frames);
int16_t WebRtcOpus_DecodePlcMaster(OpusDecInst* inst, int16_t* decoded,
                                   int16_t number_of_lost_frames);
int16_t WebRtcOpus_DecodePlcSlave(OpusDecInst* inst, int16_t* decoded,
                                  int16_t number_of_lost_frames);



















int16_t WebRtcOpus_DecodeFec(OpusDecInst* inst, const uint8_t* encoded,
                             int16_t encoded_bytes, int16_t* decoded,
                             int16_t* audio_type);












int WebRtcOpus_DurationEst(OpusDecInst* inst,
                           const uint8_t* payload,
                           int payload_length_bytes);


















int WebRtcOpus_FecDurationEst(const uint8_t* payload,
                              int payload_length_bytes);












int WebRtcOpus_PacketHasFec(const uint8_t* payload,
                            int payload_length_bytes);

#ifdef __cplusplus
}  
#endif

#endif
