




#ifndef SHARED_SURFACE_ANGLE_H_
#define SHARED_SURFACE_ANGLE_H_

#include <windows.h>

#include "SharedSurface.h"

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
    const GLuint mFence;

    SharedSurface_ANGLEShareHandle(GLContext* gl,
                                   GLLibraryEGL* egl,
                                   const gfx::IntSize& size,
                                   bool hasAlpha,
                                   EGLContext context,
                                   EGLSurface pbuffer,
                                   HANDLE shareHandle,
                                   GLuint fence);

    EGLDisplay Display();

public:
    virtual ~SharedSurface_ANGLEShareHandle();

    virtual void LockProdImpl() MOZ_OVERRIDE;
    virtual void UnlockProdImpl() MOZ_OVERRIDE;

    virtual void Fence() MOZ_OVERRIDE;
    virtual bool WaitSync() MOZ_OVERRIDE;
    virtual bool PollSync() MOZ_OVERRIDE;

    virtual void Fence_ContentThread_Impl() MOZ_OVERRIDE;
    virtual bool WaitSync_ContentThread_Impl() MOZ_OVERRIDE;
    virtual bool PollSync_ContentThread_Impl() MOZ_OVERRIDE;

    
    HANDLE GetShareHandle() {
        return mShareHandle;
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
                                    const SurfaceCaps& caps);

    virtual UniquePtr<SharedSurface> CreateShared(const gfx::IntSize& size) MOZ_OVERRIDE {
        bool hasAlpha = mReadCaps.alpha;
        return SharedSurface_ANGLEShareHandle::Create(mProdGL,
                                                      mContext, mConfig,
                                                      size, hasAlpha);
    }
};

} 
} 

#endif 
