













#ifndef SHARED_SURFACE_H_
#define SHARED_SURFACE_H_

#include <stdint.h>
#include "mozilla/Attributes.h"
#include "GLDefs.h"
#include "mozilla/gfx/Point.h"
#include "SurfaceTypes.h"

namespace mozilla {
namespace gfx {

class SurfaceFactory;

class SharedSurface
{
protected:
    const SharedSurfaceType mType;
    const APITypeT mAPI;
    const AttachmentType mAttachType;
    const gfx::IntSize mSize;
    const bool mHasAlpha;
    bool mIsLocked;

    SharedSurface(SharedSurfaceType type,
                  APITypeT api,
                  AttachmentType attachType,
                  const gfx::IntSize& size,
                  bool hasAlpha)
        : mType(type)
        , mAPI(api)
        , mAttachType(attachType)
        , mSize(size)
        , mHasAlpha(hasAlpha)
        , mIsLocked(false)
    {
    }

public:
    virtual ~SharedSurface() {
    }

    static void Copy(SharedSurface* src, SharedSurface* dest,
                     SurfaceFactory* factory);

    
    
    virtual void LockProd() {
        MOZ_ASSERT(!mIsLocked);
        LockProdImpl();
        mIsLocked = true;
    }

    
    virtual void UnlockProd() {
        if (!mIsLocked)
            return;

        UnlockProdImpl();
        mIsLocked = false;
    }

    virtual void LockProdImpl() = 0;
    virtual void UnlockProdImpl() = 0;

    virtual void Fence() = 0;
    virtual bool WaitSync() = 0;

    
    
    
    virtual void WaitForBufferOwnership() {}

    SharedSurfaceType Type() const {
        return mType;
    }

    APITypeT APIType() const {
        return mAPI;
    }

    AttachmentType AttachType() const {
        return mAttachType;
    }

    const gfx::IntSize& Size() const {
        return mSize;
    }

    bool HasAlpha() const {
        return mHasAlpha;
    }
};

} 
} 

#endif 
