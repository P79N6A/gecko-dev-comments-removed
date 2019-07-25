




































#ifndef _MOZILLA_GFX_2D_H
#define _MOZILLA_GFX_2D_H

#include "Point.h"
#include "Rect.h"
#include "Matrix.h"




#include "mozilla/RefPtr.h"

struct _cairo_surface;
typedef _cairo_surface cairo_surface_t;

struct ID3D10Device1;
struct ID3D10Texture2D;

namespace mozilla {
namespace gfx {

class SourceSurface;
class DataSourceSurface;
class DrawTarget;

struct NativeSurface {
  NativeSurfaceType mType;
  SurfaceFormat mFormat;
  void *mSurface;
};

struct NativeFont {
  NativeFontType mType;
  void *mFont;
};












struct DrawOptions {
  DrawOptions(Float aAlpha = 1.0f,
              CompositionOp aCompositionOp = OP_OVER,
              AntialiasMode aAntialiasMode = AA_GRAY,
              Snapping aSnapping = SNAP_NONE)
    : mAlpha(aAlpha)
    , mCompositionOp(aCompositionOp)
    , mAntialiasMode(aAntialiasMode)
    , mSnapping(aSnapping)
  {}

  Float mAlpha;
  CompositionOp mCompositionOp : 8;
  AntialiasMode mAntialiasMode : 2;
  Snapping mSnapping : 1;
};










struct StrokeOptions {
  StrokeOptions(Float aLineWidth = 1.0f,
                JoinStyle aLineJoin = JOIN_MITER_OR_BEVEL,
                CapStyle aLineCap = CAP_BUTT,
                Float aMiterLimit = 10.0f)
    : mLineWidth(aLineWidth)
    , mMiterLimit(aMiterLimit)
    , mLineJoin(aLineJoin)
    , mLineCap(aLineCap)
  {}

  Float mLineWidth;
  Float mMiterLimit;
  JoinStyle mLineJoin : 4;
  CapStyle mLineCap : 3;
};







struct DrawSurfaceOptions {
  DrawSurfaceOptions(Filter aFilter = FILTER_LINEAR)
    : mFilter(aFilter)
  { }

  Filter mFilter : 3;
};






class GradientStops : public RefCounted<GradientStops>
{
public:
  virtual ~GradientStops() {}

  virtual BackendType GetBackendType() const = 0;

protected:
  GradientStops() {}
};







class Pattern
{
public:
  virtual ~Pattern() {}

  virtual PatternType GetType() const = 0;

protected:
  Pattern() {}
};

class ColorPattern : public Pattern
{
public:
  ColorPattern(const Color &aColor)
    : mColor(aColor)
  {}

  virtual PatternType GetType() const { return PATTERN_COLOR; }

  Color mColor;
};






class LinearGradientPattern : public Pattern
{
public:
  





  LinearGradientPattern(const Point &aBegin,
                        const Point &aEnd,
                        GradientStops *aStops)
    : mBegin(aBegin)
    , mEnd(aEnd)
    , mStops(aStops)
  {
  }

  virtual PatternType GetType() const { return PATTERN_LINEAR_GRADIENT; }

  Point mBegin;
  Point mEnd;
  RefPtr<GradientStops> mStops;
};






class RadialGradientPattern : public Pattern
{
public:
  





  RadialGradientPattern(const Point &aCenter,
                        const Point &aOrigin,
                        Float aRadius,
                        GradientStops *aStops)
    : mCenter(aCenter)
    , mOrigin(aOrigin)
    , mRadius(aRadius)
    , mStops(aStops)
  {
  }

  virtual PatternType GetType() const { return PATTERN_RADIAL_GRADIENT; }

  Point mCenter;
  Point mOrigin;
  Float mRadius;
  RefPtr<GradientStops> mStops;
};





class SurfacePattern : public Pattern
{
public:
  SurfacePattern(SourceSurface *aSourceSurface, ExtendMode aExtendMode)
    : mSurface(aSourceSurface)
    , mExtendMode(aExtendMode)
  {}

