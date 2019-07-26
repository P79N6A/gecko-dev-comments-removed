









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_LINUX_VIDEO_X11_CHANNEL_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_LINUX_VIDEO_X11_CHANNEL_H_

#include "video_render_defines.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include <sys/shm.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

namespace webrtc {
class CriticalSectionWrapper;

#define DEFAULT_RENDER_FRAME_WIDTH 352
#define DEFAULT_RENDER_FRAME_HEIGHT 288


class VideoX11Channel: public VideoRenderCallback
{
public:
    VideoX11Channel(WebRtc_Word32 id);

    virtual ~VideoX11Channel();

    virtual WebRtc_Word32 RenderFrame(const WebRtc_UWord32 streamId,
                                      I420VideoFrame& videoFrame);

    WebRtc_Word32 FrameSizeChange(WebRtc_Word32 width, WebRtc_Word32 height,
                                  WebRtc_Word32 numberOfStreams);
    WebRtc_Word32 DeliverFrame(const I420VideoFrame& videoFrame);
    WebRtc_Word32 GetFrameSize(WebRtc_Word32& width, WebRtc_Word32& height);
    WebRtc_Word32 Init(Window window, float left, float top, float right,
                       float bottom);
    WebRtc_Word32 ChangeWindow(Window window);
    WebRtc_Word32
            GetStreamProperties(WebRtc_UWord32& zOrder, float& left,
                                float& top, float& right, float& bottom) const;
    WebRtc_Word32 ReleaseWindow();

    bool IsPrepared()
    {
        return _prepared;
    }

private:

    WebRtc_Word32
            CreateLocalRenderer(WebRtc_Word32 width, WebRtc_Word32 height);
    WebRtc_Word32 RemoveRenderer();

    
    
    int GetWidthHeight(VideoType type, int bufferSize, int& width,
                       int& height);

    CriticalSectionWrapper& _crit;

    Display* _display;
    XShmSegmentInfo _shminfo;
    XImage* _image;
    Window _window;
    GC _gc;
    WebRtc_Word32 _width; 
    WebRtc_Word32 _height; 
    WebRtc_Word32 _outWidth; 
    WebRtc_Word32 _outHeight; 
    WebRtc_Word32 _xPos; 
    WebRtc_Word32 _yPos;
    bool _prepared; 
    WebRtc_Word32 _dispCount;

    unsigned char* _buffer;
    float _top;
    float _left;
    float _right;
    float _bottom;

    WebRtc_Word32 _Id;

};


} 

#endif 
