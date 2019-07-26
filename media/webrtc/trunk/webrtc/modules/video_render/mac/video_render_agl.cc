









#include "engine_configurations.h"

#if defined(CARBON_RENDERING)

#include "video_render_agl.h"


#include "critical_section_wrapper.h"
#include "event_wrapper.h"
#include "trace.h"
#include "thread_wrapper.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"

namespace webrtc {







#pragma mark VideoChannelAGL constructor

VideoChannelAGL::VideoChannelAGL(AGLContext& aglContext, int iId, VideoRenderAGL* owner) :
    _aglContext( aglContext),
    _id( iId),
    _owner( owner),
    _width( 0),
    _height( 0),
    _stretchedWidth( 0),
    _stretchedHeight( 0),
    _startWidth( 0.0f),
    _startHeight( 0.0f),
    _stopWidth( 0.0f),
    _stopHeight( 0.0f),
    _xOldWidth( 0),
    _yOldHeight( 0),
    _oldStretchedHeight(0),
    _oldStretchedWidth( 0),
    _buffer( 0),
    _bufferSize( 0),
    _incommingBufferSize(0),
    _bufferIsUpdated( false),
    _sizeInitialized( false),
    _numberOfStreams( 0),
    _bVideoSizeStartedChanging(false),
    _pixelFormat( GL_RGBA),
    _pixelDataType( GL_UNSIGNED_INT_8_8_8_8),
    _texture( 0)

{
    
}

VideoChannelAGL::~VideoChannelAGL()
{
    
    if (_buffer)
    {
        delete [] _buffer;
        _buffer = NULL;
    }

    aglSetCurrentContext(_aglContext);

    if (_texture != 0)
    {
        glDeleteTextures(1, (const GLuint*) &_texture);
        _texture = 0;
    }
}

WebRtc_Word32 VideoChannelAGL::RenderFrame(const WebRtc_UWord32 streamId,
                                           I420VideoFrame& videoFrame) {
  _owner->LockAGLCntx();
  if (_width != videoFrame.width() ||
      _height != videoFrame.height()) {
    if (FrameSizeChange(videoFrame.width(), videoFrame.height(), 1) == -1) {
      WEBRTC_TRACE(kTraceError, kTraceVideoRenderer, _id, "%s:%d FrameSize
                   Change returned an error", __FUNCTION__, __LINE__);
      _owner->UnlockAGLCntx();
      return -1;
    }
  }

  _owner->UnlockAGLCntx();
  return DeliverFrame(videoFrame);
}

int VideoChannelAGL::UpdateSize(int , int )
{
    _owner->LockAGLCntx();
    _owner->UnlockAGLCntx();
    return 0;
}

int VideoChannelAGL::UpdateStretchSize(int stretchHeight, int stretchWidth)
{

    _owner->LockAGLCntx();
    _stretchedHeight = stretchHeight;
    _stretchedWidth = stretchWidth;
    _owner->UnlockAGLCntx();
    return 0;
}

int VideoChannelAGL::FrameSizeChange(int width, int height, int numberOfStreams)
{
    

    _owner->LockAGLCntx();

    if (width == _width && _height == height)
    {
        
        _numberOfStreams = numberOfStreams;
        _owner->UnlockAGLCntx();
        return 0;
    }

    _width = width;
    _height = height;

    
    if (_buffer)
    {
        delete [] _buffer;
        _bufferSize = 0;
    }

    _incommingBufferSize = CalcBufferSize(kI420, _width, _height);
    _bufferSize = CalcBufferSize(kARGB, _width, _height);
    _buffer = new unsigned char [_bufferSize];
    memset(_buffer, 0, _bufferSize * sizeof(unsigned char));

    if (aglSetCurrentContext(_aglContext) == false)
    {
        _owner->UnlockAGLCntx();
        return -1;
    }

    
    if (_texture != 0)
    {
        glDeleteTextures(1, (const GLuint*) &_texture);
        _texture = 0;
    }

    
    glGenTextures(1, (GLuint *) &_texture);

    GLenum glErr = glGetError();

    if (glErr != GL_NO_ERROR)
    {
    }

    
    
    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, _texture);

    
    glTexParameterf(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_PRIORITY, 1.0);

    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_STORAGE_HINT_APPLE, GL_STORAGE_SHARED_APPLE);

    
    GLint texSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);

    if (texSize < _width || texSize < _height)
    {
        
        _owner->UnlockAGLCntx();
        return -1;
    }

    
    glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 
            0, 
            GL_RGBA, 
            _width, 
            _height, 
            0, 
            _pixelFormat, 
            _pixelDataType, 
            _buffer); 

    glErr = glGetError();
    if (glErr != GL_NO_ERROR)
    {
        _owner->UnlockAGLCntx();
        return -1;
    }

    _owner->UnlockAGLCntx();
    return 0;
}


int VideoChannelAGL::DeliverFrame(const I420VideoFrame& videoFrame) {
  _owner->LockAGLCntx();

  if (_texture == 0) {
    _owner->UnlockAGLCntx();
    return 0;
  }

  int length = CalcBufferSize(kI420, videoFrame.width(), videoFrame.height());
  if (length != _incommingBufferSize) {
    _owner->UnlockAGLCntx();
    return -1;
  }

  
  int rgbret = ConvertFromYV12(videoFrame, kBGRA, 0, _buffer);
  if (rgbret < 0) {
    _owner->UnlockAGLCntx();
    return -1;
  }

  aglSetCurrentContext(_aglContext);

  
  
  glBindTexture(GL_TEXTURE_RECTANGLE_EXT, _texture);
  GLenum glErr = glGetError();
  if (glErr != GL_NO_ERROR) {
    _owner->UnlockAGLCntx();
    return -1;
  }

  
  glTexSubImage2D(GL_TEXTURE_RECTANGLE_EXT,
                  0, 
                  0, 
                  0, 
                  _width, 
                  _height, 
                  _pixelFormat, 
                  _pixelDataType, 
                  (const GLvoid*) _buffer); 

  if (glGetError() != GL_NO_ERROR) {
    _owner->UnlockAGLCntx();
    return -1;
  }

  _bufferIsUpdated = true;
  _owner->UnlockAGLCntx();

  return 0;
}

