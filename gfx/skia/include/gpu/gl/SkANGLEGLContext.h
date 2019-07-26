






#ifndef SkANGLEGLContext_DEFINED
#define SkANGLEGLContext_DEFINED

#if SK_ANGLE

#include "SkGLContext.h"

#include <GLES2/gl2.h>
#include <EGL/egl.h>

class SkANGLEGLContext : public SkGLContext {
public:
    SkANGLEGLContext();

    virtual ~SkANGLEGLContext();

    virtual void makeCurrent() const SK_OVERRIDE;

    class AutoContextRestore {
    public:
        AutoContextRestore();
        ~AutoContextRestore();

    private:
        EGLContext fOldEGLContext;
        EGLDisplay fOldDisplay;
        EGLSurface fOldSurface;
    };

protected:
    virtual const GrGLInterface* createGLContext() SK_OVERRIDE;
    virtual void destroyGLContext() SK_OVERRIDE;

private:
    EGLContext fContext;
    EGLDisplay fDisplay;
    EGLSurface fSurface;
};

#endif

#endif
