




#ifndef GFX_D311_SHARE_HANDLE_IMAGE_H
#define GFX_D311_SHARE_HANDLE_IMAGE_H

#include "mozilla/RefPtr.h"
#include "ImageContainer.h"
#include "nsAutoPtr.h"
#include "d3d11.h"
#include "mozilla/layers/TextureClient.h"

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

  D3D11ShareHandleImage() : Image(NULL, ImageFormat::D3D11_SHARE_HANDLE_TEXTURE), mSize(0, 0) {}
  virtual ~D3D11ShareHandleImage() {}

  
  
  HRESULT SetData(const Data& aData);

  gfx::IntSize GetSize() override;

  virtual TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() override;

  virtual TextureClient* GetTextureClient(CompositableClient* aClient) override;

  ID3D11Texture2D* GetTexture() const;

  virtual nsIntRect GetPictureRect() override { return mPictureRect; }

private:

  gfx::IntSize mSize;
  nsIntRect mPictureRect;
  RefPtr<ID3D11Texture2D> mTexture;
  RefPtr<TextureClient> mTextureClient;
  HANDLE mShareHandle;
  gfx::SurfaceFormat mFormat;

};

} 
} 

#endif 