  virtual PatternType GetType() const { return PATTERN_SURFACE; }

  RefPtr<SourceSurface> mSurface;
  ExtendMode mExtendMode;
  Filter mFilter;
};






class SourceSurface : public RefCounted<SourceSurface>
{
public:
  virtual ~SourceSurface() {}

  virtual SurfaceType GetType() const = 0;
  virtual IntSize GetSize() const = 0;
  virtual SurfaceFormat GetFormat() const = 0;

  



  virtual TemporaryRef<DataSourceSurface> GetDataSurface() = 0;
};

class DataSourceSurface : public SourceSurface
{
public:
  
  virtual unsigned char *GetData() = 0;
  



  virtual int32_t Stride() = 0;

  virtual TemporaryRef<DataSourceSurface> GetDataSurface() { RefPtr<DataSourceSurface> temp = this; return temp.forget(); }
};


class PathSink : public RefCounted<PathSink>
{
public:
  virtual ~PathSink() {}

  



  virtual void MoveTo(const Point &aPoint) = 0;
  
  virtual void LineTo(const Point &aPoint) = 0;
  
  virtual void BezierTo(const Point &aCP1,
                        const Point &aCP2,
                        const Point &aCP3) = 0;
  
  virtual void QuadraticBezierTo(const Point &aCP1,
                                 const Point &aCP2) = 0;
  


  virtual void Close() = 0;
  
  virtual void Arc(const Point &aOrigin, float aRadius, float aStartAngle,
                   float aEndAngle, bool aAntiClockwise = false) = 0;
  


  virtual Point CurrentPoint() const = 0;
};

class PathBuilder;




class Path : public RefCounted<Path>
{
public:
  virtual ~Path() {}
  
  virtual BackendType GetBackendType() const = 0;

  


  virtual TemporaryRef<PathBuilder> CopyToBuilder(FillRule aFillRule = FILL_WINDING) const = 0;
  virtual TemporaryRef<PathBuilder> TransformedCopyToBuilder(const Matrix &aTransform,
                                                             FillRule aFillRule = FILL_WINDING) const = 0;

  



  virtual bool ContainsPoint(const Point &aPoint, const Matrix &aTransform) const = 0;

  


  virtual FillRule GetFillRule() const = 0;
};




class PathBuilder : public PathSink
{
public:
  


  virtual TemporaryRef<Path> Finish() = 0;
};

struct Glyph
{
  uint32_t mIndex;
  Point mPosition;
};





struct GlyphBuffer
{
  
  const Glyph *mGlyphs;
  
  uint32_t mNumGlyphs;
};





class ScaledFont : public RefCounted<ScaledFont>
{
public:
  virtual ~ScaledFont() {}

  virtual FontType GetType() const = 0;

  




  virtual TemporaryRef<Path> GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget) = 0;

protected:
  ScaledFont() {}
};






class DrawTarget : public RefCounted<DrawTarget>
{
public:
  DrawTarget() : mTransformDirty(false) {}
  virtual ~DrawTarget() {}

  virtual BackendType GetType() const = 0;
  virtual TemporaryRef<SourceSurface> Snapshot() = 0;
  virtual IntSize GetSize() = 0;

  



  virtual void Flush() = 0;

  










  virtual void DrawSurface(SourceSurface *aSurface,
                           const Rect &aDest,
                           const Rect &aSource,
                           const DrawSurfaceOptions &aSurfOptions = DrawSurfaceOptions(),
                           const DrawOptions &aOptions = DrawOptions()) = 0;

  










  virtual void DrawSurfaceWithShadow(SourceSurface *aSurface,
                                     const Point &aDest,
                                     const Color &aColor,
                                     const Point &aOffset,
                                     Float aSigma) = 0;

  





  virtual void ClearRect(const Rect &aRect) = 0;

  








