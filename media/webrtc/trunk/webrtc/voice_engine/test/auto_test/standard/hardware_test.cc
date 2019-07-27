









#include "webrtc/modules/audio_device/include/audio_device.h"
#include "webrtc/voice_engine/test/auto_test/fixtures/after_streaming_fixture.h"

class HardwareTest : public AfterStreamingFixture {
};

#if !defined(WEBRTC_IOS) && !defined(WEBRTC_ANDROID)
TEST_F(HardwareTest, AbleToQueryForDevices) {
  int num_recording_devices = 0;
  int num_playout_devices = 0;
  EXPECT_EQ(0, voe_hardware_->GetNumOfRecordingDevices(num_recording_devices));
  EXPECT_EQ(0, voe_hardware_->GetNumOfPlayoutDevices(num_playout_devices));

  ASSERT_GT(num_recording_devices, 0) <<
      "There seem to be no recording devices on your system, "
      "and this test really doesn't make sense then.";
  ASSERT_GT(num_playout_devices, 0) <<
      "There seem to be no playout devices on your system, "
      "and this test really doesn't make sense then.";

  
  
#ifdef _WIN32
  
  EXPECT_EQ(0, voe_hardware_->SetRecordingDevice(
      webrtc::AudioDeviceModule::kDefaultCommunicationDevice));
  
  EXPECT_EQ(0, voe_hardware_->SetPlayoutDevice(
      webrtc::AudioDeviceModule::kDefaultCommunicationDevice));
#else
  
  EXPECT_EQ(0, voe_hardware_->SetRecordingDevice(0));
  EXPECT_EQ(0, voe_hardware_->SetPlayoutDevice(0));
#endif

  
  
  char device_name[128] = {0};
  char guid_name[128] = {0};
  EXPECT_EQ(0, voe_hardware_->GetRecordingDeviceName(
      0, device_name, guid_name));
  EXPECT_EQ(0, voe_hardware_->GetPlayoutDeviceName(
      0, device_name, guid_name));
}
#endif
