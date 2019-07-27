




#ifndef MOZILLA_GFX_MACIOSURFACETEXTURECLIENTOGL_H
#define MOZILLA_GFX_MACIOSURFACETEXTURECLIENTOGL_H

#include "mozilla/layers/TextureClientOGL.h"

class MacIOSurface;

namespace mozilla {
namespace layers {

class MacIOSurfaceTextureClientOGL : public TextureClient
{
public:
  explicit MacIOSurfaceTextureClientOGL(TextureFlags aFlags);

  virtual ~MacIOSurfaceTextureClientOGL();

  void InitWith(MacIOSurface* aSurface);

  virtual bool Lock(OpenMode aMode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool IsLocked() const MOZ_OVERRIDE;

  virtual bool IsAllocated() const MOZ_OVERRIDE { return !!mSurface; }

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const;

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return false; }

  
  
  
  virtual TemporaryRef<TextureClient>
  CreateSimilar(TextureFlags, TextureAllocationFlags) const MOZ_OVERRIDE { return nullptr; }

protected:
  RefPtr<MacIOSurface> mSurface;
  bool mIsLocked;
};

}
}

#endif 
