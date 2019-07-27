









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_OPUS_INTERFACE_AUDIO_ENCODER_OPUS_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_OPUS_INTERFACE_AUDIO_ENCODER_OPUS_H_

#include <vector>

#include "webrtc/modules/audio_coding/codecs/opus/interface/opus_interface.h"
#include "webrtc/modules/audio_coding/codecs/audio_encoder.h"

namespace webrtc {

class AudioEncoderOpus : public AudioEncoder {
 public:
  struct Config {
    Config();
    bool IsOk() const;
    int frame_size_ms;
    int num_channels;
  };

  explicit AudioEncoderOpus(const Config& config);
  virtual ~AudioEncoderOpus() OVERRIDE;

  virtual int sample_rate_hz() const OVERRIDE;
  virtual int num_channels() const OVERRIDE;
  virtual int Num10MsFramesInNextPacket() const OVERRIDE;

 protected:
  virtual bool Encode(uint32_t timestamp,
                      const int16_t* audio,
                      size_t max_encoded_bytes,
                      uint8_t* encoded,
                      size_t* encoded_bytes,
                      uint32_t* encoded_timestamp) OVERRIDE;

 private:
  const int num_10ms_frames_per_packet_;
  const int num_channels_;
  const int samples_per_10ms_frame_;
  std::vector<int16_t> input_buffer_;
  OpusEncInst* inst_;
  uint32_t first_timestamp_in_buffer_;
};

}  
#endif  
