




#ifndef MOZILLA_GFX_COMPOSITINGRENDERTARGETOGL_H
#define MOZILLA_GFX_COMPOSITINGRENDERTARGETOGL_H

#include "GLContextTypes.h"             
#include "GLDefs.h"                     
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


namespace mozilla {
namespace gl {
  class BindableTexture;
}
namespace gfx {
  class DataSourceSurface;
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
  CompositingRenderTargetOGL(CompositorOGL* aCompositor, const gfx::IntPoint& aOrigin,
                             GLuint aTexure, GLuint aFBO)
    : CompositingRenderTarget(aOrigin)
    , mInitParams()
    , mCompositor(aCompositor)
    , mGL(aCompositor->gl())
    , mTextureHandle(aTexure)
    , mFBO(aFBO)
  {}

  ~CompositingRenderTargetOGL();

  



  static TemporaryRef<CompositingRenderTargetOGL>
  RenderTargetForWindow(CompositorOGL* aCompositor,
                        const gfx::IntSize& aSize)
  {
    RefPtr<CompositingRenderTargetOGL> result
      = new CompositingRenderTargetOGL(aCompositor, gfx::IntPoint(0, 0), 0, 0);
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

  void BindTexture(GLenum aTextureUnit, GLenum aTextureTarget);

  


  void BindRenderTarget();

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
    return mInitParams.mSize;
  }

  gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE
  {
    
    MOZ_ASSERT(false, "Not implemented");
    return gfx::SurfaceFormat::UNKNOWN;
  }

#ifdef MOZ_DUMP_PAINTING
  virtual TemporaryRef<gfx::DataSourceSurface> Dump(Compositor* aCompositor);
#endif

private:
  



  void InitializeImpl();

  InitParams mInitParams;
  CompositorOGL* mCompositor;
  GLContext* mGL;
  GLuint mTextureHandle;
  GLuint mFBO;
};

}
}

#endif 