int VideoChannelAGL::RenderOffScreenBuffer()
{

    _owner->LockAGLCntx();

    if (_texture == 0)
    {
        _owner->UnlockAGLCntx();
        return 0;
    }

    GLfloat xStart = 2.0f * _startWidth - 1.0f;
    GLfloat xStop = 2.0f * _stopWidth - 1.0f;
    GLfloat yStart = 1.0f - 2.0f * _stopHeight;
    GLfloat yStop = 1.0f - 2.0f * _startHeight;

    aglSetCurrentContext(_aglContext);
    glBindTexture(GL_TEXTURE_RECTANGLE_EXT, _texture);

    if(_stretchedWidth != _oldStretchedWidth || _stretchedHeight != _oldStretchedHeight)
    {
        glViewport(0, 0, _stretchedWidth, _stretchedHeight);
    }
    _oldStretchedHeight = _stretchedHeight;
    _oldStretchedWidth = _stretchedWidth;

    
    glLoadIdentity();

    glEnable(GL_TEXTURE_RECTANGLE_EXT);

    glBegin(GL_POLYGON);
    {
        glTexCoord2f(0.0, 0.0); glVertex2f(xStart, yStop);
        glTexCoord2f(_width, 0.0); glVertex2f(xStop, yStop);
        glTexCoord2f(_width, _height); glVertex2f(xStop, yStart);
        glTexCoord2f(0.0, _height); glVertex2f(xStart, yStart);
    }
    glEnd();

    glDisable(GL_TEXTURE_RECTANGLE_EXT);

    _bufferIsUpdated = false;

    _owner->UnlockAGLCntx();
    return 0;
}

int VideoChannelAGL::IsUpdated(bool& isUpdated)
{
    _owner->LockAGLCntx();
    isUpdated = _bufferIsUpdated;
    _owner->UnlockAGLCntx();

    return 0;
}

int VideoChannelAGL::SetStreamSettings(int , float startWidth, float startHeight, float stopWidth, float stopHeight)
{

    _owner->LockAGLCntx();

    _startWidth = startWidth;
    _stopWidth = stopWidth;
    _startHeight = startHeight;
    _stopHeight = stopHeight;

    int oldWidth = _width;
    int oldHeight = _height;
    int oldNumberOfStreams = _numberOfStreams;

    _width = 0;
    _height = 0;

    int retVal = FrameSizeChange(oldWidth, oldHeight, oldNumberOfStreams);

    _owner->UnlockAGLCntx();

    return retVal;
}

int VideoChannelAGL::SetStreamCropSettings(int , float , float , float , float )
{
    return -1;
}

#pragma mark VideoRenderAGL WindowRef constructor

VideoRenderAGL::VideoRenderAGL(WindowRef windowRef, bool fullscreen, int iId) :
_hiviewRef( 0),
_windowRef( windowRef),
_fullScreen( fullscreen),
_id( iId),
_renderCritSec(*CriticalSectionWrapper::CreateCriticalSection()),
_screenUpdateThread( 0),
_screenUpdateEvent( 0),
_isHIViewRef( false),
_aglContext( 0),
_windowWidth( 0),
_windowHeight( 0),
_lastWindowWidth( -1),
_lastWindowHeight( -1),
_lastHiViewWidth( -1),
_lastHiViewHeight( -1),
_currentParentWindowHeight( 0),
_currentParentWindowWidth( 0),
_currentParentWindowBounds( ),
_windowHasResized( false),
_lastParentWindowBounds( ),
_currentHIViewBounds( ),
_lastHIViewBounds( ),
_windowRect( ),
_aglChannels( ),
_zOrderToChannel( ),
_hiviewEventHandlerRef( NULL),
_windowEventHandlerRef( NULL),
_currentViewBounds( ),
_lastViewBounds( ),
_renderingIsPaused( false),
_threadID( )

{
    

    _screenUpdateThread = ThreadWrapper::CreateThread(ScreenUpdateThreadProc, this, kRealtimePriority);
    _screenUpdateEvent = EventWrapper::Create();

    if(!IsValidWindowPtr(_windowRef))
    {
        
    }
    else
    {
        
    }

    GetWindowRect(_windowRect);

    _lastViewBounds.origin.x = 0;
    _lastViewBounds.origin.y = 0;
    _lastViewBounds.size.width = 0;
    _lastViewBounds.size.height = 0;

}



#pragma mark WindowRef Event Handler
pascal OSStatus VideoRenderAGL::sHandleWindowResized (EventHandlerCallRef ,
        EventRef theEvent,
        void* userData)
{
    WindowRef windowRef = NULL;

    int eventType = GetEventKind(theEvent);

    
    GetEventParameter (theEvent,
            kEventParamDirectObject,
            typeWindowRef,
            NULL,
            sizeof (WindowRef),
            NULL,
            &windowRef);

    VideoRenderAGL* obj = (VideoRenderAGL*)(userData);

    bool updateUI = true;
    if(kEventWindowBoundsChanged == eventType)
    {
    }
    else if(kEventWindowBoundsChanging == eventType)
    {
    }
    else if(kEventWindowZoomed == eventType)
    {
    }
    else if(kEventWindowExpanding == eventType)
    {
    }
    else if(kEventWindowExpanded == eventType)
    {
    }
    else if(kEventWindowClickResizeRgn == eventType)
    {
    }
    else if(kEventWindowClickDragRgn == eventType)
    {
    }
    else
    {
        updateUI = false;
    }

    if(true == updateUI)
    {
        obj->ParentWindowResized(windowRef);
        obj->UpdateClipping();
        obj->RenderOffScreenBuffers();
    }

    return noErr;
}

