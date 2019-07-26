









#include "gtest/gtest.h"
#include "webrtc/modules/media_file/interface/media_file.h"
#include "webrtc/system_wrappers/interface/sleep.h"
#include "webrtc/test/testsupport/fileutils.h"

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

TEST_F(MediaFileTest, StartPlayingAudioFileWithoutError) {
  
  
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
