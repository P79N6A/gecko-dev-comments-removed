




#ifndef MOZILLA_GFX_TEXTURECLIENT_X11_H
#define MOZILLA_GFX_TEXTURECLIENT_X11_H

#include "mozilla/layers/TextureClient.h"
#include "ISurfaceAllocator.h" 
#include "mozilla/layers/ShadowLayerUtilsX11.h"

namespace mozilla {
namespace layers {




class TextureClientX11
 : public TextureClient,
   public TextureClientSurface
{
 public:
  TextureClientX11(gfx::SurfaceFormat format, TextureFlags aFlags = TEXTURE_FLAGS_DEFAULT);

  ~TextureClientX11();

  

  virtual TextureClientSurface* AsTextureClientSurface() MOZ_OVERRIDE { return this; }

  virtual bool IsAllocated() const MOZ_OVERRIDE;

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  virtual TextureClientData* DropTextureData() MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE { return mSize; }

  virtual bool Lock(OpenMode aMode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool IsLocked() const MOZ_OVERRIDE { return mLocked; }

  virtual bool AllocateForSurface(gfx::IntSize aSize, TextureAllocationFlags flags) MOZ_OVERRIDE;

  virtual bool CanExposeDrawTarget() const MOZ_OVERRIDE { return true; }

  virtual TemporaryRef<gfx::DrawTarget> GetAsDrawTarget() MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const { return mFormat; }

  virtual bool UpdateSurface(gfxASurface* aSurface) MOZ_OVERRIDE;

  virtual already_AddRefed<gfxASurface> GetAsSurface() MOZ_OVERRIDE;

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return false; }

 private:
  gfx::SurfaceFormat mFormat;
  gfx::IntSize mSize;
  RefPtr<gfxXlibSurface> mSurface;
  bool mLocked;
};

} 
} 

#endif
