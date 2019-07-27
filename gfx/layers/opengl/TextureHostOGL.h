




#ifndef MOZILLA_GFX_TEXTUREOGL_H
#define MOZILLA_GFX_TEXTUREOGL_H

#include <stddef.h>                     
#include <stdint.h>                     
#include "CompositableHost.h"
#include "GLContextTypes.h"             
#include "GLDefs.h"                     
#include "GLTextureImage.h"             
#include "gfxTypes.h"
#include "mozilla/GfxMessageUtils.h"    
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorOGL.h"  
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/TextureHost.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "OGLShaderProgram.h"           
#ifdef MOZ_WIDGET_GONK
#include <ui/GraphicBuffer.h>
#if ANDROID_VERSION >= 17
#include <ui/Fence.h>
#endif
#endif

class gfxReusableSurfaceWrapper;
class nsIntRegion;

namespace mozilla {
namespace gfx {
class DataSourceSurface;
}

namespace gl {
class AndroidSurfaceTexture;
}

namespace layers {

class Compositor;
class CompositorOGL;
class TextureImageTextureSourceOGL;
class TextureSharedDataGonkOGL;
class GLTextureSource;

inline void ApplyFilterToBoundTexture(gl::GLContext* aGL,
                                      gfx::Filter aFilter,
                                      GLuint aTarget = LOCAL_GL_TEXTURE_2D)
{
  GLenum filter =
    (aFilter == gfx::Filter::POINT ? LOCAL_GL_NEAREST : LOCAL_GL_LINEAR);

  aGL->fTexParameteri(aTarget, LOCAL_GL_TEXTURE_MIN_FILTER, filter);
  aGL->fTexParameteri(aTarget, LOCAL_GL_TEXTURE_MAG_FILTER, filter);
}




















class TextureSourceOGL
{
public:
  TextureSourceOGL()
    : mHasCachedFilter(false)
  {}

  virtual bool IsValid() const = 0;

  virtual void BindTexture(GLenum aTextureUnit, gfx::Filter aFilter) = 0;

  virtual gfx::IntSize GetSize() const = 0;

  virtual GLenum GetTextureTarget() const { return LOCAL_GL_TEXTURE_2D; }

  virtual gfx::SurfaceFormat GetFormat() const = 0;

  virtual GLenum GetWrapMode() const = 0;

  virtual gfx::Matrix4x4 GetTextureTransform() { return gfx::Matrix4x4(); }

  virtual TextureImageTextureSourceOGL* AsTextureImageTextureSource() { return nullptr; }

  virtual GLTextureSource* AsGLTextureSource() { return nullptr; }

  void SetFilter(gl::GLContext* aGL, gfx::Filter aFilter)
  {
    if (mHasCachedFilter &&
        mCachedFilter == aFilter) {
      return;
    }
    mHasCachedFilter = true;
    mCachedFilter = aFilter;
    ApplyFilterToBoundTexture(aGL, aFilter, GetTextureTarget());
  }

  void ClearCachedFilter() { mHasCachedFilter = false; }

private:
  gfx::Filter mCachedFilter;
  bool mHasCachedFilter;
};




class TextureHostOGL
{
public:
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17

  



  virtual bool SetReleaseFence(const android::sp<android::Fence>& aReleaseFence);

  


  virtual android::sp<android::Fence> GetAndResetReleaseFence();

  virtual void SetAcquireFence(const android::sp<android::Fence>& aAcquireFence);

  


  virtual android::sp<android::Fence> GetAndResetAcquireFence();

  virtual void WaitAcquireFenceSyncComplete();

protected:
  android::sp<android::Fence> mReleaseFence;

  android::sp<android::Fence> mAcquireFence;

  




