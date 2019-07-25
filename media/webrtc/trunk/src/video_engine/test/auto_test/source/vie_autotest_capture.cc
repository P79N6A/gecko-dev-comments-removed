









#include "vie_autotest.h"

#include "common_types.h"
#include "engine_configurations.h"
#include "gflags/gflags.h"
#include "tb_interfaces.h"
#include "tb_video_channel.h"
#include "tick_util.h"
#include "vie_autotest_defines.h"
#include "video_capture_factory.h"
#include "vie_base.h"
#include "vie_capture.h"
#include "vie_codec.h"
#include "vie_network.h"
#include "vie_render.h"
#include "vie_rtp_rtcp.h"
#include "voe_base.h"

DEFINE_bool(capture_test_ensure_resolution_alignment_in_capture_device, true,
            "If true, we will give resolutions slightly below a reasonable "
            "value to test the camera's ability to choose a good resolution. "
            "If false, we will provide reasonable resolutions instead.");

class CaptureObserver: public webrtc::ViECaptureObserver
{
public:
    CaptureObserver() :
        _brightness(webrtc::Normal),
        _alarm(webrtc::AlarmCleared),
        _frameRate(0) {}

    virtual void BrightnessAlarm(const int captureId,
                                 const webrtc::Brightness brightness)
    {
        _brightness = brightness;
        switch (brightness)
        {
            case webrtc::Normal:
                ViETest::Log("  BrightnessAlarm Normal");
                break;
            case webrtc::Bright:
                ViETest::Log("  BrightnessAlarm Bright");
                break;
            case webrtc::Dark:
                ViETest::Log("  BrightnessAlarm Dark");
                break;
        }
    }

    virtual void CapturedFrameRate(const int captureId,
                                   const unsigned char frameRate)
    {
        ViETest::Log("  CapturedFrameRate %u", frameRate);
        _frameRate = frameRate;
    }

    virtual void NoPictureAlarm(const int captureId,
                                const webrtc::CaptureAlarm alarm)
    {
        _alarm = alarm;
        if (alarm == webrtc::AlarmRaised)
        {
            ViETest::Log("NoPictureAlarm CARaised.");
        }
        else
        {
            ViETest::Log("NoPictureAlarm CACleared.");
        }
    }

    webrtc::Brightness _brightness;
    webrtc::CaptureAlarm _alarm;
    unsigned char _frameRate;
};

class CaptureEffectFilter: public webrtc::ViEEffectFilter {
 public:
  CaptureEffectFilter(unsigned int expected_width, unsigned int expected_height)
    : number_of_captured_frames_(0),
      expected_width_(expected_width),
      expected_height_(expected_height) {
  }

  
  virtual int Transform(int size, unsigned char* frameBuffer,
                        unsigned int timeStamp90KHz, unsigned int width,
                        unsigned int height) {
    EXPECT_TRUE(frameBuffer != NULL);
    EXPECT_EQ(expected_width_, width);
    EXPECT_EQ(expected_height_, height);
    ++number_of_captured_frames_;
    return 0;
  }

  int number_of_captured_frames_;

 protected:
  unsigned int expected_width_;
  unsigned int expected_height_;
 };

