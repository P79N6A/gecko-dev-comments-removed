









#ifndef WEBRTC_MODULES_UTILITY_SOURCE_VIDEO_FRAMES_QUEUE_H_
#define WEBRTC_MODULES_UTILITY_SOURCE_VIDEO_FRAMES_QUEUE_H_

#ifdef WEBRTC_MODULE_UTILITY_VIDEO

#include <list>

#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/engine_configurations.h"
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
  typedef std::list<I420VideoFrame*> FrameList;
  
  
  enum {KMaxNumberOfFrames = 300};

  
  
  
  FrameList    _incomingFrames;
  
  FrameList    _emptyFrames;

  
  uint32_t _renderDelayMs;
};
}  
#endif 
#endif  
