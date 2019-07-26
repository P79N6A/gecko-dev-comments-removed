






#ifndef SkGLContextHelper_DEFINED
#define SkGLContextHelper_DEFINED

#include "GrGLExtensions.h"
#include "GrGLInterface.h"






class SkGLContextHelper : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkGLContextHelper)

    SkGLContextHelper();
    virtual ~SkGLContextHelper();

    


    bool init(const int width, const int height);

    int getFBOID() const { return fFBO; }

    const GrGLInterface* gl() const { return fGL; }

    virtual void makeCurrent() const = 0;

    bool hasExtension(const char* extensionName) const {
        GrAssert(NULL != fGL);
        return fExtensions.has(extensionName);
    }

protected:
    





    virtual const GrGLInterface* createGLContext() = 0;

    


    virtual void destroyGLContext() = 0;

private:
    GrGLExtensions fExtensions;
    GrGLuint fFBO;
    GrGLuint fColorBufferID;
    GrGLuint fDepthStencilBufferID;
    const GrGLInterface* fGL;

    typedef SkRefCnt INHERITED;
};





#define SK_GL(ctx, X) (ctx).gl()->f ## X;    \
                      SkASSERT(GR_GL_NO_ERROR == (ctx).gl()->fGetError())
#define SK_GL_RET(ctx, RET, X) (RET) = (ctx).gl()->f ## X;    \
                  SkASSERT(GR_GL_NO_ERROR == (ctx).gl()->fGetError())
#define SK_GL_NOERRCHECK(ctx, X) (ctx).gl()->f ## X
#define SK_GL_RET_NOERRCHECK(ctx, RET, X) (RET) = (ctx).gl()->f ## X

#endif
