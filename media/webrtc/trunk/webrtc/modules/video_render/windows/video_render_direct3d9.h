









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_WINDOWS_VIDEO_RENDER_DIRECT3D9_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_WINDOWS_VIDEO_RENDER_DIRECT3D9_H_


#include "webrtc/modules/video_render/windows/i_video_render_win.h"

#include <d3d9.h>
#include <d3dx9.h>
#include <ddraw.h>

#include <Map>


#include "webrtc/modules/video_render/include/video_render_defines.h"

#pragma comment(lib, "d3d9.lib")       // located in DirectX SDK

namespace webrtc {
class CriticalSectionWrapper;
class EventWrapper;
class Trace;
class ThreadWrapper;

class D3D9Channel: public VideoRenderCallback
{
public:
    D3D9Channel(LPDIRECT3DDEVICE9 pd3DDevice,
                    CriticalSectionWrapper* critSect, Trace* trace);

    virtual ~D3D9Channel();

    
    
    virtual int FrameSizeChange(int width, int height, int numberOfStreams);

    
    virtual int DeliverFrame(const I420VideoFrame& videoFrame);
    virtual int32_t RenderFrame(const uint32_t streamId,
                                I420VideoFrame& videoFrame);

    
    int IsUpdated(bool& isUpdated);
    
    int RenderOffFrame();
    
    LPDIRECT3DTEXTURE9 GetTexture();
    
    int GetTextureWidth();
    int GetTextureHeight();
    
    void SetStreamSettings(uint16_t streamId,
                           uint32_t zOrder,
                           float startWidth,
                           float startHeight,
                           float stopWidth,
                           float stopHeight);
    int GetStreamSettings(uint16_t streamId,
                          uint32_t& zOrder,
                          float& startWidth,
                          float& startHeight,
                          float& stopWidth,
                          float& stopHeight);

    int ReleaseTexture();
    int RecreateTexture(LPDIRECT3DDEVICE9 pd3DDevice);

protected:

private:
    
    CriticalSectionWrapper* _critSect;
    LPDIRECT3DDEVICE9 _pd3dDevice;
    LPDIRECT3DTEXTURE9 _pTexture;

    bool _bufferIsUpdated;
    
    int _width;
    int _height;
    
    
    uint16_t _streamId;
    uint32_t _zOrder;
    float _startWidth;
    float _startHeight;
    float _stopWidth;
    float _stopHeight;
};

class VideoRenderDirect3D9: IVideoRenderWin
{
public:
    VideoRenderDirect3D9(Trace* trace, HWND hWnd, bool fullScreen);
    ~VideoRenderDirect3D9();

public:
    

    




    virtual int32_t Init();

    




    virtual VideoRenderCallback
            * CreateChannel(const uint32_t streamId,
                            const uint32_t zOrder,
                            const float left,
                            const float top,
                            const float right,
                            const float bottom);

    virtual int32_t DeleteChannel(const uint32_t streamId);

    virtual int32_t GetStreamSettings(const uint32_t channel,
                                      const uint16_t streamId,
                                      uint32_t& zOrder,
                                      float& left, float& top,
                                      float& right, float& bottom);

    





    virtual int32_t StartRender();
    virtual int32_t StopRender();

    





    virtual bool IsFullScreen();

    virtual int32_t SetCropping(const uint32_t channel,
                                const uint16_t streamId,
                                const float left, const float top,
                                const float right, const float bottom);

    virtual int32_t ConfigureRenderer(const uint32_t channel,
                                      const uint16_t streamId,
                                      const unsigned int zOrder,
                                      const float left, const float top,
                                      const float right, const float bottom);

    virtual int32_t SetTransparentBackground(const bool enable);

    virtual int32_t ChangeWindow(void* window);

    virtual int32_t GetGraphicsMemory(uint64_t& totalMemory,
                                      uint64_t& availableMemory);

    virtual int32_t SetText(const uint8_t textId,
                            const uint8_t* text,
                            const int32_t textLength,
                            const uint32_t colorText,
                            const uint32_t colorBg,
                            const float left, const float top,
                            const float rigth, const float bottom);

    virtual int32_t SetBitmap(const void* bitMap,
                              const uint8_t pictureId,
                              const void* colorKey,
                              const float left, const float top,
                              const float right, const float bottom);

public:
    
    D3D9Channel* GetD3DChannel(int channel);
    int UpdateRenderSurface();

protected:
    
    static bool ScreenUpdateThreadProc(void* obj);
    bool ScreenUpdateProcess();

private:
    
    int InitDevice();
    int CloseDevice();

    
    int SetTransparentColor(LPDIRECT3DTEXTURE9 pTexture,
                            DDCOLORKEY* transparentColorKey,
                            DWORD width,
                            DWORD height);

    CriticalSectionWrapper& _refD3DCritsect;
    Trace* _trace;
    ThreadWrapper* _screenUpdateThread;
    EventWrapper* _screenUpdateEvent;

    HWND _hWnd;
    bool _fullScreen;
    RECT _originalHwndRect;
    
    int _channel;
    
    UINT _winWidth;
    UINT _winHeight;

    
    LPDIRECT3D9 _pD3D; 
    LPDIRECT3DDEVICE9 _pd3dDevice; 
    LPDIRECT3DVERTEXBUFFER9 _pVB; 
    LPDIRECT3DTEXTURE9 _pTextureLogo;

    std::map<int, D3D9Channel*> _d3dChannels;
    std::multimap<int, unsigned int> _d3dZorder;

    
    float _logoLeft;
    float _logoTop;
    float _logoRight;
    float _logoBottom;

    typedef HRESULT (WINAPI *DIRECT3DCREATE9EX)(UINT SDKVersion, IDirect3D9Ex**);
    LPDIRECT3DSURFACE9 _pd3dSurface;

    DWORD GetVertexProcessingCaps();
    int InitializeD3D(HWND hWnd, D3DPRESENT_PARAMETERS* pd3dpp);

    D3DPRESENT_PARAMETERS _d3dpp;
    int ResetDevice();

    int UpdateVerticeBuffer(LPDIRECT3DVERTEXBUFFER9 pVB, int offset,
                            float startWidth, float startHeight,
                            float stopWidth, float stopHeight);

    
    DWORD _totalMemory;
    DWORD _availableMemory;
};

}  

#endif 
