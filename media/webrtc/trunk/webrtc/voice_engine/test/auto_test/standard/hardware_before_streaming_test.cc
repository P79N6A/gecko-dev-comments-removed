









#include <cstring>

#include "after_initialization_fixture.h"

using namespace webrtc;

static const char* kNoDevicesErrorMessage =
    "Either you have no recording / playout device "
    "on your system, or the method failed.";

class HardwareBeforeStreamingTest : public AfterInitializationFixture {
};



TEST_F(HardwareBeforeStreamingTest,
       SetAudioDeviceLayerFailsSinceTheVoiceEngineHasBeenInitialized) {
  EXPECT_NE(0, voe_hardware_->SetAudioDeviceLayer(kAudioPlatformDefault));
  EXPECT_EQ(VE_ALREADY_INITED, voe_base_->LastError());
}

TEST_F(HardwareBeforeStreamingTest,
       GetCPULoadSucceedsOnWindowsButNotOtherPlatforms) {
  int load_percent;
#if defined(_WIN32)
  EXPECT_EQ(0, voe_hardware_->GetCPULoad(load_percent));
#else
  EXPECT_NE(0, voe_hardware_->GetCPULoad(load_percent)) <<
      "Should fail on non-Windows platforms.";
#endif
}



#ifdef WEBRTC_IOS
TEST_F(HardwareBeforeStreamingTest, ResetsAudioDeviceOnIphone) {
  EXPECT_EQ(0, voe_hardware_->ResetAudioDevice());
}
#endif


#if !defined(WEBRTC_IOS) & !defined(WEBRTC_ANDROID)

TEST_F(HardwareBeforeStreamingTest, GetSystemCpuLoadSucceeds) {
#ifdef _WIN32
  
  
  Sleep(2000);
#endif

  int load_percent;
  EXPECT_EQ(0, voe_hardware_->GetSystemCPULoad(load_percent));
}

TEST_F(HardwareBeforeStreamingTest, GetPlayoutDeviceStatusReturnsTrue) {
  bool play_available = false;
  EXPECT_EQ(0, voe_hardware_->GetPlayoutDeviceStatus(play_available));
  ASSERT_TRUE(play_available) <<
      "Ensures that the method works and that hardware is in the right state.";
}

TEST_F(HardwareBeforeStreamingTest, GetRecordingDeviceStatusReturnsTrue) {
  bool recording_available = false;
  EXPECT_EQ(0, voe_hardware_->GetRecordingDeviceStatus(recording_available));
  EXPECT_TRUE(recording_available) <<
      "Ensures that the method works and that hardware is in the right state.";
}

  
TEST_F(HardwareBeforeStreamingTest,
       GetRecordingDeviceNameRetrievesDeviceNames) {
  char device_name[128] = {0};
  char guid_name[128] = {0};

#ifdef _WIN32
  EXPECT_EQ(0, voe_hardware_->GetRecordingDeviceName(
      -1, device_name, guid_name));
  EXPECT_GT(strlen(device_name), 0u) << kNoDevicesErrorMessage;
  device_name[0] = '\0';

  EXPECT_EQ(0, voe_hardware_->GetPlayoutDeviceName(
      -1, device_name, guid_name));
  EXPECT_GT(strlen(device_name), 0u) << kNoDevicesErrorMessage;

#else
  EXPECT_EQ(0, voe_hardware_->GetRecordingDeviceName(
      0, device_name, guid_name));
  EXPECT_GT(strlen(device_name), 0u) << kNoDevicesErrorMessage;
  device_name[0] = '\0';

  EXPECT_EQ(0, voe_hardware_->GetPlayoutDeviceName(
      0, device_name, guid_name));
  EXPECT_GT(strlen(device_name), 0u) << kNoDevicesErrorMessage;
#endif  
}

TEST_F(HardwareBeforeStreamingTest,
       AllEnumeratedRecordingDevicesCanBeSetAsRecordingDevice) {
  
  
  
  
  
  
  int num_of_recording_devices = 0;
  EXPECT_EQ(0, voe_hardware_->GetNumOfRecordingDevices(
      num_of_recording_devices));
  EXPECT_GT(num_of_recording_devices, 0) << kNoDevicesErrorMessage;

  char device_name[128] = {0};
  char guid_name[128] = {0};

  for (int i = 0; i < num_of_recording_devices; i++) {
    EXPECT_EQ(0, voe_hardware_->GetRecordingDeviceName(
        i, device_name, guid_name));
    EXPECT_GT(strlen(device_name), 0u) <<
        "There should be no empty device names "
        "among the ones the system gives us.";
    EXPECT_EQ(0, voe_hardware_->SetRecordingDevice(i));
  }
}

TEST_F(HardwareBeforeStreamingTest,
       AllEnumeratedPlayoutDevicesCanBeSetAsPlayoutDevice) {
  
  int num_of_playout_devices = 0;
  EXPECT_EQ(0, voe_hardware_->GetNumOfPlayoutDevices(
      num_of_playout_devices));
  EXPECT_GT(num_of_playout_devices, 0) << kNoDevicesErrorMessage;

  char device_name[128] = {0};
  char guid_name[128] = {0};

  for (int i = 0; i < num_of_playout_devices; ++i) {
    EXPECT_EQ(0, voe_hardware_->GetPlayoutDeviceName(
        i, device_name, guid_name));
    EXPECT_GT(strlen(device_name), 0u) <<
        "There should be no empty device names "
        "among the ones the system gives us.";
    EXPECT_EQ(0, voe_hardware_->SetPlayoutDevice(i));
  }
}

TEST_F(HardwareBeforeStreamingTest,
       SetDeviceWithMagicalArgumentsSetsDefaultSoundDevices) {
#ifdef _WIN32
  
  EXPECT_EQ(0, voe_hardware_->SetRecordingDevice(-1));
  EXPECT_EQ(0, voe_hardware_->SetPlayoutDevice(-1));
#else
  EXPECT_EQ(0, voe_hardware_->SetRecordingDevice(0));
  EXPECT_EQ(0, voe_hardware_->SetPlayoutDevice(0));
#endif
}

#endif 
