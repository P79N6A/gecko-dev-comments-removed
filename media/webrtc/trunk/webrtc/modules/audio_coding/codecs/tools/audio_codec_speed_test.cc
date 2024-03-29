









#include "webrtc/modules/audio_coding/codecs/tools/audio_codec_speed_test.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/test/testsupport/fileutils.h"

using ::std::tr1::get;

namespace webrtc {

AudioCodecSpeedTest::AudioCodecSpeedTest(int block_duration_ms,
                                         int input_sampling_khz,
                                         int output_sampling_khz)
    : block_duration_ms_(block_duration_ms),
      input_sampling_khz_(input_sampling_khz),
      output_sampling_khz_(output_sampling_khz),
      input_length_sample_(block_duration_ms_ * input_sampling_khz_),
      output_length_sample_(block_duration_ms_ * output_sampling_khz_),
      data_pointer_(0),
      loop_length_samples_(0),
      max_bytes_(0),
      encoded_bytes_(0),
      encoding_time_ms_(0.0),
      decoding_time_ms_(0.0),
      out_file_(NULL) {
}

void AudioCodecSpeedTest::SetUp() {
  channels_ = get<0>(GetParam());
  bit_rate_ = get<1>(GetParam());
  in_filename_ = test::ResourcePath(get<2>(GetParam()), get<3>(GetParam()));
  save_out_data_ = get<4>(GetParam());

  FILE* fp = fopen(in_filename_.c_str(), "rb");
  assert(fp != NULL);

  
  fseek(fp, 0, SEEK_END);
  loop_length_samples_ = ftell(fp) / sizeof(int16_t);
  rewind(fp);

  
  in_data_.reset(new int16_t[loop_length_samples_ +
      input_length_sample_ * channels_]);

  data_pointer_ = 0;

  
  ASSERT_EQ(fread(&in_data_[0], sizeof(int16_t), loop_length_samples_, fp),
            loop_length_samples_);
  fclose(fp);

  
  
  
  memcpy(&in_data_[loop_length_samples_], &in_data_[0],
         input_length_sample_ * channels_ * sizeof(int16_t));

  max_bytes_ = input_length_sample_ * channels_ * sizeof(int16_t);
  out_data_.reset(new int16_t[output_length_sample_ * channels_]);
  bit_stream_.reset(new uint8_t[max_bytes_]);

  if (save_out_data_) {
    std::string out_filename =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();

    
    size_t found;
    while ((found = out_filename.find('/')) != std::string::npos)
      out_filename.replace(found, 1, "_");

    out_filename = test::OutputPath() + out_filename + ".pcm";

    out_file_ = fopen(out_filename.c_str(), "wb");
    assert(out_file_ != NULL);

    printf("Output to be saved in %s.\n", out_filename.c_str());
  }
}

void AudioCodecSpeedTest::TearDown() {
  if (save_out_data_) {
    fclose(out_file_);
  }
}

void AudioCodecSpeedTest::EncodeDecode(size_t audio_duration_sec) {
  size_t time_now_ms = 0;
  float time_ms;

  printf("Coding %d kHz-sampled %d-channel audio at %d bps ...\n",
         input_sampling_khz_, channels_, bit_rate_);

  while (time_now_ms < audio_duration_sec * 1000) {
    
    time_ms = EncodeABlock(&in_data_[data_pointer_], &bit_stream_[0],
                           max_bytes_, &encoded_bytes_);
    encoding_time_ms_ += time_ms;
    time_ms = DecodeABlock(&bit_stream_[0], encoded_bytes_, &out_data_[0]);
    decoding_time_ms_ += time_ms;
    if (save_out_data_) {
      fwrite(&out_data_[0], sizeof(int16_t),
             output_length_sample_ * channels_, out_file_);
    }
    data_pointer_ = (data_pointer_ + input_length_sample_ * channels_) %
        loop_length_samples_;
    time_now_ms += block_duration_ms_;
  }

  printf("Encoding: %.2f%% real time,\nDecoding: %.2f%% real time.\n",
         (encoding_time_ms_ / audio_duration_sec) / 10.0,
         (decoding_time_ms_ / audio_duration_sec) / 10.0);
}

}  
