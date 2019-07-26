









#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_TB_INTERFACES_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_TB_INTERFACES_H_

#include <string>

#include "webrtc/common_types.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/video_engine/include/vie_base.h"
#include "webrtc/video_engine/include/vie_capture.h"
#include "webrtc/video_engine/include/vie_codec.h"
#include "webrtc/video_engine/include/vie_image_process.h"
#include "webrtc/video_engine/include/vie_network.h"
#include "webrtc/video_engine/include/vie_render.h"
#include "webrtc/video_engine/include/vie_rtp_rtcp.h"
#include "webrtc/video_engine/vie_defines.h"




class TbInterfaces
{
public:
    
    TbInterfaces(const std::string& test_name);
    ~TbInterfaces(void);

    webrtc::VideoEngine* video_engine;
    webrtc::ViEBase* base;
    webrtc::ViECapture* capture;
    webrtc::ViERender* render;
    webrtc::ViERTP_RTCP* rtp_rtcp;
    webrtc::ViECodec* codec;
    webrtc::ViENetwork* network;
    webrtc::ViEImageProcess* image_process;

    int LastError() {
        return base->LastError();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(TbInterfaces);
};

#endif  
