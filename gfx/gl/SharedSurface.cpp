




#include "SharedSurface.h"

#include "GLContext.h"
#include "GLBlitHelper.h"
#include "ScopedGLHelpers.h"
#include "SharedSurfaceGL.h"

namespace mozilla {
namespace gl {

 void
SharedSurface::ProdCopy(SharedSurface* src, SharedSurface* dest,
                        SurfaceFactory* factory)
{
    GLContext* gl = src->mGL;

    
    
    MOZ_ASSERT((src == gl->GetLockedSurface()) == src->IsLocked());

    gl->MakeCurrent();

    if (src->mAttachType  == AttachmentType::Screen &&
        dest->mAttachType == AttachmentType::Screen)
    {
        
        nsAutoPtr<SharedSurface_GLTexture> tempSurf;
        tempSurf = SharedSurface_GLTexture::Create(gl,
                                                   gl,
                                                   factory->mFormats,
                                                   src->mSize,
                                                   factory->mCaps.alpha);

        ProdCopy(src, tempSurf, factory);
        ProdCopy(tempSurf, dest, factory);
        return;
    }

    if (src->mAttachType == AttachmentType::Screen) {
        SharedSurface* origLocked = gl->GetLockedSurface();
        bool srcNeedsUnlock = false;
        bool origNeedsRelock = false;
        if (origLocked != src) {
            if (origLocked) {
                origLocked->UnlockProd();
                origNeedsRelock = true;
            }

            src->LockProd();
            srcNeedsUnlock = true;
        }

        if (dest->mAttachType == AttachmentType::GLTexture) {
            GLuint destTex = dest->ProdTexture();
            GLenum destTarget = dest->ProdTextureTarget();

            gl->BlitHelper()->BlitFramebufferToTexture(0, destTex,
                                                       src->mSize, dest->mSize,
                                                       destTarget, true);
        } else if (dest->mAttachType == AttachmentType::GLRenderbuffer) {
            GLuint destRB = dest->ProdRenderbuffer();
            ScopedFramebufferForRenderbuffer destWrapper(gl, destRB);

            gl->BlitHelper()->BlitFramebufferToFramebuffer(0, destWrapper.FB(),
                                                           src->mSize, dest->mSize,
                                                           true);
        } else {
            MOZ_CRASH("Unhandled dest->mAttachType.");
        }

        if (srcNeedsUnlock)
            src->UnlockProd();

        if (origNeedsRelock)
            origLocked->LockProd();

        return;
    }

    if (dest->mAttachType == AttachmentType::Screen) {
        SharedSurface* origLocked = gl->GetLockedSurface();
        bool destNeedsUnlock = false;
        bool origNeedsRelock = false;
        if (origLocked != dest) {
            if (origLocked) {
                origLocked->UnlockProd();
                origNeedsRelock = true;
            }

            dest->LockProd();
            destNeedsUnlock = true;
        }

        if (src->mAttachType == AttachmentType::GLTexture) {
            GLuint srcTex = src->ProdTexture();
            GLenum srcTarget = src->ProdTextureTarget();

            gl->BlitHelper()->BlitTextureToFramebuffer(srcTex, 0,
                                                       src->mSize, dest->mSize,
                                                       srcTarget, true);
        } else if (src->mAttachType == AttachmentType::GLRenderbuffer) {
            GLuint srcRB = src->ProdRenderbuffer();
            ScopedFramebufferForRenderbuffer srcWrapper(gl, srcRB);

            gl->BlitHelper()->BlitFramebufferToFramebuffer(srcWrapper.FB(), 0,
                                                           src->mSize, dest->mSize,
                                                           true);
        } else {
            MOZ_CRASH("Unhandled src->mAttachType.");
        }

        if (destNeedsUnlock)
            dest->UnlockProd();

        if (origNeedsRelock)
            origLocked->LockProd();

        return;
    }

    
    

    if (src->mAttachType == AttachmentType::GLTexture) {
        GLuint srcTex = src->ProdTexture();
        GLenum srcTarget = src->ProdTextureTarget();

        if (dest->mAttachType == AttachmentType::GLTexture) {
            GLuint destTex = dest->ProdTexture();
            GLenum destTarget = dest->ProdTextureTarget();

            gl->BlitHelper()->BlitTextureToTexture(srcTex, destTex,
                                                   src->mSize, dest->mSize,
                                                   srcTarget, destTarget);

            return;
        }

        if (dest->mAttachType == AttachmentType::GLRenderbuffer) {
            GLuint destRB = dest->ProdRenderbuffer();
            ScopedFramebufferForRenderbuffer destWrapper(gl, destRB);

            gl->BlitHelper()->BlitTextureToFramebuffer(srcTex, destWrapper.FB(),
                                                       src->mSize, dest->mSize, srcTarget);

            return;
        }

        MOZ_CRASH("Unhandled dest->mAttachType.");
    }

    if (src->mAttachType == AttachmentType::GLRenderbuffer) {
        GLuint srcRB = src->ProdRenderbuffer();
        ScopedFramebufferForRenderbuffer srcWrapper(gl, srcRB);

        if (dest->mAttachType == AttachmentType::GLTexture) {
            GLuint destTex = dest->ProdTexture();
            GLenum destTarget = dest->ProdTextureTarget();

            gl->BlitHelper()->BlitFramebufferToTexture(srcWrapper.FB(), destTex,
                                                       src->mSize, dest->mSize, destTarget);

            return;
        }

        if (dest->mAttachType == AttachmentType::GLRenderbuffer) {
            GLuint destRB = dest->ProdRenderbuffer();
            ScopedFramebufferForRenderbuffer destWrapper(gl, destRB);

            gl->BlitHelper()->BlitFramebufferToFramebuffer(srcWrapper.FB(), destWrapper.FB(),
                                                           src->mSize, dest->mSize);

            return;
        }

        MOZ_CRASH("Unhandled dest->mAttachType.");
    }

    MOZ_CRASH("Unhandled src->mAttachType.");
}




void
SharedSurface::LockProd()
{
    MOZ_ASSERT(!mIsLocked);

    LockProdImpl();

    mGL->LockSurface(this);
    mIsLocked = true;
}

void
SharedSurface::UnlockProd()
{
    if (!mIsLocked)
        return;

    UnlockProdImpl();

    mGL->UnlockSurface(this);
    mIsLocked = false;
}




static void
ChooseBufferBits(const SurfaceCaps& caps,
                 SurfaceCaps* const out_drawCaps,
                 SurfaceCaps* const out_readCaps)
{
    MOZ_ASSERT(out_drawCaps);
    MOZ_ASSERT(out_readCaps);

    SurfaceCaps screenCaps;

    screenCaps.color = caps.color;
    screenCaps.alpha = caps.alpha;
    screenCaps.bpp16 = caps.bpp16;

    screenCaps.depth = caps.depth;
    screenCaps.stencil = caps.stencil;

    screenCaps.antialias = caps.antialias;
    screenCaps.preserve = caps.preserve;

    if (caps.antialias) {
        *out_drawCaps = screenCaps;
        out_readCaps->Clear();

        
        out_readCaps->color = caps.color;
        out_readCaps->alpha = caps.alpha;
        out_readCaps->bpp16 = caps.bpp16;
    } else {
        out_drawCaps->Clear();
        *out_readCaps = screenCaps;
    }
}

SurfaceFactory::SurfaceFactory(GLContext* gl,
                               SharedSurfaceType type,
                               const SurfaceCaps& caps)
    : mGL(gl)
    , mCaps(caps)
    , mType(type)
    , mFormats(gl->ChooseGLFormats(caps))
{
    ChooseBufferBits(mCaps, &mDrawCaps, &mReadCaps);
}

SurfaceFactory::~SurfaceFactory()
{
    while (!mScraps.empty()) {
        SharedSurface* cur = mScraps.front();
        mScraps.pop();

        delete cur;
    }
}

SharedSurface*
SurfaceFactory::NewSharedSurface(const gfx::IntSize& size)
{
    
    while (!mScraps.empty()) {
        SharedSurface* cur = mScraps.front();
        mScraps.pop();
        if (cur->mSize == size)
            return cur;

        
        delete cur;
    }

    SharedSurface* ret = CreateShared(size);

    return ret;
}


void
SurfaceFactory::Recycle(SharedSurface*& surf)
{
    if (!surf)
        return;

    if (surf->mType == mType) {
        mScraps.push(surf);
    } else {
        delete surf;
    }

    surf = nullptr;
}

} 
} 
