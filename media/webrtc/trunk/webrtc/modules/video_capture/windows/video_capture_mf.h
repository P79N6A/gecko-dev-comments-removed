









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_WINDOWS_VIDEO_CAPTURE_MF_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_WINDOWS_VIDEO_CAPTURE_MF_H_

#include "webrtc/modules/video_capture/video_capture_impl.h"

namespace webrtc {
namespace videocapturemodule {





class VideoCaptureMF : public VideoCaptureImpl {
 public:
  explicit VideoCaptureMF(const int32_t id);

  int32_t Init(const int32_t id, const char* device_id);

  
  virtual int32_t StartCapture(const VideoCaptureCapability& capability);
  virtual int32_t StopCapture();
  virtual bool CaptureStarted();
  virtual int32_t CaptureSettings(
      VideoCaptureCapability& settings);  

 protected:
  virtual ~VideoCaptureMF();
};

}  
}  

#endif  
