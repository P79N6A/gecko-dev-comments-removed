









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_WINDOWS_VIDEO_RENDER_DIRECTDRAW_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_WINDOWS_VIDEO_RENDER_DIRECTDRAW_H_

#include "typedefs.h"
#include "i_video_render_win.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"

#include "ddraw.h"
#include <Map>
#include <List>


#include "video_render_defines.h"

#pragma comment(lib, "ddraw.lib")       // located in DirectX SDK

namespace webrtc {
class CriticalSectionWrapper;
class EventWrapper;
class ThreadWrapper;
class Trace;

class VideoRenderDirectDraw;


typedef IDirectDraw7 DirectDraw;
typedef IDirectDrawSurface7 DirectDrawSurface;
typedef DDSURFACEDESC2 DirectDrawSurfaceDesc;
typedef DDSCAPS2 DirectDrawCaps;
typedef std::pair<int, unsigned int> ZorderPair;

class WindowsThreadCpuUsage
{
public:
    WindowsThreadCpuUsage();
    int GetCpuUsage(); 
    DWORD LastGetCpuTime()
    {
        return _lastGetCpuUsageTime;
    }
    const enum
    {
        CPU_CHECK_INTERVAL = 1000
    };
private:
    _int64 _lastCpuUsageTime;
    DWORD _lastGetCpuUsageTime;
    int _lastCpuUsage;
    HANDLE _hThread;
    int _cores;
};

class DirectDrawStreamSettings
{
public:
    DirectDrawStreamSettings();

    float _startWidth;
    float _stopWidth;
    float _startHeight;
    float _stopHeight;

    float _cropStartWidth;
    float _cropStopWidth;
    float _cropStartHeight;
    float _cropStopHeight;
};

class DirectDrawBitmapSettings
{
public:
    DirectDrawBitmapSettings();
    ~DirectDrawBitmapSettings();

    int SetBitmap(Trace* trace, DirectDraw* directDraw);

    HBITMAP _transparentBitMap;
    float _transparentBitmapLeft;
    float _transparentBitmapRight;
    float _transparentBitmapTop;
    float _transparentBitmapBottom;
    int _transparentBitmapWidth;
    int _transparentBitmapHeight;
    DDCOLORKEY* _transparentBitmapColorKey;
    DirectDrawSurface* _transparentBitmapSurface; 
};

class DirectDrawTextSettings
{
public:
    DirectDrawTextSettings();
    ~DirectDrawTextSettings();

    int SetText(const char* text, int textLength, COLORREF colorText,
                COLORREF colorBg, float left, float top, float right,
                float bottom);

    char* _ptrText;
    WebRtc_UWord32 _textLength;
    COLORREF _colorRefText;
    COLORREF _colorRefBackground;
    float _textLeft;
    float _textRight;
    float _textTop;
    float _textBottom;
    bool _transparent;
};

class DirectDrawChannel: public VideoRenderCallback
{
public:
    DirectDrawChannel(DirectDraw* directDraw,
                          VideoType blitVideoType,
                          VideoType incomingVideoType,
                          VideoType screenVideoType,
                          VideoRenderDirectDraw* owner);

    int FrameSizeChange(int width, int height, int numberOfStreams);
    int DeliverFrame(unsigned char* buffer, int buffeSize,
                     unsigned int timeStamp90KHz);
    virtual WebRtc_Word32 RenderFrame(const WebRtc_UWord32 streamId,
                                      VideoFrame& videoFrame);

    int ChangeDeliverColorFormat(bool useScreenType);

    void AddRef();
    void Release();

    void SetStreamSettings(VideoRenderDirectDraw* DDObj, short streamId,
                           float startWidth, float startHeight,
                           float stopWidth, float stopHeight);
    void SetStreamCropSettings(VideoRenderDirectDraw* DDObj,
                               short streamId, float startWidth,
                               float startHeight, float stopWidth,
                               float stopHeight);

