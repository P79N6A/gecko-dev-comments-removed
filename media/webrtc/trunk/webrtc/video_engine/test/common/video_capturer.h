








#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_VIDEO_CAPTURER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_VIDEO_CAPTURER_H_

#include <stddef.h>

namespace webrtc {

class Clock;

namespace newapi {
class VideoSendStreamInput;
}  

namespace test {

class VideoCapturer {
 public:
  static VideoCapturer* Create(newapi::VideoSendStreamInput* input,
                               size_t width,
                               size_t height,
                               int fps,
                               Clock* clock);
  virtual ~VideoCapturer() {}

  virtual void Start() = 0;
  virtual void Stop() = 0;

 protected:
  explicit VideoCapturer(newapi::VideoSendStreamInput* input);
  newapi::VideoSendStreamInput* input_;
};
}  
}  

#endif  
