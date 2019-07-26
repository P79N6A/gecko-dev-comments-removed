




#include "D3D9SurfaceImage.h"
#include "gfxImageSurface.h"

namespace mozilla {
namespace layers {

HRESULT
D3D9SurfaceImage::SetData(const Data& aData)
{
  NS_ENSURE_TRUE(aData.mSurface, E_POINTER);
  HRESULT hr;
  RefPtr<IDirect3DSurface9> surface = aData.mSurface;

  RefPtr<IDirect3DDevice9> device;
  hr = surface->GetDevice(byRef(device));
  NS_ENSURE_TRUE(SUCCEEDED(hr), E_FAIL);

  RefPtr<IDirect3D9> d3d9;
  hr = device->GetDirect3D(byRef(d3d9));
  NS_ENSURE_TRUE(SUCCEEDED(hr), E_FAIL);

  D3DSURFACE_DESC desc;
  surface->GetDesc(&desc);
  
  
  hr = d3d9->CheckDeviceFormatConversion(D3DADAPTER_DEFAULT,
                                         D3DDEVTYPE_HAL,
                                         desc.Format,
                                         D3DFMT_X8R8G8B8);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  
  
  
  const nsIntRect& region = aData.mRegion;
  RefPtr<IDirect3DTexture9> texture;
  HANDLE shareHandle = nullptr;
  hr = device->CreateTexture(region.width,
                             region.height,
                             1,
                             D3DUSAGE_RENDERTARGET,
                             D3DFMT_X8R8G8B8,
                             D3DPOOL_DEFAULT,
                             byRef(texture),
                             &shareHandle);
  NS_ENSURE_TRUE(SUCCEEDED(hr) && shareHandle, hr);

  
  RefPtr<IDirect3DSurface9> textureSurface;
  hr = texture->GetSurfaceLevel(0, byRef(textureSurface));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  
  textureSurface->GetDesc(&mDesc);

  RECT src = { region.x, region.y, region.x+region.width, region.y+region.height };
  hr = device->StretchRect(surface, &src, textureSurface, nullptr, D3DTEXF_NONE);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  
  
  
  RefPtr<IDirect3DQuery9> query;
  hr = device->CreateQuery(D3DQUERYTYPE_EVENT, byRef(query));
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);
  hr = query->Issue(D3DISSUE_END);
  NS_ENSURE_TRUE(SUCCEEDED(hr), hr);

  mTexture = texture;
  mShareHandle = shareHandle;
  mSize = gfxIntSize(region.width, region.height);
  mQuery = query;

  return S_OK;
}

void
D3D9SurfaceImage::EnsureSynchronized()
{
  if (!mQuery) {
    
    return;
  }
  int iterations = 0;
  while (iterations < 10 && S_FALSE == mQuery->GetData(nullptr, 0, D3DGETDATA_FLUSH)) {
    Sleep(1);
    iterations++;
  }
  mQuery = nullptr;
}

HANDLE
D3D9SurfaceImage::GetShareHandle()
{
  
  
  EnsureSynchronized();
  return mShareHandle;
}

const D3DSURFACE_DESC&
D3D9SurfaceImage::GetDesc() const
{
  return mDesc;
}

gfxIntSize
D3D9SurfaceImage::GetSize()
{
  return mSize;
}

already_AddRefed<gfxASurface>
D3D9SurfaceImage::GetAsSurface()
{
  NS_ENSURE_TRUE(mTexture, nullptr);

  HRESULT hr;
  nsRefPtr<gfxImageSurface> surface =
    new gfxImageSurface(mSize, gfxImageFormatRGB24);

  if (!surface->CairoSurface() || surface->CairoStatus()) {
    NS_WARNING("Failed to created Cairo image surface for D3D9SurfaceImage.");
    return nullptr;
  }

  
  EnsureSynchronized();

  
  
  RefPtr<IDirect3DSurface9> textureSurface;
  hr = mTexture->GetSurfaceLevel(0, byRef(textureSurface));
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  RefPtr<IDirect3DDevice9> device;
  hr = mTexture->GetDevice(byRef(device));
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  RefPtr<IDirect3DSurface9> systemMemorySurface;
  hr = device->CreateOffscreenPlainSurface(mDesc.Width,
                                           mDesc.Height,
                                           D3DFMT_X8R8G8B8,
                                           D3DPOOL_SYSTEMMEM,
                                           byRef(systemMemorySurface),
                                           0);
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  hr = device->GetRenderTargetData(textureSurface, systemMemorySurface);
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  D3DLOCKED_RECT rect;
  hr = systemMemorySurface->LockRect(&rect, nullptr, 0);
  NS_ENSURE_TRUE(SUCCEEDED(hr), nullptr);

  const unsigned char* src = (const unsigned char*)(rect.pBits);
  const unsigned srcPitch = rect.Pitch;
  for (int y = 0; y < mSize.height; y++) {
    memcpy(surface->Data() + surface->Stride() * y,
           (unsigned char*)(src) + srcPitch * y,
           mSize.width * 4);
  }

  systemMemorySurface->UnlockRect();

  return surface.forget();
}

} 
} 
