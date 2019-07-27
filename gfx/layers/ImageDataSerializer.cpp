




#include "ImageDataSerializer.h"
#include "gfx2DGlue.h"                  
#include "mozilla/gfx/Point.h"          
#include "mozilla/Assertions.h"         
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/Logging.h"        
#include "mozilla/gfx/Tools.h"          
#include "mozilla/mozalloc.h"           

namespace mozilla {
namespace layers {

using namespace gfx;
















namespace {
struct SurfaceBufferInfo
{
  int32_t width;
  int32_t height;
  SurfaceFormat format;

  static int32_t GetOffset()
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

static inline int32_t
ComputeStride(SurfaceFormat aFormat, int32_t aWidth)
{
  CheckedInt<int32_t> size = BytesPerPixel(aFormat);
  size *= aWidth;
  if (!size.isValid() || size.value() <= 0) {
    gfxDebug() << "ComputeStride overflow " << aWidth;
    return 0;
  }

  return GetAlignedStride<4>(size.value());
}

uint32_t
ImageDataSerializerBase::ComputeMinBufferSize(IntSize aSize,
                                              SurfaceFormat aFormat)
{
  MOZ_ASSERT(aSize.height >= 0 && aSize.width >= 0);
  if (aSize.height <= 0 || aSize.width <= 0) {
    gfxDebug() << "Non-positive image buffer size request " << aSize.width << "x" << aSize.height;
    return 0;
  }

  CheckedInt<int32_t> bufsize = ComputeStride(aFormat, aSize.width);
  bufsize *= aSize.height;

  if (!bufsize.isValid() || bufsize.value() <= 0) {
    gfxDebug() << "Buffer size overflow " << aSize.width << "x" << aSize.height;
    return 0;
  }

  return SurfaceBufferInfo::GetOffset()
       + GetAlignedStride<16>(bufsize.value());
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

already_AddRefed<DrawTarget>
ImageDataSerializerBase::GetAsDrawTarget(gfx::BackendType aBackend)
{
  MOZ_ASSERT(IsValid());
  return gfx::Factory::CreateDrawTargetForData(aBackend,
                                               GetData(), GetSize(),
                                               GetStride(), GetFormat());
}

already_AddRefed<gfx::DataSourceSurface>
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
