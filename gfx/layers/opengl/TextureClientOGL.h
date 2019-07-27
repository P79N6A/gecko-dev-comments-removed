




#ifndef MOZILLA_GFX_TEXTURECLIENTOGL_H
#define MOZILLA_GFX_TEXTURECLIENTOGL_H

#include "GLContextTypes.h"             
#include "gfxTypes.h"
#include "mozilla/Attributes.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/TextureClient.h"  

namespace mozilla {
namespace gl {
class SurfaceStream;
}
}

namespace mozilla {
namespace layers {

class CompositableForwarder;





class SharedTextureClientOGL : public TextureClient
{
public:
  explicit SharedTextureClientOGL(TextureFlags aFlags);

  ~SharedTextureClientOGL();

  virtual bool IsAllocated() const MOZ_OVERRIDE;

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  virtual bool Lock(OpenMode mode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool IsLocked() const MOZ_OVERRIDE { return mIsLocked; }

  virtual bool HasInternalBuffer() const MOZ_OVERRIDE { return false; }

  void InitWith(gl::SharedTextureHandle aHandle,
                gfx::IntSize aSize,
                gl::SharedTextureShareType aShareType,
                bool aInverted = false);

  virtual gfx::IntSize GetSize() const { return mSize; }

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE
  {
    return gfx::SurfaceFormat::UNKNOWN;
  }

  
  
  
  virtual TemporaryRef<TextureClient>
  CreateSimilar(TextureFlags, TextureAllocationFlags) const MOZ_OVERRIDE { return nullptr; }

  virtual bool AllocateForSurface(gfx::IntSize aSize, TextureAllocationFlags aFlags) MOZ_OVERRIDE
  {
    return false;
  }

protected:
  gl::SharedTextureHandle mHandle;
  gfx::IntSize mSize;
  gl::SharedTextureShareType mShareType;
  bool mInverted;
  bool mIsLocked;
};

} 
} 

#endif
