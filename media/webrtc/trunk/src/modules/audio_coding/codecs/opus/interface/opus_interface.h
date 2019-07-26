









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_OPUS_INTERFACE_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_OPUS_INTERFACE_H_

#include <opus.h>

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct WebRTCOpusEncInst OpusEncInst;
typedef struct WebRTCOpusDecInst OpusDecInst;

int16_t WebRtcOpus_EncoderCreate(OpusEncInst **inst, int32_t channels);
int16_t WebRtcOpus_EncoderFree(OpusEncInst *inst);




















int16_t WebRtcOpus_Encode(OpusEncInst   *inst,
                          int16_t       *audioIn,
                          uint8_t       *encoded,
                          int16_t        encodedLenByte,
                          int16_t        len);













int16_t WebRtcOpus_SetBitRate(OpusEncInst *inst, int32_t rate);

int16_t WebRtcOpus_DecoderCreate(OpusDecInst **inst,
                                 int32_t       fs,
                                 int32_t       channels);
int16_t WebRtcOpus_DecoderFree(OpusDecInst *inst);












int16_t WebRtcOpus_DecoderInit(OpusDecInst* inst);





















int16_t WebRtcOpus_DecodeNative(OpusDecInst *inst,
                                int16_t     *encoded,
                                int16_t      len,
                                int16_t     *decoded,
                                int16_t     *audioType);





















int16_t WebRtcOpus_Decode(OpusDecInst   *inst,
                          int16_t       *encoded,
                          int16_t        len,
                          int16_t       *decoded,
                          int16_t       *audioType);















int16_t WebRtcOpus_DecodePlc(OpusDecInst   *inst,
                             int16_t       *decoded,
                             int16_t        noOfLostFrames);

















int16_t WebRtcOpus_Version(char *version, int16_t lenBytes);

#ifdef __cplusplus
} 
#endif

#endif
