




#ifndef MOZILLA_GFX_TEXTURECLIENTOGL_H
#define MOZILLA_GFX_TEXTURECLIENTOGL_H

#include "GLContextTypes.h"             
#include "gfxASurface.h"                
#include "mozilla/Attributes.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/TextureClient.h"  

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
};


class DeprecatedTextureClientSharedOGL : public DeprecatedTextureClient
{
public:
  DeprecatedTextureClientSharedOGL(CompositableForwarder* aForwarder, const TextureInfo& aTextureInfo);
  ~DeprecatedTextureClientSharedOGL() { ReleaseResources(); }

  virtual bool SupportsType(DeprecatedTextureClientType aType) MOZ_OVERRIDE { return aType == TEXTURE_SHARED_GL; }
  virtual bool EnsureAllocated(gfx::IntSize aSize, gfxContentType aType);
  virtual void ReleaseResources();
  virtual gfxContentType GetContentType() MOZ_OVERRIDE { return GFX_CONTENT_COLOR_ALPHA; }

protected:
  gl::GLContext* mGL;
  gfx::IntSize mSize;

  friend class CompositingFactory;
};


class DeprecatedTextureClientSharedOGLExternal : public DeprecatedTextureClientSharedOGL
{
public:
  DeprecatedTextureClientSharedOGLExternal(CompositableForwarder* aForwarder, const TextureInfo& aTextureInfo)
    : DeprecatedTextureClientSharedOGL(aForwarder, aTextureInfo)
  {}

  virtual bool SupportsType(DeprecatedTextureClientType aType) MOZ_OVERRIDE { return aType == TEXTURE_SHARED_GL_EXTERNAL; }
  virtual void ReleaseResources() {}
};

class DeprecatedTextureClientStreamOGL : public DeprecatedTextureClient
{
public:
  DeprecatedTextureClientStreamOGL(CompositableForwarder* aForwarder, const TextureInfo& aTextureInfo)
    : DeprecatedTextureClient(aForwarder, aTextureInfo)
  {}
  ~DeprecatedTextureClientStreamOGL() { ReleaseResources(); }

  virtual bool SupportsType(DeprecatedTextureClientType aType) MOZ_OVERRIDE { return aType == TEXTURE_STREAM_GL; }
  virtual bool EnsureAllocated(gfx::IntSize aSize, gfxContentType aType) { return true; }
  virtual void ReleaseResources() { mDescriptor = SurfaceDescriptor(); }
  virtual gfxContentType GetContentType() MOZ_OVERRIDE { return GFX_CONTENT_COLOR_ALPHA; }
};

} 
} 

#endif
