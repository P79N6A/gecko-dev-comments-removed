




#ifndef MOZILLA_GFX_TEXTUREOGL_H
#define MOZILLA_GFX_TEXTUREOGL_H

#include <stddef.h>                     
#include <stdint.h>                     
#include "CompositableHost.h"
#include "GLContextTypes.h"             
#include "GLDefs.h"                     
#include "GLTextureImage.h"             
#include "gfx3DMatrix.h"                
#include "gfxTypes.h"
#include "mozilla/GfxMessageUtils.h"    
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/layers/TextureHost.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsTraceRefcnt.h"              
#include "LayerManagerOGLProgram.h"     
#ifdef MOZ_WIDGET_GONK
#include <ui/GraphicBuffer.h>
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










class CompositableQuirksGonkOGL : public CompositableQuirks
{
public:
  CompositableQuirksGonkOGL();
  virtual ~CompositableQuirksGonkOGL();

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;
  GLuint GetTexture();
  void DeleteTextureIfPresent();
  gl::GLContext* gl() const;
protected:
  RefPtr<CompositorOGL> mCompositor;
  GLuint mTexture;
};
















inline ShaderProgramType
GetProgramTypeForSurfaceFormat(gfx::SurfaceFormat aFormat)
 {
  switch (aFormat) {
  case gfx::FORMAT_B8G8R8A8:
    return BGRALayerProgramType;;
  case gfx::FORMAT_B8G8R8X8:
    return BGRXLayerProgramType;;
  case gfx::FORMAT_R8G8B8X8:
    return RGBXLayerProgramType;;
  case gfx::FORMAT_R8G8B8A8:
    return RGBALayerProgramType;;
  default:
    MOZ_CRASH("unhandled program type");
  }
}

inline ShaderProgramType
GetProgramTypeForTexture(const DeprecatedTextureHost *aDeprecatedTextureHost)
{
  return GetProgramTypeForSurfaceFormat(aDeprecatedTextureHost->GetFormat());
}





class TextureSourceOGL
{
public:
  virtual bool IsValid() const = 0;

  virtual void BindTexture(GLenum aTextureUnit) = 0;

  virtual void UnbindTexture() = 0;

  virtual gfx::IntSize GetSize() const = 0;

  virtual GLenum GetTextureTarget() const { return LOCAL_GL_TEXTURE_2D; }

  virtual gfx::SurfaceFormat GetFormat() const = 0;

  virtual GLenum GetWrapMode() const = 0;

  virtual gfx3DMatrix GetTextureTransform() { return gfx3DMatrix(); }

  virtual TextureImageDeprecatedTextureHostOGL* AsTextureImageDeprecatedTextureHost() { return nullptr; }
};










class TextureImageTextureSourceOGL : public DataTextureSource
                                   , public TextureSourceOGL
                                   , public TileIterator
{
public:
  TextureImageTextureSourceOGL(gl::GLContext* aGL, bool aAllowBiImage = true)
    : mGL(aGL)
    , mAllowBigImage(aAllowBiImage)
    , mIterating(false)
  {}

  

  virtual bool Update(gfx::DataSourceSurface* aSurface,
                      TextureFlags aFlags,
                      nsIntRegion* aDestRegion = nullptr,
                      gfx::IntPoint* aSrcOffset = nullptr) MOZ_OVERRIDE;

  

  virtual void DeallocateDeviceData() MOZ_OVERRIDE
  {
    mTexImage = nullptr;
    SetUpdateSerial(0);
  }

  virtual TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE { return this; }

  virtual void BindTexture(GLenum aTextureUnit) MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  virtual bool IsValid() const MOZ_OVERRIDE { return !!mTexImage; }

  virtual void UnbindTexture() MOZ_OVERRIDE
  {
    mTexImage->ReleaseTexture();
  }

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
  bool mAllowBigImage;
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
                         gfx::IntSize aSize,
                         const gfx3DMatrix& aTexTransform);

  virtual TextureSourceOGL* AsSourceOGL() { return this; }

  virtual void BindTexture(GLenum activetex) MOZ_OVERRIDE;

  virtual bool IsValid() const MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }

  virtual gfx3DMatrix GetTextureTransform() MOZ_OVERRIDE { return mTextureTransform; }

  virtual GLenum GetTextureTarget() const { return mTextureTarget; }

  virtual GLenum GetWrapMode() const MOZ_OVERRIDE { return mWrapMode; }

  virtual void UnbindTexture() MOZ_OVERRIDE {}

  
  virtual void DeallocateDeviceData() {}

  void DetachSharedHandle();

  void SetCompositor(CompositorOGL* aCompositor);

  gl::GLContext* gl() const;

protected:
  gfx3DMatrix mTextureTransform;
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
  SharedTextureHostOGL(uint64_t aID,
                       TextureFlags aFlags,
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

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE
  {
    return nullptr; 
  }

  gl::GLContext* gl() const;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "SharedTextureHostOGL"; }
