




#ifndef MOZILLA_GFX_TEXTURECLIENT_X11_H
#define MOZILLA_GFX_TEXTURECLIENT_X11_H

#include "mozilla/layers/TextureClient.h"
#include "ISurfaceAllocator.h" 
#include "mozilla/layers/ShadowLayerUtilsX11.h"

namespace mozilla {
namespace layers {




class TextureClientX11
 : public TextureClient,
   public TextureClientSurface,
   public TextureClientDrawTarget
{
 public:
  TextureClientX11(gfx::SurfaceFormat format, TextureFlags aFlags = TEXTURE_FLAGS_DEFAULT);
  ~TextureClientX11();

  

  TextureClientSurface* AsTextureClientSurface() MOZ_OVERRIDE { return this; }
  TextureClientDrawTarget* AsTextureClientDrawTarget() MOZ_OVERRIDE { return this; }

  bool IsAllocated() const MOZ_OVERRIDE;
  bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;
  TextureClientData* DropTextureData() MOZ_OVERRIDE;
  gfx::IntSize GetSize() const {
    return mSize;
  }

  bool Lock(OpenMode aMode) MOZ_OVERRIDE;
  void Unlock() MOZ_OVERRIDE;
  bool IsLocked() const MOZ_OVERRIDE { return mLocked; }

  

  bool UpdateSurface(gfxASurface* aSurface) MOZ_OVERRIDE;
  already_AddRefed<gfxASurface> GetAsSurface() MOZ_OVERRIDE;
  bool AllocateForSurface(gfx::IntSize aSize, TextureAllocationFlags flags) MOZ_OVERRIDE;

  

  TemporaryRef<gfx::DrawTarget> GetAsDrawTarget() MOZ_OVERRIDE;
  gfx::SurfaceFormat GetFormat() const {
    return mFormat;
  }

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
