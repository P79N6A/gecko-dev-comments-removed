




































#include "SourceSurfaceCG.h"

namespace mozilla {
namespace gfx {

SourceSurfaceCG::SourceSurfaceCG()
{
}

SourceSurfaceCG::~SourceSurfaceCG()
{
  CGImageRelease(mImage);
}

IntSize
SourceSurfaceCG::GetSize() const
{
  IntSize size;
  size.width = CGImageGetHeight(mImage);
  size.height = CGImageGetWidth(mImage);
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
  return NULL;
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

  switch (aFormat) {
    case B8G8R8A8:
      colorSpace = CGColorSpaceCreateDeviceRGB();
      bitinfo = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host;
      bitsPerComponent = 8;
      bitsPerPixel = 32;
      break;

    case B8G8R8X8:
      colorSpace = CGColorSpaceCreateDeviceRGB();
      bitinfo = kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host;
      bitsPerComponent = 8;
      bitsPerPixel = 32;
      break;

    case A8:
      
      bitsPerComponent = 8;
      bitsPerPixel = 8;
  };

  void *data = malloc(aStride * aSize.height);
  memcpy(aData, data, aStride * aSize.height);

  mFormat = aFormat;

  dataProvider = CGDataProviderCreateWithData (data,
                                               data,
					       aSize.height * aStride,
					       releaseCallback);

  if (aFormat == A8) {
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

  if (mImage) {
    return false;
  }

  return true;
}

}
}
