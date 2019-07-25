









#include "libEGL/Surface.h"

#include "common/debug.h"

#include "libEGL/main.h"
#include "libEGL/Display.h"

namespace egl
{
Surface::Surface(Display *display, const Config *config, HWND window) 
    : mDisplay(display), mConfig(config), mWindow(window)
{
    mSwapChain = NULL;
    mDepthStencil = NULL;
    mBackBuffer = NULL;
    mFlipTexture = NULL;
    mFlipState = NULL;
    mPreFlipState = NULL;

    mPixelAspectRatio = (EGLint)(1.0 * EGL_DISPLAY_SCALING);   
    mRenderBuffer = EGL_BACK_BUFFER;
    mSwapBehavior = EGL_BUFFER_PRESERVED;
    mSwapInterval = -1;
    setSwapInterval(1);

    subclassWindow();
    resetSwapChain();
}

Surface::~Surface()
{
    unsubclassWindow();
    release();
}

void Surface::release()
{
    if (mSwapChain)
    {
        mSwapChain->Release();
        mSwapChain = NULL;
    }

    if (mBackBuffer)
    {
        mBackBuffer->Release();
        mBackBuffer = NULL;
    }

    if (mDepthStencil)
    {
        mDepthStencil->Release();
        mDepthStencil = NULL;
    }

    if (mFlipTexture)
    {
        mFlipTexture->Release();
        mFlipTexture = NULL;
    }

    if (mFlipState)
    {
        mFlipState->Release();
        mFlipState = NULL;
    }

    if (mPreFlipState)
    {
        mPreFlipState->Release();
        mPreFlipState = NULL;
    }
}

void Surface::resetSwapChain()
{
    RECT windowRect;
    if (!GetClientRect(getWindowHandle(), &windowRect))
    {
        ASSERT(false);

        ERR("Could not retrieve the window dimensions");
        return;
    }

    resetSwapChain(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
}

void Surface::resetSwapChain(int backbufferWidth, int backbufferHeight)
{
    IDirect3DDevice9 *device = mDisplay->getDevice();

    if (device == NULL)
    {
        return;
    }

    
    
    device->EvictManagedResources();
    release();
    
    D3DPRESENT_PARAMETERS presentParameters = {0};

    presentParameters.AutoDepthStencilFormat = mConfig->mDepthStencilFormat;
    presentParameters.BackBufferCount = 1;
    presentParameters.BackBufferFormat = mConfig->mRenderTargetFormat;
    presentParameters.EnableAutoDepthStencil = FALSE;
    presentParameters.Flags = 0;
    presentParameters.hDeviceWindow = getWindowHandle();
    presentParameters.MultiSampleQuality = 0;                  
    presentParameters.MultiSampleType = D3DMULTISAMPLE_NONE;   
    presentParameters.PresentationInterval = mPresentInterval;
    presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    presentParameters.Windowed = TRUE;
    presentParameters.BackBufferWidth = backbufferWidth;
    presentParameters.BackBufferHeight = backbufferHeight;

    HRESULT result = device->CreateAdditionalSwapChain(&presentParameters, &mSwapChain);

    if (FAILED(result))
    {
        ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);

        ERR("Could not create additional swap chains: %08lX", result);
        return error(EGL_BAD_ALLOC);
    }

    result = device->CreateDepthStencilSurface(presentParameters.BackBufferWidth, presentParameters.BackBufferHeight,
                                               presentParameters.AutoDepthStencilFormat, presentParameters.MultiSampleType,
                                               presentParameters.MultiSampleQuality, FALSE, &mDepthStencil, NULL);

    if (FAILED(result))
    {
        ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);

        mSwapChain->Release();

        ERR("Could not create depthstencil surface for new swap chain: %08lX", result);
        return error(EGL_BAD_ALLOC);
    }

    ASSERT(SUCCEEDED(result));

    result = device->CreateTexture(presentParameters.BackBufferWidth, presentParameters.BackBufferHeight, 1, D3DUSAGE_RENDERTARGET,
                                   presentParameters.BackBufferFormat, D3DPOOL_DEFAULT, &mFlipTexture, NULL);

    if (FAILED(result))
    {
        ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);

        mSwapChain->Release();
        mDepthStencil->Release();

        ERR("Could not create flip texture for new swap chain: %08lX", result);
        return error(EGL_BAD_ALLOC);
    }

    mSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &mBackBuffer);
    mWidth = presentParameters.BackBufferWidth;
    mHeight = presentParameters.BackBufferHeight;

    mPresentIntervalDirty = false;

    InvalidateRect(mWindow, NULL, FALSE);

    
    releaseRecordedState(device);
}

