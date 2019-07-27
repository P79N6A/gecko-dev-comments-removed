




#ifndef mozilla_gfx_DrawTargetCG_h
#define mozilla_gfx_DrawTargetCG_h

#include <ApplicationServices/ApplicationServices.h>

#include "2D.h"
#include "Rect.h"
#include "PathCG.h"
#include "SourceSurfaceCG.h"
#include "GLDefs.h"
#include "Tools.h"

namespace mozilla {
namespace gfx {

static inline CGAffineTransform
GfxMatrixToCGAffineTransform(Matrix m)
{
  CGAffineTransform t;
  t.a = m._11;
  t.b = m._12;
  t.c = m._21;
  t.d = m._22;
  t.tx = m._31;
  t.ty = m._32;
  return t;
}

static inline Rect
CGRectToRect(CGRect rect)
{
  return Rect(rect.origin.x,
              rect.origin.y,
              rect.size.width,
              rect.size.height);
}

static inline Point
CGPointToPoint(CGPoint point)
{
  return Point(point.x, point.y);
}

static inline void
SetStrokeOptions(CGContextRef cg, const StrokeOptions &aStrokeOptions)
{
  switch (aStrokeOptions.mLineCap)
  {
    case CapStyle::BUTT:
      CGContextSetLineCap(cg, kCGLineCapButt);
      break;
    case CapStyle::ROUND:
      CGContextSetLineCap(cg, kCGLineCapRound);
      break;
    case CapStyle::SQUARE:
      CGContextSetLineCap(cg, kCGLineCapSquare);
      break;
  }

  switch (aStrokeOptions.mLineJoin)
  {
    case JoinStyle::BEVEL:
      CGContextSetLineJoin(cg, kCGLineJoinBevel);
      break;
    case JoinStyle::ROUND:
      CGContextSetLineJoin(cg, kCGLineJoinRound);
      break;
    case JoinStyle::MITER:
    case JoinStyle::MITER_OR_BEVEL:
      CGContextSetLineJoin(cg, kCGLineJoinMiter);
      break;
  }

  CGContextSetLineWidth(cg, aStrokeOptions.mLineWidth);
  CGContextSetMiterLimit(cg, aStrokeOptions.mMiterLimit);

  
  if (aStrokeOptions.mDashLength > 0) {
    
    CGFloat *dashes = new CGFloat[aStrokeOptions.mDashLength];
    for (size_t i=0; i<aStrokeOptions.mDashLength; i++) {
      dashes[i] = aStrokeOptions.mDashPattern[i];
    }
    CGContextSetLineDash(cg, aStrokeOptions.mDashOffset, dashes, aStrokeOptions.mDashLength);
    delete[] dashes;
  }
}

class GlyphRenderingOptionsCG : public GlyphRenderingOptions
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(GlyphRenderingOptionsCG, override)

  explicit GlyphRenderingOptionsCG(const Color &aFontSmoothingBackgroundColor)
    : mFontSmoothingBackgroundColor(aFontSmoothingBackgroundColor)
  {}

  const Color &FontSmoothingBackgroundColor() const { return mFontSmoothingBackgroundColor; }

  virtual FontType GetType() const override { return FontType::MAC; }

private:
  Color mFontSmoothingBackgroundColor;
};

class DrawTargetCG : public DrawTarget
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DrawTargetCG, override)
  friend class BorrowedCGContext;
  friend class SourceSurfaceCGBitmapContext;
  DrawTargetCG();
  virtual ~DrawTargetCG();

  virtual DrawTargetType GetType() const override;
  virtual BackendType GetBackendType() const override;
  virtual TemporaryRef<SourceSurface> Snapshot() override;

  virtual void DrawSurface(SourceSurface *aSurface,
                           const Rect &aDest,
                           const Rect &aSource,
                           const DrawSurfaceOptions &aSurfOptions = DrawSurfaceOptions(),
                           const DrawOptions &aOptions = DrawOptions()) override;
  virtual void DrawFilter(FilterNode *aNode,
                          const Rect &aSourceRect,
                          const Point &aDestPoint,
                          const DrawOptions &aOptions = DrawOptions()) override;
  virtual void MaskSurface(const Pattern &aSource,
                           SourceSurface *aMask,
                           Point aOffset,
                           const DrawOptions &aOptions = DrawOptions()) override;

  virtual void FillRect(const Rect &aRect,
                        const Pattern &aPattern,
                        const DrawOptions &aOptions = DrawOptions()) override;


  
  bool Init(BackendType aType, const IntSize &aSize, SurfaceFormat&);
  bool Init(BackendType aType, unsigned char* aData, const IntSize &aSize, int32_t aStride, SurfaceFormat aFormat);
  bool Init(CGContextRef cgContext, const IntSize &aSize);

  
  virtual void Flush() override;

  virtual void DrawSurfaceWithShadow(SourceSurface *, const Point &, const Color &, const Point &, Float, CompositionOp) override;
  virtual void ClearRect(const Rect &) override;
  virtual void CopySurface(SourceSurface *, const IntRect&, const IntPoint&) override;
  virtual void StrokeRect(const Rect &, const Pattern &, const StrokeOptions&, const DrawOptions&) override;
  virtual void StrokeLine(const Point &, const Point &, const Pattern &, const StrokeOptions &, const DrawOptions &) override;
  virtual void Stroke(const Path *, const Pattern &, const StrokeOptions &, const DrawOptions &) override;
  virtual void Fill(const Path *, const Pattern &, const DrawOptions &) override;
  virtual void FillGlyphs(ScaledFont *, const GlyphBuffer&, const Pattern &, const DrawOptions &, const GlyphRenderingOptions *) override;
  virtual void Mask(const Pattern &aSource,
                    const Pattern &aMask,
                    const DrawOptions &aOptions = DrawOptions()) override;
  virtual void PushClip(const Path *) override;
  virtual void PushClipRect(const Rect &aRect) override;
  virtual void PopClip() override;
  virtual TemporaryRef<SourceSurface> CreateSourceSurfaceFromNativeSurface(const NativeSurface&) const override { return nullptr;}
  virtual TemporaryRef<DrawTarget> CreateSimilarDrawTarget(const IntSize &, SurfaceFormat) const override;
  virtual TemporaryRef<PathBuilder> CreatePathBuilder(FillRule) const override;
  virtual TemporaryRef<GradientStops> CreateGradientStops(GradientStop *, uint32_t,
                                                          ExtendMode aExtendMode = ExtendMode::CLAMP) const override;
  virtual TemporaryRef<FilterNode> CreateFilter(FilterType aType) override;

  virtual void *GetNativeSurface(NativeSurfaceType) override;

  virtual IntSize GetSize() override { return mSize; }


  
  virtual TemporaryRef<SourceSurface> CreateSourceSurfaceFromData(unsigned char *aData,
                                                            const IntSize &aSize,
                                                            int32_t aStride,
                                                            SurfaceFormat aFormat) const override;
  virtual TemporaryRef<SourceSurface> OptimizeSourceSurface(SourceSurface *aSurface) const override;
  CGContextRef GetCGContext() {
      return mCg;
  }

  
  
  static size_t GetMaxSurfaceSize() {
    return 32767;
  }

private:
  void MarkChanged();

  IntSize mSize;
  CGColorSpaceRef mColorSpace;
  CGContextRef mCg;

  





  AlignedArray<uint8_t> mData;

  RefPtr<SourceSurfaceCGContext> mSnapshot;
  bool mMayContainInvalidPremultipliedData;

#ifdef DEBUG
  std::vector<CGRect> mSavedClipBounds;
#endif
};

}
}

#endif

