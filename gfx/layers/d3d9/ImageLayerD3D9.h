




































#ifndef GFX_IMAGELAYERD3D9_H
#define GFX_IMAGELAYERD3D9_H

#include "LayerManagerD3D9.h"
#include "ImageLayers.h"
#include "yuv_convert.h"

namespace mozilla {
namespace layers {

class THEBES_API ImageContainerD3D9 : public ImageContainer
{
public:
  ImageContainerD3D9(IDirect3DDevice9 *aDevice);
  virtual ~ImageContainerD3D9() {}

  virtual already_AddRefed<Image> CreateImage(const Image::Format* aFormats,
                                              PRUint32 aNumFormats);

  virtual void SetCurrentImage(Image* aImage);

  virtual already_AddRefed<Image> GetCurrentImage();

  virtual already_AddRefed<gfxASurface> GetCurrentAsSurface(gfxIntSize* aSize);

  virtual gfxIntSize GetCurrentSize();

  virtual PRBool SetLayerManager(LayerManager *aManager);

  virtual LayerManager::LayersBackend GetBackendType() { return LayerManager::LAYERS_D3D9; }

  IDirect3DDevice9 *device() { return mDevice; }
  void SetDevice(IDirect3DDevice9 *aDevice) { mDevice = aDevice; }

private:
  nsRefPtr<Image> mActiveImage;

  nsRefPtr<IDirect3DDevice9> mDevice;
};

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

class THEBES_API PlanarYCbCrImageD3D9 : public PlanarYCbCrImage,
                                        public ImageD3D9
{
public:
  PlanarYCbCrImageD3D9();
  ~PlanarYCbCrImageD3D9() {}

  virtual void SetData(const Data &aData);

  



  void AllocateTextures(IDirect3DDevice9 *aDevice);
  






  void FreeTextures();
  PRBool HasData() { return mHasData; }

  virtual already_AddRefed<gfxASurface> GetAsSurface();

  nsAutoArrayPtr<PRUint8> mBuffer;
  LayerManagerD3D9 *mManager;
  Data mData;
  gfxIntSize mSize;
  nsRefPtr<IDirect3DTexture9> mYTexture;
  nsRefPtr<IDirect3DTexture9> mCrTexture;
  nsRefPtr<IDirect3DTexture9> mCbTexture;
  PRPackedBool mHasData;
};


class THEBES_API CairoImageD3D9 : public CairoImage,
                                  public ImageD3D9
{
public:
  CairoImageD3D9(IDirect3DDevice9 *aDevice)
    : CairoImage(static_cast<ImageD3D9*>(this))
    , mDevice(aDevice)
  { }
  ~CairoImageD3D9();

  virtual void SetData(const Data &aData);

  virtual already_AddRefed<gfxASurface> GetAsSurface();

  IDirect3DDevice9 *device() { return mDevice; }
  void SetDevice(IDirect3DDevice9 *aDevice);

  



  virtual IDirect3DTexture9* GetOrCreateTexture();
  const gfxIntSize& GetSize() { return mSize; }

  bool HasAlpha() {
    return mCachedSurface->GetContentType() ==
      gfxASurface::CONTENT_COLOR_ALPHA;
  }

private:
  gfxIntSize mSize;
  nsRefPtr<gfxASurface> mCachedSurface;
  nsRefPtr<IDirect3DDevice9> mDevice;
  nsRefPtr<IDirect3DTexture9> mTexture;
  LayerManagerD3D9 *mManager;
};

} 
} 
#endif 