HWND Surface::getWindowHandle()
{
    return mWindow;
}

void Surface::writeRecordableFlipState(IDirect3DDevice9 *device)
{
    
    device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
    device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);
    device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    device->SetPixelShader(NULL);
    device->SetVertexShader(NULL);

    
    device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    device->SetTexture(0, NULL);   
    device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
    device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

    device->SetStreamSourceFreq(0, 1);   

    RECT scissorRect = {0};   
    device->SetScissorRect(&scissorRect);
    D3DVIEWPORT9 viewport = {0, 0, mWidth, mHeight, 0.0f, 1.0f};
    device->SetViewport(&viewport);
}

void Surface::applyFlipState(IDirect3DDevice9 *device)
{
    HRESULT hr;

    if (mFlipState == NULL)
    {
        

        
        hr = device->BeginStateBlock();
        ASSERT(SUCCEEDED(hr));
        writeRecordableFlipState(device);
        hr = device->EndStateBlock(&mPreFlipState);
        ASSERT(SUCCEEDED(hr) || hr == D3DERR_OUTOFVIDEOMEMORY || hr == E_OUTOFMEMORY);

        if (SUCCEEDED(hr))
        {
            mPreFlipState->Capture();
        }

        
        hr = device->BeginStateBlock();
        ASSERT(SUCCEEDED(hr));

        writeRecordableFlipState(device);

        hr = device->EndStateBlock(&mFlipState);
        ASSERT(SUCCEEDED(hr) || hr == D3DERR_OUTOFVIDEOMEMORY || hr == E_OUTOFMEMORY);

        if (FAILED(hr))
        {
            mFlipState = NULL;
            mPreFlipState->Release();
            mPreFlipState = NULL;
        }
        else
        {
            hr = mFlipState->Apply();
            ASSERT(SUCCEEDED(hr));
        }
    }
    else
    {
        hr = mPreFlipState->Capture();
        ASSERT(SUCCEEDED(hr));
        hr = mFlipState->Apply();
        ASSERT(SUCCEEDED(hr));
    }

    device->GetRenderTarget(0, &mPreFlipBackBuffer);
    device->GetDepthStencilSurface(&mPreFlipDepthStencil);

    device->SetRenderTarget(0, mBackBuffer);
    device->SetDepthStencilSurface(NULL);
}

void Surface::restoreState(IDirect3DDevice9 *device)
{
    device->SetRenderTarget(0, mPreFlipBackBuffer);
    device->SetDepthStencilSurface(mPreFlipDepthStencil);

    if (mPreFlipBackBuffer)
    {
        mPreFlipBackBuffer->Release();
        mPreFlipBackBuffer = NULL;
    }

    if (mPreFlipDepthStencil)
    {
        mPreFlipDepthStencil->Release();
        mPreFlipDepthStencil = NULL;
    }

    mPreFlipState->Apply();
}



void Surface::releaseRecordedState(IDirect3DDevice9 *device)
{
    if (mFlipState)
    {
        mFlipState->Release();
        mFlipState = NULL;
    }

    if (mPreFlipState)
    {
        mPreFlipState->Release();
        mPreFlipState = NULL;
    }
}
#define kSurfaceProperty L"Egl::SurfaceOwner"
#define kParentWndProc L"Egl::SurfaceParentWndProc"

static LRESULT CALLBACK SurfaceWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  if (message == WM_SIZE) {
      Surface* surf = reinterpret_cast<Surface*>(GetProp(hwnd, kSurfaceProperty));
      if(surf) {
        surf->checkForOutOfDateSwapChain();
      }
  }
  WNDPROC prevWndFunc = reinterpret_cast<WNDPROC >(GetProp(hwnd, kParentWndProc));
  return CallWindowProc(prevWndFunc, hwnd, message, wparam, lparam);
}

void Surface::subclassWindow()
{
  SetLastError(0);
  LONG oldWndProc = SetWindowLong(mWindow, GWL_WNDPROC, reinterpret_cast<LONG>(SurfaceWindowProc));
  if(oldWndProc == 0 && GetLastError() != ERROR_SUCCESS) {
    mWindowSubclassed = false;
    return;
  }

  SetProp(mWindow, kSurfaceProperty, reinterpret_cast<HANDLE>(this));
  SetProp(mWindow, kParentWndProc, reinterpret_cast<HANDLE>(oldWndProc));
  mWindowSubclassed = true;
}

