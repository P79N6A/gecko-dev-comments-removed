




#include "SourceSurfaceCG.h"
#include "DrawTargetCG.h"
#include "DataSourceSurfaceWrapper.h"
#include "DataSurfaceHelpers.h"
#include "mozilla/Types.h" 

#include "MacIOSurface.h"
#include "Tools.h"

namespace mozilla {
namespace gfx {


SourceSurfaceCG::~SourceSurfaceCG()
{
  CGImageRelease(mImage);
}

IntSize
SourceSurfaceCG::GetSize() const
{
  IntSize size;
  size.width = CGImageGetWidth(mImage);
  size.height = CGImageGetHeight(mImage);
  return size;
}

SurfaceFormat
SourceSurfaceCG::GetFormat() const
{
  return mFormat;
}

already_AddRefed<DataSourceSurface>
SourceSurfaceCG::GetDataSurface()
{
  
  CGImageRetain(mImage);
  RefPtr<DataSourceSurface> dataSurf = new DataSourceSurfaceCG(mImage);

  
  
  return MakeAndAddRef<DataSourceSurfaceWrapper>(dataSurf);
}

static void releaseCallback(void *info, const void *data, size_t size) {
  free(info);
}

CGImageRef
CreateCGImage(void *aInfo,
              const void *aData,
              const IntSize &aSize,
              int32_t aStride,
              SurfaceFormat aFormat)
{
  return CreateCGImage(releaseCallback,
                       aInfo,
                       aData,
                       aSize,
                       aStride,
                       aFormat);
}

CGImageRef
CreateCGImage(CGDataProviderReleaseDataCallback aCallback,
              void *aInfo,
              const void *aData,
              const IntSize &aSize,
              int32_t aStride,
              SurfaceFormat aFormat)
{
  
  CGColorSpaceRef colorSpace = nullptr;
  CGBitmapInfo bitinfo = 0;
  int bitsPerComponent = 0;
  int bitsPerPixel = 0;

  switch (aFormat) {
    case SurfaceFormat::B8G8R8A8:
      colorSpace = CGColorSpaceCreateDeviceRGB();
      bitinfo = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host;
      bitsPerComponent = 8;
      bitsPerPixel = 32;
      break;

    case SurfaceFormat::B8G8R8X8:
      colorSpace = CGColorSpaceCreateDeviceRGB();
      bitinfo = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host;
      bitsPerComponent = 8;
      bitsPerPixel = 32;
      break;

    case SurfaceFormat::A8:
      
      bitsPerComponent = 8;
      bitsPerPixel = 8;
      break;

    default:
      MOZ_CRASH();
  }

  size_t bufLen = BufferSizeFromStrideAndHeight(aStride, aSize.height);
  if (bufLen == 0) {
    return nullptr;
  }
  CGDataProviderRef dataProvider = CGDataProviderCreateWithData(aInfo,
                                                                aData,
                                                                bufLen,
                                                                aCallback);

  CGImageRef image;
  if (aFormat == SurfaceFormat::A8) {
    CGFloat decode[] = {1.0, 0.0};
    image = CGImageMaskCreate (aSize.width, aSize.height,
                               bitsPerComponent,
                               bitsPerPixel,
                               aStride,
                               dataProvider,
                               decode,
                               true);
  } else {
    image = CGImageCreate (aSize.width, aSize.height,
                           bitsPerComponent,
                           bitsPerPixel,
                           aStride,
                           colorSpace,
                           bitinfo,
                           dataProvider,
                           nullptr,
                           true,
                           kCGRenderingIntentDefault);
  }

  CGDataProviderRelease(dataProvider);
  CGColorSpaceRelease(colorSpace);

  return image;
}

bool
SourceSurfaceCG::InitFromData(unsigned char *aData,
                               const IntSize &aSize,
                               int32_t aStride,
                               SurfaceFormat aFormat)
{
  assert(aSize.width >= 0 && aSize.height >= 0);

  size_t bufLen = BufferSizeFromStrideAndHeight(aStride, aSize.height);
  if (bufLen == 0) {
    mImage = nullptr;
    return false;
  }

  void *data = malloc(bufLen);
  
  
  memcpy(data, aData, bufLen - aStride + (aSize.width * BytesPerPixel(aFormat)));

  mFormat = aFormat;
  mImage = CreateCGImage(data, data, aSize, aStride, aFormat);

  return mImage != nullptr;
}

DataSourceSurfaceCG::~DataSourceSurfaceCG()
{
  CGImageRelease(mImage);
  free(CGBitmapContextGetData(mCg));
  CGContextRelease(mCg);
}

IntSize
DataSourceSurfaceCG::GetSize() const
{
  IntSize size;
  size.width = CGImageGetWidth(mImage);
  size.height = CGImageGetHeight(mImage);
  return size;
}

bool
DataSourceSurfaceCG::InitFromData(unsigned char *aData,
                               const IntSize &aSize,
                               int32_t aStride,
                               SurfaceFormat aFormat)
{
  if (aSize.width <= 0 || aSize.height <= 0) {
    return false;
  }

  size_t bufLen = BufferSizeFromStrideAndHeight(aStride, aSize.height);
  if (bufLen == 0) {
    mImage = nullptr;
    return false;
  }

  void *data = malloc(bufLen);
  memcpy(data, aData, bufLen - aStride + (aSize.width * BytesPerPixel(aFormat)));

  mFormat = aFormat;
  mImage = CreateCGImage(data, data, aSize, aStride, aFormat);

  if (!mImage) {
    free(data);
    return false;
  }

  return true;
}

CGContextRef CreateBitmapContextForImage(CGImageRef image)
{
  CGColorSpaceRef colorSpace;

  size_t width  = CGImageGetWidth(image);
  size_t height = CGImageGetHeight(image);

  int bitmapBytesPerRow = (width * 4);
  int bitmapByteCount   = (bitmapBytesPerRow * height);

  void *data = calloc(bitmapByteCount, 1);
  
  colorSpace = CGColorSpaceCreateDeviceRGB();
  assert(colorSpace);

  
  
  
  
  
  CGContextRef cg = CGBitmapContextCreate(data,
                                          width,
                                          height,
                                          8,
                                          bitmapBytesPerRow,
                                          colorSpace,
                                          kCGBitmapByteOrder32Host | kCGImageAlphaPremultipliedFirst);
  assert(cg);

  CGColorSpaceRelease(colorSpace);

  return cg;
}

DataSourceSurfaceCG::DataSourceSurfaceCG(CGImageRef aImage)
{
  mFormat = SurfaceFormat::B8G8R8A8;
  mImage = aImage;
  mCg = CreateBitmapContextForImage(aImage);
  if (mCg == nullptr) {
    
    return;
  }

  
  CGFloat w = CGImageGetWidth(aImage);
  CGFloat h = CGImageGetHeight(aImage);
  CGRect rect = {{0,0},{w,h}};

  
  
  
  CGContextDrawImage(mCg, rect, aImage);

  
  
  mData = CGBitmapContextGetData(mCg);
  assert(mData);
}

unsigned char *
DataSourceSurfaceCG::GetData()
{
  
  
  
  
  
  
  
  
  return (unsigned char*)mData;
}

SourceSurfaceCGBitmapContext::SourceSurfaceCGBitmapContext(DrawTargetCG *aDrawTarget)
{
  mDrawTarget = aDrawTarget;
  mFormat = aDrawTarget->GetFormat();
  mCg = (CGContextRef)aDrawTarget->GetNativeSurface(NativeSurfaceType::CGCONTEXT);
  if (!mCg)
    abort();

  mSize.width = CGBitmapContextGetWidth(mCg);
  mSize.height = CGBitmapContextGetHeight(mCg);
  mStride = CGBitmapContextGetBytesPerRow(mCg);
  mData = CGBitmapContextGetData(mCg);

  mImage = nullptr;
}

void SourceSurfaceCGBitmapContext::EnsureImage() const
{
  
  
  
  
  
  
  
  if (!mImage) {
    if (!mData) abort();
    mImage = CreateCGImage(nullptr, mData, mSize, mStride, mFormat);
  }
}

IntSize
SourceSurfaceCGBitmapContext::GetSize() const
{
  return mSize;
}

void
SourceSurfaceCGBitmapContext::DrawTargetWillChange()
{
  if (mDrawTarget) {
    
    size_t stride = CGBitmapContextGetBytesPerRow(mCg);
    size_t height = CGBitmapContextGetHeight(mCg);

    size_t bufLen = BufferSizeFromStrideAndHeight(stride, height);
    if (bufLen == 0) {
      mDataHolder.Dealloc();
      mData = nullptr;
    } else {
      static_assert(sizeof(decltype(mDataHolder[0])) == 1,
                    "mDataHolder.Realloc() takes an object count, so its objects must be 1-byte sized if we use bufLen");
      mDataHolder.Realloc( bufLen);
      mData = mDataHolder;

      
      
      
      memcpy(mData, CGBitmapContextGetData(mCg), bufLen);
    }

    
    if (mImage)
      CGImageRelease(mImage);
    mImage = nullptr;

    mCg = nullptr;
    mDrawTarget = nullptr;
  }
}

void
SourceSurfaceCGBitmapContext::DrawTargetWillGoAway()
{
  if (mDrawTarget) {
    if (mDrawTarget->mData != CGBitmapContextGetData(mCg)) {
      DrawTargetWillChange();
      return;
    }

    
    mDataHolder.Swap(mDrawTarget->mData);
    mData = mDataHolder;
    mCg = nullptr;
    mDrawTarget = nullptr;
    
  }
}

SourceSurfaceCGBitmapContext::~SourceSurfaceCGBitmapContext()
{
  if (mImage)
    CGImageRelease(mImage);
}

SourceSurfaceCGIOSurfaceContext::SourceSurfaceCGIOSurfaceContext(DrawTargetCG *aDrawTarget)
{
  CGContextRef cg = (CGContextRef)aDrawTarget->GetNativeSurface(NativeSurfaceType::CGCONTEXT_ACCELERATED);

  RefPtr<MacIOSurface> surf = MacIOSurface::IOSurfaceContextGetSurface(cg);

  mFormat = aDrawTarget->GetFormat();
  mSize.width = surf->GetWidth();
  mSize.height = surf->GetHeight();

  
  
  mImage = nullptr;

  aDrawTarget->Flush();
  surf->Lock();
  size_t bytesPerRow = surf->GetBytesPerRow();
  size_t ioHeight = surf->GetHeight();
  void* ioData = surf->GetBaseAddress();
  
  
  mData = malloc(ioHeight*bytesPerRow);
  memcpy(mData, ioData, ioHeight*(bytesPerRow));
  mStride = bytesPerRow;
  surf->Unlock();
}

void SourceSurfaceCGIOSurfaceContext::EnsureImage() const
{
  

  
  
  
  
  
  
  
  if (!mImage) {
    mImage = CreateCGImage(mData, mData, mSize, mStride, SurfaceFormat::B8G8R8A8);
  }

}

IntSize
SourceSurfaceCGIOSurfaceContext::GetSize() const
{
  return mSize;
}

void
SourceSurfaceCGIOSurfaceContext::DrawTargetWillChange()
{
}

SourceSurfaceCGIOSurfaceContext::~SourceSurfaceCGIOSurfaceContext()
{
  if (mImage)
    CGImageRelease(mImage);
  else
    free(mData);
}

unsigned char*
SourceSurfaceCGIOSurfaceContext::GetData()
{
  return (unsigned char*)mData;
}

}
}
