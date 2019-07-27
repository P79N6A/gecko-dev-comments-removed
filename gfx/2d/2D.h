




#ifndef _MOZILLA_GFX_2D_H
#define _MOZILLA_GFX_2D_H

#include "Types.h"
#include "Point.h"
#include "Rect.h"
#include "Matrix.h"
#include "Quaternion.h"
#include "UserData.h"





#include "mozilla/GenericRefCounted.h"




#include "mozilla/RefPtr.h"

#include "mozilla/DebugOnly.h"

#ifdef MOZ_ENABLE_FREETYPE
#include <string>
#endif

struct _cairo_surface;
typedef _cairo_surface cairo_surface_t;

struct _cairo_scaled_font;
typedef _cairo_scaled_font cairo_scaled_font_t;

struct ID3D10Device1;
struct ID3D10Texture2D;
struct ID3D11Texture2D;
struct ID3D11Device;
struct ID2D1Device;
struct IDWriteRenderingParams;

class GrContext;

struct CGContext;
typedef struct CGContext *CGContextRef;

namespace mozilla {

namespace gfx {

class SourceSurface;
class DataSourceSurface;
class DrawTarget;
class DrawEventRecorder;
class FilterNode;
class LogForwarder;

struct NativeSurface {
  NativeSurfaceType mType;
  SurfaceFormat mFormat;
  gfx::IntSize mSize;
  void *mSurface;
};

struct NativeFont {
  NativeFontType mType;
  void *mFont;
};





struct DrawOptions {
  
  explicit DrawOptions(Float aAlpha = 1.0f,
                       CompositionOp aCompositionOp = CompositionOp::OP_OVER,
                       AntialiasMode aAntialiasMode = AntialiasMode::DEFAULT)
    : mAlpha(aAlpha)
    , mCompositionOp(aCompositionOp)
    , mAntialiasMode(aAntialiasMode)
  {}

  Float mAlpha;                 

  CompositionOp mCompositionOp; 

  AntialiasMode mAntialiasMode; 

};





struct StrokeOptions {
  
  explicit StrokeOptions(Float aLineWidth = 1.0f,
                         JoinStyle aLineJoin = JoinStyle::MITER_OR_BEVEL,
                         CapStyle aLineCap = CapStyle::BUTT,
                         Float aMiterLimit = 10.0f,
                         size_t aDashLength = 0,
                         const Float* aDashPattern = 0,
                         Float aDashOffset = 0.f)
    : mLineWidth(aLineWidth)
    , mMiterLimit(aMiterLimit)
    , mDashPattern(aDashLength > 0 ? aDashPattern : 0)
    , mDashLength(aDashLength)
    , mDashOffset(aDashOffset)
    , mLineJoin(aLineJoin)
    , mLineCap(aLineCap)
  {
    MOZ_ASSERT(aDashLength == 0 || aDashPattern);
  }

  Float mLineWidth;          
  Float mMiterLimit;         
  const Float* mDashPattern; 



  size_t mDashLength;        
  Float mDashOffset;         

  JoinStyle mLineJoin;       
  CapStyle mLineCap;         
};




struct DrawSurfaceOptions {
  
  explicit DrawSurfaceOptions(Filter aFilter = Filter::LINEAR,
                              SamplingBounds aSamplingBounds = SamplingBounds::UNBOUNDED)
    : mFilter(aFilter)
    , mSamplingBounds(aSamplingBounds)
  { }

  Filter mFilter;                 

  SamplingBounds mSamplingBounds; 




};






class GradientStops : public RefCounted<GradientStops>
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(GradientStops)
  virtual ~GradientStops() {}

  virtual BackendType GetBackendType() const = 0;
  virtual bool IsValid() const { return true; }

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
  
  
  explicit ColorPattern(const Color &aColor)
    : mColor(aColor)
  {}

  virtual PatternType GetType() const override
  {
    return PatternType::COLOR;
  }

  Color mColor;
};






class LinearGradientPattern : public Pattern
{
public:
  
  LinearGradientPattern(const Point &aBegin,
                        const Point &aEnd,
                        GradientStops *aStops,
                        const Matrix &aMatrix = Matrix())
    : mBegin(aBegin)
    , mEnd(aEnd)
    , mStops(aStops)
    , mMatrix(aMatrix)
  {
  }

  virtual PatternType GetType() const override
  {
    return PatternType::LINEAR_GRADIENT;
  }

  Point mBegin;                 
  Point mEnd;                   


  RefPtr<GradientStops> mStops; 


