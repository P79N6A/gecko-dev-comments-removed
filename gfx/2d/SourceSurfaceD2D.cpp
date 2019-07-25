




































#include "SourceSurfaceD2D.h"
#include "Logging.h"

namespace mozilla {
namespace gfx {

SourceSurfaceD2D::SourceSurfaceD2D()
{
}

SourceSurfaceD2D::~SourceSurfaceD2D()
{
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

TemporaryRef<DataSourceSurface>
SourceSurfaceD2D::GetDataSurface()
{
  return NULL;
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
    int newStride = BytesPerPixel(aFormat) * aSize.width;
    mRawData.resize(aSize.height * newStride);
    for (int y = 0; y < aSize.height; y++) {
      memcpy(&mRawData.front() + y * newStride, aData + y * aStride, newStride);
    }
    gfxDebug() << "Bitmap does not fit in texture, saving raw data.";
    return true;
  }

  D2D1_BITMAP_PROPERTIES props =
    D2D1::BitmapProperties(D2D1::PixelFormat(DXGIFormat(aFormat), AlphaMode(aFormat)));
  hr = aRT->CreateBitmap(D2DIntSize(aSize), aData, aStride, props, byRef(mBitmap));

  if (FAILED(hr)) {
    gfxWarning() << "Failed to create D2D Bitmap for data. Code: " << hr;
    return false;
  }

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
    gfxWarning() << "Failed to QI texture to surface. Code: " << hr;
    return false;
  }

  D3D10_TEXTURE2D_DESC desc;
  aTexture->GetDesc(&desc);

  mSize = IntSize(desc.Width, desc.Height);
  mFormat = aFormat;

  D2D1_BITMAP_PROPERTIES props =
    D2D1::BitmapProperties(D2D1::PixelFormat(DXGIFormat(aFormat), AlphaMode(aFormat)));
  hr = aRT->CreateSharedBitmap(IID_IDXGISurface, surf, &props, byRef(mBitmap));

  if (FAILED(hr)) {
    gfxWarning() << "Failed to create SharedBitmap. Code: " << hr;
    return false;
  }

  return true;
}

}
}
