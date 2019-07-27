







#ifndef LIBEGL_MAIN_H_
#define LIBEGL_MAIN_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace egl
{
struct Current
{
    EGLint error;
    EGLenum API;
    EGLDisplay display;
    EGLSurface drawSurface;
    EGLSurface readSurface;
};

void setCurrentError(EGLint error);
EGLint getCurrentError();

void setCurrentAPI(EGLenum API);
EGLenum getCurrentAPI();

void setCurrentDisplay(EGLDisplay dpy);
EGLDisplay getCurrentDisplay();

void setCurrentDrawSurface(EGLSurface surface);
EGLSurface getCurrentDrawSurface();

void setCurrentReadSurface(EGLSurface surface);
EGLSurface getCurrentReadSurface();

void error(EGLint errorCode);

template<class T>
const T &error(EGLint errorCode, const T &returnValue)
{
    error(errorCode);

    return returnValue;
}

template<class T>
const T &success(const T &returnValue)
{
    egl::setCurrentError(EGL_SUCCESS);

    return returnValue;
}

}

#endif  