  Matrix mMatrix;               

};






class RadialGradientPattern : public Pattern
{
public:
  
  RadialGradientPattern(const Point &aCenter1,
                        const Point &aCenter2,
                        Float aRadius1,
                        Float aRadius2,
                        GradientStops *aStops,
                        const Matrix &aMatrix = Matrix())
    : mCenter1(aCenter1)
    , mCenter2(aCenter2)
    , mRadius1(aRadius1)
    , mRadius2(aRadius2)
    , mStops(aStops)
    , mMatrix(aMatrix)
  {
  }

  virtual PatternType GetType() const override
  {
    return PatternType::RADIAL_GRADIENT;
  }

  Point mCenter1; 
  Point mCenter2; 
  Float mRadius1; 
  Float mRadius2; 
  RefPtr<GradientStops> mStops; 


  Matrix mMatrix; 
};





class SurfacePattern : public Pattern
{
public:
  
  SurfacePattern(SourceSurface *aSourceSurface, ExtendMode aExtendMode,
                 const Matrix &aMatrix = Matrix(), Filter aFilter = Filter::GOOD,
                 const IntRect &aSamplingRect = IntRect())
    : mSurface(aSourceSurface)
    , mExtendMode(aExtendMode)
    , mFilter(aFilter)
    , mMatrix(aMatrix)
    , mSamplingRect(aSamplingRect)
  {}

  virtual PatternType GetType() const override
  {
    return PatternType::SURFACE;
  }

  RefPtr<SourceSurface> mSurface; 
  ExtendMode mExtendMode;         

  Filter mFilter;                 
  Matrix mMatrix;                 

  IntRect mSamplingRect;          

};

class StoredPattern;
class DrawTargetCaptureImpl;






class SourceSurface : public RefCounted<SourceSurface>
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(SourceSurface)
  virtual ~SourceSurface() {}

  virtual SurfaceType GetType() const = 0;
  virtual IntSize GetSize() const = 0;
  virtual SurfaceFormat GetFormat() const = 0;

  




  virtual bool IsValid() const { return true; }

  



  virtual TemporaryRef<DataSourceSurface> GetDataSurface() = 0;

  


  virtual void *GetNativeSurface(NativeSurfaceType aType) {
    return nullptr;
  }

  void AddUserData(UserDataKey *key, void *userData, void (*destroy)(void*)) {
    mUserData.Add(key, userData, destroy);
  }
  void *GetUserData(UserDataKey *key) {
    return mUserData.Get(key);
  }

protected:
  friend class DrawTargetCaptureImpl;
  friend class StoredPattern;

  
  
  
  
  virtual void GuaranteePersistance() {}

  UserData mUserData;
};

class DataSourceSurface : public SourceSurface
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DataSourceSurface, override)
  DataSourceSurface()
    : mIsMapped(false)
  {
  }

#ifdef DEBUG
  virtual ~DataSourceSurface()
  {
    MOZ_ASSERT(!mIsMapped, "Someone forgot to call Unmap()");
  }
#endif

  struct MappedSurface {
    uint8_t *mData;
    int32_t mStride;
  };

  enum MapType {
    READ,
    WRITE,
    READ_WRITE
  };

  virtual SurfaceType GetType() const override { return SurfaceType::DATA; }
  



  virtual uint8_t *GetData() = 0;

  




  virtual int32_t Stride() = 0;

  


  virtual bool Map(MapType, MappedSurface *aMappedSurface)
  {
    aMappedSurface->mData = GetData();
    aMappedSurface->mStride = Stride();
    mIsMapped = !!aMappedSurface->mData;
    return mIsMapped;
  }

  virtual void Unmap()
  {
    MOZ_ASSERT(mIsMapped);
    mIsMapped = false;
  }

  



  virtual TemporaryRef<DataSourceSurface> GetDataSurface() override;

protected:
  bool mIsMapped;
};


class PathSink : public RefCounted<PathSink>
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(PathSink)
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
class FlattenedPath;




