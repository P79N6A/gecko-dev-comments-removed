






#ifndef SkDebugGLContext_DEFINED
#define SkDebugGLContext_DEFINED

#include "SkGLContextHelper.h"

class SkDebugGLContext : public SkGLContextHelper {

public:
    SkDebugGLContext() {};

    virtual void makeCurrent() const SK_OVERRIDE {};
    virtual void swapBuffers() const SK_OVERRIDE {};

protected:
    virtual const GrGLInterface* createGLContext(GrGLStandard forcedGpuAPI) SK_OVERRIDE;

    virtual void destroyGLContext() SK_OVERRIDE {};
};

#endif
