









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_AUDIO_ENCODER_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_AUDIO_ENCODER_H_

#include <algorithm>

#include "webrtc/base/checks.h"
#include "webrtc/typedefs.h"

namespace webrtc {



class AudioEncoder {
 public:
  virtual ~AudioEncoder() {}

  
  
  
  
  
  
  
  bool Encode(uint32_t timestamp,
              const int16_t* audio,
              size_t num_samples_per_channel,
              size_t max_encoded_bytes,
              uint8_t* encoded,
              size_t* encoded_bytes,
              uint32_t* encoded_timestamp) {
    CHECK_EQ(num_samples_per_channel,
             static_cast<size_t>(sample_rate_hz() / 100));
    bool ret = Encode(timestamp,
                      audio,
                      max_encoded_bytes,
                      encoded,
                      encoded_bytes,
                      encoded_timestamp);
    CHECK_LE(*encoded_bytes, max_encoded_bytes);
    return ret;
  }

  
  
  virtual int sample_rate_hz() const = 0;
  virtual int num_channels() const = 0;

  
  
  
  
  
  virtual int Num10MsFramesInNextPacket() const = 0;

 protected:
  virtual bool Encode(uint32_t timestamp,
                      const int16_t* audio,
                      size_t max_encoded_bytes,
                      uint8_t* encoded,
                      size_t* encoded_bytes,
                      uint32_t* encoded_timestamp) = 0;
};

}  
#endif  
