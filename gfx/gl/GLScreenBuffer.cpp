




#include "GLScreenBuffer.h"

#include <cstring>
#include "GLContext.h"
#include "GLBlitHelper.h"
#include "GLReadTexImageHelper.h"
#include "SharedSurfaceGL.h"
#include "SurfaceStream.h"
#ifdef MOZ_WIDGET_GONK
#include "SharedSurfaceGralloc.h"
#include "nsXULAppAPI.h"
#endif
#ifdef XP_MACOSX
#include "SharedSurfaceIO.h"
#endif
#include "ScopedGLHelpers.h"
#include "gfx2DGlue.h"
#include "../layers/ipc/ShadowLayers.h"

namespace mozilla {
namespace gl {

using gfx::SurfaceFormat;

UniquePtr<GLScreenBuffer>
GLScreenBuffer::Create(GLContext* gl,
                       const gfx::IntSize& size,
                       const SurfaceCaps& caps)
{
    UniquePtr<GLScreenBuffer> ret;
    if (caps.antialias &&
        !gl->IsSupported(GLFeature::framebuffer_multisample))
    {
        return Move(ret);
    }

    UniquePtr<SurfaceFactory> factory;

#ifdef MOZ_WIDGET_GONK
    
    if (!factory &&
        caps.surfaceAllocator &&
        XRE_GetProcessType() != GeckoProcessType_Default)
    {
        factory = MakeUnique<SurfaceFactory_Gralloc>(gl, caps);
    }
#endif
#ifdef XP_MACOSX
    
    if (!factory) {
        factory = SurfaceFactory_IOSurface::Create(gl, caps);
    }
#endif

    if (!factory) {
        factory = MakeUnique<SurfaceFactory_Basic>(gl, caps);
    }

    auto streamType = SurfaceStream::ChooseGLStreamType(SurfaceStream::MainThread,
                                                        caps.preserve);
    RefPtr<SurfaceStream> stream;
    stream = SurfaceStream::CreateForType(streamType, gl, nullptr);

    ret.reset( new GLScreenBuffer(gl, caps, Move(factory), stream) );
    return Move(ret);
}

GLScreenBuffer::~GLScreenBuffer()
{
    mDraw = nullptr;
    mRead = nullptr;

    
    
    
    
    
    
    mFactory = nullptr;
}


void
GLScreenBuffer::BindAsFramebuffer(GLContext* const gl, GLenum target) const
{
    GLuint drawFB = DrawFB();
    GLuint readFB = ReadFB();

    if (!gl->IsSupported(GLFeature::framebuffer_blit)) {
        MOZ_ASSERT(drawFB == readFB);
        gl->raw_fBindFramebuffer(target, readFB);
        return;
    }

    switch (target) {
    case LOCAL_GL_FRAMEBUFFER:
        gl->raw_fBindFramebuffer(LOCAL_GL_DRAW_FRAMEBUFFER_EXT, drawFB);
        gl->raw_fBindFramebuffer(LOCAL_GL_READ_FRAMEBUFFER_EXT, readFB);
        break;

    case LOCAL_GL_DRAW_FRAMEBUFFER_EXT:
        if (!gl->IsSupported(GLFeature::framebuffer_blit))
            NS_WARNING("DRAW_FRAMEBUFFER requested but unavailable.");

        gl->raw_fBindFramebuffer(LOCAL_GL_DRAW_FRAMEBUFFER_EXT, drawFB);
        break;

    case LOCAL_GL_READ_FRAMEBUFFER_EXT:
        if (!gl->IsSupported(GLFeature::framebuffer_blit))
            NS_WARNING("READ_FRAMEBUFFER requested but unavailable.");

        gl->raw_fBindFramebuffer(LOCAL_GL_READ_FRAMEBUFFER_EXT, readFB);
        break;

    default:
        MOZ_CRASH("Bad `target` for BindFramebuffer.");
    }
}

void
GLScreenBuffer::BindFB(GLuint fb)
{
    GLuint drawFB = DrawFB();
    GLuint readFB = ReadFB();

    mUserDrawFB = fb;
    mUserReadFB = fb;
    mInternalDrawFB = (fb == 0) ? drawFB : fb;
    mInternalReadFB = (fb == 0) ? readFB : fb;

    if (mInternalDrawFB == mInternalReadFB) {
        mGL->raw_fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, mInternalDrawFB);
    } else {
        MOZ_ASSERT(mGL->IsSupported(GLFeature::framebuffer_blit));
        mGL->raw_fBindFramebuffer(LOCAL_GL_DRAW_FRAMEBUFFER_EXT, mInternalDrawFB);
        mGL->raw_fBindFramebuffer(LOCAL_GL_READ_FRAMEBUFFER_EXT, mInternalReadFB);
    }

#ifdef DEBUG
    mInInternalMode_DrawFB = false;
    mInInternalMode_ReadFB = false;
#endif
}

void
GLScreenBuffer::BindDrawFB(GLuint fb)
{
    MOZ_ASSERT(mGL->IsSupported(GLFeature::framebuffer_blit));

    GLuint drawFB = DrawFB();
    mUserDrawFB = fb;
    mInternalDrawFB = (fb == 0) ? drawFB : fb;

    mGL->raw_fBindFramebuffer(LOCAL_GL_DRAW_FRAMEBUFFER_EXT, mInternalDrawFB);

#ifdef DEBUG
    mInInternalMode_DrawFB = false;
#endif
}

void
GLScreenBuffer::BindReadFB(GLuint fb)
{
    MOZ_ASSERT(mGL->IsSupported(GLFeature::framebuffer_blit));

    GLuint readFB = ReadFB();
    mUserReadFB = fb;
    mInternalReadFB = (fb == 0) ? readFB : fb;

    mGL->raw_fBindFramebuffer(LOCAL_GL_READ_FRAMEBUFFER_EXT, mInternalReadFB);

#ifdef DEBUG
    mInInternalMode_ReadFB = false;
#endif
}

void
GLScreenBuffer::BindFB_Internal(GLuint fb)
{
    mInternalDrawFB = mUserDrawFB = fb;
    mInternalReadFB = mUserReadFB = fb;
    mGL->raw_fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, mInternalDrawFB);

#ifdef DEBUG
    mInInternalMode_DrawFB = true;
    mInInternalMode_ReadFB = true;
#endif
}

