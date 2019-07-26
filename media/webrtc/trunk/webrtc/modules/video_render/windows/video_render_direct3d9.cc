










#include "video_render_direct3d9.h"


#include <windows.h>


#include "critical_section_wrapper.h"
#include "event_wrapper.h"
#include "trace.h"
#include "thread_wrapper.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"

namespace webrtc {


struct CUSTOMVERTEX
{
    FLOAT x, y, z;
    DWORD color; 
    FLOAT u, v;
};


#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)






D3D9Channel::D3D9Channel(LPDIRECT3DDEVICE9 pd3DDevice,
                                 CriticalSectionWrapper* critSect,
                                 Trace* trace) :
    _width(0),
    _height(0),
    _pd3dDevice(pd3DDevice),
    _pTexture(NULL),
    _bufferIsUpdated(false),
    _critSect(critSect),
    _streamId(0),
    _zOrder(0),
    _startWidth(0),
    _startHeight(0),
    _stopWidth(0),
    _stopHeight(0)
{

}

D3D9Channel::~D3D9Channel()
{
    
    if (_pTexture != NULL)
    {
        _pTexture->Release();
        _pTexture = NULL;
    }
}

void D3D9Channel::SetStreamSettings(WebRtc_UWord16 streamId,
                                        WebRtc_UWord32 zOrder,
                                        float startWidth,
                                        float startHeight,
                                        float stopWidth,
                                        float stopHeight)
{
    _streamId = streamId;
    _zOrder = zOrder;
    _startWidth = startWidth;
    _startHeight = startHeight;
    _stopWidth = stopWidth;
    _stopHeight = stopHeight;
}

int D3D9Channel::GetStreamSettings(WebRtc_UWord16 streamId,
                                       WebRtc_UWord32& zOrder,
                                       float& startWidth,
                                       float& startHeight,
                                       float& stopWidth,
                                       float& stopHeight)
{
    streamId = _streamId;
    zOrder = _zOrder;
    startWidth = _startWidth;
    startHeight = _startHeight;
    stopWidth = _stopWidth;
    stopHeight = _stopHeight;
    return 0;
}

int D3D9Channel::GetTextureWidth()
{
    return _width;
}

int D3D9Channel::GetTextureHeight()
{
    return _height;
}


int D3D9Channel::FrameSizeChange(int width, int height, int numberOfStreams)
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                 "FrameSizeChange, wifth: %d, height: %d, streams: %d", width,
                 height, numberOfStreams);

    CriticalSectionScoped cs(_critSect);
    _width = width;
    _height = height;

    
    if (_pTexture != NULL)
    {
        _pTexture->Release();
        _pTexture = NULL;
    }

    HRESULT ret = E_POINTER;

    if (_pd3dDevice)
      ret = _pd3dDevice->CreateTexture(_width, _height, 1, 0, D3DFMT_A8R8G8B8,
                                       D3DPOOL_MANAGED, &_pTexture, NULL);

    if (FAILED(ret))
    {
        _pTexture = NULL;
        return -1;
    }

    return 0;
}

WebRtc_Word32 D3D9Channel::RenderFrame(const WebRtc_UWord32 streamId,
                                       I420VideoFrame& videoFrame)
{
    CriticalSectionScoped cs(_critSect);
    if (_width != videoFrame.width() || _height != videoFrame.height())
    {
        if (FrameSizeChange(videoFrame.width(), videoFrame.height(), 1) == -1)
        {
            return -1;
        }
    }
    return DeliverFrame(videoFrame);
}


int D3D9Channel::DeliverFrame(const I420VideoFrame& videoFrame) {
  WEBRTC_TRACE(kTraceStream, kTraceVideo, -1,
               "DeliverFrame to D3D9Channel");

  CriticalSectionScoped cs(_critSect);

  
  
  if (_bufferIsUpdated) {
    WEBRTC_TRACE(kTraceStream, kTraceVideo, -1,
                 "Last frame hasn't been rendered yet. Drop this frame.");
    return -1;
  }

  if (!_pd3dDevice) {
    WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                 "D3D for rendering not initialized.");
    return -1;
  }

  if (!_pTexture) {
    WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                 "Texture for rendering not initialized.");
    return -1;
  }

  D3DLOCKED_RECT lr;

  if (FAILED(_pTexture->LockRect(0, &lr, NULL, 0))) {
    WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                 "Failed to lock a texture in D3D9 Channel.");
    return -1;
  }
  UCHAR* pRect = (UCHAR*) lr.pBits;

  ConvertFromI420(videoFrame, kARGB, 0, pRect);

  if (FAILED(_pTexture->UnlockRect(0))) {
    WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                 "Failed to unlock a texture in D3D9 Channel.");
    return -1;
  }

  _bufferIsUpdated = true;
  return 0;
}


