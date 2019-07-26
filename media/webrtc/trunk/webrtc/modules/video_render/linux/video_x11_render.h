









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_LINUX_VIDEO_X11_RENDER_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_LINUX_VIDEO_X11_RENDER_H_

#include "video_render_defines.h"

#include <X11/Xlib.h>
#include <map>

namespace webrtc {
class CriticalSectionWrapper;

class VideoX11Channel;

class VideoX11Render
{

public:
    VideoX11Render(Window window);
    ~VideoX11Render();

    WebRtc_Word32 Init();
    WebRtc_Word32 ChangeWindow(Window window);

    VideoX11Channel* CreateX11RenderChannel(WebRtc_Word32 streamId,
                                                WebRtc_Word32 zOrder,
                                                const float left,
                                                const float top,
                                                const float right,
                                                const float bottom);

    WebRtc_Word32 DeleteX11RenderChannel(WebRtc_Word32 streamId);

    WebRtc_Word32 GetIncomingStreamProperties(WebRtc_Word32 streamId,
                                              WebRtc_UWord32& zOrder,
                                              float& left, float& top,
                                              float& right, float& bottom);

private:
    Window _window;
    CriticalSectionWrapper& _critSect;
    std::map<int, VideoX11Channel*> _streamIdToX11ChannelMap;

};


} 

#endif 
