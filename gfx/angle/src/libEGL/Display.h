









#ifndef LIBEGL_DISPLAY_H_
#define LIBEGL_DISPLAY_H_

#include <set>
#include <vector>

#include "libEGL/Config.h"

namespace gl
{
class Context;
}

namespace egl
{
class Surface;

class Display
{
  public:
    ~Display();

    bool initialize();
    void terminate();

    static egl::Display *getDisplay(EGLNativeDisplayType displayId);

    static const char *getExtensionString(egl::Display *display);

    bool getConfigs(EGLConfig *configs, const EGLint *attribList, EGLint configSize, EGLint *numConfig);
    bool getConfigAttrib(EGLConfig config, EGLint attribute, EGLint *value);

    EGLSurface createWindowSurface(HWND window, EGLConfig config, const EGLint *attribList);
    EGLSurface createOffscreenSurface(EGLConfig config, HANDLE shareHandle, const EGLint *attribList);
    EGLContext createContext(EGLConfig configHandle, EGLint clientVersion, const gl::Context *shareContext, bool notifyResets, bool robustAccess);

    void destroySurface(egl::Surface *surface);
    void destroyContext(gl::Context *context);

    bool isInitialized() const;
    bool isValidConfig(EGLConfig config);
    bool isValidContext(gl::Context *context);
    bool isValidSurface(egl::Surface *surface);
    bool hasExistingWindowSurface(HWND window);

    rx::Renderer *getRenderer() { return mRenderer; };

    
    virtual void notifyDeviceLost();
    virtual void recreateSwapChains();

    const char *getExtensionString() const;
    const char *getVendorString() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Display);

    Display(EGLNativeDisplayType displayId, HDC deviceContext);

    bool restoreLostDevice();

    EGLNativeDisplayType mDisplayId;
    const HDC mDc;

    bool mSoftwareDevice;
    
    typedef std::set<Surface*> SurfaceSet;
    SurfaceSet mSurfaceSet;

    ConfigSet mConfigSet;

    typedef std::set<gl::Context*> ContextSet;
    ContextSet mContextSet;

    rx::Renderer *mRenderer;

    static std::string generateClientExtensionString();

    void initDisplayExtensionString();
    std::string mDisplayExtensionString;

    void initVendorString();
    std::string mVendorString;
};
}

#endif   
