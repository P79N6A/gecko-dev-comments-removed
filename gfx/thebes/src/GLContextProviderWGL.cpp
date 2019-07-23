


































#include "GLContextProvider.h"
#include "nsDebug.h"
#include "nsIWidget.h"

namespace mozilla {
namespace gl {

GLContextProvider sGLContextProvider;

static class WGLLibrary
{
public:
    WGLLibrary() : mInitialized(PR_FALSE) {}

    typedef HGLRC (GLAPIENTRY * PFNWGLCREATECONTEXTPROC) (HDC);
    PFNWGLCREATECONTEXTPROC fCreateContext;
    typedef BOOL (GLAPIENTRY * PFNWGLDELETECONTEXTPROC) (HGLRC);
    PFNWGLDELETECONTEXTPROC fDeleteContext;
    typedef BOOL (GLAPIENTRY * PFNWGLMAKECURRENTPROC) (HDC, HGLRC);
    PFNWGLMAKECURRENTPROC fMakeCurrent;
    typedef PROC (GLAPIENTRY * PFNWGLGETPROCADDRESSPROC) (LPCSTR);
    PFNWGLGETPROCADDRESSPROC fGetProcAddress;

    PRBool EnsureInitialized()
    {
        if (mInitialized) {
            return PR_TRUE;
        }
        if (!mOGLLibrary) {
            mOGLLibrary = PR_LoadLibrary("Opengl32.dll");
            if (!mOGLLibrary) {
                NS_WARNING("Couldn't load OpenGL DLL.");
                return PR_FALSE;
            }
        }
        fCreateContext = (PFNWGLCREATECONTEXTPROC)
          PR_FindFunctionSymbol(mOGLLibrary, "wglCreateContext");
        fDeleteContext = (PFNWGLDELETECONTEXTPROC)
          PR_FindFunctionSymbol(mOGLLibrary, "wglDeleteContext");
        fMakeCurrent = (PFNWGLMAKECURRENTPROC)
          PR_FindFunctionSymbol(mOGLLibrary, "wglMakeCurrent");
        fGetProcAddress = (PFNWGLGETPROCADDRESSPROC)
          PR_FindFunctionSymbol(mOGLLibrary, "wglGetProcAddress");
        if (!fCreateContext || !fDeleteContext ||
            !fMakeCurrent || !fGetProcAddress) {
            return PR_FALSE;
        }
        mInitialized = PR_TRUE;
        return PR_TRUE;
    }

private:
    PRBool mInitialized;
    PRLibrary *mOGLLibrary;
} sWGLLibrary;

class GLContextWGL : public GLContext
{
public:
    GLContextWGL(HDC aDC, HGLRC aContext)
        : mContext(aContext), mDC(aDC) {}

    ~GLContextWGL()
    {
        sWGLLibrary.fDeleteContext(mContext);
    }

    PRBool Init()
    {
        MakeCurrent();
        SetupLookupFunction();
        return InitWithPrefix("gl", PR_TRUE);
    }

    PRBool MakeCurrent()
    {
        BOOL succeeded = sWGLLibrary.fMakeCurrent(mDC, mContext);
        NS_ASSERTION(succeeded, "Failed to make GL context current!");
        return succeeded;
    }

    PRBool SetupLookupFunction()
    {
        mLookupFunc = (PlatformLookupFunction)sWGLLibrary.fGetProcAddress;
        return PR_TRUE;
    }

private:
    HGLRC mContext;
    HDC mDC;
};

already_AddRefed<GLContext>
GLContextProvider::CreateForWindow(nsIWidget *aWidget)
{
    if (!sWGLLibrary.EnsureInitialized()) {
        return nsnull;
    }
    





    HDC dc = (HDC)aWidget->GetNativeData(NS_NATIVE_GRAPHIC);

    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int iFormat = ChoosePixelFormat(dc, &pfd);

    SetPixelFormat(dc, iFormat, &pfd);
    HGLRC context = sWGLLibrary.fCreateContext(dc);
    if (!context) {
        return nsnull;
    }

    nsRefPtr<GLContextWGL> glContext = new GLContextWGL(dc, context);
    glContext->Init();

    return glContext.forget().get();
}

already_AddRefed<GLContext>
GLContextProvider::CreatePbuffer(const gfxSize &)
{
    return nsnull;
}

} 
} 
