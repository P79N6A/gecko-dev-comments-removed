




































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

  
  virtual LayerType GetType() { return TYPE_CANVAS; }
  virtual Layer* GetLayer() { return this; }
  virtual void RenderLayer(int aPreviousDestination);

protected:
  nsRefPtr<gfxASurface> mSurface;
  nsRefPtr<GLContext> mGLContext;

  unsigned int mTexture;

  nsIntRect mBounds;
  nsIntRect mUpdatedRect;

  PRPackedBool mGLBufferIsPremultiplied;
  PRPackedBool mNeedsYFlip;
};

} 
} 
#endif 
