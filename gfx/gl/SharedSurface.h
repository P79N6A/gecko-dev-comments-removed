













#ifndef SHARED_SURFACE_H_
#define SHARED_SURFACE_H_

#include "mozilla/StandardInteger.h"
#include "mozilla/Attributes.h"
#include "GLDefs.h"
#include "gfxPoint.h"
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
    const gfxIntSize mSize;
    const bool mHasAlpha;
    bool mIsLocked;

    SharedSurface(SharedSurfaceType type,
                  APITypeT api,
                  AttachmentType attachType,
                  const gfxIntSize& size,
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


    SharedSurfaceType Type() const {
        return mType;
    }

    APITypeT APIType() const {
        return mAPI;
    }

    AttachmentType AttachType() const {
        return mAttachType;
    }

    const gfxIntSize& Size() const {
        return mSize;
    }

    bool HasAlpha() const {
        return mHasAlpha;
    }


    
    virtual GLuint Texture() const {
        MOZ_ASSERT(AttachType() == AttachmentType::GLTexture);
        MOZ_NOT_REACHED("Did you forget to override this function?");
        return 0;
    }

    virtual GLuint Renderbuffer() const {
        MOZ_ASSERT(AttachType() == AttachmentType::GLRenderbuffer);
        MOZ_NOT_REACHED("Did you forget to override this function?");
        return 0;
    }
};

} 
} 

#endif 
