









#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/media_file/interface/media_file.h"
#include "webrtc/system_wrappers/interface/compile_assert.h"
#include "webrtc/system_wrappers/interface/sleep.h"
#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/testsupport/gtest_disable.h"

class MediaFileTest : public testing::Test {
 protected:
  void SetUp() {
    
    media_file_ = webrtc::MediaFile::CreateMediaFile(0);
    ASSERT_TRUE(media_file_ != NULL);
  }
  void TearDown() {
    webrtc::MediaFile::DestroyMediaFile(media_file_);
    media_file_ = NULL;
  }
  webrtc::MediaFile* media_file_;
};

TEST_F(MediaFileTest, DISABLED_ON_ANDROID(StartPlayingAudioFileWithoutError)) {
  
  
  const std::string audio_file = webrtc::test::ProjectRootPath() +
      "data/voice_engine/audio_tiny48.wav";
  ASSERT_EQ(0, media_file_->StartPlayingAudioFile(
      audio_file.c_str(),
      0,
      false,
      webrtc::kFileFormatWavFile));

  ASSERT_EQ(true, media_file_->IsPlaying());

  webrtc::SleepMs(1);

  ASSERT_EQ(0, media_file_->StopPlaying());
}

TEST_F(MediaFileTest, WriteWavFile) {
  
  static const int kHeaderSize = 44;
  static const int kPayloadSize = 320;
  webrtc::CodecInst codec = {0, "L16", 16000, kPayloadSize, 1};
  std::string outfile = webrtc::test::OutputPath() + "wavtest.wav";
  ASSERT_EQ(0,
            media_file_->StartRecordingAudioFile(
                outfile.c_str(), webrtc::kFileFormatWavFile, codec));
  static const int8_t kFakeData[kPayloadSize] = {0};
  ASSERT_EQ(0, media_file_->IncomingAudioData(kFakeData, kPayloadSize));
  ASSERT_EQ(0, media_file_->StopRecording());

  
  static const uint8_t kExpectedHeader[] = {
    'R', 'I', 'F', 'F',
    0x64, 0x1, 0, 0,  
    'W', 'A', 'V', 'E',
    'f', 'm', 't', ' ',
    0x10, 0, 0, 0,  
    0x1, 0,  
    0x1, 0,  
    0x80, 0x3e, 0, 0,  
    0, 0x7d, 0, 0,  
    0x2, 0,  
    0x10, 0,  
    'd', 'a', 't', 'a',
    0x40, 0x1, 0, 0,  
  };
  COMPILE_ASSERT(sizeof(kExpectedHeader) == kHeaderSize, header_size);

  EXPECT_EQ(size_t(kHeaderSize + kPayloadSize),
            webrtc::test::GetFileSize(outfile));
  FILE* f = fopen(outfile.c_str(), "rb");
  ASSERT_TRUE(f);

  uint8_t header[kHeaderSize];
  ASSERT_EQ(1u, fread(header, kHeaderSize, 1, f));
  EXPECT_EQ(0, memcmp(kExpectedHeader, header, kHeaderSize));

  uint8_t payload[kPayloadSize];
  ASSERT_EQ(1u, fread(payload, kPayloadSize, 1, f));
  EXPECT_EQ(0, memcmp(kFakeData, payload, kPayloadSize));

  EXPECT_EQ(0, fclose(f));
}