#pragma mark VideoRenderAGL HIViewRef constructor

VideoRenderAGL::VideoRenderAGL(HIViewRef windowRef, bool fullscreen, int iId) :
_hiviewRef( windowRef),
_windowRef( 0),
_fullScreen( fullscreen),
_id( iId),
_renderCritSec(*CriticalSectionWrapper::CreateCriticalSection()),
_screenUpdateThread( 0),
_screenUpdateEvent( 0),
_isHIViewRef( false),
_aglContext( 0),
_windowWidth( 0),
_windowHeight( 0),
_lastWindowWidth( -1),
_lastWindowHeight( -1),
_lastHiViewWidth( -1),
_lastHiViewHeight( -1),
_currentParentWindowHeight( 0),
_currentParentWindowWidth( 0),
_currentParentWindowBounds( ),
_windowHasResized( false),
_lastParentWindowBounds( ),
_currentHIViewBounds( ),
_lastHIViewBounds( ),
_windowRect( ),
_aglChannels( ),
_zOrderToChannel( ),
_hiviewEventHandlerRef( NULL),
_windowEventHandlerRef( NULL),
_currentViewBounds( ),
_lastViewBounds( ),
_renderingIsPaused( false),
_threadID( )
{
    
    

    _screenUpdateThread = ThreadWrapper::CreateThread(ScreenUpdateThreadProc, this, kRealtimePriority);
    _screenUpdateEvent = EventWrapper::Create();

    GetWindowRect(_windowRect);

    _lastViewBounds.origin.x = 0;
    _lastViewBounds.origin.y = 0;
    _lastViewBounds.size.width = 0;
    _lastViewBounds.size.height = 0;

#ifdef NEW_HIVIEW_PARENT_EVENT_HANDLER
    
    

    


    static const EventTypeSpec windowEventTypes[] =
    {
        kEventClassWindow, kEventWindowBoundsChanged,
        kEventClassWindow, kEventWindowBoundsChanging,
        kEventClassWindow, kEventWindowZoomed,
        kEventClassWindow, kEventWindowExpanded,
        kEventClassWindow, kEventWindowClickResizeRgn,
        kEventClassWindow, kEventWindowClickDragRgn
    };

    WindowRef parentWindow = HIViewGetWindow(windowRef);

    InstallWindowEventHandler (parentWindow,
            NewEventHandlerUPP (sHandleWindowResized),
            GetEventTypeCount(windowEventTypes),
            windowEventTypes,
            (void *) this, 
            &_windowEventHandlerRef);

#endif

#ifdef NEW_HIVIEW_EVENT_HANDLER	
    

    static const EventTypeSpec hiviewEventTypes[] =
    {
        kEventClassControl, kEventControlBoundsChanged,
        kEventClassControl, kEventControlDraw
        
        
        
        
        
        

    };

    HIViewInstallEventHandler(_hiviewRef,
            NewEventHandlerUPP(sHandleHiViewResized),
            GetEventTypeCount(hiviewEventTypes),
            hiviewEventTypes,
            (void *) this,
            &_hiviewEventHandlerRef);

#endif
}



#pragma mark HIViewRef Event Handler
pascal OSStatus VideoRenderAGL::sHandleHiViewResized (EventHandlerCallRef nextHandler, EventRef theEvent, void* userData)
{
    
    HIViewRef hiviewRef = NULL;

    
    int eventType = GetEventKind(theEvent);
    OSStatus status = noErr;
    status = GetEventParameter (theEvent,
            kEventParamDirectObject,
            typeControlRef,
            NULL,
            sizeof (ControlRef),
            NULL,
            &hiviewRef);

    VideoRenderAGL* obj = (VideoRenderAGL*)(userData);
    WindowRef parentWindow = HIViewGetWindow(hiviewRef);
    bool updateUI = true;

    if(kEventControlBoundsChanged == eventType)
    {
    }
    else if(kEventControlDraw == eventType)
    {
    }
    else
    {
        updateUI = false;
    }

    if(true == updateUI)
    {
        obj->ParentWindowResized(parentWindow);
        obj->UpdateClipping();
        obj->RenderOffScreenBuffers();
    }

    return status;
}

VideoRenderAGL::~VideoRenderAGL()
{

    


#ifdef USE_EVENT_HANDLERS
    
    OSStatus status;
    if(_isHIViewRef)
    {
        status = RemoveEventHandler(_hiviewEventHandlerRef);
    }
    else
    {
        status = RemoveEventHandler(_windowEventHandlerRef);
    }
    if(noErr != status)
    {
        if(_isHIViewRef)
        {

            
        }
        else
        {
            
        }
    }

#endif

    OSStatus status;
#ifdef NEW_HIVIEW_PARENT_EVENT_HANDLER
    if(_windowEventHandlerRef)
    {
        status = RemoveEventHandler(_windowEventHandlerRef);
        if(status != noErr)
        {
            
        }
    }
#endif

#ifdef NEW_HIVIEW_EVENT_HANDLER	
    if(_hiviewEventHandlerRef)
    {
        status = RemoveEventHandler(_hiviewEventHandlerRef);
        if(status != noErr)
        {
            
        }
    }
#endif

    
    ThreadWrapper* tmpPtr = _screenUpdateThread;
    _screenUpdateThread = NULL;

    if (tmpPtr)
    {
        tmpPtr->SetNotAlive();
        _screenUpdateEvent->Set();
        _screenUpdateEvent->StopTimer();

        if (tmpPtr->Stop())
        {
            delete tmpPtr;
        }
        delete _screenUpdateEvent;
        _screenUpdateEvent = NULL;
    }

    if (_aglContext != 0)
    {
        aglSetCurrentContext(_aglContext);
        aglDestroyContext(_aglContext);
        _aglContext = 0;
    }

    
    std::map<int, VideoChannelAGL*>::iterator it = _aglChannels.begin();
    while (it!= _aglChannels.end())
    {
        delete it->second;
        _aglChannels.erase(it);
        it = _aglChannels.begin();
    }
    _aglChannels.clear();

    
    std::multimap<int, int>::iterator zIt = _zOrderToChannel.begin();
    while(zIt != _zOrderToChannel.end())
    {
        _zOrderToChannel.erase(zIt);
        zIt = _zOrderToChannel.begin();
    }
    _zOrderToChannel.clear();

    


}

