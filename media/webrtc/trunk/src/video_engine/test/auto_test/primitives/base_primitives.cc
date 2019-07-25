









#include "base_primitives.h"

#include "vie_autotest.h"
#include "vie_autotest_defines.h"
#include "video_capture_factory.h"

void TestI420CallSetup(webrtc::ViECodec* codec_interface,
                       webrtc::VideoEngine* video_engine,
                       webrtc::ViEBase* base_interface,
                       webrtc::ViENetwork* network_interface,
                       int video_channel,
                       const char* device_name) {
  webrtc::VideoCodec video_codec;
  memset(&video_codec, 0, sizeof(webrtc::VideoCodec));

  
  
  for (int i = 0; i < codec_interface->NumberOfCodecs(); i++) {
    EXPECT_EQ(0, codec_interface->GetCodec(i, video_codec));

    
    if (video_codec.codecType == webrtc::kVideoCodecI420) {
      video_codec.width = 176;
      video_codec.height = 144;
      EXPECT_EQ(0, codec_interface->SetSendCodec(video_channel, video_codec));
    }

    EXPECT_EQ(0, codec_interface->SetReceiveCodec(video_channel, video_codec));
  }

  
  EXPECT_EQ(0, codec_interface->GetSendCodec(video_channel, video_codec));
  EXPECT_EQ(webrtc::kVideoCodecI420, video_codec.codecType);

  
  char version[1024] = "";
  EXPECT_EQ(0, base_interface->GetVersion(version));
  ViETest::Log("\nUsing WebRTC Video Engine version: %s", version);

  const char *ipAddress = "127.0.0.1";
  WebRtc_UWord16 rtpPortListen = 6100;
  WebRtc_UWord16 rtpPortSend = 6100;
  EXPECT_EQ(0, network_interface->SetLocalReceiver(video_channel,
                                                   rtpPortListen));
  EXPECT_EQ(0, base_interface->StartReceive(video_channel));
  EXPECT_EQ(0, network_interface->SetSendDestination(video_channel, ipAddress,
                                                     rtpPortSend));
  EXPECT_EQ(0, base_interface->StartSend(video_channel));

  
  ViETest::Log("Call started");

  AutoTestSleep(KAutoTestSleepTimeMs);

  
  EXPECT_EQ(0, base_interface->StopSend(video_channel));
}
