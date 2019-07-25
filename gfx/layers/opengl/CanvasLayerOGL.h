




































#ifndef GFX_CANVASLAYEROGL_H
#define GFX_CANVASLAYEROGL_H

#include "mozilla/layers/PLayers.h"
#include "mozilla/layers/ShadowLayers.h"

#include "LayerManagerOGL.h"
#include "gfxASurface.h"
#if defined(MOZ_WIDGET_GTK2) && !defined(MOZ_PLATFORM_MAEMO)
#include "GLXLibrary.h"
#include "mozilla/X11Util.h"
#endif

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
      mTexture(0),
      mDelayedUpdates(PR_FALSE)
#if defined(MOZ_WIDGET_GTK2) && !defined(MOZ_PLATFORM_MAEMO)
      ,mPixmap(0)
#endif
  { 
      mImplData = static_cast<LayerOGL*>(this);
  }
  ~CanvasLayerOGL() { Destroy(); }

  
  virtual void Initialize(const Data& aData);

  
  virtual void Destroy();
  virtual Layer* GetLayer() { return this; }
  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);

protected:
  void UpdateSurface();

  nsRefPtr<gfxASurface> mCanvasSurface;
  nsRefPtr<GLContext> mCanvasGLContext;
  gl::ShaderProgramType mLayerProgram;

  void MakeTexture();
  GLuint mTexture;

  PRPackedBool mDelayedUpdates;
  PRPackedBool mGLBufferIsPremultiplied;
  PRPackedBool mNeedsYFlip;
#if defined(MOZ_WIDGET_GTK2) && !defined(MOZ_PLATFORM_MAEMO)
  GLXPixmap mPixmap;
#endif
};




class ShadowCanvasLayerOGL : public ShadowCanvasLayer,
                             public LayerOGL
{
  typedef gl::TextureImage TextureImage;

public:
  ShadowCanvasLayerOGL(LayerManagerOGL* aManager);
  virtual ~ShadowCanvasLayerOGL();

  
  virtual void Initialize(const Data& aData);
  virtual void Init(const SurfaceDescriptor& aNewFront, const nsIntSize& aSize, bool needYFlip);

  
  virtual void Updated(const nsIntRect&) {}

  
  virtual void Swap(const SurfaceDescriptor& aNewFront,
                    SurfaceDescriptor* aNewBack);

  virtual void DestroyFrontBuffer();

  virtual void Disconnect();

  
  void Destroy();
  Layer* GetLayer();
  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);

private:
  nsRefPtr<TextureImage> mTexImage;

  SurfaceDescriptor mDeadweight;
  PRPackedBool mNeedsYFlip;
};

} 
} 
#endif 
