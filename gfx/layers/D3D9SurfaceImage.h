




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
    Data(IDirect3DSurface9* aSurface, const nsIntSize& aSize)
      : mSurface(aSurface), mSize(aSize) {}
    RefPtr<IDirect3DSurface9> mSurface;
    nsIntSize mSize;
  };

  D3D9SurfaceImage() : Image(NULL, D3D9_RGB32_TEXTURE), mSize(0, 0) {}
  virtual ~D3D9SurfaceImage() {}

  
  
  HRESULT SetData(const Data& aData);

  
  const D3DSURFACE_DESC& GetDesc() const;

  
  
  
  
  HANDLE GetShareHandle();

  gfxIntSize GetSize() MOZ_OVERRIDE;

  already_AddRefed<gfxASurface> GetAsSurface() MOZ_OVERRIDE;

private:

  
  
  void EnsureSynchronized();

  gfxIntSize mSize;
  RefPtr<IDirect3DTexture9> mTexture;
  RefPtr<IDirect3DQuery9> mQuery;
  HANDLE mShareHandle;
  D3DSURFACE_DESC mDesc;
};

} 
} 

#endif 
