








#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_INCLUDE_MOCK_MOCK_VIDEO_CAPTURE_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_INCLUDE_MOCK_MOCK_VIDEO_CAPTURE_H_

#include "webrtc/modules/video_capture/include/video_capture.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace webrtc {

class MockVideoCaptureModule : public VideoCaptureModule {
 public:
  
  MOCK_METHOD0(TimeUntilNextProcess, int32_t());
  MOCK_METHOD0(Process, int32_t());

  
  MOCK_METHOD0(AddRef, int32_t());
  MOCK_METHOD0(Release, int32_t());

  
  MOCK_METHOD1(RegisterCaptureDataCallback,
      void(VideoCaptureDataCallback& dataCallback));
  MOCK_METHOD0(DeRegisterCaptureDataCallback, void());
  MOCK_METHOD1(RegisterCaptureCallback, void(VideoCaptureFeedBack& callBack));
  MOCK_METHOD0(DeRegisterCaptureCallback, void());
  MOCK_METHOD1(StartCapture, int32_t(const VideoCaptureCapability& capability));
  MOCK_METHOD0(StopCapture, int32_t());
  MOCK_CONST_METHOD0(CurrentDeviceName, const char*());
  MOCK_METHOD0(CaptureStarted, bool());
  MOCK_METHOD1(CaptureSettings, int32_t(VideoCaptureCapability& settings));
  MOCK_METHOD1(SetCaptureDelay, void(int32_t delayMS));
  MOCK_METHOD0(CaptureDelay, int32_t());
  MOCK_METHOD1(SetCaptureRotation, int32_t(VideoCaptureRotation rotation));
  MOCK_METHOD1(GetEncodeInterface,
               VideoCaptureEncodeInterface*(const VideoCodec& codec));
  MOCK_METHOD1(EnableFrameRateCallback, void(const bool enable));
  MOCK_METHOD1(EnableNoPictureAlarm, void(const bool enable));
};

}  

#endif  