int D3D9Channel::RenderOffFrame()
{
    WEBRTC_TRACE(kTraceStream, kTraceVideo, -1,
                 "Frame has been rendered to the screen.");
    CriticalSectionScoped cs(_critSect);
    _bufferIsUpdated = false;
    return 0;
}


int D3D9Channel::IsUpdated(bool& isUpdated)
{
    CriticalSectionScoped cs(_critSect);
    isUpdated = _bufferIsUpdated;
    return 0;
}


LPDIRECT3DTEXTURE9 D3D9Channel::GetTexture()
{
    CriticalSectionScoped cs(_critSect);
    return _pTexture;
}

int D3D9Channel::ReleaseTexture()
{
    CriticalSectionScoped cs(_critSect);

    
    if (_pTexture != NULL)
    {
        _pTexture->Release();
        _pTexture = NULL;
    }
    _pd3dDevice = NULL;
    return 0;
}

int D3D9Channel::RecreateTexture(LPDIRECT3DDEVICE9 pd3DDevice)
{
    CriticalSectionScoped cs(_critSect);

    _pd3dDevice = pd3DDevice;

    if (_pTexture != NULL)
    {
        _pTexture->Release();
        _pTexture = NULL;
    }

    HRESULT ret;

    ret = _pd3dDevice->CreateTexture(_width, _height, 1, 0, D3DFMT_A8R8G8B8,
                                     D3DPOOL_MANAGED, &_pTexture, NULL);

    if (FAILED(ret))
    {
        _pTexture = NULL;
        return -1;
    }

    return 0;
}






VideoRenderDirect3D9::VideoRenderDirect3D9(Trace* trace,
                                                   HWND hWnd,
                                                   bool fullScreen) :
    _refD3DCritsect(*CriticalSectionWrapper::CreateCriticalSection()),
    _trace(trace),
    _hWnd(hWnd),
    _fullScreen(fullScreen),
    _pTextureLogo(NULL),
    _pVB(NULL),
    _pd3dDevice(NULL),
    _pD3D(NULL),
    _d3dChannels(),
    _d3dZorder(),
    _screenUpdateThread(NULL),
    _screenUpdateEvent(NULL),
    _logoLeft(0),
    _logoTop(0),
    _logoRight(0),
    _logoBottom(0),
    _pd3dSurface(NULL),
    _totalMemory(-1),
    _availableMemory(-1)
{
    _screenUpdateThread = ThreadWrapper::CreateThread(ScreenUpdateThreadProc,
                                                      this, kRealtimePriority);
    _screenUpdateEvent = EventWrapper::Create();
    SetRect(&_originalHwndRect, 0, 0, 0, 0);
}

VideoRenderDirect3D9::~VideoRenderDirect3D9()
{
    

    
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
    }
    delete _screenUpdateEvent;

    
    CloseDevice();

    
    std::map<int, D3D9Channel*>::iterator it = _d3dChannels.begin();
    while (it != _d3dChannels.end())
    {
        delete it->second;
        it = _d3dChannels.erase(it);
    }
    
    _d3dZorder.clear();

    if (_fullScreen)
    {
        
        ::SetWindowPos(_hWnd, HWND_NOTOPMOST, _originalHwndRect.left,
                       _originalHwndRect.top, _originalHwndRect.right
                               - _originalHwndRect.left,
                       _originalHwndRect.bottom - _originalHwndRect.top,
                       SWP_FRAMECHANGED);
        ::RedrawWindow(_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW
                | RDW_ERASE);
        ::RedrawWindow(NULL, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW
                | RDW_ERASE);
    }

    delete &_refD3DCritsect;
}

DWORD VideoRenderDirect3D9::GetVertexProcessingCaps()
{
    D3DCAPS9 caps;
    DWORD dwVertexProcessing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    if (SUCCEEDED(_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                                       &caps)))
    {
        if ((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
                == D3DDEVCAPS_HWTRANSFORMANDLIGHT)
        {
            dwVertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
        }
    }
    return dwVertexProcessing;
}

