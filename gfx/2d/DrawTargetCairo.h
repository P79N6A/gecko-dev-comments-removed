




#ifndef _MOZILLA_GFX_DRAWTARGET_CAIRO_H_
#define _MOZILLA_GFX_DRAWTARGET_CAIRO_H_

#include "2D.h"
#include "cairo.h"
#include "PathCairo.h"

#include <vector>

namespace mozilla {
namespace gfx {

class SourceSurfaceCairo;

class GradientStopsCairo : public GradientStops
{
  public:
    GradientStopsCairo(GradientStop* aStops, uint32_t aNumStops,
                       ExtendMode aExtendMode)
     : mExtendMode(aExtendMode)
    {
      for (uint32_t i = 0; i < aNumStops; ++i) {
        mStops.push_back(aStops[i]);
      }
    }

    virtual ~GradientStopsCairo() {}

    const std::vector<GradientStop>& GetStops() const
    {
      return mStops;
    }

    ExtendMode GetExtendMode() const
    {
      return mExtendMode;
    }

    virtual BackendType GetBackendType() const { return BACKEND_CAIRO; }

  private:
    std::vector<GradientStop> mStops;
    ExtendMode mExtendMode;
};

class DrawTargetCairo : public DrawTarget
{
public:
  friend class BorrowedCairoContext;

  DrawTargetCairo();
  virtual ~DrawTargetCairo();

  virtual BackendType GetType() const { return BACKEND_CAIRO; }
  virtual TemporaryRef<SourceSurface> Snapshot();
  virtual IntSize GetSize();

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
                          const DrawOptions &aOptions,
                          const GlyphRenderingOptions *aRenderingOptions = nullptr);
  virtual void Mask(const Pattern &aSource,
                    const Pattern &aMask,
                    const DrawOptions &aOptions = DrawOptions());
  virtual void MaskSurface(const Pattern &aSource,
                           SourceSurface *aMask,
                           Point aOffset,
                           const DrawOptions &aOptions = DrawOptions());

  virtual void PushClip(const Path *aPath);
  virtual void PushClipRect(const Rect &aRect);
  virtual void PopClip();

  virtual TemporaryRef<PathBuilder> CreatePathBuilder(FillRule aFillRule = FILL_WINDING) const;

  virtual TemporaryRef<SourceSurface> CreateSourceSurfaceFromData(unsigned char *aData,
                                                            const IntSize &aSize,
                                                            int32_t aStride,
                                                            SurfaceFormat aFormat) const;
  virtual TemporaryRef<SourceSurface> OptimizeSourceSurface(SourceSurface *aSurface) const;
  virtual TemporaryRef<SourceSurface>
    CreateSourceSurfaceFromNativeSurface(const NativeSurface &aSurface) const;
  virtual TemporaryRef<DrawTarget>
    CreateSimilarDrawTarget(const IntSize &aSize, SurfaceFormat aFormat) const;
  virtual TemporaryRef<DrawTarget>
    CreateShadowDrawTarget(const IntSize &aSize, SurfaceFormat aFormat,
                           float aSigma) const;

  virtual TemporaryRef<GradientStops>
    CreateGradientStops(GradientStop *aStops,
                        uint32_t aNumStops,
                        ExtendMode aExtendMode = EXTEND_CLAMP) const;

  virtual void *GetNativeSurface(NativeSurfaceType aType);

  bool Init(cairo_surface_t* aSurface, const IntSize& aSize);

  void SetPathObserver(CairoPathContext* aPathObserver);

  virtual void SetTransform(const Matrix& aTransform);

  
  
  
  void PrepareForDrawing(cairo_t* aContext, const Path* aPath = nullptr);

private: 
  
  bool InitAlreadyReferenced(cairo_surface_t* aSurface, const IntSize& aSize);

  enum DrawPatternType { DRAW_FILL, DRAW_STROKE };
  void DrawPattern(const Pattern& aPattern,
                   const StrokeOptions& aStrokeOptions,
                   const DrawOptions& aOptions,
                   DrawPatternType aDrawType);

  
  
  
  void WillChange(const Path* aPath = nullptr);

  
  
  void MarkSnapshotIndependent();

  
  
  void ClearSurfaceForUnboundedSource(const CompositionOp &aOperator);
private: 
  cairo_t* mContext;
  cairo_surface_t* mSurface;
  IntSize mSize;

  
  
  RefPtr<SourceSurfaceCairo> mSnapshot;

  
  
  
  
  mutable CairoPathContext* mPathObserver;
};

}
}

#endif 