    int GetStreamSettings(VideoRenderDirectDraw* DDObj, short streamId,
                          float& startWidth, float& startHeight,
                          float& stopWidth, float& stopHeight);

    void GetLargestSize(RECT* mixingRect);
    int
            BlitFromOffscreenBufferToMixingBuffer(
                                                  VideoRenderDirectDraw* DDObj,
                                                  short streamID,
                                                  DirectDrawSurface* mixingSurface,
                                                  RECT &dstRect, bool demuxing);
    bool IsOffScreenSurfaceUpdated(VideoRenderDirectDraw* DDobj);

protected:
    virtual ~DirectDrawChannel();

private:
    CriticalSectionWrapper* _critSect; 
    int _refCount;
    int _width;
    int _height;
    int _numberOfStreams;
    bool _deliverInScreenType;
    bool _doubleBuffer;
    DirectDraw* _directDraw;
    DirectDrawSurface* _offScreenSurface; 
    DirectDrawSurface* _offScreenSurfaceNext; 
    VideoType _blitVideoType;
    VideoType _originalBlitVideoType;
    VideoType _incomingVideoType;
    VideoType _screenVideoType;
    enum
    {
        MAX_FRAMEDELIVER_TIME = 20
    }; 
    enum
    {
        MAX_NO_OF_LATE_FRAMEDELIVER_TIME = 10
    }; 
    VideoFrame _tempRenderBuffer;

    std::map<unsigned long long, DirectDrawStreamSettings*>
            _streamIdToSettings;
    bool _offScreenSurfaceUpdated;
    VideoRenderDirectDraw* _owner;
};

class VideoRenderDirectDraw: IVideoRenderWin
{
public:
    VideoRenderDirectDraw(Trace* trace, HWND hWnd, bool fullscreen);
    ~VideoRenderDirectDraw();
public:
    

    




    virtual WebRtc_Word32 Init();

    




    virtual VideoRenderCallback
            * CreateChannel(const WebRtc_UWord32 streamId,
                            const WebRtc_UWord32 zOrder, const float left,
                            const float top, const float right,
                            const float bottom);

    virtual WebRtc_Word32 DeleteChannel(const WebRtc_UWord32 streamId);

    virtual WebRtc_Word32 GetStreamSettings(const WebRtc_UWord32 channel,
                                            const WebRtc_UWord16 streamId,
                                            WebRtc_UWord32& zOrder,
                                            float& left, float& top,
                                            float& right, float& bottom);

    





    virtual WebRtc_Word32 StartRender();
    virtual WebRtc_Word32 StopRender();

    





    virtual bool IsFullScreen();

    virtual WebRtc_Word32 SetCropping(const WebRtc_UWord32 channel,
                                      const WebRtc_UWord16 streamId,
                                      const float left, const float top,
                                      const float right, const float bottom);

    virtual WebRtc_Word32 SetTransparentBackground(const bool enable);

    virtual WebRtc_Word32 ChangeWindow(void* window);

    virtual WebRtc_Word32 GetGraphicsMemory(WebRtc_UWord64& totalMemory,
                                            WebRtc_UWord64& availableMemory);

    virtual WebRtc_Word32 SetText(const WebRtc_UWord8 textId,
                                  const WebRtc_UWord8* text,
                                  const WebRtc_Word32 textLength,
                                  const WebRtc_UWord32 colorText,
                                  const WebRtc_UWord32 colorBg,
                                  const float left, const float top,
                                  const float rigth, const float bottom);

    virtual WebRtc_Word32 SetBitmap(const void* bitMap,
                                    const WebRtc_UWord8 pictureId,
                                    const void* colorKey, const float left,
                                    const float top, const float right,
                                    const float bottom);

    virtual WebRtc_Word32 ConfigureRenderer(const WebRtc_UWord32 channel,
                                            const WebRtc_UWord16 streamId,
                                            const unsigned int zOrder,
                                            const float left, const float top,
                                            const float right,
                                            const float bottom);
public:

    
    int Stop();