class Path : public RefCounted<Path>
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(Path)
  virtual ~Path();
  
  virtual BackendType GetBackendType() const = 0;

  


  virtual TemporaryRef<PathBuilder> CopyToBuilder(FillRule aFillRule = FillRule::FILL_WINDING) const = 0;
  virtual TemporaryRef<PathBuilder> TransformedCopyToBuilder(const Matrix &aTransform,
                                                             FillRule aFillRule = FillRule::FILL_WINDING) const = 0;

  



  virtual bool ContainsPoint(const Point &aPoint, const Matrix &aTransform) const = 0;


  



  virtual bool StrokeContainsPoint(const StrokeOptions &aStrokeOptions,
                                   const Point &aPoint,
                                   const Matrix &aTransform) const = 0;

  



  virtual Rect GetBounds(const Matrix &aTransform = Matrix()) const = 0;

  




  virtual Rect GetStrokedBounds(const StrokeOptions &aStrokeOptions,
                                const Matrix &aTransform = Matrix()) const = 0;

  


  virtual void StreamToSink(PathSink *aSink) const = 0;

  


  virtual FillRule GetFillRule() const = 0;

  virtual Float ComputeLength();

  virtual Point ComputePointAtLength(Float aLength,
                                     Point* aTangent = nullptr);

protected:
  Path();
  void EnsureFlattenedPath();

  RefPtr<FlattenedPath> mFlattenedPath;
};




class PathBuilder : public PathSink
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(PathBuilder)
  


  virtual TemporaryRef<Path> Finish() = 0;

  virtual BackendType GetBackendType() const = 0;
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
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(ScaledFont)
  virtual ~ScaledFont() {}

  typedef void (*FontFileDataOutput)(const uint8_t *aData, uint32_t aLength, uint32_t aIndex, Float aGlyphSize, void *aBaton);

  virtual FontType GetType() const = 0;

  




  virtual TemporaryRef<Path> GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget) = 0;

  




  virtual void CopyGlyphsToBuilder(const GlyphBuffer &aBuffer, PathBuilder *aBuilder, BackendType aBackendType, const Matrix *aTransformHint = nullptr) = 0;

  virtual bool GetFontFileData(FontFileDataOutput, void *) { return false; }

  void AddUserData(UserDataKey *key, void *userData, void (*destroy)(void*)) {
    mUserData.Add(key, userData, destroy);
  }
  void *GetUserData(UserDataKey *key) {
    return mUserData.Get(key);
  }

protected:
  ScaledFont() {}

  UserData mUserData;
};







class GlyphRenderingOptions : public RefCounted<GlyphRenderingOptions>
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(GlyphRenderingOptions)
  virtual ~GlyphRenderingOptions() {}

  virtual FontType GetType() const = 0;

protected:
  GlyphRenderingOptions() {}
};

class DrawTargetCapture;






