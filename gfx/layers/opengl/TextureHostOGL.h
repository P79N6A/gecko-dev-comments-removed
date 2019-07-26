




#ifndef MOZILLA_GFX_TEXTUREOGL_H
#define MOZILLA_GFX_TEXTUREOGL_H

#include "ImageLayerOGL.h"
#include "GLContextTypes.h"
#include "gfx2DGlue.h"
#include "mozilla/layers/Effects.h"
#include "gfxReusableSurfaceWrapper.h"
#include "TiledLayerBuffer.h" 

#ifdef MOZ_WIDGET_GONK
#include <ui/GraphicBuffer.h>
#endif

namespace mozilla {
namespace layers {

class TextureImageTextureHostOGL;
class CompositorOGL;




















class TextureSourceOGL
{
public:
  virtual bool IsValid() const = 0;
  virtual void BindTexture(GLenum aTextureUnit) = 0;
  virtual gfx::IntSize GetSize() const = 0;
  virtual gl::ShaderProgramType GetShaderProgram() const {
    MOZ_NOT_REACHED("unhandled shader type");
  }
  
  virtual GLenum GetTextureTarget() const { return LOCAL_GL_TEXTURE_2D; }
  virtual GLenum GetWrapMode() const = 0;
  virtual gfx3DMatrix GetTextureTransform() { return gfx3DMatrix(); }

  virtual TextureImageTextureHostOGL* AsTextureImageTextureHost() { return nullptr; }
};

inline gl::ShaderProgramType
GetProgramTypeForTexture(const TextureHost *aTextureHost)
{
  switch (aTextureHost->GetFormat()) {
  case gfx::FORMAT_B8G8R8A8:
    return gl::BGRALayerProgramType;;
  case gfx::FORMAT_B8G8R8X8:
    return gl::BGRXLayerProgramType;;
  case gfx::FORMAT_R8G8B8X8:
    return gl::RGBXLayerProgramType;;
  case gfx::FORMAT_R8G8B8A8:
    return gl::RGBALayerProgramType;;
  default:
    MOZ_NOT_REACHED("unhandled program type");
  }
}




class TextureImageTextureHostOGL : public TextureHost
                                 , public TextureSourceOGL
                                 , public TileIterator
{
public:
  TextureImageTextureHostOGL(gl::TextureImage* aTexImage = nullptr)
    : mTexture(aTexImage)
    , mGL(nullptr)
    , mIterating(false)
  {
    MOZ_COUNT_CTOR(TextureImageTextureHostOGL);
  }

  ~TextureImageTextureHostOGL();

  TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE
  {
    return this;
  }

  virtual TextureImageTextureHostOGL* AsTextureImageTextureHost() MOZ_OVERRIDE
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
                      TextureHost *aDest,
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

  gfx::IntSize GetSize() const MOZ_OVERRIDE;

  gl::ShaderProgramType GetShaderProgram() const MOZ_OVERRIDE
  {
    return GetProgramTypeForTexture(this);
  }

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

  nsIntRect GetTileRect() MOZ_OVERRIDE
  {
    return mTexture->GetTileRect();
  }

  size_t GetTileCount() MOZ_OVERRIDE
  {
    return mTexture->GetTileCount();
  }

  bool NextTile() MOZ_OVERRIDE
  {
    return mTexture->NextTile();
  }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "TextureImageTextureHostOGL"; }
#endif

protected:
  nsRefPtr<gl::TextureImage> mTexture;
  gl::GLContext* mGL;
  bool mIterating;
};













class YCbCrTextureHostOGL : public TextureHost
{
public:
  YCbCrTextureHostOGL()
    : mGL(nullptr)
  {
    MOZ_COUNT_CTOR(YCbCrTextureHostOGL);
    mYTexture  = new Channel;
    mCbTexture = new Channel;
    mCrTexture = new Channel;
    mFormat = gfx::FORMAT_YUV;
  }

  ~YCbCrTextureHostOGL()
  {
    MOZ_COUNT_DTOR(YCbCrTextureHostOGL);
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
      NS_WARNING("YCbCrTextureHost::GetSize called but no data has been set yet");
      return gfx::IntSize(0,0);
    }
    return mYTexture->GetSize();
  }

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "YCbCrTextureHostOGL"; }
#endif

