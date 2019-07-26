









#include "video_frames_queue.h"

#ifdef WEBRTC_MODULE_UTILITY_VIDEO

#include <cassert>

#include "module_common_types.h"
#include "tick_util.h"
#include "trace.h"

namespace webrtc {
VideoFramesQueue::VideoFramesQueue()
    : _incomingFrames(),
      _renderDelayMs(10)
{
}

VideoFramesQueue::~VideoFramesQueue() {
  while (!_incomingFrames.Empty()) {
    ListItem* item = _incomingFrames.First();
    if (item) {
      I420VideoFrame* ptrFrame = static_cast<I420VideoFrame*>(item->GetItem());
      assert(ptrFrame != NULL);
      delete ptrFrame;
    }
    _incomingFrames.Erase(item);
  }
  while (!_emptyFrames.Empty()) {
    ListItem* item = _emptyFrames.First();
    if (item) {
      I420VideoFrame* ptrFrame =
        static_cast<I420VideoFrame*>(item->GetItem());
      assert(ptrFrame != NULL);
      delete ptrFrame;
    }
    _emptyFrames.Erase(item);
  }
}

WebRtc_Word32 VideoFramesQueue::AddFrame(const I420VideoFrame& newFrame) {
  I420VideoFrame* ptrFrameToAdd = NULL;
  
  if (!_emptyFrames.Empty()) {
    ListItem* item = _emptyFrames.First();
    if (item) {
      ptrFrameToAdd = static_cast<I420VideoFrame*>(item->GetItem());
      _emptyFrames.Erase(item);
    }
  }
  if (!ptrFrameToAdd) {
    if (_emptyFrames.GetSize() + _incomingFrames.GetSize() >
        KMaxNumberOfFrames) {
      WEBRTC_TRACE(kTraceWarning, kTraceVideoRenderer, -1,
                   "%s: too many frames, limit: %d", __FUNCTION__,
                   KMaxNumberOfFrames);
      return -1;
    }

    WEBRTC_TRACE(kTraceMemory, kTraceVideoRenderer, -1,
                 "%s: allocating buffer %d", __FUNCTION__,
                 _emptyFrames.GetSize() + _incomingFrames.GetSize());

    ptrFrameToAdd = new I420VideoFrame();
    if (!ptrFrameToAdd) {
      WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, -1,
                   "%s: could not create new frame for", __FUNCTION__);
      return -1;
    }
  }
  ptrFrameToAdd->CopyFrame(newFrame);
  _incomingFrames.PushBack(ptrFrameToAdd);
  return 0;
}





I420VideoFrame* VideoFramesQueue::FrameToRecord() {
  I420VideoFrame* ptrRenderFrame = NULL;
  ListItem* item = _incomingFrames.First();
  while(item) {
    I420VideoFrame* ptrOldestFrameInList =
        static_cast<I420VideoFrame*>(item->GetItem());
    if (ptrOldestFrameInList->render_time_ms() <=
        TickTime::MillisecondTimestamp() + _renderDelayMs) {
      if (ptrRenderFrame) {
        
        
        
        ReturnFrame(ptrRenderFrame);
        _incomingFrames.PopFront();
      }
      item = _incomingFrames.Next(item);
      ptrRenderFrame = ptrOldestFrameInList;
    } else {
      
      
      break;
    }
  }
  return ptrRenderFrame;
}

WebRtc_Word32 VideoFramesQueue::ReturnFrame(I420VideoFrame* ptrOldFrame) {
  ptrOldFrame->set_timestamp(0);
  ptrOldFrame->set_width(0);
  ptrOldFrame->set_height(0);
  ptrOldFrame->set_render_time_ms(0);
  ptrOldFrame->ResetSize();
  _emptyFrames.PushBack(ptrOldFrame);
  return 0;
}

WebRtc_Word32 VideoFramesQueue::SetRenderDelay(WebRtc_UWord32 renderDelay) {
  _renderDelayMs = renderDelay;
  return 0;
}
} 
#endif 
