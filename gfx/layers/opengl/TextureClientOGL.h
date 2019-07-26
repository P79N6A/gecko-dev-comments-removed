




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
namespace gfx {
class SurfaceStream;
}
}

namespace mozilla {
namespace layers {

class CompositableForwarder;





class SharedTextureClientOGL : public TextureClient
{
public:
  SharedTextureClientOGL(TextureFlags aFlags);

  ~SharedTextureClientOGL();

  virtual bool IsAllocated() const MOZ_OVERRIDE;

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  virtual bool Lock(OpenMode mode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool IsLocked() const MOZ_OVERRIDE { return mIsLocked; }

  void InitWith(gl::SharedTextureHandle aHandle,
                gfx::IntSize aSize,
                gl::SharedTextureShareType aShareType,
                bool aInverted = false);

  virtual gfx::IntSize GetSize() const { return mSize; }

  virtual TextureClientData* DropTextureData() MOZ_OVERRIDE
  {
    
    
    
    MarkInvalid();
    return nullptr;
  }

protected:
  gl::SharedTextureHandle mHandle;
  gfx::IntSize mSize;
  gl::SharedTextureShareType mShareType;
  bool mInverted;
  bool mIsLocked;
};




class StreamTextureClientOGL : public TextureClient
{
public:
  StreamTextureClientOGL(TextureFlags aFlags);

  ~StreamTextureClientOGL();

  virtual bool IsAllocated() const MOZ_OVERRIDE;

  virtual bool Lock(OpenMode mode) MOZ_OVERRIDE;

  virtual void Unlock() MOZ_OVERRIDE;

  virtual bool IsLocked() const MOZ_OVERRIDE { return mIsLocked; }

  virtual bool ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor) MOZ_OVERRIDE;

  virtual TextureClientData* DropTextureData() MOZ_OVERRIDE { return nullptr; }

  void InitWith(gfx::SurfaceStream* aStream);

  virtual gfx::IntSize GetSize() const { return gfx::IntSize(); }

protected:
  gfx::SurfaceStream* mStream;
  bool mIsLocked;
};

} 
} 

#endif
