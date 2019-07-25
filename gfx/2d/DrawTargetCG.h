




































#pragma once

#include <ApplicationServices/ApplicationServices.h>

#include "2D.h"
#include "Rect.h"
namespace mozilla {
namespace gfx {

class DrawTargetCG : public DrawTarget
{
public:
  DrawTargetCG();
  virtual ~DrawTargetCG();

  virtual BackendType GetType() const { return COREGRAPHICS; }
  virtual TemporaryRef<SourceSurface> Snapshot();

  virtual void DrawSurface(SourceSurface *aSurface,
                           const Rect &aDest,
                           const Rect &aSource,
                           const DrawOptions &aOptions = DrawOptions(),
                           const DrawSurfaceOptions &aSurfOptions = DrawSurfaceOptions());

  virtual void FillRect(const Rect &aRect,
                        const Pattern &aPattern,
                        const DrawOptions &aOptions = DrawOptions());


  bool Init(const IntSize &aSize);
  bool Init(CGContextRef cgContext, const IntSize &aSize);

  
  virtual TemporaryRef<SourceSurface> CreateSourceSurfaceFromData(unsigned char *aData,
                                                            const IntSize &aSize,
                                                            int32_t aStride,
                                                            SurfaceFormat aFormat) const;
  virtual TemporaryRef<SourceSurface> OptimizeSourceSurface(SourceSurface *aSurface) const;
private:
  bool InitCGRenderTarget();

  IntSize mSize;
  CGContextRef mCg;

};

}
}
