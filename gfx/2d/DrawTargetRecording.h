




#ifndef MOZILLA_GFX_DRAWTARGETRECORDING_H_
#define MOZILLA_GFX_DRAWTARGETRECORDING_H_

#include "2D.h"
#include "DrawEventRecorder.h"

namespace mozilla {
namespace gfx {

class DrawTargetRecording : public DrawTarget
{
public:
  DrawTargetRecording(DrawEventRecorder *aRecorder, DrawTarget *aDT);
  ~DrawTargetRecording();

  virtual BackendType GetType() const { return mFinalDT->GetType(); }

  virtual TemporaryRef<SourceSurface> Snapshot();

  virtual IntSize GetSize() { return mFinalDT->GetSize(); }

  



  virtual void Flush() { mFinalDT->Flush(); }

  










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
                          const DrawOptions &aOptions = DrawOptions(),
                          const GlyphRenderingOptions *aRenderingOptions = NULL);

  








  virtual void Mask(const Pattern &aSource,
                    const Pattern &aMask,
                    const DrawOptions &aOptions = DrawOptions());

  




  virtual void PushClip(const Path *aPath);

  





  virtual void PushClipRect(const Rect &aRect);

  


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

  









  virtual TemporaryRef<GradientStops>
    CreateGradientStops(GradientStop *aStops,
                        uint32_t aNumStops,
                        ExtendMode aExtendMode = EXTEND_CLAMP) const;

  



  virtual void SetTransform(const Matrix &aTransform);

  


  virtual void *GetNativeSurface(NativeSurfaceType aType) { return mFinalDT->GetNativeSurface(aType); }

private:
  Path *GetPathForPathRecording(const Path *aPath) const;
  void EnsureStored(const Path *aPath);

  RefPtr<DrawEventRecorderPrivate> mRecorder;
  RefPtr<DrawTarget> mFinalDT;
};

}
}

#endif 