int VideoRenderAGL::GetOpenGLVersion(int& aglMajor, int& aglMinor)
{
    aglGetVersion((GLint *) &aglMajor, (GLint *) &aglMinor);
    return 0;
}

int VideoRenderAGL::Init()
{
    LockAGLCntx();

    
    if (!_screenUpdateThread)
    {
        UnlockAGLCntx();
        
        return -1;
    }
    unsigned int threadId;
    _screenUpdateThread->Start(threadId);

    
    unsigned int monitorFreq = 60;
    _screenUpdateEvent->StartTimer(true, 1000/monitorFreq);

    
    if (CreateMixingContext() == -1)
    {
        
        UnlockAGLCntx();
        return -1;
    }

    UnlockAGLCntx();
    return 0;
}

VideoChannelAGL* VideoRenderAGL::CreateAGLChannel(int channel, int zOrder, float startWidth, float startHeight, float stopWidth, float stopHeight)
{

    LockAGLCntx();

    

    if (HasChannel(channel))
    {
        
        UnlockAGLCntx();k
        return NULL;
    }

    if (_zOrderToChannel.find(zOrder) != _zOrderToChannel.end())
    {
        
        
    }

    VideoChannelAGL* newAGLChannel = new VideoChannelAGL(_aglContext, _id, this);

    if (newAGLChannel->SetStreamSettings(0, startWidth, startHeight, stopWidth, stopHeight) == -1)
    {
        if (newAGLChannel)
        {
            delete newAGLChannel;
            newAGLChannel = NULL;
        }
        
        
        UnlockAGLCntx();
        return NULL;
    }
k
    _aglChannels[channel] = newAGLChannel;
    _zOrderToChannel.insert(std::pair<int, int>(zOrder, channel));

    UnlockAGLCntx();
    return newAGLChannel;
}

int VideoRenderAGL::DeleteAllAGLChannels()
{
    CriticalSectionScoped cs(&_renderCritSec);

    
    
    std::map<int, VideoChannelAGL*>::iterator it;
    it = _aglChannels.begin();

    while (it != _aglChannels.end())
    {
        VideoChannelAGL* channel = it->second;
        if (channel)
        delete channel;

        _aglChannels.erase(it);
        it = _aglChannels.begin();
    }
    _aglChannels.clear();
    return 0;
}

int VideoRenderAGL::DeleteAGLChannel(int channel)
{
    CriticalSectionScoped cs(&_renderCritSec);
    

    std::map<int, VideoChannelAGL*>::iterator it;
    it = _aglChannels.find(channel);
    if (it != _aglChannels.end())
    {
        delete it->second;
        _aglChannels.erase(it);
    }
    else
    {
        
        return -1;
    }

    std::multimap<int, int>::iterator zIt = _zOrderToChannel.begin();
    while( zIt != _zOrderToChannel.end())
    {
        if (zIt->second == channel)
        {
            _zOrderToChannel.erase(zIt);
            break;
        }
        zIt++;
    }

    return 0;
}

int VideoRenderAGL::StopThread()
{
    CriticalSectionScoped cs(&_renderCritSec);
    ThreadWrapper* tmpPtr = _screenUpdateThread;
    

    if (tmpPtr)
    {
        tmpPtr->SetNotAlive();
        _screenUpdateEvent->Set();
        if (tmpPtr->Stop())
        {
            delete tmpPtr;
        }
    }

    delete _screenUpdateEvent;
    _screenUpdateEvent = NULL;

    return 0;
}

bool VideoRenderAGL::IsFullScreen()
{
    CriticalSectionScoped cs(&_renderCritSec);
    return _fullScreen;
}

bool VideoRenderAGL::HasChannels()
{

    CriticalSectionScoped cs(&_renderCritSec);

    if (_aglChannels.begin() != _aglChannels.end())
    {
        return true;
    }

    return false;
}

bool VideoRenderAGL::HasChannel(int channel)
{
    CriticalSectionScoped cs(&_renderCritSec);

    std::map<int, VideoChannelAGL*>::iterator it = _aglChannels.find(channel);
    if (it != _aglChannels.end())
    {
        return true;
    }

    return false;
}

int VideoRenderAGL::GetChannels(std::list<int>& channelList)
{

    CriticalSectionScoped cs(&_renderCritSec);
    std::map<int, VideoChannelAGL*>::iterator it = _aglChannels.begin();

    while (it != _aglChannels.end())
    {
        channelList.push_back(it->first);
        it++;
    }

    return 0;
}

VideoChannelAGL* VideoRenderAGL::ConfigureAGLChannel(int channel, int zOrder, float startWidth, float startHeight, float stopWidth, float stopHeight)
{

    CriticalSectionScoped cs(&_renderCritSec);

    std::map<int, VideoChannelAGL*>::iterator it = _aglChannels.find(channel);

    if (it != _aglChannels.end())
    {
        VideoChannelAGL* aglChannel = it->second;
        if (aglChannel->SetStreamSettings(0, startWidth, startHeight, stopWidth, stopHeight) == -1)
        {
            return NULL;
        }

        std::multimap<int, int>::iterator it = _zOrderToChannel.begin();
        while(it != _zOrderToChannel.end())
        {
            if (it->second == channel)
            {
                if (it->first != zOrder)
                {
                    _zOrderToChannel.erase(it);
                    _zOrderToChannel.insert(std::pair<int, int>(zOrder, channel));
                }
                break;
            }
            it++;
        }
        return aglChannel;
    }

    return NULL;
}

