




#ifndef MOZILLA_GFX_TEXTURECLIENT_SHAREDSURFACE_H
#define MOZILLA_GFX_TEXTURECLIENT_SHAREDSURFACE_H

#include <cstddef>                      
#include <stdint.h>                     
#include "GLContextTypes.h"             
#include "TextureClient.h"
#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersSurfaces.h"  

namespace mozilla {
namespace gl {
class GLContext;
class SharedSurface;
class SurfaceFactory;
}

namespace layers {

class SharedSurfaceTextureClient : public TextureClient
{
protected:
  const UniquePtr<gl::SharedSurface> mSurf;

  friend class gl::SurfaceFactory;

  SharedSurfaceTextureClient(ISurfaceAllocator* aAllocator, TextureFlags aFlags,
                             UniquePtr<gl::SharedSurface> surf,
                             gl::SurfaceFactory* factory);

  ~SharedSurfaceTextureClient();

public:
  virtual bool IsAllocated() const override { return true; }
  virtual bool Lock(OpenMode) override { return false; }
  virtual bool IsLocked() const override { return false; }
  virtual bool HasInternalBuffer() const override { return false; }

  virtual gfx::SurfaceFormat GetFormat() const override {
    return gfx::SurfaceFormat::UNKNOWN;
  }

  virtual already_AddRefed<TextureClient>
  CreateSimilar(TextureFlags, TextureAllocationFlags) const override {
    return nullptr;
  }

  virtual bool AllocateForSurface(gfx::IntSize,
                                  TextureAllocationFlags) override {
    MOZ_CRASH("Should never hit this.");
    return false;
  }

  virtual gfx::IntSize GetSize() const override;

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) override;

  gl::SharedSurface* Surf() const {
    return mSurf.get();
  }
};

} 
} 

#endif 
