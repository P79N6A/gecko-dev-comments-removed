





































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
      mDataIsPremultiplied(PR_FALSE),
      mNeedsYFlip(PR_FALSE)
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

  void CreateTexture();

protected:
  typedef mozilla::gl::GLContext GLContext;

  
  bool mIsInteropTexture;

  nsRefPtr<gfxASurface> mSurface;
  nsRefPtr<GLContext> mGLContext;

  PRUint32 mCanvasFramebuffer;

  nsRefPtr<IDirect3DTexture9> mTexture;

  nsIntRect mBounds;

  PRPackedBool mDataIsPremultiplied;
  PRPackedBool mNeedsYFlip;
};

} 
} 
#endif 
