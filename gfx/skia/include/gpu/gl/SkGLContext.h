






#ifndef SkGLContext_DEFINED
#define SkGLContext_DEFINED

#include "GrGLInterface.h"
#include "SkString.h"






class SkGLContext : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkGLContext)

    SkGLContext();
    virtual ~SkGLContext();

    


    bool init(const int width, const int height);

    int getFBOID() const { return fFBO; }

    const GrGLInterface* gl() const { return fGL; }

    virtual void makeCurrent() const = 0;

    bool hasExtension(const char* extensionName) const;

protected:
    





    virtual const GrGLInterface* createGLContext() = 0;

    


    virtual void destroyGLContext() = 0;

private:
    SkString fExtensionString;
    GrGLuint fFBO;
    GrGLuint fColorBufferID;
    GrGLuint fDepthStencilBufferID;
    const GrGLInterface* fGL;

    typedef SkRefCnt INHERITED;
};





#define SK_GL(ctx, X) (ctx).gl()->f ## X

#endif
