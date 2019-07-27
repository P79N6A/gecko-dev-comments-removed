




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















class gfxContext MOZ_FINAL {
    typedef mozilla::gfx::Path Path;

    NS_INLINE_DECL_REFCOUNTING(gfxContext)

public:

    




    explicit gfxContext(mozilla::gfx::DrawTarget *aTarget,
                        const mozilla::gfx::Point& aDeviceOffset = mozilla::gfx::Point());

    




    static already_AddRefed<gfxContext> ContextForDrawTarget(mozilla::gfx::DrawTarget* aTarget);

    


    gfxASurface *OriginalSurface();

    





    already_AddRefed<gfxASurface> CurrentSurface(gfxFloat *dx, gfxFloat *dy);
    already_AddRefed<gfxASurface> CurrentSurface() {
        return CurrentSurface(nullptr, nullptr);
    }

    



    cairo_t *GetCairo();

    mozilla::gfx::DrawTarget *GetDrawTarget() { return mDT; }

    


    bool HasError();

    


    
    void Save();
    void Restore();

    



    






    void Stroke();
    




    void Fill();

    





    void FillWithOpacity(gfxFloat aOpacity);

    


    void NewPath();

    




    void ClosePath();

    


    mozilla::TemporaryRef<Path> GetPath();

    


    void SetPath(Path* path);

    


    void MoveTo(const gfxPoint& pt);

    



    void NewSubPath();

    


    gfxPoint CurrentPoint();

    




    void LineTo(const gfxPoint& pt);

    


    void CurveTo(const gfxPoint& pt1, const gfxPoint& pt2, const gfxPoint& pt3);

    


    void QuadraticCurveTo(const gfxPoint& pt1, const gfxPoint& pt2);

    






    void Arc(const gfxPoint& center, gfxFloat radius,
             gfxFloat angle1, gfxFloat angle2);

    







    void NegativeArc(const gfxPoint& center, gfxFloat radius,
                     gfxFloat angle1, gfxFloat angle2);

    
    


    void Line(const gfxPoint& start, const gfxPoint& end); 

    



    void Rectangle(const gfxRect& rect, bool snapToPixels = false);
    void SnappedRectangle(const gfxRect& rect) { return Rectangle(rect, true); }

    





    void Ellipse(const gfxPoint& center, const gfxSize& dimensions);

    


    void Polygon(const gfxPoint *points, uint32_t numPoints);

    







    void RoundedRectangle(const gfxRect& rect,
                          const gfxCornerSizes& corners,
                          bool draw_clockwise = true);

    



    





    void Rotate(gfxFloat angle);

    




    void Multiply(const gfxMatrix& other);
    




    void MultiplyAndNudgeToIntegers(const gfxMatrix& other);

    


    void SetMatrix(const gfxMatrix& matrix);

    


    void IdentityMatrix();

    


    gfxMatrix CurrentMatrix() const;

    




    void NudgeCurrentMatrixToIntegers();

    



    gfxPoint DeviceToUser(const gfxPoint& point) const;

    



    gfxSize DeviceToUser(const gfxSize& size) const;

    




    gfxRect DeviceToUser(const gfxRect& rect) const;

    



    gfxPoint UserToDevice(const gfxPoint& point) const;

    



    gfxSize UserToDevice(const gfxSize& size) const;

    




    gfxRect UserToDevice(const gfxRect& rect) const;

    










    bool UserToDevicePixelSnapped(gfxRect& rect, bool ignoreScale = false) const;

    










    bool UserToDevicePixelSnapped(gfxPoint& pt, bool ignoreScale = false) const;

    






    void PixelSnappedRectangleAndSetPattern(const gfxRect& rect, gfxPattern *pattern);

    



    



    void SetDeviceColor(const gfxRGBA& c);

    




    bool GetDeviceColor(gfxRGBA& c);

    




    void SetColor(const gfxRGBA& c);

    






    void SetSource(gfxASurface *surface, const gfxPoint& offset = gfxPoint(0.0, 0.0));

    


    void SetPattern(gfxPattern *pattern);

    


    already_AddRefed<gfxPattern> GetPattern();

    


    



    void Paint(gfxFloat alpha = 1.0);

    


    



    void Mask(gfxPattern *pattern);

    



    void Mask(gfxASurface *surface, const gfxPoint& offset = gfxPoint(0.0, 0.0));

    void Mask(mozilla::gfx::SourceSurface *surface, const mozilla::gfx::Point& offset = mozilla::gfx::Point());

    



    



    void DrawSurface(gfxASurface *surface, const gfxSize& size);

    



    typedef enum {
        gfxLineSolid,
        gfxLineDashed,
        gfxLineDotted
    } gfxLineType;

    void SetDash(gfxLineType ltype);
    void SetDash(gfxFloat *dashes, int ndash, gfxFloat offset);
    
    
    
    bool CurrentDash(FallibleTArray<gfxFloat>& dashes, gfxFloat* offset) const;
    
    gfxFloat CurrentDashOffset() const;

    


    void SetLineWidth(gfxFloat width);

    




    gfxFloat CurrentLineWidth() const;

    enum GraphicsLineCap {
        LINE_CAP_BUTT,
        LINE_CAP_ROUND,
        LINE_CAP_SQUARE
    };
    


    void SetLineCap(GraphicsLineCap cap);
    GraphicsLineCap CurrentLineCap() const;

