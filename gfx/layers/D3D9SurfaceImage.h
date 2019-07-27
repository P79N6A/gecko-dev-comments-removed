




#ifndef GFX_D3DSURFACEIMAGE_H
#define GFX_D3DSURFACEIMAGE_H

#include "mozilla/RefPtr.h"
#include "ImageContainer.h"
#include "nsAutoPtr.h"
#include "d3d9.h"

namespace mozilla {
namespace layers {





class D3D9SurfaceImage : public Image {
public:

  struct Data {
    Data(IDirect3DSurface9* aSurface, const gfx::IntRect& aRegion)
      : mSurface(aSurface), mRegion(aRegion) {}
    RefPtr<IDirect3DSurface9> mSurface;
    gfx::IntRect mRegion;
  };

  D3D9SurfaceImage();
  virtual ~D3D9SurfaceImage();

  
  
  HRESULT SetData(const Data& aData);

  
  const D3DSURFACE_DESC& GetDesc() const;

  gfx::IntSize GetSize() override;

  virtual already_AddRefed<gfx::SourceSurface> GetAsSourceSurface() override;

  virtual TextureClient* GetTextureClient(CompositableClient* aClient) override;

  virtual bool IsValid() override;

private:

  
  
  void EnsureSynchronized();

  gfx::IntSize mSize;
  RefPtr<IDirect3DTexture9> mTexture;
  RefPtr<IDirect3DQuery9> mQuery;
  RefPtr<TextureClient> mTextureClient;
  HANDLE mShareHandle;
  D3DSURFACE_DESC mDesc;
  bool mValid;
};

} 
} 

#endif 
