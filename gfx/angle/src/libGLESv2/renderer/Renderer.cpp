#include "precompiled.h"








#include <EGL/eglext.h>
#include "libGLESv2/main.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/renderer/Renderer.h"
#include "common/utilities.h"
#include "third_party/trace_event/trace_event.h"
#include "libGLESv2/Shader.h"

#if defined (ANGLE_ENABLE_D3D9)
#include "libGLESv2/renderer/d3d/d3d9/Renderer9.h"
#endif 

#if defined (ANGLE_ENABLE_D3D11)
#include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"
#endif 

#if !defined(ANGLE_DEFAULT_D3D11)

#define ANGLE_DEFAULT_D3D11 0
#endif

namespace rx
{

Renderer::Renderer(egl::Display *display)
    : mDisplay(display),
      mCapsInitialized(false),
      mCurrentClientVersion(2)
{
}

Renderer::~Renderer()
{
    gl::Shader::releaseCompiler();
}

const gl::Caps &Renderer::getCaps() const
{
    if (!mCapsInitialized)
    {
        mCaps = generateCaps();
        mCapsInitialized = true;
    }

    return mCaps;
}

}

extern "C"
{

rx::Renderer *glCreateRenderer(egl::Display *display, HDC hDc, EGLNativeDisplayType displayId)
{
#if defined(ANGLE_ENABLE_D3D11)
    if (ANGLE_DEFAULT_D3D11 ||
        displayId == EGL_D3D11_ELSE_D3D9_DISPLAY_ANGLE ||
        displayId == EGL_D3D11_ONLY_DISPLAY_ANGLE)
    {
        rx::Renderer11 *renderer = new rx::Renderer11(display, hDc);
        if (renderer->initialize() == EGL_SUCCESS)
        {
            return renderer;
        }
        else
        {
            
            SafeDelete(renderer);
        }
    }
#endif

#if defined(ANGLE_ENABLE_D3D9)
    if (displayId != EGL_D3D11_ONLY_DISPLAY_ANGLE)
    {
        rx::Renderer9 *renderer = new rx::Renderer9(display, hDc);
        if (renderer->initialize() == EGL_SUCCESS)
        {
            return renderer;
        }
        else
        {
            SafeDelete(renderer);
        }
    }
#endif

    return NULL;
}

void glDestroyRenderer(rx::Renderer *renderer)
{
    delete renderer;
}

}