int VideoRenderDirect3D9::InitializeD3D(HWND hWnd,
                                            D3DPRESENT_PARAMETERS* pd3dpp)
{
    
    if (NULL == (_pD3D = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        return -1;
    }

    
    DWORD dwVertexProcessing = GetVertexProcessingCaps();

    
    D3DDISPLAYMODE d3ddm;
    _pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
    pd3dpp->BackBufferFormat = d3ddm.Format;

    
    if (FAILED(_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                   dwVertexProcessing | D3DCREATE_MULTITHREADED
                                           | D3DCREATE_FPU_PRESERVE, pd3dpp,
                                   &_pd3dDevice)))
    {
        
        if (FAILED(_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF,
                                       hWnd, dwVertexProcessing
                                               | D3DCREATE_MULTITHREADED
                                               | D3DCREATE_FPU_PRESERVE,
                                       pd3dpp, &_pd3dDevice)))
        {
            return -1;
        }
    }

    return 0;
}

int VideoRenderDirect3D9::ResetDevice()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                 "VideoRenderDirect3D9::ResetDevice");

    CriticalSectionScoped cs(&_refD3DCritsect);

    
    std::map<int, D3D9Channel*>::iterator it;
    it = _d3dChannels.begin();
    while (it != _d3dChannels.end())
    {
        if (it->second)
        {
            it->second->ReleaseTexture();
        }
        it++;
    }

    
    if (CloseDevice() != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "VideoRenderDirect3D9::ResetDevice failed to CloseDevice");
        return -1;
    }

    
    if (InitDevice() != 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "VideoRenderDirect3D9::ResetDevice failed to InitDevice");
        return -1;
    }

    
    it = _d3dChannels.begin();
    while (it != _d3dChannels.end())
    {
        if (it->second)
        {
            it->second->RecreateTexture(_pd3dDevice);
        }
        it++;
    }

    return 0;
}

int VideoRenderDirect3D9::InitDevice()
{
    
    ZeroMemory(&_d3dpp, sizeof(_d3dpp));
    _d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    _d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    if (GetWindowRect(_hWnd, &_originalHwndRect) == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "VideoRenderDirect3D9::InitDevice Could not get window size");
        return -1;
    }
    if (!_fullScreen)
    {
        _winWidth = _originalHwndRect.right - _originalHwndRect.left;
        _winHeight = _originalHwndRect.bottom - _originalHwndRect.top;
        _d3dpp.Windowed = TRUE;
        _d3dpp.BackBufferHeight = 0;
        _d3dpp.BackBufferWidth = 0;
    }
    else
    {
        _winWidth = (LONG) ::GetSystemMetrics(SM_CXSCREEN);
        _winHeight = (LONG) ::GetSystemMetrics(SM_CYSCREEN);
        _d3dpp.Windowed = FALSE;
        _d3dpp.BackBufferWidth = _winWidth;
        _d3dpp.BackBufferHeight = _winHeight;
        _d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }

    if (InitializeD3D(_hWnd, &_d3dpp) == -1)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "VideoRenderDirect3D9::InitDevice failed in InitializeD3D");
        return -1;
    }

    
    _pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    
    _pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

    
    _pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    _pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    _pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    _pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    _pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    _pd3dDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

    
    CUSTOMVERTEX Vertices[] = {
            
            { -1.0f, -1.0f, 0.0f, 0xffffffff, 0, 1 }, { -1.0f, 1.0f, 0.0f,
                    0xffffffff, 0, 0 },
            { 1.0f, -1.0f, 0.0f, 0xffffffff, 1, 1 }, { 1.0f, 1.0f, 0.0f,
                    0xffffffff, 1, 0 } };

    
    if (FAILED(_pd3dDevice->CreateVertexBuffer(sizeof(Vertices), 0,
                                               D3DFVF_CUSTOMVERTEX,
                                               D3DPOOL_DEFAULT, &_pVB, NULL )))
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "Failed to create the vertex buffer.");
        return -1;
    }

    
    VOID* pVertices;
    if (FAILED(_pVB->Lock(0, sizeof(Vertices), (void**) &pVertices, 0)))
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "Failed to lock the vertex buffer.");
        return -1;
    }
    memcpy(pVertices, Vertices, sizeof(Vertices));
    _pVB->Unlock();

    return 0;
}

