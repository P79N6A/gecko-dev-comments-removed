






#ifndef SkGLContext_DEFINED
#define SkGLContext_DEFINED

#include "GrGLInterface.h"






class SkGLContext : public SkRefCnt {
public:
    SkGLContext();
    virtual ~SkGLContext();

    


    bool init(const int width, const int height);

    int getFBOID() const { return fFBO; }

    const GrGLInterface* gl() const { return fGL; }

    virtual void makeCurrent() const = 0;

protected:
    





    virtual const GrGLInterface* createGLContext() = 0;

    


    virtual void destroyGLContext() = 0;

private:
    GrGLuint fFBO;
    const GrGLInterface* fGL;
};





#define SK_GL(ctx, X) (ctx).gl()->f ## X

#endif