bool VideoRenderAGL::ScreenUpdateThreadProc(void* obj)
{
    return static_cast<VideoRenderAGL*>(obj)->ScreenUpdateProcess();
}

bool VideoRenderAGL::ScreenUpdateProcess()
{
    _screenUpdateEvent->Wait(100);

    LockAGLCntx();

    if (!_screenUpdateThread)
    {
        UnlockAGLCntx();
        return false;
    }

    if (aglSetCurrentContext(_aglContext) == GL_FALSE)
    {
        UnlockAGLCntx();
        return true;
    }

    if (GetWindowRect(_windowRect) == -1)
    {
        UnlockAGLCntx();
        return true;
    }

    if (_windowWidth != (_windowRect.right - _windowRect.left)
            || _windowHeight != (_windowRect.bottom - _windowRect.top))
    {
        
        if (aglUpdateContext(_aglContext) == GL_FALSE)
        {
            UnlockAGLCntx();
            return true;
        }
        _windowWidth = _windowRect.right - _windowRect.left;
        _windowHeight = _windowRect.bottom - _windowRect.top;
    }

    
    
    
#ifndef NEW_HIVIEW_PARENT_EVENT_HANDLER
    if (_isHIViewRef)
    {

        if(FALSE == HIViewIsValid(_hiviewRef))
        {

            
            UnlockAGLCntx();
            return true;
        }
        WindowRef window = HIViewGetWindow(_hiviewRef);

        if(FALSE == IsValidWindowPtr(window))
        {
            
            UnlockAGLCntx();
            return true;
        }
        if (window == NULL)
        {
            
            UnlockAGLCntx();
            return true;
        }

        if(FALSE == MacIsWindowVisible(window))
        {
            
            UnlockAGLCntx();
            return true;
        }

        HIRect viewBounds; 
        int windowWidth = 0; 
        int windowHeight = 0; 

        
        
        Rect contentBounds =
        {   0, 0, 0, 0}; 

#if		defined(USE_CONTENT_RGN)
        GetWindowBounds(window, kWindowContentRgn, &contentBounds);
#elif	defined(USE_STRUCT_RGN)
        GetWindowBounds(window, kWindowStructureRgn, &contentBounds);
#endif

        Rect globalBounds =
        {   0, 0, 0, 0}; 
        globalBounds.top = contentBounds.top;
        globalBounds.right = contentBounds.right;
        globalBounds.bottom = contentBounds.bottom;
        globalBounds.left = contentBounds.left;

        windowHeight = globalBounds.bottom - globalBounds.top;
        windowWidth = globalBounds.right - globalBounds.left;

        
        HIViewGetBounds(_hiviewRef, &viewBounds);
        HIViewConvertRect(&viewBounds, _hiviewRef, NULL);

        
        if (_lastWindowHeight == -1 &&
                _lastWindowWidth == -1)
        {
            _lastWindowWidth = windowWidth;
            _lastWindowHeight = windowHeight;

            _lastViewBounds.origin.x = viewBounds.origin.x;
            _lastViewBounds.origin.y = viewBounds.origin.y;
            _lastViewBounds.size.width = viewBounds.size.width;
            _lastViewBounds.size.height = viewBounds.size.height;
        }
        sfasdfasdf

        bool resized = false;

        
        if (windowHeight != _lastWindowHeight ||
                windowWidth != _lastWindowWidth)
        {
            resized = true;
        }

        
        if (_lastViewBounds.origin.x != viewBounds.origin.x ||
                _lastViewBounds.origin.y != viewBounds.origin.y ||
                _lastViewBounds.size.width != viewBounds.size.width ||
                _lastViewBounds.size.height != viewBounds.size.height)
        {
            
            resized = true;
        }

        if (resized)
        {

            

            
            
            const GLint offs[4] =
            {   (int)(0.5f + viewBounds.origin.x),
                (int)(0.5f + windowHeight - (viewBounds.origin.y + viewBounds.size.height)),
                viewBounds.size.width, viewBounds.size.height};

            
            contentBounds.top, contentBounds.right, contentBounds.bottom, contentBounds.left);
            
            

            aglSetDrawable (_aglContext, GetWindowPort(window));
            aglSetInteger(_aglContext, AGL_BUFFER_RECT, offs);
            aglEnable(_aglContext, AGL_BUFFER_RECT);

            
            glViewport(0.0f, 0.0f, (GLsizei) viewBounds.size.width, (GLsizei) viewBounds.size.height);

        }
        _lastWindowWidth = windowWidth;
        _lastWindowHeight = windowHeight;

        _lastViewBounds.origin.x = viewBounds.origin.x;
        _lastViewBounds.origin.y = viewBounds.origin.y;
        _lastViewBounds.size.width = viewBounds.size.width;
        _lastViewBounds.size.height = viewBounds.size.height;

    }
#endif
    if (_fullScreen)
    {
        
        
        
    }
    else
    {
        
        bool updated = false;

        
        
        std::map<int, VideoChannelAGL*>::iterator it = _aglChannels.begin();
        while (it != _aglChannels.end())
        {

            VideoChannelAGL* aglChannel = it->second;
            aglChannel->UpdateStretchSize(_windowHeight, _windowWidth);
            aglChannel->IsUpdated(updated);
            if (updated)
            {
                break;
            }
            it++;
        }

        if (updated)
        {
            
            if (RenderOffScreenBuffers() != -1)
            {
                
                
            }
            else
            {
                
            }
        }
    }

    UnlockAGLCntx();

    
    return true;
}