void ViEAutoTest::ViECaptureStandardTest()
{
    
    
    

    
    
    

    TbInterfaces ViE("ViECaptureStandardTest");

    webrtc::VideoCaptureModule::DeviceInfo* devInfo =
        webrtc::VideoCaptureFactory::CreateDeviceInfo(0);

    int numberOfCaptureDevices = devInfo->NumberOfDevices();
    ViETest::Log("Number of capture devices %d", numberOfCaptureDevices);
    EXPECT_GT(numberOfCaptureDevices, 0);

    int captureDeviceId[10];
    memset(captureDeviceId, 0, sizeof(captureDeviceId));
    webrtc::VideoCaptureModule* vcpms[10];
    memset(vcpms, 0, sizeof(vcpms));

    
    for (int deviceIndex = 0;
         deviceIndex < numberOfCaptureDevices;
         ++deviceIndex)
    {
        char deviceName[128];
        char deviceUniqueName[512];

        EXPECT_EQ(0, devInfo->GetDeviceName(deviceIndex,
                                            deviceName,
                                            sizeof(deviceName),
                                            deviceUniqueName,
                                            sizeof(deviceUniqueName)));
        ViETest::Log("Found capture device %s\nUnique name %s", deviceName,
                     deviceUniqueName);

#if !defined(WEBRTC_MAC_INTEL)  
        int numberOfCapabilities =
            devInfo->NumberOfCapabilities(deviceUniqueName);
        EXPECT_GT(numberOfCapabilities, 0);

        for (int capIndex = 0; capIndex < numberOfCapabilities; ++capIndex)
        {
            webrtc::VideoCaptureCapability capability;
            EXPECT_EQ(0, devInfo->GetCapability(deviceUniqueName, capIndex,
                                                capability));
            ViETest::Log("Capture capability %d (of %u)", capIndex + 1,
                         numberOfCapabilities);
            ViETest::Log("witdh %d, height %d, frame rate %d",
                         capability.width, capability.height, capability.maxFPS);
            ViETest::Log("expected delay %d, color type %d, encoding %d",
                         capability.expectedCaptureDelay, capability.rawType,
                         capability.codecType);

            EXPECT_GT(capability.width, 0);
            EXPECT_GT(capability.height, 0);
            EXPECT_GT(capability.maxFPS, -1);  
            EXPECT_GT(capability.expectedCaptureDelay, 0);
        }
#endif
    }
    
#if !defined(WEBRTC_MAC_INTEL)

    
    for (int deviceIndex = 0;
         deviceIndex < numberOfCaptureDevices;
         ++deviceIndex)
    {
        char deviceName[128];
        char deviceUniqueName[512];

        EXPECT_EQ(0, devInfo->GetDeviceName(deviceIndex,
                                            deviceName,
                                            sizeof(deviceName),
                                            deviceUniqueName,
                                            sizeof(deviceUniqueName)));

        webrtc::VideoCaptureModule* vcpm =
            webrtc::VideoCaptureFactory::Create(
                deviceIndex, deviceUniqueName);
        EXPECT_TRUE(vcpm != NULL);
        vcpm->AddRef();
        vcpms[deviceIndex] = vcpm;

        EXPECT_EQ(0, ViE.capture->AllocateCaptureDevice(
            *vcpm, captureDeviceId[deviceIndex]));

        webrtc::VideoCaptureCapability capability;
        EXPECT_EQ(0, devInfo->GetCapability(deviceUniqueName, 0, capability));

        
        
        CaptureEffectFilter filter(capability.width, capability.height);
        EXPECT_EQ(0, ViE.image_process->RegisterCaptureEffectFilter(
            captureDeviceId[deviceIndex], filter));

        ViETest::Log("Testing Device %s capability width %d  height %d",
                     deviceUniqueName, capability.width, capability.height);

        if (FLAGS_capture_test_ensure_resolution_alignment_in_capture_device) {
          
          
          capability.height = capability.height - 2;
          capability.width  = capability.width  - 2;
        }

        webrtc::CaptureCapability vieCapability;
        vieCapability.width = capability.width;
        vieCapability.height = capability.height;
        vieCapability.codecType = capability.codecType;
        vieCapability.maxFPS = capability.maxFPS;
        vieCapability.rawType = capability.rawType;

        EXPECT_EQ(0, ViE.capture->StartCapture(captureDeviceId[deviceIndex],
                                               vieCapability));
        webrtc::TickTime startTime = webrtc::TickTime::Now();

        while (filter.number_of_captured_frames_ < 10
               && (webrtc::TickTime::Now() - startTime).Milliseconds() < 10000)
        {
            AutoTestSleep(100);
        }

        EXPECT_GT(filter.number_of_captured_frames_, 9) <<
            "Should capture at least some frames";

        EXPECT_EQ(0, ViE.image_process->DeregisterCaptureEffectFilter(
            captureDeviceId[deviceIndex]));

#ifdef WEBRTC_ANDROID  
        EXPECT_EQ(0, ViE.capture->StopCapture(captureDeviceId[deviceIndex]));
        EXPECT_EQ(0, ViE.capture->ReleaseCaptureDevice(
            captureDeviceId[deviceIndex]));
#endif
    }

    
    
    


    
    for (int deviceIndex = 0;
        deviceIndex < numberOfCaptureDevices;
        ++deviceIndex) {
#if !defined(WEBRTC_ANDROID)
      
      EXPECT_EQ(0, ViE.capture->StopCapture(
          captureDeviceId[deviceIndex]));
      EXPECT_EQ(0, ViE.capture->ReleaseCaptureDevice(
          captureDeviceId[deviceIndex]));
#endif  
      vcpms[deviceIndex]->Release();
    }
#endif  
}

void ViEAutoTest::ViECaptureExtendedTest() {
    ViECaptureStandardTest();
    ViECaptureAPITest();
    ViECaptureExternalCaptureTest();
}