void
GLScreenBuffer::BindDrawFB_Internal(GLuint fb)
{
    MOZ_ASSERT(mGL->IsSupported(GLFeature::framebuffer_blit));

    mInternalDrawFB = mUserDrawFB = fb;
    mGL->raw_fBindFramebuffer(LOCAL_GL_DRAW_FRAMEBUFFER_EXT, mInternalDrawFB);

#ifdef DEBUG
    mInInternalMode_DrawFB = true;
#endif
}

void
GLScreenBuffer::BindReadFB_Internal(GLuint fb)
{
    MOZ_ASSERT(mGL->IsSupported(GLFeature::framebuffer_blit));

    mInternalReadFB = mUserReadFB = fb;
    mGL->raw_fBindFramebuffer(LOCAL_GL_READ_FRAMEBUFFER_EXT, mInternalReadFB);

#ifdef DEBUG
    mInInternalMode_ReadFB = true;
#endif
}


GLuint
GLScreenBuffer::GetDrawFB() const
{
#ifdef DEBUG
    MOZ_ASSERT(mGL->IsCurrent());
    MOZ_ASSERT(!mInInternalMode_DrawFB);

    
    
    
    
    GLuint actual = 0;
    mGL->raw_fGetIntegerv(LOCAL_GL_DRAW_FRAMEBUFFER_BINDING_EXT, (GLint*)&actual);

    GLuint predicted = mInternalDrawFB;
    if (predicted != actual) {
        printf_stderr("Misprediction: Bound draw FB predicted: %d. Was: %d.\n",
                      predicted, actual);
        MOZ_ASSERT(false, "Draw FB binding misprediction!");
    }
#endif

    return mUserDrawFB;
}

GLuint
GLScreenBuffer::GetReadFB() const
{
#ifdef DEBUG
    MOZ_ASSERT(mGL->IsCurrent());
    MOZ_ASSERT(!mInInternalMode_ReadFB);

    
    
    GLuint actual = 0;
    if (mGL->IsSupported(GLFeature::framebuffer_blit))
        mGL->raw_fGetIntegerv(LOCAL_GL_READ_FRAMEBUFFER_BINDING_EXT, (GLint*)&actual);
    else
        mGL->raw_fGetIntegerv(LOCAL_GL_FRAMEBUFFER_BINDING, (GLint*)&actual);

    GLuint predicted = mInternalReadFB;
    if (predicted != actual) {
        printf_stderr("Misprediction: Bound read FB predicted: %d. Was: %d.\n",
                      predicted, actual);
        MOZ_ASSERT(false, "Read FB binding misprediction!");
    }
#endif

    return mUserReadFB;
}