void VideoRenderAGL::ParentWindowResized(WindowRef window)
{
    

    LockAGLCntx();
k
    
    _windowHasResized = false;

    if(FALSE == HIViewIsValid(_hiviewRef))
    {
        
        UnlockAGLCntx();
        return;
    }

    if(FALSE == IsValidWindowPtr(window))
    {
        
        UnlockAGLCntx();
        return;
    }

    if (window == NULL)
    {
        
        UnlockAGLCntx();
        return;
    }

    if(FALSE == MacIsWindowVisible(window))
    {
        
        UnlockAGLCntx();
        return;
    }

    Rect contentBounds =
    {   0, 0, 0, 0};

#if		defined(USE_CONTENT_RGN)
    GetWindowBounds(window, kWindowContentRgn, &contentBounds);
#elif	defined(USE_STRUCT_RGN)
    GetWindowBounds(window, kWindowStructureRgn, &contentBounds);
#endif

    

    
    _currentParentWindowBounds.top = contentBounds.top;
    _currentParentWindowBounds.left = contentBounds.left;
    _currentParentWindowBounds.bottom = contentBounds.bottom;
    _currentParentWindowBounds.right = contentBounds.right;

    _currentParentWindowWidth = _currentParentWindowBounds.right - _currentParentWindowBounds.left;
    _currentParentWindowHeight = _currentParentWindowBounds.bottom - _currentParentWindowBounds.top;

    _windowHasResized = true;

    
    HIRect viewBounds;
    HIViewGetBounds(_hiviewRef, &viewBounds);
    HIViewConvertRect(&viewBounds, _hiviewRef, NULL);

    const GLint offs[4] =
    {   (int)(0.5f + viewBounds.origin.x),
        (int)(0.5f + _currentParentWindowHeight - (viewBounds.origin.y + viewBounds.size.height)),
        viewBounds.size.width, viewBounds.size.height};
    
    

    aglSetCurrentContext(_aglContext);
    aglSetDrawable (_aglContext, GetWindowPort(window));
    aglSetInteger(_aglContext, AGL_BUFFER_RECT, offs);
    aglEnable(_aglContext, AGL_BUFFER_RECT);

    
    glViewport(0.0f, 0.0f, (GLsizei) viewBounds.size.width, (GLsizei) viewBounds.size.height);

    UnlockAGLCntx();

    return;
}

int VideoRenderAGL::CreateMixingContext()
{

    LockAGLCntx();

    

    
    

    GLint attributes[] =
    {
        AGL_DOUBLEBUFFER,
        AGL_WINDOW,
        AGL_RGBA,
        AGL_NO_RECOVERY,
        AGL_ACCELERATED,
        AGL_RED_SIZE, 8,
        AGL_GREEN_SIZE, 8,
        AGL_BLUE_SIZE, 8,
        AGL_ALPHA_SIZE, 8,
        AGL_DEPTH_SIZE, 24,
        AGL_NONE,
    };

    AGLPixelFormat aglPixelFormat;

    

    
    aglPixelFormat = aglChoosePixelFormat(NULL, 0, attributes);
    if (NULL == aglPixelFormat)
    {
        
        UnlockAGLCntx();
        return -1;
    }

    
    _aglContext = aglCreateContext(aglPixelFormat, NULL);
    if (_aglContext == NULL)
    {
        
        UnlockAGLCntx();
        return -1;
    }

    
    aglDestroyPixelFormat(aglPixelFormat);

    
    if (aglSetCurrentContext(_aglContext) == false)
    {
        
        UnlockAGLCntx();
        return -1;
    }

    if (_isHIViewRef)
    {
        
        
#if 0
        
        
        
        
        if (aglSetHIViewRef(_aglContext,_hiviewRef) == false)
        {
            
            UnlockAGLCntx();
            return -1;
        }
#else

        
        WindowRef window = GetControlOwner(_hiviewRef);

        Rect globalBounds =
        {   0,0,0,0}; 
        HIRect viewBounds; 
        int windowHeight = 0;

        
        
        
        
        
        
        
        


        
#if		defined(USE_CONTENT_RGN)
        GetWindowBounds(window, kWindowContentRgn, &globalBounds);
#elif	defined(USE_STRUCT_RGN)
        GetWindowBounds(window, kWindowStructureRgn, &globalBounds);
#endif
        windowHeight = globalBounds.bottom - globalBounds.top;

        
        HIViewGetBounds(_hiviewRef, &viewBounds);

        HIViewConvertRect(&viewBounds, _hiviewRef, NULL);

        const GLint offs[4] =
        {   (int)(0.5f + viewBounds.origin.x),
            (int)(0.5f + windowHeight - (viewBounds.origin.y + viewBounds.size.height)),
            viewBounds.size.width, viewBounds.size.height};

        


        aglSetDrawable (_aglContext, GetWindowPort(window));
        aglSetInteger(_aglContext, AGL_BUFFER_RECT, offs);
        aglEnable(_aglContext, AGL_BUFFER_RECT);

        GLint surfaceOrder = 1; 
        
        aglSetInteger(_aglContext, AGL_SURFACE_ORDER, &surfaceOrder);

        glViewport(0.0f, 0.0f, (GLsizei) viewBounds.size.width, (GLsizei) viewBounds.size.height);
#endif

    }
    else
    {
        if(GL_FALSE == aglSetDrawable (_aglContext, GetWindowPort(_windowRef)))
        {
            
            UnlockAGLCntx();
            return -1;
        }
    }

    _windowWidth = _windowRect.right - _windowRect.left;
    _windowHeight = _windowRect.bottom - _windowRect.top;

    
    int surfaceOpacity = 1;
    if (aglSetInteger(_aglContext, AGL_SURFACE_OPACITY, (const GLint *) &surfaceOpacity) == false)
    {
        
        UnlockAGLCntx();
        return -1;
    }

    
    
    int swapInterval = 0; 
    if (aglSetInteger(_aglContext, AGL_SWAP_INTERVAL, (const GLint *) &swapInterval) == false)
    {
        
        UnlockAGLCntx();
        return -1;
    }

    
    if (GetWindowRect(_windowRect) == -1)
    {
        
        UnlockAGLCntx();
        return -1;
    }

    
    glDisable(GL_DITHER);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_FOG);
    glDisable(GL_TEXTURE_2D);
    glPixelZoom(1.0, 1.0);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    GLenum glErr = glGetError();

    if (glErr)
    {
    }

    UpdateClipping();

    

    UnlockAGLCntx();
    return 0;
}

