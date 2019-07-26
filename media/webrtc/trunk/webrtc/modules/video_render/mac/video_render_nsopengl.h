









#include "engine_configurations.h"
#if defined(COCOA_RENDERING)

#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_MAC_VIDEO_RENDER_NSOPENGL_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_MAC_VIDEO_RENDER_NSOPENGL_H_

#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/glu.h>
#import <OpenGL/glext.h>
#include <QuickTime/QuickTime.h>
#include <list>
#include <map>

#include "video_render_defines.h"

#import "cocoa_render_view.h"
#import "cocoa_full_screen_window.h"

class Trace;

namespace webrtc {
class EventWrapper;
class ThreadWrapper;
class VideoRenderNSOpenGL;
class CriticalSectionWrapper;

class VideoChannelNSOpenGL : public VideoRenderCallback
{

public:

    VideoChannelNSOpenGL(NSOpenGLContext *nsglContext, int iId, VideoRenderNSOpenGL* owner);
    virtual ~VideoChannelNSOpenGL();

    
    virtual int DeliverFrame(const I420VideoFrame& videoFrame);

    
    virtual int FrameSizeChange(int width, int height, int numberOfStreams);

    virtual int UpdateSize(int width, int height);

    
    int SetStreamSettings(int streamId, float startWidth, float startHeight, float stopWidth, float stopHeight);
    int SetStreamCropSettings(int streamId, float startWidth, float startHeight, float stopWidth, float stopHeight);

    
    int RenderOffScreenBuffer();

    
    int IsUpdated(bool& isUpdated);
    virtual int UpdateStretchSize(int stretchHeight, int stretchWidth);

    
    virtual WebRtc_Word32 RenderFrame(const WebRtc_UWord32 streamId,
                                      I420VideoFrame& videoFrame);

    
    int ChangeContext(NSOpenGLContext *nsglContext);
    WebRtc_Word32 GetChannelProperties(float& left,
            float& top,
            float& right,
            float& bottom);

private:

    NSOpenGLContext* _nsglContext;
    int _id;
    VideoRenderNSOpenGL* _owner;
    WebRtc_Word32 _width;
    WebRtc_Word32 _height;
    float _startWidth;
    float _startHeight;
    float _stopWidth;
    float _stopHeight;
    int _stretchedWidth;
    int _stretchedHeight;
    int _oldStretchedHeight;
    int _oldStretchedWidth;
    unsigned char* _buffer;
    int _bufferSize;
    int _incommingBufferSize;
    bool _bufferIsUpdated;
    int _numberOfStreams;
    GLenum _pixelFormat;
    GLenum _pixelDataType;
    unsigned int _texture;
};

class VideoRenderNSOpenGL
{

public: 
    VideoRenderNSOpenGL(CocoaRenderView *windowRef, bool fullScreen, int iId);
    ~VideoRenderNSOpenGL();

    static int GetOpenGLVersion(int& nsglMajor, int& nsglMinor);

    
    int Init();
    VideoChannelNSOpenGL* CreateNSGLChannel(int streamID, int zOrder, float startWidth, float startHeight, float stopWidth, float stopHeight);
    VideoChannelNSOpenGL* ConfigureNSGLChannel(int channel, int zOrder, float startWidth, float startHeight, float stopWidth, float stopHeight);
    int DeleteNSGLChannel(int channel);
    int DeleteAllNSGLChannels();
    int StopThread();
    bool IsFullScreen();
    bool HasChannels();
    bool HasChannel(int channel);
    int GetChannels(std::list<int>& channelList);
    void LockAGLCntx();
    void UnlockAGLCntx();

    
    int ChangeWindow(CocoaRenderView* newWindowRef);
    WebRtc_Word32 ChangeUniqueID(WebRtc_Word32 id);
    WebRtc_Word32 StartRender();
    WebRtc_Word32 StopRender();
    WebRtc_Word32 DeleteNSGLChannel(const WebRtc_UWord32 streamID);
    WebRtc_Word32 GetChannelProperties(const WebRtc_UWord16 streamId,
            WebRtc_UWord32& zOrder,
            float& left,
            float& top,
            float& right,
            float& bottom);

    WebRtc_Word32 SetText(const WebRtc_UWord8 textId,
            const WebRtc_UWord8* text,
            const WebRtc_Word32 textLength,
            const WebRtc_UWord32 textColorRef,
            const WebRtc_UWord32 backgroundColorRef,
            const float left,
            const float top,
            const float right,
            const float bottom);

    
    int configureNSOpenGLEngine();
    int configureNSOpenGLView();
    int setRenderTargetWindow();
    int setRenderTargetFullScreen();

protected: 
    static bool ScreenUpdateThreadProc(void* obj);
    bool ScreenUpdateProcess();
    int GetWindowRect(Rect& rect);

private: 

    int CreateMixingContext();
    int RenderOffScreenBuffers();
    int DisplayBuffers();

private: 


    CocoaRenderView* _windowRef;
    bool _fullScreen;
    int _id;
    CriticalSectionWrapper& _nsglContextCritSec;
    ThreadWrapper* _screenUpdateThread;
    EventWrapper* _screenUpdateEvent;
    NSOpenGLContext* _nsglContext;
    NSOpenGLContext* _nsglFullScreenContext;
    CocoaFullScreenWindow* _fullScreenWindow;
    Rect _windowRect; 
    int _windowWidth;
    int _windowHeight;
    std::map<int, VideoChannelNSOpenGL*> _nsglChannels;
    std::multimap<int, int> _zOrderToChannel;
    unsigned int _threadID;
    bool _renderingIsPaused;
    NSView* _windowRefSuperView;
    NSRect _windowRefSuperViewFrame;
};

} 

#endif   
#endif	 

