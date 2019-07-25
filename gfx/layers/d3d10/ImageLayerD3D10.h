




































#ifndef GFX_IMAGELAYERD3D10_H
#define GFX_IMAGELAYERD3D10_H

#include "LayerManagerD3D10.h"
#include "ImageLayers.h"
#include "yuv_convert.h"

namespace mozilla {
namespace layers {

class THEBES_API ImageContainerD3D10 : public ImageContainer
{
public:
  ImageContainerD3D10(ID3D10Device1 *aDevice);
  virtual ~ImageContainerD3D10() {}

  virtual already_AddRefed<Image> CreateImage(const Image::Format* aFormats,
                                              PRUint32 aNumFormats);

  virtual void SetCurrentImage(Image* aImage);

  virtual already_AddRefed<Image> GetCurrentImage();

  virtual already_AddRefed<gfxASurface> GetCurrentAsSurface(gfxIntSize* aSize);

  virtual gfxIntSize GetCurrentSize();

  virtual PRBool SetLayerManager(LayerManager *aManager);

  virtual LayerManager::LayersBackend GetBackendType() { return LayerManager::LAYERS_D3D10; }

  ID3D10Device1 *device() { return mDevice; }
  void SetDevice(ID3D10Device1 *aDevice) { mDevice = aDevice; }

private:
  nsRefPtr<Image> mActiveImage;
  nsRefPtr<ID3D10Device1> mDevice;
};

class THEBES_API ImageLayerD3D10 : public ImageLayer,
                                   public LayerD3D10
{
public:
  ImageLayerD3D10(LayerManagerD3D10 *aManager)
    : ImageLayer(aManager, NULL)
    , LayerD3D10(aManager)
  {
    mImplData = static_cast<LayerD3D10*>(this);
  }

  
  virtual Layer* GetLayer();

  virtual void RenderLayer();
};

class THEBES_API ImageD3D10
{
public:
  virtual already_AddRefed<gfxASurface> GetAsSurface() = 0;
};

class THEBES_API PlanarYCbCrImageD3D10 : public PlanarYCbCrImage,
                                         public ImageD3D10
{
public:
  PlanarYCbCrImageD3D10(ID3D10Device1 *aDevice);
  ~PlanarYCbCrImageD3D10() {}

  virtual void SetData(const Data &aData);

  



  void AllocateTextures();

  PRBool HasData() { return mHasData; }

  virtual already_AddRefed<gfxASurface> GetAsSurface();

  nsAutoArrayPtr<PRUint8> mBuffer;
  nsRefPtr<ID3D10Device1> mDevice;
  Data mData;
  gfxIntSize mSize;
  nsRefPtr<ID3D10Texture2D> mYTexture;
  nsRefPtr<ID3D10Texture2D> mCrTexture;
  nsRefPtr<ID3D10Texture2D> mCbTexture;
  nsRefPtr<ID3D10ShaderResourceView> mYView;
  nsRefPtr<ID3D10ShaderResourceView> mCbView;
  nsRefPtr<ID3D10ShaderResourceView> mCrView;
  PRPackedBool mHasData;
  gfx::YUVType mType; 
};


class THEBES_API CairoImageD3D10 : public CairoImage,
                                   public ImageD3D10
{
public:
  CairoImageD3D10(ID3D10Device1 *aDevice)
    : CairoImage(static_cast<ImageD3D10*>(this))
    , mDevice(aDevice)
    , mHasAlpha(true)
  { }
  ~CairoImageD3D10();

  virtual void SetData(const Data &aData);

  virtual already_AddRefed<gfxASurface> GetAsSurface();

  nsRefPtr<ID3D10Device1> mDevice;
  nsRefPtr<ID3D10Texture2D> mTexture;
  nsRefPtr<ID3D10ShaderResourceView> mSRView;
  gfxIntSize mSize;
  bool mHasAlpha;
};

} 
} 
#endif 
