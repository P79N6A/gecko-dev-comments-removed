





































#ifndef GFX_CANVASLAYEROGL_H
#define GFX_CANVASLAYEROGL_H

#include "LayerManagerD3D9.h"
#include "GLContext.h"
#include "gfxASurface.h"

namespace mozilla {
namespace layers {

class THEBES_API CanvasLayerD3D9 :
  public CanvasLayer,
  public LayerD3D9
{
public:
  CanvasLayerD3D9(LayerManagerD3D9 *aManager)
    : CanvasLayer(aManager, NULL),
      LayerD3D9(aManager),
      mTexture(0),
      mGLBufferIsPremultiplied(PR_FALSE),
      mNeedsYFlip(PR_FALSE)
  {
      mImplData = static_cast<LayerD3D9*>(this);
  }

  ~CanvasLayerD3D9();

  
  virtual void Initialize(const Data& aData);
  virtual void Updated(const nsIntRect& aRect);

  
  virtual LayerType GetType();
  virtual Layer* GetLayer();
  virtual void RenderLayer();

protected:
  typedef mozilla::gl::GLContext GLContext;

  nsRefPtr<gfxASurface> mSurface;
  nsRefPtr<GLContext> mGLContext;
  nsRefPtr<IDirect3DTexture9> mTexture;

  nsIntRect mBounds;

  PRPackedBool mGLBufferIsPremultiplied;
  PRPackedBool mNeedsYFlip;
};

} 
} 
#endif 
