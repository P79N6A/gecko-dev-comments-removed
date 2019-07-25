





































#ifndef GFX_CANVASLAYERD3D10_H
#define GFX_CANVASLAYERD3D10_H

#include "LayerManagerD3D10.h"
#include "GLContext.h"
#include "gfxASurface.h"

namespace mozilla {
namespace layers {

class THEBES_API CanvasLayerD3D10 : public CanvasLayer,
                                    public LayerD3D10
{
public:
  CanvasLayerD3D10(LayerManagerD3D10 *aManager)
    : CanvasLayer(aManager, NULL)
    , LayerD3D10(aManager)
    , mDataIsPremultiplied(PR_FALSE)
    , mNeedsYFlip(PR_FALSE)
    , mHasAlpha(PR_TRUE)
  {
      mImplData = static_cast<LayerD3D10*>(this);
  }

  ~CanvasLayerD3D10();

  
  virtual void Initialize(const Data& aData);

  
  virtual Layer* GetLayer();
  virtual void RenderLayer();

private:
  typedef mozilla::gl::GLContext GLContext;

  void UpdateSurface();

  nsRefPtr<gfxASurface> mSurface;
  mozilla::RefPtr<mozilla::gfx::DrawTarget> mDrawTarget;
  nsRefPtr<GLContext> mGLContext;
  nsRefPtr<ID3D10Texture2D> mTexture;
  nsRefPtr<ID3D10ShaderResourceView> mSRView;

  PRUint32 mCanvasFramebuffer;

  PRPackedBool mDataIsPremultiplied;
  PRPackedBool mNeedsYFlip;
  PRPackedBool mIsD2DTexture;
  PRPackedBool mUsingSharedTexture;
  PRPackedBool mHasAlpha;
};

} 
} 
#endif 
