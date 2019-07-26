






#ifndef SkNullGLContext_DEFINED
#define SkNullGLContext_DEFINED

#include "SkGLContext.h"

class SkNullGLContext : public SkGLContext {

public:
    SkNullGLContext() {};

    virtual void makeCurrent() const SK_OVERRIDE {};

protected:
    virtual const GrGLInterface* createGLContext() SK_OVERRIDE;

    virtual void destroyGLContext() SK_OVERRIDE {};
};

#endif

