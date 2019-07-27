




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
class nsSurfaceTexture;
struct nsIntPoint;
struct nsIntRect;
struct nsIntSize;

namespace mozilla {
namespace gfx {
class DataSourceSurface;
}

namespace layers {

class Compositor;
class CompositorOGL;
class TextureImageTextureSourceOGL;
class TextureSharedDataGonkOGL;










class CompositableDataGonkOGL : public CompositableBackendSpecificData
{
protected:
  virtual ~CompositableDataGonkOGL();

public:
  CompositableDataGonkOGL();
  virtual void ClearData() MOZ_OVERRIDE;
  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  TextureSharedDataGonkOGL* GetTextureBackendSpecificData();
protected:
  nsRefPtr<TextureSharedDataGonkOGL> mTextureBackendSpecificData;
  RefPtr<CompositorOGL> mCompositor;
};











class TextureSharedDataGonkOGL
{
protected:
  virtual ~TextureSharedDataGonkOGL();

public:
  NS_INLINE_DECL_REFCOUNTING(TextureSharedDataGonkOGL)

  TextureSharedDataGonkOGL();
  TextureSharedDataGonkOGL(GLuint aTexture, EGLImage aImage, CompositorOGL* aCompositor);

  void SetCompositor(Compositor* aCompositor);
  void ClearData();

  
  void SetOwnedByTextureHost()
  {
    mOwnedByCompositableHost = false;
  }

  
  bool IsOwnedByCompositableHost()
  {
    return mOwnedByCompositableHost;
  }

  bool IsAllowingSharingTextureHost()
  {
    return mAllowSharingTextureHost;
  }

  void SetAllowSharingTextureHost(bool aAllow)
  {
    mAllowSharingTextureHost = aAllow;
  }

  
  
  
  
  TemporaryRef<TextureSharedDataGonkOGL> GetNewTextureBackendSpecificData(EGLImage aImage);

  GLuint GetTexture();
  void DeleteTextureIfPresent();
  gl::GLContext* gl() const;
  void BindEGLImage(GLuint aTarget, EGLImage aImage);
  void ClearBoundEGLImage(EGLImage aImage);
  bool IsEGLImageBound(EGLImage aImage);
protected:
  GLuint GetAndResetGLTextureOwnership();

  bool mOwnedByCompositableHost;
  bool mAllowSharingTextureHost;
  RefPtr<CompositorOGL> mCompositor;
  GLuint mTexture;
  EGLImage mBoundEGLImage;
};

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










class TextureImageTextureSourceOGL MOZ_FINAL : public DataTextureSource
                                             , public TextureSourceOGL
                                             , public BigImageIterator
{
public:
  explicit TextureImageTextureSourceOGL(gl::GLContext* aGL,
                                        TextureFlags aFlags = TextureFlags::DEFAULT)
    : mGL(aGL)
    , mFlags(aFlags)
    , mIterating(false)
  {}

  

  virtual bool Update(gfx::DataSourceSurface* aSurface,
                      nsIntRegion* aDestRegion = nullptr,
                      gfx::IntPoint* aSrcOffset = nullptr) MOZ_OVERRIDE;

  void EnsureBuffer(const nsIntSize& aSize,
                            gfxContentType aContentType);

  void CopyTo(const nsIntRect& aSourceRect,
                      DataTextureSource* aDest,
                      const nsIntRect& aDestRect);

  virtual TextureImageTextureSourceOGL* AsTextureImageTextureSource() { return this; }

  

  virtual void DeallocateDeviceData() MOZ_OVERRIDE
  {
    mTexImage = nullptr;
    SetUpdateSerial(0);
  }

  virtual TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE { return this; }

  virtual void BindTexture(GLenum aTextureUnit, gfx::Filter aFilter) MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  virtual bool IsValid() const MOZ_OVERRIDE { return !!mTexImage; }

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual GLenum GetWrapMode() const MOZ_OVERRIDE
  {
    return mTexImage->GetWrapMode();
  }

  

  virtual BigImageIterator* AsBigImageIterator() MOZ_OVERRIDE { return this; }

  virtual void BeginBigImageIteration() MOZ_OVERRIDE
  {
    mTexImage->BeginBigImageIteration();
    mIterating = true;
  }

  virtual void EndBigImageIteration() MOZ_OVERRIDE
  {
    mIterating = false;
  }

  virtual nsIntRect GetTileRect() MOZ_OVERRIDE;

  virtual size_t GetTileCount() MOZ_OVERRIDE
  {
    return mTexImage->GetTileCount();
  }

  virtual bool NextTile() MOZ_OVERRIDE
  {
    return mTexImage->NextTile();
  }

protected:
  nsRefPtr<gl::TextureImage> mTexImage;
  gl::GLContext* mGL;
  TextureFlags mFlags;
  bool mIterating;
};









class GLTextureSource : public TextureSource
                      , public TextureSourceOGL
{
public:
  GLTextureSource(CompositorOGL* aCompositor,
                  GLuint aTex,
                  gfx::SurfaceFormat aFormat,
                  GLenum aTarget,
                  gfx::IntSize aSize);

  virtual TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE { return this; }

  virtual void BindTexture(GLenum activetex, gfx::Filter aFilter) MOZ_OVERRIDE;

  virtual bool IsValid() const MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }

