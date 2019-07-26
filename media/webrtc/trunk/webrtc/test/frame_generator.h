








#ifndef WEBRTC_COMMON_VIDEO_TEST_FRAME_GENERATOR_H_
#define WEBRTC_COMMON_VIDEO_TEST_FRAME_GENERATOR_H_

#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {

class FrameGenerator {
 public:
  FrameGenerator() {}
  virtual ~FrameGenerator() {}

  
  virtual I420VideoFrame* NextFrame() = 0;

  static FrameGenerator* Create(size_t width, size_t height);
  static FrameGenerator* CreateFromYuvFile(const char* file,
                                           size_t width,
                                           size_t height);
};
}  
}  

#endif  
