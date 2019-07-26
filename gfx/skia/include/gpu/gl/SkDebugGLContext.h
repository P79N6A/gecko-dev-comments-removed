






#ifndef SkDebugGLContext_DEFINED
#define SkDebugGLContext_DEFINED

#include "SkGLContext.h"

class SkDebugGLContext : public SkGLContext {

public:
    SkDebugGLContext() {};

    virtual void makeCurrent() const SK_OVERRIDE {};

protected:
    virtual const GrGLInterface* createGLContext() SK_OVERRIDE;

    virtual void destroyGLContext() SK_OVERRIDE {};
};

#endif

