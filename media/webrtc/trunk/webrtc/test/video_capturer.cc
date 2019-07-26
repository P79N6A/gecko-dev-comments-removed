









#include "webrtc/test/video_capturer.h"

#include "webrtc/test/testsupport/fileutils.h"
#include "webrtc/test/frame_generator_capturer.h"
#include "webrtc/test/vcm_capturer.h"

namespace webrtc {
namespace test {

class NullCapturer : public VideoCapturer {
 public:
  NullCapturer() : VideoCapturer(NULL) {}
  virtual ~NullCapturer() {}

  virtual void Start() {}
  virtual void Stop() {}
};

VideoCapturer::VideoCapturer(VideoSendStreamInput* input)
    : input_(input) {}

VideoCapturer* VideoCapturer::Create(VideoSendStreamInput* input,
                                     size_t width,
                                     size_t height,
                                     int fps,
                                     Clock* clock) {
  VcmCapturer* vcm_capturer = VcmCapturer::Create(input, width, height, fps);

  if (vcm_capturer != NULL) {
    return vcm_capturer;
  }
  

  FrameGeneratorCapturer* frame_generator_capturer =
      FrameGeneratorCapturer::Create(input, width, height, fps, clock);
  if (frame_generator_capturer != NULL) {
    return frame_generator_capturer;
  }
  

  return new NullCapturer();
}
}  
}  
