









#include "video_render_frames.h"
#include "module_common_types.h"
#include "tick_util.h"
#include "trace.h"
#include <cassert>

namespace webrtc {

VideoRenderFrames::VideoRenderFrames() :
    _incomingFrames(), _renderDelayMs(10)
{
}

VideoRenderFrames::~VideoRenderFrames()
{
    ReleaseAllFrames();
}

WebRtc_Word32 VideoRenderFrames::AddFrame(VideoFrame* ptrNewFrame)
{
    const WebRtc_Word64 timeNow = TickTime::MillisecondTimestamp();

    if (ptrNewFrame->RenderTimeMs() + KOldRenderTimestampMS < timeNow)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVideoRenderer, -1,
                     "%s: too old frame.", __FUNCTION__);
        return -1;
    }
    if (ptrNewFrame->RenderTimeMs() > timeNow + KFutureRenderTimestampMS)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVideoRenderer, -1,
                     "%s: frame too long into the future.", __FUNCTION__);
        return -1;
    }

    
    VideoFrame* ptrFrameToAdd = NULL;
    if (!_emptyFrames.Empty())
    {
        ListItem* item = _emptyFrames.First();
        if (item)
        {
            ptrFrameToAdd = static_cast<VideoFrame*> (item->GetItem());
            _emptyFrames.Erase(item);
        }
    }
    if (!ptrFrameToAdd)
    {

        if (_emptyFrames.GetSize() + _incomingFrames.GetSize()
                > KMaxNumberOfFrames)
        {
            
            WEBRTC_TRACE(kTraceWarning, kTraceVideoRenderer,
                         -1, "%s: too many frames, limit: %d", __FUNCTION__,
                         KMaxNumberOfFrames);
            return -1;
        }

        
        WEBRTC_TRACE(kTraceMemory, kTraceVideoRenderer, -1,
                     "%s: allocating buffer %d", __FUNCTION__,
                     _emptyFrames.GetSize() + _incomingFrames.GetSize());

        ptrFrameToAdd = new VideoFrame();
        if (!ptrFrameToAdd)
        {
            WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, -1,
                         "%s: could not create new frame for", __FUNCTION__);
            return -1;
        }
    }

    ptrFrameToAdd->VerifyAndAllocate(ptrNewFrame->Length());
    ptrFrameToAdd->SwapFrame(const_cast<VideoFrame&> (*ptrNewFrame)); 
    _incomingFrames.PushBack(ptrFrameToAdd);

    return _incomingFrames.GetSize();
}

VideoFrame*
VideoRenderFrames::FrameToRender()
{
    VideoFrame* ptrRenderFrame = NULL;
    while (!_incomingFrames.Empty())
    {
        ListItem* item = _incomingFrames.First();
        if (item)
        {
            VideoFrame* ptrOldestFrameInList =
                    static_cast<VideoFrame*> (item->GetItem());
            if (ptrOldestFrameInList->RenderTimeMs()
                    <= TickTime::MillisecondTimestamp() + _renderDelayMs)
            {
                
                if (ptrRenderFrame)
                {
                    
                    ptrRenderFrame->SetWidth(0);
                    ptrRenderFrame->SetHeight(0);
                    ptrRenderFrame->SetLength(0);
                    ptrRenderFrame->SetRenderTime(0);
                    ptrRenderFrame->SetTimeStamp(0);
                    _emptyFrames.PushFront(ptrRenderFrame);
                }
                ptrRenderFrame = ptrOldestFrameInList;
                _incomingFrames.Erase(item);
            }
            else
            {
                
                break;
            }
        }
        else
        {
            assert(false);
        }
    }
    return ptrRenderFrame;
}

WebRtc_Word32 VideoRenderFrames::ReturnFrame(VideoFrame* ptrOldFrame)
{
    ptrOldFrame->SetWidth(0);
    ptrOldFrame->SetHeight(0);
    ptrOldFrame->SetRenderTime(0);
    ptrOldFrame->SetLength(0);
    _emptyFrames.PushBack(ptrOldFrame);

    return 0;
}

WebRtc_Word32 VideoRenderFrames::ReleaseAllFrames()
{
    while (!_incomingFrames.Empty())
    {
        ListItem* item = _incomingFrames.First();
        if (item)
        {
            VideoFrame* ptrFrame =
                    static_cast<VideoFrame*> (item->GetItem());
            assert(ptrFrame != NULL);
            ptrFrame->Free();
            delete ptrFrame;
        }
        _incomingFrames.Erase(item);
    }
    while (!_emptyFrames.Empty())
    {
        ListItem* item = _emptyFrames.First();
        if (item)
        {
            VideoFrame* ptrFrame =
                    static_cast<VideoFrame*> (item->GetItem());
            assert(ptrFrame != NULL);
            ptrFrame->Free();
            delete ptrFrame;
        }
        _emptyFrames.Erase(item);
    }
    return 0;
}

WebRtc_Word32 KEventMaxWaitTimeMs = 200;

WebRtc_UWord32 VideoRenderFrames::TimeToNextFrameRelease()
{
    WebRtc_Word64 timeToRelease = 0;
    ListItem* item = _incomingFrames.First();
    if (item)
    {
        VideoFrame* oldestFrame =
                static_cast<VideoFrame*> (item->GetItem());
        timeToRelease = oldestFrame->RenderTimeMs() - _renderDelayMs
                - TickTime::MillisecondTimestamp();
        if (timeToRelease < 0)
        {
            timeToRelease = 0;
        }
    }
    else
    {
        timeToRelease = KEventMaxWaitTimeMs;
    }

    return (WebRtc_UWord32) timeToRelease;
}


WebRtc_Word32 VideoRenderFrames::SetRenderDelay(
                                                const WebRtc_UWord32 renderDelay)
{
    _renderDelayMs = renderDelay;
    return 0;
}

} 

