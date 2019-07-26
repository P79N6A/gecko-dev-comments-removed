









#ifndef WEBRTC_MODULES_UTILITY_SOURCE_VIDEO_FRAMES_QUEUE_H_
#define WEBRTC_MODULES_UTILITY_SOURCE_VIDEO_FRAMES_QUEUE_H_

#ifdef WEBRTC_MODULE_UTILITY_VIDEO

#include "common_video/interface/i420_video_frame.h"
#include "engine_configurations.h"
#include "list_wrapper.h"
#include "typedefs.h"

namespace webrtc {

class VideoFramesQueue {
 public:
  VideoFramesQueue();
  ~VideoFramesQueue();

  
  WebRtc_Word32 AddFrame(const I420VideoFrame& newFrame);

  
  
  
  I420VideoFrame* FrameToRecord();

  
  WebRtc_Word32 SetRenderDelay(WebRtc_UWord32 renderDelay);

 protected:
  
  
  WebRtc_Word32 ReturnFrame(I420VideoFrame* ptrOldFrame);

 private:
  
  
  enum {KMaxNumberOfFrames = 300};

  
  
  
  ListWrapper    _incomingFrames;
  
  ListWrapper    _emptyFrames;

  
  WebRtc_UWord32 _renderDelayMs;
};
} 
#endif
#endif