GLuint
GLScreenBuffer::GetFB() const
{
    MOZ_ASSERT(GetDrawFB() == GetReadFB());
    return GetDrawFB();
}


void
GLScreenBuffer::DeletingFB(GLuint fb)
{
    if (fb == mInternalDrawFB) {
        mInternalDrawFB = 0;
        mUserDrawFB = 0;
    }
    if (fb == mInternalReadFB) {
        mInternalReadFB = 0;
        mUserReadFB = 0;
    }
}


void
GLScreenBuffer::AfterDrawCall()
{
    if (mUserDrawFB != 0)
        return;

    RequireBlit();
}

void
GLScreenBuffer::BeforeReadCall()
{
    if (mUserReadFB != 0)
        return;

    AssureBlitted();
}

bool
GLScreenBuffer::ReadPixels(GLint x, GLint y,
                           GLsizei width, GLsizei height,
                           GLenum format, GLenum type,
                           GLvoid* pixels)
{
    
    
    
    
    
    SharedSurface* surf;
    if (GetReadFB() == 0) {
        surf = SharedSurf();
    } else {
        surf = mGL->mFBOMapping[GetReadFB()];
    }
    if (surf) {
        return surf->ReadPixels(x, y, width, height, format, type, pixels);
    }

    return false;
}

void
GLScreenBuffer::RequireBlit()
{
    mNeedsBlit = true;
}

void
GLScreenBuffer::AssureBlitted()
{
    if (!mNeedsBlit)
        return;

    if (mDraw) {
        GLuint drawFB = DrawFB();
        GLuint readFB = ReadFB();

        MOZ_ASSERT(drawFB != 0);
        MOZ_ASSERT(drawFB != readFB);
        MOZ_ASSERT(mGL->IsSupported(GLFeature::framebuffer_blit));
        MOZ_ASSERT(mDraw->mSize == mRead->Size());

        ScopedBindFramebuffer boundFB(mGL);
        ScopedGLState scissor(mGL, LOCAL_GL_SCISSOR_TEST, false);

        BindReadFB_Internal(drawFB);
        BindDrawFB_Internal(readFB);

        const gfx::IntSize&  srcSize = mDraw->mSize;
        const gfx::IntSize& destSize = mRead->Size();

        mGL->raw_fBlitFramebuffer(0, 0,  srcSize.width,  srcSize.height,
                                  0, 0, destSize.width, destSize.height,
                                  LOCAL_GL_COLOR_BUFFER_BIT,
                                  LOCAL_GL_NEAREST);
        
    }

    mNeedsBlit = false;
}

void
GLScreenBuffer::Morph(UniquePtr<SurfaceFactory> newFactory,
                      SurfaceStreamType streamType)
{
    MOZ_ASSERT(mStream);

    if (newFactory) {
        mFactory = Move(newFactory);
    }

    if (mStream->mType == streamType)
        return;

    mStream = SurfaceStream::CreateForType(streamType, mGL, mStream);
    MOZ_ASSERT(mStream);
}

bool
GLScreenBuffer::Attach(SharedSurface* surf, const gfx::IntSize& size)
{
    ScopedBindFramebuffer autoFB(mGL);

    if (mRead && SharedSurf())
        SharedSurf()->UnlockProd();

    surf->LockProd();

    if (mRead &&
        surf->mAttachType == SharedSurf()->mAttachType &&
        size == Size())
    {
        
        mRead->Attach(surf);
    } else {
        
        UniquePtr<DrawBuffer> draw;
        bool drawOk = CreateDraw(size, &draw);  

        UniquePtr<ReadBuffer> read = CreateRead(surf);
        bool readOk = !!read;

        if (!drawOk || !readOk) {
            surf->UnlockProd();

            return false;
        }

        mDraw = Move(draw);
        mRead = Move(read);
    }

    
    MOZ_ASSERT(SharedSurf() == surf);

    if (!PreserveBuffer()) {
        
    }

    return true;
}

