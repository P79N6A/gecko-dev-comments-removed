




#ifndef GFX_D3DSURFACEIMAGE_H
#define GFX_D3DSURFACEIMAGE_H

#include "mozilla/RefPtr.h"
#include "ImageContainer.h"
#include "nsAutoPtr.h"
#include "d3d9.h"
#include "mozilla/gfx/Point.h"

namespace mozilla {
namespace layers {





class D3D9SurfaceImage : public Image {
public:

  struct Data {
    Data(IDirect3DSurface9* aSurface, const nsIntRect& aRegion)
      : mSurface(aSurface), mRegion(aRegion) {}
    RefPtr<IDirect3DSurface9> mSurface;
    nsIntRect mRegion;
  };

  D3D9SurfaceImage() : Image(nullptr, D3D9_RGB32_TEXTURE), mSize(0, 0) {}
  virtual ~D3D9SurfaceImage() {}

  
  
  HRESULT SetData(const Data& aData);

  
  const D3DSURFACE_DESC& GetDesc() const;

  
  
  
  
  HANDLE GetShareHandle();

  gfx::IntSize GetSize() MOZ_OVERRIDE;

  already_AddRefed<gfxASurface> GetAsSurface() MOZ_OVERRIDE;

private:

  
  
  void EnsureSynchronized();

  gfx::IntSize mSize;
  RefPtr<IDirect3DTexture9> mTexture;
  RefPtr<IDirect3DQuery9> mQuery;
  HANDLE mShareHandle;
  D3DSURFACE_DESC mDesc;
};

} 
} 

#endif 
