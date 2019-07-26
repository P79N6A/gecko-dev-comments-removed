













#include "vie_autotest_defines.h"
#include "vie_autotest.h"
#include "engine_configurations.h"

#include "video_render.h"

#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "tb_interfaces.h"
#include "tb_video_channel.h"
#include "tb_capture_device.h"

#if defined(WIN32)
#include <windows.h>
#include <ddraw.h>
#include <tchar.h>
#elif defined(WEBRTC_LINUX)
    
    #undef RGB
    #define RGB(r,g,b)          ((unsigned long)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((unsigned long)(unsigned char)(b))<<16)))
    





#elif defined(WEBRTC_MAC)
#endif

class ViEAutoTestExternalRenderer: public webrtc::ExternalRenderer
{
public:
    ViEAutoTestExternalRenderer() :
        _width(0),
        _height(0)
    {
    }
    virtual int FrameSizeChange(unsigned int width, unsigned int height,
                                unsigned int numberOfStreams)
    {
        _width = width;
        _height = height;
        return 0;
    }

    virtual int DeliverFrame(unsigned char* buffer, int bufferSize,
                             uint32_t time_stamp,
                             int64_t render_time) {
      if (bufferSize != CalcBufferSize(webrtc::kI420, _width, _height)) {
        ViETest::Log("Incorrect render buffer received, of length = %d\n",
                     bufferSize);
        return 0;
      }
      return 0;
    }

public:
    virtual ~ViEAutoTestExternalRenderer()
    {
    }
private:
    int _width, _height;
};

void ViEAutoTest::ViERenderStandardTest()
{
    
    
    
    int rtpPort = 6000;

    TbInterfaces ViE("ViERenderStandardTest");

    
    TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecVP8);
    TbCaptureDevice tbCapture(ViE); 
    tbCapture.ConnectTo(tbChannel.videoChannel);
    tbChannel.StartReceive(rtpPort);
    tbChannel.StartSend(rtpPort);

    EXPECT_EQ(0, ViE.render->RegisterVideoRenderModule(*_vrm1));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbCapture.captureId, _window1, 0, 0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, ViE.render->StartRender(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->RegisterVideoRenderModule(*_vrm2));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbChannel.videoChannel, _window2, 1, 0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, ViE.render->StartRender(tbChannel.videoChannel));

    ViETest::Log("\nCapture device is renderered in Window 1");
    ViETest::Log("Remote stream is renderered in Window 2");
    AutoTestSleep(kAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.render->StopRender(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbCapture.captureId));

    
#ifndef WEBRTC_ANDROID
    EXPECT_EQ(0, ViE.render->DeRegisterVideoRenderModule(*_vrm1));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbCapture.captureId, _window2, 0, 0.75, 0.75, 1.0, 1.0));
    EXPECT_EQ(0, ViE.render->StartRender(tbCapture.captureId));

    ViETest::Log("\nCapture device is now rendered in Window 2, PiP.");
    ViETest::Log("Switching to full screen rendering in %d seconds.\n",
                 kAutoTestSleepTimeMs / 1000);
    AutoTestSleep(kAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.render->DeRegisterVideoRenderModule(*_vrm2));

    
    webrtc::VideoRender::DestroyVideoRender(_vrm1);
    _vrm1 = NULL;
    _vrm1 = webrtc::VideoRender::CreateVideoRender(
        4563, _window1, true, _renderType);
    EXPECT_TRUE(_vrm1 != NULL);

    EXPECT_EQ(0, ViE.render->RegisterVideoRenderModule(*_vrm1));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbCapture.captureId, _window1, 0, 0.75f, 0.75f, 1.0f, 1.0f));
    EXPECT_EQ(0, ViE.render->StartRender(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbChannel.videoChannel, _window1, 1, 0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, ViE.render->StartRender(tbChannel.videoChannel));

    AutoTestSleep(kAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbCapture.captureId));

    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.render->DeRegisterVideoRenderModule(*_vrm1));

    
    webrtc::VideoRender::DestroyVideoRender(_vrm1);
    _vrm1 = NULL;
    _vrm1 = webrtc::VideoRender::CreateVideoRender(
        4561, _window1, false, _renderType);
    EXPECT_TRUE(_vrm1 != NULL);
#endif

    
    
    


    
    
    
    tbCapture.Disconnect(tbChannel.videoChannel);
}

void ViEAutoTest::ViERenderExtendedTest()
{
    int rtpPort = 6000;

    TbInterfaces ViE("ViERenderExtendedTest");

    
    TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecVP8);
    TbCaptureDevice tbCapture(ViE); 
    tbCapture.ConnectTo(tbChannel.videoChannel);
    tbChannel.StartReceive(rtpPort);
    tbChannel.StartSend(rtpPort);

    EXPECT_EQ(0, ViE.render->RegisterVideoRenderModule(*_vrm1));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbCapture.captureId, _window1, 0, 0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, ViE.render->StartRender(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->RegisterVideoRenderModule(*_vrm2));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbChannel.videoChannel, _window2, 1, 0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, ViE.render->StartRender(tbChannel.videoChannel));

    ViETest::Log("\nCapture device is renderered in Window 1");
    ViETest::Log("Remote stream is renderered in Window 2");
    AutoTestSleep(kAutoTestSleepTimeMs);

