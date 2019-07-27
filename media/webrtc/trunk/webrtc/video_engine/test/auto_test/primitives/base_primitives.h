









#ifndef SRC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_SOURCE_BASE_PRIMITIVES_H_
#define SRC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_SOURCE_BASE_PRIMITIVES_H_

namespace webrtc {
class VideoEngine;
class ViEBase;
class ViECodec;
class ViENetwork;
class ViERTP_RTCP;
}





void TestI420CallSetup(webrtc::ViECodec* codec_interface,
                       webrtc::VideoEngine* video_engine,
                       webrtc::ViEBase* base_interface,
                       webrtc::ViENetwork* network_interface,
                       webrtc::ViERTP_RTCP* rtp_rtcp_interface,
                       int video_channel,
                       const char* device_name);

#endif  
