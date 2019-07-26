





#ifndef GLCONTEXTCGL_H_
#define GLCONTEXTCGL_H_

#include "GLContext.h"

#include "OpenGL/OpenGL.h"

#ifdef __OBJC__
#include <AppKit/NSOpenGL.h>
#else
typedef void NSOpenGLContext;
#endif

namespace mozilla {
namespace gl {

class GLContextCGL : public GLContext
{
    friend class GLContextProviderCGL;

    NSOpenGLContext *mContext;

public:
    GLContextCGL(const SurfaceCaps& caps,
                 GLContext *shareContext,
                 NSOpenGLContext *context,
                 bool isOffscreen = false);

    ~GLContextCGL();

    virtual GLContextType GetContextType() MOZ_OVERRIDE { return GLContextType::CGL; }

    static GLContextCGL* Cast(GLContext* gl) {
        MOZ_ASSERT(gl->GetContextType() == GLContextType::CGL);
        return static_cast<GLContextCGL*>(gl);
    }

    bool Init();

    NSOpenGLContext* GetNSOpenGLContext() const { return mContext; }
    CGLContextObj GetCGLContext() const;

    bool MakeCurrentImpl(bool aForce = false);

    virtual bool IsCurrent();

    virtual GLenum GetPreferredARGB32Format() MOZ_OVERRIDE;

    bool SetupLookupFunction();

    bool IsDoubleBuffered();

    bool SupportsRobustness();

    bool SwapBuffers();

    bool ResizeOffscreen(const gfx::IntSize& aNewSize);
};

}
}

#endif 
