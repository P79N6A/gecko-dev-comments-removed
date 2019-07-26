









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_INTERFACE_VIDEO_RENDER_DEFINES_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_INTERFACE_VIDEO_RENDER_DEFINES_H_


#include "common_types.h"
#include "common_video/interface/i420_video_frame.h"
#include "modules/interface/module_common_types.h"

namespace webrtc
{

#ifndef NULL
#define NULL    0
#endif


enum VideoRenderType
{
    kRenderExternal = 0, 
    kRenderWindows = 1, 
    kRenderCocoa = 2, 
    kRenderCarbon = 3,
    kRenderiPhone = 4, 
    kRenderAndroid = 5, 
    kRenderX11 = 6, 
    kRenderDefault
};


enum VideoRenderError
{
    kRenderShutDown = 0,
    kRenderPerformanceAlarm = 1
};



class VideoRenderCallback
{
public:
    virtual WebRtc_Word32 RenderFrame(const WebRtc_UWord32 streamId,
                                      I420VideoFrame& videoFrame) = 0;

protected:
    virtual ~VideoRenderCallback()
    {
    }
};


class VideoRenderFeedback
{
public:
    virtual void OnRenderError(const WebRtc_Word32 streamId,
                               const VideoRenderError error) = 0;

protected:
    virtual ~VideoRenderFeedback()
    {
    }
};


enum StretchMode
{
    kStretchToInsideEdge = 1,
    kStretchToOutsideEdge = 2,
    kStretchMatchWidth = 3,
    kStretchMatchHeight = 4,
    kStretchNone = 5
};

enum Rotation
{
    kRotation0 = 0,
    kRotation90 = 1,
    kRotation180 = 2,
    kRotation270 = 3
};

} 

#endif  
