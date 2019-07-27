




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
  DIBTextureClient(ISurfaceAllocator* aAllocator,
                   gfx::SurfaceFormat aFormat,
                   TextureFlags aFlags);

  virtual ~DIBTextureClient();

  

  virtual bool IsAllocated() const override { return !!mSurface; }

  virtual bool Lock(OpenMode aOpenMode) override;

  virtual void Unlock() override;

  virtual bool IsLocked() const override{ return mIsLocked; }

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) override;

  virtual gfx::IntSize GetSize() const override { return mSize; }

  virtual gfx::SurfaceFormat GetFormat() const override { return mFormat; }

  virtual bool CanExposeDrawTarget() const override { return true; }

  virtual gfx::DrawTarget* BorrowDrawTarget() override;

  virtual bool AllocateForSurface(gfx::IntSize aSize,
    TextureAllocationFlags aFlags = ALLOC_DEFAULT) override;

  virtual bool HasInternalBuffer() const override { return true; }

  virtual TemporaryRef<TextureClient>
  CreateSimilar(TextureFlags aFlags = TextureFlags::DEFAULT,
                TextureAllocationFlags aAllocFlags = ALLOC_DEFAULT) const override;

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

  virtual bool BindTextureSource(CompositableTextureSourceRef& aTexture) override;

  virtual void DeallocateDeviceData() override;

  virtual void SetCompositor(Compositor* aCompositor) override;

  virtual gfx::SurfaceFormat GetFormat() const override { return mFormat; }

  virtual gfx::IntSize GetSize() const override { return mSize; }

  virtual bool Lock() override;

  virtual void Unlock() override;

  virtual void Updated(const nsIntRegion* aRegion = nullptr) override;

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() override
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