#endif

protected:
  gfx::IntSize mSize;
  CompositorOGL* mCompositor;
  gl::SharedTextureHandle mSharedHandle;
  gl::SharedTextureShareType mShareType;

  RefPtr<SharedTextureSourceOGL> mTextureSource;
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

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;

  
  void BindTexture(GLenum aTextureUnit) MOZ_OVERRIDE
  {
    mTexture->BindTexture(aTextureUnit);
  }

  void UnbindTexture() MOZ_OVERRIDE
  {
    mTexture->ReleaseTexture();
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

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "TextureImageDeprecatedTextureHostOGL"; }
#endif

protected:
  nsRefPtr<gl::TextureImage> mTexture;
  gl::GLContext* mGL;
  bool mIterating;
};












class YCbCrDeprecatedTextureHostOGL : public DeprecatedTextureHost
{
public:
  YCbCrDeprecatedTextureHostOGL()
    : mGL(nullptr)
  {
    MOZ_COUNT_CTOR(YCbCrDeprecatedTextureHostOGL);
    mYTexture  = new Channel;
    mCbTexture = new Channel;
    mCrTexture = new Channel;
    mFormat = gfx::FORMAT_YUV;
  }

  ~YCbCrDeprecatedTextureHostOGL()
  {
    MOZ_COUNT_DTOR(YCbCrDeprecatedTextureHostOGL);
  }

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual void UpdateImpl(const SurfaceDescriptor& aImage,
                          nsIntRegion* aRegion = nullptr,
                          nsIntPoint* aOffset = nullptr) MOZ_OVERRIDE;

  virtual bool Lock() MOZ_OVERRIDE;

  virtual bool IsValid() const MOZ_OVERRIDE
  {
    return mYTexture->IsValid()
        && mCbTexture->IsValid()
        && mCrTexture->IsValid();
  }

  struct Channel : public TextureSourceOGL
                 , public TextureSource
  {
    TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE
    {
      return this;
    }
    nsRefPtr<gl::TextureImage> mTexImage;

    void BindTexture(GLenum aUnit) MOZ_OVERRIDE
    {
      mTexImage->BindTexture(aUnit);
    }
    void UnbindTexture() MOZ_OVERRIDE
    {
      mTexImage->ReleaseTexture();
    }
    virtual bool IsValid() const MOZ_OVERRIDE
    {
      return !!mTexImage;
    }
    virtual gfx::IntSize GetSize() const MOZ_OVERRIDE
    {
      return gfx::IntSize(mTexImage->GetSize().width, mTexImage->GetSize().height);
    }
    virtual GLenum GetWrapMode() const MOZ_OVERRIDE
    {
      return mTexImage->GetWrapMode();
    }
    virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE
    {
      return gfx::FORMAT_A8;
    }
  };

  

  TextureSource* GetSubSource(int index) MOZ_OVERRIDE
  {
    switch (index) {
      case 0 : return mYTexture.get();
      case 1 : return mCbTexture.get();
      case 2 : return mCrTexture.get();
    }
    return nullptr;
  }

  gfx::IntSize GetSize() const MOZ_OVERRIDE
  {
    if (!mYTexture->mTexImage) {
      NS_WARNING("YCbCrDeprecatedTextureHost::GetSize called but no data has been set yet");
      return gfx::IntSize(0,0);
    }
    return mYTexture->GetSize();
  }

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "YCbCrDeprecatedTextureHostOGL"; }
#endif

private:
  RefPtr<Channel> mYTexture;
  RefPtr<Channel> mCbTexture;
  RefPtr<Channel> mCrTexture;
  gl::GLContext* mGL;
};


