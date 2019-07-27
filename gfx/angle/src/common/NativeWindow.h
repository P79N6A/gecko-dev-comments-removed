










#ifndef COMMON_NATIVEWINDOW_H_
#define COMMON_NATIVEWINDOW_H_

#include <EGL/eglplatform.h>
#include "common/debug.h"
#include "common/platform.h"





#if defined(ANGLE_ENABLE_WINDOWS_STORE)
typedef IDXGISwapChain1 DXGISwapChain;
typedef IDXGIFactory2 DXGIFactory;

#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>
#include <windows.applicationmodel.core.h>
#include <memory>

class IInspectableNativeWindow;

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

#else
#ifdef ANGLE_ENABLE_D3D11
typedef IDXGISwapChain DXGISwapChain;
typedef IDXGIFactory DXGIFactory;
#endif
#endif

namespace rx
{
class NativeWindow
{
public:
    explicit NativeWindow(EGLNativeWindowType window);

    bool initialize();
    bool getClientRect(LPRECT rect);
    bool isIconic();

#ifdef ANGLE_ENABLE_D3D11
    HRESULT createSwapChain(ID3D11Device* device, DXGIFactory* factory,
                            DXGI_FORMAT format, UINT width, UINT height,
                            DXGISwapChain** swapChain);
#endif

    inline EGLNativeWindowType getNativeWindow() const { return mWindow; }

private:
    EGLNativeWindowType mWindow;

#if defined(ANGLE_ENABLE_WINDOWS_STORE)
    std::shared_ptr<IInspectableNativeWindow> mImpl;
#endif

};
}

bool isValidEGLNativeWindowType(EGLNativeWindowType window);

#endif 
