




#ifndef MOZILLA_GFX_COMPOSITINGRENDERTARGETOGL_H
#define MOZILLA_GFX_COMPOSITINGRENDERTARGETOGL_H

#include "mozilla-config.h"             
#include "GLContext.h"                  
#include "GLContextTypes.h"             
#include "GLDefs.h"                     
#include "gfxMatrix.h"                  
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/TextureHost.h" 
#include "mozilla/layers/CompositorOGL.h"  
#include "mozilla/mozalloc.h"           
#include "nsAString.h"
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsString.h"                   

class gfxImageSurface;

namespace mozilla {
namespace gl {
  class BindableTexture;
}

namespace layers {

class TextureSource;

class CompositingRenderTargetOGL : public CompositingRenderTarget
{
  typedef mozilla::gl::GLContext GLContext;

  
  struct InitParams
  {
    InitParams() : mStatus(NO_PARAMS) {}
    InitParams(const gfx::IntSize& aSize,
               GLenum aFBOTextureTarget,
               SurfaceInitMode aInit)
      : mStatus(READY)
      , mSize(aSize)
      , mFBOTextureTarget(aFBOTextureTarget)
      , mInit(aInit)
    {}

    enum {
      NO_PARAMS,
      READY,
      INITIALIZED
    } mStatus;
    gfx::IntSize mSize;
    GLenum mFBOTextureTarget;
    SurfaceInitMode mInit;
  };

public:
  CompositingRenderTargetOGL(CompositorOGL* aCompositor, GLuint aTexure, GLuint aFBO)
    : mInitParams()
    , mTransform()
    , mCompositor(aCompositor)
    , mGL(aCompositor->gl())
    , mTextureHandle(aTexure)
    , mFBO(aFBO)
  {}

  ~CompositingRenderTargetOGL()
  {
    mGL->fDeleteTextures(1, &mTextureHandle);
    mGL->fDeleteFramebuffers(1, &mFBO);
  }

  



  static TemporaryRef<CompositingRenderTargetOGL>
  RenderTargetForWindow(CompositorOGL* aCompositor,
                        const gfx::IntSize& aSize,
                        const gfxMatrix& aTransform)
  {
    RefPtr<CompositingRenderTargetOGL> result
      = new CompositingRenderTargetOGL(aCompositor, 0, 0);
    result->mTransform = aTransform;
    result->mInitParams = InitParams(aSize, 0, INIT_MODE_NONE);
    result->mInitParams.mStatus = InitParams::INITIALIZED;
    return result.forget();
  }

  





  void Initialize(const gfx::IntSize& aSize,
                  GLenum aFBOTextureTarget,
                  SurfaceInitMode aInit)
  {
    MOZ_ASSERT(mInitParams.mStatus == InitParams::NO_PARAMS, "Initialized twice?");
    
    mInitParams = InitParams(aSize, aFBOTextureTarget, aInit);
  }

  void BindTexture(GLenum aTextureUnit, GLenum aTextureTarget)
  {
    MOZ_ASSERT(mInitParams.mStatus == InitParams::INITIALIZED);
    MOZ_ASSERT(mTextureHandle != 0);
    mGL->fActiveTexture(aTextureUnit);
    mGL->fBindTexture(aTextureTarget, mTextureHandle);
  }

  


  void BindRenderTarget()
  {
    if (mInitParams.mStatus != InitParams::INITIALIZED) {
      InitializeImpl();
    } else {
      MOZ_ASSERT(mInitParams.mStatus == InitParams::INITIALIZED);
      mGL->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, mFBO);
      GLenum result = mGL->fCheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER);
      if (result != LOCAL_GL_FRAMEBUFFER_COMPLETE) {
        nsAutoCString msg;
        msg.AppendPrintf("Framebuffer not complete -- error 0x%x, aFBOTextureTarget 0x%x, aRect.width %d, aRect.height %d",
                         result, mInitParams.mFBOTextureTarget, mInitParams.mSize.width, mInitParams.mSize.height);
        NS_WARNING(msg.get());
      }

      mCompositor->PrepareViewport(mInitParams.mSize, mTransform);
    }
  }

  GLuint GetFBO() const
  {
    MOZ_ASSERT(mInitParams.mStatus == InitParams::INITIALIZED);
    return mFBO;
  }

  GLuint GetTextureHandle() const
  {
    MOZ_ASSERT(mInitParams.mStatus == InitParams::INITIALIZED);
    return mTextureHandle;
  }

  
  TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE
  {
    
    MOZ_ASSERT(false, "CompositingRenderTargetOGL should not be used as a TextureSource");
    return nullptr;
  }
  gfx::IntSize GetSize() const MOZ_OVERRIDE
  {
    
    MOZ_ASSERT(false, "CompositingRenderTargetOGL should not be used as a TextureSource");
    return gfx::IntSize(0, 0);
  }

  gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE
  {
    
    MOZ_ASSERT(false, "Not implemented");
    return gfx::FORMAT_UNKNOWN;
  }

  const gfxMatrix& GetTransform() {
    return mTransform;
  }

#ifdef MOZ_DUMP_PAINTING
  virtual already_AddRefed<gfxImageSurface> Dump(Compositor* aCompositor)
  {
    MOZ_ASSERT(mInitParams.mStatus == InitParams::INITIALIZED);
    CompositorOGL* compositorOGL = static_cast<CompositorOGL*>(aCompositor);
    return mGL->GetTexImage(mTextureHandle, true, compositorOGL->GetFBOFormat());
  }
#endif

private:
  



  void InitializeImpl()
  {
    MOZ_ASSERT(mInitParams.mStatus == InitParams::READY);

    mGL->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, mFBO);
    mGL->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER,
                               LOCAL_GL_COLOR_ATTACHMENT0,
                               mInitParams.mFBOTextureTarget,
                               mTextureHandle,
                               0);

    
    
    GLenum result = mGL->fCheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER);
    if (result != LOCAL_GL_FRAMEBUFFER_COMPLETE) {
      nsAutoCString msg;
      msg.AppendPrintf("Framebuffer not complete -- error 0x%x, aFBOTextureTarget 0x%x, mFBO %d, mTextureHandle %d, aRect.width %d, aRect.height %d",
                       result, mInitParams.mFBOTextureTarget, mFBO, mTextureHandle, mInitParams.mSize.width, mInitParams.mSize.height);
      NS_ERROR(msg.get());
    }

    mCompositor->PrepareViewport(mInitParams.mSize, mTransform);
    mGL->fScissor(0, 0, mInitParams.mSize.width, mInitParams.mSize.height);
    if (mInitParams.mInit == INIT_MODE_CLEAR) {
      mGL->fClearColor(0.0, 0.0, 0.0, 0.0);
      mGL->fClear(LOCAL_GL_COLOR_BUFFER_BIT);
    }

    mInitParams.mStatus = InitParams::INITIALIZED;
  }

  InitParams mInitParams;
  gfxMatrix mTransform;
  CompositorOGL* mCompositor;
  GLContext* mGL;
  GLuint mTextureHandle;
  GLuint mFBO;
};

}
}

#endif 
