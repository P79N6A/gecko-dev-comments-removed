









#include "video_engine/test/libvietest/include/tb_capture_device.h"

#include "gtest/gtest.h"
#include "video_engine/test/libvietest/include/tb_interfaces.h"

TbCaptureDevice::TbCaptureDevice(TbInterfaces& Engine) :
    captureId(-1),
    ViE(Engine),
    vcpm_(NULL)
{
    const unsigned int KMaxDeviceNameLength = 128;
    const unsigned int KMaxUniqueIdLength = 256;
    char deviceName[KMaxDeviceNameLength];
    memset(deviceName, 0, KMaxDeviceNameLength);
    char uniqueId[KMaxUniqueIdLength];
    memset(uniqueId, 0, KMaxUniqueIdLength);

    bool captureDeviceSet = false;

    webrtc::VideoCaptureModule::DeviceInfo* devInfo =
        webrtc::VideoCaptureFactory::CreateDeviceInfo(0);
    for (size_t captureIdx = 0;
        captureIdx < devInfo->NumberOfDevices();
        captureIdx++)
    {
        EXPECT_EQ(0, devInfo->GetDeviceName(captureIdx, deviceName,
                                            KMaxDeviceNameLength, uniqueId,
                                            KMaxUniqueIdLength));

        vcpm_ = webrtc::VideoCaptureFactory::Create(
            captureIdx, uniqueId);
        if (vcpm_ == NULL)  
        {
            continue;
        }
        vcpm_->AddRef();

        int error = ViE.capture->AllocateCaptureDevice(*vcpm_, captureId);
        if (error == 0)
        {
            captureDeviceSet = true;
            break;
        }
    }
    delete devInfo;
    EXPECT_TRUE(captureDeviceSet);
    if (!captureDeviceSet) {
        return;
    }

    device_name_ = deviceName;
    EXPECT_EQ(0, ViE.capture->StartCapture(captureId));
}

TbCaptureDevice::~TbCaptureDevice(void)
{
    EXPECT_EQ(0, ViE.capture->StopCapture(captureId));
    EXPECT_EQ(0, ViE.capture->ReleaseCaptureDevice(captureId));
    if (vcpm_)
      vcpm_->Release();
}

void TbCaptureDevice::ConnectTo(int videoChannel)
{
    EXPECT_EQ(0, ViE.capture->ConnectCaptureDevice(captureId, videoChannel));
}

void TbCaptureDevice::Disconnect(int videoChannel)
{
    EXPECT_EQ(0, ViE.capture->DisconnectCaptureDevice(videoChannel));
}

std::string TbCaptureDevice::device_name() const {
  return device_name_;
}
