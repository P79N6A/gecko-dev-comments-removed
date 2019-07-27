




#ifndef GFX_CONTEXT_H
#define GFX_CONTEXT_H

#include "gfxTypes.h"

#include "gfxASurface.h"
#include "gfxPoint.h"
#include "gfxRect.h"
#include "gfxMatrix.h"
#include "gfxPattern.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"

#include "mozilla/gfx/2D.h"

typedef struct _cairo cairo_t;
class GlyphBufferAzure;
template <typename T> class FallibleTArray;

namespace mozilla {
namespace gfx {
struct RectCornerRadii;
}
}















class gfxContext final {
    typedef mozilla::gfx::CapStyle CapStyle;
    typedef mozilla::gfx::JoinStyle JoinStyle;
    typedef mozilla::gfx::FillRule FillRule;
    typedef mozilla::gfx::Path Path;
    typedef mozilla::gfx::Pattern Pattern;
    typedef mozilla::gfx::Rect Rect;
    typedef mozilla::gfx::RectCornerRadii RectCornerRadii;

    NS_INLINE_DECL_REFCOUNTING(gfxContext)

public:

    




    explicit gfxContext(mozilla::gfx::DrawTarget *aTarget,
                        const mozilla::gfx::Point& aDeviceOffset = mozilla::gfx::Point());

    




    static already_AddRefed<gfxContext> ContextForDrawTarget(mozilla::gfx::DrawTarget* aTarget);

    





    already_AddRefed<gfxASurface> CurrentSurface(gfxFloat *dx, gfxFloat *dy);
    already_AddRefed<gfxASurface> CurrentSurface() {
        return CurrentSurface(nullptr, nullptr);
    }

    



    cairo_t *GetCairo();

    mozilla::gfx::DrawTarget *GetDrawTarget() { return mDT; }

    


    
    void Save();
    void Restore();

    



    




    void Fill();
    void Fill(const Pattern& aPattern);

    


    void NewPath();

    




    void ClosePath();

    


    mozilla::TemporaryRef<Path> GetPath();

    


    void SetPath(Path* path);

    


    void MoveTo(const gfxPoint& pt);

    


    gfxPoint CurrentPoint();

    




    void LineTo(const gfxPoint& pt);

    
    


    void Line(const gfxPoint& start, const gfxPoint& end); 

    



    void Rectangle(const gfxRect& rect, bool snapToPixels = false);
    void SnappedRectangle(const gfxRect& rect) { return Rectangle(rect, true); }

    



    




    void Multiply(const gfxMatrix& other);

    


    void SetMatrix(const gfxMatrix& matrix);

    


    gfxMatrix CurrentMatrix() const;

    



    gfxPoint DeviceToUser(const gfxPoint& point) const;

    



    gfxSize DeviceToUser(const gfxSize& size) const;

    




    gfxRect DeviceToUser(const gfxRect& rect) const;

    



    gfxPoint UserToDevice(const gfxPoint& point) const;

    



    gfxSize UserToDevice(const gfxSize& size) const;

    




    gfxRect UserToDevice(const gfxRect& rect) const;

    










    bool UserToDevicePixelSnapped(gfxRect& rect, bool ignoreScale = false) const;

    










    bool UserToDevicePixelSnapped(gfxPoint& pt, bool ignoreScale = false) const;

    



    



    void SetDeviceColor(const gfxRGBA& c);

    




    bool GetDeviceColor(gfxRGBA& c);

    




    void SetColor(const gfxRGBA& c);

    






    void SetSource(gfxASurface *surface, const gfxPoint& offset = gfxPoint(0.0, 0.0));

    


    void SetPattern(gfxPattern *pattern);

    



    void SetFontSmoothingBackgroundColor(const mozilla::gfx::Color& aColor);
    mozilla::gfx::Color GetFontSmoothingBackgroundColor();

    


    already_AddRefed<gfxPattern> GetPattern();

    


    



    void Paint(gfxFloat alpha = 1.0);

    


    



    void Mask(mozilla::gfx::SourceSurface *aSurface, const mozilla::gfx::Matrix& aTransform);

    



    void Mask(gfxASurface *surface, const gfxPoint& offset = gfxPoint(0.0, 0.0));

    void Mask(mozilla::gfx::SourceSurface *surface, float alpha = 1.0f, const mozilla::gfx::Point& offset = mozilla::gfx::Point());

    



    void SetDash(gfxFloat *dashes, int ndash, gfxFloat offset);
    
    
    
    bool CurrentDash(FallibleTArray<gfxFloat>& dashes, gfxFloat* offset) const;
    
    gfxFloat CurrentDashOffset() const;

    


    void SetLineWidth(gfxFloat width);

    




    gfxFloat CurrentLineWidth() const;

    