class SharedDeprecatedTextureHostOGL : public DeprecatedTextureHost
                           , public TextureSourceOGL
{
public:
  typedef gfxContentType ContentType;
  typedef mozilla::gl::GLContext GLContext;
  typedef mozilla::gl::TextureImage TextureImage;

  SharedDeprecatedTextureHostOGL()
    : mGL(nullptr)
    , mTextureHandle(0)
    , mWrapMode(LOCAL_GL_CLAMP_TO_EDGE)
    , mSharedHandle(0)
    , mShareType(gl::SameProcess)
  {}

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual ~SharedDeprecatedTextureHostOGL()
  {
    if (mSharedHandle || mTextureHandle) {
      DeleteTextures();
    }
  }

  virtual GLuint GetTextureHandle()
  {
    return mTextureHandle;
  }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE
  {
    return DeprecatedTextureHost::GetFormat();
  }

  virtual TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE { return this; }

  bool IsValid() const MOZ_OVERRIDE { return !!mSharedHandle; }

  
  
  virtual void UpdateImpl(const SurfaceDescriptor& aImage,
                          nsIntRegion* aRegion = nullptr,
                          nsIntPoint* aOffset = nullptr) MOZ_OVERRIDE;
  virtual void SwapTexturesImpl(const SurfaceDescriptor& aImage,
                                nsIntRegion* aRegion = nullptr) MOZ_OVERRIDE;
  virtual bool Lock() MOZ_OVERRIDE;
  virtual void Unlock() MOZ_OVERRIDE;

  virtual GLenum GetWrapMode() const MOZ_OVERRIDE { return mWrapMode; }
  virtual void SetWrapMode(GLenum aMode) { mWrapMode = aMode; }

  virtual GLenum GetTextureTarget() const MOZ_OVERRIDE
  {
    return mTextureTarget;
  }

  gfx::IntSize GetSize() const MOZ_OVERRIDE {
    return mSize;
  }

  void BindTexture(GLenum activetex) MOZ_OVERRIDE
  {
    MOZ_ASSERT(mGL);
    
    MOZ_ASSERT(activetex == LOCAL_GL_TEXTURE0);
  }
  void UnbindTexture() MOZ_OVERRIDE {}
  GLuint GetTextureID() { return mTextureHandle; }
  ContentType GetContentType()
  {
    return (mFormat == gfx::FORMAT_B8G8R8A8) ?
             GFX_CONTENT_COLOR_ALPHA :
             GFX_CONTENT_COLOR;
  }

  virtual gfx3DMatrix GetTextureTransform() MOZ_OVERRIDE;

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "SharedDeprecatedTextureHostOGL"; }
#endif

protected:
  void DeleteTextures();

  gfx::IntSize mSize;
  nsRefPtr<gl::GLContext> mGL;
  GLuint mTextureHandle;
  GLenum mWrapMode;
  GLenum mTextureTarget;
  gl::SharedTextureHandle mSharedHandle;
  gl::SharedTextureShareType mShareType;
};

class SurfaceStreamHostOGL : public DeprecatedTextureHost
                           , public TextureSourceOGL
{
public:
  typedef gfxContentType ContentType;
  typedef mozilla::gl::GLContext GLContext;
  typedef mozilla::gl::TextureImage TextureImage;

  virtual ~SurfaceStreamHostOGL()
  {
    DeleteTextures();
  }

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual GLuint GetTextureHandle()
  {
    return mTextureHandle;
  }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE
  {
    return DeprecatedTextureHost::GetFormat();
  }

  virtual TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE { return this; }

  bool IsValid() const MOZ_OVERRIDE { return true; }

  
  virtual void UpdateImpl(const SurfaceDescriptor& aImage,
                          nsIntRegion* aRegion,
                          nsIntPoint* aOffset);
  virtual bool Lock() MOZ_OVERRIDE;
  virtual void Unlock() MOZ_OVERRIDE;

  virtual GLenum GetWrapMode() const MOZ_OVERRIDE {
    return mWrapMode;
  }
  virtual void SetWrapMode(GLenum aMode) {
    mWrapMode = aMode;
  }

  gfx::IntSize GetSize() const MOZ_OVERRIDE {
    return mSize;
  }

  virtual GLenum GetTextureTarget() const MOZ_OVERRIDE
  {
    return mTextureTarget;
  }

  void BindTexture(GLenum activetex) MOZ_OVERRIDE;

  void UnbindTexture() MOZ_OVERRIDE {}

  GLuint GetTextureID() { return mTextureHandle; }
  ContentType GetContentType() {
    return (mFormat == gfx::FORMAT_B8G8R8A8) ?
             GFX_CONTENT_COLOR_ALPHA :
             GFX_CONTENT_COLOR;
  }

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "SurfaceStreamHostOGL"; }
#endif

  SurfaceStreamHostOGL()
    : mGL(nullptr)
    , mTextureHandle(0)
    , mTextureTarget(LOCAL_GL_TEXTURE_2D)
    , mUploadTexture(0)
    , mWrapMode(LOCAL_GL_CLAMP_TO_EDGE)
    , mStream(nullptr)
  {}

protected:
  void DeleteTextures();

  gfx::IntSize mSize;
  nsRefPtr<GLContext> mGL;
  GLuint mTextureHandle;
  GLenum mTextureTarget;
  GLuint mUploadTexture;
  GLenum mWrapMode;
  nsRefPtr<GLContext> mStreamGL;
  gfx::SurfaceStream *mStream;
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
  virtual void BindTexture(GLenum aTextureUnit);
  virtual void UnbindTexture() MOZ_OVERRIDE {}
  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE
  {
    return mSize;
  }

  virtual void SwapTexturesImpl(const SurfaceDescriptor& aImage,
                                nsIntRegion* aRegion = nullptr)
  { MOZ_ASSERT(false, "Tiles should not use this path"); }

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "TiledDeprecatedTextureHostOGL"; }
#endif

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

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "GrallocDeprecatedTextureHostOGL"; }
#endif

  void BindTexture(GLenum aTextureUnit) MOZ_OVERRIDE;
  void UnbindTexture() MOZ_OVERRIDE {}

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