private:
  RefPtr<Channel> mYTexture;
  RefPtr<Channel> mCbTexture;
  RefPtr<Channel> mCrTexture;
  gl::GLContext* mGL;
};

class SharedTextureHostOGL : public TextureHost
                           , public TextureSourceOGL
{
public:
  typedef gfxASurface::gfxContentType ContentType;
  typedef mozilla::gl::GLContext GLContext;
  typedef mozilla::gl::TextureImage TextureImage;

  SharedTextureHostOGL()
    : mGL(nullptr)
    , mTextureHandle(0)
    , mWrapMode(LOCAL_GL_CLAMP_TO_EDGE)
    , mSharedHandle(0)
    , mShareType(GLContext::SameProcess)
  {}

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual ~SharedTextureHostOGL()
  {
    if (mSharedHandle || mTextureHandle) {
      DeleteTextures();
    }
  }

  virtual GLuint GetTextureHandle()
  {
    return mTextureHandle;
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

  gl::ShaderProgramType GetShaderProgram() const MOZ_OVERRIDE
  {
    return mShaderProgram;
  }

  gfx::IntSize GetSize() const MOZ_OVERRIDE {
    return mSize;
  }

  void BindTexture(GLenum activetex) MOZ_OVERRIDE
  {
    MOZ_ASSERT(mGL);
    
    MOZ_ASSERT(activetex == LOCAL_GL_TEXTURE0);
  }
  void ReleaseTexture() {}
  GLuint GetTextureID() { return mTextureHandle; }
  ContentType GetContentType()
  {
    return (mFormat == gfx::FORMAT_B8G8R8A8) ?
             gfxASurface::CONTENT_COLOR_ALPHA :
             gfxASurface::CONTENT_COLOR;
  }

  virtual gfx3DMatrix GetTextureTransform() MOZ_OVERRIDE;

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "SharedTextureHostOGL"; }
#endif

protected:
  void DeleteTextures();

  gfx::IntSize mSize;
  nsRefPtr<gl::GLContext> mGL;
  GLuint mTextureHandle;
  GLenum mWrapMode;
  GLenum mTextureTarget;
  gl::SharedTextureHandle mSharedHandle;
  gl::ShaderProgramType mShaderProgram;
  gl::GLContext::SharedTextureShareType mShareType;
};

class SurfaceStreamHostOGL : public TextureHost
                           , public TextureSourceOGL
{
public:
  typedef gfxASurface::gfxContentType ContentType;
  typedef mozilla::gl::GLContext GLContext;
  typedef mozilla::gl::TextureImage TextureImage;

  virtual ~SurfaceStreamHostOGL()
  {
    DeleteTextures();
    *mBuffer = SurfaceDescriptor();
  }

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual GLuint GetTextureHandle()
  {
    return mTextureHandle;
  }

  virtual TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE { return this; }

  bool IsValid() const MOZ_OVERRIDE { return true; }

  
  virtual void SwapTexturesImpl(const SurfaceDescriptor& aImage,
                                nsIntRegion* aRegion = nullptr) MOZ_OVERRIDE;
  virtual bool Lock() MOZ_OVERRIDE;
  virtual void Unlock() MOZ_OVERRIDE;

  virtual GLenum GetWrapMode() const MOZ_OVERRIDE {
    return mWrapMode;
  }
  virtual void SetWrapMode(GLenum aMode) {
    mWrapMode = aMode;
  }

  gl::ShaderProgramType GetShaderProgram() const MOZ_OVERRIDE
  {
    return mShaderProgram;
  }

  gfx::IntSize GetSize() const MOZ_OVERRIDE {
    return mSize;
  }

  virtual GLenum GetTextureTarget() const MOZ_OVERRIDE
  {
    return LOCAL_GL_TEXTURE_2D;
  }

  void BindTexture(GLenum activetex) MOZ_OVERRIDE {
    MOZ_ASSERT(mGL);
    mGL->fActiveTexture(activetex);
    mGL->fBindTexture(LOCAL_GL_TEXTURE_2D, mTextureHandle);
  }
  void ReleaseTexture() {
  }
  GLuint GetTextureID() { return mTextureHandle; }
  ContentType GetContentType() {
    return (mFormat == gfx::FORMAT_B8G8R8A8) ?
             gfxASurface::CONTENT_COLOR_ALPHA :
             gfxASurface::CONTENT_COLOR;
  }

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "SurfaceStreamHostOGL"; }
#endif

  SurfaceStreamHostOGL()
    : mGL(nullptr)
    , mTextureHandle(0)
    , mUploadTexture(0)
    , mWrapMode(LOCAL_GL_CLAMP_TO_EDGE)
  {}

