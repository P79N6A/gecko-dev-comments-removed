




#ifndef GFX_CONTEXT_H
#define GFX_CONTEXT_H

#include "gfxTypes.h"

#include "gfxASurface.h"
#include "gfxColor.h"
#include "gfxPoint.h"
#include "gfxRect.h"
#include "gfxMatrix.h"
#include "gfxPattern.h"
#include "gfxPath.h"
#include "nsISupportsImpl.h"
#include "nsTArray.h"

#include "mozilla/gfx/2D.h"

typedef struct _cairo cairo_t;
struct GlyphBufferAzure;
template <typename T> class FallibleTArray;















class THEBES_API gfxContext {
    NS_INLINE_DECL_REFCOUNTING(gfxContext)

public:
    


    gfxContext(gfxASurface *surface);

    


    gfxContext(mozilla::gfx::DrawTarget *aTarget);

    ~gfxContext();

    


    gfxASurface *OriginalSurface();

    





    already_AddRefed<gfxASurface> CurrentSurface(gfxFloat *dx, gfxFloat *dy);
    already_AddRefed<gfxASurface> CurrentSurface() {
        return CurrentSurface(NULL, NULL);
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

    


    already_AddRefed<gfxPath> CopyPath() const;

    


    void AppendPath(gfxPath* path);

    


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

    





    void Ellipse(const gfxPoint& center, const gfxSize& dimensions);

    


    void Polygon(const gfxPoint *points, PRUint32 numPoints);

    







    void RoundedRectangle(const gfxRect& rect,
                          const gfxCornerSizes& corners,
                          bool draw_clockwise = true);

    



    



    void Translate(const gfxPoint& pt);

    



    void Scale(gfxFloat x, gfxFloat y);

    





    void Rotate(gfxFloat angle);

    




    void Multiply(const gfxMatrix& other);

    


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
        OPERATOR_SATURATE
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

    


    void PushGroup(gfxASurface::gfxContentType content = gfxASurface::CONTENT_COLOR);
    










    void PushGroupAndCopyBackground(gfxASurface::gfxContentType content = gfxASurface::CONTENT_COLOR);
    already_AddRefed<gfxPattern> PopGroup();
    void PopGroupToSource();

    


    bool PointInFill(const gfxPoint& pt);
    bool PointInStroke(const gfxPoint& pt);

    


    gfxRect GetUserPathExtent();
    gfxRect GetUserFillExtent();
    gfxRect GetUserStrokeExtent();

    


    already_AddRefed<gfxFlattenedPath> GetFlattenedPath();

    



    enum {
        










        FLAG_SIMPLIFY_OPERATORS = (1 << 0),
        



        FLAG_DISABLE_SNAPPING = (1 << 1),
        


        FLAG_DISABLE_COPY_BACKGROUND = (1 << 2)
    };

    void SetFlag(PRInt32 aFlag) { mFlags |= aFlag; }
    void ClearFlag(PRInt32 aFlag) { mFlags &= ~aFlag; }
    PRInt32 GetFlags() const { return mFlags; }

    bool IsCairo() const { return !mDT; }

#ifdef MOZ_DUMP_PAINTING
    



    


    void WriteAsPNG(const char* aFile);

    


    void DumpAsDataURL();

    


    void CopyAsDataURL();
#endif

private:
  friend class GeneralPattern;
  friend struct GlyphBufferAzure;

  typedef mozilla::gfx::Matrix Matrix;
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::Color Color;
  typedef mozilla::gfx::StrokeOptions StrokeOptions;
  typedef mozilla::gfx::Float Float;
  typedef mozilla::gfx::Rect Rect;
  typedef mozilla::gfx::CompositionOp CompositionOp;
  typedef mozilla::gfx::Path Path;
  typedef mozilla::gfx::PathBuilder PathBuilder;
  typedef mozilla::gfx::SourceSurface SourceSurface;
  
  struct AzureState {
    AzureState()
      : op(mozilla::gfx::OP_OVER)
      , opIsClear(false)
      , color(0, 0, 0, 1.0f)
      , clipWasReset(false)
      , fillRule(mozilla::gfx::FILL_WINDING)
      , aaMode(mozilla::gfx::AA_SUBPIXEL)
      , patternTransformChanged(false)
    {}

    mozilla::gfx::CompositionOp op;
    bool opIsClear;
    Color color;
    nsRefPtr<gfxPattern> pattern;
    mozilla::RefPtr<SourceSurface> sourceSurface;
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
  };

  
  void EnsurePath();
  
  void EnsurePathBuilder();
  void FillAzure(mozilla::gfx::Float aOpacity);
  void PushClipsToDT(mozilla::gfx::DrawTarget *aDT);
  CompositionOp GetOp();
  void TransformWillChange();

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

  cairo_t *mCairo;
  cairo_t *mRefCairo;
  nsRefPtr<gfxASurface> mSurface;
  PRInt32 mFlags;

  mozilla::RefPtr<DrawTarget> mDT;
  mozilla::RefPtr<DrawTarget> mOriginalDT;
};






class THEBES_API gfxContextAutoSaveRestore
{
public:
  gfxContextAutoSaveRestore() : mContext(nsnull) {}

