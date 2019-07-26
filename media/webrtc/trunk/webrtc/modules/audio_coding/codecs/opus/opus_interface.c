









#include "modules/audio_coding/codecs/opus/interface/opus_interface.h"

#include <stdlib.h>
#include <string.h>

#include "opus.h"

#include "common_audio/signal_processing/resample_by_2_internal.h"
#include "common_audio/signal_processing/include/signal_processing_library.h"

enum {
  
  kWebRtcOpusMaxEncodeFrameSizeMs = 20,

  




  kWebRtcOpusMaxDecodeFrameSizeMs = 120,

  
  kWebRtcOpusMaxFrameSize = 48 * kWebRtcOpusMaxDecodeFrameSizeMs * 2,
};

struct WebRtcOpusEncInst {
  OpusEncoder* encoder;
};

int16_t WebRtcOpus_EncoderCreate(OpusEncInst** inst, int32_t channels) {
  OpusEncInst* state;
  state = (OpusEncInst*) calloc(1, sizeof(OpusEncInst));
  if (state) {
    int error;
    
    int application = (channels == 1) ?
        OPUS_APPLICATION_VOIP : OPUS_APPLICATION_AUDIO;

    state->encoder = opus_encoder_create(48000, channels, application, &error);
    if (error == OPUS_OK || state->encoder != NULL ) {
      *inst = state;
      return 0;
    }
    free(state);
  }
  return -1;
}

int16_t WebRtcOpus_EncoderFree(OpusEncInst* inst) {
  opus_encoder_destroy(inst->encoder);
  free(inst);
  return 0;
}

int16_t WebRtcOpus_Encode(OpusEncInst* inst, int16_t* audio_in, int16_t samples,
                          int16_t length_encoded_buffer, uint8_t* encoded) {
  opus_int16* audio = (opus_int16*) audio_in;
  unsigned char* coded = encoded;
  int res;

  if (samples > 48 * kWebRtcOpusMaxEncodeFrameSizeMs) {
    return -1;
  }

  res = opus_encode(inst->encoder, audio, samples, coded,
                    length_encoded_buffer);

  if (res > 0) {
    return res;
  }
  return -1;
}

int16_t WebRtcOpus_SetBitRate(OpusEncInst* inst, int32_t rate) {
  return opus_encoder_ctl(inst->encoder, OPUS_SET_BITRATE(rate));
}

struct WebRtcOpusDecInst {
  int16_t state_48_32_left[8];
  int16_t state_48_32_right[8];
  OpusDecoder* decoder_left;
  OpusDecoder* decoder_right;
  int channels;
};

int16_t WebRtcOpus_DecoderCreate(OpusDecInst** inst, int channels) {
  int error_l;
  int error_r;
  OpusDecInst* state;

  
  state = (OpusDecInst*) calloc(1, sizeof(OpusDecInst));
  if (state == NULL) {
    return -1;
  }

  
  state->decoder_left = opus_decoder_create(48000, channels, &error_l);
  state->decoder_right = opus_decoder_create(48000, channels, &error_r);
  if (error_l == OPUS_OK && error_r == OPUS_OK && state->decoder_left != NULL
      && state->decoder_right != NULL) {
    
    state->channels = channels;
    *inst = state;
    return 0;
  }

  
  if (state->decoder_left) {
    opus_decoder_destroy(state->decoder_left);
  }
  if (state->decoder_right) {
    opus_decoder_destroy(state->decoder_right);
  }
  free(state);
  state = NULL;
  return -1;
}

int16_t WebRtcOpus_DecoderFree(OpusDecInst* inst) {
  opus_decoder_destroy(inst->decoder_left);
  opus_decoder_destroy(inst->decoder_right);
  free(inst);
  return 0;
}

int WebRtcOpus_DecoderChannels(OpusDecInst* inst) {
  return inst->channels;
}

int16_t WebRtcOpus_DecoderInit(OpusDecInst* inst) {
  int error = opus_decoder_ctl(inst->decoder_left, OPUS_RESET_STATE);
  if (error == OPUS_OK) {
    memset(inst->state_48_32_left, 0, sizeof(inst->state_48_32_left));
    return 0;
  }
  return -1;
}

