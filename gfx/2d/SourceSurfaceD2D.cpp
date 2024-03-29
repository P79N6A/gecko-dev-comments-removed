




#include "SourceSurfaceD2D.h"
#include "DrawTargetD2D.h"
#include "Logging.h"
#include "Tools.h"

namespace mozilla {
namespace gfx {

SourceSurfaceD2D::SourceSurfaceD2D()
{
}

SourceSurfaceD2D::~SourceSurfaceD2D()
{
  if (mBitmap) {
    DrawTargetD2D::mVRAMUsageSS -= GetByteSize();
  }
}

IntSize
SourceSurfaceD2D::GetSize() const
{
  return mSize;
}

SurfaceFormat
SourceSurfaceD2D::GetFormat() const
{
  return mFormat;
}

bool
SourceSurfaceD2D::IsValid() const
{
  return mDevice == Factory::GetDirect3D10Device();
}

already_AddRefed<DataSourceSurface>
SourceSurfaceD2D::GetDataSurface()
{
  RefPtr<DataSourceSurfaceD2D> result = new DataSourceSurfaceD2D(this);
  if (result->IsValid()) {
    return result.forget();
  }
  return nullptr;
}

bool
SourceSurfaceD2D::InitFromData(unsigned char *aData,
                               const IntSize &aSize,
                               int32_t aStride,
                               SurfaceFormat aFormat,
                               ID2D1RenderTarget *aRT)
{
  HRESULT hr;

  mFormat = aFormat;
  mSize = aSize;

  if ((uint32_t)aSize.width > aRT->GetMaximumBitmapSize() ||
      (uint32_t)aSize.height > aRT->GetMaximumBitmapSize()) {
    gfxDebug() << "Bitmap does not fit in texture.";
    return false;
  }

  D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(D2DPixelFormat(aFormat));
  hr = aRT->CreateBitmap(D2DIntSize(aSize), props, byRef(mBitmap));

  if (FAILED(hr)) {
    gfxWarning() << "Failed to create D2D Bitmap for data. Code: " << hexa(hr);
    return false;
  }

  hr = mBitmap->CopyFromMemory(nullptr, aData, aStride);

  if (FAILED(hr)) {
    gfxWarning() << "Failed to copy data to D2D bitmap. Code: " << hexa(hr);
    return false;
  }

  DrawTargetD2D::mVRAMUsageSS += GetByteSize();
  mDevice = Factory::GetDirect3D10Device();

  return true;
}

bool
SourceSurfaceD2D::InitFromTexture(ID3D10Texture2D *aTexture,
                                  SurfaceFormat aFormat,
                                  ID2D1RenderTarget *aRT)
{
  HRESULT hr;

  RefPtr<IDXGISurface> surf;

  hr = aTexture->QueryInterface((IDXGISurface**)&surf);

  if (FAILED(hr)) {
    gfxWarning() << "Failed to QI texture to surface. Code: " << hexa(hr);
    return false;
  }

  D3D10_TEXTURE2D_DESC desc;
  aTexture->GetDesc(&desc);

  mSize = IntSize(desc.Width, desc.Height);
  mFormat = aFormat;

  D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(D2DPixelFormat(aFormat));
  hr = aRT->CreateSharedBitmap(IID_IDXGISurface, surf, &props, byRef(mBitmap));

  if (FAILED(hr)) {
    gfxWarning() << "Failed to create SharedBitmap. Code: " << hexa(hr);
    return false;
  }

  aTexture->GetDevice(byRef(mDevice));
  DrawTargetD2D::mVRAMUsageSS += GetByteSize();

  return true;
}

uint32_t
SourceSurfaceD2D::GetByteSize() const
{
  return mSize.width * mSize.height * BytesPerPixel(mFormat);
}

DataSourceSurfaceD2D::DataSourceSurfaceD2D(SourceSurfaceD2D* aSourceSurface)
  : mTexture(nullptr)
  , mFormat(aSourceSurface->mFormat)
  , mSize(aSourceSurface->mSize)
  , mMapped(false)
{
  
  
  
  

  CD3D10_TEXTURE2D_DESC desc(DXGIFormat(mFormat), mSize.width, mSize.height);
  desc.MipLevels = 1;
  desc.Usage = D3D10_USAGE_DEFAULT;
  desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
  RefPtr<ID3D10Texture2D> sourceTexture;
  HRESULT hr = aSourceSurface->mDevice->CreateTexture2D(&desc, nullptr,
                                                        byRef(sourceTexture));
  if (FAILED(hr)) {
    gfxWarning() << "Failed to create texture. Code: " << hexa(hr);
    return;
  }

  RefPtr<IDXGISurface> dxgiSurface;
  hr = sourceTexture->QueryInterface((IDXGISurface**)byRef(dxgiSurface));
  if (FAILED(hr)) {
    gfxWarning() << "Failed to create DXGI surface. Code: " << hexa(hr);
    return;
  }

  D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));

