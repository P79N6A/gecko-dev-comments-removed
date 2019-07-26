




#ifndef MOZILLA_GFX_MACIOSURFACETEXTUREHOSTOGL_H
#define MOZILLA_GFX_MACIOSURFACETEXTUREHOSTOGL_H

#include "mozilla/layers/TextureHostOGL.h"

class MacIOSurface;

namespace mozilla {
namespace layers {







class MacIOSurfaceTextureSourceOGL : public NewTextureSource
                                   , public TextureSourceOGL
{
public:
  MacIOSurfaceTextureSourceOGL(CompositorOGL* aCompositor,
                               MacIOSurface* aSurface);
  virtual ~MacIOSurfaceTextureSourceOGL();

  virtual TextureSourceOGL* AsSourceOGL() { return this; }

  virtual void BindTexture(GLenum activetex) MOZ_OVERRIDE;

  virtual bool IsValid() const MOZ_OVERRIDE { return !!gl(); }

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  virtual GLenum GetTextureTarget() const { return LOCAL_GL_TEXTURE_RECTANGLE_ARB; }

  virtual GLenum GetWrapMode() const MOZ_OVERRIDE { return LOCAL_GL_CLAMP_TO_EDGE; }

  virtual void UnbindTexture() MOZ_OVERRIDE {}

  
  virtual void DeallocateDeviceData() {}

  void SetCompositor(CompositorOGL* aCompositor) {
    mCompositor = aCompositor;
  }

  gl::GLContext* gl() const;

protected:
  CompositorOGL* mCompositor;
  RefPtr<MacIOSurface> mSurface;
};






class MacIOSurfaceTextureHostOGL : public TextureHost
{
public:
  MacIOSurfaceTextureHostOGL(uint64_t aID,
                             TextureFlags aFlags,
                             const SurfaceDescriptorMacIOSurface& aDescriptor);

  
  virtual void DeallocateDeviceData() MOZ_OVERRIDE {}

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual bool Lock() MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  virtual NewTextureSource* GetTextureSources() MOZ_OVERRIDE
  {
    return mTextureSource;
  }

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE
  {
    return nullptr; 
  }

  gl::GLContext* gl() const;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "MacIOSurfaceTextureHostOGL"; }
#endif

protected:
  CompositorOGL* mCompositor;
  RefPtr<MacIOSurfaceTextureSourceOGL> mTextureSource;
  RefPtr<MacIOSurface> mSurface;
};

}
}

#endif 