    void SetLineCap(CapStyle cap);
    CapStyle CurrentLineCap() const;

    



    void SetLineJoin(JoinStyle join);
    JoinStyle CurrentLineJoin() const;

    void SetMiterLimit(gfxFloat limit);
    gfxFloat CurrentMiterLimit() const;

    



    void SetFillRule(FillRule rule);
    FillRule CurrentFillRule() const;

    



    
    enum GraphicsOperator {
        OPERATOR_SOURCE,

        OPERATOR_OVER,
        OPERATOR_IN,
        OPERATOR_OUT,
        OPERATOR_ATOP,

        OPERATOR_DEST,
        OPERATOR_DEST_OVER,
        OPERATOR_DEST_IN,
        OPERATOR_DEST_OUT,
        OPERATOR_DEST_ATOP,

        OPERATOR_XOR,
        OPERATOR_ADD,
        OPERATOR_SATURATE,

        OPERATOR_MULTIPLY,
        OPERATOR_SCREEN,
        OPERATOR_OVERLAY,
        OPERATOR_DARKEN,
        OPERATOR_LIGHTEN,
        OPERATOR_COLOR_DODGE,
        OPERATOR_COLOR_BURN,
        OPERATOR_HARD_LIGHT,
        OPERATOR_SOFT_LIGHT,
        OPERATOR_DIFFERENCE,
        OPERATOR_EXCLUSION,
        OPERATOR_HUE,
        OPERATOR_SATURATION,
        OPERATOR_COLOR,
        OPERATOR_LUMINOSITY
    };
    





    void SetOperator(GraphicsOperator op);
    GraphicsOperator CurrentOperator() const;

    void SetAntialiasMode(mozilla::gfx::AntialiasMode mode);
    mozilla::gfx::AntialiasMode CurrentAntialiasMode() const;

    



    



    void Clip();

    



    void Clip(const Rect& rect);
    void Clip(const gfxRect& rect); 
    void Clip(Path* aPath);

    void PopClip();

    



    gfxRect GetClipExtents();

    




    bool ClipContainsRect(const gfxRect& aRect);

    


    void PushGroup(gfxContentType content = gfxContentType::COLOR);
    










    void PushGroupAndCopyBackground(gfxContentType content = gfxContentType::COLOR);
    already_AddRefed<gfxPattern> PopGroup();
    void PopGroupToSource();

    mozilla::TemporaryRef<mozilla::gfx::SourceSurface>
    PopGroupToSurface(mozilla::gfx::Matrix* aMatrix);

    mozilla::gfx::Point GetDeviceOffset() const;

    
    void GetRoundOffsetsToPixels(bool *aRoundX, bool *aRoundY);

#ifdef MOZ_DUMP_PAINTING
    



    


    void WriteAsPNG(const char* aFile);

    


    void DumpAsDataURI();

    


    void CopyAsDataURI();
#endif

    static mozilla::gfx::UserDataKey sDontUseAsSourceKey;

private:
    ~gfxContext();

  friend class PatternFromState;
  friend class GlyphBufferAzure;

  typedef mozilla::gfx::Matrix Matrix;
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::Color Color;
  typedef mozilla::gfx::StrokeOptions StrokeOptions;
  typedef mozilla::gfx::Float Float;
  typedef mozilla::gfx::CompositionOp CompositionOp;
  typedef mozilla::gfx::PathBuilder PathBuilder;
  typedef mozilla::gfx::SourceSurface SourceSurface;
  
  struct AzureState {
    AzureState()
      : op(mozilla::gfx::CompositionOp::OP_OVER)
      , color(0, 0, 0, 1.0f)
      , clipWasReset(false)
      , fillRule(mozilla::gfx::FillRule::FILL_WINDING)
      , aaMode(mozilla::gfx::AntialiasMode::SUBPIXEL)
      , patternTransformChanged(false)
    {}

    mozilla::gfx::CompositionOp op;
    Color color;
    nsRefPtr<gfxPattern> pattern;
    nsRefPtr<gfxASurface> sourceSurfCairo;
    mozilla::RefPtr<SourceSurface> sourceSurface;
    mozilla::gfx::Point sourceSurfaceDeviceOffset;
    Matrix surfTransform;
    Matrix transform;
    struct PushedClip {
      mozilla::RefPtr<Path> path;
      Rect rect;
      Matrix transform;
    };
    nsTArray<PushedClip> pushedClips;
    nsTArray<Float> dashPattern;
    bool clipWasReset;
    mozilla::gfx::FillRule fillRule;
    StrokeOptions strokeOptions;
    mozilla::RefPtr<DrawTarget> drawTarget;
    mozilla::RefPtr<DrawTarget> parentTarget;
    mozilla::gfx::AntialiasMode aaMode;
    bool patternTransformChanged;
    Matrix patternTransform;
    Color fontSmoothingBackgroundColor;
    