  android::sp<android::Fence> mPrevReleaseFence;
#endif
};










class TextureImageTextureSourceOGL final : public DataTextureSource
                                         , public TextureSourceOGL
                                         , public BigImageIterator
{
public:
  explicit TextureImageTextureSourceOGL(CompositorOGL *aCompositor,
                                        TextureFlags aFlags = TextureFlags::DEFAULT)
    : mCompositor(aCompositor)
    , mFlags(aFlags)
    , mIterating(false)
  {}

  

  virtual bool Update(gfx::DataSourceSurface* aSurface,
                      nsIntRegion* aDestRegion = nullptr,
                      gfx::IntPoint* aSrcOffset = nullptr) override;

  void EnsureBuffer(const gfx::IntSize& aSize,
                    gfxContentType aContentType);

  void CopyTo(const gfx::IntRect& aSourceRect,
              DataTextureSource* aDest,
              const gfx::IntRect& aDestRect);

  virtual TextureImageTextureSourceOGL* AsTextureImageTextureSource() override { return this; }

  

  virtual void DeallocateDeviceData() override
  {
    mTexImage = nullptr;
    SetUpdateSerial(0);
  }

  virtual TextureSourceOGL* AsSourceOGL() override { return this; }

  virtual void BindTexture(GLenum aTextureUnit, gfx::Filter aFilter) override;

  virtual gfx::IntSize GetSize() const override;

  virtual gfx::SurfaceFormat GetFormat() const override;

  virtual bool IsValid() const override { return !!mTexImage; }

  virtual void SetCompositor(Compositor* aCompositor) override;

  virtual GLenum GetWrapMode() const override
  {
    return mTexImage->GetWrapMode();
  }

  

  virtual BigImageIterator* AsBigImageIterator() override { return this; }

  virtual void BeginBigImageIteration() override
  {
    mTexImage->BeginBigImageIteration();
    mIterating = true;
  }

  virtual void EndBigImageIteration() override
  {
    mIterating = false;
  }

  virtual gfx::IntRect GetTileRect() override;

  virtual size_t GetTileCount() override
  {
    return mTexImage->GetTileCount();
  }

  virtual bool NextTile() override
  {
    return mTexImage->NextTile();
  }

protected:
  nsRefPtr<gl::TextureImage> mTexImage;
  RefPtr<CompositorOGL> mCompositor;
  TextureFlags mFlags;
  bool mIterating;
};









class GLTextureSource : public TextureSource
                      , public TextureSourceOGL
{
public:
  GLTextureSource(CompositorOGL* aCompositor,
                  GLuint aTextureHandle,
                  GLenum aTarget,
                  gfx::IntSize aSize,
                  gfx::SurfaceFormat aFormat,
                  bool aExternallyOwned = false);

  ~GLTextureSource();

  virtual GLTextureSource* AsGLTextureSource() override { return this; }

  virtual TextureSourceOGL* AsSourceOGL() override { return this; }

  virtual void BindTexture(GLenum activetex, gfx::Filter aFilter) override;

  virtual bool IsValid() const override;

  virtual gfx::IntSize GetSize() const override { return mSize; }

  virtual gfx::SurfaceFormat GetFormat() const override { return mFormat; }

  virtual GLenum GetTextureTarget() const override { return mTextureTarget; }

  virtual GLenum GetWrapMode() const override { return LOCAL_GL_CLAMP_TO_EDGE; }

  virtual void DeallocateDeviceData() override;

  virtual void SetCompositor(Compositor* aCompositor) override;

  void SetSize(gfx::IntSize aSize) { mSize = aSize; }

  void SetFormat(gfx::SurfaceFormat aFormat) { mFormat = aFormat; }

  GLuint GetTextureHandle() const { return mTextureHandle; }

  gl::GLContext* gl() const;

protected:
  void DeleteTextureHandle();

  RefPtr<CompositorOGL> mCompositor;
  GLuint mTextureHandle;
  GLenum mTextureTarget;
  gfx::IntSize mSize;
  gfx::SurfaceFormat mFormat;
  
  
  bool mExternallyOwned;
};




#ifdef MOZ_WIDGET_ANDROID

class SurfaceTextureSource : public TextureSource
                           , public TextureSourceOGL
{
public:
  SurfaceTextureSource(CompositorOGL* aCompositor,
                       mozilla::gl::AndroidSurfaceTexture* aSurfTex,
                       gfx::SurfaceFormat aFormat,
                       GLenum aTarget,
                       GLenum aWrapMode,
                       gfx::IntSize aSize);

  virtual TextureSourceOGL* AsSourceOGL() { return this; }

  virtual void BindTexture(GLenum activetex, gfx::Filter aFilter) override;

  virtual bool IsValid() const override;

  virtual gfx::IntSize GetSize() const override { return mSize; }

  virtual gfx::SurfaceFormat GetFormat() const override { return mFormat; }

  virtual gfx::Matrix4x4 GetTextureTransform() override;

  virtual GLenum GetTextureTarget() const { return mTextureTarget; }

  virtual GLenum GetWrapMode() const override { return mWrapMode; }

  
  virtual void DeallocateDeviceData() override {}

  virtual void SetCompositor(Compositor* aCompositor) override;

  gl::GLContext* gl() const;

protected:
  RefPtr<CompositorOGL> mCompositor;
  mozilla::gl::AndroidSurfaceTexture* const mSurfTex;
  const gfx::SurfaceFormat mFormat;
  const GLenum mTextureTarget;
  const GLenum mWrapMode;
  const gfx::IntSize mSize;
};

class SurfaceTextureHost : public TextureHost
{
public:
  SurfaceTextureHost(TextureFlags aFlags,
                     mozilla::gl::AndroidSurfaceTexture* aSurfTex,
                     gfx::IntSize aSize);

