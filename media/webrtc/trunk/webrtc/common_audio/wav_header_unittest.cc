









#include <limits>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/common_audio/wav_header.h"
#include "webrtc/system_wrappers/interface/compile_assert.h"



TEST(WavHeaderTest, CheckWavParameters) {
  
  EXPECT_TRUE(webrtc::CheckWavParameters(1, 8000, webrtc::kWavFormatPcm, 1, 0));
  EXPECT_FALSE(
      webrtc::CheckWavParameters(0, 8000, webrtc::kWavFormatPcm, 1, 0));
  EXPECT_FALSE(
      webrtc::CheckWavParameters(-1, 8000, webrtc::kWavFormatPcm, 1, 0));
  EXPECT_FALSE(webrtc::CheckWavParameters(1, 0, webrtc::kWavFormatPcm, 1, 0));
  EXPECT_FALSE(webrtc::CheckWavParameters(1, 8000, webrtc::WavFormat(0), 1, 0));
  EXPECT_FALSE(
      webrtc::CheckWavParameters(1, 8000, webrtc::kWavFormatPcm, 0, 0));

  
  EXPECT_TRUE(webrtc::CheckWavParameters(1, 8000, webrtc::kWavFormatPcm, 2, 0));
  EXPECT_FALSE(
      webrtc::CheckWavParameters(1, 8000, webrtc::kWavFormatPcm, 4, 0));
  EXPECT_FALSE(
      webrtc::CheckWavParameters(1, 8000, webrtc::kWavFormatALaw, 2, 0));
  EXPECT_FALSE(
      webrtc::CheckWavParameters(1, 8000, webrtc::kWavFormatMuLaw, 2, 0));

  
  EXPECT_FALSE(webrtc::CheckWavParameters(
      1 << 20, 1 << 20, webrtc::kWavFormatPcm, 1, 0));
  EXPECT_FALSE(webrtc::CheckWavParameters(
      1, 8000, webrtc::kWavFormatPcm, 1, std::numeric_limits<uint32_t>::max()));

  
  EXPECT_FALSE(
      webrtc::CheckWavParameters(3, 8000, webrtc::kWavFormatPcm, 1, 5));
}

TEST(WavHeaderTest, ReadWavHeaderWithErrors) {
  int num_channels = 0;
  int sample_rate = 0;
  webrtc::WavFormat format = webrtc::kWavFormatPcm;
  int bytes_per_sample = 0;
  uint32_t num_samples = 0;

  
  
  
  
  static const uint8_t kBadRiffID[] = {
    'R', 'i', 'f', 'f',  
    0xbd, 0xd0, 0x5b, 0x07,  
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0,  
    6, 0,  
    17, 0,  
    0x39, 0x30, 0, 0,  
    0xc9, 0x33, 0x03, 0,  
    17, 0,  
    8, 0,  
    'd', 'a', 't', 'a',
    0x99, 0xd0, 0x5b, 0x07,  
    0xa4, 0xa4, 0xa4, 0xa4,  
  };
  EXPECT_FALSE(
      webrtc::ReadWavHeader(kBadRiffID, &num_channels, &sample_rate,
                            &format, &bytes_per_sample, &num_samples));

  static const uint8_t kBadBitsPerSample[] = {
    'R', 'I', 'F', 'F',
    0xbd, 0xd0, 0x5b, 0x07,  
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0,  
    6, 0,  
    17, 0,  
    0x39, 0x30, 0, 0,  
    0xc9, 0x33, 0x03, 0,  
    17, 0,  
    1, 0,  
    'd', 'a', 't', 'a',
    0x99, 0xd0, 0x5b, 0x07,  
    0xa4, 0xa4, 0xa4, 0xa4,  
  };
  EXPECT_FALSE(
      webrtc::ReadWavHeader(kBadBitsPerSample, &num_channels, &sample_rate,
                            &format, &bytes_per_sample, &num_samples));

  static const uint8_t kBadByteRate[] = {
    'R', 'I', 'F', 'F',
    0xbd, 0xd0, 0x5b, 0x07,  
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0,  
    6, 0,  
    17, 0,  
    0x39, 0x30, 0, 0,  
    0x00, 0x33, 0x03, 0,  
    17, 0,  
    8, 0,  
    'd', 'a', 't', 'a',
    0x99, 0xd0, 0x5b, 0x07,  
    0xa4, 0xa4, 0xa4, 0xa4,  
  };
  EXPECT_FALSE(
      webrtc::ReadWavHeader(kBadByteRate, &num_channels, &sample_rate,
                            &format, &bytes_per_sample, &num_samples));
}


TEST(WavHeaderTest, WriteAndReadWavHeader) {
  static const int kSize = 4 + webrtc::kWavHeaderSize + 4;
  uint8_t buf[kSize];
  memset(buf, 0xa4, sizeof(buf));
  webrtc::WriteWavHeader(
      buf + 4, 17, 12345, webrtc::kWavFormatALaw, 1, 123457689);
  static const uint8_t kExpectedBuf[] = {
    0xa4, 0xa4, 0xa4, 0xa4,  
    'R', 'I', 'F', 'F',
    0xbd, 0xd0, 0x5b, 0x07,  
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    16, 0, 0, 0,  
    6, 0,  
    17, 0,  
    0x39, 0x30, 0, 0,  
    0xc9, 0x33, 0x03, 0,  
    17, 0,  
    8, 0,  
    'd', 'a', 't', 'a',
    0x99, 0xd0, 0x5b, 0x07,  
    0xa4, 0xa4, 0xa4, 0xa4,  
  };
  COMPILE_ASSERT(sizeof(kExpectedBuf) == kSize, buf_size);
  EXPECT_EQ(0, memcmp(kExpectedBuf, buf, kSize));

  int num_channels = 0;
  int sample_rate = 0;
  webrtc::WavFormat format = webrtc::kWavFormatPcm;
  int bytes_per_sample = 0;
  uint32_t num_samples = 0;
  EXPECT_TRUE(
      webrtc::ReadWavHeader(buf + 4, &num_channels, &sample_rate, &format,
                            &bytes_per_sample, &num_samples));
  EXPECT_EQ(17, num_channels);
  EXPECT_EQ(12345, sample_rate);
  EXPECT_EQ(webrtc::kWavFormatALaw, format);
  EXPECT_EQ(1, bytes_per_sample);
  EXPECT_EQ(123457689u, num_samples);
}
