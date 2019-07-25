




































#ifndef MOZILLA_GFX_SOURCESURFACESKIA_H_
#define MOZILLA_GFX_SOURCESURFACESKIA_H_

#include "2D.h"
#include <vector>
#include "skia/SkCanvas.h"
#include "skia/SkBitmap.h"

namespace mozilla {
namespace gfx {

class SourceSurfaceSkia : public DataSourceSurface
{
public:
  SourceSurfaceSkia();
  ~SourceSurfaceSkia();

  virtual SurfaceType GetType() const { return SURFACE_SKIA; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const;

  SkBitmap& GetBitmap() { return mBitmap; }

  bool InitFromData(unsigned char* aData,
                    const IntSize &aSize,
                    int32_t aStride,
                    SurfaceFormat aFormat);

  bool InitWithBitmap(SkCanvas* aBitmap,
                      SurfaceFormat aFormat);


  virtual unsigned char *GetData();

  virtual int32_t Stride() { return mStride; }

private:
  friend class DrawTargetSkia;

  SkBitmap mBitmap;
  SurfaceFormat mFormat;
  IntSize mSize;
  int32_t mStride;
};

}
}

#endif 