class DrawTarget : public RefCounted<DrawTarget>
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DrawTarget)
  DrawTarget() : mTransformDirty(false), mPermitSubpixelAA(false) {}
  virtual ~DrawTarget() {}

  virtual DrawTargetType GetType() const = 0;

  virtual BackendType GetBackendType() const = 0;
  




  virtual TemporaryRef<SourceSurface> Snapshot() = 0;
  virtual IntSize GetSize() = 0;

  




  virtual bool LockBits(uint8_t** aData, IntSize* aSize,
                        int32_t* aStride, SurfaceFormat* aFormat) { return false; }
  virtual void ReleaseBits(uint8_t* aData) {}

  



  virtual void Flush() = 0;

  





  virtual void DrawCapturedDT(DrawTargetCapture *aCaptureDT,
                              const Matrix& aTransform);

  










  virtual void DrawSurface(SourceSurface *aSurface,
                           const Rect &aDest,
                           const Rect &aSource,
                           const DrawSurfaceOptions &aSurfOptions = DrawSurfaceOptions(),
                           const DrawOptions &aOptions = DrawOptions()) = 0;

  







  virtual void DrawFilter(FilterNode *aNode,
                          const Rect &aSourceRect,
                          const Point &aDestPoint,
                          const DrawOptions &aOptions = DrawOptions()) = 0;

  













  virtual void DrawSurfaceWithShadow(SourceSurface *aSurface,
                                     const Point &aDest,
                                     const Color &aColor,
                                     const Point &aOffset,
                                     Float aSigma,
                                     CompositionOp aOperator) = 0;

  





  virtual void ClearRect(const Rect &aRect) = 0;

  








  virtual void CopySurface(SourceSurface *aSurface,
                           const IntRect &aSourceRect,
                           const IntPoint &aDestination) = 0;

  





  virtual void CopyRect(const IntRect &aSourceRect,
                        const IntPoint &aDestination)
  {
    RefPtr<SourceSurface> source = Snapshot();
    CopySurface(source, aSourceRect, aDestination);
  }

  






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
                          const DrawOptions &aOptions = DrawOptions(),
                          const GlyphRenderingOptions *aRenderingOptions = nullptr) = 0;

  








  virtual void Mask(const Pattern &aSource,
                    const Pattern &aMask,
                    const DrawOptions &aOptions = DrawOptions()) = 0;

  









  virtual void MaskSurface(const Pattern &aSource,
                           SourceSurface *aMask,
                           Point aOffset,
                           const DrawOptions &aOptions = DrawOptions()) = 0;

  




  virtual void PushClip(const Path *aPath) = 0;

  





  virtual void PushClipRect(const Rect &aRect) = 0;

  


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

  





  virtual TemporaryRef<DrawTargetCapture> CreateCaptureDT(const IntSize& aSize);

  







  virtual TemporaryRef<DrawTarget>
    CreateShadowDrawTarget(const IntSize &aSize, SurfaceFormat aFormat,
                           float aSigma) const
  {
    return CreateSimilarDrawTarget(aSize, aFormat);
  }

  






  virtual TemporaryRef<PathBuilder> CreatePathBuilder(FillRule aFillRule = FillRule::FILL_WINDING) const = 0;

  









  virtual TemporaryRef<GradientStops>
    CreateGradientStops(GradientStop *aStops,
                        uint32_t aNumStops,
                        ExtendMode aExtendMode = ExtendMode::CLAMP) const = 0;

  





  virtual TemporaryRef<FilterNode> CreateFilter(FilterType aType) = 0;

  Matrix GetTransform() const { return mTransform; }

  
















  virtual void SetTransform(const Matrix &aTransform)
    { mTransform = aTransform; mTransformDirty = true; }

  inline void ConcatTransform(const Matrix &aTransform)
    { SetTransform(aTransform * Matrix(GetTransform())); }

  SurfaceFormat GetFormat() const { return mFormat; }

  


  virtual void *GetNativeSurface(NativeSurfaceType aType) { return nullptr; }

  virtual bool IsDualDrawTarget() const { return false; }
  virtual bool IsTiledDrawTarget() const { return false; }
  virtual bool SupportsRegionClipping() const { return true; }

  void AddUserData(UserDataKey *key, void *userData, void (*destroy)(void*)) {
    mUserData.Add(key, userData, destroy);
  }
  void *GetUserData(UserDataKey *key) const {
    return mUserData.Get(key);
  }
  void *RemoveUserData(UserDataKey *key) {
    return mUserData.Remove(key);
  }

  




  void SetOpaqueRect(const IntRect &aRect) {
    mOpaqueRect = aRect;
  }

  const IntRect &GetOpaqueRect() const {
    return mOpaqueRect;
  }

  virtual void SetPermitSubpixelAA(bool aPermitSubpixelAA) {
    mPermitSubpixelAA = aPermitSubpixelAA;
  }

  bool GetPermitSubpixelAA() {
    return mPermitSubpixelAA;
  }

#ifdef USE_SKIA_GPU
  virtual bool InitWithGrContext(GrContext* aGrContext,
                                 const IntSize &aSize,
                                 SurfaceFormat aFormat)
  {
    MOZ_CRASH();
  }
#endif

protected:
  UserData mUserData;
  Matrix mTransform;
  IntRect mOpaqueRect;
  bool mTransformDirty : 1;
  bool mPermitSubpixelAA : 1;

  SurfaceFormat mFormat;
};

class DrawTargetCapture : public DrawTarget
{
};

class DrawEventRecorder : public RefCounted<DrawEventRecorder>
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DrawEventRecorder)
  virtual ~DrawEventRecorder() { }
};

struct Tile
{
  RefPtr<DrawTarget> mDrawTarget;
  IntPoint mTileOrigin;
};

struct TileSet
{
  Tile* mTiles;
  size_t mTileCount;
};

class GFX2D_API Factory
{
public:
  static bool HasSSE2();

  



  static bool CheckSurfaceSize(const IntSize &sz, int32_t limit = 0);

  


  static bool ReasonableSurfaceSize(const IntSize &aSize);

  static TemporaryRef<DrawTarget> CreateDrawTargetForCairoSurface(cairo_surface_t* aSurface, const IntSize& aSize, SurfaceFormat* aFormat = nullptr);

  static TemporaryRef<DrawTarget>
    CreateDrawTarget(BackendType aBackend, const IntSize &aSize, SurfaceFormat aFormat);

  static TemporaryRef<DrawTarget>
    CreateRecordingDrawTarget(DrawEventRecorder *aRecorder, DrawTarget *aDT);
     
