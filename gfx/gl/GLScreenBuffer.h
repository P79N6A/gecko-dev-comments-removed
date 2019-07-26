













#ifndef SCREEN_BUFFER_H_
#define SCREEN_BUFFER_H_

#include "SurfaceTypes.h"
#include "GLContextTypes.h"
#include "GLDefs.h"
#include "gfxPoint.h"


class gfxImageSurface;

namespace mozilla {
    namespace gfx {
        class SurfaceStream;
        class SharedSurface;
    }
    namespace gl {
        class GLContext;
        class SharedSurface_GL;
        class SurfaceFactory_GL;
    }
}

namespace mozilla {
namespace gl {

class DrawBuffer
{
protected:
    typedef struct gfx::SurfaceCaps SurfaceCaps;

public:
    
    static DrawBuffer* Create(GLContext* const gl,
                              const SurfaceCaps& caps,
                              const GLFormats& formats,
                              const gfxIntSize& size);

protected:
    GLContext* const mGL;
    const gfxIntSize mSize;
    const GLuint mFB;
    const GLuint mColorMSRB;
    const GLuint mDepthRB;
    const GLuint mStencilRB;

    DrawBuffer(GLContext* gl,
               const gfxIntSize& size,
               GLuint fb,
               GLuint colorMSRB,
               GLuint depthRB,
               GLuint stencilRB)
        : mGL(gl)
        , mSize(size)
        , mFB(fb)
        , mColorMSRB(colorMSRB)
        , mDepthRB(depthRB)
        , mStencilRB(stencilRB)
    {}

public:
    virtual ~DrawBuffer();

    const gfxIntSize& Size() const {
        return mSize;
    }

    GLuint FB() const {
        return mFB;
    }
};

class ReadBuffer
{
protected:
    typedef struct gfx::SurfaceCaps SurfaceCaps;

public:
    
    static ReadBuffer* Create(GLContext* gl,
                              const SurfaceCaps& caps,
                              const GLFormats& formats,
                              SharedSurface_GL* surf);

protected:
    GLContext* const mGL;

    const GLuint mFB;
    
    const GLuint mDepthRB;
    const GLuint mStencilRB;
    
    SharedSurface_GL* mSurf; 

    ReadBuffer(GLContext* gl,
               GLuint fb,
               GLuint depthRB,
               GLuint stencilRB,
               SharedSurface_GL* surf)
        : mGL(gl)
        , mFB(fb)
        , mDepthRB(depthRB)
        , mStencilRB(stencilRB)
        , mSurf(surf)
    {}

public:
    virtual ~ReadBuffer();

    
    void Attach(SharedSurface_GL* surf);

    const gfxIntSize& Size() const;

    GLuint FB() const {
        return mFB;
    }

    SharedSurface_GL* SharedSurf() const {
        return mSurf;
    }
};


class GLScreenBuffer
{
protected:
    typedef class gfx::SurfaceStream SurfaceStream;
    typedef class gfx::SharedSurface SharedSurface;
    typedef gfx::SurfaceStreamType SurfaceStreamType;
    typedef gfx::SharedSurfaceType SharedSurfaceType;
    typedef struct gfx::SurfaceCaps SurfaceCaps;

public:
    
    static GLScreenBuffer* Create(GLContext* gl,
                                  const gfxIntSize& size,
                                  const SurfaceCaps& caps);

protected:
    GLContext* const mGL;         
    SurfaceCaps mCaps;
    SurfaceFactory_GL* mFactory;  
    SurfaceStream* mStream;       

    DrawBuffer* mDraw;            
    ReadBuffer* mRead;            

    bool mNeedsBlit;

    
    GLuint mUserDrawFB;
    GLuint mUserReadFB;
    GLuint mInternalDrawFB;
    GLuint mInternalReadFB;

#ifdef DEBUG
    bool mInInternalMode_DrawFB;
    bool mInInternalMode_ReadFB;
#endif

    GLScreenBuffer(GLContext* gl,
                   const SurfaceCaps& caps,
                   SurfaceFactory_GL* factory,
                   SurfaceStream* stream)
        : mGL(gl)
        , mCaps(caps)
        , mFactory(factory)
        , mStream(stream)
        , mDraw(nullptr)
        , mRead(nullptr)
        , mNeedsBlit(true)
        , mUserDrawFB(0)
        , mUserReadFB(0)
        , mInternalDrawFB(0)
        , mInternalReadFB(0)
#ifdef DEBUG
        , mInInternalMode_DrawFB(true)
        , mInInternalMode_ReadFB(true)
#endif
    {}

public:
    virtual ~GLScreenBuffer();

    SurfaceStream* Stream() const {
        return mStream;
    }

    SurfaceFactory_GL* Factory() const {
        return mFactory;
    }

    SharedSurface_GL* SharedSurf() const {
        MOZ_ASSERT(mRead);
        return mRead->SharedSurf();
    }

    bool PreserveBuffer() const {
        return mCaps.preserve;
    }

    const SurfaceCaps& Caps() const {
        return mCaps;
    }

    GLuint DrawFB() const {
        if (!mDraw)
            return ReadFB();

        return mDraw->FB();
    }

    GLuint ReadFB() const {
        return mRead->FB();
    }

    void DeletingFB(GLuint fb);

    const gfxIntSize& Size() const {
        MOZ_ASSERT(mRead);
        MOZ_ASSERT(!mDraw || mDraw->Size() == mRead->Size());
        return mRead->Size();
    }

    void BindAsFramebuffer(GLContext* const gl, GLenum target) const;

    void RequireBlit();
    void AssureBlitted();
    void AfterDrawCall();
    void BeforeReadCall();

    











    void Morph(SurfaceFactory_GL* newFactory, SurfaceStreamType streamType);

protected:
    
    bool Swap(const gfxIntSize& size);

public:
    bool PublishFrame(const gfxIntSize& size);

    bool Resize(const gfxIntSize& size);

    void Readback(SharedSurface_GL* src, gfxImageSurface* dest);

protected:
    void Attach(SharedSurface* surface, const gfxIntSize& size);

    DrawBuffer* CreateDraw(const gfxIntSize& size);
    ReadBuffer* CreateRead(SharedSurface_GL* surf);

public:
    




    void BindFB(GLuint fb);
    void BindDrawFB(GLuint fb);
    void BindReadFB(GLuint fb);
    GLuint GetFB() const;
    GLuint GetDrawFB() const;
    GLuint GetReadFB() const;

    
    
    void BindDrawFB_Internal(GLuint fb);
    void BindReadFB_Internal(GLuint fb);
};

}   
}   

#endif  
