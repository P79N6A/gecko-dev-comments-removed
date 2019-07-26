









#ifndef WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_TB_CAPTURE_DEVICE_H_
#define WEBRTC_VIDEO_ENGINE_MAIN_TEST_AUTOTEST_INTERFACE_TB_CAPTURE_DEVICE_H_

#include <string>

#include "webrtc/modules/video_capture/include/video_capture_factory.h"

class TbInterfaces;

class TbCaptureDevice
{
public:
    TbCaptureDevice(TbInterfaces& Engine);
    ~TbCaptureDevice(void);

    int captureId;
    void ConnectTo(int videoChannel);
    void Disconnect(int videoChannel);
    std::string device_name() const;

private:
    TbInterfaces& ViE;
    webrtc::VideoCaptureModule* vcpm_;
    std::string device_name_;
};

#endif  