int VideoRenderAGL::RenderOffScreenBuffers()
{
    LockAGLCntx();

    
    if (GetWindowRect(_windowRect) == -1)
    {
        
        UnlockAGLCntx();
        return -1;
    }

    if (aglSetCurrentContext(_aglContext) == false)
    {
        
        UnlockAGLCntx();
        return -1;
    }

    
    glClear(GL_COLOR_BUFFER_BIT);

    
    for (std::multimap<int, int>::reverse_iterator rIt = _zOrderToChannel.rbegin();
    rIt != _zOrderToChannel.rend();
    rIt++)
    {
        int channelId = rIt->second;
        std::map<int, VideoChannelAGL*>::iterator it = _aglChannels.find(channelId);

        VideoChannelAGL* aglChannel = it->second;

        aglChannel->RenderOffScreenBuffer();
    }

    SwapAndDisplayBuffers();

    UnlockAGLCntx();
    return 0;
}

int VideoRenderAGL::SwapAndDisplayBuffers()
{

    LockAGLCntx();
    if (_fullScreen)
    {
        
        
        
        
        
    }
    else
    {
        
        glFlush();
        aglSwapBuffers(_aglContext);
        HIViewSetNeedsDisplay(_hiviewRef, true);
    }

    UnlockAGLCntx();
    return 0;
}

int VideoRenderAGL::GetWindowRect(Rect& rect)
{

    LockAGLCntx();

    if (_isHIViewRef)
    {
        if (_hiviewRef)
        {
            HIRect HIViewRect1;
            if(FALSE == HIViewIsValid(_hiviewRef))
            {
                rect.top = 0;
                rect.left = 0;
                rect.right = 0;
                rect.bottom = 0;
                
                UnlockAGLCntx();
            }
            HIViewGetBounds(_hiviewRef,&HIViewRect1);
            HIRectConvert(&HIViewRect1, 1, NULL, 2, NULL);
            if(HIViewRect1.origin.x < 0)
            {
                rect.top = 0;
                
            }
            else
            {
                rect.top = HIViewRect1.origin.x;
            }

            if(HIViewRect1.origin.y < 0)
            {
                rect.left = 0;
                
            }
            else
            {
                rect.left = HIViewRect1.origin.y;
            }

            if(HIViewRect1.size.width < 0)
            {
                rect.right = 0;
                
            }
            else
            {
                rect.right = HIViewRect1.size.width;
            }

            if(HIViewRect1.size.height < 0)
            {
                rect.bottom = 0;
                
            }
            else
            {
                rect.bottom = HIViewRect1.size.height;
            }

            
            UnlockAGLCntx();
        }
        else
        {
            
            UnlockAGLCntx();
        }
    }
    else
    {
        if (_windowRef)
        {
            GetWindowBounds(_windowRef, kWindowContentRgn, &rect);
            UnlockAGLCntx();
        }
        else
        {
            
            UnlockAGLCntx();
        }
    }
}

int VideoRenderAGL::UpdateClipping()
{
    
    LockAGLCntx();

    if(_isHIViewRef)
    {
        if(FALSE == HIViewIsValid(_hiviewRef))
        {
            
            UnlockAGLCntx();
            return -1;
        }

        RgnHandle visibleRgn = NewRgn();
        SetEmptyRgn (visibleRgn);

        if(-1 == CalculateVisibleRegion((ControlRef)_hiviewRef, visibleRgn, true))
        {
        }

        if(GL_FALSE == aglSetCurrentContext(_aglContext))
        {
            GLenum glErr = aglGetError();
            
        }

        if(GL_FALSE == aglEnable(_aglContext, AGL_CLIP_REGION))
        {
            GLenum glErr = aglGetError();
            
        }

        if(GL_FALSE == aglSetInteger(_aglContext, AGL_CLIP_REGION, (const GLint*)visibleRgn))
        {
            GLenum glErr = aglGetError();
            
        }

        DisposeRgn(visibleRgn);
    }
    else
    {
        
    }

    
    UnlockAGLCntx();
    return true;
}

