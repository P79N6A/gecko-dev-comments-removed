









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_WINDOWS_VIDEO_CAPTURE_MF_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_WINDOWS_VIDEO_CAPTURE_MF_H_

#include "modules/video_capture/video_capture_impl.h"

namespace webrtc {
namespace videocapturemodule {





class VideoCaptureMF : public VideoCaptureImpl {
 public:
  explicit VideoCaptureMF(const WebRtc_Word32 id);

  WebRtc_Word32 Init(const WebRtc_Word32 id, const char* device_id);

  
  virtual WebRtc_Word32 StartCapture(const VideoCaptureCapability& capability);
  virtual WebRtc_Word32 StopCapture();
  virtual bool CaptureStarted();
  virtual WebRtc_Word32 CaptureSettings(
      VideoCaptureCapability& settings);  

 protected:
  virtual ~VideoCaptureMF();
};

}  
}  

#endif  
