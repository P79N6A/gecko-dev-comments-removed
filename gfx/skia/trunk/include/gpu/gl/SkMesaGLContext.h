






#ifndef SkMesaGLContext_DEFINED
#define SkMesaGLContext_DEFINED

#include "SkGLContextHelper.h"

#if SK_MESA

class SkMesaGLContext : public SkGLContextHelper {
private:
    typedef intptr_t Context;

public:
    SkMesaGLContext();

    virtual ~SkMesaGLContext();

    virtual void makeCurrent() const SK_OVERRIDE;
    virtual void swapBuffers() const SK_OVERRIDE;

    class AutoContextRestore {
    public:
        AutoContextRestore();
        ~AutoContextRestore();

    private:
        Context fOldContext;
        GrGLint fOldWidth;
        GrGLint fOldHeight;
        GrGLint fOldFormat;
        void*   fOldImage;
    };

protected:
    virtual const GrGLInterface* createGLContext(GrGLStandard forcedGpuAPI) SK_OVERRIDE;
    virtual void destroyGLContext() SK_OVERRIDE;

private:
    Context fContext;
    GrGLubyte *fImage;
};

#endif

#endif
