









#include "webrtc/modules/utility/source/video_frames_queue.h"

#ifdef WEBRTC_MODULE_UTILITY_VIDEO

#include <assert.h>

#include "webrtc/common_video/interface/texture_video_frame.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {
VideoFramesQueue::VideoFramesQueue()
    : _renderDelayMs(10)
{
}

VideoFramesQueue::~VideoFramesQueue() {
  for (FrameList::iterator iter = _incomingFrames.begin();
       iter != _incomingFrames.end(); ++iter) {
      delete *iter;
  }
  for (FrameList::iterator iter = _emptyFrames.begin();
       iter != _emptyFrames.end(); ++iter) {
      delete *iter;
  }
}

int32_t VideoFramesQueue::AddFrame(const I420VideoFrame& newFrame) {
  if (newFrame.native_handle() != NULL) {
    _incomingFrames.push_back(new TextureVideoFrame(
        static_cast<NativeHandle*>(newFrame.native_handle()),
        newFrame.width(),
        newFrame.height(),
        newFrame.timestamp(),
        newFrame.render_time_ms()));
    return 0;
  }

  I420VideoFrame* ptrFrameToAdd = NULL;
  
  if (!_emptyFrames.empty()) {
    ptrFrameToAdd = _emptyFrames.front();
    _emptyFrames.pop_front();
  }
  if (!ptrFrameToAdd) {
    if (_emptyFrames.size() + _incomingFrames.size() >
        KMaxNumberOfFrames) {
      WEBRTC_TRACE(kTraceWarning, kTraceVideoRenderer, -1,
                   "%s: too many frames, limit: %d", __FUNCTION__,
                   KMaxNumberOfFrames);
      return -1;
    }

    WEBRTC_TRACE(kTraceMemory, kTraceVideoRenderer, -1,
                 "%s: allocating buffer %d", __FUNCTION__,
                 _emptyFrames.size() + _incomingFrames.size());

    ptrFrameToAdd = new I420VideoFrame();
  }
  ptrFrameToAdd->CopyFrame(newFrame);
  _incomingFrames.push_back(ptrFrameToAdd);
  return 0;
}





I420VideoFrame* VideoFramesQueue::FrameToRecord() {
  I420VideoFrame* ptrRenderFrame = NULL;
  for (FrameList::iterator iter = _incomingFrames.begin();
       iter != _incomingFrames.end(); ++iter) {
    I420VideoFrame* ptrOldestFrameInList = *iter;
    if (ptrOldestFrameInList->render_time_ms() <=
        TickTime::MillisecondTimestamp() + _renderDelayMs) {
      
      
      
      ReturnFrame(ptrRenderFrame);
      iter = _incomingFrames.erase(iter);
      ptrRenderFrame = ptrOldestFrameInList;
    } else {
      
      
      break;
    }
  }
  return ptrRenderFrame;
}

int32_t VideoFramesQueue::ReturnFrame(I420VideoFrame* ptrOldFrame) {
  
  if (ptrOldFrame->native_handle() == NULL) {
    ptrOldFrame->set_timestamp(0);
    ptrOldFrame->set_width(0);
    ptrOldFrame->set_height(0);
    ptrOldFrame->set_render_time_ms(0);
    ptrOldFrame->ResetSize();
    _emptyFrames.push_back(ptrOldFrame);
  } else {
    delete ptrOldFrame;
  }
  return 0;
}

int32_t VideoFramesQueue::SetRenderDelay(uint32_t renderDelay) {
  _renderDelayMs = renderDelay;
  return 0;
}
}  
#endif 
