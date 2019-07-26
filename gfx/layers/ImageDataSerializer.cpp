




#include "mozilla/layers/ImageDataSerializer.h"
#include "gfxImageSurface.h"
#include "mozilla/gfx/2D.h"
#include "gfx2DGlue.h"
#include "mozilla/gfx/Tools.h"

namespace mozilla {
namespace layers {
















namespace {
struct SurfaceBufferInfo
{
  uint32_t width;
  uint32_t height;
  gfx::SurfaceFormat format;

  static uint32_t GetOffset()
  {
    return gfx::GetAlignedStride<16>(sizeof(SurfaceBufferInfo));
  }
};
} 

static SurfaceBufferInfo* GetBufferInfo(uint8_t* aBuffer)
{
  return reinterpret_cast<SurfaceBufferInfo*>(aBuffer);
}


void
ImageDataSerializer::InitializeBufferInfo(gfx::IntSize aSize,
                                          gfx::SurfaceFormat aFormat)
{
  SurfaceBufferInfo* info = GetBufferInfo(mData);
  info->width = aSize.width;
  info->height = aSize.height;
  info->format = aFormat;
}

uint32_t
ImageDataSerializer::ComputeMinBufferSize(gfx::IntSize aSize,
                                          gfx::SurfaceFormat aFormat)
{
  
  
  gfxIntSize gfxSize = gfxIntSize(aSize.width, aSize.height);
  uint32_t bufsize = aSize.height * gfx::BytesPerPixel(aFormat) * aSize.width;
  return SurfaceBufferInfo::GetOffset()
       + gfx::GetAlignedStride<16>(bufsize);
}

bool
ImageDataSerializerBase::IsValid() const
{
  
  return !!mData;
}

uint8_t*
ImageDataSerializerBase::GetData()
{
  MOZ_ASSERT(IsValid());
  return mData + SurfaceBufferInfo::GetOffset();
}

gfx::IntSize
ImageDataSerializerBase::GetSize() const
{
  MOZ_ASSERT(IsValid());
  SurfaceBufferInfo* info = GetBufferInfo(mData);
  return gfx::IntSize(info->width, info->height);
}

gfx::SurfaceFormat
ImageDataSerializerBase::GetFormat() const
{
  MOZ_ASSERT(IsValid());
  return GetBufferInfo(mData)->format;
}

TemporaryRef<gfxImageSurface>
ImageDataSerializerBase::GetAsThebesSurface()
{
  MOZ_ASSERT(IsValid());
  SurfaceBufferInfo* info = GetBufferInfo(mData);
  uint32_t stride = gfxASurface::BytesPerPixel(
    gfx::SurfaceFormatToImageFormat(GetFormat())) * info->width;
  gfxIntSize size(info->width, info->height);
  RefPtr<gfxImageSurface> surf =
    new gfxImageSurface(GetData(), size, stride,
                        gfx::SurfaceFormatToImageFormat(GetFormat()));
  return surf.forget();
}

TemporaryRef<gfx::DataSourceSurface>
ImageDataSerializerBase::GetAsSurface()
{
  MOZ_ASSERT(IsValid());
  SurfaceBufferInfo* info = GetBufferInfo(mData);
  gfx::IntSize size(info->width, info->height);
  uint32_t stride = gfxASurface::BytesPerPixel(
    gfx::SurfaceFormatToImageFormat(GetFormat())) * info->width;
  RefPtr<gfx::DataSourceSurface> surf =
    gfx::Factory::CreateWrappingDataSourceSurface(GetData(), stride, size, GetFormat());
  return surf.forget();
}

} 
} 
