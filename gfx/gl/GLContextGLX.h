





#ifndef GLCONTEXTGLX_H_
#define GLCONTEXTGLX_H_

#include "GLContext.h"
#include "GLXLibrary.h"

namespace mozilla {
namespace gl {

typedef GLXLibrary::LibraryType LibType;

class GLContextGLX : public GLContext
{
public:
    static already_AddRefed<GLContextGLX>
    CreateGLContext(const SurfaceCaps& caps,
                    GLContextGLX* shareContext,
                    bool isOffscreen,
                    Display* display,
                    GLXDrawable drawable,
                    GLXFBConfig cfg,
                    bool deleteDrawable,
                    LibType libType = GLXLibrary::OPENGL_LIB,
                    gfxXlibSurface* pixmap = nullptr);

    ~GLContextGLX();

    virtual GLContextType GetContextType() MOZ_OVERRIDE { return ContextTypeGLX; }

    static GLContextGLX* Cast(GLContext* gl) {
        MOZ_ASSERT(gl->GetContextType() == ContextTypeGLX);
        return static_cast<GLContextGLX*>(gl);
    }

    bool Init();

    bool MakeCurrentImpl(bool aForce = false);

    virtual bool IsCurrent();

    bool SetupLookupFunction();

    bool IsDoubleBuffered();

    bool SupportsRobustness();

    bool SwapBuffers();

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
                 gfxXlibSurface *aPixmap,
                 LibType libType);

    GLXContext mContext;
    Display *mDisplay;
    GLXDrawable mDrawable;
    bool mDeleteDrawable;
    bool mDoubleBuffered;

    GLXLibrary* mGLX;

    nsRefPtr<gfxXlibSurface> mPixmap;
};

}
}

#endif 
