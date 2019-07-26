









#include "webrtc/modules/desktop_capture/screen_capture_frame_queue.h"

#include <algorithm>

#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/modules/desktop_capture/shared_desktop_frame.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/typedefs.h"

namespace webrtc {

ScreenCaptureFrameQueue::ScreenCaptureFrameQueue() : current_(0) {}

ScreenCaptureFrameQueue::~ScreenCaptureFrameQueue() {}

void ScreenCaptureFrameQueue::MoveToNextFrame() {
  current_ = (current_ + 1) % kQueueLength;

  
  
  assert(!frames_[current_].get() || !frames_[current_]->IsShared());
}

void ScreenCaptureFrameQueue::ReplaceCurrentFrame(DesktopFrame* frame) {
  frames_[current_].reset(SharedDesktopFrame::Wrap(frame));
}

void ScreenCaptureFrameQueue::Reset() {
  for (int i = 0; i < kQueueLength; ++i)
    frames_[i].reset();
}

}  
