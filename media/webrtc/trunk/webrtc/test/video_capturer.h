








#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_VIDEO_CAPTURER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_VIDEO_CAPTURER_H_

#include <stddef.h>

namespace webrtc {

class Clock;

class VideoSendStreamInput;

namespace test {

class VideoCapturer {
 public:
  static VideoCapturer* Create(VideoSendStreamInput* input,
                               size_t width,
                               size_t height,
                               int fps,
                               Clock* clock);
  virtual ~VideoCapturer() {}

  virtual void Start() = 0;
  virtual void Stop() = 0;

 protected:
  explicit VideoCapturer(VideoSendStreamInput* input);
  VideoSendStreamInput* input_;
};
}  
}  

#endif  
