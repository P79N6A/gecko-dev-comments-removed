




































#ifndef GFX_SHADOWBUFFERD3D9_H
#define GFX_SHADOWBUFFERD3D9_H

#include "LayerManagerD3D9.h"

namespace mozilla {
namespace layers {

class LayerManagerD3D9;
class LayerD3D9;

class ShadowBufferD3D9
{
  NS_INLINE_DECL_REFCOUNTING(ShadowBufferD3D9)
public:

  ShadowBufferD3D9(LayerD3D9* aLayer)
    : mLayer(aLayer)
  {
  }
  virtual ~ShadowBufferD3D9() {}
  
  void Upload(gfxASurface* aUpdate, const nsIntRect& aVisibleRect);

  void RenderTo(LayerManagerD3D9 *aD3DManager, const nsIntRegion& aVisibleRegion);

  nsIntSize GetSize() {
    if (mTexture)
      return nsIntSize(mTextureRect.Width(), mTextureRect.Height());
    return nsIntSize(0, 0);
  }

  already_AddRefed<IDirect3DTexture9> GetTexture()
  {
    nsRefPtr<IDirect3DTexture9> result = mTexture;
    return result.forget();
  }
protected:
  nsRefPtr<IDirect3DTexture9> mTexture;
  nsIntRect mTextureRect;
  LayerD3D9* mLayer;
};

} 
} 
#endif 