bool
GLScreenBuffer::Swap(const gfx::IntSize& size)
{
    SharedSurface* nextSurf = mStream->SwapProducer(mFactory.get(), size);
    if (!nextSurf) {
        SurfaceFactory_Basic basicFactory(mGL, mFactory->mCaps);
        nextSurf = mStream->SwapProducer(&basicFactory, size);
        if (!nextSurf)
          return false;

        NS_WARNING("SwapProd failed for sophisticated Factory type, fell back to Basic.");
    }
    MOZ_ASSERT(nextSurf);

    return Attach(nextSurf, size);
}

bool
GLScreenBuffer::PublishFrame(const gfx::IntSize& size)
{
    AssureBlitted();

    bool good = Swap(size);
    return good;
}

bool
GLScreenBuffer::Resize(const gfx::IntSize& size)
{
    SharedSurface* surf = mStream->Resize(mFactory.get(), size);
    if (!surf)
        return false;

    return Attach(surf, size);
}

bool
GLScreenBuffer::CreateDraw(const gfx::IntSize& size,
                           UniquePtr<DrawBuffer>* out_buffer)
{
    GLContext* gl = mFactory->mGL;
    const GLFormats& formats = mFactory->mFormats;
    const SurfaceCaps& caps = mFactory->DrawCaps();

    return DrawBuffer::Create(gl, caps, formats, size, out_buffer);
}

UniquePtr<ReadBuffer>
GLScreenBuffer::CreateRead(SharedSurface* surf)
{
    GLContext* gl = mFactory->mGL;
    const GLFormats& formats = mFactory->mFormats;
    const SurfaceCaps& caps = mFactory->ReadCaps();

    return ReadBuffer::Create(gl, caps, formats, surf);
}

void
GLScreenBuffer::Readback(SharedSurface* src, gfx::DataSourceSurface* dest)
{
  MOZ_ASSERT(src && dest);
  MOZ_ASSERT(dest->GetSize() == src->mSize);
  MOZ_ASSERT(dest->GetFormat() == (src->mHasAlpha ? SurfaceFormat::B8G8R8A8
                                                  : SurfaceFormat::B8G8R8X8));

  mGL->MakeCurrent();

  bool needsSwap = src != SharedSurf();
  if (needsSwap) {
      SharedSurf()->UnlockProd();
      src->LockProd();
  }


  {
      UniquePtr<ReadBuffer> buffer = CreateRead(src);
      MOZ_ASSERT(buffer);

      ScopedBindFramebuffer autoFB(mGL, buffer->mFB);
      ReadPixelsIntoDataSurface(mGL, dest);
  }

  if (needsSwap) {
      src->UnlockProd();
      SharedSurf()->LockProd();
  }
}




bool
DrawBuffer::Create(GLContext* const gl,
                   const SurfaceCaps& caps,
                   const GLFormats& formats,
                   const gfx::IntSize& size,
                   UniquePtr<DrawBuffer>* out_buffer)
{
    MOZ_ASSERT(out_buffer);
    *out_buffer = nullptr;

    if (!caps.color) {
        MOZ_ASSERT(!caps.alpha && !caps.depth && !caps.stencil);

        
        return true;
    }

    GLuint colorMSRB = 0;
    GLuint depthRB   = 0;
    GLuint stencilRB = 0;

    GLuint* pColorMSRB = caps.antialias ? &colorMSRB : nullptr;
    GLuint* pDepthRB   = caps.depth     ? &depthRB   : nullptr;
    GLuint* pStencilRB = caps.stencil   ? &stencilRB : nullptr;

    if (!formats.color_rbFormat)
        pColorMSRB = nullptr;

    if (pDepthRB && pStencilRB) {
        if (!formats.depth && !formats.depthStencil)
            pDepthRB = nullptr;

        if (!formats.stencil && !formats.depthStencil)
            pStencilRB = nullptr;
    } else {
        if (!formats.depth)
            pDepthRB = nullptr;

        if (!formats.stencil)
            pStencilRB = nullptr;
    }

    CreateRenderbuffersForOffscreen(gl, formats, size, caps.antialias,
                                    pColorMSRB, pDepthRB, pStencilRB);

    GLuint fb = 0;
    gl->fGenFramebuffers(1, &fb);
    gl->AttachBuffersToFB(0, colorMSRB, depthRB, stencilRB, fb);

    UniquePtr<DrawBuffer> ret( new DrawBuffer(gl, size, fb, colorMSRB,
                                              depthRB, stencilRB) );

    if (!gl->IsFramebufferComplete(fb))
        return false;

    *out_buffer = Move(ret);
    return true;
}

