









#include "after_streaming_fixture.h"
#include "modules/audio_device/include/audio_device.h"
#include "voe_test_defines.h"

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

#ifdef _WIN32
TEST_F(HardwareTest, GetCpuLoadWorksOnWindows) {
  int load = -1;
  EXPECT_EQ(0, voe_hardware_->GetCPULoad(load));
  EXPECT_GE(0, load);
  TEST_LOG("Voice engine CPU load = %d%%\n", load);
}
#else
TEST_F(HardwareTest, GetCpuLoadReturnsErrorOnNonWindowsPlatform) {
  int load = -1;
  EXPECT_EQ(-1, voe_hardware_->GetCPULoad(load));
}
#endif

#if !defined(WEBRTC_MAC) && !defined(WEBRTC_ANDROID)
TEST_F(HardwareTest, GetSystemCpuLoadWorksExceptOnMacAndAndroid) {
#ifdef _WIN32
  
  
  Sleep(2000);
#endif
  int load = -1;
  EXPECT_EQ(0, voe_hardware_->GetSystemCPULoad(load));
  EXPECT_GE(load, 0);
  TEST_LOG("System CPU load = %d%%\n", load);
}
#endif

TEST_F(HardwareTest, BuiltInWasapiAECWorksForAudioWindowsCoreAudioLayer) {
#ifdef WEBRTC_IOS
  
  EXPECT_EQ(0, voe_hardware_->ResetAudioDevice());
  Sleep(2000);
#endif
  EXPECT_EQ(0, voe_base_->StopSend(channel_));
  EXPECT_EQ(0, voe_base_->StopPlayout(channel_));

  webrtc::AudioLayers given_layer;
  EXPECT_EQ(0, voe_hardware_->GetAudioDeviceLayer(given_layer));
  if (given_layer != webrtc::kAudioWindowsCore) {
    
    EXPECT_EQ(-1, voe_hardware_->EnableBuiltInAEC(true));
    EXPECT_EQ(-1, voe_hardware_->EnableBuiltInAEC(false));
    return;
  }

  TEST_LOG("Testing AEC for Audio Windows Core.\n");
  EXPECT_EQ(0, voe_base_->StartSend(channel_));

  
  EXPECT_EQ(-1, voe_hardware_->EnableBuiltInAEC(true));
  EXPECT_EQ(-1, voe_hardware_->EnableBuiltInAEC(false));

  EXPECT_EQ(0, voe_base_->StopSend(channel_));
  EXPECT_EQ(0, voe_hardware_->EnableBuiltInAEC(true));

  
  EXPECT_EQ(-1, voe_base_->StartSend(channel_));

  EXPECT_EQ(0, voe_base_->StartPlayout(channel_));
  EXPECT_EQ(0, voe_base_->StartSend(channel_));
  TEST_LOG("Processing capture data with built-in AEC...\n");
  Sleep(2000);

  TEST_LOG("Looping through capture devices...\n");
  int num_devs = 0;
  char dev_name[128] = { 0 };
  char guid_name[128] = { 0 };
  EXPECT_EQ(0, voe_hardware_->GetNumOfRecordingDevices(num_devs));
  for (int dev_index = 0; dev_index < num_devs; ++dev_index) {
    EXPECT_EQ(0, voe_hardware_->GetRecordingDeviceName(dev_index,
                                                       dev_name,
                                                       guid_name));
    TEST_LOG("%d: %s\n", dev_index, dev_name);
    EXPECT_EQ(0, voe_hardware_->SetRecordingDevice(dev_index));
    Sleep(2000);
  }

  EXPECT_EQ(0, voe_hardware_->SetPlayoutDevice(-1));
  EXPECT_EQ(0, voe_hardware_->SetRecordingDevice(-1));

  TEST_LOG("Looping through render devices, restarting for each "
      "device...\n");
  EXPECT_EQ(0, voe_hardware_->GetNumOfPlayoutDevices(num_devs));
  for (int dev_index = 0; dev_index < num_devs; ++dev_index) {
    EXPECT_EQ(0, voe_hardware_->GetPlayoutDeviceName(dev_index,
                                                     dev_name,
                                                     guid_name));
    TEST_LOG("%d: %s\n", dev_index, dev_name);
    EXPECT_EQ(0, voe_hardware_->SetPlayoutDevice(dev_index));
    Sleep(2000);
  }

  TEST_LOG("Using default devices...\n");
  EXPECT_EQ(0, voe_hardware_->SetRecordingDevice(-1));
  EXPECT_EQ(0, voe_hardware_->SetPlayoutDevice(-1));
  Sleep(2000);

  
  EXPECT_EQ(0, voe_base_->StopPlayout(channel_));

  EXPECT_EQ(0, voe_base_->StopSend(channel_));
  EXPECT_EQ(0, voe_base_->StopPlayout(channel_));
  Sleep(2000);  

  TEST_LOG("Disabling built-in AEC.\n");
  EXPECT_EQ(0, voe_hardware_->EnableBuiltInAEC(false));

  EXPECT_EQ(0, voe_base_->StartSend(channel_));
  EXPECT_EQ(0, voe_base_->StartPlayout(channel_));
}
