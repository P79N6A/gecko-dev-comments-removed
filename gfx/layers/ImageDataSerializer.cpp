




#include "ImageDataSerializer.h"
#include "gfx2DGlue.h"                  
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
GetBufferInfo(uint8_t* aData, size_t aDataSize)
{
  return aDataSize >= sizeof(SurfaceBufferInfo)
         ? reinterpret_cast<SurfaceBufferInfo*>(aData)
         : nullptr;
}

void
ImageDataSerializer::InitializeBufferInfo(IntSize aSize,
                                          SurfaceFormat aFormat)
{
  SurfaceBufferInfo* info = GetBufferInfo(mData, mDataSize);
  MOZ_ASSERT(info); 
  info->width = aSize.width;
  info->height = aSize.height;
  info->format = aFormat;
  Validate();
}

static inline uint32_t
ComputeStride(SurfaceFormat aFormat, uint32_t aWidth)
{
  return GetAlignedStride<4>(BytesPerPixel(aFormat) * aWidth);
}

uint32_t
ImageDataSerializerBase::ComputeMinBufferSize(IntSize aSize,
                                          SurfaceFormat aFormat)
{
  uint32_t bufsize = aSize.height * ComputeStride(aFormat, aSize.width);
  return SurfaceBufferInfo::GetOffset()
       + GetAlignedStride<16>(bufsize);
}

void
ImageDataSerializerBase::Validate()
{
  mIsValid = false;
  if (!mData) {
    return;
  }
  SurfaceBufferInfo* info = GetBufferInfo(mData, mDataSize);
  if (!info) {
    return;
  }
  size_t requiredSize =
           ComputeMinBufferSize(IntSize(info->width, info->height), info->format);
  mIsValid = requiredSize <= mDataSize;
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
  SurfaceBufferInfo* info = GetBufferInfo(mData, mDataSize);
  return ComputeStride(GetFormat(), info->width);
}

IntSize
ImageDataSerializerBase::GetSize() const
{
  MOZ_ASSERT(IsValid());
  SurfaceBufferInfo* info = GetBufferInfo(mData, mDataSize);
  return IntSize(info->width, info->height);
}

SurfaceFormat
ImageDataSerializerBase::GetFormat() const
{
  MOZ_ASSERT(IsValid());
  return GetBufferInfo(mData, mDataSize)->format;
}

TemporaryRef<DrawTarget>
ImageDataSerializerBase::GetAsDrawTarget(gfx::BackendType aBackend)
{
  MOZ_ASSERT(IsValid());
  return gfx::Factory::CreateDrawTargetForData(aBackend,
                                               GetData(), GetSize(),
                                               GetStride(), GetFormat());
}

TemporaryRef<gfx::DataSourceSurface>
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
