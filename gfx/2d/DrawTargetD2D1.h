




#ifndef MOZILLA_GFX_DRAWTARGETD2D1_H_
#define MOZILLA_GFX_DRAWTARGETD2D1_H_

#include "2D.h"
#include <d3d11.h>
#include <d2d1_1.h>
#include "PathD2D.h"
#include "HelpersD2D.h"

#include <vector>
#include <sstream>

#include <unordered_set>

struct IDWriteFactory;

namespace mozilla {
namespace gfx {

class SourceSurfaceD2D1;

const int32_t kLayerCacheSize1 = 5;

class DrawTargetD2D1 : public DrawTarget
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DrawTargetD2D1)
  DrawTargetD2D1();
  virtual ~DrawTargetD2D1();

  virtual DrawTargetType GetType() const override { return DrawTargetType::HARDWARE_RASTER; }
  virtual BackendType GetBackendType() const { return BackendType::DIRECT2D1_1; }
  virtual TemporaryRef<SourceSurface> Snapshot();
  virtual IntSize GetSize() { return mSize; }

  virtual void Flush();
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
                                     CompositionOp aOperator);
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

  virtual TemporaryRef<SourceSurface> CreateSourceSurfaceFromData(unsigned char *aData,
                                                                  const IntSize &aSize,
                                                                  int32_t aStride,
                                                                  SurfaceFormat aFormat) const;
  virtual TemporaryRef<SourceSurface> OptimizeSourceSurface(SourceSurface *aSurface) const;

  virtual TemporaryRef<SourceSurface>
    CreateSourceSurfaceFromNativeSurface(const NativeSurface &aSurface) const { return nullptr; }
  
  virtual TemporaryRef<DrawTarget>
    CreateSimilarDrawTarget(const IntSize &aSize, SurfaceFormat aFormat) const;

  virtual TemporaryRef<PathBuilder> CreatePathBuilder(FillRule aFillRule = FillRule::FILL_WINDING) const;

  virtual TemporaryRef<GradientStops>
    CreateGradientStops(GradientStop *aStops,
                        uint32_t aNumStops,
                        ExtendMode aExtendMode = ExtendMode::CLAMP) const;

  virtual TemporaryRef<FilterNode> CreateFilter(FilterType aType);

  virtual bool SupportsRegionClipping() const { return false; }

  virtual void *GetNativeSurface(NativeSurfaceType aType) { return nullptr; }

  bool Init(const IntSize &aSize, SurfaceFormat aFormat);
  bool Init(ID3D11Texture2D* aTexture, SurfaceFormat aFormat);
  uint32_t GetByteSize() const;

  TemporaryRef<ID2D1Image> GetImageForSurface(SourceSurface *aSurface, Matrix &aSourceTransform,
                                              ExtendMode aExtendMode, const IntRect* aSourceRect = nullptr);

  TemporaryRef<ID2D1Image> GetImageForSurface(SourceSurface *aSurface, ExtendMode aExtendMode) {
    Matrix mat;
    return GetImageForSurface(aSurface, mat, aExtendMode, nullptr);
  }

  static ID2D1Factory1 *factory();
  static void CleanupD2D();
  static IDWriteFactory *GetDWriteFactory();

  operator std::string() const {
    std::stringstream stream;
    stream << "DrawTargetD2D 1.1 (" << this << ")";
    return stream.str();
  }

  static uint32_t GetMaxSurfaceSize() {
    return D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
  }

  static uint64_t mVRAMUsageDT;
  static uint64_t mVRAMUsageSS;

private:
  friend class SourceSurfaceD2D1;

  typedef std::unordered_set<DrawTargetD2D1*> TargetSet;

  
  
  void MarkChanged();
  void PrepareForDrawing(CompositionOp aOp, const Pattern &aPattern);
  void FinalizeDrawing(CompositionOp aOp, const Pattern &aPattern);
  void FlushTransformToDC() {
    if (mTransformDirty) {
      mDC->SetTransform(D2DMatrix(mTransform));
      mTransformDirty = false;
    }
  }
  void AddDependencyOnSource(SourceSurfaceD2D1* aSource);

  
  
  
  
  TemporaryRef<ID2D1Geometry> GetClippedGeometry(IntRect *aClipBounds);

  TemporaryRef<ID2D1Geometry> GetInverseClippedGeometry();

  bool GetDeviceSpaceClipRect(D2D1_RECT_F& aClipRect, bool& aIsPixelAligned);

  void PopAllClips();
  void PushAllClips();
  void PushClipsToDC(ID2D1DeviceContext *aDC);
  void PopClipsFromDC(ID2D1DeviceContext *aDC);

  TemporaryRef<ID2D1Brush> CreateTransparentBlackBrush();
  TemporaryRef<ID2D1Brush> CreateBrushForPattern(const Pattern &aPattern, Float aAlpha = 1.0f);

  void PushD2DLayer(ID2D1DeviceContext *aDC, ID2D1Geometry *aGeometry, const D2D1_MATRIX_3X2_F &aTransform);

  IntSize mSize;

  RefPtr<ID3D11Device> mDevice;
  RefPtr<ID3D11Texture2D> mTexture;
  RefPtr<ID2D1Geometry> mCurrentClippedGeometry;
  
  
  
  IntRect mCurrentClipBounds;
  mutable RefPtr<ID2D1DeviceContext> mDC;
  RefPtr<ID2D1Bitmap1> mBitmap;
  RefPtr<ID2D1Bitmap1> mTempBitmap;
  RefPtr<ID2D1Effect> mBlendEffect;

  
  RefPtr<IDWriteRenderingParams> mTextRenderingParams;

  
  struct PushedClip
  {
    D2D1_RECT_F mBounds;
    union {
      
      
      D2D1_MATRIX_3X2_F mTransform;
      bool mIsPixelAligned;
    };
    RefPtr<PathD2D> mPath;
  };
  std::vector<PushedClip> mPushedClips;

  
  
  RefPtr<SourceSurfaceD2D1> mSnapshot;
  
  TargetSet mDependentTargets;
  
  TargetSet mDependingOnTargets;

  
  bool mClipsArePushed;
  static ID2D1Factory1 *mFactory;
  static IDWriteFactory *mDWriteFactory;
};

}
}

#endif 
