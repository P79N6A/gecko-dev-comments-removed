





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
    MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(GLContextCGL, MOZ_OVERRIDE)
    GLContextCGL(const SurfaceCaps& caps, NSOpenGLContext* context,
                 bool isOffscreen, ContextProfile profile);

    ~GLContextCGL();

    virtual GLContextType GetContextType() const MOZ_OVERRIDE { return GLContextType::CGL; }

    static GLContextCGL* Cast(GLContext* gl) {
        MOZ_ASSERT(gl->GetContextType() == GLContextType::CGL);
        return static_cast<GLContextCGL*>(gl);
    }

    bool Init() MOZ_OVERRIDE;

    NSOpenGLContext* GetNSOpenGLContext() const { return mContext; }
    CGLContextObj GetCGLContext() const;

    virtual bool MakeCurrentImpl(bool aForce) MOZ_OVERRIDE;

    virtual bool IsCurrent() MOZ_OVERRIDE;

    virtual GLenum GetPreferredARGB32Format() const MOZ_OVERRIDE;

    virtual bool SetupLookupFunction() MOZ_OVERRIDE;

    virtual bool IsDoubleBuffered() const MOZ_OVERRIDE;

    virtual bool SupportsRobustness() const MOZ_OVERRIDE;

    virtual bool SwapBuffers() MOZ_OVERRIDE;
};

}
}

#endif 
