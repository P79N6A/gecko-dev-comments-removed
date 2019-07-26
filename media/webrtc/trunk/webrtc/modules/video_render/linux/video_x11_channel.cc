









#include "video_x11_channel.h"

#include "critical_section_wrapper.h"
#include "trace.h"

namespace webrtc {

#define DISP_MAX 128

static Display *dispArray[DISP_MAX];
static int dispCount = 0;


VideoX11Channel::VideoX11Channel(WebRtc_Word32 id) :
    _crit(*CriticalSectionWrapper::CreateCriticalSection()), _display(NULL),
          _shminfo(), _image(NULL), _window(0L), _gc(NULL),
          _width(DEFAULT_RENDER_FRAME_WIDTH),
          _height(DEFAULT_RENDER_FRAME_HEIGHT), _outWidth(0), _outHeight(0),
          _xPos(0), _yPos(0), _prepared(false), _dispCount(0), _buffer(NULL),
          _top(0.0), _left(0.0), _right(0.0), _bottom(0.0),
          _Id(id)
{
}

VideoX11Channel::~VideoX11Channel()
{
    if (_prepared)
    {
        _crit.Enter();
        ReleaseWindow();
        _crit.Leave();
    }
    delete &_crit;
}

WebRtc_Word32 VideoX11Channel::RenderFrame(const WebRtc_UWord32 streamId,
                                           I420VideoFrame& videoFrame) {
  CriticalSectionScoped cs(&_crit);
  if (_width != videoFrame.width() || _height
      != videoFrame.height()) {
      if (FrameSizeChange(videoFrame.width(), videoFrame.height(), 1) == -1) {
        return -1;
    }
  }
  return DeliverFrame(videoFrame);
}

WebRtc_Word32 VideoX11Channel::FrameSizeChange(WebRtc_Word32 width,
                                                   WebRtc_Word32 height,
                                                   WebRtc_Word32 )
{
    CriticalSectionScoped cs(&_crit);
    if (_prepared)
    {
        RemoveRenderer();
    }
    if (CreateLocalRenderer(width, height) == -1)
    {
        return -1;
    }

    return 0;
}

WebRtc_Word32 VideoX11Channel::DeliverFrame(const I420VideoFrame& videoFrame) {
  CriticalSectionScoped cs(&_crit);
  if (!_prepared) {
    return 0;
  }

  if (!dispArray[_dispCount]) {
    return -1;
  }

  ConvertFromI420(videoFrame, kARGB, 0, _buffer);

  
  XShmPutImage(_display, _window, _gc, _image, 0, 0, _xPos, _yPos, _width,
               _height, True);

  
  XSync(_display, False);
  return 0;
}

WebRtc_Word32 VideoX11Channel::GetFrameSize(WebRtc_Word32& width,
                                                WebRtc_Word32& height)
{
    width = _width;
    height = _height;

    return 0;
}

WebRtc_Word32 VideoX11Channel::Init(Window window, float left, float top,
                                        float right, float bottom)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _Id, "%s",
                 __FUNCTION__);
    CriticalSectionScoped cs(&_crit);

    _window = window;
    _left = left;
    _right = right;
    _top = top;
    _bottom = bottom;

    _display = XOpenDisplay(NULL); 
    if (!_window || !_display)
    {
        return -1;
    }

    if (dispCount < DISP_MAX)
    {
        dispArray[dispCount] = _display;
        _dispCount = dispCount;
        dispCount++;
    }
    else
    {
        return -1;
    }

    if ((1 < left || left < 0) || (1 < top || top < 0) || (1 < right || right
            < 0) || (1 < bottom || bottom < 0))
    {
        return -1;
    }

    
    int x, y;
    unsigned int winWidth, winHeight, borderwidth, depth;
    Window rootret;
    if (XGetGeometry(_display, _window, &rootret, &x, &y, &winWidth,
                     &winHeight, &borderwidth, &depth) == 0)
    {
        return -1;
    }

    _xPos = (WebRtc_Word32) (winWidth * left);
    _yPos = (WebRtc_Word32) (winHeight * top);
    _outWidth = (WebRtc_Word32) (winWidth * (right - left));
    _outHeight = (WebRtc_Word32) (winHeight * (bottom - top));
    if (_outWidth % 2)
        _outWidth++; 
    if (_outHeight % 2)
        _outHeight++;

    _gc = XCreateGC(_display, _window, 0, 0);
    if (!_gc) {
      
      assert(false);
      return -1;
    }

    if (CreateLocalRenderer(winWidth, winHeight) == -1)
    {
        return -1;
    }
    return 0;

}

