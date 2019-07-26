









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_IOS_VIDEO_CAPTURE_IOS_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_IOS_VIDEO_CAPTURE_IOS_H_

#include "webrtc/modules/video_capture/video_capture_impl.h"

@class VideoCaptureIosObjC;

namespace webrtc {
namespace videocapturemodule {
class VideoCaptureIos : public VideoCaptureImpl {
 public:
  explicit VideoCaptureIos(const int32_t capture_id);
  virtual ~VideoCaptureIos();

  static VideoCaptureModule* Create(const int32_t capture_id,
                                    const char* device_unique_id_utf8);

  
  virtual int32_t StartCapture(
      const VideoCaptureCapability& capability) OVERRIDE;
  virtual int32_t StopCapture() OVERRIDE;
  virtual bool CaptureStarted() OVERRIDE;
  virtual int32_t CaptureSettings(VideoCaptureCapability& settings) OVERRIDE;

 private:
  VideoCaptureIosObjC* capture_device_;
  bool is_capturing_;
  int32_t id_;
  VideoCaptureCapability capability_;
};

}  
}  

#endif  
