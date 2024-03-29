




#ifndef MOZILLA_GFX_DRAWTARGETCAPTURE_H_
#define MOZILLA_GFX_DRAWTARGETCAPTURE_H_

#include "2D.h"
#include <vector>

#include "Filters.h"

namespace mozilla {
namespace gfx {

class DrawingCommand;

class DrawTargetCaptureImpl : public DrawTargetCapture
{
public:
  DrawTargetCaptureImpl()
  {}

  bool Init(const IntSize& aSize, DrawTarget* aRefDT);

  virtual BackendType GetBackendType() const { return mRefDT->GetBackendType(); }
  virtual DrawTargetType GetType() const { return mRefDT->GetType(); }

  virtual already_AddRefed<SourceSurface> Snapshot();
  virtual IntSize GetSize() { return mSize; }

  virtual void Flush() {}
  virtual void DrawSurface(SourceSurface *aSurface,
                           const Rect &aDest,
                           const Rect &aSource,
                           const DrawSurfaceOptions &aSurfOptions,
                           const DrawOptions &aOptions);
  virtual void DrawFilter(FilterNode *aNode,
                          const Rect &aSourceRect,
                          const Point &aDestPoint,
                          const DrawOptions &aOptions = DrawOptions());
  virtual void DrawSurfaceWithShadow(SourceSurface *aSurface,
                                     const Point &aDest,
                                     const Color &aColor,
                                     const Point &aOffset,
                                     Float aSigma,
                                     CompositionOp aOperator) {  }

  virtual void ClearRect(const Rect &aRect);
  virtual void MaskSurface(const Pattern &aSource,
                           SourceSurface *aMask,
                           Point aOffset,
                           const DrawOptions &aOptions = DrawOptions());

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
                          const DrawOptions &aOptions = DrawOptions(),
                          const GlyphRenderingOptions *aRenderingOptions = nullptr);
  virtual void Mask(const Pattern &aSource,
                    const Pattern &aMask,
                    const DrawOptions &aOptions = DrawOptions());
  virtual void PushClip(const Path *aPath);
  virtual void PushClipRect(const Rect &aRect);
  virtual void PopClip();

  virtual void SetTransform(const Matrix &aTransform);

  virtual already_AddRefed<SourceSurface> CreateSourceSurfaceFromData(unsigned char *aData,
                                                                  const IntSize &aSize,
                                                                  int32_t aStride,
                                                                  SurfaceFormat aFormat) const
  {
    return mRefDT->CreateSourceSurfaceFromData(aData, aSize, aStride, aFormat);
  }
  virtual already_AddRefed<SourceSurface> OptimizeSourceSurface(SourceSurface *aSurface) const
  {
    return mRefDT->OptimizeSourceSurface(aSurface);
  }

  virtual already_AddRefed<SourceSurface>
    CreateSourceSurfaceFromNativeSurface(const NativeSurface &aSurface) const
  {
    return mRefDT->CreateSourceSurfaceFromNativeSurface(aSurface);
  }

  virtual already_AddRefed<DrawTarget>
    CreateSimilarDrawTarget(const IntSize &aSize, SurfaceFormat aFormat) const
  {
    return mRefDT->CreateSimilarDrawTarget(aSize, aFormat);
  }

  virtual already_AddRefed<PathBuilder> CreatePathBuilder(FillRule aFillRule = FillRule::FILL_WINDING) const
  {
    return mRefDT->CreatePathBuilder(aFillRule);
  }

  virtual already_AddRefed<GradientStops>
    CreateGradientStops(GradientStop *aStops,
                        uint32_t aNumStops,
                        ExtendMode aExtendMode = ExtendMode::CLAMP) const
  {
    return mRefDT->CreateGradientStops(aStops, aNumStops, aExtendMode);
  }
  virtual already_AddRefed<FilterNode> CreateFilter(FilterType aType)
  {
    return mRefDT->CreateFilter(aType);
  }

  void ReplayToDrawTarget(DrawTarget* aDT, const Matrix& aTransform);

protected:
  ~DrawTargetCaptureImpl();

private:

  
  
  
  template<typename T>
  T* AppendToCommandList()
  {
    size_t oldSize = mDrawCommandStorage.size();
    mDrawCommandStorage.resize(mDrawCommandStorage.size() + sizeof(T) + sizeof(uint32_t));
    uint8_t* nextDrawLocation = &mDrawCommandStorage.front() + oldSize;
    *(uint32_t*)(nextDrawLocation) = sizeof(T) + sizeof(uint32_t);
    return reinterpret_cast<T*>(nextDrawLocation + sizeof(uint32_t));
  }
  RefPtr<DrawTarget> mRefDT;

  IntSize mSize;

  std::vector<uint8_t> mDrawCommandStorage;
};

} 

} 


#endif 