  virtual void CopySurface(SourceSurface *aSurface,
                           const IntRect &aSourceRect,
                           const IntPoint &aDestination) = 0;

  






  virtual void FillRect(const Rect &aRect,
                        const Pattern &aPattern,
                        const DrawOptions &aOptions = DrawOptions()) = 0;

  






  virtual void StrokeRect(const Rect &aRect,
                          const Pattern &aPattern,
                          const StrokeOptions &aStrokeOptions = StrokeOptions(),
                          const DrawOptions &aOptions = DrawOptions()) = 0;

  







  virtual void StrokeLine(const Point &aStart,
                          const Point &aEnd,
                          const Pattern &aPattern,
                          const StrokeOptions &aStrokeOptions = StrokeOptions(),
                          const DrawOptions &aOptions = DrawOptions()) = 0;

  







  virtual void Stroke(const Path *aPath,
                      const Pattern &aPattern,
                      const StrokeOptions &aStrokeOptions = StrokeOptions(),
                      const DrawOptions &aOptions = DrawOptions()) = 0;
  
  






  virtual void Fill(const Path *aPath,
                    const Pattern &aPattern,
                    const DrawOptions &aOptions = DrawOptions()) = 0;

  


  virtual void FillGlyphs(ScaledFont *aFont,
                          const GlyphBuffer &aBuffer,
                          const Pattern &aPattern,
                          const DrawOptions &aOptions = DrawOptions()) = 0;

  




  virtual void PushClip(const Path *aPath) = 0;

  


  virtual void PopClip() = 0;

  



  virtual TemporaryRef<SourceSurface> CreateSourceSurfaceFromData(unsigned char *aData,
                                                            const IntSize &aSize,
                                                            int32_t aStride,
                                                            SurfaceFormat aFormat) const = 0;

  




  virtual TemporaryRef<SourceSurface> OptimizeSourceSurface(SourceSurface *aSurface) const = 0;

  




  virtual TemporaryRef<SourceSurface>
    CreateSourceSurfaceFromNativeSurface(const NativeSurface &aSurface) const = 0;

  


  virtual TemporaryRef<DrawTarget>
    CreateSimilarDrawTarget(const IntSize &aSize, SurfaceFormat aFormat) const = 0;

  


  virtual TemporaryRef<PathBuilder> CreatePathBuilder(FillRule aFillRule = FILL_WINDING) const = 0;

  







  virtual TemporaryRef<GradientStops> CreateGradientStops(GradientStop *aStops, uint32_t aNumStops) const = 0;

  const Matrix &GetTransform() const { return mTransform; }

  



  virtual void SetTransform(const Matrix &aTransform)
    { mTransform = aTransform; mTransformDirty = true; }

  SurfaceFormat GetFormat() { return mFormat; }

  


  virtual void *GetNativeSurface(NativeSurfaceType aType) = 0;

protected:
  Matrix mTransform;
  bool mTransformDirty : 1;

  SurfaceFormat mFormat;
};

class Factory
{
public:
#ifdef USE_CAIRO
  static TemporaryRef<DrawTarget> CreateDrawTargetForCairoSurface(cairo_surface_t* aSurface);
#endif

  static TemporaryRef<DrawTarget> CreateDrawTarget(BackendType aBackend, const IntSize &aSize, SurfaceFormat aFormat);
  static TemporaryRef<ScaledFont> CreateScaledFontForNativeFont(const NativeFont &aNativeFont, Float aSize);

#ifdef WIN32
  static TemporaryRef<DrawTarget> CreateDrawTargetForD3D10Texture(ID3D10Texture2D *aTexture, SurfaceFormat aFormat);
  static void SetDirect3D10Device(ID3D10Device1 *aDevice);
  static ID3D10Device1 *GetDirect3D10Device();

private:
  static ID3D10Device1 *mD3D10Device;
#endif
};

}
}

#endif 