  virtual GLenum GetTextureTarget() const { return mTextureTarget; }

  virtual GLenum GetWrapMode() const MOZ_OVERRIDE { return LOCAL_GL_CLAMP_TO_EDGE; }

  virtual void DeallocateDeviceData() MOZ_OVERRIDE {}

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  gl::GLContext* gl() const;

protected:
  const gfx::IntSize mSize;
  RefPtr<CompositorOGL> mCompositor;
  const GLuint mTex;
  const gfx::SurfaceFormat mFormat;
  const GLenum mTextureTarget;
};




#ifdef MOZ_WIDGET_ANDROID

class SurfaceTextureSource : public TextureSource
                           , public TextureSourceOGL
{
public:
  SurfaceTextureSource(CompositorOGL* aCompositor,
                       nsSurfaceTexture* aSurfTex,
                       gfx::SurfaceFormat aFormat,
                       GLenum aTarget,
                       GLenum aWrapMode,
                       gfx::IntSize aSize);

  virtual TextureSourceOGL* AsSourceOGL() { return this; }

  virtual void BindTexture(GLenum activetex, gfx::Filter aFilter) MOZ_OVERRIDE;

  virtual bool IsValid() const MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }

  virtual gfx::Matrix4x4 GetTextureTransform() MOZ_OVERRIDE;

  virtual GLenum GetTextureTarget() const { return mTextureTarget; }

  virtual GLenum GetWrapMode() const MOZ_OVERRIDE { return mWrapMode; }

  
  virtual void DeallocateDeviceData() MOZ_OVERRIDE {}

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  gl::GLContext* gl() const;

protected:
  RefPtr<CompositorOGL> mCompositor;
  nsSurfaceTexture* const mSurfTex;
  const gfx::SurfaceFormat mFormat;
  const GLenum mTextureTarget;
  const GLenum mWrapMode;
  const gfx::IntSize mSize;
};

class SurfaceTextureHost : public TextureHost
{
public:
  SurfaceTextureHost(TextureFlags aFlags,
                     nsSurfaceTexture* aSurfTex,
                     gfx::IntSize aSize);

  virtual ~SurfaceTextureHost();

  
  virtual void DeallocateDeviceData() MOZ_OVERRIDE {}

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual bool Lock() MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  virtual TextureSource* GetTextureSources() MOZ_OVERRIDE
  {
    return mTextureSource;
  }

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE
  {
    return nullptr; 
  }

  gl::GLContext* gl() const;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual const char* Name() { return "SurfaceTextureHost"; }

protected:
  nsSurfaceTexture* const mSurfTex;
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

  virtual TextureSourceOGL* AsSourceOGL() { return this; }

  virtual void BindTexture(GLenum activetex, gfx::Filter aFilter) MOZ_OVERRIDE;

  virtual bool IsValid() const MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }

  virtual gfx::Matrix4x4 GetTextureTransform() MOZ_OVERRIDE;

  virtual GLenum GetTextureTarget() const { return mTextureTarget; }

  virtual GLenum GetWrapMode() const MOZ_OVERRIDE { return mWrapMode; }

  
  virtual void DeallocateDeviceData() MOZ_OVERRIDE {}

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

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
                     gfx::IntSize aSize);

  virtual ~EGLImageTextureHost();

  
  virtual void DeallocateDeviceData() MOZ_OVERRIDE {}

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual bool Lock() MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  virtual TextureSource* GetTextureSources() MOZ_OVERRIDE
  {
    return mTextureSource;
  }

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE
  {
    return nullptr; 
  }

  gl::GLContext* gl() const;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual const char* Name() { return "EGLImageTextureHost"; }

protected:
  const EGLImage mImage;
  const gfx::IntSize mSize;
  RefPtr<CompositorOGL> mCompositor;
  RefPtr<EGLImageTextureSource> mTextureSource;
};

} 
} 

#endif 
