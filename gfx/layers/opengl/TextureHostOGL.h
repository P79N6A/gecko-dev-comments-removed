




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

class gfxImageSurface;
class gfxReusableSurfaceWrapper;
class nsIntRegion;
struct nsIntPoint;
struct nsIntRect;
struct nsIntSize;

namespace mozilla {
namespace gfx {
class DataSourceSurface;
class SurfaceStream;
}

namespace layers {

class Compositor;
class CompositorOGL;
class TextureImageDeprecatedTextureHostOGL;










class CompositableDataGonkOGL : public CompositableBackendSpecificData
{
public:
  CompositableDataGonkOGL();
  virtual ~CompositableDataGonkOGL();

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;
  virtual void ClearData() MOZ_OVERRIDE;
  gl::GLContext* gl() const;
protected:
  RefPtr<CompositorOGL> mCompositor;
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

  virtual TextureImageDeprecatedTextureHostOGL* AsTextureImageDeprecatedTextureHost() { return nullptr; }

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
#if MOZ_WIDGET_GONK && ANDROID_VERSION >= 17

  



  virtual bool SetReleaseFence(const android::sp<android::Fence>& aReleaseFence);

  


  virtual android::sp<android::Fence> GetAndResetReleaseFence();
protected:
  android::sp<android::Fence> mReleaseFence;
#endif
};










class TextureImageTextureSourceOGL : public DataTextureSource
                                   , public TextureSourceOGL
                                   , public TileIterator
{
public:
  TextureImageTextureSourceOGL(gl::GLContext* aGL,
                               TextureFlags aFlags = TEXTURE_FLAGS_DEFAULT)
    : mGL(aGL)
    , mFlags(aFlags)
    , mIterating(false)
  {}

  

  virtual bool Update(gfx::DataSourceSurface* aSurface,
                      nsIntRegion* aDestRegion = nullptr,
                      gfx::IntPoint* aSrcOffset = nullptr) MOZ_OVERRIDE;

  

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

  

  virtual TileIterator* AsTileIterator() MOZ_OVERRIDE { return this; }

  virtual void BeginTileIteration() MOZ_OVERRIDE
  {
    mTexImage->BeginTileIteration();
    mIterating = true;
  }

  virtual void EndTileIteration() MOZ_OVERRIDE
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









class SharedTextureSourceOGL : public NewTextureSource
                             , public TextureSourceOGL
{
public:
  typedef gl::SharedTextureShareType SharedTextureShareType;

  SharedTextureSourceOGL(CompositorOGL* aCompositor,
                         gl::SharedTextureHandle aHandle,
                         gfx::SurfaceFormat aFormat,
                         GLenum aTarget,
                         GLenum aWrapMode,
                         SharedTextureShareType aShareType,
                         gfx::IntSize aSize);

  virtual TextureSourceOGL* AsSourceOGL() { return this; }

  virtual void BindTexture(GLenum activetex, gfx::Filter aFilter) MOZ_OVERRIDE;

  virtual bool IsValid() const MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }

  virtual gfx::Matrix4x4 GetTextureTransform() MOZ_OVERRIDE;

  virtual GLenum GetTextureTarget() const { return mTextureTarget; }

  virtual GLenum GetWrapMode() const MOZ_OVERRIDE { return mWrapMode; }

  
  virtual void DeallocateDeviceData() {}

  void DetachSharedHandle();

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  gl::GLContext* gl() const;

protected:
  gfx::IntSize mSize;
  CompositorOGL* mCompositor;
  gl::SharedTextureHandle mSharedHandle;
  gfx::SurfaceFormat mFormat;
  SharedTextureShareType mShareType;
  GLenum mTextureTarget;
  GLenum mWrapMode;
};






class SharedTextureHostOGL : public TextureHost
{
public:
  SharedTextureHostOGL(TextureFlags aFlags,
                       gl::SharedTextureShareType aShareType,
                       gl::SharedTextureHandle aSharedhandle,
                       gfx::IntSize aSize,
                       bool inverted);

  virtual ~SharedTextureHostOGL();

  
  virtual void DeallocateDeviceData() MOZ_OVERRIDE {}

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual bool Lock() MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  virtual NewTextureSource* GetTextureSources() MOZ_OVERRIDE
  {
    return mTextureSource;
  }

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE
  {
    return nullptr; 
  }

  gl::GLContext* gl() const;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual const char* Name() { return "SharedTextureHostOGL"; }

protected:
  gfx::IntSize mSize;
  CompositorOGL* mCompositor;
  gl::SharedTextureHandle mSharedHandle;
  gl::SharedTextureShareType mShareType;