#ifdef _WIN32
    ViETest::Log("\nConfiguring Window2");
    ViETest::Log("you will see video only in first quadrant");
    EXPECT_EQ(0, ViE.render->ConfigureRender(
        tbChannel.videoChannel, 0, 0.0f, 0.0f, 0.5f, 0.5f));
    AutoTestSleep(kAutoTestSleepTimeMs);

    ViETest::Log("you will see video only in fourth quadrant");
    EXPECT_EQ(0, ViE.render->ConfigureRender(
        tbChannel.videoChannel, 0, 0.5f, 0.5f, 1.0f, 1.0f));
    AutoTestSleep(kAutoTestSleepTimeMs);

    ViETest::Log("normal video on Window2");
    EXPECT_EQ(0, ViE.render->ConfigureRender(
        tbChannel.videoChannel, 0, 0.0f, 0.0f, 1.0f, 1.0f));
    AutoTestSleep(kAutoTestSleepTimeMs);
#endif

    ViETest::Log("Mirroring Local Preview (Window1) Left-Right");
    EXPECT_EQ(0, ViE.render->MirrorRenderStream(
        tbCapture.captureId, true, false, true));
    AutoTestSleep(kAutoTestSleepTimeMs);

    ViETest::Log("\nMirroring Local Preview (Window1) Left-Right and Up-Down");
    EXPECT_EQ(0, ViE.render->MirrorRenderStream(
        tbCapture.captureId, true, true, true));
    AutoTestSleep(kAutoTestSleepTimeMs);

    ViETest::Log("\nMirroring Remote Window(Window2) Up-Down");
    EXPECT_EQ(0, ViE.render->MirrorRenderStream(
        tbChannel.videoChannel, true, true, false));
    AutoTestSleep(kAutoTestSleepTimeMs);

    ViETest::Log("Disabling Mirroing on Window1 and Window2");
    EXPECT_EQ(0, ViE.render->MirrorRenderStream(
        tbCapture.captureId, false, false, false));
    AutoTestSleep(kAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.render->MirrorRenderStream(
        tbChannel.videoChannel, false, false, false));
    AutoTestSleep(kAutoTestSleepTimeMs);

    ViETest::Log("\nEnabling Full Screen render in 5 sec");

    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->DeRegisterVideoRenderModule(*_vrm1));
    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.render->DeRegisterVideoRenderModule(*_vrm2));

    
    webrtc::VideoRender::DestroyVideoRender(_vrm1);
    _vrm1 = NULL;
    _vrm1 = webrtc::VideoRender::CreateVideoRender(
        4563, _window1, true, _renderType);
    EXPECT_TRUE(_vrm1 != NULL);

    EXPECT_EQ(0, ViE.render->RegisterVideoRenderModule(*_vrm1));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbCapture.captureId, _window1, 0, 0.0f, 0.0f, 1.0f, 1.0f));
    EXPECT_EQ(0, ViE.render->StartRender(tbCapture.captureId));
    AutoTestSleep(kAutoTestSleepTimeMs);

    ViETest::Log("\nStop renderer");
    EXPECT_EQ(0, ViE.render->StopRender(tbCapture.captureId));
    ViETest::Log("\nRemove renderer");
    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbCapture.captureId));

    EXPECT_EQ(0, ViE.render->DeRegisterVideoRenderModule(*_vrm1));

    
    webrtc::VideoRender::DestroyVideoRender(_vrm1);
    _vrm1 = NULL;
    _vrm1 = webrtc::VideoRender::CreateVideoRender(4564, NULL, false,
                                                   _renderType);
    EXPECT_TRUE(_vrm1 != NULL);

    EXPECT_EQ(0, ViE.render->RegisterVideoRenderModule(*_vrm1));

    ViETest::Log("\nExternal Render Test");
    ViEAutoTestExternalRenderer externalRenderObj;
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbCapture.captureId, webrtc::kVideoI420, &externalRenderObj));
    EXPECT_EQ(0, ViE.render->StartRender(tbCapture.captureId));
    AutoTestSleep(kAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.render->StopRender(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->DeRegisterVideoRenderModule(*_vrm1));

    
    
    webrtc::VideoRender::DestroyVideoRender(_vrm1);
    _vrm1 = NULL;
    _vrm1 = webrtc::VideoRender::CreateVideoRender(
        4561, _window1, false, _renderType);
    EXPECT_TRUE(_vrm1 != NULL);
    tbCapture.Disconnect(tbChannel.videoChannel);
}

void ViEAutoTest::ViERenderAPITest() {
  TbInterfaces ViE("ViERenderAPITest");

  TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecVP8);
  TbCaptureDevice tbCapture(ViE);
  tbCapture.ConnectTo(tbChannel.videoChannel);
  tbChannel.StartReceive();
  tbChannel.StartSend();

  EXPECT_EQ(0, ViE.render->AddRenderer(
      tbCapture.captureId, _window1, 0, 0.0, 0.0, 1.0, 1.0));
  EXPECT_EQ(0, ViE.render->StartRender(tbCapture.captureId));
  EXPECT_EQ(0, ViE.render->AddRenderer(
      tbChannel.videoChannel, _window2, 1, 0.0, 0.0, 1.0, 1.0));
  EXPECT_EQ(0, ViE.render->StartRender(tbChannel.videoChannel));

  
  
  EXPECT_EQ(-1, ViE.render->SetExpectedRenderDelay(tbChannel.videoChannel, 50));
  EXPECT_EQ(0, ViE.render->StopRender(tbChannel.videoChannel));
  
  EXPECT_EQ(-1, ViE.render->SetExpectedRenderDelay(tbChannel.videoChannel, 9));
  EXPECT_EQ(-1, ViE.render->SetExpectedRenderDelay(tbChannel.videoChannel,
                                                   501));
  
  EXPECT_EQ(0, ViE.render->SetExpectedRenderDelay(tbChannel.videoChannel, 11));
  EXPECT_EQ(0, ViE.render->SetExpectedRenderDelay(tbChannel.videoChannel, 499));
}
