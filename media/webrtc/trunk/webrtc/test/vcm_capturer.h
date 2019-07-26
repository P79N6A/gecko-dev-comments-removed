








#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_VCM_CAPTURER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_VCM_CAPTURER_H_

#include "webrtc/common_types.h"
#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/modules/video_capture/include/video_capture.h"
#include "webrtc/test/video_capturer.h"

namespace webrtc {
namespace test {

class VcmCapturer : public VideoCapturer, public VideoCaptureDataCallback {
 public:
  static VcmCapturer* Create(VideoSendStreamInput* input, size_t width,
                             size_t height, size_t target_fps);
  virtual ~VcmCapturer();

  virtual void Start() OVERRIDE;
  virtual void Stop() OVERRIDE;

  virtual void OnIncomingCapturedFrame(
      const int32_t id, I420VideoFrame& frame) OVERRIDE;  
  virtual void OnCaptureDelayChanged(const int32_t id, const int32_t delay)
      OVERRIDE;

 private:
  explicit VcmCapturer(VideoSendStreamInput* input);
  bool Init(size_t width, size_t height, size_t target_fps);
  void Destroy();

  bool started_;
  VideoCaptureModule* vcm_;
  VideoCaptureCapability capability_;
};
}  
}  

#endif  
