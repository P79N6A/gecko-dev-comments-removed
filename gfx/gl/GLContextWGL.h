





#ifndef GLCONTEXTWGL_H_
#define GLCONTEXTWGL_H_

#include "GLContext.h"
#include "WGLLibrary.h"

namespace mozilla {
namespace gl {

class GLContextWGL : public GLContext
{
public:
    
    GLContextWGL(const SurfaceCaps& caps,
                 GLContext* sharedContext,
                 bool isOffscreen,
                 HDC aDC,
                 HGLRC aContext,
                 HWND aWindow = nullptr);

    
    GLContextWGL(const SurfaceCaps& caps,
                 GLContext* sharedContext,
                 bool isOffscreen,
                 HANDLE aPbuffer,
                 HDC aDC,
                 HGLRC aContext,
                 int aPixelFormat);

    ~GLContextWGL();

    virtual GLContextType GetContextType() MOZ_OVERRIDE { return GLContextType::WGL; }

    static GLContextWGL* Cast(GLContext* gl) {
        MOZ_ASSERT(gl->GetContextType() == GLContextType::WGL);
        return static_cast<GLContextWGL*>(gl);
    }

    bool Init();

    bool MakeCurrentImpl(bool aForce = false);

    virtual bool IsCurrent();

    void SetIsDoubleBuffered(bool aIsDB);

    virtual bool IsDoubleBuffered();

    bool SupportsRobustness();

    virtual bool SwapBuffers();

    bool SetupLookupFunction();

    bool ResizeOffscreen(const gfx::IntSize& aNewSize);

    HGLRC Context() { return mContext; }

protected:
    friend class GLContextProviderWGL;

    HDC mDC;
    HGLRC mContext;
    HWND mWnd;
    HANDLE mPBuffer;
    int mPixelFormat;
    bool mIsDoubleBuffered;
};

}
}

#endif 
