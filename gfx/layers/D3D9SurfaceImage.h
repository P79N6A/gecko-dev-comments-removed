




#ifndef GFX_D3DSURFACEIMAGE_H
#define GFX_D3DSURFACEIMAGE_H

#include "mozilla/RefPtr.h"
#include "ImageContainer.h"
#include "nsAutoPtr.h"
#include "d3d9.h"

namespace mozilla {
namespace layers {





class D3D9SurfaceImage : public Image
                       , public ISharedImage {
public:

  struct Data {
    Data(IDirect3DSurface9* aSurface, const nsIntRect& aRegion)
      : mSurface(aSurface), mRegion(aRegion) {}
    RefPtr<IDirect3DSurface9> mSurface;
    nsIntRect mRegion;
  };

  D3D9SurfaceImage();
  virtual ~D3D9SurfaceImage();

  virtual ISharedImage* AsSharedImage() MOZ_OVERRIDE { return this; }

  
  
  HRESULT SetData(const Data& aData);

  
  const D3DSURFACE_DESC& GetDesc() const;

  gfx::IntSize GetSize() MOZ_OVERRIDE;

  virtual TemporaryRef<gfx::SourceSurface> GetAsSourceSurface() MOZ_OVERRIDE;

  virtual TextureClient* GetTextureClient(CompositableClient* aClient) MOZ_OVERRIDE;
  virtual uint8_t* GetBuffer() MOZ_OVERRIDE { return nullptr; }

  virtual bool IsValid() MOZ_OVERRIDE;

private:

  gfx::IntSize mSize;
  RefPtr<IDirect3DTexture9> mTexture;
  RefPtr<TextureClient> mTextureClient;
  HANDLE mShareHandle;
  D3DSURFACE_DESC mDesc;
  bool mIsValid;
};

} 
} 

#endif 
