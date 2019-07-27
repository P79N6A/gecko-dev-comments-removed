









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_OPUS_OPUS_INST_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_OPUS_OPUS_INST_H_

#include "opus.h"

struct WebRtcOpusEncInst {
  OpusEncoder* encoder;
};

struct WebRtcOpusDecInst {
  OpusDecoder* decoder_left;
  OpusDecoder* decoder_right;
  int prev_decoded_samples;
  int channels;
};


#endif  