DrawBuffer::~DrawBuffer()
{
    mGL->MakeCurrent();

    GLuint fb = mFB;
    GLuint rbs[] = {
        mColorMSRB,
        mDepthRB,
        mStencilRB
    };

    mGL->fDeleteFramebuffers(1, &fb);
    mGL->fDeleteRenderbuffers(3, rbs);
}




UniquePtr<ReadBuffer>
ReadBuffer::Create(GLContext* gl,
                   const SurfaceCaps& caps,
                   const GLFormats& formats,
                   SharedSurface* surf)
{
    MOZ_ASSERT(surf);

    if (surf->mAttachType == AttachmentType::Screen) {
        

        return UniquePtr<ReadBuffer>( new ReadBuffer(gl, 0, 0, 0,
                                                     surf) );
    }

    GLuint depthRB = 0;
    GLuint stencilRB = 0;

    GLuint* pDepthRB   = caps.depth   ? &depthRB   : nullptr;
    GLuint* pStencilRB = caps.stencil ? &stencilRB : nullptr;

    CreateRenderbuffersForOffscreen(gl, formats, surf->mSize, caps.antialias,
                                    nullptr, pDepthRB, pStencilRB);

    GLuint colorTex = 0;
    GLuint colorRB = 0;
    GLenum target = 0;

    switch (surf->mAttachType) {
    case AttachmentType::GLTexture:
        colorTex = surf->ProdTexture();
        target = surf->ProdTextureTarget();
        break;
    case AttachmentType::GLRenderbuffer:
        colorRB = surf->ProdRenderbuffer();
        break;
    default:
        MOZ_CRASH("Unknown attachment type?");
    }
    MOZ_ASSERT(colorTex || colorRB);

    GLuint fb = 0;
    gl->fGenFramebuffers(1, &fb);
    gl->AttachBuffersToFB(colorTex, colorRB, depthRB, stencilRB, fb, target);
    gl->mFBOMapping[fb] = surf;

    UniquePtr<ReadBuffer> ret( new ReadBuffer(gl, fb, depthRB,
                                              stencilRB, surf) );
    if (!gl->IsFramebufferComplete(fb)) {
      ret = nullptr;
    }

    return Move(ret);
}

ReadBuffer::~ReadBuffer()
{
    mGL->MakeCurrent();

    GLuint fb = mFB;
    GLuint rbs[] = {
        mDepthRB,
        mStencilRB
    };

    mGL->fDeleteFramebuffers(1, &fb);
    mGL->fDeleteRenderbuffers(2, rbs);
    mGL->mFBOMapping.erase(mFB);
}

void
ReadBuffer::Attach(SharedSurface* surf)
{
    MOZ_ASSERT(surf && mSurf);
    MOZ_ASSERT(surf->mAttachType == mSurf->mAttachType);
    MOZ_ASSERT(surf->mSize == mSurf->mSize);

    
    if (surf->mAttachType != AttachmentType::Screen) {
        GLuint colorTex = 0;
        GLuint colorRB = 0;
        GLenum target = 0;

        switch (surf->mAttachType) {
        case AttachmentType::GLTexture:
            colorTex = surf->ProdTexture();
            target = surf->ProdTextureTarget();
            break;
        case AttachmentType::GLRenderbuffer:
            colorRB = surf->ProdRenderbuffer();
            break;
        default:
            MOZ_CRASH("Unknown attachment type?");
        }

        mGL->AttachBuffersToFB(colorTex, colorRB, 0, 0, mFB, target);
        mGL->mFBOMapping[mFB] = surf;
        MOZ_ASSERT(mGL->IsFramebufferComplete(mFB));
    }

    mSurf = surf;
}

const gfx::IntSize&
ReadBuffer::Size() const
{
    return mSurf->mSize;
}

} 
} 