void ViEAutoTest::ViECaptureAPITest()
{
    
    
    

    
    
    
    TbInterfaces ViE("ViECaptureAPITest");

    ViE.capture->NumberOfCaptureDevices();

    char deviceName[128];
    char deviceUniqueName[512];
    int captureId = 0;

    webrtc::VideoCaptureModule::DeviceInfo* devInfo =
        webrtc::VideoCaptureFactory::CreateDeviceInfo(0);
    EXPECT_TRUE(devInfo != NULL);

    
    EXPECT_EQ(0, devInfo->GetDeviceName(0, deviceName,
                                        sizeof(deviceName),
                                        deviceUniqueName,
                                        sizeof(deviceUniqueName)));

    webrtc::VideoCaptureModule* vcpm =
        webrtc::VideoCaptureFactory::Create(0, deviceUniqueName);
    vcpm->AddRef();
    EXPECT_TRUE(vcpm != NULL);

    
    EXPECT_EQ(0, ViE.capture->AllocateCaptureDevice(*vcpm, captureId));

    
    EXPECT_EQ(0, ViE.capture->StartCapture(captureId));

    
    EXPECT_NE(0, ViE.capture->StartCapture(captureId));
    EXPECT_EQ(kViECaptureDeviceAlreadyStarted, ViE.LastError());

    
    EXPECT_NE(0, ViE.capture->StartCapture(captureId + 1));
    EXPECT_EQ(kViECaptureDeviceDoesNotExist, ViE.LastError());

    
    EXPECT_NE(0, ViE.capture->StopCapture(captureId + 1));
    EXPECT_EQ(kViECaptureDeviceDoesNotExist, ViE.LastError());

    
    EXPECT_EQ(0, ViE.capture->StopCapture(captureId));

    
    EXPECT_NE(0, ViE.capture->StopCapture(captureId));
    EXPECT_EQ(kViECaptureDeviceNotStarted, ViE.LastError());

    
    EXPECT_NE(0, ViE.capture->ConnectCaptureDevice(captureId, 0));
    EXPECT_EQ(kViECaptureDeviceInvalidChannelId, ViE.LastError());

    TbVideoChannel channel(ViE);

    
    EXPECT_NE(0, ViE.capture->ConnectCaptureDevice(captureId + 1,
                                                   channel.videoChannel));
    EXPECT_EQ(kViECaptureDeviceDoesNotExist, ViE.LastError());

    
    EXPECT_EQ(0, ViE.capture->ConnectCaptureDevice(captureId,
                                                   channel.videoChannel));

    
    EXPECT_NE(0, ViE.capture->ConnectCaptureDevice(captureId,
                                                   channel.videoChannel));
    EXPECT_EQ(kViECaptureDeviceAlreadyConnected, ViE.LastError());

    
    EXPECT_EQ(0, ViE.capture->StartCapture(captureId));

    
    EXPECT_NE(0, ViE.capture->ReleaseCaptureDevice(captureId + 1));
    EXPECT_EQ(kViECaptureDeviceDoesNotExist, ViE.LastError());

    
    EXPECT_EQ(0, ViE.capture->ReleaseCaptureDevice(captureId));

    
    EXPECT_NE(0, ViE.capture->ReleaseCaptureDevice(captureId));
    EXPECT_EQ(kViECaptureDeviceDoesNotExist, ViE.LastError());

    
    webrtc::VideoCaptureRotation orientation;
    char dummy_name[5];
    EXPECT_NE(0, devInfo->GetOrientation(dummy_name, orientation));

    
    EXPECT_NE(0, ViE.capture->SetRotateCapturedFrames(
        captureId, webrtc::RotateCapturedFrame_90));
    EXPECT_EQ(kViECaptureDeviceDoesNotExist, ViE.LastError());

    
    EXPECT_EQ(0, ViE.capture->AllocateCaptureDevice(*vcpm, captureId));

    EXPECT_EQ(0, ViE.capture->SetRotateCapturedFrames(
        captureId, webrtc::RotateCapturedFrame_0));
    EXPECT_EQ(0, ViE.capture->SetRotateCapturedFrames(
        captureId, webrtc::RotateCapturedFrame_90));
    EXPECT_EQ(0, ViE.capture->SetRotateCapturedFrames(
        captureId, webrtc::RotateCapturedFrame_180));
    EXPECT_EQ(0, ViE.capture->SetRotateCapturedFrames(
        captureId, webrtc::RotateCapturedFrame_270));

    
    EXPECT_EQ(0, ViE.capture->ReleaseCaptureDevice(captureId));

    
    
    
    delete devInfo;
    vcpm->Release();
}

