









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_VIDEO_RENDER_FRAMES_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_VIDEO_RENDER_FRAMES_H_

#include "list_wrapper.h"
#include "video_render.h"

namespace webrtc {


class VideoRenderFrames
{
public:
    VideoRenderFrames();
    ~VideoRenderFrames();

    


    WebRtc_Word32 AddFrame(VideoFrame* ptrNewFrame);

    


    VideoFrame* FrameToRender();

    


    WebRtc_Word32 ReturnFrame(VideoFrame* ptrOldFrame);

    


    WebRtc_Word32 ReleaseAllFrames();

    


    WebRtc_UWord32 TimeToNextFrameRelease();

    


    WebRtc_Word32 SetRenderDelay(const WebRtc_UWord32 renderDelay);

private:
    enum
    {
        KMaxNumberOfFrames = 300
    }; 
    enum
    {
        KOldRenderTimestampMS = 500
    }; 
    enum
    {
        KFutureRenderTimestampMS = 10000
    }; 

    ListWrapper _incomingFrames; 
    ListWrapper _emptyFrames; 

    WebRtc_UWord32 _renderDelayMs; 
};

} 

#endif