  RefPtr<SharedTextureSourceOGL> mTextureSource;
};






class StreamTextureSourceOGL : public NewTextureSource
                             , public TextureSourceOGL
{
public:
  StreamTextureSourceOGL(CompositorOGL* aCompositor,
                         gfx::SurfaceStream* aStream)
    : mCompositor(aCompositor)
    , mStream(aStream)
    , mTextureHandle(0)
    , mTextureTarget(LOCAL_GL_TEXTURE_2D)
    , mUploadTexture(0)
    , mFormat(gfx::SurfaceFormat::UNKNOWN)
  {
    MOZ_COUNT_CTOR(StreamTextureSourceOGL);
  }

  ~StreamTextureSourceOGL()
  {
    MOZ_COUNT_DTOR(StreamTextureSourceOGL);
  }

  virtual TextureSourceOGL* AsSourceOGL() { return this; }

  virtual void BindTexture(GLenum activetex, gfx::Filter aFilter) MOZ_OVERRIDE;

  virtual bool IsValid() const MOZ_OVERRIDE { return !!gl(); }

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }

  virtual GLenum GetTextureTarget() const { return mTextureTarget; }

  virtual GLenum GetWrapMode() const MOZ_OVERRIDE { return LOCAL_GL_CLAMP_TO_EDGE; }

  virtual void DeallocateDeviceData();

  bool RetrieveTextureFromStream();

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

protected:
  gl::GLContext* gl() const;

  CompositorOGL* mCompositor;
  gfx::SurfaceStream* mStream;
  GLuint mTextureHandle;
  GLenum mTextureTarget;
  GLuint mUploadTexture;
  gfx::IntSize mSize;
  gfx::SurfaceFormat mFormat;
};




class StreamTextureHostOGL : public TextureHost
{
public:
  StreamTextureHostOGL(TextureFlags aFlags,
                       const SurfaceStreamDescriptor& aDesc);

  virtual ~StreamTextureHostOGL();

  
  virtual void DeallocateDeviceData() MOZ_OVERRIDE {}

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual bool Lock() MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  virtual NewTextureSource* GetTextureSources() MOZ_OVERRIDE
  {
    return mTextureSource;
  }

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE
  {
    return nullptr; 
  }

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "StreamTextureHostOGL"; }
#endif

protected:
  CompositorOGL* mCompositor;
  gfx::SurfaceStream* mStream;
  RefPtr<StreamTextureSourceOGL> mTextureSource;
};




class TextureImageDeprecatedTextureHostOGL : public DeprecatedTextureHost
                                           , public TextureSourceOGL
                                           , public TileIterator
{
public:
  TextureImageDeprecatedTextureHostOGL(gl::TextureImage* aTexImage = nullptr)
    : mTexture(aTexImage)
    , mGL(nullptr)
    , mIterating(false)
  {
    MOZ_COUNT_CTOR(TextureImageDeprecatedTextureHostOGL);
  }

  ~TextureImageDeprecatedTextureHostOGL();

  TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE
  {
    return this;
  }

  virtual TextureImageDeprecatedTextureHostOGL* AsTextureImageDeprecatedTextureHost() MOZ_OVERRIDE
  {
    return this;
  }

  
  
  void SetGLContext(gl::GLContext* aGL)
  {
    mGL = aGL;
  }

  

  void UpdateImpl(const SurfaceDescriptor& aImage,
                  nsIntRegion* aRegion = nullptr,
                  nsIntPoint* aOffset = nullptr) MOZ_OVERRIDE;

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual void EnsureBuffer(const nsIntSize& aSize, gfxContentType aType) MOZ_OVERRIDE;

  virtual void CopyTo(const nsIntRect& aSourceRect,
                      DeprecatedTextureHost *aDest,
                      const nsIntRect& aDestRect) MOZ_OVERRIDE;

  bool IsValid() const MOZ_OVERRIDE
  {
    return !!mTexture;
  }

  virtual bool Lock() MOZ_OVERRIDE;

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE;

  
  void BindTexture(GLenum aTextureUnit, gfx::Filter aFilter) MOZ_OVERRIDE
  {
    mTexture->BindTexture(aTextureUnit);
  }

  gfx::IntSize GetSize() const MOZ_OVERRIDE;

  GLenum GetWrapMode() const MOZ_OVERRIDE
  {
    return mTexture->GetWrapMode();
  }

  gl::TextureImage* GetTextureImage()
  {
    return mTexture;
  }

  void SetTextureImage(gl::TextureImage* aImage)
  {
    mTexture = aImage;
  }

  

  TileIterator* AsTileIterator() MOZ_OVERRIDE
  {
    return this;
  }

  void BeginTileIteration() MOZ_OVERRIDE
  {
    mTexture->BeginTileIteration();
    mIterating = true;
  }

  void EndTileIteration() MOZ_OVERRIDE
  {
    mIterating = false;
  }

  nsIntRect GetTileRect() MOZ_OVERRIDE;

  size_t GetTileCount() MOZ_OVERRIDE
  {
    return mTexture->GetTileCount();
  }

  bool NextTile() MOZ_OVERRIDE
  {
    return mTexture->NextTile();
  }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE
  {
    return DeprecatedTextureHost::GetFormat();
  }

  virtual const char* Name() { return "TextureImageDeprecatedTextureHostOGL"; }

protected:
  nsRefPtr<gl::TextureImage> mTexture;
  gl::GLContext* mGL;
  bool mIterating;
};