WebRtc_Word32 VideoX11Channel::ChangeWindow(Window window)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _Id, "%s",
                 __FUNCTION__);
    CriticalSectionScoped cs(&_crit);

    
    RemoveRenderer();
    _window = window;

    
    int x, y;
    unsigned int winWidth, winHeight, borderwidth, depth;
    Window rootret;
    if (XGetGeometry(_display, _window, &rootret, &x, &y, &winWidth,
                     &winHeight, &borderwidth, &depth) == -1)
    {
        return -1;
    }
    _xPos = (int) (winWidth * _left);
    _yPos = (int) (winHeight * _top);
    _outWidth = (int) (winWidth * (_right - _left));
    _outHeight = (int) (winHeight * (_bottom - _top));
    if (_outWidth % 2)
        _outWidth++; 
    if (_outHeight % 2)
        _outHeight++;

    
    if (CreateLocalRenderer(_width, _height) == -1)
    {
        return -1;
    }
    return 0;
}

WebRtc_Word32 VideoX11Channel::ReleaseWindow()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _Id, "%s",
                 __FUNCTION__);
    CriticalSectionScoped cs(&_crit);

    RemoveRenderer();
    if (_gc) {
      XFreeGC(_display, _gc);
      _gc = NULL;
    }
    if (_display)
    {
        XCloseDisplay(_display);
        _display = NULL;
    }
    return 0;
}

WebRtc_Word32 VideoX11Channel::CreateLocalRenderer(WebRtc_Word32 width,
                                                       WebRtc_Word32 height)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _Id, "%s",
                 __FUNCTION__);
    CriticalSectionScoped cs(&_crit);

    if (!_window || !_display)
    {
        return -1;
    }

    if (_prepared)
    {
        WEBRTC_TRACE(kTraceWarning, kTraceVideoRenderer, _Id,
                     "Renderer already prepared, exits.");
        return -1;
    }

    _width = width;
    _height = height;

    
    _image = XShmCreateImage(_display, CopyFromParent, 24, ZPixmap, NULL,
                             &_shminfo, _width, _height); 
    _shminfo.shmid = shmget(IPC_PRIVATE, (_image->bytes_per_line
            * _image->height), IPC_CREAT | 0777);
    _shminfo.shmaddr = _image->data = (char*) shmat(_shminfo.shmid, 0, 0);
    if (_image->data == reinterpret_cast<char*>(-1))
    {
        return -1;
    }
    _buffer = (unsigned char*) _image->data;
    _shminfo.readOnly = False;

    
    if (!XShmAttach(_display, &_shminfo))
    {
        
        return -1;
    }
    XSync(_display, False);

    _prepared = true;
    return 0;
}

WebRtc_Word32 VideoX11Channel::RemoveRenderer()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _Id, "%s",
                 __FUNCTION__);

    if (!_prepared)
    {
        return 0;
    }
    _prepared = false;

    
    XShmDetach(_display, &_shminfo);
    XDestroyImage( _image );
    _image = NULL;
    shmdt(_shminfo.shmaddr);
    _shminfo.shmaddr = NULL;
    _buffer = NULL;
    shmctl(_shminfo.shmid, IPC_RMID, 0);
    _shminfo.shmid = 0;
    return 0;
}

WebRtc_Word32 VideoX11Channel::GetStreamProperties(WebRtc_UWord32& zOrder,
                                                       float& left, float& top,
                                                       float& right,
                                                       float& bottom) const
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideoRenderer, _Id, "%s",
                 __FUNCTION__);

    zOrder = 0; 
    left = _left;
    top = _top;
    right = _right;
    bottom = _bottom;

    return 0;
}


} 


