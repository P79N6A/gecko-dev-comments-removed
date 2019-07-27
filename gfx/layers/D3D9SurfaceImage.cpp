




#include "D3D9SurfaceImage.h"
#include "gfx2DGlue.h"
#include "mozilla/layers/TextureD3D9.h"
#include "mozilla/layers/CompositableClient.h"
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/gfx/Types.h"

namespace mozilla {
namespace layers {


D3D9SurfaceImage::D3D9SurfaceImage()
  : Image(nullptr, ImageFormat::D3D9_RGB32_TEXTURE)
  , mSize(0, 0)
{}

D3D9SurfaceImage::~D3D9SurfaceImage() {}

static const GUID sD3D9TextureUsage =
{ 0x631e1338, 0xdc22, 0x497f, { 0xa1, 0xa8, 0xb4, 0xfe, 0x3a, 0xf4, 0x13, 0x4d } };




class TextureMemoryMeasurer9 : public IUnknown
{
public:
  TextureMemoryMeasurer9(size_t aMemoryUsed)
  {
    mMemoryUsed = aMemoryUsed;
    gfxWindowsPlatform::sD3D9MemoryUsed += mMemoryUsed;
    mRefCnt = 0;
  }
  ~TextureMemoryMeasurer9()
  {
    gfxWindowsPlatform::sD3D9MemoryUsed -= mMemoryUsed;
  }
  STDMETHODIMP_(ULONG) AddRef() {
    mRefCnt++;
    return mRefCnt;
  }
  STDMETHODIMP QueryInterface(REFIID riid,
                              void **ppvObject)
  {
    IUnknown *punk = nullptr;
    if (riid == IID_IUnknown) {
      punk = this;
    }
    *ppvObject = punk;
    if (punk) {
      punk->AddRef();
      return S_OK;
    } else {
      return E_NOINTERFACE;
    }
  }

  STDMETHODIMP_(ULONG) Release() {
    int refCnt = --mRefCnt;
    if (refCnt == 0) {
      delete this;
    }
    return refCnt;
  }
private:
  int mRefCnt;
  int mMemoryUsed;
};


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

  
  texture->SetPrivateData(sD3D9TextureUsage, new TextureMemoryMeasurer9(region.width * region.height * 4), sizeof(IUnknown *), D3DSPD_IUNKNOWN);

  
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
  mSize = gfx::IntSize(region.width, region.height);
  mQuery = query;

  return S_OK;
}

void
D3D9SurfaceImage::EnsureSynchronized()
{
  RefPtr<IDirect3DQuery9> query = mQuery;
  if (!query) {
    
    return;
  }
  int iterations = 0;
  while (iterations < 10 && S_FALSE == query->GetData(nullptr, 0, D3DGETDATA_FLUSH)) {
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

gfx::IntSize
D3D9SurfaceImage::GetSize()
{
  return mSize;
}

TextureClient*
D3D9SurfaceImage::GetTextureClient(CompositableClient* aClient)
{
  EnsureSynchronized();
  if (!mTextureClient) {
    RefPtr<SharedTextureClientD3D9> textureClient =
      new SharedTextureClientD3D9(aClient->GetForwarder(),
                                  gfx::SurfaceFormat::B8G8R8X8,
                                  TextureFlags::DEFAULT);
    textureClient->InitWith(mTexture, mShareHandle, mDesc);
    mTextureClient = textureClient;
  }
  return mTextureClient;
}

TemporaryRef<gfx::SourceSurface>
D3D9SurfaceImage::GetAsSourceSurface()
{
  NS_ENSURE_TRUE(mTexture, nullptr);

  HRESULT hr;
  RefPtr<gfx::DataSourceSurface> surface = gfx::Factory::CreateDataSourceSurface(mSize, gfx::SurfaceFormat::B8G8R8X8);
  if (NS_WARN_IF(!surface)) {
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

  gfx::DataSourceSurface::MappedSurface mappedSurface;
  if (!surface->Map(gfx::DataSourceSurface::WRITE, &mappedSurface)) {
    systemMemorySurface->UnlockRect();
    return nullptr;
  }

  const unsigned char* src = (const unsigned char*)(rect.pBits);
  const unsigned srcPitch = rect.Pitch;
  for (int y = 0; y < mSize.height; y++) {
    memcpy(mappedSurface.mData + mappedSurface.mStride * y,
           (unsigned char*)(src) + srcPitch * y,
           mSize.width * 4);
  }

  systemMemorySurface->UnlockRect();
  surface->Unmap();

  return surface;
}

} 
} 
