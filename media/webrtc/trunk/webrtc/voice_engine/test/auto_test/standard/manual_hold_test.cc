









#include "after_streaming_fixture.h"


class ManualHoldTest : public AfterStreamingFixture {
};

TEST_F(ManualHoldTest, SetOnHoldStatusBlockAudio) {
  TEST_LOG("Channel not on hold => should hear audio.\n");
  Sleep(2000);
  TEST_LOG("Put channel on hold => should *not* hear audio.\n");
  EXPECT_EQ(0, voe_base_->SetOnHoldStatus(channel_, true));
  Sleep(2000);
  TEST_LOG("Remove on hold => should hear audio again.\n");
  EXPECT_EQ(0, voe_base_->SetOnHoldStatus(channel_, false));
  Sleep(2000);
  TEST_LOG("Put sending on hold => should *not* hear audio.\n");
  EXPECT_EQ(0, voe_base_->SetOnHoldStatus(channel_, true, webrtc::kHoldSendOnly));
  Sleep(2000);
}

TEST_F(ManualHoldTest, SetOnHoldStatusBlocksLocalFileAudio) {
  TEST_LOG("Start playing a file locally => "
      "you should now hear this file being played out.\n");
  voe_file_->StopPlayingFileAsMicrophone(channel_);
  EXPECT_EQ(0, voe_file_->StartPlayingFileLocally(
      channel_, resource_manager_.long_audio_file_path().c_str(), true));
  Sleep(2000);

  TEST_LOG("Put playing on hold => should *not* hear audio.\n");
  EXPECT_EQ(0, voe_base_->SetOnHoldStatus(
      channel_, true, webrtc::kHoldPlayOnly));
  Sleep(2000);
}
