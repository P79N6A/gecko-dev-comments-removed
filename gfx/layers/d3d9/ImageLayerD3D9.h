




#ifndef GFX_IMAGELAYERD3D9_H
#define GFX_IMAGELAYERD3D9_H

#include "LayerManagerD3D9.h"
#include "ImageLayers.h"
#include "ImageContainer.h"
#include "yuv_convert.h"

namespace mozilla {
namespace layers {

class ImageLayerD3D9 : public ImageLayer,
                       public LayerD3D9
{
public:
  ImageLayerD3D9(LayerManagerD3D9 *aManager)
    : ImageLayer(aManager, nullptr)
    , LayerD3D9(aManager)
  {
    mImplData = static_cast<LayerD3D9*>(this);
  }

  
  virtual Layer* GetLayer();

  virtual void RenderLayer();

  virtual already_AddRefed<IDirect3DTexture9> GetAsTexture(gfx::IntSize* aSize);

private:
  IDirect3DTexture9* GetTexture(Image *aImage, bool& aHasAlpha);
};


struct TextureD3D9BackendData : public ImageBackendData
{
  nsRefPtr<IDirect3DTexture9> mTexture;
};

struct PlanarYCbCrD3D9BackendData : public ImageBackendData
{
  nsRefPtr<IDirect3DTexture9> mYTexture;
  nsRefPtr<IDirect3DTexture9> mCrTexture;
  nsRefPtr<IDirect3DTexture9> mCbTexture;
};

} 
} 
#endif 
