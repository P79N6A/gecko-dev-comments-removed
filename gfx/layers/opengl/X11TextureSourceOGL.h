




#ifndef MOZILLA_GFX_X11TEXTURESOURCEOGL__H
#define MOZILLA_GFX_X11TEXTURESOURCEOGL__H

#ifdef GL_PROVIDER_GLX

#include "mozilla/layers/TextureHostOGL.h"
#include "mozilla/gfx/2D.h"

namespace mozilla {
namespace layers {


class X11TextureSourceOGL
  : public TextureSourceOGL,
    public NewTextureSource
{
public:
  X11TextureSourceOGL(CompositorOGL* aCompositor, gfxXlibSurface* aSurface);
  ~X11TextureSourceOGL();

  virtual X11TextureSourceOGL* AsSourceOGL() MOZ_OVERRIDE { return this; }

  virtual bool IsValid() const MOZ_OVERRIDE { return !!gl(); } ;
  virtual void BindTexture(GLenum aTextureUnit) MOZ_OVERRIDE;
  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;
  virtual GLenum GetTextureTarget() const MOZ_OVERRIDE {
    return LOCAL_GL_TEXTURE_2D;
  }
  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;
  virtual GLenum GetWrapMode() const MOZ_OVERRIDE {
     return LOCAL_GL_CLAMP_TO_EDGE;
  }

  virtual void DeallocateDeviceData() MOZ_OVERRIDE;

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  gl::GLContext* gl() const;
  static gfx::SurfaceFormat ContentTypeToSurfaceFormat(gfxContentType aType);

protected:
  CompositorOGL* mCompositor;
  nsRefPtr<gfxXlibSurface> mSurface;
  RefPtr<gfx::SourceSurface> mSourceSurface;
  GLuint mTexture;
};

} 
} 

#endif

#endif 