int16_t WebRtcOpus_DecoderInitSlave(OpusDecInst* inst) {
  int error = opus_decoder_ctl(inst->decoder_right, OPUS_RESET_STATE);
  if (error == OPUS_OK) {
    memset(inst->state_48_32_right, 0, sizeof(inst->state_48_32_right));
    return 0;
  }
  return -1;
}

static int DecodeNative(OpusDecoder* inst, int16_t* encoded,
                        int16_t encoded_bytes, int16_t* decoded,
                        int16_t* audio_type) {
  unsigned char* coded = (unsigned char*) encoded;
  opus_int16* audio = (opus_int16*) decoded;

  int res = opus_decode(inst, coded, encoded_bytes, audio,
                        kWebRtcOpusMaxFrameSize, 0);
  
  *audio_type = 0;

  if (res > 0) {
    return res;
  }
  return -1;
}

int16_t WebRtcOpus_Decode(OpusDecInst* inst, int16_t* encoded,
                          int16_t encoded_bytes, int16_t* decoded,
                          int16_t* audio_type) {
  


  int16_t buffer16[kWebRtcOpusMaxFrameSize];
  int32_t buffer32[kWebRtcOpusMaxFrameSize + 7];
  int decoded_samples;
  int blocks;
  int16_t output_samples;
  int i;

  





  
  decoded_samples = DecodeNative(inst->decoder_left, encoded, encoded_bytes,
                                 buffer16, audio_type);
  if (decoded_samples < 0) {
    return -1;
  }
  if (inst->channels == 2) {
    


    for (i = 0; i < decoded_samples; i++) {
      

      buffer16[i] = buffer16[i * 2];
    }
  }
  
  for (i = 0; i < 7; i++) {
    buffer32[i] = inst->state_48_32_left[i];
    inst->state_48_32_left[i] = buffer16[decoded_samples - 7 + i];
  }
  for (i = 0; i < decoded_samples; i++) {
    buffer32[7 + i] = buffer16[i];
  }
  

  blocks = decoded_samples / 3;
  WebRtcSpl_Resample48khzTo32khz(buffer32, buffer32, blocks);
  output_samples = (int16_t) (blocks * 2);
  WebRtcSpl_VectorBitShiftW32ToW16(decoded, output_samples, buffer32, 15);

  return output_samples;
}

int16_t WebRtcOpus_DecodeSlave(OpusDecInst* inst, int16_t* encoded,
                               int16_t encoded_bytes, int16_t* decoded,
                               int16_t* audio_type) {
  


  int16_t buffer16[kWebRtcOpusMaxFrameSize];
  int32_t buffer32[kWebRtcOpusMaxFrameSize + 7];
  int decoded_samples;
  int blocks;
  int16_t output_samples;
  int i;

  
  decoded_samples = DecodeNative(inst->decoder_right, encoded, encoded_bytes,
                                 buffer16, audio_type);
  if (decoded_samples < 0) {
    return -1;
  }
  if (inst->channels == 2) {
    


    for (i = 0; i < decoded_samples; i++) {
      

      buffer16[i] = buffer16[i * 2 + 1];
    }
  } else {
    
    return -1;
  }
  
  for (i = 0; i < 7; i++) {
    buffer32[i] = inst->state_48_32_right[i];
    inst->state_48_32_right[i] = buffer16[decoded_samples - 7 + i];
  }
  for (i = 0; i < decoded_samples; i++) {
    buffer32[7 + i] = buffer16[i];
  }
  

  blocks = decoded_samples / 3;
  WebRtcSpl_Resample48khzTo32khz(buffer32, buffer32, blocks);
  output_samples = (int16_t) (blocks * 2);
  WebRtcSpl_VectorBitShiftW32ToW16(decoded, output_samples, buffer32, 15);

  return output_samples;
}

int16_t WebRtcOpus_DecodePlc(OpusDecInst* inst, int16_t* decoded,
                             int16_t number_of_lost_frames) {
  


  return -1;
}

int WebRtcOpus_DurationEst(OpusDecInst* inst,
                           const uint8_t* payload,
                           int payload_length_bytes)
{
  int frames, samples;
  frames = opus_packet_get_nb_frames(payload, payload_length_bytes);
  if (frames < 0) {
    
    return 0;
  }
  samples = frames * opus_packet_get_samples_per_frame(payload, 48000);
  if (samples < 120 || samples > 5760) {
    
    return 0;
  }
  return samples;
}