    mozilla::gfx::Point deviceOffset;
  };

  
  void EnsurePath();
  
  void EnsurePathBuilder();
  void FillAzure(const Pattern& aPattern, mozilla::gfx::Float aOpacity);
  void PushClipsToDT(mozilla::gfx::DrawTarget *aDT);
  CompositionOp GetOp();
  void ChangeTransform(const mozilla::gfx::Matrix &aNewMatrix, bool aUpdatePatternTransform = true);
  Rect GetAzureDeviceSpaceClipBounds();
  Matrix GetDeviceTransform() const;
  Matrix GetDTTransform() const;
  void PushNewDT(gfxContentType content);

  bool mPathIsRect;
  bool mTransformChanged;
  Matrix mPathTransform;
  Rect mRect;
  mozilla::RefPtr<PathBuilder> mPathBuilder;
  mozilla::RefPtr<Path> mPath;
  Matrix mTransform;
  nsTArray<AzureState> mStateStack;

  AzureState &CurrentState() { return mStateStack[mStateStack.Length() - 1]; }
  const AzureState &CurrentState() const { return mStateStack[mStateStack.Length() - 1]; }

  cairo_t *mRefCairo;

  mozilla::RefPtr<DrawTarget> mDT;
  mozilla::RefPtr<DrawTarget> mOriginalDT;
};






class gfxContextAutoSaveRestore
{
public:
  gfxContextAutoSaveRestore() : mContext(nullptr) {}

  explicit gfxContextAutoSaveRestore(gfxContext *aContext) : mContext(aContext) {
    mContext->Save();
  }

  ~gfxContextAutoSaveRestore() {
    Restore();
  }

  void SetContext(gfxContext *aContext) {
    NS_ASSERTION(!mContext, "Not going to call Restore() on some context!!!");
    mContext = aContext;
    mContext->Save();    
  }

  void EnsureSaved(gfxContext *aContext) {
    MOZ_ASSERT(!mContext || mContext == aContext, "wrong context");
    if (!mContext) {
        mContext = aContext;
        mContext->Save();
    }
  }

  void Restore() {
    if (mContext) {
      mContext->Restore();
      mContext = nullptr;
    }
  }

private:
  gfxContext *mContext;
};






class gfxContextMatrixAutoSaveRestore
{
public:
    gfxContextMatrixAutoSaveRestore() :
        mContext(nullptr)
    {
    }

    explicit gfxContextMatrixAutoSaveRestore(gfxContext *aContext) :
        mContext(aContext), mMatrix(aContext->CurrentMatrix())
    {
    }

    ~gfxContextMatrixAutoSaveRestore()
    {
        if (mContext) {
            mContext->SetMatrix(mMatrix);
        }
    }

    void SetContext(gfxContext *aContext)
    {
        NS_ASSERTION(!mContext, "Not going to restore the matrix on some context!");
        mContext = aContext;
        mMatrix = aContext->CurrentMatrix();
    }

    void Restore()
    {
        if (mContext) {
            mContext->SetMatrix(mMatrix);
        }
    }

    const gfxMatrix& Matrix()
    {
        MOZ_ASSERT(mContext, "mMatrix doesn't contain a useful matrix");
        return mMatrix;
    }

private:
    gfxContext *mContext;
    gfxMatrix   mMatrix;
};


class gfxContextAutoDisableSubpixelAntialiasing {
public:
    gfxContextAutoDisableSubpixelAntialiasing(gfxContext *aContext, bool aDisable)
    {
        if (aDisable) {
            mDT = aContext->GetDrawTarget();
            mSubpixelAntialiasingEnabled = mDT->GetPermitSubpixelAA();
            mDT->SetPermitSubpixelAA(false);
        }
    }
    ~gfxContextAutoDisableSubpixelAntialiasing()
    {
        if (mDT) {
            mDT->SetPermitSubpixelAA(mSubpixelAntialiasingEnabled);
        }
    }

private:
    mozilla::RefPtr<mozilla::gfx::DrawTarget> mDT;
    bool mSubpixelAntialiasingEnabled;
};




class PatternFromState
{
public:
  explicit PatternFromState(gfxContext *aContext) : mContext(aContext), mPattern(nullptr) {}
  ~PatternFromState() { if (mPattern) { mPattern->~Pattern(); } }

  operator mozilla::gfx::Pattern&();

private:
  union {
    mozilla::AlignedStorage2<mozilla::gfx::ColorPattern> mColorPattern;
    mozilla::AlignedStorage2<mozilla::gfx::SurfacePattern> mSurfacePattern;
  };

  gfxContext *mContext;
  mozilla::gfx::Pattern *mPattern;
};

#endif 
