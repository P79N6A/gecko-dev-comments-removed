




#ifndef SHARED_SURFACE_ANGLE_H_
#define SHARED_SURFACE_ANGLE_H_

#include <windows.h>
#include "SharedSurface.h"

struct IDXGIKeyedMutex;
struct ID3D11Texture2D;

namespace mozilla {
namespace gl {

class GLContext;
class GLLibraryEGL;

class SharedSurface_ANGLEShareHandle
    : public SharedSurface
{
public:
    static UniquePtr<SharedSurface_ANGLEShareHandle> Create(GLContext* gl,
                                                            EGLContext context,
                                                            EGLConfig config,
                                                            const gfx::IntSize& size,
                                                            bool hasAlpha);

    static SharedSurface_ANGLEShareHandle* Cast(SharedSurface* surf) {
        MOZ_ASSERT(surf->mType == SharedSurfaceType::EGLSurfaceANGLE);

        return (SharedSurface_ANGLEShareHandle*)surf;
    }

protected:
    GLLibraryEGL* const mEGL;
    const EGLContext mContext;
    const EGLSurface mPBuffer;
    const HANDLE mShareHandle;
    RefPtr<IDXGIKeyedMutex> mKeyedMutex;
    RefPtr<IDXGIKeyedMutex> mConsumerKeyedMutex;
    RefPtr<ID3D11Texture2D> mConsumerTexture;

    const GLuint mFence;

    SharedSurface_ANGLEShareHandle(GLContext* gl,
                                   GLLibraryEGL* egl,
                                   const gfx::IntSize& size,
                                   bool hasAlpha,
                                   EGLContext context,
                                   EGLSurface pbuffer,
                                   HANDLE shareHandle,
                                   const RefPtr<IDXGIKeyedMutex>& keyedMutex,
                                   GLuint fence);

    EGLDisplay Display();

public:
    virtual ~SharedSurface_ANGLEShareHandle();

    virtual void LockProdImpl() override;
    virtual void UnlockProdImpl() override;

    virtual void Fence() override;
    virtual void ProducerAcquireImpl() override;
    virtual void ProducerReleaseImpl() override;
    virtual void ConsumerAcquireImpl() override;
    virtual void ConsumerReleaseImpl() override;
    virtual bool WaitSync() override;
    virtual bool PollSync() override;

    virtual void Fence_ContentThread_Impl() override;
    virtual bool WaitSync_ContentThread_Impl() override;
    virtual bool PollSync_ContentThread_Impl() override;

    
    HANDLE GetShareHandle() {
        return mShareHandle;
    }

    const RefPtr<ID3D11Texture2D>& GetConsumerTexture() const {
        return mConsumerTexture;
    }
};



class SurfaceFactory_ANGLEShareHandle
    : public SurfaceFactory
{
protected:
    GLContext* const mProdGL;
    GLLibraryEGL* const mEGL;
    EGLContext mContext;
    EGLConfig mConfig;

public:
    static UniquePtr<SurfaceFactory_ANGLEShareHandle> Create(GLContext* gl,
                                                             const SurfaceCaps& caps);

protected:
    SurfaceFactory_ANGLEShareHandle(GLContext* gl,
                                    GLLibraryEGL* egl,
                                    const SurfaceCaps& caps,
                                    bool* const out_success);

    virtual UniquePtr<SharedSurface> CreateShared(const gfx::IntSize& size) override {
        bool hasAlpha = mReadCaps.alpha;
        return SharedSurface_ANGLEShareHandle::Create(mProdGL,
                                                      mContext, mConfig,
                                                      size, hasAlpha);
    }
};

} 
} 

#endif 
