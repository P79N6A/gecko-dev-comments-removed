









#include "modules/video_render/main/source/video_render_frames.h"

#include <cassert>

#include "modules/interface/module_common_types.h"
#include "system_wrappers/interface/tick_util.h"
#include "system_wrappers/interface/trace.h"

namespace webrtc {

WebRtc_Word32 KEventMaxWaitTimeMs = 200;

VideoRenderFrames::VideoRenderFrames()
    : incoming_frames_(),
      render_delay_ms_(10) {
}

VideoRenderFrames::~VideoRenderFrames() {
  ReleaseAllFrames();
}

WebRtc_Word32 VideoRenderFrames::AddFrame(VideoFrame* new_frame) {
  const WebRtc_Word64 time_now = TickTime::MillisecondTimestamp();

  if (new_frame->RenderTimeMs() + KOldRenderTimestampMS < time_now) {
    WEBRTC_TRACE(kTraceWarning, kTraceVideoRenderer, -1,
                 "%s: too old frame.", __FUNCTION__);
    return -1;
  }
  if (new_frame->RenderTimeMs() > time_now + KFutureRenderTimestampMS) {
    WEBRTC_TRACE(kTraceWarning, kTraceVideoRenderer, -1,
                 "%s: frame too long into the future.", __FUNCTION__);
    return -1;
  }

  
  VideoFrame* frame_to_add = NULL;
  if (!empty_frames_.Empty()) {
    ListItem* item = empty_frames_.First();
    if (item) {
      frame_to_add = static_cast<VideoFrame*>(item->GetItem());
      empty_frames_.Erase(item);
    }
  }
  if (!frame_to_add) {
    if (empty_frames_.GetSize() + incoming_frames_.GetSize() >
        KMaxNumberOfFrames) {
      
      WEBRTC_TRACE(kTraceWarning, kTraceVideoRenderer,
                   -1, "%s: too many frames, limit: %d", __FUNCTION__,
                   KMaxNumberOfFrames);
      return -1;
    }

    
    WEBRTC_TRACE(kTraceMemory, kTraceVideoRenderer, -1,
                 "%s: allocating buffer %d", __FUNCTION__,
                 empty_frames_.GetSize() + incoming_frames_.GetSize());

    frame_to_add = new VideoFrame();
    if (!frame_to_add) {
      WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, -1,
                   "%s: could not create new frame for", __FUNCTION__);
      return -1;
    }
  }

  frame_to_add->VerifyAndAllocate(new_frame->Length());
  
  
  frame_to_add->SwapFrame(const_cast<VideoFrame&>(*new_frame));
  incoming_frames_.PushBack(frame_to_add);

  return incoming_frames_.GetSize();
}

VideoFrame* VideoRenderFrames::FrameToRender() {
  VideoFrame* render_frame = NULL;
  while (!incoming_frames_.Empty()) {
    ListItem* item = incoming_frames_.First();
    if (item) {
      VideoFrame* oldest_frame_in_list =
          static_cast<VideoFrame*>(item->GetItem());
      if (oldest_frame_in_list->RenderTimeMs() <=
          TickTime::MillisecondTimestamp() + render_delay_ms_) {
        
        if (render_frame) {
          
          render_frame->SetWidth(0);
          render_frame->SetHeight(0);
          render_frame->SetLength(0);
          render_frame->SetRenderTime(0);
          render_frame->SetTimeStamp(0);
          empty_frames_.PushFront(render_frame);
        }
        render_frame = oldest_frame_in_list;
        incoming_frames_.Erase(item);
      } else {
        
        break;
      }
    } else {
      assert(false);
    }
  }
  return render_frame;
}

WebRtc_Word32 VideoRenderFrames::ReturnFrame(VideoFrame* old_frame) {
  old_frame->SetWidth(0);
  old_frame->SetHeight(0);
  old_frame->SetRenderTime(0);
  old_frame->SetLength(0);
  empty_frames_.PushBack(old_frame);
  return 0;
}

WebRtc_Word32 VideoRenderFrames::ReleaseAllFrames() {
  while (!incoming_frames_.Empty()) {
    ListItem* item = incoming_frames_.First();
    if (item) {
      VideoFrame* frame = static_cast<VideoFrame*>(item->GetItem());
      assert(frame != NULL);
      frame->Free();
      delete frame;
    }
    incoming_frames_.Erase(item);
  }
  while (!empty_frames_.Empty()) {
    ListItem* item = empty_frames_.First();
    if (item) {
      VideoFrame* frame = static_cast<VideoFrame*>(item->GetItem());
      assert(frame != NULL);
      frame->Free();
      delete frame;
    }
    empty_frames_.Erase(item);
  }
  return 0;
}

WebRtc_UWord32 VideoRenderFrames::TimeToNextFrameRelease() {
  WebRtc_Word64 time_to_release = 0;
  ListItem* item = incoming_frames_.First();
  if (item) {
    VideoFrame* oldest_frame = static_cast<VideoFrame*>(item->GetItem());
    time_to_release = oldest_frame->RenderTimeMs() - render_delay_ms_
                      - TickTime::MillisecondTimestamp();
    if (time_to_release < 0) {
      time_to_release = 0;
    }
  } else {
    time_to_release = KEventMaxWaitTimeMs;
  }
  return static_cast<WebRtc_UWord32>(time_to_release);
}

WebRtc_Word32 VideoRenderFrames::SetRenderDelay(
    const WebRtc_UWord32 render_delay) {
  render_delay_ms_ = render_delay;
  return 0;
}

}  
