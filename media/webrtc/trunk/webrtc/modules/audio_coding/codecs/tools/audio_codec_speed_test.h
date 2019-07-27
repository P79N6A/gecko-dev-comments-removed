









#ifndef WEBRTC_MODULES_AUDIO_CODING_CODECS_TOOLS_AUDIO_CODEC_SPEED_TEST_H_
#define WEBRTC_MODULES_AUDIO_CODING_CODECS_TOOLS_AUDIO_CODEC_SPEED_TEST_H_

#include <string>
#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {



typedef std::tr1::tuple<int, int, std::string, std::string, bool> coding_param;

class AudioCodecSpeedTest : public testing::TestWithParam<coding_param> {
 protected:
  AudioCodecSpeedTest(int block_duration_ms,
                      int input_sampling_khz,
                      int output_sampling_khz);
  virtual void SetUp();
  virtual void TearDown();

  
  
  
  
  
  virtual float EncodeABlock(int16_t* in_data, uint8_t* bit_stream,
                             int max_bytes, int* encoded_bytes) = 0;

  
  
  
  
  
  virtual float DecodeABlock(const uint8_t* bit_stream, int encoded_bytes,
                             int16_t* out_data) = 0;

  
  
  void EncodeDecode(size_t audio_duration);

  int block_duration_ms_;
  int input_sampling_khz_;
  int output_sampling_khz_;

  
  int input_length_sample_;

  
  int output_length_sample_;

  scoped_ptr<int16_t[]> in_data_;
  scoped_ptr<int16_t[]> out_data_;
  size_t data_pointer_;
  size_t loop_length_samples_;
  scoped_ptr<uint8_t[]> bit_stream_;

  
  int max_bytes_;

  int encoded_bytes_;
  float encoding_time_ms_;
  float decoding_time_ms_;
  FILE* out_file_;

  int channels_;

  
  int bit_rate_;

  std::string in_filename_;

  
  bool save_out_data_;
};

}  

#endif  
