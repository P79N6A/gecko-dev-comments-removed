




































#include "base/basictypes.h"

#include "nsIDOMXULElement.h"

#include "prmem.h"
#include "prenv.h"

#include "nsIServiceManager.h"
#include "nsMathUtils.h"

#include "nsContentUtils.h"

#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIDOMCanvasRenderingContext2D.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsHTMLCanvasElement.h"
#include "nsSVGEffects.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIVariant.h"

#include "nsIInterfaceRequestorUtils.h"
#include "nsIFrame.h"
#include "nsDOMError.h"
#include "nsIScriptError.h"

#include "nsCSSParser.h"
#include "mozilla/css/StyleRule.h"
#include "mozilla/css/Declaration.h"
#include "nsComputedDOMStyle.h"
#include "nsStyleSet.h"

#include "nsPrintfCString.h"

#include "nsReadableUtils.h"

#include "nsColor.h"
#include "nsGfxCIID.h"
#include "nsIScriptSecurityManager.h"
#include "nsIDocShell.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIXPConnect.h"
#include "jsapi.h"
#include "nsDisplayList.h"

#include "nsTArray.h"

#include "imgIEncoder.h"

#include "gfxContext.h"
#include "gfxASurface.h"
#include "gfxImageSurface.h"
#include "gfxPlatform.h"
#include "gfxFont.h"
#include "gfxTextRunCache.h"
#include "gfxBlur.h"
#include "gfxUtils.h"

#include "nsFrameManager.h"
#include "nsFrameLoader.h"
#include "nsBidiPresUtils.h"
#include "Layers.h"
#include "CanvasUtils.h"
#include "nsIMemoryReporter.h"
#include "nsStyleUtil.h"
#include "CanvasImageCache.h"

#include <algorithm>
#include "mozilla/dom/ContentParent.h"
#include "mozilla/ipc/PDocumentRendererParent.h"
#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/ipc/DocumentRendererParent.h"

#include "mozilla/gfx/2D.h"

#ifdef XP_WIN
#include "gfxWindowsPlatform.h"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


#undef DrawText

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::dom;
using namespace mozilla::gfx;
using namespace mozilla::ipc;
using namespace mozilla::css;

namespace mgfx = mozilla::gfx;

static float kDefaultFontSize = 10.0;
static NS_NAMED_LITERAL_STRING(kDefaultFontName, "sans-serif");
static NS_NAMED_LITERAL_STRING(kDefaultFontStyle, "10px sans-serif");


#define VALIDATE(_f)  if (!NS_finite(_f)) return PR_FALSE

static PRBool FloatValidate (double f1) {
  VALIDATE(f1);
  return PR_TRUE;
}

static PRBool FloatValidate (double f1, double f2) {
  VALIDATE(f1); VALIDATE(f2);
  return PR_TRUE;
}

static PRBool FloatValidate (double f1, double f2, double f3) {
  VALIDATE(f1); VALIDATE(f2); VALIDATE(f3);
  return PR_TRUE;
}

static PRBool FloatValidate (double f1, double f2, double f3, double f4) {
  VALIDATE(f1); VALIDATE(f2); VALIDATE(f3); VALIDATE(f4);
  return PR_TRUE;
}

static PRBool FloatValidate (double f1, double f2, double f3, double f4, double f5) {
  VALIDATE(f1); VALIDATE(f2); VALIDATE(f3); VALIDATE(f4); VALIDATE(f5);
  return PR_TRUE;
}

static PRBool FloatValidate (double f1, double f2, double f3, double f4, double f5, double f6) {
  VALIDATE(f1); VALIDATE(f2); VALIDATE(f3); VALIDATE(f4); VALIDATE(f5); VALIDATE(f6);
  return PR_TRUE;
}

#undef VALIDATE


static nsIMemoryReporter *gCanvasAzureMemoryReporter = nsnull;
static PRInt64 gCanvasAzureMemoryUsed = 0;

static PRInt64 GetCanvasAzureMemoryUsed(void *) {
  return gCanvasAzureMemoryUsed;
}




NS_MEMORY_REPORTER_IMPLEMENT(CanvasAzureMemory,
  "canvas-2d-pixel-bytes",
  MR_OTHER,
  "Memory used by 2D canvases. Each canvas requires (width * height * 4) "
  "bytes.",
  GetCanvasAzureMemoryUsed,
  nsnull)




#define NS_CANVASGRADIENTAZURE_PRIVATE_IID \
    {0x28425a6a, 0x90e0, 0x4d42, {0x9c, 0x75, 0xff, 0x60, 0x09, 0xb3, 0x10, 0xa8}}
class nsCanvasGradientAzure : public nsIDOMCanvasGradient
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_CANVASGRADIENTAZURE_PRIVATE_IID)

  enum Type
  {
    LINEAR = 0,
    RADIAL
  };

  Type GetType()
  {
    return mType;
  }


  GradientStops *GetGradientStopsForTarget(DrawTarget *aRT)
  {
    if (mStops && mStops->GetBackendType() == aRT->GetType()) {
      return mStops;
    }

    mStops = aRT->CreateGradientStops(mRawStops.Elements(), mRawStops.Length());

    return mStops;
  }

  NS_DECL_ISUPPORTS

protected:
  nsCanvasGradientAzure(Type aType) : mType(aType)
  {}

  nsTArray<GradientStop> mRawStops;
  RefPtr<GradientStops> mStops;
  Type mType;
};

class nsCanvasRadialGradientAzure : public nsCanvasGradientAzure
{
public:
  nsCanvasRadialGradientAzure(const Point &aBeginOrigin, Float aBeginRadius,
                              const Point &aEndOrigin, Float aEndRadius)
    : nsCanvasGradientAzure(RADIAL)
    , mCenter(aEndOrigin)
    , mRadius(aEndRadius)
  {
    mOffsetStart = aBeginRadius / mRadius;

    mOffsetRatio = 1 - mOffsetStart;
    mOrigin = ((mCenter * aBeginRadius) - (aBeginOrigin * mRadius)) /
              (aBeginRadius - mRadius);
  }


  
  NS_IMETHOD AddColorStop (float offset,
                            const nsAString& colorstr)
  {
    if (!FloatValidate(offset) || offset < 0.0 || offset > 1.0) {
      return NS_ERROR_DOM_INDEX_SIZE_ERR;
    }

    nscolor color;
    nsCSSParser parser;
    nsresult rv = parser.ParseColorString(nsString(colorstr),
                                          nsnull, 0, &color);
    if (NS_FAILED(rv)) {
      return NS_ERROR_DOM_SYNTAX_ERR;
    }

    mStops = nsnull;

    GradientStop newStop;

    newStop.offset = offset * mOffsetRatio + mOffsetStart;
    newStop.color = Color::FromABGR(color);

    mRawStops.AppendElement(newStop);

    return NS_OK;
  }

  
  Point mCenter;
  Float mRadius;
  Point mOrigin;

  Float mOffsetStart;
  Float mOffsetRatio;
};

class nsCanvasLinearGradientAzure : public nsCanvasGradientAzure
{
public:
  nsCanvasLinearGradientAzure(const Point &aBegin, const Point &aEnd)
    : nsCanvasGradientAzure(LINEAR)
    , mBegin(aBegin)
    , mEnd(aEnd)
  {
  }

  
  NS_IMETHOD AddColorStop (float offset,
                            const nsAString& colorstr)
  {
    if (!FloatValidate(offset) || offset < 0.0 || offset > 1.0) {
      return NS_ERROR_DOM_INDEX_SIZE_ERR;
    }

    nscolor color;
    nsCSSParser parser;
    nsresult rv = parser.ParseColorString(nsString(colorstr),
                                          nsnull, 0, &color);
    if (NS_FAILED(rv)) {
      return NS_ERROR_DOM_SYNTAX_ERR;
    }

    mStops = nsnull;

    GradientStop newStop;

    newStop.offset = offset;
    newStop.color = Color::FromABGR(color);

    mRawStops.AppendElement(newStop);

    return NS_OK;
  }

protected:
  friend class nsCanvasRenderingContext2DAzure;

  
  Point mBegin;
  
  Point mEnd;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsCanvasGradientAzure, NS_CANVASGRADIENTAZURE_PRIVATE_IID)

NS_IMPL_ADDREF(nsCanvasGradientAzure)
NS_IMPL_RELEASE(nsCanvasGradientAzure)




NS_INTERFACE_MAP_BEGIN(nsCanvasGradientAzure)
  NS_INTERFACE_MAP_ENTRY(nsCanvasGradientAzure)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCanvasGradient)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CanvasGradient)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END




#define NS_CANVASPATTERNAZURE_PRIVATE_IID \
    {0xc9bacc25, 0x28da, 0x421e, {0x9a, 0x4b, 0xbb, 0xd6, 0x93, 0x05, 0x12, 0xbc}}
class nsCanvasPatternAzure : public nsIDOMCanvasPattern
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_CANVASPATTERNAZURE_PRIVATE_IID)

  enum RepeatMode
  {
    REPEAT,
    REPEATX,
    REPEATY,
    NOREPEAT
  };

  nsCanvasPatternAzure(SourceSurface* aSurface,
                       RepeatMode aRepeat,
                       nsIPrincipal* principalForSecurityCheck,
                       PRBool forceWriteOnly)
    : mSurface(aSurface)
    , mRepeat(aRepeat)
    , mPrincipal(principalForSecurityCheck)
    , mForceWriteOnly(forceWriteOnly)
  {
  }

  NS_DECL_ISUPPORTS

  RefPtr<SourceSurface> mSurface;
  RepeatMode mRepeat;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  PRPackedBool mForceWriteOnly;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsCanvasPatternAzure, NS_CANVASPATTERNAZURE_PRIVATE_IID)

NS_IMPL_ADDREF(nsCanvasPatternAzure)
NS_IMPL_RELEASE(nsCanvasPatternAzure)




NS_INTERFACE_MAP_BEGIN(nsCanvasPatternAzure)
  NS_INTERFACE_MAP_ENTRY(nsCanvasPatternAzure)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCanvasPattern)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CanvasPattern)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END




#define NS_TEXTMETRICSAZURE_PRIVATE_IID \
  {0x9793f9e7, 0x9dc1, 0x4e9c, {0x81, 0xc8, 0xfc, 0xa7, 0x14, 0xf4, 0x30, 0x79}}
class nsTextMetricsAzure : public nsIDOMTextMetrics
{
public:
  nsTextMetricsAzure(float w) : width(w) { }

  virtual ~nsTextMetricsAzure() { }

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_TEXTMETRICSAZURE_PRIVATE_IID)

  NS_IMETHOD GetWidth(float* w) {
    *w = width;
    return NS_OK;
  }

  NS_DECL_ISUPPORTS

private:
  float width;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsTextMetricsAzure, NS_TEXTMETRICSAZURE_PRIVATE_IID)

NS_IMPL_ADDREF(nsTextMetricsAzure)
NS_IMPL_RELEASE(nsTextMetricsAzure)




NS_INTERFACE_MAP_BEGIN(nsTextMetricsAzure)
  NS_INTERFACE_MAP_ENTRY(nsTextMetricsAzure)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTextMetrics)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(TextMetrics)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

struct nsCanvasBidiProcessorAzure;


static const Float SIGMA_MAX = 100;




class nsCanvasRenderingContext2DAzure :
  public nsIDOMCanvasRenderingContext2D,
  public nsICanvasRenderingContextInternal
{
public:
  nsCanvasRenderingContext2DAzure();
  virtual ~nsCanvasRenderingContext2DAzure();

  nsresult Redraw();

  
  NS_IMETHOD SetCanvasElement(nsHTMLCanvasElement* aParentCanvas);
  NS_IMETHOD SetDimensions(PRInt32 width, PRInt32 height);
  NS_IMETHOD InitializeWithSurface(nsIDocShell *shell, gfxASurface *surface, PRInt32 width, PRInt32 height)
  { return NS_ERROR_NOT_IMPLEMENTED; }

  NS_IMETHOD Render(gfxContext *ctx, gfxPattern::GraphicsFilter aFilter);
  NS_IMETHOD GetInputStream(const char* aMimeType,
                            const PRUnichar* aEncoderOptions,
                            nsIInputStream **aStream);
  NS_IMETHOD GetThebesSurface(gfxASurface **surface);

  TemporaryRef<SourceSurface> GetSurfaceSnapshot()
  { return mTarget ? mTarget->Snapshot() : nsnull; }

  NS_IMETHOD SetIsOpaque(PRBool isOpaque);
  NS_IMETHOD Reset();
  already_AddRefed<CanvasLayer> GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                                CanvasLayer *aOldLayer,
                                                LayerManager *aManager);
  void MarkContextClean();
  NS_IMETHOD SetIsIPC(PRBool isIPC);
  
  void Redraw(const mgfx::Rect &r);
  NS_IMETHOD Redraw(const gfxRect &r) { Redraw(ToRect(r)); return NS_OK; }

  
  nsresult RedrawUser(const gfxRect &r);

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsCanvasRenderingContext2DAzure, nsIDOMCanvasRenderingContext2D)

  
  NS_DECL_NSIDOMCANVASRENDERINGCONTEXT2D

  enum Style {
    STYLE_STROKE = 0,
    STYLE_FILL,
    STYLE_MAX
  };