void ViEAutoTest::ViECaptureExternalCaptureTest()
{
    
    
    

    TbInterfaces ViE("ViECaptureExternalCaptureTest");
    TbVideoChannel channel(ViE);
    channel.StartReceive();
    channel.StartSend();

    webrtc::VideoCaptureExternal* externalCapture;
    int captureId = 0;

    
    webrtc::VideoCaptureModule* vcpm = webrtc::VideoCaptureFactory::Create(
        0, externalCapture);
    EXPECT_TRUE(vcpm != NULL);
    EXPECT_TRUE(externalCapture != NULL);
    vcpm->AddRef();

    EXPECT_EQ(0, ViE.capture->AllocateCaptureDevice(*vcpm, captureId));

    
    EXPECT_EQ(0, ViE.capture->ConnectCaptureDevice(captureId,
                                                   channel.videoChannel));

    
    EXPECT_EQ(0, ViE.render->AddRenderer(captureId, _window1, 1, 0.0, 0.0,
                                         1.0, 1.0));

    
    EXPECT_EQ(0, ViE.render->AddRenderer(channel.videoChannel, _window2, 1,
                                         0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, ViE.render->StartRender(captureId));
    EXPECT_EQ(0, ViE.render->StartRender(channel.videoChannel));

    
    CaptureObserver observer;
    EXPECT_EQ(0, ViE.capture->RegisterObserver(captureId, observer));

    
    EXPECT_EQ(0, ViE.capture->EnableBrightnessAlarm(captureId, true));

    CaptureEffectFilter effectFilter(176, 144);
    EXPECT_EQ(0, ViE.image_process->RegisterCaptureEffectFilter(captureId,
                                                                effectFilter));

    
    ViETest::Log("You should see local preview from external capture\n"
                 "in window 1 and the remote video in window 2.\n");

    
    
    
    const unsigned int videoFrameLength = (176 * 144 * 3) / 2;
    unsigned char* videoFrame = new unsigned char[videoFrameLength];
    memset(videoFrame, 128, 176 * 144);

    int frameCount = 0;
    webrtc::VideoCaptureCapability capability;
    capability.width = 176;
    capability.height = 144;
    capability.rawType = webrtc::kVideoI420;

    ViETest::Log("Testing external capturing and frame rate callbacks.");
    
    
    while (frameCount < 120)
    {
        externalCapture->IncomingFrame(
            videoFrame, videoFrameLength, capability,
            webrtc::TickTime::Now().MillisecondTimestamp());
        AutoTestSleep(33);

        if (effectFilter.number_of_captured_frames_ > 2)
        {
            EXPECT_EQ(webrtc::Normal, observer._brightness) <<
                "Brightness or picture alarm should not have been called yet.";
            EXPECT_EQ(webrtc::AlarmCleared, observer._alarm) <<
                "Brightness or picture alarm should not have been called yet.";
        }
        frameCount++;
    }

    
    
    for (int i = 0; i < 176 * 144; ++i)
    {
        if (videoFrame[i] <= 155)
            videoFrame[i] = videoFrame[i] + 100;
        else
            videoFrame[i] = 255;
    }
    ViETest::Log("Testing Brighness alarm");
    for (int frame = 0; frame < 30; ++frame)
    {
        externalCapture->IncomingFrame(
            videoFrame, videoFrameLength, capability,
            webrtc::TickTime::Now().MillisecondTimestamp());
        AutoTestSleep(33);
    }
    EXPECT_EQ(webrtc::Bright, observer._brightness) <<
        "Should be bright at this point since we are using a bright image.";

    
    for (int i = 0; i < 176 * 144; ++i)
    {
        videoFrame[i] = videoFrame[i] > 200 ? videoFrame[i] - 200 : 0;
    }
    for (int frame = 0; frame < 30; ++frame)
    {
        externalCapture->IncomingFrame(
            videoFrame, videoFrameLength, capability,
            webrtc::TickTime::Now().MillisecondTimestamp());
        AutoTestSleep(33);
    }
    EXPECT_EQ(webrtc::Dark, observer._brightness) <<
        "Should be dark at this point since we are using a dark image.";
    EXPECT_GT(effectFilter.number_of_captured_frames_, 150) <<
        "Frames should have been played.";

    EXPECT_GE(observer._frameRate, 29) <<
        "Frame rate callback should be approximately correct.";
    EXPECT_LE(observer._frameRate, 30) <<
        "Frame rate callback should be approximately correct.";

    
    ViETest::Log("Testing NoPictureAlarm.");
    AutoTestSleep(1050);

    EXPECT_EQ(webrtc::AlarmRaised, observer._alarm) <<
        "No picture alarm should be raised.";
    for (int frame = 0; frame < 10; ++frame)
    {
        externalCapture->IncomingFrame(
            videoFrame, videoFrameLength, capability,
            webrtc::TickTime::Now().MillisecondTimestamp());
        AutoTestSleep(33);
    }
    EXPECT_EQ(webrtc::AlarmCleared, observer._alarm) <<
        "Alarm should be cleared since ge just got some data.";

    delete videoFrame;

    
    EXPECT_EQ(0, ViE.capture->ReleaseCaptureDevice(captureId));

    
    EXPECT_NE(0, ViE.capture->ReleaseCaptureDevice(captureId));
    EXPECT_EQ(kViECaptureDeviceDoesNotExist, ViE.LastError());
    vcpm->Release();

    
    
    
}
