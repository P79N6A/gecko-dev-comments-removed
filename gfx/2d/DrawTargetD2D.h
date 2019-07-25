




































#ifndef MOZILLA_GFX_DRAWTARGETD2D_H_
#define MOZILLA_GFX_DRAWTARGETD2D_H_

#include "2D.h"
#include "PathD2D.h"
#include <d3d10_1.h>
#include "HelpersD2D.h"

#include <vector>
#include <sstream>

namespace mozilla {
namespace gfx {

class SourceSurfaceD2DTarget;

struct PrivateD3D10DataD2D
{
  RefPtr<ID3D10Effect> mEffect;
  RefPtr<ID3D10InputLayout> mInputLayout;
  RefPtr<ID3D10Buffer> mVB;
  RefPtr<ID3D10BlendState> mBlendStates[OP_COUNT];
};

class DrawTargetD2D : public DrawTarget
{
public:
  DrawTargetD2D();
  virtual ~DrawTargetD2D();

  virtual BackendType GetType() const { return BACKEND_DIRECT2D; }
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
                                     Float aSigma);
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

  virtual void *GetNativeSurface(NativeSurfaceType aType);

  bool Init(const IntSize &aSize, SurfaceFormat aFormat);
  bool Init(ID3D10Texture2D *aTexture, SurfaceFormat aFormat);
  bool InitD3D10Data();

  static ID2D1Factory *factory();

  operator std::string() const {
    std::stringstream stream;
    stream << "DrawTargetD2D(" << this << ")";
    return stream.str();
  }
private:
  friend class AutoSaveRestoreClippedOut;
  friend class SourceSurfaceD2DTarget;

  bool InitD2DRenderTarget();
  void PrepareForDrawing(ID2D1RenderTarget *aRT);

  
  
  void MarkChanged();

  ID3D10BlendState *GetBlendStateForOperator(CompositionOp aOperator);
  ID2D1RenderTarget *GetRTForOperator(CompositionOp aOperator);
  void FinalizeRTForOperator(CompositionOp aOperator, const Rect &aBounds);
  void EnsureViews();
  void PopAllClips();

  TemporaryRef<ID2D1RenderTarget> CreateRTForTexture(ID3D10Texture2D *aTexture);

  TemporaryRef<ID2D1Brush> CreateBrushForPattern(const Pattern &aPattern, Float aAlpha = 1.0f);
  TemporaryRef<ID2D1StrokeStyle> CreateStrokeStyleForOptions(const StrokeOptions &aStrokeOptions);

  IntSize mSize;

  RefPtr<ID3D10Device1> mDevice;
  RefPtr<ID3D10Texture2D> mTexture;
  mutable RefPtr<ID2D1RenderTarget> mRT;

  
  RefPtr<ID3D10Texture2D> mTempTexture;
  RefPtr<ID3D10RenderTargetView> mRTView;
  RefPtr<ID3D10ShaderResourceView> mSRView;
  RefPtr<ID2D1RenderTarget> mTempRT;
  RefPtr<ID3D10RenderTargetView> mTempRTView;

  
  struct PushedClip
  {
    RefPtr<ID2D1Layer> mLayer;
    D2D1_RECT_F mBounds;
    D2D1_MATRIX_3X2_F mTransform;
    RefPtr<PathD2D> mPath;
  };
  std::vector<PushedClip> mPushedClips;
  
  
  
  std::vector<SourceSurfaceD2DTarget*> mSnapshots;
  
  std::vector<RefPtr<DrawTargetD2D>> mDependentTargets;

  
  bool mClipsArePushed;
  PrivateD3D10DataD2D *mPrivateData;
  static ID2D1Factory *mFactory;
};

}
}

#endif 
