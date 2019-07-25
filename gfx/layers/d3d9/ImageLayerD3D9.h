




































#ifndef GFX_IMAGELAYERD3D9_H
#define GFX_IMAGELAYERD3D9_H

#include "LayerManagerD3D9.h"
#include "ImageLayers.h"
#include "yuv_convert.h"

namespace mozilla {
namespace layers {

class ShadowBufferD3D9;

class THEBES_API ImageLayerD3D9 : public ImageLayer,
                                  public LayerD3D9
{
public:
  ImageLayerD3D9(LayerManagerD3D9 *aManager)
    : ImageLayer(aManager, NULL)
    , LayerD3D9(aManager)
  {
    mImplData = static_cast<LayerD3D9*>(this);
  }

  
  virtual Layer* GetLayer();

  virtual void RenderLayer();
};

class THEBES_API ImageD3D9
{
public:
  virtual already_AddRefed<gfxASurface> GetAsSurface() = 0;
};


struct CairoD3D9BackendData : public ImageBackendData
{
  nsRefPtr<IDirect3DTexture9> mTexture;
};

struct PlanarYCbCrD3D9BackendData : public ImageBackendData
{
  nsRefPtr<IDirect3DTexture9> mYTexture;
  nsRefPtr<IDirect3DTexture9> mCrTexture;
  nsRefPtr<IDirect3DTexture9> mCbTexture;
};

class ShadowImageLayerD3D9 : public ShadowImageLayer,
                            public LayerD3D9
{
public:
  ShadowImageLayerD3D9(LayerManagerD3D9* aManager);
  virtual ~ShadowImageLayerD3D9();

  
  virtual void Swap(const SharedImage& aFront,
                    SharedImage* aNewBack);

  virtual void Disconnect();

  
  virtual void Destroy();

  virtual Layer* GetLayer();

  virtual void RenderLayer();

private:
  nsRefPtr<ShadowBufferD3D9> mBuffer;
  nsRefPtr<PlanarYCbCrImage> mYCbCrImage;
};

} 
} 
#endif 
