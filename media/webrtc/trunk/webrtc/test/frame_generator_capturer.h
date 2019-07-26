








#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_FRAME_GENERATOR_CAPTURER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_FRAME_GENERATOR_CAPTURER_H_

#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/test/video_capturer.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class CriticalSectionWrapper;
class EventWrapper;
class ThreadWrapper;

namespace test {

class FrameGenerator;

class FrameGeneratorCapturer : public VideoCapturer {
 public:
  static FrameGeneratorCapturer* Create(VideoSendStreamInput* input,
                                        size_t width,
                                        size_t height,
                                        int target_fps,
                                        Clock* clock);

  static FrameGeneratorCapturer* CreateFromYuvFile(VideoSendStreamInput* input,
                                                   const char* file_name,
                                                   size_t width,
                                                   size_t height,
                                                   int target_fps,
                                                   Clock* clock);
  virtual ~FrameGeneratorCapturer();

  virtual void Start() OVERRIDE;
  virtual void Stop() OVERRIDE;

 private:
  FrameGeneratorCapturer(Clock* clock,
                         VideoSendStreamInput* input,
                         FrameGenerator* frame_generator,
                         int target_fps);
  bool Init();
  void InsertFrame();
  static bool Run(void* obj);

  Clock* clock_;
  bool sending_;

  scoped_ptr<EventWrapper> tick_;
  scoped_ptr<CriticalSectionWrapper> lock_;
  scoped_ptr<ThreadWrapper> thread_;
  scoped_ptr<FrameGenerator> frame_generator_;

  int target_fps_;
};
}  
}  

#endif  
