




#ifndef GFX_D311_SHARE_HANDLE_IMAGE_H
#define GFX_D311_SHARE_HANDLE_IMAGE_H

#include "mozilla/RefPtr.h"
#include "ImageContainer.h"
#include "nsAutoPtr.h"
#include "d3d11.h"
#include "mozilla/layers/LayersSurfaces.h"

namespace mozilla {
namespace layers {





class D3D11ShareHandleImage : public Image {
public:

  struct Data {
    Data(ID3D11Texture2D* aTexture,
         ID3D11Device* aDevice,
         ID3D11DeviceContext* aContext,
         const nsIntRect& aRegion)
      : mTexture(aTexture),
        mDevice(aDevice),
        mContext(aContext),
        mRegion(aRegion) {}
    RefPtr<ID3D11Texture2D> mTexture;
    RefPtr<ID3D11Device> mDevice;
    RefPtr<ID3D11DeviceContext> mContext;
    nsIntRect mRegion;
  };

  D3D11ShareHandleImage() : Image(NULL, D3D11_SHARE_HANDLE_TEXTURE), mSize(0, 0) {}
  virtual ~D3D11ShareHandleImage() {}

  
  
  HRESULT SetData(const Data& aData);

  
  HANDLE GetShareHandle();

  gfxIntSize GetSize() MOZ_OVERRIDE;

  already_AddRefed<gfxASurface> GetAsSurface() MOZ_OVERRIDE;

  ID3D11Texture2D* GetTexture() const;

  SurfaceDescriptor GetSurfaceDescriptor() {
    return mSurfaceDescriptor;
  }

private:

  gfxIntSize mSize;
  RefPtr<ID3D11Texture2D> mTexture;
  HANDLE mShareHandle;
  SurfaceDescriptor mSurfaceDescriptor;

};

} 
} 

#endif 
