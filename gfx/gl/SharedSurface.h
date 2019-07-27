













#ifndef SHARED_SURFACE_H_
#define SHARED_SURFACE_H_

#include <queue>
#include <stdint.h>

#include "GLContextTypes.h"
#include "GLDefs.h"
#include "mozilla/Attributes.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/WeakPtr.h"
#include "SurfaceTypes.h"

namespace mozilla {
namespace gl {

class GLContext;
class SurfaceFactory;

class SharedSurface
{
public:
    static void ProdCopy(SharedSurface* src, SharedSurface* dest,
                         SurfaceFactory* factory);

    const SharedSurfaceType mType;
    const AttachmentType mAttachType;
    GLContext* const mGL;
    const gfx::IntSize mSize;
    const bool mHasAlpha;
protected:
    bool mIsLocked;

    SharedSurface(SharedSurfaceType type,
                  AttachmentType attachType,
                  GLContext* gl,
                  const gfx::IntSize& size,
                  bool hasAlpha)
        : mType(type)
        , mAttachType(attachType)
        , mGL(gl)
        , mSize(size)
        , mHasAlpha(hasAlpha)
        , mIsLocked(false)
    {
    }

public:
    virtual ~SharedSurface() {
    }

    bool IsLocked() const {
        return mIsLocked;
    }

    
    
    void LockProd();

    
    void UnlockProd();

protected:
    virtual void LockProdImpl() = 0;
    virtual void UnlockProdImpl() = 0;

public:
    virtual void Fence() = 0;
    virtual bool WaitSync() = 0;
    virtual bool PollSync() = 0;

    
    
    
    virtual void WaitForBufferOwnership() {}

    
    virtual GLenum ProdTextureTarget() const {
        MOZ_ASSERT(mAttachType == AttachmentType::GLTexture);
        return LOCAL_GL_TEXTURE_2D;
    }

    virtual GLuint ProdTexture() {
        MOZ_ASSERT(mAttachType == AttachmentType::GLTexture);
        MOZ_CRASH("Did you forget to override this function?");
    }

    virtual GLuint ProdRenderbuffer() {
        MOZ_ASSERT(mAttachType == AttachmentType::GLRenderbuffer);
        MOZ_CRASH("Did you forget to override this function?");
    }

    virtual bool ReadPixels(GLint x, GLint y,
                            GLsizei width, GLsizei height,
                            GLenum format, GLenum type,
                            GLvoid* pixels)
    {
        return false;
    }
};

template<typename T>
class UniquePtrQueue
{
    std::queue<T*> mQueue;

public:
    ~UniquePtrQueue() {
        MOZ_ASSERT(Empty());
    }
    
    bool Empty() const {
        return mQueue.empty();
    }

    void Push(UniquePtr<T> up) {
        T* p = up.release();
        mQueue.push(p);
    }

    UniquePtr<T> Pop() {
        UniquePtr<T> ret;
        if (mQueue.empty())
            return Move(ret);

        T* p = mQueue.front();
        mQueue.pop();

        ret.reset(p);
        return Move(ret);
    }
};

class SurfaceFactory
{
public:
    GLContext* const mGL;
    const SurfaceCaps mCaps;
    const SharedSurfaceType mType;
    const GLFormats mFormats;

protected:
    SurfaceCaps mDrawCaps;
    SurfaceCaps mReadCaps;

    SurfaceFactory(GLContext* gl,
                   SharedSurfaceType type,
                   const SurfaceCaps& caps);

public:
    virtual ~SurfaceFactory();

    const SurfaceCaps& DrawCaps() const {
        return mDrawCaps;
    }

    const SurfaceCaps& ReadCaps() const {
        return mReadCaps;
    }

protected:
    virtual UniquePtr<SharedSurface> CreateShared(const gfx::IntSize& size) = 0;

    UniquePtrQueue<SharedSurface> mScraps;

public:
    UniquePtr<SharedSurface> NewSharedSurface(const gfx::IntSize& size);

    
    void Recycle(UniquePtr<SharedSurface> surf);
};

} 
} 

#endif 
