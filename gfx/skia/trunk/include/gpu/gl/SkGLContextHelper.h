






#ifndef SkGLContextHelper_DEFINED
#define SkGLContextHelper_DEFINED

#include "GrGLInterface.h"






class SK_API SkGLContextHelper : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkGLContextHelper)

    SkGLContextHelper();
    virtual ~SkGLContextHelper();

    


    bool init(GrGLStandard forcedGpuAPI, const int width, const int height);

    int getFBOID() const { return fFBO; }

    const GrGLInterface* gl() const { return fGL; }

    virtual void makeCurrent() const = 0;

    









    virtual void swapBuffers() const = 0;

    bool hasExtension(const char* extensionName) const {
        SkASSERT(NULL != fGL);
        return fGL->hasExtension(extensionName);
    }

protected:
    





    virtual const GrGLInterface* createGLContext(GrGLStandard forcedGpuAPI) = 0;

    


    virtual void destroyGLContext() = 0;

private:
    GrGLuint fFBO;
    GrGLuint fColorBufferID;
    GrGLuint fDepthStencilBufferID;
    const GrGLInterface* fGL;

    typedef SkRefCnt INHERITED;
};





#define SK_GL(ctx, X) (ctx).gl()->fFunctions.f ## X;    \
                      SkASSERT(0 == (ctx).gl()->fFunctions.fGetError())
#define SK_GL_RET(ctx, RET, X) (RET) = (ctx).gl()->fFunctions.f ## X;    \
                  SkASSERT(0 == (ctx).gl()->fFunctions.fGetError())
#define SK_GL_NOERRCHECK(ctx, X) (ctx).gl()->fFunctions.f ## X
#define SK_GL_RET_NOERRCHECK(ctx, RET, X) (RET) = (ctx).gl()->fFunctions.f ## X

#endif