void Surface::unsubclassWindow()
{
  if(!mWindowSubclassed)
    return;
  
  
  
  
  
  ASSERT(GetWindowLong(mWindow, GWL_WNDPROC) == reinterpret_cast<LONG>(SurfaceWindowProc));

  
  LONG prevWndFunc = reinterpret_cast<LONG>(GetProp(mWindow, kParentWndProc));
  SetWindowLong(mWindow, GWL_WNDPROC, prevWndFunc);
  RemoveProp(mWindow, kSurfaceProperty);
  RemoveProp(mWindow, kParentWndProc);
  mWindowSubclassed = false;
}

bool Surface::checkForOutOfDateSwapChain()
{
    RECT client;
    if (!GetClientRect(getWindowHandle(), &client))
    {
        ASSERT(false);
        return false;
    }

    
    int clientWidth = client.right - client.left;
    int clientHeight = client.bottom - client.top;
    bool sizeDirty = clientWidth != getWidth() || clientHeight != getHeight();

    if (sizeDirty || mPresentIntervalDirty)
    {
        resetSwapChain(clientWidth, clientHeight);
        if (static_cast<egl::Surface*>(getCurrentDrawSurface()) == this)
        {
            glMakeCurrent(glGetCurrentContext(), static_cast<egl::Display*>(getCurrentDisplay()), this);
        }

        return true;
    }
    return false;
}

DWORD Surface::convertInterval(EGLint interval)
{
    switch(interval)
    {
      case 0: return D3DPRESENT_INTERVAL_IMMEDIATE;
      case 1: return D3DPRESENT_INTERVAL_ONE;
      case 2: return D3DPRESENT_INTERVAL_TWO;
      case 3: return D3DPRESENT_INTERVAL_THREE;
      case 4: return D3DPRESENT_INTERVAL_FOUR;
      default: UNREACHABLE();
    }

    return D3DPRESENT_INTERVAL_DEFAULT;
}


bool Surface::swap()
{
    if (mSwapChain)
    {
        IDirect3DDevice9 *device = mDisplay->getDevice();

        applyFlipState(device);
        device->SetTexture(0, mFlipTexture);

        
        
        float w = static_cast<float>(getWidth());
        float h = static_cast<float>(getHeight());
        float quad[4][6] = {{0 - 0.5f, 0 - 0.5f, 0.0f, 1.0f, 0.0f, 1.0f},
                            {w - 0.5f, 0 - 0.5f, 0.0f, 1.0f, 1.0f, 1.0f},
                            {w - 0.5f, h - 0.5f, 0.0f, 1.0f, 1.0f, 0.0f},
                            {0 - 0.5f, h - 0.5f, 0.0f, 1.0f, 0.0f, 0.0f}};   

        mDisplay->startScene();
        device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, quad, 6 * sizeof(float));

        restoreState(device);

        mDisplay->endScene();

        HRESULT result = mSwapChain->Present(NULL, NULL, NULL, NULL, 0);

        if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY || result == D3DERR_DRIVERINTERNALERROR)
        {
            return error(EGL_BAD_ALLOC, false);
        }

        if (result == D3DERR_DEVICELOST)
        {
            return error(EGL_CONTEXT_LOST, false);
        }

        ASSERT(SUCCEEDED(result));

        checkForOutOfDateSwapChain();
    }

    return true;
}

EGLint Surface::getWidth() const
{
    return mWidth;
}

EGLint Surface::getHeight() const
{
    return mHeight;
}

IDirect3DSurface9 *Surface::getRenderTarget()
{
    IDirect3DSurface9 *textureSurface = NULL;

    if (mFlipTexture)
    {
        mFlipTexture->GetSurfaceLevel(0, &textureSurface);
    }

    return textureSurface;
}

IDirect3DSurface9 *Surface::getDepthStencil()
{
    if (mDepthStencil)
    {
        mDepthStencil->AddRef();
    }

    return mDepthStencil;
}

void Surface::setSwapInterval(EGLint interval)
{
    if (mSwapInterval == interval)
    {
        return;
    }
    
    mSwapInterval = interval;
    mSwapInterval = std::max(mSwapInterval, mDisplay->getMinSwapInterval());
    mSwapInterval = std::min(mSwapInterval, mDisplay->getMaxSwapInterval());

    mPresentInterval = convertInterval(mSwapInterval);
    mPresentIntervalDirty = true;
}
}