  virtual ~SurfaceTextureHost();

  
  virtual void DeallocateDeviceData() override {}

  virtual void SetCompositor(Compositor* aCompositor) override;

  virtual bool Lock() override;

  virtual void Unlock() override;

  virtual gfx::SurfaceFormat GetFormat() const override;

  virtual bool BindTextureSource(CompositableTextureSourceRef& aTexture) override
  {
    aTexture = mTextureSource;
    return !!aTexture;
  }

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() override
  {
    return nullptr; 
  }

  gl::GLContext* gl() const;

  virtual gfx::IntSize GetSize() const override { return mSize; }

  virtual const char* Name() { return "SurfaceTextureHost"; }

protected:
  mozilla::gl::AndroidSurfaceTexture* const mSurfTex;
  const gfx::IntSize mSize;
  RefPtr<CompositorOGL> mCompositor;
  RefPtr<SurfaceTextureSource> mTextureSource;
};

#endif 




class EGLImageTextureSource : public TextureSource
                            , public TextureSourceOGL
{
public:
  EGLImageTextureSource(CompositorOGL* aCompositor,
                        EGLImage aImage,
                        gfx::SurfaceFormat aFormat,
                        GLenum aTarget,
                        GLenum aWrapMode,
                        gfx::IntSize aSize);

  virtual TextureSourceOGL* AsSourceOGL() override { return this; }

  virtual void BindTexture(GLenum activetex, gfx::Filter aFilter) override;

  virtual bool IsValid() const override;

  virtual gfx::IntSize GetSize() const override { return mSize; }

  virtual gfx::SurfaceFormat GetFormat() const override { return mFormat; }

  virtual gfx::Matrix4x4 GetTextureTransform() override;

  virtual GLenum GetTextureTarget() const override { return mTextureTarget; }

  virtual GLenum GetWrapMode() const override { return mWrapMode; }

  
  virtual void DeallocateDeviceData() override {}

  virtual void SetCompositor(Compositor* aCompositor) override;

  gl::GLContext* gl() const;

protected:
  RefPtr<CompositorOGL> mCompositor;
  const EGLImage mImage;
  const gfx::SurfaceFormat mFormat;
  const GLenum mTextureTarget;
  const GLenum mWrapMode;
  const gfx::IntSize mSize;
};

class EGLImageTextureHost : public TextureHost
{
public:
  EGLImageTextureHost(TextureFlags aFlags,
                     EGLImage aImage,
                     EGLSync aSync,
                     gfx::IntSize aSize);

  virtual ~EGLImageTextureHost();

  
  virtual void DeallocateDeviceData() override {}

  virtual void SetCompositor(Compositor* aCompositor) override;

  virtual bool Lock() override;

  virtual void Unlock() override;

  virtual gfx::SurfaceFormat GetFormat() const override;

  virtual bool BindTextureSource(CompositableTextureSourceRef& aTexture) override
  {
    aTexture = mTextureSource;
    return !!aTexture;
  }

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() override
  {
    return nullptr; 
  }

  gl::GLContext* gl() const;

  virtual gfx::IntSize GetSize() const override { return mSize; }

  virtual const char* Name() override { return "EGLImageTextureHost"; }

protected:
  const EGLImage mImage;
  const EGLSync mSync;
  const gfx::IntSize mSize;
  RefPtr<CompositorOGL> mCompositor;
  RefPtr<EGLImageTextureSource> mTextureSource;
};

} 
} 

#endif 
