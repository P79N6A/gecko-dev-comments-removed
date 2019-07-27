










#ifndef COMMON_NATIVEWINDOW_H_
#define COMMON_NATIVEWINDOW_H_

#include <EGL/eglplatform.h>
#include "common/debug.h"
#include "common/platform.h"

#ifdef ANGLE_ENABLE_D3D11




typedef IDXGISwapChain DXGISwapChain;
typedef IDXGIFactory DXGIFactory;
#endif

namespace rx
{
class NativeWindow
{
  public:
    explicit NativeWindow(EGLNativeWindowType window);

    
    
    
    inline bool initialize() { return true; }
    inline bool getClientRect(LPRECT rect) { return GetClientRect(mWindow, rect) == TRUE; }
    inline bool isIconic() { return IsIconic(mWindow) == TRUE; }

#ifdef ANGLE_ENABLE_D3D11
    HRESULT createSwapChain(ID3D11Device* device, DXGIFactory* factory,
                            DXGI_FORMAT format, UINT width, UINT height,
                            DXGISwapChain** swapChain);
#endif

    inline EGLNativeWindowType getNativeWindow() const { return mWindow; }

  private:
    EGLNativeWindowType mWindow;
};
}

bool isValidEGLNativeWindowType(EGLNativeWindowType window);

#endif 
