









#ifndef WEBRTC_VIDEO_ENGINE_TEST_AUTO_TEST_PRIMITIVES_GENERAL_PRIMITIVES_H_
#define WEBRTC_VIDEO_ENGINE_TEST_AUTO_TEST_PRIMITIVES_GENERAL_PRIMITIVES_H_

class ViEToFileRenderer;

#include "common_types.h"

namespace webrtc {
class VideoCaptureModule;
class ViEBase;
class ViECapture;
class ViECodec;
class ViERender;
class ViERTP_RTCP;
struct VideoCodec;
}

enum ProtectionMethod {
  kNack,
  kHybridNackFec,
};



const int kDoNotForceResolution = 0;





void FindCaptureDeviceOnSystem(webrtc::ViECapture* capture,
                               char* device_name,
                               const unsigned int kDeviceNameLength,
                               int* device_id,
                               webrtc::VideoCaptureModule** device_video);





void RenderInWindow(webrtc::ViERender* video_render_interface,
                    int  frame_provider_id,
                    void* os_window,
                    float z_index);




void RenderToFile(webrtc::ViERender* renderer_interface,
                  int frame_provider_id,
                  ViEToFileRenderer* to_file_renderer);


void ConfigureRtpRtcp(webrtc::ViERTP_RTCP* rtcp_interface,
                      ProtectionMethod protection_method,
                      int video_channel);




bool FindSpecificCodec(webrtc::VideoCodecType of_type,
                       webrtc::ViECodec* codec_interface,
                       webrtc::VideoCodec* result);




void SetSuitableResolution(webrtc::VideoCodec* video_codec,
                           int forced_codec_width,
                           int forced_codec_height);

#endif  
