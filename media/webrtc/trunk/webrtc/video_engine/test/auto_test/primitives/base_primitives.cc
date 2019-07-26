









#include "webrtc/video_engine/test/auto_test/primitives/base_primitives.h"

#include "webrtc/modules/video_capture/include/video_capture_factory.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest_defines.h"
#include "webrtc/video_engine/test/libvietest/include/tb_external_transport.h"

static void ConfigureCodecsToI420(int video_channel,
                                  webrtc::VideoCodec video_codec,
                                  webrtc::ViECodec* codec_interface) {
  
  
  for (int i = 0; i < codec_interface->NumberOfCodecs(); i++) {
    EXPECT_EQ(0, codec_interface->GetCodec(i, video_codec));

    
    if (video_codec.codecType == webrtc::kVideoCodecI420) {
      video_codec.width = 176;
      video_codec.height = 144;
      video_codec.maxBitrate = 32000;
      video_codec.startBitrate = 32000;
      EXPECT_EQ(0, codec_interface->SetSendCodec(video_channel, video_codec));
    }

    EXPECT_EQ(0, codec_interface->SetReceiveCodec(video_channel, video_codec));
  }
  
  EXPECT_EQ(0, codec_interface->GetSendCodec(video_channel, video_codec));
  EXPECT_EQ(webrtc::kVideoCodecI420, video_codec.codecType);
}

void TestI420CallSetup(webrtc::ViECodec* codec_interface,
                       webrtc::VideoEngine* video_engine,
                       webrtc::ViEBase* base_interface,
                       webrtc::ViENetwork* network_interface,
                       int video_channel,
                       const char* device_name) {
  webrtc::VideoCodec video_codec;
  memset(&video_codec, 0, sizeof(webrtc::VideoCodec));

  ConfigureCodecsToI420(video_channel, video_codec, codec_interface);

  TbExternalTransport external_transport(
      *network_interface, video_channel, NULL);
  EXPECT_EQ(0, network_interface->RegisterSendTransport(
      video_channel, external_transport));
  EXPECT_EQ(0, base_interface->StartReceive(video_channel));
  EXPECT_EQ(0, base_interface->StartSend(video_channel));

  
  ViETest::Log("Call started");
  AutoTestSleep(kAutoTestSleepTimeMs);

  
  ViETest::Log("Stopping call.");
  EXPECT_EQ(0, base_interface->StopSend(video_channel));

  
  AutoTestSleep(1000);

  EXPECT_EQ(0, base_interface->StopReceive(video_channel));
  EXPECT_EQ(0, network_interface->DeregisterSendTransport(video_channel));
}
