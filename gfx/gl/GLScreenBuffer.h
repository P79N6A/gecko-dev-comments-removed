













#ifndef SCREEN_BUFFER_H_
#define SCREEN_BUFFER_H_

#include "GLContextTypes.h"
#include "GLDefs.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/UniquePtr.h"
#include "SharedSurface.h"
#include "SurfaceTypes.h"

namespace mozilla {
namespace gl {

class GLContext;
class SharedSurface;
class ShSurfHandle;
class SurfaceFactory;

class DrawBuffer
{
public:
    
    
    static bool Create(GLContext* const gl,
                       const SurfaceCaps& caps,
                       const GLFormats& formats,
                       const gfx::IntSize& size,
                       UniquePtr<DrawBuffer>* out_buffer);

protected:
    GLContext* const mGL;
public:
    const gfx::IntSize mSize;
    const GLsizei mSamples;
    const GLuint mFB;
protected:
    const GLuint mColorMSRB;
    const GLuint mDepthRB;
    const GLuint mStencilRB;

    DrawBuffer(GLContext* gl,
               const gfx::IntSize& size,
               GLsizei samples,
               GLuint fb,
               GLuint colorMSRB,
               GLuint depthRB,
               GLuint stencilRB)
        : mGL(gl)
        , mSize(size)
        , mSamples(samples)
        , mFB(fb)
        , mColorMSRB(colorMSRB)
        , mDepthRB(depthRB)
        , mStencilRB(stencilRB)
    {}

public:
    virtual ~DrawBuffer();
};

class ReadBuffer
{
public:
    
    static UniquePtr<ReadBuffer> Create(GLContext* gl,
                                        const SurfaceCaps& caps,
                                        const GLFormats& formats,
                                        SharedSurface* surf);

protected:
    GLContext* const mGL;
public:
    const GLuint mFB;
protected:
    
    const GLuint mDepthRB;
    const GLuint mStencilRB;
    
    SharedSurface* mSurf;

    ReadBuffer(GLContext* gl,
               GLuint fb,
               GLuint depthRB,
               GLuint stencilRB,
               SharedSurface* surf)
        : mGL(gl)
        , mFB(fb)
        , mDepthRB(depthRB)
        , mStencilRB(stencilRB)
        , mSurf(surf)
    {}

public:
    virtual ~ReadBuffer();

    
    void Attach(SharedSurface* surf);

    const gfx::IntSize& Size() const;

    SharedSurface* SharedSurf() const {
        return mSurf;
    }

    void SetReadBuffer(GLenum mode) const;
};


class GLScreenBuffer
{
public:
    
    static UniquePtr<GLScreenBuffer> Create(GLContext* gl,
                                            const gfx::IntSize& size,
                                            const SurfaceCaps& caps);

protected:
    GLContext* const mGL; 
public:
    const SurfaceCaps mCaps;
protected:
    UniquePtr<SurfaceFactory> mFactory;

    RefPtr<ShSurfHandle> mBack;
    RefPtr<ShSurfHandle> mFront;

    UniquePtr<DrawBuffer> mDraw;
    UniquePtr<ReadBuffer> mRead;

    bool mNeedsBlit;

    GLenum mUserReadBufferMode;

    
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
                   UniquePtr<SurfaceFactory> factory)
        : mGL(gl)
        , mCaps(caps)
        , mFactory(Move(factory))
        , mNeedsBlit(true)
        , mUserReadBufferMode(LOCAL_GL_BACK)
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

    SurfaceFactory* Factory() const {
        return mFactory.get();
    }

    ShSurfHandle* Front() const {
        return mFront;
    }

    SharedSurface* SharedSurf() const {
        MOZ_ASSERT(mRead);
        return mRead->SharedSurf();
    }

    bool ShouldPreserveBuffer() const {
        return mCaps.preserve;
    }

    GLuint DrawFB() const {
        if (!mDraw)
            return ReadFB();

        return mDraw->mFB;
    }

    GLuint ReadFB() const {
        return mRead->mFB;
    }

    GLsizei Samples() const {
        if (!mDraw)
            return 1;

        return mDraw->mSamples;
    }

    void DeletingFB(GLuint fb);

    const gfx::IntSize& Size() const {
        MOZ_ASSERT(mRead);
        MOZ_ASSERT(!mDraw || mDraw->mSize == mRead->Size());
        return mRead->Size();
    }

    void BindAsFramebuffer(GLContext* const gl, GLenum target) const;

    void RequireBlit();
    void AssureBlitted();
    void AfterDrawCall();
    void BeforeReadCall();

    void SetReadBuffer(GLenum userMode);

    






    bool ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                    GLenum format, GLenum type, GLvoid *pixels);

    
    void Morph(UniquePtr<SurfaceFactory> newFactory);

protected:
    
    bool Swap(const gfx::IntSize& size);

public:
    bool PublishFrame(const gfx::IntSize& size);

    bool Resize(const gfx::IntSize& size);

protected:
    bool Attach(SharedSurface* surf, const gfx::IntSize& size);

    bool CreateDraw(const gfx::IntSize& size, UniquePtr<DrawBuffer>* out_buffer);
    UniquePtr<ReadBuffer> CreateRead(SharedSurface* surf);

public:
    




    void BindFB(GLuint fb);
    void BindDrawFB(GLuint fb);
    void BindReadFB(GLuint fb);
    GLuint GetFB() const;
    GLuint GetDrawFB() const;
    GLuint GetReadFB() const;

    
    
    void BindFB_Internal(GLuint fb);
    void BindDrawFB_Internal(GLuint fb);
    void BindReadFB_Internal(GLuint fb);

    bool IsDrawFramebufferDefault() const;
    bool IsReadFramebufferDefault() const;
};

}   
}   

#endif  