class TiledDeprecatedTextureHostOGL : public DeprecatedTextureHost
                          , public TextureSourceOGL
{
public:
  TiledDeprecatedTextureHostOGL()
    : mTextureHandle(0)
    , mGL(nullptr)
  {}
  ~TiledDeprecatedTextureHostOGL();

  virtual void SetCompositor(Compositor* aCompositor);

  
  virtual void Update(gfxReusableSurfaceWrapper* aReusableSurface, TextureFlags aFlags, const gfx::IntSize& aSize) MOZ_OVERRIDE;
  virtual bool Lock() MOZ_OVERRIDE;
  virtual void Unlock() MOZ_OVERRIDE {}

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE
  {
    return DeprecatedTextureHost::GetFormat();
  }

  virtual TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE { return this; }
  virtual bool IsValid() const MOZ_OVERRIDE { return true; }
  virtual GLenum GetWrapMode() const MOZ_OVERRIDE { return LOCAL_GL_CLAMP_TO_EDGE; }
  virtual void BindTexture(GLenum aTextureUnit, gfx::Filter aFilter);
  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE
  {
    return mSize;
  }

  virtual void SwapTexturesImpl(const SurfaceDescriptor& aImage,
                                nsIntRegion* aRegion = nullptr)
  { MOZ_ASSERT(false, "Tiles should not use this path"); }

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE;

  virtual const char* Name() { return "TiledDeprecatedTextureHostOGL"; }

protected:
  void DeleteTextures();

  virtual uint64_t GetIdentifier() const MOZ_OVERRIDE {
    return static_cast<uint64_t>(mTextureHandle);
  }

private:
  GLenum GetTileType()
  {
    
    return mGLFormat == LOCAL_GL_RGB ? LOCAL_GL_UNSIGNED_SHORT_5_6_5 : LOCAL_GL_UNSIGNED_BYTE;
  }

  gfx::IntSize mSize;
  GLuint mTextureHandle;
  GLenum mGLFormat;
  nsRefPtr<gl::GLContext> mGL;
};

#ifdef MOZ_WIDGET_GONK





class GrallocDeprecatedTextureHostOGL
  : public DeprecatedTextureHost
  , public TextureSourceOGL
{
public:
  GrallocDeprecatedTextureHostOGL();

  ~GrallocDeprecatedTextureHostOGL();

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual void UpdateImpl(const SurfaceDescriptor& aImage,
                          nsIntRegion* aRegion = nullptr,
                          nsIntPoint* aOffset = nullptr) MOZ_OVERRIDE;
  virtual void SwapTexturesImpl(const SurfaceDescriptor& aImage,
                          nsIntRegion* aRegion = nullptr) MOZ_OVERRIDE;
  virtual bool Lock() MOZ_OVERRIDE;
  virtual void Unlock() MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE
  {
    return mGraphicBuffer.get() ? gfx::IntSize(mGraphicBuffer->getWidth(), mGraphicBuffer->getHeight()) : gfx::IntSize(0, 0);
  }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  GLenum GetWrapMode() const MOZ_OVERRIDE
  {
    return LOCAL_GL_CLAMP_TO_EDGE;
  }

  virtual GLenum GetTextureTarget() const MOZ_OVERRIDE
  {
    return mTextureTarget;
  }

  bool IsValid() const MOZ_OVERRIDE;

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE;

  virtual const char* Name() { return "GrallocDeprecatedTextureHostOGL"; }

  void BindTexture(GLenum aTextureUnit, gfx::Filter aFilter) MOZ_OVERRIDE;

  virtual TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE
  {
    return this;
  }

  
  
  virtual void SetBuffer(SurfaceDescriptor* aBuffer, ISurfaceAllocator* aAllocator) MOZ_OVERRIDE;

  
  virtual void ForgetBuffer()
  {
    if (mBuffer) {
      
      
      
      
      delete mBuffer;
      mBuffer = nullptr;
    }

    mGraphicBuffer = nullptr;
    DeleteTextures();
  }

  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE;

  GLuint GetGLTexture();

private:
  gl::GLContext* gl() const;

  void DeleteTextures();

  RefPtr<CompositorOGL> mCompositor;
  android::sp<android::GraphicBuffer> mGraphicBuffer;
  GLenum mTextureTarget;
  EGLImage mEGLImage;
  
  bool mIsRBSwapped;
};
#endif

} 
} 

#endif 
