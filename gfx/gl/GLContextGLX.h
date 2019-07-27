





#ifndef GLCONTEXTGLX_H_
#define GLCONTEXTGLX_H_

#include "GLContext.h"
#include "GLXLibrary.h"

namespace mozilla {
namespace gl {

class GLContextGLX : public GLContext
{
public:
    MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(GLContextGLX, override)
    static already_AddRefed<GLContextGLX>
    CreateGLContext(const SurfaceCaps& caps,
                    GLContextGLX* shareContext,
                    bool isOffscreen,
                    Display* display,
                    GLXDrawable drawable,
                    GLXFBConfig cfg,
                    bool deleteDrawable,
                    gfxXlibSurface* pixmap = nullptr);

    ~GLContextGLX();

    virtual GLContextType GetContextType() const override { return GLContextType::GLX; }

    static GLContextGLX* Cast(GLContext* gl) {
        MOZ_ASSERT(gl->GetContextType() == GLContextType::GLX);
        return static_cast<GLContextGLX*>(gl);
    }

    bool Init() override;

    virtual bool MakeCurrentImpl(bool aForce) override;

    virtual bool IsCurrent() override;

    virtual bool SetupLookupFunction() override;

    virtual bool IsDoubleBuffered() const override;

    virtual bool SupportsRobustness() const override;

    virtual bool SwapBuffers() override;

private:
    friend class GLContextProviderGLX;

    GLContextGLX(const SurfaceCaps& caps,
                 GLContext* shareContext,
                 bool isOffscreen,
                 Display *aDisplay,
                 GLXDrawable aDrawable,
                 GLXContext aContext,
                 bool aDeleteDrawable,
                 bool aDoubleBuffered,
                 gfxXlibSurface *aPixmap);

    GLXContext mContext;
    Display *mDisplay;
    GLXDrawable mDrawable;
    bool mDeleteDrawable;
    bool mDoubleBuffered;

    GLXLibrary* mGLX;

    nsRefPtr<gfxXlibSurface> mPixmap;
    bool mOwnsContext;
};

}
}

#endif 
