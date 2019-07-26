




#ifndef GFX_CANVASLAYERD3D9_H
#define GFX_CANVASLAYERD3D9_H

#include "LayerManagerD3D9.h"
#include "GLContextTypes.h"

class gfxASurface;

namespace mozilla {
namespace layers {


class CanvasLayerD3D9 :
  public CanvasLayer,
  public LayerD3D9
{
public:
  CanvasLayerD3D9(LayerManagerD3D9 *aManager);
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
  RefPtr<gfx::DrawTarget> mDrawTarget;

  bool mDataIsPremultiplied;
  bool mNeedsYFlip;
  bool mHasAlpha;

  nsAutoArrayPtr<uint8_t> mCachedTempBlob;
  uint32_t mCachedTempBlob_Size;

  uint8_t* GetTempBlob(const uint32_t aSize)
  {
      if (!mCachedTempBlob || aSize != mCachedTempBlob_Size) {
          mCachedTempBlob = new uint8_t[aSize];
          mCachedTempBlob_Size = aSize;
      }

      return mCachedTempBlob;
  }

  void DiscardTempBlob()
  {
      mCachedTempBlob = nullptr;
  }
};

} 
} 
#endif 
