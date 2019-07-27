




#ifndef MOZILLA_GFX_TEXTURECLIENTOGL_H
#define MOZILLA_GFX_TEXTURECLIENTOGL_H

#include "GLContextTypes.h"             
#include "GLImages.h"
#include "gfxTypes.h"
#include "mozilla/Attributes.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/TextureClient.h"  
#include "AndroidSurfaceTexture.h"

namespace mozilla {

namespace layers {

class CompositableForwarder;

class EGLImageTextureClient : public TextureClient
{
public:
  EGLImageTextureClient(ISurfaceAllocator* aAllocator,
                        TextureFlags aFlags,
                        EGLImageImage* aImage,
                        gfx::IntSize aSize);

  virtual bool IsAllocated() const MOZ_OVERRIDE { return true; }

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return false; }

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  
  virtual bool Lock(OpenMode mode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool IsLocked() const MOZ_OVERRIDE { return mIsLocked; }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE
  {
    return gfx::SurfaceFormat::UNKNOWN;
  }

  virtual TemporaryRef<TextureClient>
  CreateSimilar(TextureFlags aFlags = TextureFlags::DEFAULT,
                TextureAllocationFlags aAllocFlags = ALLOC_DEFAULT) const MOZ_OVERRIDE
  {
    return nullptr;
  }

  virtual bool AllocateForSurface(gfx::IntSize aSize, TextureAllocationFlags aFlags) MOZ_OVERRIDE
  {
    return false;
  }

protected:
  RefPtr<EGLImageImage> mImage;
  const gfx::IntSize mSize;
  bool mIsLocked;
};

#ifdef MOZ_WIDGET_ANDROID

class SurfaceTextureClient : public TextureClient
{
public:
  SurfaceTextureClient(ISurfaceAllocator* aAllocator,
                       TextureFlags aFlags,
                       gl::AndroidSurfaceTexture* aSurfTex,
                       gfx::IntSize aSize,
                       gl::OriginPos aOriginPos);

  ~SurfaceTextureClient();

  virtual bool IsAllocated() const MOZ_OVERRIDE { return true; }

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return false; }

  virtual gfx::IntSize GetSize() const { return mSize; }

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  
  virtual bool Lock(OpenMode mode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool IsLocked() const MOZ_OVERRIDE { return mIsLocked; }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE
  {
    return gfx::SurfaceFormat::UNKNOWN;
  }

  virtual TemporaryRef<TextureClient>
  CreateSimilar(TextureFlags aFlags = TextureFlags::DEFAULT,
                TextureAllocationFlags aAllocFlags = ALLOC_DEFAULT) const MOZ_OVERRIDE
  {
    return nullptr;
  }

  virtual bool AllocateForSurface(gfx::IntSize aSize, TextureAllocationFlags aFlags) MOZ_OVERRIDE
  {
    return false;
  }

protected:
  const RefPtr<gl::AndroidSurfaceTexture> mSurfTex;
  const gfx::IntSize mSize;
  bool mIsLocked;
};

#endif 

} 
} 

#endif