protected:
  nsresult InitializeWithTarget(DrawTarget *surface, PRInt32 width, PRInt32 height);

  



  static PRUint32 sNumLivingContexts;

  


  static PRUint8 (*sUnpremultiplyTable)[256];

  


  static PRUint8 (*sPremultiplyTable)[256];

  
  nsresult SetStyleFromStringOrInterface(const nsAString& aStr, nsISupports *aInterface, Style aWhichStyle);
  nsresult GetStyleAsStringOrInterface(nsAString& aStr, nsISupports **aInterface, PRInt32 *aType, Style aWhichStyle);

  void StyleColorToString(const nscolor& aColor, nsAString& aStr);

  


  void EnsureUnpremultiplyTable();

  


  void EnsurePremultiplyTable();

  



  void EnsureWritablePath();

  
  void EnsureUserSpacePath();

  void TransformWillUpdate();

  
  void FillRuleChanged();

  



  SurfaceFormat GetSurfaceFormat() const;

  nsHTMLCanvasElement *HTMLCanvasElement() {
    return static_cast<nsHTMLCanvasElement*>(mCanvasElement.get());
  }

  
  PRInt32 mWidth, mHeight;

  
  
  
  PRPackedBool mValid;
  
  
  PRPackedBool mZero;

  PRPackedBool mOpaque;

  
  
  PRPackedBool mResetLayer;
  
  PRPackedBool mIPC;

  
  nsCOMPtr<nsIDOMHTMLCanvasElement> mCanvasElement;

  
  nsCOMPtr<nsIDocShell> mDocShell;

  
  RefPtr<DrawTarget> mTarget;

  



  PRPackedBool mIsEntireFrameInvalid;
  




  PRPackedBool mPredictManyRedrawCalls;

  























  RefPtr<Path> mPath;
  RefPtr<PathBuilder> mDSPathBuilder;
  RefPtr<PathBuilder> mPathBuilder;
  bool mPathTransformWillUpdate;
  Matrix mPathToDS;

  


  PRUint32 mInvalidateCount;
  static const PRUint32 kCanvasMaxInvalidateCount = 100;

  



  PRBool NeedToDrawShadow()
  {
    const ContextState& state = CurrentState();

    
    
    return state.op == OP_OVER && NS_GET_A(state.shadowColor) != 0;
  }

  


  nsIPresShell *GetPresShell() {
    nsCOMPtr<nsIContent> content =
      do_QueryInterface(static_cast<nsIDOMHTMLCanvasElement*>(mCanvasElement));
    if (content) {
      nsIDocument* ownerDoc = content->GetOwnerDoc();
      return ownerDoc ? ownerDoc->GetShell() : nsnull;
    }
    if (mDocShell) {
      nsCOMPtr<nsIPresShell> shell;
      mDocShell->GetPresShell(getter_AddRefs(shell));
      return shell.get();
    }
    return nsnull;
  }

  
  enum TextAlign {
    TEXT_ALIGN_START,
    TEXT_ALIGN_END,
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_RIGHT,
    TEXT_ALIGN_CENTER
  };

  enum TextBaseline {
    TEXT_BASELINE_TOP,
    TEXT_BASELINE_HANGING,
    TEXT_BASELINE_MIDDLE,
    TEXT_BASELINE_ALPHABETIC,
    TEXT_BASELINE_IDEOGRAPHIC,
    TEXT_BASELINE_BOTTOM
  };

  gfxFontGroup *GetCurrentFontStyle();

  enum TextDrawOperation {
    TEXT_DRAW_OPERATION_FILL,
    TEXT_DRAW_OPERATION_STROKE,
    TEXT_DRAW_OPERATION_MEASURE
  };

  



  nsresult DrawOrMeasureText(const nsAString& text,
                              float x,
                              float y,
                              float maxWidth,
                              TextDrawOperation op,
                              float* aWidth);

  
  class ContextState {
  public:
      ContextState() : textAlign(TEXT_ALIGN_START),
                       textBaseline(TEXT_BASELINE_ALPHABETIC),
                       lineWidth(1.0f),
                       miterLimit(10.0f),
                       globalAlpha(1.0f),
                       shadowBlur(0.0),
                       op(OP_OVER),
                       fillRule(FILL_WINDING),
                       lineCap(CAP_BUTT),
                       lineJoin(JOIN_MITER_OR_BEVEL),
                       imageSmoothingEnabled(PR_TRUE)
      { }

      ContextState(const ContextState& other)
          : fontGroup(other.fontGroup),
            font(other.font),
            textAlign(other.textAlign),
            textBaseline(other.textBaseline),
            shadowColor(other.shadowColor),
            transform(other.transform),
            shadowOffset(other.shadowOffset),
            lineWidth(other.lineWidth),
            miterLimit(other.miterLimit),
            globalAlpha(other.globalAlpha),
            shadowBlur(other.shadowBlur),
            op(other.op),
            fillRule(FILL_WINDING),
            lineCap(other.lineCap),
            lineJoin(other.lineJoin),
            imageSmoothingEnabled(other.imageSmoothingEnabled)
      {
          for (int i = 0; i < STYLE_MAX; i++) {
              colorStyles[i] = other.colorStyles[i];
              gradientStyles[i] = other.gradientStyles[i];
              patternStyles[i] = other.patternStyles[i];
          }
      }

      void SetColorStyle(Style whichStyle, nscolor color) {
          colorStyles[whichStyle] = color;
          gradientStyles[whichStyle] = nsnull;
          patternStyles[whichStyle] = nsnull;
      }

      void SetPatternStyle(Style whichStyle, nsCanvasPatternAzure* pat) {
          gradientStyles[whichStyle] = nsnull;
          patternStyles[whichStyle] = pat;
      }

      void SetGradientStyle(Style whichStyle, nsCanvasGradientAzure* grad) {
          gradientStyles[whichStyle] = grad;
          patternStyles[whichStyle] = nsnull;
      }

      


      PRBool StyleIsColor(Style whichStyle) const
      {
          return !(patternStyles[whichStyle] ||
                    gradientStyles[whichStyle]);
      }


      std::vector<RefPtr<Path> > clipsPushed;

      nsRefPtr<gfxFontGroup> fontGroup;
      nsRefPtr<nsCanvasGradientAzure> gradientStyles[STYLE_MAX];
      nsRefPtr<nsCanvasPatternAzure> patternStyles[STYLE_MAX];

      nsString font;
      TextAlign textAlign;
      TextBaseline textBaseline;

      nscolor colorStyles[STYLE_MAX];
      nscolor shadowColor;

      Matrix transform;
      Point shadowOffset;
      Float lineWidth;
      Float miterLimit;
      Float globalAlpha;
      Float shadowBlur;

      CompositionOp op;
      FillRule fillRule;
      CapStyle lineCap;
      JoinStyle lineJoin;

      PRPackedBool imageSmoothingEnabled;
  };

  class GeneralPattern
  {
  public:
    GeneralPattern() : mPattern(nsnull) {}
    ~GeneralPattern()
    {
      if (mPattern) {
        mPattern->~Pattern();
      }
    }

    Pattern& ForStyle(nsCanvasRenderingContext2DAzure *aCtx,
                      Style aStyle,
                      DrawTarget *aRT)
    {
      
      
      NS_ASSERTION(!mPattern, "ForStyle() should only be called once on GeneralPattern!");

      const nsCanvasRenderingContext2DAzure::ContextState &state = aCtx->CurrentState();

      if (state.StyleIsColor(aStyle)) {
        mPattern = new (mColorPattern.addr()) ColorPattern(Color::FromABGR(state.colorStyles[aStyle]));
      } else if (state.gradientStyles[aStyle] &&
                 state.gradientStyles[aStyle]->GetType() == nsCanvasGradientAzure::LINEAR) {
        nsCanvasLinearGradientAzure *gradient =
          static_cast<nsCanvasLinearGradientAzure*>(state.gradientStyles[aStyle].get());

        mPattern = new (mLinearGradientPattern.addr())
          LinearGradientPattern(gradient->mBegin, gradient->mEnd,
                                gradient->GetGradientStopsForTarget(aRT));
      } else if (state.gradientStyles[aStyle] &&
                 state.gradientStyles[aStyle]->GetType() == nsCanvasGradientAzure::RADIAL) {
        nsCanvasRadialGradientAzure *gradient =
          static_cast<nsCanvasRadialGradientAzure*>(state.gradientStyles[aStyle].get());

        mPattern = new (mRadialGradientPattern.addr())
          RadialGradientPattern(gradient->mCenter, gradient->mOrigin, gradient->mRadius,
                                gradient->GetGradientStopsForTarget(aRT));
      } else if (state.patternStyles[aStyle]) {
        if (aCtx->mCanvasElement) {
          CanvasUtils::DoDrawImageSecurityCheck(aCtx->HTMLCanvasElement(),
                                                state.patternStyles[aStyle]->mPrincipal,
                                                state.patternStyles[aStyle]->mForceWriteOnly);
        }

        ExtendMode mode;
        if (state.patternStyles[aStyle]->mRepeat == nsCanvasPatternAzure::NOREPEAT) {
          mode = EXTEND_CLAMP;
        } else {
          mode = EXTEND_WRAP;
        }
        mPattern = new (mSurfacePattern.addr())
          SurfacePattern(state.patternStyles[aStyle]->mSurface, mode);
      }

      return *mPattern;
    }

    union {
      AlignedStorage2<ColorPattern> mColorPattern;
      AlignedStorage2<LinearGradientPattern> mLinearGradientPattern;
      AlignedStorage2<RadialGradientPattern> mRadialGradientPattern;
      AlignedStorage2<SurfacePattern> mSurfacePattern;
    };
    Pattern *mPattern;
  };

  



  class AdjustedTarget
  {
  public:
    AdjustedTarget(nsCanvasRenderingContext2DAzure *ctx,
                   const mgfx::Rect *aBounds = nsnull)
      : mCtx(nsnull)
    {
      if (!ctx->NeedToDrawShadow()) {
        mTarget = ctx->mTarget;
        return;
      }
      mCtx = ctx;

      const ContextState &state = mCtx->CurrentState();

      mSigma = state.shadowBlur / 2.0f;

      if (mSigma > SIGMA_MAX) {
        mSigma = SIGMA_MAX;
      }
        
      Matrix transform = mCtx->mTarget->GetTransform();
      if (!aBounds) {
        mTempSize = IntSize(ctx->mWidth, ctx->mHeight);

        
        
        if (state.shadowOffset.x > 0) {
          mTempSize.width += state.shadowOffset.x;
          mSurfOffset.x = -state.shadowOffset.x;
          transform._31 += state.shadowOffset.x;
        } else {
          mTempSize.width -= state.shadowOffset.x;
        }
        if (state.shadowOffset.y > 0) {
          mTempSize.height += state.shadowOffset.y;
          mSurfOffset.y = -state.shadowOffset.y;
          transform._32 += state.shadowOffset.y;
        } else {
          mTempSize.height -= state.shadowOffset.y;
        }

        if (mSigma > 0) {
          float blurRadius = mSigma * 3;
          mSurfOffset.x -= blurRadius;
          mSurfOffset.y -= blurRadius;
          mTempSize.width += blurRadius;
          mTempSize.height += blurRadius;
          transform._31 += blurRadius;
          transform._32 += blurRadius;
        }
      } 
        
      mTarget =
        mCtx->mTarget->CreateSimilarDrawTarget(mTempSize,
                                                FORMAT_B8G8R8A8);

      mTarget->SetTransform(transform);

      if (!mTarget) {
        
        
        mTarget = ctx->mTarget;
        mCtx = nsnull;
      }
    }

    ~AdjustedTarget()
    {
      if (!mCtx) {
        return;
      }

      RefPtr<SourceSurface> snapshot = mTarget->Snapshot();
      
      mCtx->mTarget->DrawSurfaceWithShadow(snapshot, mSurfOffset,
                                           Color::FromABGR(mCtx->CurrentState().shadowColor),
                                           mCtx->CurrentState().shadowOffset, mSigma);
    }

    DrawTarget* operator->()
    {
      return mTarget;
    }

  private:
    RefPtr<DrawTarget> mTarget;
    nsCanvasRenderingContext2DAzure *mCtx;
    Float mSigma;
    IntSize mTempSize;
    Point mSurfOffset;
  };

  nsAutoTArray<ContextState, 3> mStyleStack;

  inline ContextState& CurrentState() {
    return mStyleStack[mStyleStack.Length() - 1];
  }
    
  
  void GetAppUnitsValues(PRUint32 *perDevPixel, PRUint32 *perCSSPixel) {
    
    PRUint32 devPixel = 60;
    PRUint32 cssPixel = 60;

    nsIPresShell *ps = GetPresShell();
    nsPresContext *pc;

    if (!ps) goto FINISH;
    pc = ps->GetPresContext();
    if (!pc) goto FINISH;
    devPixel = pc->AppUnitsPerDevPixel();
    cssPixel = pc->AppUnitsPerCSSPixel();

  FINISH:
    if (perDevPixel)
      *perDevPixel = devPixel;
    if (perCSSPixel)
      *perCSSPixel = cssPixel;
  }

  friend struct nsCanvasBidiProcessorAzure;
};

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsCanvasRenderingContext2DAzure)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsCanvasRenderingContext2DAzure)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsCanvasRenderingContext2DAzure)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsCanvasRenderingContext2DAzure)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCanvasElement)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsCanvasRenderingContext2DAzure)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCanvasElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END




NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsCanvasRenderingContext2DAzure)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCanvasRenderingContext2D)
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextInternal)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMCanvasRenderingContext2D)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CanvasRenderingContext2D)
NS_INTERFACE_MAP_END







PRUint32 nsCanvasRenderingContext2DAzure::sNumLivingContexts = 0;
PRUint8 (*nsCanvasRenderingContext2DAzure::sUnpremultiplyTable)[256] = nsnull;
PRUint8 (*nsCanvasRenderingContext2DAzure::sPremultiplyTable)[256] = nsnull;