    DirectDrawChannel* ShareDirectDrawChannel(int channel);
    DirectDrawChannel* ConfigureDirectDrawChannel(int channel,
                                                      unsigned char streamID,
                                                      int zOrder, float left,
                                                      float top, float right,
                                                      float bottom);

    int AddDirectDrawChannel(int channel, unsigned char streamID, int zOrder,
                             DirectDrawChannel*);

    VideoType GetPerferedVideoFormat();
    bool HasChannels();
    bool HasChannel(int channel);
    bool DeliverInScreenType();
    int GetChannels(std::list<int>& channelList);

    
    int GetScreenResolution(int& screenWidth, int& screenHeight);
    int UpdateSystemCPUUsage(int systemCPU);

    int SetBitmap(HBITMAP bitMap, unsigned char pictureId,
                  DDCOLORKEY* colorKey, float left, float top, float rigth,
                  float bottom);

    bool IsPrimaryOrMixingSurfaceOnSystem();
    bool CanBltFourCC()
    {
        return _bCanBltFourcc;
    }

protected:
    static bool RemoteRenderingThreadProc(void* obj);
    bool RemoteRenderingProcess();

private:
    int CheckCapabilities();
    int CreateMixingSurface();
    int CreatePrimarySurface();

    int FillSurface(DirectDrawSurface *pDDSurface, RECT* rect);
    int DrawOnSurface(unsigned char* buffer, int buffeSize);
    int BlitFromOffscreenBuffersToMixingBuffer();
    int BlitFromBitmapBuffersToMixingBuffer();
    int BlitFromTextToMixingBuffer();

    bool HasHWNDChanged();
    void DecideBestRenderingMode(bool hwndChanged, int totalRenderTime);

    
    int WaitAndFlip(int& waitTime);
    int BlitFromMixingBufferToBackBuffer();

    
    int BlitFromMixingBufferToFrontBuffer(bool hwndChanged, int& waitTime);

    
    Trace* _trace;
    CriticalSectionWrapper* _confCritSect; 

    bool _fullscreen;
    bool _demuxing;
    bool _transparentBackground;
    bool _supportTransparency;
    bool _canStretch;
    bool _canMirrorLeftRight;
    bool _clearMixingSurface;
    bool _deliverInScreenType;
    bool _renderModeWaitForCorrectScanLine;
    bool _deliverInHalfFrameRate;
    bool _deliverInQuarterFrameRate;
    bool _bCanBltFourcc;
    bool _frameChanged; 
    int _processCount;
    HWND _hWnd;
    RECT _screenRect; 
    RECT _mixingRect;
    RECT _originalHwndRect;
    RECT _hwndRect;

    VideoType _incomingVideoType;
    VideoType _blitVideoType;
    VideoType _rgbVideoType;

    DirectDraw* _directDraw;
    DirectDrawSurface* _primarySurface; 
    DirectDrawSurface* _backSurface; 
    DirectDrawSurface* _mixingSurface; 

    std::map<unsigned char, DirectDrawBitmapSettings*> _bitmapSettings;
    std::map<unsigned char, DirectDrawTextSettings*> _textSettings;
    std::map<int, DirectDrawChannel*> _directDrawChannels;
    std::multimap<int, unsigned int> _directDrawZorder;

    EventWrapper* _fullScreenWaitEvent;
    EventWrapper* _screenEvent;
    ThreadWrapper* _screenRenderThread;
    WindowsThreadCpuUsage _screenRenderCpuUsage;

    int _lastRenderModeCpuUsage;

    
    bool _blit;

    
    DWORD _totalMemory;
    DWORD _availableMemory;
    int _systemCPUUsage;

    
    int _maxAllowedRenderTime;
    int _nrOfTooLongRenderTimes;
    bool _isPrimaryOrMixingSurfaceOnSystem;
};

} 


#endif 