  gfxContextAutoSaveRestore(gfxContext *aContext) : mContext(aContext) {
    mContext->Save();
  }

  ~gfxContextAutoSaveRestore() {
    if (mContext) {
      mContext->Restore();
    }
  }

  void SetContext(gfxContext *aContext) {
    NS_ASSERTION(!mContext, "Not going to call Restore() on some context!!!");
    mContext = aContext;
    mContext->Save();    
  }

  void Reset(gfxContext *aContext) {
    
    NS_PRECONDITION(aContext, "must provide a context");
    if (mContext) {
      mContext->Restore();
    }
    mContext = aContext;
    mContext->Save();
  }

private:
  gfxContext *mContext;
};








class THEBES_API gfxContextPathAutoSaveRestore
{
public:
    gfxContextPathAutoSaveRestore() : mContext(nsnull) {}

    gfxContextPathAutoSaveRestore(gfxContext *aContext, bool aSave = true) : mContext(aContext)
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
            mPath = mContext->CopyPath();
        }
    }

    



    void Restore()
    {
        if (mPath) {
            mContext->NewPath();
            mContext->AppendPath(mPath);
            mPath = nsnull;
        }
    }

private:
    gfxContext *mContext;

    nsRefPtr<gfxPath> mPath;
};






class THEBES_API gfxContextMatrixAutoSaveRestore
{
public:
    gfxContextMatrixAutoSaveRestore(gfxContext *aContext) :
        mContext(aContext), mMatrix(aContext->CurrentMatrix())
    {
    }

    ~gfxContextMatrixAutoSaveRestore()
    {
        mContext->SetMatrix(mMatrix);
    }

    const gfxMatrix& Matrix()
    {
        return mMatrix;
    }

private:
    gfxContext *mContext;
    gfxMatrix   mMatrix;
};


class THEBES_API gfxContextAutoDisableSubpixelAntialiasing {
public:
    gfxContextAutoDisableSubpixelAntialiasing(gfxContext *aContext, bool aDisable)
    {
        if (aDisable) {
            if (aContext->IsCairo()) {
                mSurface = aContext->CurrentSurface();
                if (!mSurface) {
                  return;
                }
                mSubpixelAntialiasingEnabled = mSurface->GetSubpixelAntialiasingEnabled();
                mSurface->SetSubpixelAntialiasingEnabled(false);
            } else {
                mDT = aContext->GetDrawTarget();

                mSubpixelAntialiasingEnabled = mDT->GetPermitSubpixelAA();
                mDT->SetPermitSubpixelAA(false);
            }
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
