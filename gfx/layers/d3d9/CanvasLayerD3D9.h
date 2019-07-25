





































#ifndef GFX_CANVASLAYERD3D9_H
#define GFX_CANVASLAYERD3D9_H

#include "LayerManagerD3D9.h"
#include "GLContext.h"
#include "gfxASurface.h"

namespace mozilla {
namespace layers {

class ShadowBufferD3D9;

class THEBES_API CanvasLayerD3D9 :
  public CanvasLayer,
  public LayerD3D9
{
public:
  CanvasLayerD3D9(LayerManagerD3D9 *aManager)
    : CanvasLayer(aManager, NULL)
    , LayerD3D9(aManager)
    , mDataIsPremultiplied(false)
    , mNeedsYFlip(false)
    , mHasAlpha(true)
  {
      mImplData = static_cast<LayerD3D9*>(this);
      aManager->deviceManager()->mLayersWithResources.AppendElement(this);
  }

  ~CanvasLayerD3D9();

  
  virtual void Initialize(const Data& aData);

  
  virtual Layer* GetLayer();
  virtual void RenderLayer();
  virtual void CleanResources();
  virtual void LayerManagerDestroyed();

  void CreateTexture();

protected:
  typedef mozilla::gl::GLContext GLContext;

  void UpdateSurface();

  nsRefPtr<gfxASurface> mSurface;
  nsRefPtr<GLContext> mGLContext;
  nsRefPtr<IDirect3DTexture9> mTexture;

  PRUint32 mCanvasFramebuffer;

  bool mDataIsPremultiplied;
  bool mNeedsYFlip;
  bool mHasAlpha;
};




class ShadowCanvasLayerD3D9 : public ShadowCanvasLayer,
                             public LayerD3D9
{
public:
  ShadowCanvasLayerD3D9(LayerManagerD3D9* aManager);
  virtual ~ShadowCanvasLayerD3D9();

  
  virtual void Initialize(const Data& aData);
  
  virtual void Updated(const nsIntRect&) {}

  
  virtual void Swap(const CanvasSurface& aNewFront,
                    bool needYFlip,
                    CanvasSurface* aNewBack);
  virtual void DestroyFrontBuffer();
  virtual void Disconnect();

  virtual void Destroy();

  
  virtual Layer* GetLayer();
  virtual void RenderLayer();
  virtual void CleanResources();
  virtual void LayerManagerDestroyed();

private:
  virtual void Init(bool needYFlip);

  bool mNeedsYFlip;
  nsRefPtr<ShadowBufferD3D9> mBuffer;
};

} 
} 
#endif 