  static TemporaryRef<DrawTarget>
    CreateDrawTargetForData(BackendType aBackend, unsigned char* aData, const IntSize &aSize, int32_t aStride, SurfaceFormat aFormat);

  static TemporaryRef<ScaledFont>
    CreateScaledFontForNativeFont(const NativeFont &aNativeFont, Float aSize);

  








  static TemporaryRef<ScaledFont>
    CreateScaledFontForTrueTypeData(uint8_t *aData, uint32_t aSize, uint32_t aFaceIndex, Float aGlyphSize, FontType aType);

  




  static TemporaryRef<ScaledFont>
    CreateScaledFontWithCairo(const NativeFont &aNativeFont, Float aSize, cairo_scaled_font_t* aScaledFont);

  





  static TemporaryRef<DataSourceSurface>
    CreateDataSourceSurface(const IntSize &aSize, SurfaceFormat aFormat, bool aZero = false);

  






  static TemporaryRef<DataSourceSurface>
    CreateDataSourceSurfaceWithStride(const IntSize &aSize, SurfaceFormat aFormat, int32_t aStride, bool aZero = false);

  





  static TemporaryRef<DataSourceSurface>
    CreateWrappingDataSourceSurface(uint8_t *aData, int32_t aStride,
                                    const IntSize &aSize, SurfaceFormat aFormat);

  static TemporaryRef<DrawEventRecorder>
    CreateEventRecorderForFile(const char *aFilename);

  static void SetGlobalEventRecorder(DrawEventRecorder *aRecorder);

  
  static void SetLogForwarder(LogForwarder* aLogFwd);

  static uint32_t GetMaxSurfaceSize(BackendType aType);

  static LogForwarder* GetLogForwarder() { return mLogForwarder; }

private:
  static LogForwarder* mLogForwarder;
public:

#ifdef USE_SKIA_GPU
  static TemporaryRef<DrawTarget>
    CreateDrawTargetSkiaWithGrContext(GrContext* aGrContext,
                                      const IntSize &aSize,
                                      SurfaceFormat aFormat);
#endif

  static void PurgeAllCaches();

#if defined(USE_SKIA) && defined(MOZ_ENABLE_FREETYPE)
  static TemporaryRef<GlyphRenderingOptions>
    CreateCairoGlyphRenderingOptions(FontHinting aHinting, bool aAutoHinting);
#endif
  static TemporaryRef<DrawTarget>
    CreateDualDrawTarget(DrawTarget *targetA, DrawTarget *targetB);

  





  static TemporaryRef<DrawTarget> CreateTiledDrawTarget(const TileSet& aTileSet);

  static bool DoesBackendSupportDataDrawtarget(BackendType aType);

#ifdef XP_MACOSX
  static TemporaryRef<DrawTarget> CreateDrawTargetForCairoCGContext(CGContextRef cg, const IntSize& aSize);
  static TemporaryRef<GlyphRenderingOptions>
    CreateCGGlyphRenderingOptions(const Color &aFontSmoothingBackgroundColor);
#endif

#ifdef WIN32
  static TemporaryRef<DrawTarget> CreateDrawTargetForD3D10Texture(ID3D10Texture2D *aTexture, SurfaceFormat aFormat);
  static TemporaryRef<DrawTarget>
    CreateDualDrawTargetForD3D10Textures(ID3D10Texture2D *aTextureA,
                                         ID3D10Texture2D *aTextureB,
                                         SurfaceFormat aFormat);

  static void SetDirect3D10Device(ID3D10Device1 *aDevice);
  static ID3D10Device1 *GetDirect3D10Device();
  static TemporaryRef<DrawTarget> CreateDrawTargetForD3D11Texture(ID3D11Texture2D *aTexture, SurfaceFormat aFormat);

  static void SetDirect3D11Device(ID3D11Device *aDevice);
  static ID3D11Device *GetDirect3D11Device();
  static ID2D1Device *GetD2D1Device();
  static bool SupportsD2D1();

  static TemporaryRef<GlyphRenderingOptions>
    CreateDWriteGlyphRenderingOptions(IDWriteRenderingParams *aParams);

  static uint64_t GetD2DVRAMUsageDrawTarget();
  static uint64_t GetD2DVRAMUsageSourceSurface();
  static void D2DCleanup();

private:
  static ID2D1Device *mD2D1Device;
  static ID3D10Device1 *mD3D10Device;
  static ID3D11Device *mD3D11Device;
#endif

  static DrawEventRecorder *mRecorder;
};

}
}

#endif 
