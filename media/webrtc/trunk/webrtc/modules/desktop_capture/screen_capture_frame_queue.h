









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_SCREEN_CAPTURE_FRAME_QUEUE_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_SCREEN_CAPTURE_FRAME_QUEUE_H_

#include "webrtc/modules/desktop_capture/shared_desktop_frame.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class DesktopFrame;
}  

namespace webrtc {














class ScreenCaptureFrameQueue {
 public:
  ScreenCaptureFrameQueue();
  ~ScreenCaptureFrameQueue();

  
  
  void MoveToNextFrame();

  
  
  void ReplaceCurrentFrame(DesktopFrame* frame);

  
  
  void Reset();

  SharedDesktopFrame* current_frame() const {
    return frames_[current_].get();
  }

  SharedDesktopFrame* previous_frame() const {
    return frames_[(current_ + kQueueLength - 1) % kQueueLength].get();
  }

 private:
  
  int current_;

  static const int kQueueLength = 2;
  scoped_ptr<SharedDesktopFrame> frames_[kQueueLength];

  DISALLOW_COPY_AND_ASSIGN(ScreenCaptureFrameQueue);
};

}  

#endif  
