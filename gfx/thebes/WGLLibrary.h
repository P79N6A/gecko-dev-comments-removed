



































#include "GLContext.h"

namespace mozilla {
namespace gl {

class WGLLibrary
{
public:
    WGLLibrary() : mInitialized(PR_FALSE), mOGLLibrary(nsnull) {}

    typedef HGLRC (GLAPIENTRY * PFNWGLCREATECONTEXTPROC) (HDC);
    PFNWGLCREATECONTEXTPROC fCreateContext;
    typedef BOOL (GLAPIENTRY * PFNWGLDELETECONTEXTPROC) (HGLRC);
    PFNWGLDELETECONTEXTPROC fDeleteContext;
    typedef BOOL (GLAPIENTRY * PFNWGLMAKECURRENTPROC) (HDC, HGLRC);
    PFNWGLMAKECURRENTPROC fMakeCurrent;
    typedef PROC (GLAPIENTRY * PFNWGLGETPROCADDRESSPROC) (LPCSTR);
    PFNWGLGETPROCADDRESSPROC fGetProcAddress;
    typedef HGLRC (GLAPIENTRY * PFNWGLGETCURRENTCONTEXT) (void);
    PFNWGLGETCURRENTCONTEXT fGetCurrentContext;
    typedef HDC (GLAPIENTRY * PFNWGLGETCURRENTDC) (void);
    PFNWGLGETCURRENTDC fGetCurrentDC;
    typedef BOOL (GLAPIENTRY * PFNWGLSHARELISTS) (HGLRC oldContext, HGLRC newContext);
    PFNWGLSHARELISTS fShareLists;

    typedef HANDLE (WINAPI * PFNWGLCREATEPBUFFERPROC) (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int* piAttribList);
    PFNWGLCREATEPBUFFERPROC fCreatePbuffer;
    typedef BOOL (WINAPI * PFNWGLDESTROYPBUFFERPROC) (HANDLE hPbuffer);
    PFNWGLDESTROYPBUFFERPROC fDestroyPbuffer;
    typedef HDC (WINAPI * PFNWGLGETPBUFFERDCPROC) (HANDLE hPbuffer);
    PFNWGLGETPBUFFERDCPROC fGetPbufferDC;

    typedef BOOL (WINAPI * PFNWGLBINDTEXIMAGEPROC) (HANDLE hPbuffer, int iBuffer);
    PFNWGLBINDTEXIMAGEPROC fBindTexImage;
    typedef BOOL (WINAPI * PFNWGLRELEASETEXIMAGEPROC) (HANDLE hPbuffer, int iBuffer);
    PFNWGLRELEASETEXIMAGEPROC fReleaseTexImage;

    typedef BOOL (WINAPI * PFNWGLCHOOSEPIXELFORMATPROC) (HDC hdc, const int* piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
    PFNWGLCHOOSEPIXELFORMATPROC fChoosePixelFormat;
    typedef BOOL (WINAPI * PFNWGLGETPIXELFORMATATTRIBIVPROC) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int* piAttributes, int *piValues);
    PFNWGLGETPIXELFORMATATTRIBIVPROC fGetPixelFormatAttribiv;

    bool EnsureInitialized();

private:
    bool mInitialized;
    PRLibrary *mOGLLibrary;
};


extern WGLLibrary sWGLLibrary;

} 
} 

