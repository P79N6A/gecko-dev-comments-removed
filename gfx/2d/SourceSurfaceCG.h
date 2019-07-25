




































#pragma once

#include <ApplicationServices/ApplicationServices.h>

#include "2D.h"

namespace mozilla {
namespace gfx {

class SourceSurfaceCG : public SourceSurface
{
public:
  SourceSurfaceCG();
  ~SourceSurfaceCG();

  virtual SurfaceType GetType() const { return COREGRAPHICS_IMAGE; }
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

}
}
