













#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_H_

#include "gflags/gflags.h"
#include "webrtc/common_types.h"
#include "webrtc/modules/video_render/include/video_render_defines.h"
#include "webrtc/video_engine/include/vie_base.h"
#include "webrtc/video_engine/include/vie_capture.h"
#include "webrtc/video_engine/include/vie_codec.h"
#include "webrtc/video_engine/include/vie_errors.h"
#include "webrtc/video_engine/include/vie_network.h"
#include "webrtc/video_engine/include/vie_render.h"
#include "webrtc/video_engine/include/vie_rtp_rtcp.h"
#include "webrtc/video_engine/test/auto_test/interface/vie_autotest_defines.h"
#include "webrtc/video_engine/vie_defines.h"
#include "webrtc/voice_engine/include/voe_audio_processing.h"
#include "webrtc/voice_engine/include/voe_base.h"
#include "webrtc/voice_engine/include/voe_codec.h"
#include "webrtc/voice_engine/include/voe_hardware.h"

#ifndef WEBRTC_ANDROID
#include <string>
#endif

class TbCaptureDevice;
class TbInterfaces;
class TbVideoChannel;
class ViEToFileRenderer;

DECLARE_bool(include_timing_dependent_tests);




class ViEAutoTest
{
public:
    ViEAutoTest(void* window1, void* window2);
    ~ViEAutoTest();

    
    
    
    int ViELoopbackCall();
    int ViESimulcastCall();
    int ViECustomCall();
    int ViERecordCall();

    
    
    void ViEStandardTest();
    void ViEExtendedTest();
    void ViEAPITest();

    
    void ViEBaseStandardTest();
    void ViEBaseExtendedTest();
    void ViEBaseAPITest();

    
    void ViECaptureStandardTest();
    void ViECaptureExtendedTest();
    void ViECaptureAPITest();
    void ViECaptureExternalCaptureTest();

    
    void ViECodecStandardTest();
    void ViECodecExtendedTest();
    void ViECodecExternalCodecTest();
    void ViECodecAPITest();

    
    void ViEImageProcessStandardTest();
    void ViEImageProcessExtendedTest();
    void ViEImageProcessAPITest();

    
    void ViENetworkStandardTest();
    void ViENetworkExtendedTest();
    void ViENetworkAPITest();

    
    void ViERenderStandardTest();
    void ViERenderExtendedTest();
    void ViERenderAPITest();

    
    void ViERtpRtcpStandardTest();
    void ViERtpRtcpExtendedTest();
    void ViERtpRtcpAPITest();

private:
    void PrintAudioCodec(const webrtc::CodecInst audioCodec);
    void PrintVideoCodec(const webrtc::VideoCodec videoCodec);

    
    
    void RenderCaptureDeviceAndOutputStream(TbInterfaces* video_engine,
                                            TbVideoChannel* video_channel,
                                            TbCaptureDevice* capture_device);

    void* _window1;
    void* _window2;

    webrtc::VideoRenderType _renderType;
    webrtc::VideoRender* _vrm1;
    webrtc::VideoRender* _vrm2;
};

#endif  
