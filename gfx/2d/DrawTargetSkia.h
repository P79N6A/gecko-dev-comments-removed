




































#pragma once

#include "skia/SkCanvas.h"
#include "2D.h"
#include "Rect.h"
#include "PathSkia.h"
#include <sstream>
#include <vector>
using namespace std;
#include "gfxImageSurface.h"

namespace mozilla {
namespace gfx {

class SourceSurfaceSkia;

class DrawTargetSkia : public DrawTarget
{
public:
  DrawTargetSkia();
  virtual ~DrawTargetSkia();

  virtual BackendType GetType() const { return BACKEND_SKIA; }
  virtual TemporaryRef<SourceSurface> Snapshot();
  virtual IntSize GetSize() { return mSize; }
  virtual void Flush();
  virtual void DrawSurface(SourceSurface *aSurface,
                           const Rect &aDest,
                           const Rect &aSource,
                           const DrawSurfaceOptions &aSurfOptions = DrawSurfaceOptions(),
                           const DrawOptions &aOptions = DrawOptions());
  virtual void DrawSurfaceWithShadow(SourceSurface *aSurface,
                                     const Point &aDest,
                                     const Color &aColor,
                                     const Point &aOffset,
                                     Float aSigma,
                                     CompositionOp aOperator);
  virtual void ClearRect(const Rect &aRect);
  virtual void CopySurface(SourceSurface *aSurface,
                           const IntRect &aSourceRect,
                           const IntPoint &aDestination);
  virtual void FillRect(const Rect &aRect,
                        const Pattern &aPattern,
                        const DrawOptions &aOptions = DrawOptions());
  virtual void StrokeRect(const Rect &aRect,
                          const Pattern &aPattern,
                          const StrokeOptions &aStrokeOptions = StrokeOptions(),
                          const DrawOptions &aOptions = DrawOptions());
  virtual void StrokeLine(const Point &aStart,
                          const Point &aEnd,
                          const Pattern &aPattern,
                          const StrokeOptions &aStrokeOptions = StrokeOptions(),
                          const DrawOptions &aOptions = DrawOptions());
  virtual void Stroke(const Path *aPath,
                      const Pattern &aPattern,
                      const StrokeOptions &aStrokeOptions = StrokeOptions(),
                      const DrawOptions &aOptions = DrawOptions());
  virtual void Fill(const Path *aPath,
                    const Pattern &aPattern,
                    const DrawOptions &aOptions = DrawOptions());
  virtual void FillGlyphs(ScaledFont *aFont,
                          const GlyphBuffer &aBuffer,
                          const Pattern &aPattern,
                          const DrawOptions &aOptions = DrawOptions());
  virtual void PushClip(const Path *aPath);
  virtual void PopClip();
  virtual TemporaryRef<SourceSurface> CreateSourceSurfaceFromData(unsigned char *aData,
                                                            const IntSize &aSize,
                                                            int32_t aStride,
                                                            SurfaceFormat aFormat) const;
  virtual TemporaryRef<SourceSurface> OptimizeSourceSurface(SourceSurface *aSurface) const;
  virtual TemporaryRef<SourceSurface>
    CreateSourceSurfaceFromNativeSurface(const NativeSurface &aSurface) const;
  virtual TemporaryRef<DrawTarget>
    CreateSimilarDrawTarget(const IntSize &aSize, SurfaceFormat aFormat) const;
  virtual TemporaryRef<PathBuilder> CreatePathBuilder(FillRule aFillRule = FILL_WINDING) const;
  virtual TemporaryRef<GradientStops> CreateGradientStops(GradientStop *aStops, uint32_t aNumStops) const;
  virtual void SetTransform(const Matrix &aTransform);

  bool Init(const IntSize &aSize, SurfaceFormat aFormat);
  
  operator std::string() const {
    std::stringstream stream;
    stream << "DrawTargetSkia(" << this << ")";
    return stream.str();
  }
private:
  friend class SourceSurfaceSkia;
  void AppendSnapshot(SourceSurfaceSkia* aSnapshot);
  void RemoveSnapshot(SourceSurfaceSkia* aSnapshot);

  void MarkChanged();

  IntSize mSize;
  SkBitmap mBitmap;
  SkRefPtr<SkCanvas> mCanvas;
  SkRefPtr<SkDevice> mDevice;
  nsRefPtr<gfxImageSurface> mImageSurface;
  SurfaceFormat mFormat;
  vector<SourceSurfaceSkia*> mSnapshots;
};

}
}
