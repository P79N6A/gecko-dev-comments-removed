




#ifndef GFX_CANVASLAYEROGL_H
#define GFX_CANVASLAYEROGL_H


#include "LayerManagerOGL.h"
#include "gfxASurface.h"
#if defined(GL_PROVIDER_GLX)
#include "GLXLibrary.h"
#include "mozilla/X11Util.h"
#endif

#include "mozilla/Preferences.h"

namespace mozilla {
namespace layers {

class THEBES_API CanvasLayerOGL :
  public CanvasLayer,
  public LayerOGL
{
public:
  CanvasLayerOGL(LayerManagerOGL *aManager)
    : CanvasLayer(aManager, NULL),
      LayerOGL(aManager),
      mLayerProgram(gl::RGBALayerProgramType),
      mTexture(0),
      mTextureTarget(LOCAL_GL_TEXTURE_2D),
      mDelayedUpdates(false)
#if defined(GL_PROVIDER_GLX)
      ,mPixmap(0)
#endif
  { 
      mImplData = static_cast<LayerOGL*>(this);
      mForceReadback = Preferences::GetBool("webgl.force-layers-readback", false);
  }
  ~CanvasLayerOGL() { Destroy(); }

  
  virtual void Initialize(const Data& aData);

  
  virtual void Destroy();
  virtual Layer* GetLayer() { return this; }
  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
  virtual void CleanupResources();

protected:
  void UpdateSurface();

  nsRefPtr<gfxASurface> mCanvasSurface;
  nsRefPtr<GLContext> mCanvasGLContext;
  gl::ShaderProgramType mLayerProgram;
  RefPtr<gfx::DrawTarget> mDrawTarget;

  GLuint mTexture;
  GLenum mTextureTarget;

  bool mDelayedUpdates;
  bool mGLBufferIsPremultiplied;
  bool mNeedsYFlip;
  bool mForceReadback;
#if defined(GL_PROVIDER_GLX)
  GLXPixmap mPixmap;
#endif

  nsRefPtr<gfxImageSurface> mCachedTempSurface;
  gfxIntSize mCachedSize;
  gfxASurface::gfxImageFormat mCachedFormat;

  gfxImageSurface* GetTempSurface(const gfxIntSize& aSize,
                                  const gfxASurface::gfxImageFormat aFormat)
  {
    if (!mCachedTempSurface ||
        aSize.width != mCachedSize.width ||
        aSize.height != mCachedSize.height ||
        aFormat != mCachedFormat)
    {
      mCachedTempSurface = new gfxImageSurface(aSize, aFormat);
      mCachedSize = aSize;
      mCachedFormat = aFormat;
    }

    return mCachedTempSurface;
  }

  void DiscardTempSurface()
  {
    mCachedTempSurface = nullptr;
  }
};




class ShadowCanvasLayerOGL : public ShadowCanvasLayer,
                             public LayerOGL
{
  typedef gl::TextureImage TextureImage;

public:
  ShadowCanvasLayerOGL(LayerManagerOGL* aManager);
  virtual ~ShadowCanvasLayerOGL();

  
  virtual void Initialize(const Data& aData);
  virtual void Init(const CanvasSurface& aNewFront, bool needYFlip);

  
  virtual void Updated(const nsIntRect&) {}

  
  virtual void Swap(const CanvasSurface& aNewFront,
                    bool needYFlip,
                    CanvasSurface* aNewBack);

  virtual void DestroyFrontBuffer();

  virtual void Disconnect();

  
  void Destroy();
  Layer* GetLayer();
  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE;
  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);
  virtual void CleanupResources();

private:
  nsRefPtr<TextureImage> mTexImage;

  bool mNeedsYFlip;
  SurfaceDescriptor mFrontBufferDescriptor;
  GLuint mTexture;
};

} 
} 
#endif 
