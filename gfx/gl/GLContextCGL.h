





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
    MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(GLContextCGL, override)
    GLContextCGL(const SurfaceCaps& caps, NSOpenGLContext* context,
                 bool isOffscreen, ContextProfile profile);

    ~GLContextCGL();

    virtual GLContextType GetContextType() const override { return GLContextType::CGL; }

    static GLContextCGL* Cast(GLContext* gl) {
        MOZ_ASSERT(gl->GetContextType() == GLContextType::CGL);
        return static_cast<GLContextCGL*>(gl);
    }

    bool Init() override;

    NSOpenGLContext* GetNSOpenGLContext() const { return mContext; }
    CGLContextObj GetCGLContext() const;

    virtual bool MakeCurrentImpl(bool aForce) override;

    virtual bool IsCurrent() override;

    virtual GLenum GetPreferredARGB32Format() const override;

    virtual bool SetupLookupFunction() override;

    virtual bool IsDoubleBuffered() const override;

    virtual bool SupportsRobustness() const override;

    virtual bool SwapBuffers() override;
};

} 
} 

#endif 
