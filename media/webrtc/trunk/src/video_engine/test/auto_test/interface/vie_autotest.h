













#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_VIE_AUTOTEST_H_

#include "common_types.h"

#include "voe_base.h"
#include "voe_codec.h"
#include "voe_hardware.h"
#include "voe_audio_processing.h"

#include "vie_base.h"
#include "vie_capture.h"
#include "vie_codec.h"
#include "vie_file.h"
#include "vie_network.h"
#include "vie_render.h"
#include "vie_rtp_rtcp.h"
#include "vie_defines.h"
#include "vie_errors.h"
#include "video_render_defines.h"

#include "vie_autotest_defines.h"

#ifndef WEBRTC_ANDROID
#include <string>
#endif

class TbCaptureDevice;
class TbInterfaces;
class TbVideoChannel;
class ViEToFileRenderer;




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

    
    void ViEEncryptionStandardTest();
    void ViEEncryptionExtendedTest();
    void ViEEncryptionAPITest();

    
    void ViEFileStandardTest();
    void ViEFileExtendedTest();
    void ViEFileAPITest();

    
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

    
    void ViERtpTryInjectingRandomPacketsIntoRtpStream(long rand_seed);

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
