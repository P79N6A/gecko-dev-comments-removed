









#ifndef WEBRTC_MODULES_UTILITY_SOURCE_VIDEO_FRAMES_QUEUE_H_
#define WEBRTC_MODULES_UTILITY_SOURCE_VIDEO_FRAMES_QUEUE_H_

#ifdef WEBRTC_MODULE_UTILITY_VIDEO

#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/system_wrappers/interface/list_wrapper.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class VideoFramesQueue {
 public:
  VideoFramesQueue();
  ~VideoFramesQueue();

  
  int32_t AddFrame(const I420VideoFrame& newFrame);

  
  
  
  I420VideoFrame* FrameToRecord();

  
  int32_t SetRenderDelay(uint32_t renderDelay);

 protected:
  
  
  int32_t ReturnFrame(I420VideoFrame* ptrOldFrame);

 private:
  
  
  enum {KMaxNumberOfFrames = 300};

  
  
  
  ListWrapper    _incomingFrames;
  
  ListWrapper    _emptyFrames;

  
  uint32_t _renderDelayMs;
};
}  
#endif
#endif