    enum GraphicsLineJoin {
        LINE_JOIN_MITER,
        LINE_JOIN_ROUND,
        LINE_JOIN_BEVEL
    };
    



    void SetLineJoin(GraphicsLineJoin join);
    GraphicsLineJoin CurrentLineJoin() const;

    void SetMiterLimit(gfxFloat limit);
    gfxFloat CurrentMiterLimit() const;

    



    enum FillRule {
        FILL_RULE_WINDING,
        FILL_RULE_EVEN_ODD
    };
    void SetFillRule(FillRule rule);
    FillRule CurrentFillRule() const;

    



    
    enum GraphicsOperator {
        OPERATOR_CLEAR,
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

    




    enum AntialiasMode {
        MODE_ALIASED,
        MODE_COVERAGE
    };
    void SetAntialiasMode(AntialiasMode mode);
    AntialiasMode CurrentAntialiasMode() const;

    



    



    void Clip();

    



    void ResetClip();

    



    void Clip(const gfxRect& rect); 

    



    void UpdateSurfaceClip();

    



    gfxRect GetClipExtents();

    




    bool ClipContainsRect(const gfxRect& aRect);

    


    void PushGroup(gfxContentType content = gfxContentType::COLOR);
    










    void PushGroupAndCopyBackground(gfxContentType content = gfxContentType::COLOR);
    already_AddRefed<gfxPattern> PopGroup();
    void PopGroupToSource();

    


    bool PointInFill(const gfxPoint& pt);
    bool PointInStroke(const gfxPoint& pt);

    


    gfxRect GetUserPathExtent();
    gfxRect GetUserFillExtent();
    gfxRect GetUserStrokeExtent();

    mozilla::gfx::Point GetDeviceOffset() const;

    



    enum {
        










        FLAG_SIMPLIFY_OPERATORS = (1 << 0),
        



        FLAG_DISABLE_SNAPPING = (1 << 1),
        


        FLAG_DISABLE_COPY_BACKGROUND = (1 << 2)
    };

    void SetFlag(int32_t aFlag) { mFlags |= aFlag; }
    void ClearFlag(int32_t aFlag) { mFlags &= ~aFlag; }
    int32_t GetFlags() const { return mFlags; }

    
    void GetRoundOffsetsToPixels(bool *aRoundX, bool *aRoundY);

#ifdef MOZ_DUMP_PAINTING
    



    


    void WriteAsPNG(const char* aFile);

    


    void DumpAsDataURI();

    


    void CopyAsDataURI();
#endif

    static mozilla::gfx::UserDataKey sDontUseAsSourceKey;

private:
    ~gfxContext();

  friend class GeneralPattern;
  friend class GlyphBufferAzure;

  typedef mozilla::gfx::Matrix Matrix;
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::Color Color;
  typedef mozilla::gfx::StrokeOptions StrokeOptions;
  typedef mozilla::gfx::Float Float;
  typedef mozilla::gfx::Rect Rect;
  typedef mozilla::gfx::CompositionOp CompositionOp;
  typedef mozilla::gfx::PathBuilder PathBuilder;
  typedef mozilla::gfx::SourceSurface SourceSurface;
  
  struct AzureState {
    AzureState()
      : op(mozilla::gfx::CompositionOp::OP_OVER)
      , opIsClear(false)
      , color(0, 0, 0, 1.0f)
      , clipWasReset(false)
      , fillRule(mozilla::gfx::FillRule::FILL_WINDING)
      , aaMode(mozilla::gfx::AntialiasMode::SUBPIXEL)
      , patternTransformChanged(false)
    {}

    mozilla::gfx::CompositionOp op;
    bool opIsClear;
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
    
    mozilla::gfx::Point deviceOffset;
  };

  
  void EnsurePath();
  
  void EnsurePathBuilder();
  void FillAzure(mozilla::gfx::Float aOpacity);
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
  nsRefPtr<gfxASurface> mSurface;
  int32_t mFlags;

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








class gfxContextPathAutoSaveRestore
{
    typedef mozilla::gfx::Path Path;

public:
    gfxContextPathAutoSaveRestore() : mContext(nullptr) {}

    explicit gfxContextPathAutoSaveRestore(gfxContext *aContext, bool aSave = true) : mContext(aContext)
    {
        if (aSave)
            Save();       
    }

    ~gfxContextPathAutoSaveRestore()
    {
        Restore();
    }

    void SetContext(gfxContext *aContext, bool aSave = true)
    {
        mContext = aContext;
        if (aSave)
            Save();
    }

    



    void Save()
    {
        if (!mPath && mContext) {
            mPath = mContext->GetPath();
        }
    }

    



    void Restore()
    {
        if (mPath) {
            mContext->SetPath(mPath);
            mPath = nullptr;
        }
    }

private:
    gfxContext *mContext;

    mozilla::RefPtr<Path> mPath;
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
        if (mSurface) {
            mSurface->SetSubpixelAntialiasingEnabled(mSubpixelAntialiasingEnabled);
        } else if (mDT) {
            mDT->SetPermitSubpixelAA(mSubpixelAntialiasingEnabled);
        }
    }

private:
    nsRefPtr<gfxASurface> mSurface;
    mozilla::RefPtr<mozilla::gfx::DrawTarget> mDT;
    bool mSubpixelAntialiasingEnabled;
};

#endif 
