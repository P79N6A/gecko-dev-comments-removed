









#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/voice_engine/include/voe_external_media.h"
#include "webrtc/voice_engine/test/auto_test/fakes/fake_media_process.h"
#include "webrtc/voice_engine/test/auto_test/fixtures/after_streaming_fixture.h"

class ExternalMediaTest : public AfterStreamingFixture {
 protected:
  void TestRegisterExternalMedia(int channel, webrtc::ProcessingTypes type) {
    FakeMediaProcess fake_media_process;
    EXPECT_EQ(0, voe_xmedia_->RegisterExternalMediaProcessing(
        channel, type, fake_media_process));
    Sleep(2000);

    TEST_LOG("Back to normal.\n");
    EXPECT_EQ(0, voe_xmedia_->DeRegisterExternalMediaProcessing(
        channel, type));
    Sleep(2000);
  }
};

TEST_F(ExternalMediaTest, ManualCanRecordAndPlaybackUsingExternalPlayout) {
  SwitchToManualMicrophone();

  EXPECT_EQ(0, voe_base_->StopSend(channel_));
  EXPECT_EQ(0, voe_base_->StopPlayout(channel_));
  EXPECT_EQ(0, voe_xmedia_->SetExternalPlayoutStatus(true));
  EXPECT_EQ(0, voe_base_->StartPlayout(channel_));
  EXPECT_EQ(0, voe_base_->StartSend(channel_));

  TEST_LOG("Recording data for 2 seconds starting now: please speak.\n");
  int16_t recording[32000];
  for (int i = 0; i < 200; i++) {
    int sample_length = 0;
    EXPECT_EQ(0, voe_xmedia_->ExternalPlayoutGetData(
        &(recording[i * 160]), 16000, 100, sample_length));
    EXPECT_EQ(160, sample_length);
    Sleep(10);
  }

  EXPECT_EQ(0, voe_base_->StopSend(channel_));
  EXPECT_EQ(0, voe_base_->StopPlayout(channel_));
  EXPECT_EQ(0, voe_xmedia_->SetExternalPlayoutStatus(false));
  EXPECT_EQ(0, voe_base_->StartPlayout(channel_));
  EXPECT_EQ(0, voe_xmedia_->SetExternalRecordingStatus(true));
  EXPECT_EQ(0, voe_base_->StartSend(channel_));

  TEST_LOG("Playing back recording, you should hear what you said earlier.\n");
  for (int i = 0; i < 200; i++) {
    EXPECT_EQ(0, voe_xmedia_->ExternalRecordingInsertData(
        &(recording[i * 160]), 160, 16000, 20));
    Sleep(10);
  }

  EXPECT_EQ(0, voe_base_->StopSend(channel_));
  EXPECT_EQ(0, voe_xmedia_->SetExternalRecordingStatus(false));
}

TEST_F(ExternalMediaTest,
    ManualRegisterExternalMediaProcessingOnAllChannelsAffectsPlayout) {
  TEST_LOG("Enabling external media processing: audio should be affected.\n");
  TestRegisterExternalMedia(-1, webrtc::kPlaybackAllChannelsMixed);
}

TEST_F(ExternalMediaTest,
    ManualRegisterExternalMediaOnSingleChannelAffectsPlayout) {
  TEST_LOG("Enabling external media processing: audio should be affected.\n");
  TestRegisterExternalMedia(channel_, webrtc::kRecordingPerChannel);
}

TEST_F(ExternalMediaTest,
    ManualRegisterExternalMediaOnAllChannelsMixedAffectsRecording) {
  SwitchToManualMicrophone();
  TEST_LOG("Speak and verify your voice is distorted.\n");
  TestRegisterExternalMedia(-1, webrtc::kRecordingAllChannelsMixed);
}

TEST_F(ExternalMediaTest,
       ExternalMixingCannotBeChangedDuringPlayback) {
  EXPECT_EQ(-1, voe_xmedia_->SetExternalMixing(channel_, true));
  EXPECT_EQ(-1, voe_xmedia_->SetExternalMixing(channel_, false));
}

TEST_F(ExternalMediaTest,
       ExternalMixingIsRequiredForGetAudioFrame) {
  webrtc::AudioFrame frame;
  EXPECT_EQ(-1, voe_xmedia_->GetAudioFrame(channel_, 0, &frame));
}

TEST_F(ExternalMediaTest,
       ExternalMixingPreventsAndRestoresRegularPlayback) {
  PausePlaying();
  ASSERT_EQ(0, voe_xmedia_->SetExternalMixing(channel_, true));
  TEST_LOG("Verify that no sound is played out.\n");
  ResumePlaying();
  Sleep(1000);
  PausePlaying();
  ASSERT_EQ(0, voe_xmedia_->SetExternalMixing(channel_, false));
  ResumePlaying();
  TEST_LOG("Verify that sound is played out.\n");
  ResumePlaying();
  Sleep(1000);
}

TEST_F(ExternalMediaTest,
       ExternalMixingWorks) {
  webrtc::AudioFrame frame;
  PausePlaying();
  EXPECT_EQ(0, voe_xmedia_->SetExternalMixing(channel_, true));
  ResumePlaying();
  EXPECT_EQ(0, voe_xmedia_->GetAudioFrame(channel_, 0, &frame));
  EXPECT_LT(0, frame.sample_rate_hz_);
  EXPECT_LT(0, frame.samples_per_channel_);
  PausePlaying();
  EXPECT_EQ(0, voe_xmedia_->SetExternalMixing(channel_, false));
  ResumePlaying();
}

TEST_F(ExternalMediaTest,
       ExternalMixingResamplesToDesiredFrequency) {
  const int kValidFrequencies[] = {8000, 16000, 22000, 32000, 48000};
  webrtc::AudioFrame frame;
  PausePlaying();
  EXPECT_EQ(0, voe_xmedia_->SetExternalMixing(channel_, true));
  ResumePlaying();
  for (size_t i = 0; i < sizeof(kValidFrequencies) / sizeof(int); i++) {
    int f = kValidFrequencies[i];
    EXPECT_EQ(0, voe_xmedia_->GetAudioFrame(channel_, f, &frame))
       << "Resampling succeeds for freq=" << f;
    EXPECT_EQ(f, frame.sample_rate_hz_);
    EXPECT_EQ(f / 100, frame.samples_per_channel_);
  }
  PausePlaying();
  EXPECT_EQ(0, voe_xmedia_->SetExternalMixing(channel_, false));
  ResumePlaying();
}

TEST_F(ExternalMediaTest,
       ExternalMixingResamplingToInvalidFrequenciesFails) {
  const int kInvalidFrequencies[] = {-8000, -1};
  webrtc::AudioFrame frame;
  PausePlaying();
  EXPECT_EQ(0, voe_xmedia_->SetExternalMixing(channel_, true));
  ResumePlaying();
  for (size_t i = 0; i < sizeof(kInvalidFrequencies) / sizeof(int); i++) {
    int f = kInvalidFrequencies[i];
    EXPECT_EQ(-1, voe_xmedia_->GetAudioFrame(channel_, f, &frame))
        << "Resampling fails for freq=" << f;
  }
  PausePlaying();
  EXPECT_EQ(0, voe_xmedia_->SetExternalMixing(channel_, false));
  ResumePlaying();
}
