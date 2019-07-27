




#ifndef MOZILLA_GFX_TEXTUREDIB_H
#define MOZILLA_GFX_TEXTUREDIB_H

#include "mozilla/layers/Compositor.h"
#include "mozilla/layers/TextureClient.h"
#include "mozilla/layers/TextureHost.h"
#include "mozilla/GfxMessageUtils.h"
#include "gfxWindowsPlatform.h"

namespace mozilla {
namespace layers {






class DIBTextureClient : public TextureClient
{
public:
  DIBTextureClient(gfx::SurfaceFormat aFormat, TextureFlags aFlags);

  virtual ~DIBTextureClient();

  

  virtual bool IsAllocated() const MOZ_OVERRIDE { return !!mSurface; }

  virtual bool Lock(OpenMode aOpenMode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool IsLocked() const MOZ_OVERRIDE{ return mIsLocked; }

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }

  virtual bool CanExposeDrawTarget() const MOZ_OVERRIDE { return true; }

  virtual gfx::DrawTarget* BorrowDrawTarget() MOZ_OVERRIDE;

  virtual bool AllocateForSurface(gfx::IntSize aSize,
    TextureAllocationFlags aFlags = ALLOC_DEFAULT) MOZ_OVERRIDE;

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return true; }

protected:
  nsRefPtr<gfxWindowsSurface> mSurface;
  RefPtr<gfx::DrawTarget> mDrawTarget;
  gfx::IntSize mSize;
  gfx::SurfaceFormat mFormat;
  bool mIsLocked;
};

class DIBTextureHost : public TextureHost
{
public:
  DIBTextureHost(TextureFlags aFlags,
                 const SurfaceDescriptorDIB& aDescriptor);

  virtual NewTextureSource* GetTextureSources() MOZ_OVERRIDE;

  virtual void DeallocateDeviceData() MOZ_OVERRIDE;

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mFormat; }

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual bool Lock() MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual void Updated(const nsIntRegion* aRegion = nullptr) MOZ_OVERRIDE;

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE
  {
    return nullptr; 
  }

protected:
  nsRefPtr<gfxWindowsSurface> mSurface;
  RefPtr<DataTextureSource> mTextureSource;
  RefPtr<Compositor> mCompositor;
  gfx::SurfaceFormat mFormat;
  gfx::IntSize mSize;
  bool mIsLocked;
};

}
}

#endif 
