




#ifndef MOZILLA_GFX_TEXTURECLIENTOGL_H
#define MOZILLA_GFX_TEXTURECLIENTOGL_H

#include "mozilla/layers/TextureClient.h"
#include "ISurfaceAllocator.h" 

namespace mozilla {
namespace layers {

class TextureClientSharedOGL : public TextureClient
{
public:
  TextureClientSharedOGL(CompositableForwarder* aForwarder, CompositableType aCompositableType);
  ~TextureClientSharedOGL() { ReleaseResources(); }

  virtual bool SupportsType(TextureClientType aType) MOZ_OVERRIDE { return aType == TEXTURE_SHARED_GL; }
  virtual void EnsureAllocated(gfx::IntSize aSize, gfxASurface::gfxContentType aType);
  virtual void ReleaseResources();
  virtual gfxASurface::gfxContentType GetContentType() MOZ_OVERRIDE { return gfxASurface::CONTENT_COLOR_ALPHA; }

protected:
  gl::GLContext* mGL;
  gfx::IntSize mSize;

  friend class CompositingFactory;
};


class TextureClientSharedOGLExternal : public TextureClientSharedOGL
{
public:
  TextureClientSharedOGLExternal(CompositableForwarder* aForwarder, CompositableType aCompositableType)
    : TextureClientSharedOGL(aForwarder, aCompositableType)
  {}

  virtual bool SupportsType(TextureClientType aType) MOZ_OVERRIDE { return aType == TEXTURE_SHARED_GL_EXTERNAL; }
  virtual void ReleaseResources() {}
};

class TextureClientStreamOGL : public TextureClient
{
public:
  TextureClientStreamOGL(CompositableForwarder* aForwarder, CompositableType aCompositableType)
    : TextureClient(aForwarder, aCompositableType)
  {}
  ~TextureClientStreamOGL() { ReleaseResources(); }

  virtual bool SupportsType(TextureClientType aType) MOZ_OVERRIDE { return aType == TEXTURE_STREAM_GL; }
  virtual void EnsureAllocated(gfx::IntSize aSize, gfxASurface::gfxContentType aType) { }
  virtual void ReleaseResources() { mDescriptor = SurfaceDescriptor(); }
  virtual gfxASurface::gfxContentType GetContentType() MOZ_OVERRIDE { return gfxASurface::CONTENT_COLOR_ALPHA; }
};

} 
} 

#endif
