













#ifndef SHARED_SURFACE_H_
#define SHARED_SURFACE_H_

#include <queue>
#include <stdint.h>

#include "GLContextTypes.h"
#include "GLDefs.h"
#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/WeakPtr.h"
#include "ScopedGLHelpers.h"
#include "SurfaceTypes.h"

class nsIThread;

namespace mozilla {
namespace gfx {
class DrawTarget;
}
namespace gl {

class GLContext;
class SurfaceFactory;
class ShSurfHandle;

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
    bool mIsProducerAcquired;
    bool mIsConsumerAcquired;
    DebugOnly<nsIThread* const> mOwningThread;

    SharedSurface(SharedSurfaceType type,
                  AttachmentType attachType,
                  GLContext* gl,
                  const gfx::IntSize& size,
                  bool hasAlpha);

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

    virtual void ProducerAcquireImpl() {}
    virtual void ProducerReleaseImpl() {
        Fence();
    }
    virtual void ConsumerAcquireImpl() {
        WaitSync();
    }
    virtual void ConsumerReleaseImpl() {}

public:
    void ProducerAcquire() {
        MOZ_ASSERT(!mIsProducerAcquired);
        ProducerAcquireImpl();
        mIsProducerAcquired = true;
    }
    void ProducerRelease() {
        MOZ_ASSERT(mIsProducerAcquired);
        ProducerReleaseImpl();
        mIsProducerAcquired = false;
    }
    void ConsumerAcquire() {
        MOZ_ASSERT(!mIsConsumerAcquired);
        ConsumerAcquireImpl();
        mIsConsumerAcquired = true;
    }
    void ConsumerRelease() {
        MOZ_ASSERT(mIsConsumerAcquired);
        ConsumerReleaseImpl();
        mIsConsumerAcquired = false;
    }

    virtual void Fence() = 0;
    virtual bool WaitSync() = 0;
    virtual bool PollSync() = 0;

    
    
    void Fence_ContentThread();
    bool WaitSync_ContentThread();
    bool PollSync_ContentThread();

protected:
    virtual void Fence_ContentThread_Impl() {
        Fence();
    }
    virtual bool WaitSync_ContentThread_Impl() {
        return WaitSync();
    }
    virtual bool PollSync_ContentThread_Impl() {
        return PollSync();
    }

public:
    
    
    
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

    virtual bool CopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x,
                                GLint y, GLsizei width, GLsizei height, GLint border)
    {
        return false;
    }

    virtual bool ReadPixels(GLint x, GLint y,
                            GLsizei width, GLsizei height,
                            GLenum format, GLenum type,
                            GLvoid* pixels)
    {
        return false;
    }

    virtual bool NeedsIndirectReads() const {
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

        if (!mQueue.empty()) {
            ret.reset(mQueue.front());
            mQueue.pop();
        }

        return Move(ret);
    }
};

class SurfaceFactory : public SupportsWeakPtr<SurfaceFactory>
{
public:
    
    
    MOZ_DECLARE_WEAKREFERENCE_TYPENAME(SurfaceFactory)

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
    TemporaryRef<ShSurfHandle> NewShSurfHandle(const gfx::IntSize& size);

    
    void Recycle(UniquePtr<SharedSurface> surf);
};

class ShSurfHandle : public RefCounted<ShSurfHandle>
{
public:
    MOZ_DECLARE_REFCOUNTED_TYPENAME(ShSurfHandle)

private:
    const WeakPtr<SurfaceFactory> mFactory;
    UniquePtr<SharedSurface> mSurf;

public:
    ShSurfHandle(SurfaceFactory* factory, UniquePtr<SharedSurface> surf)
        : mFactory(factory)
        , mSurf(Move(surf))
    {
        MOZ_ASSERT(mFactory);
        MOZ_ASSERT(mSurf);
    }

    ~ShSurfHandle() {
        if (mFactory) {
            mFactory->Recycle(Move(mSurf));
        }
    }

    SharedSurface* Surf() const {
        MOZ_ASSERT(mSurf.get());
        return mSurf.get();
    }
};

class ScopedReadbackFB
{
    GLContext* const mGL;
    ScopedBindFramebuffer mAutoFB;
    GLuint mTempFB;
    GLuint mTempTex;
    SharedSurface* mSurfToUnlock;
    SharedSurface* mSurfToLock;

public:
    explicit ScopedReadbackFB(SharedSurface* src);
    ~ScopedReadbackFB();
};

bool ReadbackSharedSurface(SharedSurface* src, gfx::DrawTarget* dst);
uint32_t ReadPixel(SharedSurface* src);

} 
} 

#endif 
