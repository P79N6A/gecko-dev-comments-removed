




#ifndef MOZILLA_GFX_GRALLOCTEXTUREHOST_H
#define MOZILLA_GFX_GRALLOCTEXTUREHOST_H
#ifdef MOZ_WIDGET_GONK

#include "mozilla/layers/TextureHostOGL.h"
#include "mozilla/layers/ShadowLayerUtilsGralloc.h"
#include <ui/GraphicBuffer.h>

namespace mozilla {
namespace layers {

class GrallocTextureHostOGL;

class GrallocTextureSourceOGL : public NewTextureSource
                              , public TextureSourceOGL
{
public:
  friend class GrallocTextureHostOGL;

  GrallocTextureSourceOGL(CompositorOGL* aCompositor,
                          android::GraphicBuffer* aGraphicBuffer,
                          gfx::SurfaceFormat aFormat);

  virtual ~GrallocTextureSourceOGL();

  virtual bool IsValid() const MOZ_OVERRIDE;

  virtual void BindTexture(GLenum aTextureUnit, gfx::Filter aFilter) MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;

  virtual TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE { return this; }

  virtual GLenum GetTextureTarget() const MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }

  virtual GLenum GetWrapMode() const MOZ_OVERRIDE
  {
    return LOCAL_GL_CLAMP_TO_EDGE;
  }

  virtual void SetCompositableBackendSpecificData(CompositableBackendSpecificData* aBackendData) MOZ_OVERRIDE;

  void DeallocateDeviceData();

  gl::GLContext* gl() const;

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  void ForgetBuffer()
  {
    mGraphicBuffer = nullptr;
  }

  TemporaryRef<gfx::DataSourceSurface> GetAsSurface();

  GLuint GetGLTexture();

  void Lock();

protected:
  CompositorOGL* mCompositor;
  android::sp<android::GraphicBuffer> mGraphicBuffer;
  EGLImage mEGLImage;
  GLuint mTexture;
  gfx::SurfaceFormat mFormat;
  bool mNeedsReset;
};

class GrallocTextureHostOGL : public TextureHost
#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
                            , public TextureHostOGL
#endif
{
  friend class GrallocBufferActor;
public:
  GrallocTextureHostOGL(TextureFlags aFlags,
                        const NewSurfaceDescriptorGralloc& aDescriptor);

  virtual ~GrallocTextureHostOGL();

  virtual void Updated(const nsIntRegion* aRegion) MOZ_OVERRIDE {}

  virtual bool Lock() MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual void DeallocateSharedData() MOZ_OVERRIDE;

  virtual void ForgetSharedData() MOZ_OVERRIDE;

  virtual void DeallocateDeviceData() MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE;

  virtual NewTextureSource* GetTextureSources() MOZ_OVERRIDE
  {
    return mTextureSource;
  }

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
  virtual TextureHostOGL* AsHostOGL() MOZ_OVERRIDE
  {
    return this;
  }
#endif

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE;

  virtual void SetCompositableBackendSpecificData(CompositableBackendSpecificData* aBackendData) MOZ_OVERRIDE;

  bool IsValid() const;

  virtual const char* Name() MOZ_OVERRIDE { return "GrallocTextureHostOGL"; }

private:
  NewSurfaceDescriptorGralloc mGrallocHandle;
  RefPtr<GrallocTextureSourceOGL> mTextureSource;
  gfx::IntSize mSize; 
};

} 
} 

#endif
#endif
