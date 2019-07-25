




































#include "SourceSurfaceD2DTarget.h"
#include "Logging.h"
#include "DrawTargetD2D.h"

#include <algorithm>

namespace mozilla {
namespace gfx {

SourceSurfaceD2DTarget::SourceSurfaceD2DTarget()
  : mFormat(FORMAT_B8G8R8A8)
  , mIsCopy(false)
{
}

SourceSurfaceD2DTarget::~SourceSurfaceD2DTarget()
{
  
  MarkIndependent();
}

IntSize
SourceSurfaceD2DTarget::GetSize() const
{
  D3D10_TEXTURE2D_DESC desc;
  mTexture->GetDesc(&desc);

  return IntSize(desc.Width, desc.Height);
}

SurfaceFormat
SourceSurfaceD2DTarget::GetFormat() const
{
  return mFormat;
}

TemporaryRef<DataSourceSurface>
SourceSurfaceD2DTarget::GetDataSurface()
{
  RefPtr<DataSourceSurfaceD2DTarget> dataSurf =
    new DataSourceSurfaceD2DTarget();

  D3D10_TEXTURE2D_DESC desc;
  mTexture->GetDesc(&desc);

  desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
  desc.Usage = D3D10_USAGE_STAGING;
  desc.BindFlags = 0;
  desc.MiscFlags = 0;

  HRESULT hr = Factory::GetDirect3D10Device()->CreateTexture2D(&desc, NULL, byRef(dataSurf->mTexture));

  if (FAILED(hr)) {
    gfxDebug() << "Failed to create staging texture for SourceSurface. Code: " << hr;
    return NULL;
  }
  Factory::GetDirect3D10Device()->CopyResource(dataSurf->mTexture, mTexture);

  return dataSurf;
}

ID3D10ShaderResourceView*
SourceSurfaceD2DTarget::GetSRView()
{
  if (mSRView) {
    return mSRView;
  }

  HRESULT hr = Factory::GetDirect3D10Device()->CreateShaderResourceView(mTexture, NULL, byRef(mSRView));

  if (FAILED(hr)) {
    gfxWarning() << "Failed to create ShaderResourceView. Code: " << hr;
  }

  return mSRView;
}

void
SourceSurfaceD2DTarget::DrawTargetWillChange()
{
  
  RefPtr<ID3D10Texture2D> oldTexture = mTexture;

  
  
  mIsCopy = true;

  D3D10_TEXTURE2D_DESC desc;
  mTexture->GetDesc(&desc);

  
  Factory::GetDirect3D10Device()->CreateTexture2D(&desc, NULL, byRef(mTexture));
  Factory::GetDirect3D10Device()->CopyResource(mTexture, oldTexture);

  mBitmap = NULL;

  
  MarkIndependent();
}

ID2D1Bitmap*
SourceSurfaceD2DTarget::GetBitmap(ID2D1RenderTarget *aRT)
{
  if (mBitmap) {
    return mBitmap;
  }

  HRESULT hr;
  D3D10_TEXTURE2D_DESC desc;
  mTexture->GetDesc(&desc);

  IntSize size(desc.Width, desc.Height);
  
  RefPtr<IDXGISurface> surf;
  hr = mTexture->QueryInterface((IDXGISurface**)byRef(surf));

  if (FAILED(hr)) {
    gfxWarning() << "Failed to query interface texture to DXGISurface. Code: " << hr;
    return NULL;
  }

  D2D1_BITMAP_PROPERTIES props =
    D2D1::BitmapProperties(D2D1::PixelFormat(DXGIFormat(mFormat), AlphaMode(mFormat)));
  hr = aRT->CreateSharedBitmap(IID_IDXGISurface, surf, &props, byRef(mBitmap));

  if (FAILED(hr)) {
    gfxWarning() << "Failed to create shared bitmap for DrawTarget snapshot. Code: " << hr;
    return NULL;
  }

  return mBitmap;
}

void
SourceSurfaceD2DTarget::MarkIndependent()
{
  if (!mIsCopy) {
    std::vector<SourceSurfaceD2DTarget*> *snapshots = &mDrawTarget->mSnapshots;
    snapshots->erase(std::find(snapshots->begin(), snapshots->end(), this));
  }
}

DataSourceSurfaceD2DTarget::DataSourceSurfaceD2DTarget()
  : mFormat(FORMAT_B8G8R8A8)
  , mMapped(false)
{
}

DataSourceSurfaceD2DTarget::~DataSourceSurfaceD2DTarget()
{
  if (mMapped) {
    mTexture->Unmap(0);
  }
}

IntSize
DataSourceSurfaceD2DTarget::GetSize() const
{
  D3D10_TEXTURE2D_DESC desc;
  mTexture->GetDesc(&desc);

  return IntSize(desc.Width, desc.Height);
}

SurfaceFormat
DataSourceSurfaceD2DTarget::GetFormat() const
{
  return mFormat;
}

unsigned char*
DataSourceSurfaceD2DTarget::GetData()
{
  EnsureMapped();

  return (unsigned char*)mMap.pData;
}

int32_t
DataSourceSurfaceD2DTarget::Stride()
{
  EnsureMapped();
  return mMap.RowPitch;
}

void
DataSourceSurfaceD2DTarget::EnsureMapped()
{
  if (!mMapped) {
    HRESULT hr = mTexture->Map(0, D3D10_MAP_READ, 0, &mMap);
    if (FAILED(hr)) {
      gfxWarning() << "Failed to map texture to memory. Code: " << hr;
      return;
    }
    mMapped = true;
  }
}

}
}
