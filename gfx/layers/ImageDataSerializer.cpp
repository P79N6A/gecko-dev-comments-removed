




#include "ImageDataSerializer.h"
#include "gfx2DGlue.h"                  
#include "gfxASurface.h"                
#include "gfxImageSurface.h"            
#include "gfxPoint.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/Tools.h"          
#include "mozilla/mozalloc.h"           

namespace mozilla {
namespace layers {

using namespace gfx;
















namespace {
struct SurfaceBufferInfo
{
  uint32_t width;
  uint32_t height;
  SurfaceFormat format;

  static uint32_t GetOffset()
  {
    return GetAlignedStride<16>(sizeof(SurfaceBufferInfo));
  }
};
} 

static SurfaceBufferInfo*
GetBufferInfo(uint8_t* aBuffer)
{
  return reinterpret_cast<SurfaceBufferInfo*>(aBuffer);
}


void
ImageDataSerializer::InitializeBufferInfo(IntSize aSize,
                                          SurfaceFormat aFormat)
{
  SurfaceBufferInfo* info = GetBufferInfo(mData);
  info->width = aSize.width;
  info->height = aSize.height;
  info->format = aFormat;
}

static inline uint32_t
ComputeStride(SurfaceFormat aFormat, uint32_t aWidth)
{
  return GetAlignedStride<4>(BytesPerPixel(aFormat) * aWidth);
}

uint32_t
ImageDataSerializer::ComputeMinBufferSize(IntSize aSize,
                                          SurfaceFormat aFormat)
{
  uint32_t bufsize = aSize.height * ComputeStride(aFormat, aSize.width);
  return SurfaceBufferInfo::GetOffset()
       + GetAlignedStride<16>(bufsize);
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

uint32_t
ImageDataSerializerBase::GetStride() const
{
  MOZ_ASSERT(IsValid());
  SurfaceBufferInfo* info = GetBufferInfo(mData);
  return ComputeStride(GetFormat(), info->width);
}

IntSize
ImageDataSerializerBase::GetSize() const
{
  MOZ_ASSERT(IsValid());
  SurfaceBufferInfo* info = GetBufferInfo(mData);
  return IntSize(info->width, info->height);
}

SurfaceFormat
ImageDataSerializerBase::GetFormat() const
{
  MOZ_ASSERT(IsValid());
  return GetBufferInfo(mData)->format;
}

TemporaryRef<gfxImageSurface>
ImageDataSerializerBase::GetAsThebesSurface()
{
  MOZ_ASSERT(IsValid());
  IntSize size = GetSize();
  return new gfxImageSurface(GetData(),
                             gfxIntSize(size.width, size.height),
                             GetStride(),
                             SurfaceFormatToImageFormat(GetFormat()));
}

TemporaryRef<DrawTarget>
ImageDataSerializerBase::GetAsDrawTarget()
{
  MOZ_ASSERT(IsValid());
  return gfxPlatform::GetPlatform()->CreateDrawTargetForData(GetData(),
                                                             GetSize(),
                                                             GetStride(),
                                                             GetFormat());
}

TemporaryRef<DataSourceSurface>
ImageDataSerializerBase::GetAsSurface()
{
  MOZ_ASSERT(IsValid());
  return Factory::CreateWrappingDataSourceSurface(GetData(),
                                                  GetStride(),
                                                  GetSize(),
                                                  GetFormat());
}

} 
} 
