





































#ifndef GFX_CANVASLAYERD3D9_H
#define GFX_CANVASLAYERD3D9_H

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
    : CanvasLayer(aManager, NULL)
    , LayerD3D9(aManager)
    , mDataIsPremultiplied(PR_FALSE)
    , mNeedsYFlip(PR_FALSE)
    , mHasAlpha(PR_TRUE)
  {
      mImplData = static_cast<LayerD3D9*>(this);
      aManager->deviceManager()->mLayersWithResources.AppendElement(this);
  }

  ~CanvasLayerD3D9();

  
  virtual void Initialize(const Data& aData);
  virtual void Updated(const nsIntRect& aRect);

  
  virtual Layer* GetLayer();
  virtual void RenderLayer();
  virtual void CleanResources();
  virtual void LayerManagerDestroyed();

  void CreateTexture();

protected:
  typedef mozilla::gl::GLContext GLContext;

  nsRefPtr<gfxASurface> mSurface;
  nsRefPtr<GLContext> mGLContext;
  nsRefPtr<IDirect3DTexture9> mTexture;

  PRUint32 mCanvasFramebuffer;

  PRPackedBool mDataIsPremultiplied;
  PRPackedBool mNeedsYFlip;
  PRPackedBool mHasAlpha;
};

} 
} 
#endif 