nsresult
NS_NewCanvasRenderingContext2DAzure(nsIDOMCanvasRenderingContext2D** aResult)
{
#ifndef XP_WIN
  return NS_ERROR_NOT_AVAILABLE;
#else

  if (gfxWindowsPlatform::GetPlatform()->GetRenderMode() !=
      gfxWindowsPlatform::RENDER_DIRECT2D ||
      !gfxWindowsPlatform::GetPlatform()->DWriteEnabled()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<nsIDOMCanvasRenderingContext2D> ctx = new nsCanvasRenderingContext2DAzure();
  if (!ctx)
    return NS_ERROR_OUT_OF_MEMORY;

  *aResult = ctx.forget().get();
  return NS_OK;
#endif
}

nsCanvasRenderingContext2DAzure::nsCanvasRenderingContext2DAzure()
  : mValid(PR_FALSE), mZero(PR_FALSE), mOpaque(PR_FALSE), mResetLayer(PR_TRUE)
  , mIPC(PR_FALSE)
  , mCanvasElement(nsnull)
  , mIsEntireFrameInvalid(PR_FALSE)
  , mPredictManyRedrawCalls(PR_FALSE), mPathTransformWillUpdate(false)
  , mInvalidateCount(0)
{
  sNumLivingContexts++;
}

nsCanvasRenderingContext2DAzure::~nsCanvasRenderingContext2DAzure()
{
  Reset();
  sNumLivingContexts--;
  if (!sNumLivingContexts) {
    delete[] sUnpremultiplyTable;
    delete[] sPremultiplyTable;
    sUnpremultiplyTable = nsnull;
    sPremultiplyTable = nsnull;
  }
}

nsresult
nsCanvasRenderingContext2DAzure::Reset()
{
  if (mCanvasElement) {
    HTMLCanvasElement()->InvalidateCanvas();
  }

  
  
  if (mValid && !mDocShell) {
    gCanvasAzureMemoryUsed -= mWidth * mHeight * 4;
  }

  mTarget = nsnull;
  mValid = PR_FALSE;
  mIsEntireFrameInvalid = PR_FALSE;
  mPredictManyRedrawCalls = PR_FALSE;
  return NS_OK;
}

nsresult
nsCanvasRenderingContext2DAzure::SetStyleFromStringOrInterface(const nsAString& aStr,
                                                               nsISupports *aInterface,
                                                               Style aWhichStyle)
{
  nsresult rv;
  nscolor color;

  if (!aStr.IsVoid()) {
    nsIDocument* document = mCanvasElement ?
                            HTMLCanvasElement()->GetOwnerDoc() : nsnull;

    
    
    nsCSSParser parser(document ? document->CSSLoader() : nsnull);
    rv = parser.ParseColorString(aStr, nsnull, 0, &color);
    if (NS_FAILED(rv)) {
      
      return NS_OK;
    }

    CurrentState().SetColorStyle(aWhichStyle, color);
    return NS_OK;
  }

  if (aInterface) {
    nsCOMPtr<nsCanvasGradientAzure> grad(do_QueryInterface(aInterface));
    if (grad) {
      CurrentState().SetGradientStyle(aWhichStyle, grad);
      return NS_OK;
    }

    nsCOMPtr<nsCanvasPatternAzure> pattern(do_QueryInterface(aInterface));
    if (pattern) {
      CurrentState().SetPatternStyle(aWhichStyle, pattern);
      return NS_OK;
    }
  }

  nsContentUtils::ReportToConsole(
    nsContentUtils::eDOM_PROPERTIES,
    "UnexpectedCanvasVariantStyle",
    nsnull, 0,
    nsnull,
    EmptyString(), 0, 0,
    nsIScriptError::warningFlag,
    "Canvas",
    mCanvasElement ? HTMLCanvasElement()->GetOwnerDoc() : nsnull);

  return NS_OK;
}

nsresult
nsCanvasRenderingContext2DAzure::GetStyleAsStringOrInterface(nsAString& aStr,
                                                             nsISupports **aInterface,
                                                             PRInt32 *aType,
                                                             Style aWhichStyle)
{
  const ContextState &state = CurrentState();

  if (state.patternStyles[aWhichStyle]) {
    aStr.SetIsVoid(PR_TRUE);
    NS_ADDREF(*aInterface = state.patternStyles[aWhichStyle]);
    *aType = CMG_STYLE_PATTERN;
  } else if (state.gradientStyles[aWhichStyle]) {
    aStr.SetIsVoid(PR_TRUE);
    NS_ADDREF(*aInterface = state.gradientStyles[aWhichStyle]);
    *aType = CMG_STYLE_GRADIENT;
  } else {
    StyleColorToString(state.colorStyles[aWhichStyle], aStr);
    *aInterface = nsnull;
    *aType = CMG_STYLE_STRING;
  }

  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::StyleColorToString(const nscolor& aColor, nsAString& aStr)
{
  
  
  if (NS_GET_A(aColor) == 255) {
    CopyUTF8toUTF16(nsPrintfCString(100, "#%02x%02x%02x",
                                    NS_GET_R(aColor),
                                    NS_GET_G(aColor),
                                    NS_GET_B(aColor)),
                    aStr);
  } else {
    CopyUTF8toUTF16(nsPrintfCString(100, "rgba(%d, %d, %d, ",
                                    NS_GET_R(aColor),
                                    NS_GET_G(aColor),
                                    NS_GET_B(aColor)),
                    aStr);
    aStr.AppendFloat(nsStyleUtil::ColorComponentToFloat(NS_GET_A(aColor)));
    aStr.Append(')');
  }
}

nsresult
nsCanvasRenderingContext2DAzure::Redraw()
{
  if (mIsEntireFrameInvalid) {
    return NS_OK;
  }

  mIsEntireFrameInvalid = PR_TRUE;

  if (!mCanvasElement) {
    NS_ASSERTION(mDocShell, "Redraw with no canvas element or docshell!");
    return NS_OK;
  }

  nsSVGEffects::InvalidateDirectRenderingObservers(HTMLCanvasElement());

  HTMLCanvasElement()->InvalidateCanvasContent(nsnull);

  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::Redraw(const mgfx::Rect &r)
{
  ++mInvalidateCount;

  if (mIsEntireFrameInvalid) {
    return;
  }

  if (mPredictManyRedrawCalls ||
    mInvalidateCount > kCanvasMaxInvalidateCount) {
    Redraw();
    return;
  }

  if (!mCanvasElement) {
    NS_ASSERTION(mDocShell, "Redraw with no canvas element or docshell!");
    return;
  }

  nsSVGEffects::InvalidateDirectRenderingObservers(HTMLCanvasElement());

  gfxRect tmpR = GFXRect(r);
  HTMLCanvasElement()->InvalidateCanvasContent(&tmpR);

  return;
}

nsresult
nsCanvasRenderingContext2DAzure::RedrawUser(const gfxRect& r)
{
  if (mIsEntireFrameInvalid) {
    ++mInvalidateCount;
    return NS_OK;
  }

  mgfx::Rect newr =
    mTarget->GetTransform().TransformBounds(ToRect(r));
  Redraw(newr);

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetDimensions(PRInt32 width, PRInt32 height)
{
  RefPtr<DrawTarget> target;

  
  if (height == 0 || width == 0) {
    mZero = PR_TRUE;
    height = 1;
    width = 1;
  }

  
  IntSize size(width, height);
  if (size.width <= 0xFFFF && size.height <= 0xFFFF &&
      size.width >= 0 && size.height >= 0) {
    SurfaceFormat format = GetSurfaceFormat();
    nsCOMPtr<nsIContent> content =
      do_QueryInterface(static_cast<nsIDOMHTMLCanvasElement*>(mCanvasElement));
    nsIDocument* ownerDoc = nsnull;
    if (content) {
      ownerDoc = content->GetOwnerDoc();
    }

    nsRefPtr<LayerManager> layerManager = nsnull;

    if (ownerDoc) {
      layerManager =
        nsContentUtils::PersistentLayerManagerForDocument(ownerDoc);
    }

    if (layerManager) {
      target = layerManager->CreateDrawTarget(size, format);
    } else {
      target = Factory::CreateDrawTarget(BACKEND_DIRECT2D, size, format);
    }
  }

  if (target) {
    if (gCanvasAzureMemoryReporter == nsnull) {
        gCanvasAzureMemoryReporter = new NS_MEMORY_REPORTER_NAME(CanvasAzureMemory);
      NS_RegisterMemoryReporter(gCanvasAzureMemoryReporter);
    }

    gCanvasAzureMemoryUsed += width * height * 4;
    JS_updateMallocCounter(nsContentUtils::GetCurrentJSContext(), width * height * 4);
  }

  return InitializeWithTarget(target, width, height);
}

nsresult
nsCanvasRenderingContext2DAzure::InitializeWithTarget(DrawTarget *target, PRInt32 width, PRInt32 height)
{
  Reset();

  NS_ASSERTION(mCanvasElement, "Must have a canvas element!");
  mDocShell = nsnull;

  mWidth = width;
  mHeight = height;

  mTarget = target;

  mResetLayer = PR_TRUE;

  


  if (!target)
  {
    mTarget = Factory::CreateDrawTarget(BACKEND_DIRECT2D, IntSize(1, 1), FORMAT_B8G8R8A8);
  } else {
    mValid = PR_TRUE;
  }

  
  mStyleStack.Clear();
  mPathBuilder = nsnull;
  mPath = nsnull;
  mDSPathBuilder = nsnull;

  ContextState *state = mStyleStack.AppendElement();
  state->globalAlpha = 1.0;

  state->colorStyles[STYLE_FILL] = NS_RGB(0,0,0);
  state->colorStyles[STYLE_STROKE] = NS_RGB(0,0,0);
  state->shadowColor = NS_RGBA(0,0,0,0);

  mTarget->ClearRect(mgfx::Rect(Point(0, 0), Size(mWidth, mHeight)));
    
  
  
  Redraw();

  return mValid ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetIsOpaque(PRBool isOpaque)
{
  if (isOpaque == mOpaque)
    return NS_OK;

  mOpaque = isOpaque;

  if (mValid) {
    


    return SetDimensions(mWidth, mHeight);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetIsIPC(PRBool isIPC)
{
  if (isIPC == mIPC)
      return NS_OK;

  mIPC = isIPC;

  if (mValid) {
    


    return SetDimensions(mWidth, mHeight);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Render(gfxContext *ctx, gfxPattern::GraphicsFilter aFilter)
{
  nsresult rv = NS_OK;

  if (!mValid || !mTarget) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<gfxASurface> surface;
  
  if (NS_FAILED(GetThebesSurface(getter_AddRefs(surface)))) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<gfxPattern> pat = new gfxPattern(surface);

  pat->SetFilter(aFilter);
  pat->SetExtend(gfxPattern::EXTEND_PAD);

  gfxContext::GraphicsOperator op = ctx->CurrentOperator();
  if (mOpaque)
      ctx->SetOperator(gfxContext::OPERATOR_SOURCE);

  
  
  ctx->NewPath();
  ctx->PixelSnappedRectangleAndSetPattern(gfxRect(0, 0, mWidth, mHeight), pat);
  ctx->Fill();

  if (mOpaque)
      ctx->SetOperator(op);

  return rv;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetInputStream(const char *aMimeType,
                                                const PRUnichar *aEncoderOptions,
                                                nsIInputStream **aStream)
{
  if (!mValid || !mTarget) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<gfxASurface> surface;

  if (NS_FAILED(GetThebesSurface(getter_AddRefs(surface)))) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv;
  const char encoderPrefix[] = "@mozilla.org/image/encoder;2?type=";
  nsAutoArrayPtr<char> conid(new (std::nothrow) char[strlen(encoderPrefix) + strlen(aMimeType) + 1]);

  if (!conid) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  strcpy(conid, encoderPrefix);
  strcat(conid, aMimeType);

  nsCOMPtr<imgIEncoder> encoder = do_CreateInstance(conid);
  if (!encoder) {
    return NS_ERROR_FAILURE;
  }

  nsAutoArrayPtr<PRUint8> imageBuffer(new (std::nothrow) PRUint8[mWidth * mHeight * 4]);
  if (!imageBuffer) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsRefPtr<gfxImageSurface> imgsurf =
    new gfxImageSurface(imageBuffer.get(),
                        gfxIntSize(mWidth, mHeight),
                        mWidth * 4,
                        gfxASurface::ImageFormatARGB32);

  if (!imgsurf || imgsurf->CairoStatus()) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<gfxContext> ctx = new gfxContext(imgsurf);

  if (!ctx || ctx->HasError()) {
    return NS_ERROR_FAILURE;
  }

  ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
  ctx->SetSource(surface, gfxPoint(0, 0));
  ctx->Paint();

  rv = encoder->InitFromData(imageBuffer.get(),
                              mWidth * mHeight * 4, mWidth, mHeight, mWidth * 4,
                              imgIEncoder::INPUT_FORMAT_HOSTARGB,
                              nsDependentString(aEncoderOptions));
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(encoder, aStream);
}

SurfaceFormat
nsCanvasRenderingContext2DAzure::GetSurfaceFormat() const
{
  return mOpaque ? FORMAT_B8G8R8X8 : FORMAT_B8G8R8A8;
}





NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetCanvasElement(nsHTMLCanvasElement* aCanvasElement)
{
  mCanvasElement = aCanvasElement;

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetCanvas(nsIDOMHTMLCanvasElement **canvas)
{
  NS_IF_ADDREF(*canvas = mCanvasElement);

  return NS_OK;
}





NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Save()
{
  mStyleStack[mStyleStack.Length() - 1].transform = mTarget->GetTransform();
  mStyleStack.SetCapacity(mStyleStack.Length() + 1);
  mStyleStack.AppendElement(CurrentState());
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Restore()
{
  if (mStyleStack.Length() - 1 == 0)
    return NS_OK;

  for (PRUint32 i = 0; i < CurrentState().clipsPushed.size(); i++) {
    mTarget->PopClip();
  }

  mStyleStack.RemoveElementAt(mStyleStack.Length() - 1);

  TransformWillUpdate();

  mTarget->SetTransform(CurrentState().transform);

  return NS_OK;
}





NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Scale(float x, float y)
{
  if (!FloatValidate(x,y))
    return NS_OK;

  TransformWillUpdate();

  Matrix newMatrix = mTarget->GetTransform();
  mTarget->SetTransform(newMatrix.Scale(x, y));
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Rotate(float angle)
{
  if (!FloatValidate(angle))
    return NS_OK;

  TransformWillUpdate();

  Matrix rotation = Matrix::Rotation(angle);
  mTarget->SetTransform(rotation * mTarget->GetTransform());
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Translate(float x, float y)
{
  if (!FloatValidate(x,y)) {
    return NS_OK;
  }

  TransformWillUpdate();

  Matrix newMatrix = mTarget->GetTransform();
  mTarget->SetTransform(newMatrix.Translate(x, y));
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Transform(float m11, float m12, float m21, float m22, float dx, float dy)
{
  if (!FloatValidate(m11,m12,m21,m22,dx,dy)) {
    return NS_OK;
  }

  TransformWillUpdate();

  Matrix matrix(m11, m12, m21, m22, dx, dy);
  mTarget->SetTransform(matrix * mTarget->GetTransform());
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetTransform(float m11, float m12, float m21, float m22, float dx, float dy)
{
  if (!FloatValidate(m11,m12,m21,m22,dx,dy)) {
    return NS_OK;
  }

  TransformWillUpdate();

  Matrix matrix(m11, m12, m21, m22, dx, dy);
  mTarget->SetTransform(matrix);

  return NS_OK;
}





NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetGlobalAlpha(float aGlobalAlpha)
{
  if (!FloatValidate(aGlobalAlpha) || aGlobalAlpha < 0.0 || aGlobalAlpha > 1.0) {
    return NS_OK;
  }

  CurrentState().globalAlpha = aGlobalAlpha;
    
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetGlobalAlpha(float *aGlobalAlpha)
{
  *aGlobalAlpha = CurrentState().globalAlpha;
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetStrokeStyle(nsIVariant *aValue)
{
  if (!aValue)
      return NS_ERROR_FAILURE;

  nsString str;

  nsresult rv;
  PRUint16 vtype;
  rv = aValue->GetDataType(&vtype);
  NS_ENSURE_SUCCESS(rv, rv);

  if (vtype == nsIDataType::VTYPE_INTERFACE ||
      vtype == nsIDataType::VTYPE_INTERFACE_IS)
  {
    nsIID *iid;
    nsCOMPtr<nsISupports> sup;
    rv = aValue->GetAsInterface(&iid, getter_AddRefs(sup));
    NS_ENSURE_SUCCESS(rv, rv);
    if (iid) {
      NS_Free(iid);
    }

    str.SetIsVoid(PR_TRUE);
    return SetStrokeStyle_multi(str, sup);
  }

  rv = aValue->GetAsAString(str);
  NS_ENSURE_SUCCESS(rv, rv);

  return SetStrokeStyle_multi(str, nsnull);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetStrokeStyle(nsIVariant **aResult)
{
  nsCOMPtr<nsIWritableVariant> wv = do_CreateInstance(NS_VARIANT_CONTRACTID);

  nsCOMPtr<nsISupports> sup;
  nsString str;
  PRInt32 t;
  nsresult rv = GetStrokeStyle_multi(str, getter_AddRefs(sup), &t);
  NS_ENSURE_SUCCESS(rv, rv);

  if (t == CMG_STYLE_STRING) {
    rv = wv->SetAsAString(str);
  } else if (t == CMG_STYLE_PATTERN) {
    rv = wv->SetAsInterface(NS_GET_IID(nsIDOMCanvasPattern),
                            sup);
  } else if (t == CMG_STYLE_GRADIENT) {
    rv = wv->SetAsInterface(NS_GET_IID(nsIDOMCanvasGradient),
                            sup);
  } else {
    NS_ERROR("Unknown type from GetStroke/FillStyle_multi!");
    return NS_ERROR_FAILURE;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*aResult = wv.get());
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetFillStyle(nsIVariant *aValue)
{
  if (!aValue) {
    return NS_ERROR_FAILURE;
  }

  nsString str;
  nsresult rv;
  PRUint16 vtype;
  rv = aValue->GetDataType(&vtype);
  NS_ENSURE_SUCCESS(rv, rv);

  if (vtype == nsIDataType::VTYPE_INTERFACE ||
      vtype == nsIDataType::VTYPE_INTERFACE_IS)
  {
    nsIID *iid;
    nsCOMPtr<nsISupports> sup;
    rv = aValue->GetAsInterface(&iid, getter_AddRefs(sup));
    NS_ENSURE_SUCCESS(rv, rv);

    str.SetIsVoid(PR_TRUE);
    return SetFillStyle_multi(str, sup);
  }

  rv = aValue->GetAsAString(str);
  NS_ENSURE_SUCCESS(rv, rv);

  return SetFillStyle_multi(str, nsnull);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetFillStyle(nsIVariant **aResult)
{
  nsCOMPtr<nsIWritableVariant> wv = do_CreateInstance(NS_VARIANT_CONTRACTID);

  nsCOMPtr<nsISupports> sup;
  nsString str;
  PRInt32 t;
  nsresult rv = GetFillStyle_multi(str, getter_AddRefs(sup), &t);
  NS_ENSURE_SUCCESS(rv, rv);

  if (t == CMG_STYLE_STRING) {
    rv = wv->SetAsAString(str);
  } else if (t == CMG_STYLE_PATTERN) {
    rv = wv->SetAsInterface(NS_GET_IID(nsIDOMCanvasPattern),
                            sup);
  } else if (t == CMG_STYLE_GRADIENT) {
    rv = wv->SetAsInterface(NS_GET_IID(nsIDOMCanvasGradient),
                            sup);
  } else {
    NS_ERROR("Unknown type from GetStroke/FillStyle_multi!");
    return NS_ERROR_FAILURE;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*aResult = wv.get());
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozFillRule(const nsAString& aString)
{
  FillRule rule;

  if (aString.EqualsLiteral("evenodd"))
    rule = FILL_EVEN_ODD;
  else if (aString.EqualsLiteral("nonzero"))
    rule = FILL_WINDING;
  else
    return NS_OK;

  CurrentState().fillRule = rule;
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozFillRule(nsAString& aString)
{
    switch (CurrentState().fillRule) {
    case FILL_WINDING:
        aString.AssignLiteral("nonzero"); break;
    case FILL_EVEN_ODD:
        aString.AssignLiteral("evenodd"); break;
    default:
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetStrokeStyle_multi(const nsAString& aStr, nsISupports *aInterface)
{
    return SetStyleFromStringOrInterface(aStr, aInterface, STYLE_STROKE);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetStrokeStyle_multi(nsAString& aStr, nsISupports **aInterface, PRInt32 *aType)
{
    return GetStyleAsStringOrInterface(aStr, aInterface, aType, STYLE_STROKE);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetFillStyle_multi(const nsAString& aStr, nsISupports *aInterface)
{
    return SetStyleFromStringOrInterface(aStr, aInterface, STYLE_FILL);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetFillStyle_multi(nsAString& aStr, nsISupports **aInterface, PRInt32 *aType)
{
    return GetStyleAsStringOrInterface(aStr, aInterface, aType, STYLE_FILL);
}




NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::CreateLinearGradient(float x0, float y0, float x1, float y1,
                                                      nsIDOMCanvasGradient **_retval)
{
  if (!FloatValidate(x0,y0,x1,y1)) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  nsRefPtr<nsIDOMCanvasGradient> grad =
    new nsCanvasLinearGradientAzure(Point(x0, y0), Point(x1, y1));

  *_retval = grad.forget().get();
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::CreateRadialGradient(float x0, float y0, float r0,
                                                      float x1, float y1, float r1,
                                                      nsIDOMCanvasGradient **_retval)
{
  if (!FloatValidate(x0,y0,r0,x1,y1,r1)) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  if (r0 < 0.0 || r1 < 0.0) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsRefPtr<nsIDOMCanvasGradient> grad =
    new nsCanvasRadialGradientAzure(Point(x0, y0), r0, Point(x1, y1), r1);

  *_retval = grad.forget().get();
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::CreatePattern(nsIDOMHTMLElement *image,
                                               const nsAString& repeat,
                                               nsIDOMCanvasPattern **_retval)
{
  if (!image) {
    return NS_ERROR_DOM_TYPE_MISMATCH_ERR;
  }

  nsCanvasPatternAzure::RepeatMode repeatMode =
    nsCanvasPatternAzure::NOREPEAT;

  if (repeat.IsEmpty() || repeat.EqualsLiteral("repeat")) {
    repeatMode = nsCanvasPatternAzure::REPEAT;
  } else if (repeat.EqualsLiteral("repeat-x")) {
    repeatMode = nsCanvasPatternAzure::REPEATX;
  } else if (repeat.EqualsLiteral("repeat-y")) {
    repeatMode = nsCanvasPatternAzure::REPEATY;
  } else if (repeat.EqualsLiteral("no-repeat")) {
    repeatMode = nsCanvasPatternAzure::NOREPEAT;
  } else {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  nsCOMPtr<nsIContent> content = do_QueryInterface(image);
  nsHTMLCanvasElement* canvas = nsHTMLCanvasElement::FromContent(content);

  if (canvas) {
    nsIntSize size = canvas->GetSize();
    if (size.width == 0 || size.height == 0) {
      return NS_ERROR_DOM_INVALID_STATE_ERR;
    }
  }

  
  nsCOMPtr<nsINode> node = do_QueryInterface(image);
  if (canvas && node) {
    if (canvas->CountContexts() == 1) {
      nsICanvasRenderingContextInternal *srcCanvas = canvas->GetContextAtIndex(0);

      
      if (srcCanvas) {
        RefPtr<SourceSurface> srcSurf = srcCanvas->GetSurfaceSnapshot();

        nsRefPtr<nsCanvasPatternAzure> pat =
          new nsCanvasPatternAzure(srcSurf, repeatMode, node->NodePrincipal(), canvas->IsWriteOnly());

        *_retval = pat.forget().get();
        return NS_OK;
      }
    }
  }

  
  
  nsLayoutUtils::SurfaceFromElementResult res =
    nsLayoutUtils::SurfaceFromElement(image, nsLayoutUtils::SFE_WANT_FIRST_FRAME |
                                              nsLayoutUtils::SFE_WANT_NEW_SURFACE);

  if (!res.mSurface) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  if (!res.mSurface->CairoSurface()) {
    return NS_OK;
  }

  RefPtr<SourceSurface> srcSurf =
    gfxPlatform::GetPlatform()->GetSourceSurfaceForSurface(mTarget, res.mSurface);

  nsRefPtr<nsCanvasPatternAzure> pat =
    new nsCanvasPatternAzure(srcSurf, repeatMode, res.mPrincipal, res.mIsWriteOnly);

  *_retval = pat.forget().get();
  return NS_OK;
}




NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetShadowOffsetX(float x)
{
  if (!FloatValidate(x)) {
    return NS_OK;
  }

  CurrentState().shadowOffset.x = x;
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetShadowOffsetX(float *x)
{
  *x = static_cast<float>(CurrentState().shadowOffset.x);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetShadowOffsetY(float y)
{
  if (!FloatValidate(y)) {
    return NS_OK;
  }

  CurrentState().shadowOffset.y = y;
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetShadowOffsetY(float *y)
{
  *y = static_cast<float>(CurrentState().shadowOffset.y);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetShadowBlur(float blur)
{
  if (!FloatValidate(blur) || blur < 0.0) {
    return NS_OK;
  }

  CurrentState().shadowBlur = blur;
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetShadowBlur(float *blur)
{
  *blur = CurrentState().shadowBlur;
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetShadowColor(const nsAString& colorstr)
{
  nsIDocument* document = mCanvasElement ?
                          HTMLCanvasElement()->GetOwnerDoc() : nsnull;

  
  
  nsCSSParser parser(document ? document->CSSLoader() : nsnull);
  nscolor color;
  nsresult rv = parser.ParseColorString(colorstr, nsnull, 0, &color);
  if (NS_FAILED(rv)) {
    
    return NS_OK;
  }
  CurrentState().shadowColor = color;

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetShadowColor(nsAString& color)
{
  StyleColorToString(CurrentState().shadowColor, color);

  return NS_OK;
}





NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::ClearRect(float x, float y, float w, float h)
{
  if (!FloatValidate(x,y,w,h)) {
    return NS_OK;
  }
 
  mTarget->ClearRect(mgfx::Rect(x, y, w, h));

  return RedrawUser(gfxRect(x, y, w, h));
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::FillRect(float x, float y, float w, float h)
{
  if (!FloatValidate(x,y,w,h)) {
    return NS_OK;
  }

  bool doDrawShadow = NeedToDrawShadow();

  const ContextState &state = CurrentState();

  if (state.patternStyles[STYLE_FILL]) {
    nsCanvasPatternAzure::RepeatMode repeat = 
      state.patternStyles[STYLE_FILL]->mRepeat;
    
    bool limitx = repeat == nsCanvasPatternAzure::NOREPEAT || repeat == nsCanvasPatternAzure::REPEATY;
    bool limity = repeat == nsCanvasPatternAzure::NOREPEAT || repeat == nsCanvasPatternAzure::REPEATX;

    IntSize patternSize =
      state.patternStyles[STYLE_FILL]->mSurface->GetSize();

    
    
    if (limitx) {
      if (x < 0) {
        w += x;
        if (w < 0) {
          w = 0;
        }

        x = 0;
      }
      if (x + w > patternSize.width) {
        w = patternSize.width - x;
        if (w < 0) {
          w = 0;
        }
      }
    }
    if (limity) {
      if (y < 0) {
        h += y;
        if (h < 0) {
          h = 0;
        }

        y = 0;
      }
      if (y + h > patternSize.height) {
        h = patternSize.height - y;
        if (h < 0) {
          h = 0;
        }
      }
    }
  }

  AdjustedTarget(this)->FillRect(mgfx::Rect(x, y, w, h),
                                  GeneralPattern().ForStyle(this, STYLE_FILL, mTarget),
                                  DrawOptions(state.globalAlpha, state.op));

  return RedrawUser(gfxRect(x, y, w, h));
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::StrokeRect(float x, float y, float w, float h)
{
  if (!FloatValidate(x,y,w,h)) {
    return NS_OK;
  }

  const ContextState &state = CurrentState();

  if (!w && !h) {
    return NS_OK;
  } else if (!h) {
    CapStyle cap = CAP_BUTT;
    if (state.lineJoin == JOIN_ROUND) {
      cap = CAP_ROUND;
    }
    AdjustedTarget(this)->
      StrokeLine(Point(x, y), Point(x + w, y),
                  GeneralPattern().ForStyle(this, STYLE_STROKE, mTarget),
                  StrokeOptions(state.lineWidth, state.lineJoin,
                                cap, state.miterLimit),
                  DrawOptions(state.globalAlpha, state.op));
    return NS_OK;
  } else if (!w) {
    CapStyle cap = CAP_BUTT;
    if (state.lineJoin == JOIN_ROUND) {
      cap = CAP_ROUND;
    }
    AdjustedTarget(this)->
      StrokeLine(Point(x, y), Point(x, y + h),
                  GeneralPattern().ForStyle(this, STYLE_STROKE, mTarget),
                  StrokeOptions(state.lineWidth, state.lineJoin,
                                cap, state.miterLimit),
                  DrawOptions(state.globalAlpha, state.op));
    return NS_OK;
  }

  AdjustedTarget(this)->
    StrokeRect(mgfx::Rect(x, y, w, h),
                GeneralPattern().ForStyle(this, STYLE_STROKE, mTarget),
                StrokeOptions(state.lineWidth, state.lineJoin,
                              state.lineCap, state.miterLimit),
                DrawOptions(state.globalAlpha, state.op));

  return Redraw();
}





NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::BeginPath()
{
  mPath = nsnull;
  mPathBuilder = nsnull;
  mDSPathBuilder = nsnull;
  mPathTransformWillUpdate = false;
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::ClosePath()
{
  EnsureWritablePath();

  if (mPathBuilder) {
    mPathBuilder->Close();
  } else {
    mDSPathBuilder->Close();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Fill()
{
  EnsureUserSpacePath();

  if (!mPath) {
    return NS_OK;
  }

  AdjustedTarget(this)->
    Fill(mPath, GeneralPattern().ForStyle(this, STYLE_FILL, mTarget),
         DrawOptions(CurrentState().globalAlpha, CurrentState().op));

  return Redraw();
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Stroke()
{
  EnsureUserSpacePath();

  if (!mPath) {
    return NS_OK;
  }

  const ContextState &state = CurrentState();

  AdjustedTarget(this)->
    Stroke(mPath, GeneralPattern().ForStyle(this, STYLE_STROKE, mTarget),
            StrokeOptions(state.lineWidth, state.lineJoin,
                          state.lineCap, state.miterLimit),
            DrawOptions(state.globalAlpha, state.op));

  return Redraw();
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Clip()
{
  EnsureUserSpacePath();

  if (!mPath) {
    return NS_OK;
  }

  mTarget->PushClip(mPath);
  CurrentState().clipsPushed.push_back(mPath);

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MoveTo(float x, float y)
{
  if (!FloatValidate(x,y))
      return NS_OK;

  EnsureWritablePath();

  if (mPathBuilder) {
    mPathBuilder->MoveTo(Point(x, y));
  } else {
    mDSPathBuilder->MoveTo(mTarget->GetTransform() * Point(x, y));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::LineTo(float x, float y)
{
  if (!FloatValidate(x,y))
      return NS_OK;

  EnsureWritablePath();
    
  if (mPathBuilder) {
    mPathBuilder->LineTo(Point(x, y));
  } else {
    mDSPathBuilder->LineTo(mTarget->GetTransform() * Point(x, y));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::QuadraticCurveTo(float cpx, float cpy, float x, float y)
{
  if (!FloatValidate(cpx, cpy, x, y)) {
    return NS_OK;
  }

  EnsureWritablePath();

  if (mPathBuilder) {
    mPathBuilder->QuadraticBezierTo(Point(cpx, cpy), Point(x, y));
  } else {
    Matrix transform = mTarget->GetTransform();
    mDSPathBuilder->QuadraticBezierTo(transform * Point(cpx, cpy), transform * Point(cpx, cpy));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::BezierCurveTo(float cp1x, float cp1y,
                                              float cp2x, float cp2y,
                                              float x, float y)
{
  if (!FloatValidate(cp1x, cp1y, cp2x, cp2y, x, y)) {
    return NS_OK;
  }

  EnsureWritablePath();

  if (mPathBuilder) {
    mPathBuilder->BezierTo(Point(cp1x, cp1y), Point(cp2x, cp2y), Point(x, y));
  } else {
    Matrix transform = mTarget->GetTransform();
    mDSPathBuilder->BezierTo(transform * Point(cp1x, cp1y),
                              transform * Point(cp2x, cp2y),
                              transform * Point(x, y));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::ArcTo(float x1, float y1, float x2, float y2, float radius)
{
  if (!FloatValidate(x1, y1, x2, y2, radius)) {
    return NS_OK;
  }

  if (radius < 0) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  EnsureWritablePath();

  
  Point p0;
  if (mPathBuilder) {
    p0 = mPathBuilder->CurrentPoint();
  } else {
    Matrix invTransform = mTarget->GetTransform();
    if (!invTransform.Invert()) {
      return NS_OK;
    }

    p0 = invTransform * mDSPathBuilder->CurrentPoint();
  }

  Point p1(x1, y1);
  Point p2(x2, y2);

  
  
  double dir, a2, b2, c2, cosx, sinx, d, anx, any,
          bnx, bny, x3, y3, x4, y4, cx, cy, angle0, angle1;
  bool anticlockwise;

  if (p0 == p1 || p1 == p2 || radius == 0) {
    LineTo(p1.x, p1.y);
    return NS_OK;
  }

  
  dir = (p2.x - p1.x) * (p0.y - p1.y) + (p2.y - p1.y) * (p1.x - p0.x);
  if (dir == 0) {
    LineTo(p1.x, p1.y);
    return NS_OK;
  }


  
  
  
  a2 = (p0.x-x1)*(p0.x-x1) + (p0.y-y1)*(p0.y-y1);
  b2 = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
  c2 = (p0.x-x2)*(p0.x-x2) + (p0.y-y2)*(p0.y-y2);
  cosx = (a2+b2-c2)/(2*sqrt(a2*b2));

  sinx = sqrt(1 - cosx*cosx);
  d = radius / ((1 - cosx) / sinx);

  anx = (x1-p0.x) / sqrt(a2);
  any = (y1-p0.y) / sqrt(a2);
  bnx = (x1-x2) / sqrt(b2);
  bny = (y1-y2) / sqrt(b2);
  x3 = x1 - anx*d;
  y3 = y1 - any*d;
  x4 = x1 - bnx*d;
  y4 = y1 - bny*d;
  anticlockwise = (dir < 0);
  cx = x3 + any*radius*(anticlockwise ? 1 : -1);
  cy = y3 - anx*radius*(anticlockwise ? 1 : -1);
  angle0 = atan2((y3-cy), (x3-cx));
  angle1 = atan2((y4-cy), (x4-cx));


  LineTo(x3, y3);

  Arc(cx, cy, radius, angle0, angle1, anticlockwise);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Arc(float x, float y,
                                     float r,
                                     float startAngle, float endAngle,
                                     PRBool ccw)
{
  if (!FloatValidate(x, y, r, startAngle, endAngle)) {
    return NS_OK;
  }

  if (r < 0.0)
      return NS_ERROR_DOM_INDEX_SIZE_ERR;

  EnsureWritablePath();

  
  

  Point startPoint(x + cos(startAngle) * r, y + sin(startAngle) * r);

  if (mPathBuilder) {
    mPathBuilder->LineTo(startPoint);
  } else {
    mDSPathBuilder->LineTo(mTarget->GetTransform() * startPoint);
  }

  
  
  if (!ccw && (endAngle < startAngle)) {
    Float correction = ceil((startAngle - endAngle) / (2.0f * M_PI));
    endAngle += correction * 2.0f * M_PI;
  } else if (ccw && (startAngle < endAngle)) {
    Float correction = ceil((endAngle - startAngle) / (2.0f * M_PI));
    startAngle += correction * 2.0f * M_PI;
  }

  
  if (!ccw && (endAngle - startAngle > 2 * M_PI)) {
    endAngle = startAngle + 2.0f * M_PI;
  } else if (ccw && (startAngle - endAngle > 2.0f * M_PI)) {
    endAngle = startAngle - 2.0f * M_PI;
  }

  
  Float arcSweepLeft = abs(endAngle - startAngle);
  
  Float curves = ceil(arcSweepLeft / (M_PI / 2.0f));

  Float sweepDirection = ccw ? -1.0f : 1.0f;

  Float currentStartAngle = startAngle;

  while (arcSweepLeft > 0) {
    
    
    Float currentEndAngle;

    if (arcSweepLeft > M_PI / 2.0f) {
      currentEndAngle = currentStartAngle + M_PI / 2.0f * sweepDirection;
    } else {
      currentEndAngle = currentStartAngle + arcSweepLeft * sweepDirection;
    }

    Point currentStartPoint(x + cos(currentStartAngle) * r,
                              y + sin(currentStartAngle) * r);
    Point currentEndPoint(x + cos(currentEndAngle) * r,
                            y + sin(currentEndAngle) * r);

    
    
    
    Float kappa = (4.0f / 3.0f) * tan((currentEndAngle - currentStartAngle) / 4.0f) * r;

    Point tangentStart(-sin(currentStartAngle), cos(currentStartAngle));
    Point cp1 = currentStartPoint;
    cp1 += tangentStart * kappa;

    Point revTangentEnd(sin(currentEndAngle), -cos(currentEndAngle));
    Point cp2 = currentEndPoint;
    cp2 += revTangentEnd * kappa;

    if (mPathBuilder) {
      mPathBuilder->BezierTo(cp1, cp2, currentEndPoint);
    } else {
      mDSPathBuilder->BezierTo(mTarget->GetTransform() * cp1,
                               mTarget->GetTransform() * cp2,
                               mTarget->GetTransform() * currentEndPoint);
    }

    arcSweepLeft -= M_PI / 2.0f;
    currentStartAngle = currentEndAngle;
  }


  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Rect(float x, float y, float w, float h)
{
  if (!FloatValidate(x, y, w, h)) {
    return NS_OK;
  }

  EnsureWritablePath();

  if (mPathBuilder) {
    mPathBuilder->MoveTo(Point(x, y));
    mPathBuilder->LineTo(Point(x + w, y));
    mPathBuilder->LineTo(Point(x + w, y + h));
    mPathBuilder->LineTo(Point(x, y + h));
    mPathBuilder->Close();
  } else {
    mDSPathBuilder->MoveTo(mTarget->GetTransform() * Point(x, y));
    mDSPathBuilder->LineTo(mTarget->GetTransform() * Point(x + w, y));
    mDSPathBuilder->LineTo(mTarget->GetTransform() * Point(x + w, y + h));
    mDSPathBuilder->LineTo(mTarget->GetTransform() * Point(x, y + h));
    mDSPathBuilder->Close();
  }

  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::EnsureWritablePath()
{
  if (mDSPathBuilder) {
    return;
  }

  FillRule fillRule = CurrentState().fillRule;

  if (mPathBuilder) {
    if (mPathTransformWillUpdate) {
      mPath = mPathBuilder->Finish();
      mDSPathBuilder =
        mPath->TransformedCopyToBuilder(mPathToDS, fillRule);
      mPath = nsnull;
      mPathBuilder = nsnull;
    }
    return;
  }

  if (!mPath) {
    mPathBuilder = mTarget->CreatePathBuilder(fillRule);
  } else if (!mPathTransformWillUpdate) {
    mPathBuilder = mPath->CopyToBuilder(fillRule);
  } else {
    mDSPathBuilder =
      mPath->TransformedCopyToBuilder(mPathToDS, fillRule);
  }
}

void
nsCanvasRenderingContext2DAzure::EnsureUserSpacePath()
{
  FillRule fillRule = CurrentState().fillRule;

  if (!mPath && !mPathBuilder && !mDSPathBuilder) {
    mPathBuilder = mTarget->CreatePathBuilder(fillRule);
  }

  if (mPathBuilder) {
    mPath = mPathBuilder->Finish();
    mPathBuilder = nsnull;
  }

  if (mPath && mPathTransformWillUpdate) {
    mDSPathBuilder =
      mPath->TransformedCopyToBuilder(mPathToDS, fillRule);
    mPath = nsnull;
    mPathTransformWillUpdate = false;
  }

  if (mDSPathBuilder) {
    RefPtr<Path> dsPath;
    dsPath = mDSPathBuilder->Finish();
    mDSPathBuilder = nsnull;

    Matrix inverse = mTarget->GetTransform();
    if (!inverse.Invert()) {
      return;
    }

    mPathBuilder =
      dsPath->TransformedCopyToBuilder(inverse, fillRule);
    mPath = mPathBuilder->Finish();
    mPathBuilder = nsnull;
  }

  if (mPath && mPath->GetFillRule() != fillRule) {
    mPathBuilder = mPath->CopyToBuilder(fillRule);
    mPath = mPathBuilder->Finish();
  }
}

void
nsCanvasRenderingContext2DAzure::TransformWillUpdate()
{
  
  
  if (mPath || mPathBuilder) {
    if (!mPathTransformWillUpdate) {
      
      
      
      
      mPathToDS = mTarget->GetTransform();
    }
    mPathTransformWillUpdate = true;
  }
}












static nsresult
CreateFontStyleRule(const nsAString& aFont,
                    nsINode* aNode,
                    StyleRule** aResult)
{
  nsRefPtr<StyleRule> rule;
  PRBool changed;

  nsIPrincipal* principal = aNode->NodePrincipal();
  nsIDocument* document = aNode->GetOwnerDoc();

  nsIURI* docURL = document->GetDocumentURI();
  nsIURI* baseURL = document->GetDocBaseURI();

  
  
  nsCSSParser parser(document->CSSLoader());

  nsresult rv = parser.ParseStyleAttribute(EmptyString(), docURL, baseURL,
                                            principal, getter_AddRefs(rule));
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = parser.ParseProperty(eCSSProperty_font, aFont, docURL, baseURL,
                            principal, rule->GetDeclaration(), &changed,
                            PR_FALSE);
  if (NS_FAILED(rv))
    return rv;

  rv = parser.ParseProperty(eCSSProperty_line_height,
                            NS_LITERAL_STRING("normal"), docURL, baseURL,
                            principal, rule->GetDeclaration(), &changed,
                            PR_FALSE);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rule->RuleMatched();

  rule.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetFont(const nsAString& font)
{
  nsresult rv;

  







  nsCOMPtr<nsIContent> content = do_QueryInterface(mCanvasElement);
  if (!content && !mDocShell) {
      NS_WARNING("Canvas element must be an nsIContent and non-null or a docshell must be provided");
      return NS_ERROR_FAILURE;
  }

  nsIPresShell* presShell = GetPresShell();
  if (!presShell) {
    return NS_ERROR_FAILURE;
  }
  nsIDocument* document = presShell->GetDocument();

  nsCOMArray<nsIStyleRule> rules;

  nsRefPtr<css::StyleRule> rule;
  rv = CreateFontStyleRule(font, document, getter_AddRefs(rule));

  if (NS_FAILED(rv)) {
    return rv;
  }

  css::Declaration *declaration = rule->GetDeclaration();
  
  
  
  
  
  
  const nsCSSValue *fsaVal =
    declaration->GetNormalBlock()->ValueFor(eCSSProperty_font_size_adjust);
  if (!fsaVal || (fsaVal->GetUnit() != eCSSUnit_None &&
                  fsaVal->GetUnit() != eCSSUnit_System_Font)) {
      
      
    return NS_OK;
  }

  rules.AppendObject(rule);

  nsStyleSet* styleSet = presShell->StyleSet();

  
  
  nsRefPtr<nsStyleContext> parentContext;

  if (content && content->IsInDoc()) {
      
      parentContext = nsComputedDOMStyle::GetStyleContextForElement(
              content->AsElement(),
              nsnull,
              presShell);
  } else {
    
    nsRefPtr<css::StyleRule> parentRule;
    rv = CreateFontStyleRule(NS_LITERAL_STRING("10px sans-serif"),
                              document,
                              getter_AddRefs(parentRule));

    if (NS_FAILED(rv)) {
      return rv;
    }

    nsCOMArray<nsIStyleRule> parentRules;
    parentRules.AppendObject(parentRule);
    parentContext = styleSet->ResolveStyleForRules(nsnull, parentRules);
  }

  if (!parentContext) {
      return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsStyleContext> sc =
      styleSet->ResolveStyleForRules(parentContext, rules);
  if (!sc) {
    return NS_ERROR_FAILURE;
  }

  const nsStyleFont* fontStyle = sc->GetStyleFont();

  NS_ASSERTION(fontStyle, "Could not obtain font style");

  nsIAtom* language = sc->GetStyleVisibility()->mLanguage;
  if (!language) {
    language = presShell->GetPresContext()->GetLanguageFromCharset();
  }

  
  const PRUint32 aupcp = nsPresContext::AppUnitsPerCSSPixel();
  
  const nscoord fontSize = nsStyleFont::UnZoomText(parentContext->PresContext(), fontStyle->mFont.size);

  PRBool printerFont = (presShell->GetPresContext()->Type() == nsPresContext::eContext_PrintPreview ||
                        presShell->GetPresContext()->Type() == nsPresContext::eContext_Print);

  gfxFontStyle style(fontStyle->mFont.style,
                      fontStyle->mFont.weight,
                      fontStyle->mFont.stretch,
                      NSAppUnitsToFloatPixels(fontSize, float(aupcp)),
                      language,
                      fontStyle->mFont.sizeAdjust,
                      fontStyle->mFont.systemFont,
                      printerFont,
                      fontStyle->mFont.featureSettings,
                      fontStyle->mFont.languageOverride);

  CurrentState().fontGroup =
      gfxPlatform::GetPlatform()->CreateFontGroup(fontStyle->mFont.name,
                                                  &style,
                                                  presShell->GetPresContext()->GetUserFontSet());
  NS_ASSERTION(CurrentState().fontGroup, "Could not get font group");

  
  
  
  
  declaration->GetValue(eCSSProperty_font, CurrentState().font);

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetFont(nsAString& font)
{
  
  GetCurrentFontStyle();

  font = CurrentState().font;
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetTextAlign(const nsAString& ta)
{
  if (ta.EqualsLiteral("start"))
    CurrentState().textAlign = TEXT_ALIGN_START;
  else if (ta.EqualsLiteral("end"))
    CurrentState().textAlign = TEXT_ALIGN_END;
  else if (ta.EqualsLiteral("left"))
    CurrentState().textAlign = TEXT_ALIGN_LEFT;
  else if (ta.EqualsLiteral("right"))
    CurrentState().textAlign = TEXT_ALIGN_RIGHT;
  else if (ta.EqualsLiteral("center"))
    CurrentState().textAlign = TEXT_ALIGN_CENTER;
  
  else
    return NS_ERROR_INVALID_ARG;

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetTextAlign(nsAString& ta)
{
  switch (CurrentState().textAlign)
  {
  case TEXT_ALIGN_START:
    ta.AssignLiteral("start");
    break;
  case TEXT_ALIGN_END:
    ta.AssignLiteral("end");
    break;
  case TEXT_ALIGN_LEFT:
    ta.AssignLiteral("left");
    break;
  case TEXT_ALIGN_RIGHT:
    ta.AssignLiteral("right");
    break;
  case TEXT_ALIGN_CENTER:
    ta.AssignLiteral("center");
    break;
  default:
    NS_ERROR("textAlign holds invalid value");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetTextBaseline(const nsAString& tb)
{
  if (tb.EqualsLiteral("top"))
    CurrentState().textBaseline = TEXT_BASELINE_TOP;
  else if (tb.EqualsLiteral("hanging"))
    CurrentState().textBaseline = TEXT_BASELINE_HANGING;
  else if (tb.EqualsLiteral("middle"))
    CurrentState().textBaseline = TEXT_BASELINE_MIDDLE;
  else if (tb.EqualsLiteral("alphabetic"))
    CurrentState().textBaseline = TEXT_BASELINE_ALPHABETIC;
  else if (tb.EqualsLiteral("ideographic"))
    CurrentState().textBaseline = TEXT_BASELINE_IDEOGRAPHIC;
  else if (tb.EqualsLiteral("bottom"))
    CurrentState().textBaseline = TEXT_BASELINE_BOTTOM;
  
  else
    return NS_ERROR_INVALID_ARG;

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetTextBaseline(nsAString& tb)
{
  switch (CurrentState().textBaseline)
  {
  case TEXT_BASELINE_TOP:
    tb.AssignLiteral("top");
    break;
  case TEXT_BASELINE_HANGING:
    tb.AssignLiteral("hanging");
    break;
  case TEXT_BASELINE_MIDDLE:
    tb.AssignLiteral("middle");
    break;
  case TEXT_BASELINE_ALPHABETIC:
    tb.AssignLiteral("alphabetic");
    break;
  case TEXT_BASELINE_IDEOGRAPHIC:
    tb.AssignLiteral("ideographic");
    break;
  case TEXT_BASELINE_BOTTOM:
    tb.AssignLiteral("bottom");
    break;
  default:
    NS_ERROR("textBaseline holds invalid value");
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}








static inline void
TextReplaceWhitespaceCharacters(nsAutoString& str)
{
  str.ReplaceChar("\x09\x0A\x0B\x0C\x0D", PRUnichar(' '));
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::FillText(const nsAString& text, float x, float y, float maxWidth)
{
  return DrawOrMeasureText(text, x, y, maxWidth, TEXT_DRAW_OPERATION_FILL, nsnull);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::StrokeText(const nsAString& text, float x, float y, float maxWidth)
{
  return DrawOrMeasureText(text, x, y, maxWidth, TEXT_DRAW_OPERATION_STROKE, nsnull);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MeasureText(const nsAString& rawText,
                                      nsIDOMTextMetrics** _retval)
{
  float width;

  nsresult rv = DrawOrMeasureText(rawText, 0, 0, 0, TEXT_DRAW_OPERATION_MEASURE, &width);

  if (NS_FAILED(rv)) {
    return rv;
  }

  nsRefPtr<nsIDOMTextMetrics> textMetrics = new nsTextMetricsAzure(width);
  if (!textMetrics.get()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  *_retval = textMetrics.forget().get();

  return NS_OK;
}




struct NS_STACK_CLASS nsCanvasBidiProcessorAzure : public nsBidiPresUtils::BidiProcessor
{
  virtual void SetText(const PRUnichar* text, PRInt32 length, nsBidiDirection direction)
  {
    mTextRun = gfxTextRunCache::MakeTextRun(text,
                                            length,
                                            mFontgrp,
                                            mThebes,
                                            mAppUnitsPerDevPixel,
                                            direction==NSBIDI_RTL ? gfxTextRunFactory::TEXT_IS_RTL : 0);
  }

  virtual nscoord GetWidth()
  {
    gfxTextRun::Metrics textRunMetrics = mTextRun->MeasureText(0,
                                                               mTextRun->GetLength(),
                                                               mDoMeasureBoundingBox ?
                                                                 gfxFont::TIGHT_INK_EXTENTS :
                                                                 gfxFont::LOOSE_INK_EXTENTS,
                                                               mThebes,
                                                               nsnull);

    
    
    if (mDoMeasureBoundingBox) {
      textRunMetrics.mBoundingBox.Scale(1.0 / mAppUnitsPerDevPixel);
      mBoundingBox = mBoundingBox.Union(textRunMetrics.mBoundingBox);
    }

    return static_cast<nscoord>(textRunMetrics.mAdvanceWidth/gfxFloat(mAppUnitsPerDevPixel));
  }

  virtual void DrawText(nscoord xOffset, nscoord width)
  {
    gfxPoint point = mPt;
    point.x += xOffset * mAppUnitsPerDevPixel;

    
    if (mTextRun->IsRightToLeft()) {
      
      
      
      
      
      gfxTextRun::Metrics textRunMetrics =
        mTextRun->MeasureText(0,
                              mTextRun->GetLength(),
                              mDoMeasureBoundingBox ?
                                  gfxFont::TIGHT_INK_EXTENTS :
                                  gfxFont::LOOSE_INK_EXTENTS,
                              mThebes,
                              nsnull);
      point.x += textRunMetrics.mAdvanceWidth;
      
      
      
      
    }

    PRUint32 numRuns;
    const gfxTextRun::GlyphRun *runs = mTextRun->GetGlyphRuns(&numRuns);
    const PRUint32 appUnitsPerDevUnit = mAppUnitsPerDevPixel;
    const double devUnitsPerAppUnit = 1.0/double(appUnitsPerDevUnit);
    Point baselineOrigin =
      Point(point.x * devUnitsPerAppUnit, point.y * devUnitsPerAppUnit);

    for (int c = 0; c < numRuns; c++) {
      gfxFont *font = runs[c].mFont;
      PRUint32 endRun = 0;
      if (c + 1 < numRuns) {
        endRun = runs[c + 1].mCharacterOffset;
      } else {
        endRun = mTextRun->GetLength();
      }

      const gfxTextRun::CompressedGlyph *glyphs = mTextRun->GetCharacterGlyphs();

      RefPtr<ScaledFont> scaledFont =
        gfxPlatform::GetPlatform()->GetScaledFontForFont(font);

      GlyphBuffer buffer;

      std::vector<Glyph> glyphBuf;

      float advanceSum = 0;

      for (int i = runs[c].mCharacterOffset; i < endRun; i++) {
        Glyph newGlyph;
        if (glyphs[i].IsSimpleGlyph()) {
          newGlyph.mIndex = glyphs[i].GetSimpleGlyph();
          if (mTextRun->IsRightToLeft()) {
            newGlyph.mPosition.x = baselineOrigin.x - advanceSum -
              glyphs[i].GetSimpleAdvance() * devUnitsPerAppUnit;
          } else {
            newGlyph.mPosition.x = baselineOrigin.x + advanceSum;
          }
          newGlyph.mPosition.y = baselineOrigin.y;
          advanceSum += glyphs[i].GetSimpleAdvance() * devUnitsPerAppUnit;
          glyphBuf.push_back(newGlyph);
          continue;
        }

        if (!glyphs[i].GetGlyphCount()) {
          continue;
        }

        gfxTextRun::DetailedGlyph *detailedGlyphs =
          mTextRun->GetDetailedGlyphs(i);

        for (int c = 0; c < glyphs[i].GetGlyphCount(); c++) {
          newGlyph.mIndex = detailedGlyphs[c].mGlyphID;
          if (mTextRun->IsRightToLeft()) {
            newGlyph.mPosition.x = baselineOrigin.x + detailedGlyphs[c].mXOffset * devUnitsPerAppUnit -
              advanceSum - detailedGlyphs[c].mAdvance * devUnitsPerAppUnit;
          } else {
            newGlyph.mPosition.x = baselineOrigin.x + detailedGlyphs[c].mXOffset * devUnitsPerAppUnit + advanceSum;
          }
          newGlyph.mPosition.y = baselineOrigin.y + detailedGlyphs[c].mYOffset * devUnitsPerAppUnit;
          glyphBuf.push_back(newGlyph);
          advanceSum += detailedGlyphs[c].mAdvance * devUnitsPerAppUnit;
        }
      }

      if (!glyphBuf.size()) {
        
        continue;
      }

      buffer.mGlyphs = &glyphBuf.front();
      buffer.mNumGlyphs = glyphBuf.size();

      if (mOp == nsCanvasRenderingContext2DAzure::TEXT_DRAW_OPERATION_FILL) {
        nsCanvasRenderingContext2DAzure::AdjustedTarget(mCtx)->
          FillGlyphs(scaledFont, buffer,
                      nsCanvasRenderingContext2DAzure::GeneralPattern().
                        ForStyle(mCtx, nsCanvasRenderingContext2DAzure::STYLE_FILL, mCtx->mTarget),
                      DrawOptions(mState->globalAlpha, mState->op));
      } else if (mOp == nsCanvasRenderingContext2DAzure::TEXT_DRAW_OPERATION_STROKE) {
        RefPtr<Path> path = scaledFont->GetPathForGlyphs(buffer, mCtx->mTarget);
            
        Matrix oldTransform = mCtx->mTarget->GetTransform();

        nsCanvasRenderingContext2DAzure::AdjustedTarget(mCtx)->
          Stroke(path, nsCanvasRenderingContext2DAzure::GeneralPattern().
                    ForStyle(mCtx, nsCanvasRenderingContext2DAzure::STYLE_STROKE, mCtx->mTarget),
                  StrokeOptions(mCtx->CurrentState().lineWidth, mCtx->CurrentState().lineJoin,
                                mCtx->CurrentState().lineCap, mCtx->CurrentState().miterLimit),
                  DrawOptions(mState->globalAlpha, mState->op));

      }
    }
  }

  
  gfxTextRunCache::AutoTextRun mTextRun;

  
  nsRefPtr<gfxContext> mThebes;

  
  nsCanvasRenderingContext2DAzure *mCtx;

  
  gfxPoint mPt;

  
  gfxFontGroup* mFontgrp;

  
  PRUint32 mAppUnitsPerDevPixel;

  
  nsCanvasRenderingContext2DAzure::TextDrawOperation mOp;

  
  nsCanvasRenderingContext2DAzure::ContextState *mState;

  
  gfxRect mBoundingBox;

  
  PRBool mDoMeasureBoundingBox;
};

nsresult
nsCanvasRenderingContext2DAzure::DrawOrMeasureText(const nsAString& aRawText,
                                                   float aX,
                                                   float aY,
                                                   float aMaxWidth,
                                                   TextDrawOperation aOp,
                                                   float* aWidth)
{
  nsresult rv;

  if (!FloatValidate(aX, aY, aMaxWidth))
      return NS_ERROR_DOM_SYNTAX_ERR;

  
  
  
  
  if (aMaxWidth < 0)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIContent> content = do_QueryInterface(mCanvasElement);
  if (!content && !mDocShell) {
      NS_WARNING("Canvas element must be an nsIContent and non-null or a docshell must be provided");
    return NS_ERROR_FAILURE;
  }

  nsIPresShell* presShell = GetPresShell();
  if (!presShell)
      return NS_ERROR_FAILURE;

  nsIDocument* document = presShell->GetDocument();

  nsBidiPresUtils* bidiUtils = presShell->GetPresContext()->GetBidiUtils();
  if (!bidiUtils) {
    return NS_ERROR_FAILURE;
  }

  
  nsAutoString textToDraw(aRawText);
  TextReplaceWhitespaceCharacters(textToDraw);

  
  PRBool isRTL = PR_FALSE;

  if (content && content->IsInDoc()) {
    
    nsRefPtr<nsStyleContext> canvasStyle =
      nsComputedDOMStyle::GetStyleContextForElement(content->AsElement(),
                                                    nsnull,
                                                    presShell);
    if (!canvasStyle) {
      return NS_ERROR_FAILURE;
    }

    isRTL = canvasStyle->GetStyleVisibility()->mDirection ==
      NS_STYLE_DIRECTION_RTL;
  } else {
    isRTL = GET_BIDI_OPTION_DIRECTION(document->GetBidiOptions()) == IBMBIDI_TEXTDIRECTION_RTL;
  }

  const ContextState &state = CurrentState();

  
  PRBool doDrawShadow = aOp == TEXT_DRAW_OPERATION_FILL && NeedToDrawShadow();

  nsCanvasBidiProcessorAzure processor;

  GetAppUnitsValues(&processor.mAppUnitsPerDevPixel, nsnull);
  processor.mPt = gfxPoint(aX, aY);
  processor.mThebes =
    new gfxContext(gfxPlatform::GetPlatform()->ScreenReferenceSurface());
  Matrix matrix = mTarget->GetTransform();
  processor.mThebes->SetMatrix(gfxMatrix(matrix._11, matrix._12, matrix._21, matrix._22, matrix._31, matrix._32));
  processor.mCtx = this;
  processor.mOp = aOp;
  processor.mBoundingBox = gfxRect(0, 0, 0, 0);
  processor.mDoMeasureBoundingBox = doDrawShadow || !mIsEntireFrameInvalid;
  processor.mState = &CurrentState();
    

  processor.mFontgrp = GetCurrentFontStyle();
  NS_ASSERTION(processor.mFontgrp, "font group is null");

  nscoord totalWidth;

  
  
  rv = bidiUtils->ProcessText(textToDraw.get(),
                              textToDraw.Length(),
                              isRTL ? NSBIDI_RTL : NSBIDI_LTR,
                              presShell->GetPresContext(),
                              processor,
                              nsBidiPresUtils::MODE_MEASURE,
                              nsnull,
                              0,
                              &totalWidth);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (aWidth) {
    *aWidth = static_cast<float>(totalWidth);
  }

  
  if (aOp==TEXT_DRAW_OPERATION_MEASURE) {
    return NS_OK;
  }

  
  gfxFloat anchorX;

  if (state.textAlign == TEXT_ALIGN_CENTER) {
    anchorX = .5;
  } else if (state.textAlign == TEXT_ALIGN_LEFT ||
            (!isRTL && state.textAlign == TEXT_ALIGN_START) ||
            (isRTL && state.textAlign == TEXT_ALIGN_END)) {
    anchorX = 0;
  } else {
    anchorX = 1;
  }

  processor.mPt.x -= anchorX * totalWidth;

  
  NS_ASSERTION(processor.mFontgrp->FontListLength()>0, "font group contains no fonts");
  const gfxFont::Metrics& fontMetrics = processor.mFontgrp->GetFontAt(0)->GetMetrics();

  gfxFloat anchorY;

  switch (state.textBaseline)
  {
  case TEXT_BASELINE_HANGING:
      
  case TEXT_BASELINE_TOP:
    anchorY = fontMetrics.emAscent;
    break;
    break;
  case TEXT_BASELINE_MIDDLE:
    anchorY = (fontMetrics.emAscent - fontMetrics.emDescent) * .5f;
    break;
  case TEXT_BASELINE_IDEOGRAPHIC:
    
  case TEXT_BASELINE_ALPHABETIC:
    anchorY = 0;
    break;
  case TEXT_BASELINE_BOTTOM:
    anchorY = -fontMetrics.emDescent;
    break;
  default:
    NS_ERROR("mTextBaseline holds invalid value");
    return NS_ERROR_FAILURE;
  }

  processor.mPt.y += anchorY;

  
  processor.mBoundingBox.width = totalWidth;
  processor.mBoundingBox.MoveBy(processor.mPt);

  processor.mPt.x *= processor.mAppUnitsPerDevPixel;
  processor.mPt.y *= processor.mAppUnitsPerDevPixel;

  Matrix oldTransform = mTarget->GetTransform();
  
  
  if (aMaxWidth > 0 && totalWidth > aMaxWidth) {
    Matrix newTransform = oldTransform;

    
    
    newTransform.Translate(aX, 0);
    newTransform.Scale(aMaxWidth / totalWidth, 1);
    newTransform.Translate(-aX, 0);
    
    Matrix androidCompilerBug = newTransform;
    mTarget->SetTransform(androidCompilerBug);
  }

  
  gfxRect boundingBox = processor.mBoundingBox;

  
  processor.mDoMeasureBoundingBox = PR_FALSE;

  rv = bidiUtils->ProcessText(textToDraw.get(),
                              textToDraw.Length(),
                              isRTL ? NSBIDI_RTL : NSBIDI_LTR,
                              presShell->GetPresContext(),
                              processor,
                              nsBidiPresUtils::MODE_DRAW,
                              nsnull,
                              0,
                              nsnull);


  mTarget->SetTransform(oldTransform);

  if (aOp == nsCanvasRenderingContext2DAzure::TEXT_DRAW_OPERATION_FILL && !doDrawShadow)
    return RedrawUser(boundingBox);

  return Redraw();
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozTextStyle(const nsAString& textStyle)
{
    
    return SetFont(textStyle);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozTextStyle(nsAString& textStyle)
{
    
    return GetFont(textStyle);
}

gfxFontGroup *nsCanvasRenderingContext2DAzure::GetCurrentFontStyle()
{
  
  if (!CurrentState().fontGroup) {
    nsresult rv = SetFont(kDefaultFontStyle);
    if (NS_FAILED(rv)) {
      gfxFontStyle style;
      style.size = kDefaultFontSize;
      CurrentState().fontGroup =
        gfxPlatform::GetPlatform()->CreateFontGroup(kDefaultFontName,
                                                    &style,
                                                    nsnull);
      if (CurrentState().fontGroup) {
        CurrentState().font = kDefaultFontStyle;
        rv = NS_OK;
      } else {
        rv = NS_ERROR_OUT_OF_MEMORY;
      }
    }
            
    NS_ASSERTION(NS_SUCCEEDED(rv), "Default canvas font is invalid");
  }

  return CurrentState().fontGroup;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MozDrawText(const nsAString& textToDraw)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MozMeasureText(const nsAString& textToMeasure, float *retVal)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MozPathText(const nsAString& textToPath)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MozTextAlongPath(const nsAString& textToDraw, PRBool stroke)
{
  return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
}




NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetLineWidth(float width)
{
  if (!FloatValidate(width) || width <= 0.0) {
    return NS_OK;
  }

  CurrentState().lineWidth = width;
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetLineWidth(float *width)
{
  *width = CurrentState().lineWidth;
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetLineCap(const nsAString& capstyle)
{
  CapStyle cap;

  if (capstyle.EqualsLiteral("butt")) {
    cap = CAP_BUTT;
  } else if (capstyle.EqualsLiteral("round")) {
    cap = CAP_ROUND;
  } else if (capstyle.EqualsLiteral("square")) {
    cap = CAP_SQUARE;
  } else {
    
    return NS_OK;
  }

  CurrentState().lineCap = cap;

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetLineCap(nsAString& capstyle)
{
  switch (CurrentState().lineCap) {
  case CAP_BUTT:
    capstyle.AssignLiteral("butt");
    break;
  case CAP_ROUND:
    capstyle.AssignLiteral("round");
    break;
  case CAP_SQUARE:
    capstyle.AssignLiteral("square");
    break;
  default:
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetLineJoin(const nsAString& joinstyle)
{
  JoinStyle j;

  if (joinstyle.EqualsLiteral("round")) {
    j = JOIN_ROUND;
  } else if (joinstyle.EqualsLiteral("bevel")) {
    j = JOIN_BEVEL;
  } else if (joinstyle.EqualsLiteral("miter")) {
    j = JOIN_MITER_OR_BEVEL;
  } else {
    
    return NS_OK;
  }

  CurrentState().lineJoin = j;
   
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetLineJoin(nsAString& joinstyle)
{
  switch (CurrentState().lineJoin) {
  case JOIN_ROUND:
    joinstyle.AssignLiteral("round");
    break;
  case JOIN_BEVEL:
    joinstyle.AssignLiteral("bevel");
    break;
  case JOIN_MITER_OR_BEVEL:
    joinstyle.AssignLiteral("miter");
    break;
  default:
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMiterLimit(float miter)
{
  if (!FloatValidate(miter) || miter <= 0.0)
    return NS_OK;

  CurrentState().miterLimit = miter;

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMiterLimit(float *miter)
{
  *miter = CurrentState().miterLimit;
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::IsPointInPath(float x, float y, PRBool *retVal)
{
  if (!FloatValidate(x,y)) {
    *retVal = PR_FALSE;
    return NS_OK;
  }

  EnsureUserSpacePath();

  *retVal = PR_FALSE;

  if (mPath) {
    *retVal = mPath->ContainsPoint(Point(x, y), mTarget->GetTransform());
  }

  return NS_OK;
}












NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::DrawImage(nsIDOMElement *imgElt, float a1,
                                           float a2, float a3, float a4, float a5,
                                           float a6, float a7, float a8,
                                           PRUint8 optional_argc)
{
  if (!imgElt) {
    return NS_ERROR_DOM_TYPE_MISMATCH_ERR;
  }

  if (optional_argc == 0) {
    if (!FloatValidate(a1, a2)) {
      return NS_OK;
    }
  } else if (optional_argc == 2) {
    if (!FloatValidate(a1, a2, a3, a4)) {
      return NS_OK;
    }
  } else if (optional_argc == 6) {
    if (!FloatValidate(a1, a2, a3, a4, a5, a6) || !FloatValidate(a7, a8)) {
      return NS_OK;
    }
  }

  double sx,sy,sw,sh;
  double dx,dy,dw,dh;

  nsCOMPtr<nsIContent> content = do_QueryInterface(imgElt);
  nsHTMLCanvasElement* canvas = nsHTMLCanvasElement::FromContent(content);
  if (canvas) {
    nsIntSize size = canvas->GetSize();
    if (size.width == 0 || size.height == 0) {
      return NS_ERROR_DOM_INVALID_STATE_ERR;
    }
  }
    
  RefPtr<SourceSurface> srcSurf;

  gfxIntSize imgSize;
  nsRefPtr<gfxASurface> imgsurf =
    CanvasImageCache::Lookup(imgElt, HTMLCanvasElement(), &imgSize);

  if (imgsurf) {
    srcSurf = gfxPlatform::GetPlatform()->GetSourceSurfaceForSurface(mTarget, imgsurf);
  }

  
  if (canvas == HTMLCanvasElement()) {
    
    srcSurf = mTarget->Snapshot();
    imgSize = gfxIntSize(mWidth, mHeight);
  }

  if (!srcSurf) {
    
    
    PRUint32 sfeFlags = nsLayoutUtils::SFE_WANT_FIRST_FRAME;
    nsLayoutUtils::SurfaceFromElementResult res =
      nsLayoutUtils::SurfaceFromElement(imgElt, sfeFlags);

    if (!res.mSurface) {
      
      return res.mIsStillLoading ? NS_OK : NS_ERROR_NOT_AVAILABLE;
    }

    
    if (!res.mSurface->CairoSurface()) {
      return NS_OK;
    }

    imgsurf = res.mSurface.forget();
    imgSize = res.mSize;

    if (mCanvasElement) {
      CanvasUtils::DoDrawImageSecurityCheck(HTMLCanvasElement(),
                                            res.mPrincipal, res.mIsWriteOnly);
    }

    if (res.mImageRequest) {
      CanvasImageCache::NotifyDrawImage(imgElt, HTMLCanvasElement(),
                                        res.mImageRequest, imgsurf, imgSize);
    }

    srcSurf = gfxPlatform::GetPlatform()->GetSourceSurfaceForSurface(mTarget, imgsurf);
  }

  if (optional_argc == 0) {
    dx = a1;
    dy = a2;
    sx = sy = 0.0;
    dw = sw = (double) imgSize.width;
    dh = sh = (double) imgSize.height;
  } else if (optional_argc == 2) {
    dx = a1;
    dy = a2;
    dw = a3;
    dh = a4;
    sx = sy = 0.0;
    sw = (double) imgSize.width;
    sh = (double) imgSize.height;
  } else if (optional_argc == 6) {
    sx = a1;
    sy = a2;
    sw = a3;
    sh = a4;
    dx = a5;
    dy = a6;
    dw = a7;
    dh = a8;
  } else {
    
    return NS_ERROR_INVALID_ARG;
  }

  if (sw == 0.0 || sh == 0.0) {
    
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  if (dw == 0.0 || dh == 0.0) {
    
    
    return NS_OK;
  }

  
  
  if (sw < 0.0) {
    sx += sw;
    sw = -sw;
  }
  if (sh < 0.0) {
    sy += sh;
    sh = -sh;
  }
  if (dw < 0.0) {
    dx += dw;
    dw = -dw;
  }
  if (dh < 0.0) {
    dy += dh;
    dh = -dh;
  }

  
  if (sx < 0 || sx + sw > (double) imgSize.width || 
      sy < 0 || sy + sh > (double) imgSize.height) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  Filter filter;

  if (CurrentState().imageSmoothingEnabled)
    filter = mgfx::FILTER_LINEAR;
  else
    filter = mgfx::FILTER_POINT;

  AdjustedTarget(this)->
    DrawSurface(srcSurf,
                mgfx::Rect(dx, dy, dw, dh),
                mgfx::Rect(sx, sy, sw, sh),
                DrawSurfaceOptions(filter),
                DrawOptions(CurrentState().globalAlpha, CurrentState().op));

  return RedrawUser(gfxRect(dx, dy, dw, dh));
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetGlobalCompositeOperation(const nsAString& op)
{
  CompositionOp comp_op;

#define CANVAS_OP_TO_GFX_OP(cvsop, op2d) \
  if (op.EqualsLiteral(cvsop))   \
    comp_op = OP_##op2d;

  CANVAS_OP_TO_GFX_OP("copy", SOURCE)
  else CANVAS_OP_TO_GFX_OP("source-atop", ATOP)
  else CANVAS_OP_TO_GFX_OP("source-in", IN)
  else CANVAS_OP_TO_GFX_OP("source-out", OUT)
  else CANVAS_OP_TO_GFX_OP("source-over", OVER)
  else CANVAS_OP_TO_GFX_OP("destination-in", DEST_IN)
  else CANVAS_OP_TO_GFX_OP("destination-out", DEST_OUT)
  else CANVAS_OP_TO_GFX_OP("destination-over", DEST_OVER)
  else CANVAS_OP_TO_GFX_OP("destination-atop", DEST_ATOP)
  else CANVAS_OP_TO_GFX_OP("lighter", ADD)
  else CANVAS_OP_TO_GFX_OP("xor", XOR)
  
  else return NS_OK;

#undef CANVAS_OP_TO_GFX_OP
  CurrentState().op = comp_op;
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetGlobalCompositeOperation(nsAString& op)
{
  CompositionOp comp_op = CurrentState().op;

#define CANVAS_OP_TO_GFX_OP(cvsop, op2d) \
  if (comp_op == OP_##op2d) \
    op.AssignLiteral(cvsop);

  CANVAS_OP_TO_GFX_OP("copy", SOURCE)
  else CANVAS_OP_TO_GFX_OP("destination-atop", DEST_ATOP)
  else CANVAS_OP_TO_GFX_OP("destination-in", DEST_IN)
  else CANVAS_OP_TO_GFX_OP("destination-out", DEST_OUT)
  else CANVAS_OP_TO_GFX_OP("destination-over", DEST_OVER)
  else CANVAS_OP_TO_GFX_OP("lighter", ADD)
  else CANVAS_OP_TO_GFX_OP("source-atop", ATOP)
  else CANVAS_OP_TO_GFX_OP("source-in", IN)
  else CANVAS_OP_TO_GFX_OP("source-out", OUT)
  else CANVAS_OP_TO_GFX_OP("source-over", OVER)
  else CANVAS_OP_TO_GFX_OP("xor", XOR)
  else return NS_ERROR_FAILURE;

#undef CANVAS_OP_TO_GFX_OP
    
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::DrawWindow(nsIDOMWindow* aWindow, float aX, float aY,
                                            float aW, float aH,
                                            const nsAString& aBGColor,
                                            PRUint32 flags)
{
  NS_ENSURE_ARG(aWindow != nsnull);

  
  
  if (!gfxASurface::CheckSurfaceSize(gfxIntSize(PRInt32(aW), PRInt32(aH)),
                                      0xffff))
    return NS_ERROR_FAILURE;

  nsRefPtr<gfxASurface> drawSurf;
  GetThebesSurface(getter_AddRefs(drawSurf));

  nsRefPtr<gfxContext> thebes = new gfxContext(drawSurf);

  Matrix matrix = mTarget->GetTransform();
  thebes->SetMatrix(gfxMatrix(matrix._11, matrix._12, matrix._21,
                              matrix._22, matrix._31, matrix._32));

  
  
  
  
  
  
  if (!nsContentUtils::IsCallerTrustedForRead()) {
    
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  
  if (!(flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DO_NOT_FLUSH))
      nsContentUtils::FlushLayoutForTree(aWindow);

  nsRefPtr<nsPresContext> presContext;
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aWindow);
  if (win) {
    nsIDocShell* docshell = win->GetDocShell();
    if (docshell) {
      docshell->GetPresContext(getter_AddRefs(presContext));
    }
  }
  if (!presContext)
    return NS_ERROR_FAILURE;

  nscolor bgColor;

  nsIDocument* elementDoc = mCanvasElement ?
                            HTMLCanvasElement()->GetOwnerDoc() : nsnull;

  
  
  nsCSSParser parser(elementDoc ? elementDoc->CSSLoader() : nsnull);
  nsresult rv = parser.ParseColorString(PromiseFlatString(aBGColor),
                                        nsnull, 0, &bgColor);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIPresShell* presShell = presContext->PresShell();
  NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

  nsRect r(nsPresContext::CSSPixelsToAppUnits(aX),
           nsPresContext::CSSPixelsToAppUnits(aY),
           nsPresContext::CSSPixelsToAppUnits(aW),
           nsPresContext::CSSPixelsToAppUnits(aH));
  PRUint32 renderDocFlags = (nsIPresShell::RENDER_IGNORE_VIEWPORT_SCROLLING |
                             nsIPresShell::RENDER_DOCUMENT_RELATIVE);
  if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DRAW_CARET) {
    renderDocFlags |= nsIPresShell::RENDER_CARET;
  }
  if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DRAW_VIEW) {
    renderDocFlags &= ~(nsIPresShell::RENDER_IGNORE_VIEWPORT_SCROLLING |
                        nsIPresShell::RENDER_DOCUMENT_RELATIVE);
  }
  if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_USE_WIDGET_LAYERS) {
    renderDocFlags |= nsIPresShell::RENDER_USE_WIDGET_LAYERS;
  }
  if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_ASYNC_DECODE_IMAGES) {
    renderDocFlags |= nsIPresShell::RENDER_ASYNC_DECODE_IMAGES;
  }

  rv = presShell->RenderDocument(r, renderDocFlags, bgColor, thebes);

  
  
  
  RedrawUser(gfxRect(0, 0, aW, aH));

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::AsyncDrawXULElement(nsIDOMXULElement* aElem, float aX, float aY,
                                                float aW, float aH,
                                                const nsAString& aBGColor,
                                                PRUint32 flags)
{
#if 0
    NS_ENSURE_ARG(aElem != nsnull);

    
    
    
    
    
    
    if (!nsContentUtils::IsCallerTrustedForRead()) {
        
        
        return NS_ERROR_DOM_SECURITY_ERR;
    }

    nsCOMPtr<nsIFrameLoaderOwner> loaderOwner = do_QueryInterface(aElem);
    if (!loaderOwner)
        return NS_ERROR_FAILURE;

    nsRefPtr<nsFrameLoader> frameloader = loaderOwner->GetFrameLoader();
    if (!frameloader)
        return NS_ERROR_FAILURE;

    PBrowserParent *child = frameloader->GetRemoteBrowser();
    if (!child) {
        nsCOMPtr<nsIDOMWindow> window =
            do_GetInterface(frameloader->GetExistingDocShell());
        if (!window)
            return NS_ERROR_FAILURE;

        return DrawWindow(window, aX, aY, aW, aH, aBGColor, flags);
    }

    
    
    if (!gfxASurface::CheckSurfaceSize(gfxIntSize(aW, aH), 0xffff))
        return NS_ERROR_FAILURE;

    PRBool flush =
        (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DO_NOT_FLUSH) == 0;

    PRUint32 renderDocFlags = nsIPresShell::RENDER_IGNORE_VIEWPORT_SCROLLING;
    if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DRAW_CARET) {
        renderDocFlags |= nsIPresShell::RENDER_CARET;
    }
    if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DRAW_VIEW) {
        renderDocFlags &= ~nsIPresShell::RENDER_IGNORE_VIEWPORT_SCROLLING;
    }

    nsRect rect(nsPresContext::CSSPixelsToAppUnits(aX),
                nsPresContext::CSSPixelsToAppUnits(aY),
                nsPresContext::CSSPixelsToAppUnits(aW),
                nsPresContext::CSSPixelsToAppUnits(aH));
    if (mIPC) {
        PDocumentRendererParent *pdocrender =
            child->SendPDocumentRendererConstructor(rect,
                                                    mThebes->CurrentMatrix(),
                                                    nsString(aBGColor),
                                                    renderDocFlags, flush,
                                                    nsIntSize(mWidth, mHeight));
        if (!pdocrender)
            return NS_ERROR_FAILURE;

        DocumentRendererParent *docrender =
            static_cast<DocumentRendererParent *>(pdocrender);

        docrender->SetCanvasContext(this, mThebes);
    }
#endif
    return NS_OK;
}





void
nsCanvasRenderingContext2DAzure::EnsureUnpremultiplyTable() {
  if (sUnpremultiplyTable)
    return;

  
  sUnpremultiplyTable = new PRUint8[256][256];

  
  
  
  
  

  
  for (PRUint32 c = 0; c <= 255; c++) {
    sUnpremultiplyTable[0][c] = c;
  }

  for (int a = 1; a <= 255; a++) {
    for (int c = 0; c <= 255; c++) {
      sUnpremultiplyTable[a][c] = (PRUint8)((c * 255) / a);
    }
  }
}


NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetImageData()
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetImageData_explicit(PRInt32 x, PRInt32 y, PRUint32 w, PRUint32 h,
                                                       PRUint8 *aData, PRUint32 aDataLen)
{
  if (!mValid)
    return NS_ERROR_FAILURE;

  if (!mCanvasElement && !mDocShell) {
    NS_ERROR("No canvas element and no docshell in GetImageData!!!");
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  
  
  if (mCanvasElement &&
      HTMLCanvasElement()->IsWriteOnly() &&
      !nsContentUtils::IsCallerTrustedForRead())
  {
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (w == 0 || h == 0 || aDataLen != w * h * 4)
    return NS_ERROR_DOM_SYNTAX_ERR;

  CheckedInt32 rightMost = CheckedInt32(x) + w;
  CheckedInt32 bottomMost = CheckedInt32(y) + h;

  if (!rightMost.valid() || !bottomMost.valid())
    return NS_ERROR_DOM_SYNTAX_ERR;

  if (mZero) {
    return NS_OK;
  }

  IntRect srcRect(0, 0, mWidth, mHeight);
  IntRect destRect(x, y, w, h);

  if (!srcRect.Contains(destRect)) {
    
    memset(aData, 0, aDataLen);
  }

  bool finishedPainting = false;

  IntRect srcReadRect = srcRect.Intersect(destRect);
  IntRect dstWriteRect = srcReadRect;
  dstWriteRect.MoveBy(-x, -y);

  PRUint8 *src = aData;
  PRUint32 srcStride = w * 4;
    
  if (!srcReadRect.IsEmpty()) {
    RefPtr<SourceSurface> snapshot = mTarget->Snapshot();

    RefPtr<DataSourceSurface> readback = snapshot->GetDataSurface();

    srcStride = readback->Stride();
    src = readback->GetData() + srcReadRect.y * srcStride + srcReadRect.x * 4;
  }

  
  EnsureUnpremultiplyTable();

  
  
  PRUint8 *dst = aData + dstWriteRect.y * (w * 4) + dstWriteRect.x * 4;

  for (PRUint32 j = 0; j < dstWriteRect.height; j++) {
    for (PRUint32 i = 0; i < dstWriteRect.width; i++) {
      
#ifdef IS_LITTLE_ENDIAN
      PRUint8 b = *src++;
      PRUint8 g = *src++;
      PRUint8 r = *src++;
      PRUint8 a = *src++;
#else
      PRUint8 a = *src++;
      PRUint8 r = *src++;
      PRUint8 g = *src++;
      PRUint8 b = *src++;
#endif
      
      *dst++ = sUnpremultiplyTable[a][r];
      *dst++ = sUnpremultiplyTable[a][g];
      *dst++ = sUnpremultiplyTable[a][b];
      *dst++ = a;
    }
    src += srcStride - (dstWriteRect.width * 4);
    dst += (w * 4) - (dstWriteRect.width * 4);
  }
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::EnsurePremultiplyTable() {
  if (sPremultiplyTable)
    return;

  
  sPremultiplyTable = new PRUint8[256][256];

  
  
  

  for (int a = 0; a <= 255; a++) {
    for (int c = 0; c <= 255; c++) {
      sPremultiplyTable[a][c] = (a * c + 254) / 255;
    }
  }
}

void
nsCanvasRenderingContext2DAzure::FillRuleChanged()
{
  if (mPath) {
    mPathBuilder = mPath->CopyToBuilder(CurrentState().fillRule);
    mPath = nsnull;
  }
}


NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::PutImageData()
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::PutImageData_explicit(PRInt32 x, PRInt32 y, PRUint32 w, PRUint32 h,
                                                       unsigned char *aData, PRUint32 aDataLen,
                                                       PRBool hasDirtyRect, PRInt32 dirtyX, PRInt32 dirtyY,
                                                       PRInt32 dirtyWidth, PRInt32 dirtyHeight)
{
  if (!mValid) {
    return NS_ERROR_FAILURE;
  }

  if (w == 0 || h == 0) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  IntRect dirtyRect;
  IntRect imageDataRect(0, 0, w, h);

  if (hasDirtyRect) {
    
    if (dirtyWidth < 0) {
      NS_ENSURE_TRUE(dirtyWidth != INT_MIN, NS_ERROR_DOM_INDEX_SIZE_ERR);

      CheckedInt32 checkedDirtyX = CheckedInt32(dirtyX) + dirtyWidth;

      if (!checkedDirtyX.valid())
          return NS_ERROR_DOM_INDEX_SIZE_ERR;

      dirtyX = checkedDirtyX.value();
      dirtyWidth = -(int32)dirtyWidth;
    }

    if (dirtyHeight < 0) {
      NS_ENSURE_TRUE(dirtyHeight != INT_MIN, NS_ERROR_DOM_INDEX_SIZE_ERR);

      CheckedInt32 checkedDirtyY = CheckedInt32(dirtyY) + dirtyHeight;

      if (!checkedDirtyY.valid())
          return NS_ERROR_DOM_INDEX_SIZE_ERR;

      dirtyY = checkedDirtyY.value();
      dirtyHeight = -(int32)dirtyHeight;
    }

    
    dirtyRect = imageDataRect.Intersect(IntRect(dirtyX, dirtyY, dirtyWidth, dirtyHeight));

    if (dirtyRect.Width() <= 0 || dirtyRect.Height() <= 0)
        return NS_OK;
  } else {
    dirtyRect = imageDataRect;
  }

  dirtyRect.MoveBy(IntPoint(x, y));
  dirtyRect = IntRect(0, 0, mWidth, mHeight).Intersect(dirtyRect);

  if (dirtyRect.Width() <= 0 || dirtyRect.Height() <= 0) {
    return NS_OK;
  }

  PRUint32 len = w * h * 4;
  if (aDataLen != len) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  nsRefPtr<gfxImageSurface> imgsurf = new gfxImageSurface(gfxIntSize(w, h),
                                                          gfxASurface::ImageFormatARGB32);
  if (!imgsurf || imgsurf->CairoStatus()) {
    return NS_ERROR_FAILURE;
  }

  
  EnsurePremultiplyTable();

  PRUint8 *src = aData;
  PRUint8 *dst = imgsurf->Data();

  for (PRUint32 j = 0; j < h; j++) {
    for (PRUint32 i = 0; i < w; i++) {
      PRUint8 r = *src++;
      PRUint8 g = *src++;
      PRUint8 b = *src++;
      PRUint8 a = *src++;

      
#ifdef IS_LITTLE_ENDIAN
      *dst++ = sPremultiplyTable[a][b];
      *dst++ = sPremultiplyTable[a][g];
      *dst++ = sPremultiplyTable[a][r];
      *dst++ = a;
#else
      *dst++ = a;
      *dst++ = sPremultiplyTable[a][r];
      *dst++ = sPremultiplyTable[a][g];
      *dst++ = sPremultiplyTable[a][b];
#endif
    }
  }

  RefPtr<SourceSurface> sourceSurface =
    mTarget->CreateSourceSurfaceFromData(imgsurf->Data(), IntSize(w, h), imgsurf->Stride(), FORMAT_B8G8R8A8);


  mTarget->CopySurface(sourceSurface,
                       IntRect(dirtyRect.x - x, dirtyRect.y - y,
                               dirtyRect.width, dirtyRect.height),
                       IntPoint(dirtyRect.x, dirtyRect.y));

  Redraw(mgfx::Rect(dirtyRect.x, dirtyRect.y, dirtyRect.width, dirtyRect.height));

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetThebesSurface(gfxASurface **surface)
{
  if (!mTarget) {
    nsRefPtr<gfxASurface> tmpSurf =
      gfxPlatform::GetPlatform()->CreateOffscreenSurface(gfxIntSize(1, 1), gfxASurface::CONTENT_COLOR_ALPHA);
    *surface = tmpSurf.forget().get();
    return NS_OK;
  }
    
  nsRefPtr<gfxASurface> newSurf =
    gfxPlatform::GetPlatform()->GetThebesSurfaceForDrawTarget(mTarget);    

  *surface = newSurf.forget().get();

  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::CreateImageData()
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozImageSmoothingEnabled(PRBool *retVal)
{
  *retVal = CurrentState().imageSmoothingEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozImageSmoothingEnabled(PRBool val)
{
  if (val != CurrentState().imageSmoothingEnabled) {
      CurrentState().imageSmoothingEnabled = val;
  }

  return NS_OK;
}

static PRUint8 g2DContextLayerUserData;

class CanvasRenderingContext2DUserData : public LayerUserData {
public:
  CanvasRenderingContext2DUserData(nsHTMLCanvasElement *aContent)
    : mContent(aContent) {}
  static void DidTransactionCallback(void* aData)
  {
    static_cast<CanvasRenderingContext2DUserData*>(aData)->mContent->MarkContextClean();
  }

private:
  nsRefPtr<nsHTMLCanvasElement> mContent;
};

already_AddRefed<CanvasLayer>
nsCanvasRenderingContext2DAzure::GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                           CanvasLayer *aOldLayer,
                                           LayerManager *aManager)
{
  if (!mValid) {
    return nsnull;
  }

  if (mTarget) {
    mTarget->Flush();
  }

  if (!mResetLayer && aOldLayer &&
      aOldLayer->HasUserData(&g2DContextLayerUserData)) {
      NS_ADDREF(aOldLayer);
      return aOldLayer;
  }

  nsRefPtr<CanvasLayer> canvasLayer = aManager->CreateCanvasLayer();
  if (!canvasLayer) {
      NS_WARNING("CreateCanvasLayer returned null!");
      return nsnull;
  }
  CanvasRenderingContext2DUserData *userData = nsnull;
  if (aBuilder->IsPaintingToWindow()) {
    
    
    
    
    
    

    
    
    
    
    
    userData = new CanvasRenderingContext2DUserData(HTMLCanvasElement());
    canvasLayer->SetDidTransactionCallback(
            CanvasRenderingContext2DUserData::DidTransactionCallback, userData);
  }
  canvasLayer->SetUserData(&g2DContextLayerUserData, userData);

  CanvasLayer::Data data;

  data.mDrawTarget = mTarget;
  data.mSize = nsIntSize(mWidth, mHeight);

  canvasLayer->Initialize(data);
  PRUint32 flags = mOpaque ? Layer::CONTENT_OPAQUE : 0;
  canvasLayer->SetContentFlags(flags);
  canvasLayer->Updated();

  mResetLayer = PR_FALSE;

  return canvasLayer.forget();
}

void
nsCanvasRenderingContext2DAzure::MarkContextClean()
{
  if (mInvalidateCount > 0) {
    mPredictManyRedrawCalls = mInvalidateCount > kCanvasMaxInvalidateCount;
  }
  mIsEntireFrameInvalid = PR_FALSE;
  mInvalidateCount = 0;
}