  RefPtr<ID2D1RenderTarget> renderTarget;
  hr = DrawTargetD2D::factory()->CreateDxgiSurfaceRenderTarget(dxgiSurface,
                                                               &rtProps,
                                                               byRef(renderTarget));
  if (FAILED(hr)) {
    gfxWarning() << "Failed to create render target. Code: " << hexa(hr);
    return;
  }

  renderTarget->BeginDraw();
  renderTarget->Clear(D2D1::ColorF(0, 0.0f));
  if (aSourceSurface->GetFormat() != SurfaceFormat::A8) {
    renderTarget->DrawBitmap(aSourceSurface->mBitmap,
                             D2D1::RectF(0, 0,
                             Float(mSize.width),
                             Float(mSize.height)));
  } else {
    RefPtr<ID2D1SolidColorBrush> brush;
    renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), byRef(brush));
    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    renderTarget->FillOpacityMask(aSourceSurface->mBitmap, brush, D2D1_OPACITY_MASK_CONTENT_GRAPHICS);
  }
  hr = renderTarget->EndDraw();
  if (FAILED(hr)) {
    gfxWarning() << "Failed to draw bitmap. Code: " << hexa(hr);
    return;
  }

  desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ | D3D10_CPU_ACCESS_WRITE;
  desc.Usage = D3D10_USAGE_STAGING;
  desc.BindFlags = 0;
  hr = aSourceSurface->mDevice->CreateTexture2D(&desc, nullptr, byRef(mTexture));
  if (FAILED(hr)) {
    gfxWarning() << "Failed to create staging texture. Code: " << hexa(hr);
    mTexture = nullptr;
    return;
  }

  aSourceSurface->mDevice->CopyResource(mTexture, sourceTexture);
}

DataSourceSurfaceD2D::~DataSourceSurfaceD2D()
{
  if (mMapped) {
    mTexture->Unmap(0);
  }
}

unsigned char*
DataSourceSurfaceD2D::GetData()
{
  EnsureMappedTexture();
  if (!mMapped) {
    return nullptr;
  }

  return reinterpret_cast<unsigned char*>(mData.pData);
}

int32_t
DataSourceSurfaceD2D::Stride()
{
  EnsureMappedTexture();
  if (!mMapped) {
    return 0;
  }

  return mData.RowPitch;
}

IntSize
DataSourceSurfaceD2D::GetSize() const
{
  return mSize;
}

SurfaceFormat
DataSourceSurfaceD2D::GetFormat() const
{
  return mFormat;
}

bool
DataSourceSurfaceD2D::Map(MapType aMapType, MappedSurface *aMappedSurface)
{
  
  MOZ_ASSERT(!mMapped);
  MOZ_ASSERT(!mIsMapped);

  if (!mTexture) {
    return false;
  }

  D3D10_MAP mapType;

  if (aMapType == MapType::READ) {
    mapType = D3D10_MAP_READ;
  } else if (aMapType == MapType::WRITE) {
    mapType = D3D10_MAP_WRITE;
  } else {
    mapType = D3D10_MAP_READ_WRITE;
  }

  D3D10_MAPPED_TEXTURE2D map;

  HRESULT hr = mTexture->Map(0, mapType, 0, &map);

  if (FAILED(hr)) {
    gfxWarning() << "Texture map failed with code: " << hexa(hr);
    return false;
  }

  aMappedSurface->mData = (uint8_t*)map.pData;
  aMappedSurface->mStride = map.RowPitch;
  mIsMapped = !!aMappedSurface->mData;

  return mIsMapped;
}

void
DataSourceSurfaceD2D::Unmap()
{
  MOZ_ASSERT(mIsMapped);

  mIsMapped = false;
  mTexture->Unmap(0);
}

void
DataSourceSurfaceD2D::EnsureMappedTexture()
{
  
  MOZ_ASSERT(!mIsMapped);

  if (mMapped ||
      !mTexture) {
    return;
  }

  HRESULT hr = mTexture->Map(0, D3D10_MAP_READ, 0, &mData);
  if (FAILED(hr)) {
    gfxWarning() << "Failed to map texture. Code: " << hexa(hr);
    mTexture = nullptr;
  } else {
    mMapped = true;
  }
}

}
}