WebRtc_Word32 VideoRenderDirect3D9::Init()
{
    WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                 "VideoRenderDirect3D9::Init");

    CriticalSectionScoped cs(&_refD3DCritsect);

    
    if (!_screenUpdateThread)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Thread not created");
        return -1;
    }
    unsigned int threadId;
    _screenUpdateThread->Start(threadId);

    
    unsigned int monitorFreq = 60;
    DEVMODE dm;
    
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);
    if (0 != EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm))
    {
        monitorFreq = dm.dmDisplayFrequency;
    }
    _screenUpdateEvent->StartTimer(true, 1000 / monitorFreq);

    return InitDevice();
}

WebRtc_Word32 VideoRenderDirect3D9::ChangeWindow(void* window)
{
    WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
    return -1;
}

int VideoRenderDirect3D9::UpdateRenderSurface()
{
    CriticalSectionScoped cs(&_refD3DCritsect);

    
    bool updated = false;
    std::map<int, D3D9Channel*>::iterator it;
    it = _d3dChannels.begin();
    while (it != _d3dChannels.end())
    {

        D3D9Channel* channel = it->second;
        channel->IsUpdated(updated);
        if (updated)
        {
            break;
        }
        it++;
    }
    
    if (!updated)
        return -1;

    
    _pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f,
                       0);

    
    if (SUCCEEDED(_pd3dDevice->BeginScene()))
    {
        _pd3dDevice->SetStreamSource(0, _pVB, 0, sizeof(CUSTOMVERTEX));
        _pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

        D3DXMATRIX matWorld;
        D3DXMATRIX matWorldTemp;

        
        
        LPDIRECT3DTEXTURE9 textureFromChannel = NULL;
        DWORD textureWidth, textureHeight;

        std::multimap<int, unsigned int>::reverse_iterator it;
        it = _d3dZorder.rbegin();
        while (it != _d3dZorder.rend())
        {
            
            int channel = it->second & 0x0000ffff;

            std::map<int, D3D9Channel*>::iterator ddIt;
            ddIt = _d3dChannels.find(channel);
            if (ddIt != _d3dChannels.end())
            {
                
                D3D9Channel* channelObj = ddIt->second;
                if (channelObj)
                {
                    textureFromChannel = channelObj->GetTexture();
                    textureWidth = channelObj->GetTextureWidth();
                    textureHeight = channelObj->GetTextureHeight();

                    WebRtc_UWord32 zOrder;
                    float startWidth, startHeight, stopWidth, stopHeight;
                    channelObj->GetStreamSettings(0, zOrder, startWidth,
                                                  startHeight, stopWidth,
                                                  stopHeight);

                    
                    UpdateVerticeBuffer(_pVB, 0, startWidth, startHeight,
                                        stopWidth, stopHeight);
                    _pd3dDevice->SetTexture(0, textureFromChannel);
                    _pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

                    
                    channelObj->RenderOffFrame();
                }
            }
            it++;
        }

        
        if (_pTextureLogo)
        {
            UpdateVerticeBuffer(_pVB, 0, _logoLeft, _logoTop, _logoRight,
                                _logoBottom);
            _pd3dDevice->SetTexture(0, _pTextureLogo);
            _pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        }

        
        _pd3dDevice->EndScene();
    }

    
    _pd3dDevice->Present(NULL, NULL, NULL, NULL );

    return 0;
}


int VideoRenderDirect3D9::SetTransparentColor(LPDIRECT3DTEXTURE9 pTexture,
                                                  DDCOLORKEY* transparentColorKey,
                                                  DWORD width,
                                                  DWORD height)
{
    D3DLOCKED_RECT lr;
    if (!pTexture)
        return -1;

    CriticalSectionScoped cs(&_refD3DCritsect);
    if (SUCCEEDED(pTexture->LockRect(0, &lr, NULL, D3DLOCK_DISCARD)))
    {
        for (DWORD y = 0; y < height; y++)
        {
            DWORD dwOffset = y * width;

            for (DWORD x = 0; x < width; x)
            {
                DWORD temp = ((DWORD*) lr.pBits)[dwOffset + x];
                if ((temp & 0x00FFFFFF)
                        == transparentColorKey->dwColorSpaceLowValue)
                {
                    temp &= 0x00FFFFFF;
                }
                else
                {
                    temp |= 0xFF000000;
                }
                ((DWORD*) lr.pBits)[dwOffset + x] = temp;
                x++;
            }
        }
        pTexture->UnlockRect(0);
        return 0;
    }
    return -1;
}






