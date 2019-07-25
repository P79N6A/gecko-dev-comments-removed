




































#ifndef GFX_CANVASLAYEROGL_H
#define GFX_CANVASLAYEROGL_H

#include "LayerManagerOGL.h"
#include "gfxASurface.h"

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
      mTexture(0)
  { 
      mImplData = static_cast<LayerOGL*>(this);
  }

  ~CanvasLayerOGL();

  
  virtual void Initialize(const Data& aData);
  virtual void Updated(const nsIntRect& aRect);

  
  virtual Layer* GetLayer() { return this; }
  virtual void RenderLayer(int aPreviousFrameBuffer,
                           const nsIntPoint& aOffset);

protected:
  nsRefPtr<gfxASurface> mCanvasSurface;
  nsRefPtr<GLContext> mCanvasGLContext;

  void MakeTexture();
  GLuint mTexture;

  nsIntRect mBounds;
  nsIntRect mUpdatedRect;

  PRPackedBool mGLBufferIsPremultiplied;
  PRPackedBool mNeedsYFlip;
};

} 
} 
#endif 