int VideoRenderAGL::CalculateVisibleRegion(ControlRef control, RgnHandle &visibleRgn, bool clipChildren)
{

    

    
    OSStatus osStatus = 0;
    OSErr osErr = 0;

    RgnHandle tempRgn = NewRgn();
    if (IsControlVisible(control))
    {
        RgnHandle childRgn = NewRgn();
        WindowRef window = GetControlOwner(control);
        ControlRef rootControl;
        GetRootControl(window, &rootControl); 
        ControlRef masterControl;
        osStatus = GetSuperControl(rootControl, &masterControl);
        

        if (masterControl != NULL)
        {
            CheckValidRegion(visibleRgn);
            
            osStatus = GetControlRegion(rootControl, kControlStructureMetaPart, visibleRgn);
            
            
            ControlRef tempControl = control, lastControl = 0;
            while (tempControl != masterControl) 

            {
                CheckValidRegion(tempRgn);

                
                ControlRef subControl;

                osStatus = GetControlRegion(tempControl, kControlStructureMetaPart, tempRgn); 
                
                CheckValidRegion(tempRgn);

                osErr = HIViewConvertRegion(tempRgn, tempControl, rootControl);
                
                CheckValidRegion(tempRgn);

                SectRgn(tempRgn, visibleRgn, visibleRgn);
                CheckValidRegion(tempRgn);
                CheckValidRegion(visibleRgn);
                if (EmptyRgn(visibleRgn)) 
                break;

                if (clipChildren || tempControl != control) 

                {
                    UInt16 numChildren;
                    osStatus = CountSubControls(tempControl, &numChildren); 
                    

                    
                    for (int i = 0; i < numChildren; i++)
                    {
                        osErr = GetIndexedSubControl(tempControl, numChildren - i, &subControl); 
                        
                        if ( subControl == lastControl ) 

                        {
                            
                            break;
                        }

                        if (!IsControlVisible(subControl)) 

                        {
                            
                            continue;
                        }

                        if(!subControl) continue;

                        osStatus = GetControlRegion(subControl, kControlStructureMetaPart, tempRgn); 
                        
                        CheckValidRegion(tempRgn);
                        if(osStatus != 0)
                        {
                            
                            continue;
                        }
                        if(!tempRgn)
                        {
                            
                            continue;
                        }

                        osStatus = HIViewConvertRegion(tempRgn, subControl, rootControl);
                        CheckValidRegion(tempRgn);
                        
                        if(osStatus != 0)
                        {
                            
                            continue;
                        }
                        if(!rootControl)
                        {
                            
                            continue;
                        }

                        UnionRgn(tempRgn, childRgn, childRgn);
                        CheckValidRegion(tempRgn);
                        CheckValidRegion(childRgn);
                        CheckValidRegion(visibleRgn);
                        if(!childRgn)
                        {
                            
                            continue;
                        }

                    } 
                }
                lastControl = tempControl;
                GetSuperControl(tempControl, &subControl);
                tempControl = subControl;
            }

            DiffRgn(visibleRgn, childRgn, visibleRgn);
            CheckValidRegion(visibleRgn);
            CheckValidRegion(childRgn);
            DisposeRgn(childRgn);
        }
        else
        {
            CopyRgn(tempRgn, visibleRgn);
            CheckValidRegion(tempRgn);
            CheckValidRegion(visibleRgn);
        }
        DisposeRgn(tempRgn);
    }

    
    
    return 0;
}

bool VideoRenderAGL::CheckValidRegion(RgnHandle rHandle)
{

    Handle hndSize = (Handle)rHandle;
    long size = GetHandleSize(hndSize);
    if(0 == size)
    {

        OSErr memErr = MemError();
        if(noErr != memErr)
        {
            
        }
        else
        {
            
        }

    }
    else
    {
        
    }

    if(false == IsValidRgnHandle(rHandle))
    {
        
        assert(false);
    }

    int err = QDError();
    switch(err)
    {
        case 0:
        break;
        case -147:
        
        assert(false);
        break;

        case -149:
        
        assert(false);
        break;

        default:
        
        assert(false);
        break;
    }

    return true;
}

int VideoRenderAGL::ChangeWindow(void* newWindowRef)
{

    LockAGLCntx();

    UnlockAGLCntx();
    return -1;
}
WebRtc_Word32 VideoRenderAGL::ChangeUniqueID(WebRtc_Word32 id)
{
    LockAGLCntx();

    UnlockAGLCntx();
    return -1;
}

WebRtc_Word32 VideoRenderAGL::StartRender()
{

    LockAGLCntx();
    const unsigned int MONITOR_FREQ = 60;
    if(TRUE == _renderingIsPaused)
    {
        

        
        if(FALSE == _screenUpdateThread->Start(_threadID))
        {
            
            UnlockAGLCntx();
            return -1;
        }
        if(FALSE == _screenUpdateEvent->StartTimer(true, 1000/MONITOR_FREQ))
        {
            
            UnlockAGLCntx();
            return -1;
        }

        return 0;
    }

    _screenUpdateThread = ThreadWrapper::CreateThread(ScreenUpdateThreadProc, this, kRealtimePriority);
    _screenUpdateEvent = EventWrapper::Create();

    if (!_screenUpdateThread)
    {
        
        UnlockAGLCntx();
        return -1;
    }

    _screenUpdateThread->Start(_threadID);
    _screenUpdateEvent->StartTimer(true, 1000/MONITOR_FREQ);

    

    UnlockAGLCntx();
    return 0;

}

WebRtc_Word32 VideoRenderAGL::StopRender()
{
    LockAGLCntx();

    if(!_screenUpdateThread || !_screenUpdateEvent)
    {
        _renderingIsPaused = TRUE;
        UnlockAGLCntx();
        return 0;
    }

    if(FALSE == _screenUpdateThread->Stop() || FALSE == _screenUpdateEvent->StopTimer())
    {
        _renderingIsPaused = FALSE;
        
        UnlockAGLCntx();
        return -1;
    }

    _renderingIsPaused = TRUE;

    
    UnlockAGLCntx();
    return 0;
}

WebRtc_Word32 VideoRenderAGL::DeleteAGLChannel(const WebRtc_UWord32 streamID)
{

    LockAGLCntx();

    std::map<int, VideoChannelAGL*>::iterator it;
    it = _aglChannels.begin();

    while (it != _aglChannels.end())
    {
        VideoChannelAGL* channel = it->second;
        
        delete channel;
        it++;
    }
    _aglChannels.clear();

    UnlockAGLCntx();
    return 0;
}

WebRtc_Word32 VideoRenderAGL::GetChannelProperties(const WebRtc_UWord16 streamId,
WebRtc_UWord32& zOrder,
float& left,
float& top,
float& right,
float& bottom)
{

    LockAGLCntx();
    UnlockAGLCntx();
    return -1;

}

void VideoRenderAGL::LockAGLCntx()
{
    _renderCritSec.Enter();
}
void VideoRenderAGL::UnlockAGLCntx()
{
    _renderCritSec.Leave();
}

} 

#endif   