bool VideoRenderDirect3D9::ScreenUpdateThreadProc(void* obj)
{
    return static_cast<VideoRenderDirect3D9*> (obj)->ScreenUpdateProcess();
}

bool VideoRenderDirect3D9::ScreenUpdateProcess()
{
    _screenUpdateEvent->Wait(100);

    if (!_screenUpdateThread)
    {
        
        return false;
    }
    if (!_pd3dDevice)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "d3dDevice not created.");
        return true;
    }

    HRESULT hr = _pd3dDevice->TestCooperativeLevel();

    if (SUCCEEDED(hr))
    {
        UpdateRenderSurface();
    }

    if (hr == D3DERR_DEVICELOST)
    {
        

    }
    else if (hr == D3DERR_DEVICENOTRESET)
    {
        
        
        
        ResetDevice();
    }

    return true;
}

int VideoRenderDirect3D9::CloseDevice()
{
    CriticalSectionScoped cs(&_refD3DCritsect);
    WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1,
                 "VideoRenderDirect3D9::CloseDevice");

    if (_pTextureLogo != NULL)
    {
        _pTextureLogo->Release();
        _pTextureLogo = NULL;
    }

    if (_pVB != NULL)
    {
        _pVB->Release();
        _pVB = NULL;
    }

    if (_pd3dDevice != NULL)
    {
        _pd3dDevice->Release();
        _pd3dDevice = NULL;
    }

    if (_pD3D != NULL)
    {
        _pD3D->Release();
        _pD3D = NULL;
    }

    if (_pd3dSurface != NULL)
        _pd3dSurface->Release();
    return 0;
}

D3D9Channel* VideoRenderDirect3D9::GetD3DChannel(int channel)
{
    std::map<int, D3D9Channel*>::iterator ddIt;
    ddIt = _d3dChannels.find(channel & 0x0000ffff);
    D3D9Channel* ddobj = NULL;
    if (ddIt != _d3dChannels.end())
    {
        ddobj = ddIt->second;
    }
    if (ddobj == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "Direct3D render failed to find channel");
        return NULL;
    }
    return ddobj;
}

WebRtc_Word32 VideoRenderDirect3D9::DeleteChannel(const WebRtc_UWord32 streamId)
{
    CriticalSectionScoped cs(&_refD3DCritsect);


    std::multimap<int, unsigned int>::iterator it;
    it = _d3dZorder.begin();
    while (it != _d3dZorder.end())
    {
        if ((streamId & 0x0000ffff) == (it->second & 0x0000ffff))
        {
            it = _d3dZorder.erase(it);
            break;
        }
        it++;
    }

    std::map<int, D3D9Channel*>::iterator ddIt;
    ddIt = _d3dChannels.find(streamId & 0x0000ffff);
    if (ddIt != _d3dChannels.end())
    {
        delete ddIt->second;
        _d3dChannels.erase(ddIt);        
        return 0;
    }
    return -1;
}

VideoRenderCallback* VideoRenderDirect3D9::CreateChannel(const WebRtc_UWord32 channel,
                                                                 const WebRtc_UWord32 zOrder,
                                                                 const float left,
                                                                 const float top,
                                                                 const float right,
                                                                 const float bottom)
{
    CriticalSectionScoped cs(&_refD3DCritsect);

    
    
    DeleteChannel(channel);

    D3D9Channel* d3dChannel = new D3D9Channel(_pd3dDevice,
                                                      &_refD3DCritsect, _trace);
    d3dChannel->SetStreamSettings(0, zOrder, left, top, right, bottom);

    
    _d3dChannels[channel & 0x0000ffff] = d3dChannel;

    
    
    _d3dZorder.insert(
                      std::pair<int, unsigned int>(zOrder, channel & 0x0000ffff));

    return d3dChannel;
}

WebRtc_Word32 VideoRenderDirect3D9::GetStreamSettings(const WebRtc_UWord32 channel,
                                                          const WebRtc_UWord16 streamId,
                                                          WebRtc_UWord32& zOrder,
                                                          float& left,
                                                          float& top,
                                                          float& right,
                                                          float& bottom)
{
    std::map<int, D3D9Channel*>::iterator ddIt;
    ddIt = _d3dChannels.find(channel & 0x0000ffff);
    D3D9Channel* ddobj = NULL;
    if (ddIt != _d3dChannels.end())
    {
        ddobj = ddIt->second;
    }
    if (ddobj == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "Direct3D render failed to find channel");
        return -1;
    }
    
    return ddobj->GetStreamSettings(0, zOrder, left, top, right, bottom);
    
}

