




#ifndef MOZILLA_GFX_MACIOSURFACETEXTURECLIENTOGL_H
#define MOZILLA_GFX_MACIOSURFACETEXTURECLIENTOGL_H

#include "mozilla/layers/TextureClientOGL.h"

class MacIOSurface;

namespace mozilla {
namespace layers {

class MacIOSurfaceTextureClientOGL : public TextureClient
{
public:
  explicit MacIOSurfaceTextureClientOGL(ISurfaceAllocator* aAllcator,
                                        TextureFlags aFlags);

  virtual ~MacIOSurfaceTextureClientOGL();

  
  static already_AddRefed<MacIOSurfaceTextureClientOGL>
  Create(ISurfaceAllocator* aAllocator,
         TextureFlags aFlags,
         MacIOSurface* aSurface);

  virtual bool Lock(OpenMode aMode) override;

  virtual void Unlock() override;

  virtual bool IsLocked() const override;

  virtual bool IsAllocated() const override { return !!mSurface; }

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) override;

  virtual gfx::IntSize GetSize() const override;

  virtual bool HasInternalBuffer() const override { return false; }

  virtual already_AddRefed<gfx::DataSourceSurface> GetAsSurface() override;

  
  
  
  virtual already_AddRefed<TextureClient>
  CreateSimilar(TextureFlags, TextureAllocationFlags) const override { return nullptr; }

protected:
  RefPtr<MacIOSurface> mSurface;
  bool mIsLocked;
};

} 
} 

#endif 
