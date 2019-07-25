




































#pragma once

#include <ApplicationServices/ApplicationServices.h>

#include "2D.h"

namespace mozilla {
namespace gfx {

class SourceSurfaceCG : public SourceSurface
{
public:
  SourceSurfaceCG() {}
  SourceSurfaceCG(CGImageRef aImage) : mImage(aImage) {}
  ~SourceSurfaceCG();

  virtual SurfaceType GetType() const { return SURFACE_COREGRAPHICS_IMAGE; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const;
  virtual TemporaryRef<DataSourceSurface> GetDataSurface();

  CGImageRef GetImage() { return mImage; }

  bool InitFromData(unsigned char *aData,
                    const IntSize &aSize,
                    int32_t aStride,
                    SurfaceFormat aFormat);

private:
  CGImageRef mImage;

  


  SurfaceFormat mFormat;
};

class DataSourceSurfaceCG : public DataSourceSurface
{
public:
  DataSourceSurfaceCG() {}
  DataSourceSurfaceCG(CGImageRef aImage);
  ~DataSourceSurfaceCG();

  virtual SurfaceType GetType() const { return SURFACE_DATA; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const { return FORMAT_B8G8R8A8; }

  CGImageRef GetImage() { return mImage; }

  bool InitFromData(unsigned char *aData,
                    const IntSize &aSize,
                    int32_t aStride,
                    SurfaceFormat aFormat);

  virtual unsigned char *GetData();

  virtual int32_t Stride() { return CGImageGetBytesPerRow(mImage); }


private:
  CGContextRef mCg;
  CGImageRef mImage;
  
  void *mData;
  


};

}
}
