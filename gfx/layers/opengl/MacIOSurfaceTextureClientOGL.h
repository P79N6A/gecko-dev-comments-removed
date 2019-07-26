




#ifndef MOZILLA_GFX_MACIOSURFACETEXTURECLIENTOGL_H
#define MOZILLA_GFX_MACIOSURFACETEXTURECLIENTOGL_H

#include "mozilla/layers/TextureClientOGL.h"

class MacIOSurface;

namespace mozilla {
namespace layers {

class MacIOSurfaceTextureClientOGL : public TextureClient
{
public:
  MacIOSurfaceTextureClientOGL(TextureFlags aFlags);

  virtual ~MacIOSurfaceTextureClientOGL();

  void InitWith(MacIOSurface* aSurface);

  virtual bool IsAllocated() const MOZ_OVERRIDE { return !!mSurface; }

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const;

  virtual TextureClientData* DropTextureData() MOZ_OVERRIDE;

protected:
  RefPtr<MacIOSurface> mSurface;
};

}
}

#endif 