protected:
  void DeleteTextures();

  gfx::IntSize mSize;
  nsRefPtr<GLContext> mGL;
  GLuint mTextureHandle;
  GLuint mUploadTexture;
  GLenum mWrapMode;
  gl::ShaderProgramType mShaderProgram;
};

class TiledTextureHostOGL : public TextureHost
                          , public TextureSourceOGL
{
public:
  TiledTextureHostOGL()
    : mTextureHandle(0)
    , mGL(nullptr)
  {}
  ~TiledTextureHostOGL();

  virtual void SetCompositor(Compositor* aCompositor);

  
  virtual void Update(gfxReusableSurfaceWrapper* aReusableSurface, TextureFlags aFlags, const gfx::IntSize& aSize) MOZ_OVERRIDE;
  virtual bool Lock() MOZ_OVERRIDE;
  virtual void Unlock() MOZ_OVERRIDE {}

  virtual TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE { return this; }
  virtual bool IsValid() const MOZ_OVERRIDE { return true; }
  virtual GLenum GetWrapMode() const MOZ_OVERRIDE { return LOCAL_GL_CLAMP_TO_EDGE; }
  virtual void BindTexture(GLenum aTextureUnit) MOZ_OVERRIDE
  {
    mGL->fActiveTexture(aTextureUnit);
    mGL->fBindTexture(LOCAL_GL_TEXTURE_2D, mTextureHandle);
  }
  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE
  {
    return mSize;
  }

  gl::ShaderProgramType GetShaderProgram() const MOZ_OVERRIDE
  {
    return GetProgramTypeForTexture(this);
  }

  virtual void SwapTexturesImpl(const SurfaceDescriptor& aImage,
                                nsIntRegion* aRegion = nullptr)
  { MOZ_ASSERT(false, "Tiles should not use this path"); }

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "TiledTextureHostOGL"; }
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
  gl::GLContext* mGL;
};

#ifdef MOZ_WIDGET_GONK





class GrallocTextureHostOGL
  : public TextureHost
  , public TextureSourceOGL
{
public:
  GrallocTextureHostOGL();

  ~GrallocTextureHostOGL();

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

  gl::ShaderProgramType GetShaderProgram() const MOZ_OVERRIDE
  {
    if (mTextureTarget == LOCAL_GL_TEXTURE_EXTERNAL) {
      return gl::RGBAExternalLayerProgramType;
    }
    MOZ_ASSERT(mTextureTarget == LOCAL_GL_TEXTURE_2D);
    return mFormat == gfx::FORMAT_B8G8R8A8 || mFormat == gfx::FORMAT_B8G8R8X8
           ? gl::BGRALayerProgramType
           : gl::RGBALayerProgramType;
  }

  GLenum GetWrapMode() const MOZ_OVERRIDE
  {
    return LOCAL_GL_CLAMP_TO_EDGE;
  }

  bool IsValid() const MOZ_OVERRIDE;

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "GrallocTextureHostOGL"; }
#endif

  void BindTexture(GLenum aTextureUnit) MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const;

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

private:
  gl::GLContext* gl() const;

  void DeleteTextures();

  RefPtr<CompositorOGL> mCompositor;
  android::sp<android::GraphicBuffer> mGraphicBuffer;
  GLenum mTextureTarget;
  EGLImage mEGLImage;
};
#endif

} 
} 

#endif 
