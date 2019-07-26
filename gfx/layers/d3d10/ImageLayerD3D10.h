




#ifndef GFX_IMAGELAYERD3D10_H
#define GFX_IMAGELAYERD3D10_H

#include "LayerManagerD3D10.h"
#include "ImageLayers.h"
#include "ImageContainer.h"
#include "yuv_convert.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

class ImageLayerD3D10 : public ImageLayer,
                        public LayerD3D10
{
public:
  ImageLayerD3D10(LayerManagerD3D10 *aManager)
    : ImageLayer(aManager, nullptr)
    , LayerD3D10(aManager)
  {
    mImplData = static_cast<LayerD3D10*>(this);
  }

  
  virtual Layer* GetLayer();

  virtual void RenderLayer();

  void AllocateTexturesYCbCr(PlanarYCbCrImage *aImage);

  virtual already_AddRefed<ID3D10ShaderResourceView> GetAsTexture(gfxIntSize* aSize);

private:
 ID3D10ShaderResourceView* GetImageSRView(Image* aImage, bool& aHasAlpha, IDXGIKeyedMutex **aMutex = nullptr);
};

struct PlanarYCbCrD3D10BackendData : public ImageBackendData
{
  nsRefPtr<ID3D10Texture2D> mYTexture;
  nsRefPtr<ID3D10Texture2D> mCrTexture;
  nsRefPtr<ID3D10Texture2D> mCbTexture;
  nsRefPtr<ID3D10ShaderResourceView> mYView;
  nsRefPtr<ID3D10ShaderResourceView> mCbView;
  nsRefPtr<ID3D10ShaderResourceView> mCrView;
};

struct TextureD3D10BackendData : public ImageBackendData
{
  nsRefPtr<ID3D10Texture2D> mTexture;
  nsRefPtr<ID3D10ShaderResourceView> mSRView;
};

class RemoteDXGITextureImage : public Image {
public:
  RemoteDXGITextureImage() : Image(nullptr, REMOTE_IMAGE_DXGI_TEXTURE) {}

  already_AddRefed<gfxASurface> GetAsSurface();

  IntSize GetSize() { return mSize; }

  TextureD3D10BackendData *GetD3D10TextureBackendData(ID3D10Device *aDevice);

  IntSize mSize;
  RemoteImageData::Format mFormat;
  HANDLE mHandle;
};

} 
} 
#endif 
