






#ifndef SkNullGLContext_DEFINED
#define SkNullGLContext_DEFINED

#include "SkGLContextHelper.h"

class SkNullGLContext : public SkGLContextHelper {

public:
    SkNullGLContext() {};

    virtual void makeCurrent() const SK_OVERRIDE {};

protected:
    virtual const GrGLInterface* createGLContext() SK_OVERRIDE;

    virtual void destroyGLContext() SK_OVERRIDE {};
};

#endif