int VideoRenderDirect3D9::UpdateVerticeBuffer(LPDIRECT3DVERTEXBUFFER9 pVB,
                                                  int offset,
                                                  float startWidth,
                                                  float startHeight,
                                                  float stopWidth,
                                                  float stopHeight)
{
    if (pVB == NULL)
        return -1;

    float left, right, top, bottom;

    
    
    left = startWidth * 2 - 1;
    right = stopWidth * 2 - 1;

    
    top = 1 - startHeight * 2;
    bottom = 1 - stopHeight * 2;

    CUSTOMVERTEX newVertices[] = {
            
            { left, bottom, 0.0f, 0xffffffff, 0, 1 }, { left, top, 0.0f,
                    0xffffffff, 0, 0 },
            { right, bottom, 0.0f, 0xffffffff, 1, 1 }, { right, top, 0.0f,
                    0xffffffff, 1, 0 }, };
    
    VOID* pVertices;
    if (FAILED(pVB->Lock(sizeof(CUSTOMVERTEX) * offset, sizeof(newVertices),
                         (void**) &pVertices, 0)))
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "Failed to lock the vertex buffer.");
        return -1;
    }
    memcpy(pVertices, newVertices, sizeof(newVertices));
    pVB->Unlock();

    return 0;
}

WebRtc_Word32 VideoRenderDirect3D9::StartRender()
{
    WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
    return 0;
}

WebRtc_Word32 VideoRenderDirect3D9::StopRender()
{
    WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
    return 0;
}

bool VideoRenderDirect3D9::IsFullScreen()
{
    return _fullScreen;
}

WebRtc_Word32 VideoRenderDirect3D9::SetCropping(const WebRtc_UWord32 channel,
                                                    const WebRtc_UWord16 streamId,
                                                    const float left,
                                                    const float top,
                                                    const float right,
                                                    const float bottom)
{
    WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
    return 0;
}

WebRtc_Word32 VideoRenderDirect3D9::SetTransparentBackground(
                                                                 const bool enable)
{
    WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
    return 0;
}

WebRtc_Word32 VideoRenderDirect3D9::SetText(const WebRtc_UWord8 textId,
                                                const WebRtc_UWord8* text,
                                                const WebRtc_Word32 textLength,
                                                const WebRtc_UWord32 colorText,
                                                const WebRtc_UWord32 colorBg,
                                                const float left,
                                                const float top,
                                                const float rigth,
                                                const float bottom)
{
    WEBRTC_TRACE(kTraceError, kTraceVideo, -1, "Not supported.");
    return 0;
}

