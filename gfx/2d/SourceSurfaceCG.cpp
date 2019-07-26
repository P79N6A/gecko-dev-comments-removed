




#include "SourceSurfaceCG.h"
#include "DrawTargetCG.h"

#include "QuartzSupport.h"

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

TemporaryRef<DataSourceSurface>
SourceSurfaceCG::GetDataSurface()
{
  
  CGImageRetain(mImage);
  RefPtr<DataSourceSurfaceCG> dataSurf =
    new DataSourceSurfaceCG(mImage);
  return dataSurf;
}

static void releaseCallback(void *info, const void *data, size_t size) {
  free(info);
}

static void
AssignSurfaceParametersFromFormat(SurfaceFormat aFormat,
                                  CGColorSpaceRef &aColorSpace,
                                  CGBitmapInfo &aBitinfo,
                                  int &aBitsPerComponent,
                                  int &aBitsPerPixel)
{
  switch (aFormat) {
    case FORMAT_B8G8R8A8:
      aColorSpace = CGColorSpaceCreateDeviceRGB();
      aBitinfo = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host;
      aBitsPerComponent = 8;
      aBitsPerPixel = 32;
      break;

    case FORMAT_B8G8R8X8:
      aColorSpace = CGColorSpaceCreateDeviceRGB();
      aBitinfo = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host;
      aBitsPerComponent = 8;
      aBitsPerPixel = 32;
      break;

    case FORMAT_A8:
      
      aBitsPerComponent = 8;
      aBitsPerPixel = 8;
      break;

    default:
      MOZ_CRASH();
  }
}

static CGImageRef
CreateCGImage(void *aInfo,
              const void *aData,
              const IntSize &aSize,
              int32_t aStride,
              SurfaceFormat aFormat)
{
  
  CGColorSpaceRef colorSpace = nullptr;
  CGBitmapInfo bitinfo = 0;
  int bitsPerComponent = 0;
  int bitsPerPixel = 0;

  AssignSurfaceParametersFromFormat(aFormat, colorSpace, bitinfo,
                                    bitsPerComponent, bitsPerPixel);

  CGDataProviderRef dataProvider = CGDataProviderCreateWithData(aInfo,
                                                                aData,
                                                                aSize.height * aStride,
                                                                releaseCallback);

  CGImageRef image;
  if (aFormat == FORMAT_A8) {
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

  void *data = malloc(aStride * aSize.height);
  memcpy(data, aData, aStride * aSize.height);

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
  void *data = malloc(aStride * aSize.height);
  memcpy(data, aData, aStride * aSize.height);

  mImage = CreateCGImage(data, data, aSize, aStride, aFormat);

  return mImage;
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
  mCg = (CGContextRef)aDrawTarget->GetNativeSurface(NATIVE_SURFACE_CGCONTEXT);
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
    void *info;
    if (mCg) {
      
      
      
      info = nullptr;
    } else {
      
      
      info = mData;
    }

    if (!mData) abort();

    mImage = CreateCGImage(info, mData, mSize, mStride, FORMAT_B8G8R8A8);
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

    
    mData = malloc(stride * height);

    
    
    
    memcpy(mData, CGBitmapContextGetData(mCg), stride*height);

    
    if (mImage)
      CGImageRelease(mImage);
    mImage = nullptr;

    mCg = nullptr;
    mDrawTarget = nullptr;
  }
}

SourceSurfaceCGBitmapContext::~SourceSurfaceCGBitmapContext()
{
  if (!mImage && !mCg) {
    
    free(mData);
  }
  if (mImage)
    CGImageRelease(mImage);
}

SourceSurfaceCGIOSurfaceContext::SourceSurfaceCGIOSurfaceContext(DrawTargetCG *aDrawTarget)
{
  CGContextRef cg = (CGContextRef)aDrawTarget->GetNativeSurface(NATIVE_SURFACE_CGCONTEXT_ACCELERATED);

  RefPtr<MacIOSurface> surf = MacIOSurface::IOSurfaceContextGetSurface(cg);

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
    mImage = CreateCGImage(mData, mData, mSize, mStride, FORMAT_B8G8R8A8);
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
