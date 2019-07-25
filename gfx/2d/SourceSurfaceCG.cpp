




































#include "SourceSurfaceCG.h"

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

bool
SourceSurfaceCG::InitFromData(unsigned char *aData,
                               const IntSize &aSize,
                               int32_t aStride,
                               SurfaceFormat aFormat)
{
  
  CGColorSpaceRef colorSpace = NULL;
  CGBitmapInfo bitinfo = 0;
  CGDataProviderRef dataProvider = NULL;
  int bitsPerComponent = 0;
  int bitsPerPixel = 0;

  assert(aSize.width >= 0 && aSize.height >= 0);

  switch (aFormat) {
    case FORMAT_B8G8R8A8:
      colorSpace = CGColorSpaceCreateDeviceRGB();
      bitinfo = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host;
      bitsPerComponent = 8;
      bitsPerPixel = 32;
      break;

    case FORMAT_B8G8R8X8:
      colorSpace = CGColorSpaceCreateDeviceRGB();
      bitinfo = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host;
      bitsPerComponent = 8;
      bitsPerPixel = 32;
      break;

    case FORMAT_A8:
      
      bitsPerComponent = 8;
      bitsPerPixel = 8;
  };

  void *data = malloc(aStride * aSize.height);
  memcpy(data, aData, aStride * aSize.height);

  mFormat = aFormat;

  dataProvider = CGDataProviderCreateWithData (data,
                                               data,
					       aSize.height * aStride,
					       releaseCallback);

  if (aFormat == FORMAT_A8) {
    CGFloat decode[] = {1.0, 0.0};
    mImage = CGImageMaskCreate (aSize.width, aSize.height,
				bitsPerComponent,
				bitsPerPixel,
				aStride,
				dataProvider,
				decode,
				true);

  } else {
    mImage = CGImageCreate (aSize.width, aSize.height,
			    bitsPerComponent,
			    bitsPerPixel,
			    aStride,
			    colorSpace,
			    bitinfo,
			    dataProvider,
			    NULL,
			    true,
			    kCGRenderingIntentDefault);
  }

  CGDataProviderRelease(dataProvider);
  CGColorSpaceRelease (colorSpace);

  return mImage != NULL;
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
  
  CGColorSpaceRef colorSpace = NULL;
  CGBitmapInfo bitinfo = 0;
  CGDataProviderRef dataProvider = NULL;
  int bitsPerComponent = 0;
  int bitsPerPixel = 0;

  switch (aFormat) {
    case FORMAT_B8G8R8A8:
      colorSpace = CGColorSpaceCreateDeviceRGB();
      bitinfo = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host;
      bitsPerComponent = 8;
      bitsPerPixel = 32;
      break;

    case FORMAT_B8G8R8X8:
      colorSpace = CGColorSpaceCreateDeviceRGB();
      bitinfo = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host;
      bitsPerComponent = 8;
      bitsPerPixel = 32;
      break;

    case FORMAT_A8:
      
      bitsPerComponent = 8;
      bitsPerPixel = 8;
  };

  void *data = malloc(aStride * aSize.height);
  memcpy(data, aData, aStride * aSize.height);

  

  dataProvider = CGDataProviderCreateWithData (data,
                                               data,
					       aSize.height * aStride,
					       releaseCallback);

  if (aFormat == FORMAT_A8) {
    CGFloat decode[] = {1.0, 0.0};
    mImage = CGImageMaskCreate (aSize.width, aSize.height,
				bitsPerComponent,
				bitsPerPixel,
				aStride,
				dataProvider,
				decode,
				true);

  } else {
    mImage = CGImageCreate (aSize.width, aSize.height,
			    bitsPerComponent,
			    bitsPerPixel,
			    aStride,
			    colorSpace,
			    bitinfo,
			    dataProvider,
			    NULL,
			    true,
			    kCGRenderingIntentDefault);
  }

  CGDataProviderRelease(dataProvider);
  CGColorSpaceRelease (colorSpace);

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
  if (mCg == NULL) {
    
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



}
}
