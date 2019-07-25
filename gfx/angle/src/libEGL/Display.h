









#ifndef INCLUDE_DISPLAY_H_
#define INCLUDE_DISPLAY_H_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d9.h>

#include <set>

#include "libGLESv2/Context.h"

#include "libEGL/Config.h"
#include "libEGL/Surface.h"

namespace egl
{
class Display
{
  public:
    Display(HDC deviceContext);

    ~Display();

    bool initialize();
    void terminate();

    virtual void startScene();
    virtual void endScene();

    bool getConfigs(EGLConfig *configs, const EGLint *attribList, EGLint configSize, EGLint *numConfig);
    bool getConfigAttrib(EGLConfig config, EGLint attribute, EGLint *value);

    egl::Surface *createWindowSurface(HWND window, EGLConfig config);
    EGLContext createContext(EGLConfig configHandle);

    void destroySurface(egl::Surface *surface);
    void destroyContext(gl::Context *context);

    bool isInitialized();
    bool isValidConfig(EGLConfig config);
    bool isValidContext(gl::Context *context);
    bool isValidSurface(egl::Surface *surface);
    bool hasExistingWindowSurface(HWND window);

    void setSwapInterval(GLint interval);
    DWORD getPresentInterval();
    static DWORD convertInterval(GLint interval);

    virtual IDirect3DDevice9 *getDevice();
    virtual D3DCAPS9 getDeviceCaps();

  private:
    DISALLOW_COPY_AND_ASSIGN(Display);
    const HDC mDc;

    UINT mAdapter;
    D3DDEVTYPE mDeviceType;
    IDirect3D9 *mD3d9;
    IDirect3DDevice9 *mDevice;
    D3DCAPS9 mDeviceCaps;

    bool mSceneStarted;
    GLint mSwapInterval;
    EGLint mMaxSwapInterval;
    EGLint mMinSwapInterval;
    DWORD mPresentInterval;

    typedef std::set<Surface*> SurfaceSet;
    SurfaceSet mSurfaceSet;

    ConfigSet mConfigSet;

    typedef std::set<gl::Context*> ContextSet;
    ContextSet mContextSet;
};
}

#endif   