WebRtc_Word32 VideoRenderDirect3D9::SetBitmap(const void* bitMap,
                                                  const WebRtc_UWord8 pictureId,
                                                  const void* colorKey,
                                                  const float left,
                                                  const float top,
                                                  const float right,
                                                  const float bottom)
{
    if (!bitMap)
    {
        if (_pTextureLogo != NULL)
        {
            _pTextureLogo->Release();
            _pTextureLogo = NULL;
        }
        WEBRTC_TRACE(kTraceInfo, kTraceVideo, -1, "Remove bitmap.");
        return 0;
    }

    
    if (left > 1.0f || left < 0.0f ||
        top > 1.0f || top < 0.0f ||
        right > 1.0f || right < 0.0f ||
        bottom > 1.0f || bottom < 0.0f)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "Direct3D SetBitmap invalid parameter");
        return -1;
    }

    if ((bottom <= top) || (right <= left))
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "Direct3D SetBitmap invalid parameter");
        return -1;
    }

    CriticalSectionScoped cs(&_refD3DCritsect);

    unsigned char* srcPtr;
    HGDIOBJ oldhand;
    BITMAPINFO pbi;
    BITMAP bmap;
    HDC hdcNew;
    hdcNew = CreateCompatibleDC(0);
    
    GetObject((HBITMAP)bitMap, sizeof(bmap), &bmap);
    
    oldhand = SelectObject(hdcNew, (HGDIOBJ) bitMap);
    
    DeleteObject(oldhand);
    pbi.bmiHeader.biSize = 40;
    pbi.bmiHeader.biWidth = bmap.bmWidth;
    pbi.bmiHeader.biHeight = bmap.bmHeight;
    pbi.bmiHeader.biPlanes = 1;
    pbi.bmiHeader.biBitCount = bmap.bmBitsPixel;
    pbi.bmiHeader.biCompression = BI_RGB;
    pbi.bmiHeader.biSizeImage = bmap.bmWidth * bmap.bmHeight * 3;
    srcPtr = new unsigned char[bmap.bmWidth * bmap.bmHeight * 4];
    
    int pixelHeight = GetDIBits(hdcNew, (HBITMAP)bitMap, 0, bmap.bmHeight, srcPtr, &pbi,
                                DIB_RGB_COLORS);
    if (pixelHeight == 0)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "Direct3D failed to GetDIBits in SetBitmap");
        delete[] srcPtr;
        return -1;
    }
    DeleteDC(hdcNew);
    if (pbi.bmiHeader.biBitCount != 24 && pbi.bmiHeader.biBitCount != 32)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "Direct3D failed to SetBitmap invalid bit depth");
        delete[] srcPtr;
        return -1;
    }

    HRESULT ret;
    
    if (_pTextureLogo != NULL)
    {
        _pTextureLogo->Release();
        _pTextureLogo = NULL;
    }
    ret = _pd3dDevice->CreateTexture(bmap.bmWidth, bmap.bmHeight, 1, 0,
                                     D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                     &_pTextureLogo, NULL);
    if (FAILED(ret))
    {
        _pTextureLogo = NULL;
        delete[] srcPtr;
        return -1;
    }
    if (!_pTextureLogo)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "Texture for rendering not initialized.");
        delete[] srcPtr;
        return -1;
    }

    D3DLOCKED_RECT lr;
    if (FAILED(_pTextureLogo->LockRect(0, &lr, NULL, 0)))
    {
        delete[] srcPtr;
        return -1;
    }
    unsigned char* dstPtr = (UCHAR*) lr.pBits;
    int pitch = bmap.bmWidth * 4;

    if (pbi.bmiHeader.biBitCount == 24)
    {       
        ConvertRGB24ToARGB(srcPtr, dstPtr, bmap.bmWidth, bmap.bmHeight, 0);
    }
    else
    {
        unsigned char* srcTmp = srcPtr + (bmap.bmWidth * 4) * (bmap.bmHeight - 1);
        for (int i = 0; i < bmap.bmHeight; ++i)
        {
            memcpy(dstPtr, srcTmp, bmap.bmWidth * 4);
            srcTmp -= bmap.bmWidth * 4;
            dstPtr += pitch;
        }
    }

    delete[] srcPtr;
    if (FAILED(_pTextureLogo->UnlockRect(0)))
    {
        return -1;
    }

    if (colorKey)
    {
        DDCOLORKEY* ddColorKey =
                static_cast<DDCOLORKEY*> (const_cast<void*> (colorKey));
        SetTransparentColor(_pTextureLogo, ddColorKey, bmap.bmWidth,
                            bmap.bmHeight);
    }

    
    
    _logoLeft = left;
    _logoRight = right;

    
    _logoTop = top;
    _logoBottom = bottom;

    return 0;

}

WebRtc_Word32 VideoRenderDirect3D9::GetGraphicsMemory(WebRtc_UWord64& totalMemory,
                                                          WebRtc_UWord64& availableMemory)
{
    if (_totalMemory == -1 || _availableMemory == -1)
    {
        totalMemory = 0;
        availableMemory = 0;
        return -1;
    }
    totalMemory = _totalMemory;
    availableMemory = _availableMemory;
    return 0;
}

WebRtc_Word32 VideoRenderDirect3D9::ConfigureRenderer(const WebRtc_UWord32 channel,
                                                          const WebRtc_UWord16 streamId,
                                                          const unsigned int zOrder,
                                                          const float left,
                                                          const float top,
                                                          const float right,
                                                          const float bottom)
{
    std::map<int, D3D9Channel*>::iterator ddIt;
    ddIt = _d3dChannels.find(channel & 0x0000ffff);
    D3D9Channel* ddobj = NULL;
    if (ddIt != _d3dChannels.end())
    {
        ddobj = ddIt->second;
    }
    if (ddobj == NULL)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideo, -1,
                     "Direct3D render failed to find channel");
        return -1;
    }
    
    ddobj->SetStreamSettings(0, zOrder, left, top, right, bottom);

    return 0;
}

} 

