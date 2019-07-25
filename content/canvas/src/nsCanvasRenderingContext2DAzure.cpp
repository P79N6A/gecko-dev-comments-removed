




#include "base/basictypes.h"
#include "nsCanvasRenderingContext2DAzure.h"

#include "nsIDOMXULElement.h"

#include "prmem.h"
#include "prenv.h"

#include "nsIServiceManager.h"
#include "nsMathUtils.h"

#include "nsContentUtils.h"

#include "nsIDocument.h"
#include "nsHTMLCanvasElement.h"
#include "nsSVGEffects.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIVariant.h"

#include "nsIInterfaceRequestorUtils.h"
#include "nsIFrame.h"
#include "nsError.h"
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
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIXPConnect.h"
#include "nsDisplayList.h"

#include "nsTArray.h"

#include "imgIEncoder.h"

#include "gfxContext.h"
#include "gfxASurface.h"
#include "gfxImageSurface.h"
#include "gfxPlatform.h"
#include "gfxFont.h"
#include "gfxBlur.h"
#include "gfxUtils.h"

#include "nsFrameManager.h"
#include "nsFrameLoader.h"
#include "nsBidi.h"
#include "nsBidiPresUtils.h"
#include "Layers.h"
#include "CanvasUtils.h"
#include "nsIMemoryReporter.h"
#include "nsStyleUtil.h"
#include "CanvasImageCache.h"

#include <algorithm>

#include "jsapi.h"
#include "jsfriendapi.h"

#include "mozilla/Assertions.h"
#include "mozilla/CheckedInt.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/ImageData.h"
#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/PathHelpers.h"
#include "mozilla/ipc/DocumentRendererParent.h"
#include "mozilla/ipc/PDocumentRendererParent.h"
#include "mozilla/Preferences.h"
#include "mozilla/unused.h"
#include "nsCCUncollectableMarker.h"
#include "nsWrapperCacheInlines.h"
#include "nsJSUtils.h"
#include "XPCQuickStubs.h"
#include "mozilla/dom/BindingUtils.h"
#include "nsHTMLImageElement.h"
#include "nsHTMLVideoElement.h"
#include "mozilla/dom/CanvasRenderingContext2DBinding.h"

#ifdef XP_WIN
#include "gfxWindowsPlatform.h"
#endif


#undef DrawText

using namespace mozilla;
using namespace mozilla::CanvasUtils;
using namespace mozilla::css;
using namespace mozilla::dom;
using namespace mozilla::gfx;
using namespace mozilla::ipc;
using namespace mozilla::layers;

namespace mgfx = mozilla::gfx;

static float kDefaultFontSize = 10.0;
static NS_NAMED_LITERAL_STRING(kDefaultFontName, "sans-serif");
static NS_NAMED_LITERAL_STRING(kDefaultFontStyle, "10px sans-serif");


static nsIMemoryReporter *gCanvasAzureMemoryReporter = nullptr;
static int64_t gCanvasAzureMemoryUsed = 0;

static int64_t GetCanvasAzureMemoryUsed() {
  return gCanvasAzureMemoryUsed;
}




NS_MEMORY_REPORTER_IMPLEMENT(CanvasAzureMemory,
  "canvas-2d-pixel-bytes",
  KIND_OTHER,
  UNITS_BYTES,
  GetCanvasAzureMemoryUsed,
  "Memory used by 2D canvases. Each canvas requires (width * height * 4) "
  "bytes.")

class nsCanvasRadialGradientAzure : public nsCanvasGradientAzure
{
public:
  nsCanvasRadialGradientAzure(const Point &aBeginOrigin, Float aBeginRadius,
                              const Point &aEndOrigin, Float aEndRadius)
    : nsCanvasGradientAzure(RADIAL)
    , mCenter1(aBeginOrigin)
    , mCenter2(aEndOrigin)
    , mRadius1(aBeginRadius)
    , mRadius2(aEndRadius)
  {
  }

  Point mCenter1;
  Point mCenter2;
  Float mRadius1;
  Float mRadius2;
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

protected:
  friend class CanvasGeneralPattern;

  
  Point mBegin;
  
  Point mEnd;
};





class CanvasGeneralPattern
{
public:
  typedef nsCanvasRenderingContext2DAzure::Style Style;
  typedef nsCanvasRenderingContext2DAzure::ContextState ContextState;

  CanvasGeneralPattern() : mPattern(nullptr) {}
  ~CanvasGeneralPattern()
  {
    if (mPattern) {
      mPattern->~Pattern();
    }
  }

  Pattern& ForStyle(nsCanvasRenderingContext2DAzure *aCtx,
                    Style aStyle,
                    DrawTarget *aRT)
  {
    
    
    NS_ASSERTION(!mPattern, "ForStyle() should only be called once on CanvasGeneralPattern!");

    const ContextState &state = aCtx->CurrentState();

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
        RadialGradientPattern(gradient->mCenter1, gradient->mCenter2, gradient->mRadius1,
                              gradient->mRadius2, gradient->GetGradientStopsForTarget(aRT));
    } else if (state.patternStyles[aStyle]) {
      if (aCtx->mCanvasElement) {
        CanvasUtils::DoDrawImageSecurityCheck(aCtx->mCanvasElement,
                                              state.patternStyles[aStyle]->mPrincipal,
                                              state.patternStyles[aStyle]->mForceWriteOnly,
                                              state.patternStyles[aStyle]->mCORSUsed);
      }

      ExtendMode mode;
      if (state.patternStyles[aStyle]->mRepeat == nsCanvasPatternAzure::NOREPEAT) {
        mode = EXTEND_CLAMP;
      } else {
        mode = EXTEND_REPEAT;
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
  typedef nsCanvasRenderingContext2DAzure::ContextState ContextState;

  AdjustedTarget(nsCanvasRenderingContext2DAzure *ctx,
                 mgfx::Rect *aBounds = nullptr)
    : mCtx(nullptr)
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

    mTempRect = mgfx::Rect(0, 0, ctx->mWidth, ctx->mHeight);

    static const gfxFloat GAUSSIAN_SCALE_FACTOR = (3 * sqrt(2 * M_PI) / 4) * 1.5;
    int32_t blurRadius = (int32_t) floor(mSigma * GAUSSIAN_SCALE_FACTOR + 0.5);

    
    
    mTempRect.Inflate(Margin(blurRadius + NS_MAX<Float>(state.shadowOffset.x, 0),
                             blurRadius + NS_MAX<Float>(state.shadowOffset.y, 0),
                             blurRadius + NS_MAX<Float>(-state.shadowOffset.x, 0),
                             blurRadius + NS_MAX<Float>(-state.shadowOffset.y, 0)));

    if (aBounds) {
      
      
      
      aBounds->Inflate(Margin(blurRadius, blurRadius,
                              blurRadius, blurRadius));
      mTempRect = mTempRect.Intersect(*aBounds);
    }

    mTempRect.ScaleRoundOut(1.0f);

    transform._31 -= mTempRect.x;
    transform._32 -= mTempRect.y;
      
    mTarget =
      mCtx->mTarget->CreateShadowDrawTarget(IntSize(int32_t(mTempRect.width), int32_t(mTempRect.height)),
                                             FORMAT_B8G8R8A8, mSigma);

    if (!mTarget) {
      
      
      mTarget = ctx->mTarget;
      mCtx = nullptr;
    } else {
      mTarget->SetTransform(transform);
    }
  }

  ~AdjustedTarget()
  {
    if (!mCtx) {
      return;
    }

    RefPtr<SourceSurface> snapshot = mTarget->Snapshot();
    
    mCtx->mTarget->DrawSurfaceWithShadow(snapshot, mTempRect.TopLeft(),
                                         Color::FromABGR(mCtx->CurrentState().shadowColor),
                                         mCtx->CurrentState().shadowOffset, mSigma,
                                         mCtx->CurrentState().op);
  }

  DrawTarget* operator->()
  {
    return mTarget;
  }

private:
  RefPtr<DrawTarget> mTarget;
  nsCanvasRenderingContext2DAzure *mCtx;
  Float mSigma;
  mgfx::Rect mTempRect;
};

NS_IMETHODIMP
nsCanvasGradientAzure::AddColorStop(float offset, const nsAString& colorstr)
{
  if (!FloatValidate(offset) || offset < 0.0 || offset > 1.0) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCSSValue value;
  nsCSSParser parser;
  if (!parser.ParseColorString(colorstr, nullptr, 0, value)) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  nscolor color;
  if (!nsRuleNode::ComputeColor(value, nullptr, nullptr, color)) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  mStops = nullptr;

  GradientStop newStop;

  newStop.offset = offset;
  newStop.color = Color::FromABGR(color);

  mRawStops.AppendElement(newStop);

  return NS_OK;
}

NS_DEFINE_STATIC_IID_ACCESSOR(nsCanvasGradientAzure, NS_CANVASGRADIENTAZURE_PRIVATE_IID)

NS_IMPL_ADDREF(nsCanvasGradientAzure)
NS_IMPL_RELEASE(nsCanvasGradientAzure)




NS_INTERFACE_MAP_BEGIN(nsCanvasGradientAzure)
  NS_INTERFACE_MAP_ENTRY(nsCanvasGradientAzure)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCanvasGradient)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CanvasGradient)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

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


const Float SIGMA_MAX = 100;

class CanvasRenderingContext2DUserDataAzure : public LayerUserData {
public:
    CanvasRenderingContext2DUserDataAzure(nsCanvasRenderingContext2DAzure *aContext)
    : mContext(aContext)
  {
    aContext->mUserDatas.AppendElement(this);
  }
  ~CanvasRenderingContext2DUserDataAzure()
  {
    if (mContext) {
      mContext->mUserDatas.RemoveElement(this);
    }
  }
  static void DidTransactionCallback(void* aData)
  {
      CanvasRenderingContext2DUserDataAzure* self =
      static_cast<CanvasRenderingContext2DUserDataAzure*>(aData);
    if (self->mContext) {
      self->mContext->MarkContextClean();
    }
  }
  bool IsForContext(nsCanvasRenderingContext2DAzure *aContext)
  {
    return mContext == aContext;
  }
  void Forget()
  {
    mContext = nullptr;
  }

private:
  nsCanvasRenderingContext2DAzure *mContext;
};

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsCanvasRenderingContext2DAzure)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsCanvasRenderingContext2DAzure)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsCanvasRenderingContext2DAzure)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsCanvasRenderingContext2DAzure)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCanvasElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsCanvasRenderingContext2DAzure)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsCanvasRenderingContext2DAzure)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mCanvasElement, nsINode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(nsCanvasRenderingContext2DAzure)
 if (nsCCUncollectableMarker::sGeneration && tmp->IsBlack()) {
    nsGenericElement* canvasElement = tmp->mCanvasElement;
    if (canvasElement) {
      if (canvasElement->IsPurple()) {
        canvasElement->RemovePurple();
      }
      nsGenericElement::MarkNodeChildren(canvasElement);
    }
    return true;
  }
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(nsCanvasRenderingContext2DAzure)
  return nsCCUncollectableMarker::sGeneration && tmp->IsBlack();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(nsCanvasRenderingContext2DAzure)
  return nsCCUncollectableMarker::sGeneration && tmp->IsBlack();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END




NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsCanvasRenderingContext2DAzure)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsIDOMCanvasRenderingContext2D)
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextInternal)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports,
                                   nsICanvasRenderingContextInternal)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CanvasRenderingContext2D)
NS_INTERFACE_MAP_END







uint32_t nsCanvasRenderingContext2DAzure::sNumLivingContexts = 0;
uint8_t (*nsCanvasRenderingContext2DAzure::sUnpremultiplyTable)[256] = nullptr;
uint8_t (*nsCanvasRenderingContext2DAzure::sPremultiplyTable)[256] = nullptr;

namespace mozilla {
namespace dom {

bool
AzureCanvasEnabled()
{
  return gfxPlatform::GetPlatform()->SupportsAzureCanvas();
}

}
}

nsresult
NS_NewCanvasRenderingContext2DAzure(nsIDOMCanvasRenderingContext2D** aResult)
{
  
  
  if (!AzureCanvasEnabled()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<nsIDOMCanvasRenderingContext2D> ctx = new nsCanvasRenderingContext2DAzure();
  if (!ctx)
    return NS_ERROR_OUT_OF_MEMORY;

  *aResult = ctx.forget().get();
  return NS_OK;
}

nsCanvasRenderingContext2DAzure::nsCanvasRenderingContext2DAzure()
  : mValid(false), mZero(false), mOpaque(false), mResetLayer(true)
  , mIPC(false)
  , mIsEntireFrameInvalid(false)
  , mPredictManyRedrawCalls(false), mPathTransformWillUpdate(false)
  , mInvalidateCount(0)
{
  sNumLivingContexts++;
  SetIsDOMBinding();
}

nsCanvasRenderingContext2DAzure::~nsCanvasRenderingContext2DAzure()
{
  Reset();
  
  for (uint32_t i = 0; i < mUserDatas.Length(); ++i) {
    mUserDatas[i]->Forget();
  }
  sNumLivingContexts--;
  if (!sNumLivingContexts) {
    delete[] sUnpremultiplyTable;
    delete[] sPremultiplyTable;
    sUnpremultiplyTable = nullptr;
    sPremultiplyTable = nullptr;
  }
}

JSObject*
nsCanvasRenderingContext2DAzure::WrapObject(JSContext *cx, JSObject *scope,
                                            bool *triedToWrap)
{
  return CanvasRenderingContext2DBinding::Wrap(cx, scope, this, triedToWrap);
}

bool
nsCanvasRenderingContext2DAzure::ParseColor(const nsAString& aString,
                                            nscolor* aColor)
{
  nsIDocument* document = mCanvasElement
                          ? mCanvasElement->OwnerDoc()
                          : nullptr;

  
  
  nsCSSParser parser(document ? document->CSSLoader() : nullptr);
  nsCSSValue value;
  if (!parser.ParseColorString(aString, nullptr, 0, value)) {
    return false;
  }

  nsIPresShell* presShell = GetPresShell();
  nsRefPtr<nsStyleContext> parentContext;
  if (mCanvasElement && mCanvasElement->IsInDoc()) {
    
    parentContext = nsComputedDOMStyle::GetStyleContextForElement(
      mCanvasElement, nullptr, presShell);
  }

  unused << nsRuleNode::ComputeColor(
    value, presShell ? presShell->GetPresContext() : nullptr, parentContext,
    *aColor);
  return true;
}

nsresult
nsCanvasRenderingContext2DAzure::Reset()
{
  if (mCanvasElement) {
    mCanvasElement->InvalidateCanvas();
  }

  
  
  if (mValid && !mDocShell) {
    gCanvasAzureMemoryUsed -= mWidth * mHeight * 4;
  }

  mTarget = nullptr;

  
  
  mThebesSurface = nullptr;
  mValid = false;
  mIsEntireFrameInvalid = false;
  mPredictManyRedrawCalls = false;

  return NS_OK;
}

static void
WarnAboutUnexpectedStyle(nsHTMLCanvasElement* canvasElement)
{
  nsContentUtils::ReportToConsole(
    nsIScriptError::warningFlag,
    "Canvas",
    canvasElement ? canvasElement->OwnerDoc() : nullptr,
    nsContentUtils::eDOM_PROPERTIES,
    "UnexpectedCanvasVariantStyle");
}

void
nsCanvasRenderingContext2DAzure::SetStyleFromString(const nsAString& str,
                                                    Style whichStyle)
{
  MOZ_ASSERT(!str.IsVoid());

  nscolor color;
  if (!ParseColor(str, &color)) {
    return;
  }

  CurrentState().SetColorStyle(whichStyle, color);
}

void
nsCanvasRenderingContext2DAzure::SetStyleFromStringOrInterface(const nsAString& aStr,
                                                               nsISupports *aInterface,
                                                               Style aWhichStyle)
{
  if (!aStr.IsVoid()) {
    SetStyleFromString(aStr, aWhichStyle);
    return;
  }

  if (aInterface) {
    nsCOMPtr<nsCanvasGradientAzure> grad(do_QueryInterface(aInterface));
    if (grad) {
      SetStyleFromGradient(grad, aWhichStyle);
      return;
    }

    nsCOMPtr<nsCanvasPatternAzure> pattern(do_QueryInterface(aInterface));
    if (pattern) {
      SetStyleFromPattern(pattern, aWhichStyle);
      return;
    }
  }

  WarnAboutUnexpectedStyle(mCanvasElement);
}

nsISupports*
nsCanvasRenderingContext2DAzure::GetStyleAsStringOrInterface(nsAString& aStr,
                                                             CanvasMultiGetterType& aType,
                                                             Style aWhichStyle)
{
  const ContextState &state = CurrentState();
  nsISupports* supports;
  if (state.patternStyles[aWhichStyle]) {
    aStr.SetIsVoid(true);
    supports = state.patternStyles[aWhichStyle];
    aType = CMG_STYLE_PATTERN;
  } else if (state.gradientStyles[aWhichStyle]) {
    aStr.SetIsVoid(true);
    supports = state.gradientStyles[aWhichStyle];
    aType = CMG_STYLE_GRADIENT;
  } else {
    StyleColorToString(state.colorStyles[aWhichStyle], aStr);
    supports = nullptr;
    aType = CMG_STYLE_STRING;
  }
  return supports;
}


void
nsCanvasRenderingContext2DAzure::StyleColorToString(const nscolor& aColor, nsAString& aStr)
{
  
  
  if (NS_GET_A(aColor) == 255) {
    CopyUTF8toUTF16(nsPrintfCString("#%02x%02x%02x",
                                    NS_GET_R(aColor),
                                    NS_GET_G(aColor),
                                    NS_GET_B(aColor)),
                    aStr);
  } else {
    CopyUTF8toUTF16(nsPrintfCString("rgba(%d, %d, %d, ",
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

  mIsEntireFrameInvalid = true;

  if (!mCanvasElement) {
    NS_ASSERTION(mDocShell, "Redraw with no canvas element or docshell!");
    return NS_OK;
  }

  if (!mThebesSurface)
    mThebesSurface =
      gfxPlatform::GetPlatform()->GetThebesSurfaceForDrawTarget(mTarget);
  mThebesSurface->MarkDirty();

  nsSVGEffects::InvalidateDirectRenderingObservers(mCanvasElement);

  mCanvasElement->InvalidateCanvasContent(nullptr);

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

  if (!mThebesSurface)
    mThebesSurface =
      gfxPlatform::GetPlatform()->GetThebesSurfaceForDrawTarget(mTarget);
  mThebesSurface->MarkDirty();

  nsSVGEffects::InvalidateDirectRenderingObservers(mCanvasElement);

  gfxRect tmpR = ThebesRect(r);
  mCanvasElement->InvalidateCanvasContent(&tmpR);

  return;
}

void
nsCanvasRenderingContext2DAzure::RedrawUser(const gfxRect& r)
{
  if (mIsEntireFrameInvalid) {
    ++mInvalidateCount;
    return;
  }

  mgfx::Rect newr =
    mTarget->GetTransform().TransformBounds(ToRect(r));
  Redraw(newr);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetDimensions(int32_t width, int32_t height)
{
  RefPtr<DrawTarget> target;

  
  if (height == 0 || width == 0) {
    mZero = true;
    height = 1;
    width = 1;
  } else {
    mZero = false;
  }

  
  IntSize size(width, height);
  if (size.width <= 0xFFFF && size.height <= 0xFFFF &&
      size.width >= 0 && size.height >= 0) {
    SurfaceFormat format = GetSurfaceFormat();
    nsIDocument* ownerDoc = nullptr;
    if (mCanvasElement) {
      ownerDoc = mCanvasElement->OwnerDoc();
    }

    nsRefPtr<LayerManager> layerManager = nullptr;

    if (ownerDoc) {
      layerManager =
        nsContentUtils::PersistentLayerManagerForDocument(ownerDoc);
    }

    if (layerManager) {
      target = layerManager->CreateDrawTarget(size, format);
    } else {
      target = gfxPlatform::GetPlatform()->CreateOffscreenDrawTarget(size, format);
    }
  }

  if (target) {
    if (gCanvasAzureMemoryReporter == nullptr) {
        gCanvasAzureMemoryReporter = new NS_MEMORY_REPORTER_NAME(CanvasAzureMemory);
      NS_RegisterMemoryReporter(gCanvasAzureMemoryReporter);
    }

    gCanvasAzureMemoryUsed += width * height * 4;
    JSContext* context = nsContentUtils::GetCurrentJSContext();
    if (context) {
      JS_updateMallocCounter(context, width * height * 4);
    }
  }

  return InitializeWithTarget(target, width, height);
}

nsresult
nsCanvasRenderingContext2DAzure::Initialize(int32_t width, int32_t height)
{
  mWidth = width;
  mHeight = height;

  if (!mValid) {
    
    
    
    mTarget = gfxPlatform::GetPlatform()->CreateOffscreenDrawTarget(IntSize(1, 1), FORMAT_B8G8R8A8);
  }

  mResetLayer = true;

  
  mStyleStack.Clear();
  mPathBuilder = nullptr;
  mPath = nullptr;
  mDSPathBuilder = nullptr;

  ContextState *state = mStyleStack.AppendElement();
  state->globalAlpha = 1.0;

  state->colorStyles[STYLE_FILL] = NS_RGB(0,0,0);
  state->colorStyles[STYLE_STROKE] = NS_RGB(0,0,0);
  state->shadowColor = NS_RGBA(0,0,0,0);

  if (mTarget) {
    mTarget->ClearRect(mgfx::Rect(Point(0, 0), Size(mWidth, mHeight)));
    
    
    Redraw();
  }

  return mValid ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult
nsCanvasRenderingContext2DAzure::InitializeWithTarget(DrawTarget *target, int32_t width, int32_t height)
{
  Reset();

  NS_ASSERTION(mCanvasElement, "Must have a canvas element!");
  mDocShell = nullptr;

  
  
  
  
  
  
  
  

  if (target) {
    mValid = true;
    mTarget = target;
  } else {
    mValid = false;
  }

  return Initialize(width, height);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::InitializeWithSurface(nsIDocShell *shell, gfxASurface *surface, int32_t width, int32_t height)
{
  mDocShell = shell;
  mThebesSurface = surface;

  mTarget = gfxPlatform::GetPlatform()->CreateDrawTargetForSurface(surface, IntSize(width, height));
  mValid = mTarget != nullptr;

  return Initialize(width, height);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetIsOpaque(bool isOpaque)
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
nsCanvasRenderingContext2DAzure::SetIsIPC(bool isIPC)
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
nsCanvasRenderingContext2DAzure::Render(gfxContext *ctx, gfxPattern::GraphicsFilter aFilter, uint32_t aFlags)
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

  if (!(aFlags & RenderFlagPremultAlpha)) {
      nsRefPtr<gfxASurface> curSurface = ctx->CurrentSurface();
      nsRefPtr<gfxImageSurface> gis = curSurface->GetAsImageSurface();
      NS_ABORT_IF_FALSE(gis, "If non-premult alpha, must be able to get image surface!");

      gfxUtils::UnpremultiplyImageSurface(gis);
  }

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

  nsAutoArrayPtr<uint8_t> imageBuffer(new (std::nothrow) uint8_t[mWidth * mHeight * 4]);
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
nsCanvasRenderingContext2DAzure::GetCanvas(nsIDOMHTMLCanvasElement **canvas)
{
  NS_IF_ADDREF(*canvas = GetCanvas());

  return NS_OK;
}





void
nsCanvasRenderingContext2DAzure::Save()
{
  mStyleStack[mStyleStack.Length() - 1].transform = mTarget->GetTransform();
  mStyleStack.SetCapacity(mStyleStack.Length() + 1);
  mStyleStack.AppendElement(CurrentState());
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MozSave()
{
  Save();
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::Restore()
{
  if (mStyleStack.Length() - 1 == 0)
    return;

  for (uint32_t i = 0; i < CurrentState().clipsPushed.size(); i++) {
    mTarget->PopClip();
  }

  mStyleStack.RemoveElementAt(mStyleStack.Length() - 1);

  TransformWillUpdate();

  mTarget->SetTransform(CurrentState().transform);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MozRestore()
{
  Restore();
  return NS_OK;
}





void
nsCanvasRenderingContext2DAzure::Scale(double x, double y, ErrorResult& error)
{
  if (!mTarget) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  if (!FloatValidate(x,y)) {
    return;
  }

  TransformWillUpdate();

  Matrix newMatrix = mTarget->GetTransform();
  mTarget->SetTransform(newMatrix.Scale(x, y));
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Scale(float x, float y)
{
  ErrorResult rv;
  Scale((double)x, (double)y, rv);
  return rv.ErrorCode();
}

void
nsCanvasRenderingContext2DAzure::Rotate(double angle, ErrorResult& error)
{
  if (!mTarget) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  if (!FloatValidate(angle)) {
    return;
  }

  TransformWillUpdate();

  Matrix rotation = Matrix::Rotation(angle);
  mTarget->SetTransform(rotation * mTarget->GetTransform());
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Rotate(float angle)
{
  ErrorResult rv;
  Rotate((double)angle, rv);
  return rv.ErrorCode();
}

void
nsCanvasRenderingContext2DAzure::Translate(double x, double y, ErrorResult& error)
{
  if (!mTarget) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  if (!FloatValidate(x,y)) {
    return;
  }

  TransformWillUpdate();

  Matrix newMatrix = mTarget->GetTransform();
  mTarget->SetTransform(newMatrix.Translate(x, y));
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Translate(float x, float y)
{
  ErrorResult rv;
  Translate((double)x, (double)y, rv);
  return rv.ErrorCode();
}

void
nsCanvasRenderingContext2DAzure::Transform(double m11, double m12, double m21,
                                           double m22, double dx, double dy,
                                           ErrorResult& error)
{
  if (!mTarget) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  if (!FloatValidate(m11,m12,m21,m22,dx,dy)) {
    return;
  }

  TransformWillUpdate();

  Matrix matrix(m11, m12, m21, m22, dx, dy);
  mTarget->SetTransform(matrix * mTarget->GetTransform());
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Transform(float m11, float m12, float m21, float m22, float dx, float dy)
{
  ErrorResult rv;
  Transform((double)m11, (double)m12, (double)m21, (double)m22, (double)dx,
            (double)dy, rv);
  return rv.ErrorCode();
}

void
nsCanvasRenderingContext2DAzure::SetTransform(double m11, double m12,
                                              double m21, double m22,
                                              double dx, double dy,
                                              ErrorResult& error)
{
  if (!mTarget) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  if (!FloatValidate(m11,m12,m21,m22,dx,dy)) {
    return;
  }

  TransformWillUpdate();

  Matrix matrix(m11, m12, m21, m22, dx, dy);
  mTarget->SetTransform(matrix);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetTransform(float m11, float m12, float m21, float m22, float dx, float dy)
{
  ErrorResult rv;
  SetTransform((double)m11, (double)m12, (double)m21, (double)m22, (double)dx,
               (double)dy, rv);
  return rv.ErrorCode();
}

JSObject*
MatrixToJSObject(JSContext* cx, const Matrix& matrix, ErrorResult& error)
{
  jsval elts[] = {
      DOUBLE_TO_JSVAL(matrix._11), DOUBLE_TO_JSVAL(matrix._12),
      DOUBLE_TO_JSVAL(matrix._21), DOUBLE_TO_JSVAL(matrix._22),
      DOUBLE_TO_JSVAL(matrix._31), DOUBLE_TO_JSVAL(matrix._32)
  };

  
  JSObject* obj = JS_NewArrayObject(cx, 6, elts);
  if  (!obj) {
      error.Throw(NS_ERROR_OUT_OF_MEMORY);
  }
  return obj;
}

bool
ObjectToMatrix(JSContext* cx, JSObject& obj, Matrix& matrix, ErrorResult& error)
{
  uint32_t length;
  if (!JS_GetArrayLength(cx, &obj, &length) || length != 6) {
    
    error.Throw(NS_ERROR_INVALID_ARG);
    return false;
  }

  Float* elts[] = { &matrix._11, &matrix._12, &matrix._21, &matrix._22,
                    &matrix._31, &matrix._32 };
  for (uint32_t i = 0; i < 6; ++i) {
    jsval elt;
    double d;
    if (!JS_GetElement(cx, &obj, i, &elt)) {
      error.Throw(NS_ERROR_FAILURE);
      return false;
    }
    if (!CoerceDouble(elt, &d)) {
      error.Throw(NS_ERROR_INVALID_ARG);
      return false;
    }
    if (!FloatValidate(d)) {
      
      return false;
    }
    *elts[i] = Float(d);
  }
  return true;
}

void
nsCanvasRenderingContext2DAzure::SetMozCurrentTransform(JSContext* cx,
                                                        JSObject& currentTransform,
                                                        ErrorResult& error)
{
  if (!mTarget) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  Matrix newCTM;
  if (ObjectToMatrix(cx, currentTransform, newCTM, error)) {
    mTarget->SetTransform(newCTM);
  }
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozCurrentTransform(JSContext* cx,
                                                        const jsval& matrix)
{
  if (!matrix.isObject()) {
    return NS_ERROR_INVALID_ARG;
  }

  ErrorResult rv;
  SetMozCurrentTransform(cx, matrix.toObject(), rv);
  return rv.ErrorCode();
}

JSObject*
nsCanvasRenderingContext2DAzure::GetMozCurrentTransform(JSContext* cx,
                                                        ErrorResult& error) const
{
  if (!mTarget) {
    error.Throw(NS_ERROR_FAILURE);
    return NULL;
  }

  return MatrixToJSObject(cx, mTarget->GetTransform(), error);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozCurrentTransform(JSContext* cx,
                                                        jsval* matrix)
{
  ErrorResult rv;
  JSObject* obj = GetMozCurrentTransform(cx, rv);
  if (!rv.Failed()) {
    *matrix = OBJECT_TO_JSVAL(obj);
  }
  return rv.ErrorCode();
}

void
nsCanvasRenderingContext2DAzure::SetMozCurrentTransformInverse(JSContext* cx,
                                                               JSObject& currentTransform,
                                                               ErrorResult& error)
{
  if (!mTarget) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  Matrix newCTMInverse;
  if (ObjectToMatrix(cx, currentTransform, newCTMInverse, error)) {
    
    if (newCTMInverse.Invert()) {
      mTarget->SetTransform(newCTMInverse);
    }
  }
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozCurrentTransformInverse(JSContext* cx,
                                                               const jsval& matrix)
{
  if (!matrix.isObject()) {
    return NS_ERROR_INVALID_ARG;
  }

  ErrorResult rv;
  SetMozCurrentTransformInverse(cx, matrix.toObject(), rv);
  return rv.ErrorCode();
}

JSObject*
nsCanvasRenderingContext2DAzure::GetMozCurrentTransformInverse(JSContext* cx,
                                                               ErrorResult& error) const
{
  if (!mTarget) {
    error.Throw(NS_ERROR_FAILURE);
    return NULL;
  }

  Matrix ctm = mTarget->GetTransform();

  if (!ctm.Invert()) {
    double NaN = JSVAL_TO_DOUBLE(JS_GetNaNValue(cx));
    ctm = Matrix(NaN, NaN, NaN, NaN, NaN, NaN);
  }

  return MatrixToJSObject(cx, ctm, error);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozCurrentTransformInverse(JSContext* cx,
                                                               jsval* matrix)
{
  ErrorResult rv;
  JSObject* obj = GetMozCurrentTransformInverse(cx, rv);
  if (!rv.Failed()) {
    *matrix = OBJECT_TO_JSVAL(obj);
  }
  return rv.ErrorCode();
}





NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetGlobalAlpha(float aGlobalAlpha)
{
  SetGlobalAlpha((double)aGlobalAlpha);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetGlobalAlpha(float *aGlobalAlpha)
{
  *aGlobalAlpha = GetGlobalAlpha();
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::SetStyleFromJSValue(JSContext* cx,
                                                     JS::Value& value,
                                                     Style whichStyle)
{
  if (value.isString()) {
    nsDependentJSString strokeStyle;
    if (strokeStyle.init(cx, value.toString())) {
      SetStyleFromString(strokeStyle, whichStyle);
    }
    return;
  }

  if (value.isObject()) {
    nsCOMPtr<nsISupports> holder;

    nsCanvasGradientAzure* gradient;
    nsresult rv = xpc_qsUnwrapArg<nsCanvasGradientAzure>(cx, value, &gradient,
                                                         static_cast<nsISupports**>(getter_AddRefs(holder)),
                                                         &value);
    if (NS_SUCCEEDED(rv)) {
      SetStyleFromGradient(gradient, whichStyle);
      return;
    }

    nsCanvasPatternAzure* pattern;
    rv = xpc_qsUnwrapArg<nsCanvasPatternAzure>(cx, value, &pattern,
                                               static_cast<nsISupports**>(getter_AddRefs(holder)),
                                               &value);
    if (NS_SUCCEEDED(rv)) {
      SetStyleFromPattern(pattern, whichStyle);
      return;
    }
  }

  WarnAboutUnexpectedStyle(mCanvasElement);
}

static JS::Value
WrapStyle(JSContext* cx, JSObject* obj,
          nsIDOMCanvasRenderingContext2D::CanvasMultiGetterType type,
          nsAString& str, nsISupports* supports, ErrorResult& error)
{
  JS::Value v;
  bool ok;
  switch (type) {
    case nsIDOMCanvasRenderingContext2D::CMG_STYLE_STRING:
    {
      ok = xpc::StringToJsval(cx, str, &v);
      break;
    }
    case nsIDOMCanvasRenderingContext2D::CMG_STYLE_PATTERN:
    case nsIDOMCanvasRenderingContext2D::CMG_STYLE_GRADIENT:
    {
      ok = dom::WrapObject(cx, obj, supports, &v);
      break;
    }
    default:
      MOZ_NOT_REACHED("unexpected CanvasMultiGetterType");
  }
  if (!ok) {
    error.Throw(NS_ERROR_FAILURE);
  }
  return v;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetStrokeStyle(nsIVariant *aValue)
{
  if (!aValue)
      return NS_ERROR_FAILURE;

  nsString str;

  nsresult rv;
  uint16_t vtype;
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

    str.SetIsVoid(true);
    return SetStrokeStyle_multi(str, sup);
  }

  rv = aValue->GetAsAString(str);
  NS_ENSURE_SUCCESS(rv, rv);

  return SetStrokeStyle_multi(str, nullptr);
}

JS::Value
nsCanvasRenderingContext2DAzure::GetStrokeStyle(JSContext* cx,
                                                ErrorResult& error)
{
  nsString str;
  CanvasMultiGetterType t;
  nsISupports* supports = GetStyleAsStringOrInterface(str, t, STYLE_STROKE);
  return WrapStyle(cx, GetWrapper(), t, str, supports, error);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetStrokeStyle(nsIVariant **aResult)
{
  nsCOMPtr<nsIWritableVariant> wv = do_CreateInstance(NS_VARIANT_CONTRACTID);

  nsCOMPtr<nsISupports> sup;
  nsString str;
  int32_t t;
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
  uint16_t vtype;
  rv = aValue->GetDataType(&vtype);
  NS_ENSURE_SUCCESS(rv, rv);

  if (vtype == nsIDataType::VTYPE_INTERFACE ||
      vtype == nsIDataType::VTYPE_INTERFACE_IS)
  {
    nsIID *iid;
    nsCOMPtr<nsISupports> sup;
    rv = aValue->GetAsInterface(&iid, getter_AddRefs(sup));
    NS_ENSURE_SUCCESS(rv, rv);

    str.SetIsVoid(true);
    return SetFillStyle_multi(str, sup);
  }

  rv = aValue->GetAsAString(str);
  NS_ENSURE_SUCCESS(rv, rv);

  return SetFillStyle_multi(str, nullptr);
}

JS::Value
nsCanvasRenderingContext2DAzure::GetFillStyle(JSContext* cx,
                                              ErrorResult& error)
{
  nsString str;
  CanvasMultiGetterType t;
  nsISupports* supports = GetStyleAsStringOrInterface(str, t, STYLE_FILL);
  return WrapStyle(cx, GetWrapper(), t, str, supports, error);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetFillStyle(nsIVariant **aResult)
{
  nsCOMPtr<nsIWritableVariant> wv = do_CreateInstance(NS_VARIANT_CONTRACTID);

  nsCOMPtr<nsISupports> sup;
  nsString str;
  int32_t t;
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

void
nsCanvasRenderingContext2DAzure::SetFillRule(const nsAString& aString)
{
  FillRule rule;

  if (aString.EqualsLiteral("evenodd"))
    rule = FILL_EVEN_ODD;
  else if (aString.EqualsLiteral("nonzero"))
    rule = FILL_WINDING;
  else
    return;

  CurrentState().fillRule = rule;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozFillRule(const nsAString& aString)
{
  SetFillRule(aString);
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::GetFillRule(nsAString& aString)
{
    switch (CurrentState().fillRule) {
    case FILL_WINDING:
        aString.AssignLiteral("nonzero"); break;
    case FILL_EVEN_ODD:
        aString.AssignLiteral("evenodd"); break;
    }
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozFillRule(nsAString& aString)
{
  GetFillRule(aString);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetStrokeStyle_multi(const nsAString& aStr, nsISupports *aInterface)
{
  SetStyleFromStringOrInterface(aStr, aInterface, STYLE_STROKE);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetStrokeStyle_multi(nsAString& aStr, nsISupports **aInterface, int32_t *aType)
{
  CanvasMultiGetterType type;
  NS_IF_ADDREF(*aInterface = GetStyleAsStringOrInterface(aStr, type, STYLE_STROKE));
  *aType = type;
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetFillStyle_multi(const nsAString& aStr, nsISupports *aInterface)
{
  SetStyleFromStringOrInterface(aStr, aInterface, STYLE_FILL);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetFillStyle_multi(nsAString& aStr, nsISupports **aInterface, int32_t *aType)
{
  CanvasMultiGetterType type;
  NS_IF_ADDREF(*aInterface = GetStyleAsStringOrInterface(aStr, type, STYLE_FILL));
  *aType = type;
  return NS_OK;
}




already_AddRefed<nsIDOMCanvasGradient>
nsCanvasRenderingContext2DAzure::CreateLinearGradient(double x0, double y0, double x1, double y1,
                                                      ErrorResult& aError)
{
  if (!FloatValidate(x0,y0,x1,y1)) {
    aError.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return nullptr;
  }

  nsRefPtr<nsIDOMCanvasGradient> grad =
    new nsCanvasLinearGradientAzure(Point(x0, y0), Point(x1, y1));

  return grad.forget();
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::CreateLinearGradient(float x0, float y0, float x1, float y1,
                                                      nsIDOMCanvasGradient **_retval)
{
  ErrorResult rv;
  *_retval = CreateLinearGradient(x0, y0, x1, y1, rv).get();
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMCanvasGradient>
nsCanvasRenderingContext2DAzure::CreateRadialGradient(double x0, double y0, double r0,
                                                      double x1, double y1, double r1,
                                                      ErrorResult& aError)
{
  if (!FloatValidate(x0,y0,r0,x1,y1,r1)) {
    aError.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return nullptr;
  }

  if (r0 < 0.0 || r1 < 0.0) {
    aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsRefPtr<nsIDOMCanvasGradient> grad =
    new nsCanvasRadialGradientAzure(Point(x0, y0), r0, Point(x1, y1), r1);

  return grad.forget();
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::CreateRadialGradient(float x0, float y0, float r0,
                                                      float x1, float y1, float r1,
                                                      nsIDOMCanvasGradient **_retval)
{
  ErrorResult rv;
  *_retval = CreateRadialGradient(x0, y0, r0, x1, y1, r1, rv).get();
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMCanvasPattern>
nsCanvasRenderingContext2DAzure::CreatePattern(const HTMLImageOrCanvasOrVideoElement& element,
                                               const nsAString& repeat,
                                               ErrorResult& error)
{
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
    error.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return NULL;
  }

  Element* htmlElement;
  if (element.IsHTMLCanvasElement()) {
    nsHTMLCanvasElement* canvas = element.GetAsHTMLCanvasElement();
    htmlElement = canvas;

    nsIntSize size = canvas->GetSize();
    if (size.width == 0 || size.height == 0) {
      error.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
      return NULL;
    }

    
    nsICanvasRenderingContextInternal *srcCanvas = canvas->GetContextAtIndex(0);
    if (srcCanvas) {
      
      RefPtr<SourceSurface> srcSurf = srcCanvas->GetSurfaceSnapshot();

      nsRefPtr<nsCanvasPatternAzure> pat =
        new nsCanvasPatternAzure(srcSurf, repeatMode, htmlElement->NodePrincipal(), canvas->IsWriteOnly(), false);

      return pat.forget();
    }
  } else if (element.IsHTMLImageElement()) {
    htmlElement = element.GetAsHTMLImageElement();
  } else {
    htmlElement = element.GetAsHTMLVideoElement();
  }

  
  
  nsLayoutUtils::SurfaceFromElementResult res =
    nsLayoutUtils::SurfaceFromElement(htmlElement,
      nsLayoutUtils::SFE_WANT_FIRST_FRAME | nsLayoutUtils::SFE_WANT_NEW_SURFACE);

  if (!res.mSurface) {
    error.Throw(NS_ERROR_NOT_AVAILABLE);
    return NULL;
  }

  
  if (!res.mSurface->CairoSurface() || res.mSurface->CairoStatus()) {
    return NULL;
  }

  RefPtr<SourceSurface> srcSurf =
    gfxPlatform::GetPlatform()->GetSourceSurfaceForSurface(mTarget, res.mSurface);

  nsRefPtr<nsCanvasPatternAzure> pat =
    new nsCanvasPatternAzure(srcSurf, repeatMode, res.mPrincipal,
                             res.mIsWriteOnly, res.mCORSUsed);

  return pat.forget();
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::CreatePattern(nsIDOMHTMLElement *image,
                                               const nsAString& repeat,
                                               nsIDOMCanvasPattern **_retval)
{
  HTMLImageOrCanvasOrVideoElement element;
  if (!ToHTMLImageOrCanvasOrVideoElement(image, element)) {
    return NS_ERROR_DOM_TYPE_MISMATCH_ERR;
  }

  ErrorResult rv;
  *_retval = CreatePattern(element, repeat, rv).get();
  return rv.ErrorCode();
}




NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetShadowOffsetX(float x)
{
  SetShadowOffsetX((double)x);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetShadowOffsetX(float *x)
{
  *x = static_cast<float>(GetShadowOffsetX());
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetShadowOffsetY(float y)
{
  SetShadowOffsetY((double)y);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetShadowOffsetY(float *y)
{
  *y = static_cast<float>(GetShadowOffsetY());
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetShadowBlur(float blur)
{
  SetShadowBlur((double)blur);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetShadowBlur(float *blur)
{
  *blur = GetShadowBlur();
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::SetShadowColor(const nsAString& shadowColor)
{
  nscolor color;
  if (!ParseColor(shadowColor, &color)) {
    return;
  }

  CurrentState().shadowColor = color;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozShadowColor(const nsAString& colorstr)
{
  SetShadowColor(colorstr);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozShadowColor(nsAString& color)
{
  GetShadowColor(color);
  return NS_OK;
}





void
nsCanvasRenderingContext2DAzure::ClearRect(double x, double y, double w,
                                           double h)
{
  if (!FloatValidate(x,y,w,h)) {
    return;
  }
 
  mTarget->ClearRect(mgfx::Rect(x, y, w, h));

  RedrawUser(gfxRect(x, y, w, h));
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::ClearRect(float x, float y, float w, float h)
{
  ClearRect((double)x, (double)y, (double)w, (double)h);
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::FillRect(double x, double y, double w,
                                          double h)
{
  if (!FloatValidate(x,y,w,h)) {
    return;
  }

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

  mgfx::Rect bounds;
  
  if (NeedToDrawShadow()) {
    bounds = mgfx::Rect(x, y, w, h);
    bounds = mTarget->GetTransform().TransformBounds(bounds);
  }

  AdjustedTarget(this, bounds.IsEmpty() ? nullptr : &bounds)->
    FillRect(mgfx::Rect(x, y, w, h),
             CanvasGeneralPattern().ForStyle(this, STYLE_FILL, mTarget),
             DrawOptions(state.globalAlpha, UsedOperation()));

  RedrawUser(gfxRect(x, y, w, h));
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::FillRect(float x, float y, float w, float h)
{
  FillRect((double)x, (double)y, (double)w, (double)h);
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::StrokeRect(double x, double y, double w,
                                            double h)
{
  if (!FloatValidate(x,y,w,h)) {
    return;
  }

  const ContextState &state = CurrentState();

  mgfx::Rect bounds;
  
  if (NeedToDrawShadow()) {
    bounds = mgfx::Rect(x - state.lineWidth / 2.0f, y - state.lineWidth / 2.0f,
                        w + state.lineWidth, h + state.lineWidth);
    bounds = mTarget->GetTransform().TransformBounds(bounds);
  }

  if (!w && !h) {
    return;
  }

  if (!h) {
    CapStyle cap = CAP_BUTT;
    if (state.lineJoin == JOIN_ROUND) {
      cap = CAP_ROUND;
    }
    AdjustedTarget(this, bounds.IsEmpty() ? nullptr : &bounds)->
      StrokeLine(Point(x, y), Point(x + w, y),
                  CanvasGeneralPattern().ForStyle(this, STYLE_STROKE, mTarget),
                  StrokeOptions(state.lineWidth, state.lineJoin,
                                cap, state.miterLimit,
                                state.dash.Length(),
                                state.dash.Elements(),
                                state.dashOffset),
                  DrawOptions(state.globalAlpha, UsedOperation()));
    return;
  }

  if (!w) {
    CapStyle cap = CAP_BUTT;
    if (state.lineJoin == JOIN_ROUND) {
      cap = CAP_ROUND;
    }
    AdjustedTarget(this, bounds.IsEmpty() ? nullptr : &bounds)->
      StrokeLine(Point(x, y), Point(x, y + h),
                  CanvasGeneralPattern().ForStyle(this, STYLE_STROKE, mTarget),
                  StrokeOptions(state.lineWidth, state.lineJoin,
                                cap, state.miterLimit,
                                state.dash.Length(),
                                state.dash.Elements(),
                                state.dashOffset),
                  DrawOptions(state.globalAlpha, UsedOperation()));
    return;
  }

  AdjustedTarget(this, bounds.IsEmpty() ? nullptr : &bounds)->
    StrokeRect(mgfx::Rect(x, y, w, h),
                CanvasGeneralPattern().ForStyle(this, STYLE_STROKE, mTarget),
                StrokeOptions(state.lineWidth, state.lineJoin,
                              state.lineCap, state.miterLimit,
                              state.dash.Length(),
                              state.dash.Elements(),
                              state.dashOffset),
                DrawOptions(state.globalAlpha, UsedOperation()));

  Redraw();
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::StrokeRect(float x, float y, float w, float h)
{
  StrokeRect((double)x, (double)y, (double)w, (double)h);
  return NS_OK;
}





void
nsCanvasRenderingContext2DAzure::BeginPath()
{
  mPath = nullptr;
  mPathBuilder = nullptr;
  mDSPathBuilder = nullptr;
  mPathTransformWillUpdate = false;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MozBeginPath()
{
  BeginPath();
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MozClosePath()
{
  ClosePath();
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::Fill()
{
  EnsureUserSpacePath();

  if (!mPath) {
    return;
  }

  mgfx::Rect bounds;

  if (NeedToDrawShadow()) {
    bounds = mPath->GetBounds(mTarget->GetTransform());
  }

  AdjustedTarget(this, bounds.IsEmpty() ? nullptr : &bounds)->
    Fill(mPath, CanvasGeneralPattern().ForStyle(this, STYLE_FILL, mTarget),
         DrawOptions(CurrentState().globalAlpha, UsedOperation()));

  Redraw();
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MozFill()
{
  Fill();
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::Stroke()
{
  EnsureUserSpacePath();

  if (!mPath) {
    return;
  }

  const ContextState &state = CurrentState();

  StrokeOptions strokeOptions(state.lineWidth, state.lineJoin,
                              state.lineCap, state.miterLimit,
                              state.dash.Length(), state.dash.Elements(),
                              state.dashOffset);

  mgfx::Rect bounds;
  if (NeedToDrawShadow()) {
    bounds =
      mPath->GetStrokedBounds(strokeOptions, mTarget->GetTransform());
  }

  AdjustedTarget(this, bounds.IsEmpty() ? nullptr : &bounds)->
    Stroke(mPath, CanvasGeneralPattern().ForStyle(this, STYLE_STROKE, mTarget),
           strokeOptions, DrawOptions(state.globalAlpha, UsedOperation()));

  Redraw();
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MozStroke()
{
  Stroke();
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::Clip()
{
  EnsureUserSpacePath();

  if (!mPath) {
    return;
  }

  mTarget->PushClip(mPath);
  CurrentState().clipsPushed.push_back(mPath);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MozClip()
{
  Clip();
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MoveTo(float x, float y)
{
  MoveTo((double)x, (double)y);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::LineTo(float x, float y)
{
  LineTo((double)x, (double)y);
  return NS_OK;
}
  
NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::QuadraticCurveTo(float cpx, float cpy, float x,
                                                  float y)
{
  QuadraticCurveTo((double)cpx, (double)cpy, (double)x, (double)y);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::BezierCurveTo(float cp1x, float cp1y,
                                               float cp2x, float cp2y,
                                               float x, float y)
{
  BezierCurveTo((double)cp1x, (double)cp1y, (double)cp2x, (double)cp2y,
                (double)x, (double)y);
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::ArcTo(double x1, double y1, double x2,
                                       double y2, double radius,
                                       ErrorResult& error)
{
  if (!FloatValidate(x1, y1, x2, y2, radius)) {
    return;
  }

  if (radius < 0) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  EnsureWritablePath();

  
  Point p0;
  if (mPathBuilder) {
    p0 = mPathBuilder->CurrentPoint();
  } else {
    Matrix invTransform = mTarget->GetTransform();
    if (!invTransform.Invert()) {
      return;
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
    return;
  }

  
  dir = (p2.x - p1.x) * (p0.y - p1.y) + (p2.y - p1.y) * (p1.x - p0.x);
  if (dir == 0) {
    LineTo(p1.x, p1.y);
    return;
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
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::ArcTo(float x1, float y1, float x2, float y2, float radius)
{
  ErrorResult rv;
  ArcTo(x1, y1, x2, y2, radius, rv);
  return rv.ErrorCode();
}

void
nsCanvasRenderingContext2DAzure::Arc(double x, double y, double r,
                                     double startAngle, double endAngle,
                                     bool anticlockwise, ErrorResult& error)
{
  if (!FloatValidate(x, y, r, startAngle, endAngle)) {
    return;
  }

  if (r < 0.0) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  EnsureWritablePath();

  ArcToBezier(this, Point(x, y), r, startAngle, endAngle, anticlockwise);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Arc(float x, float y,
                                     float r,
                                     float startAngle, float endAngle,
                                     bool ccw)
{
  ErrorResult rv;
  Arc(x, y, r, startAngle, endAngle, ccw, rv);
  return rv.ErrorCode();
}

void
nsCanvasRenderingContext2DAzure::Rect(double x, double y, double w, double h)
{
  if (!FloatValidate(x, y, w, h)) {
    return;
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
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::Rect(float x, float y, float w, float h)
{
  Rect((double)x, (double)y, (double)w, (double)h);
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
      mPath = nullptr;
      mPathBuilder = nullptr;
      mPathTransformWillUpdate = false;
    }
    return;
  }

  if (!mPath) {
    NS_ASSERTION(!mPathTransformWillUpdate, "mPathTransformWillUpdate should be false, if all paths are null");
    mPathBuilder = mTarget->CreatePathBuilder(fillRule);
  } else if (!mPathTransformWillUpdate) {
    mPathBuilder = mPath->CopyToBuilder(fillRule);
  } else {
    mDSPathBuilder =
      mPath->TransformedCopyToBuilder(mPathToDS, fillRule);
    mPathTransformWillUpdate = false;
  }
}

void
nsCanvasRenderingContext2DAzure::EnsureUserSpacePath(bool aCommitTransform )
{
  FillRule fillRule = CurrentState().fillRule;

  if (!mPath && !mPathBuilder && !mDSPathBuilder) {
    mPathBuilder = mTarget->CreatePathBuilder(fillRule);
  }

  if (mPathBuilder) {
    mPath = mPathBuilder->Finish();
    mPathBuilder = nullptr;
  }

  if (aCommitTransform &&
      mPath &&
      mPathTransformWillUpdate) {
    mDSPathBuilder =
      mPath->TransformedCopyToBuilder(mPathToDS, fillRule);
    mPath = nullptr;
    mPathTransformWillUpdate = false;
  }

  if (mDSPathBuilder) {
    RefPtr<Path> dsPath;
    dsPath = mDSPathBuilder->Finish();
    mDSPathBuilder = nullptr;

    Matrix inverse = mTarget->GetTransform();
    if (!inverse.Invert()) {
      NS_WARNING("Could not invert transform");
      return;
    }

    mPathBuilder =
      dsPath->TransformedCopyToBuilder(inverse, fillRule);
    mPath = mPathBuilder->Finish();
    mPathBuilder = nullptr;
  }

  if (mPath && mPath->GetFillRule() != fillRule) {
    mPathBuilder = mPath->CopyToBuilder(fillRule);
    mPath = mPathBuilder->Finish();
  }

  NS_ASSERTION(mPath, "mPath should exist");
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
  bool changed;

  nsIPrincipal* principal = aNode->NodePrincipal();
  nsIDocument* document = aNode->OwnerDoc();

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
                            false);
  if (NS_FAILED(rv))
    return rv;

  rv = parser.ParseProperty(eCSSProperty_line_height,
                            NS_LITERAL_STRING("normal"), docURL, baseURL,
                            principal, rule->GetDeclaration(), &changed,
                            false);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rule->RuleMatched();

  rule.forget(aResult);
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::SetFont(const nsAString& font,
                                         ErrorResult& error)
{
  







  if (!mCanvasElement && !mDocShell) {
    NS_WARNING("Canvas element must be non-null or a docshell must be provided");
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  nsIPresShell* presShell = GetPresShell();
  if (!presShell) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }
  nsIDocument* document = presShell->GetDocument();

  nsCOMArray<nsIStyleRule> rules;

  nsRefPtr<css::StyleRule> rule;
  error = CreateFontStyleRule(font, document, getter_AddRefs(rule));

  if (error.Failed()) {
    return;
  }

  css::Declaration *declaration = rule->GetDeclaration();
  
  
  
  
  
  
  const nsCSSValue *fsaVal =
    declaration->GetNormalBlock()->ValueFor(eCSSProperty_font_size_adjust);
  if (!fsaVal || (fsaVal->GetUnit() != eCSSUnit_None &&
                  fsaVal->GetUnit() != eCSSUnit_System_Font)) {
      
      
    return;
  }

  rules.AppendObject(rule);

  nsStyleSet* styleSet = presShell->StyleSet();

  
  
  nsRefPtr<nsStyleContext> parentContext;

  if (mCanvasElement && mCanvasElement->IsInDoc()) {
      
      parentContext = nsComputedDOMStyle::GetStyleContextForElement(
              mCanvasElement,
              nullptr,
              presShell);
  } else {
    
    nsRefPtr<css::StyleRule> parentRule;
    error = CreateFontStyleRule(NS_LITERAL_STRING("10px sans-serif"),
                                document,
                                getter_AddRefs(parentRule));

    if (error.Failed()) {
      return;
    }

    nsCOMArray<nsIStyleRule> parentRules;
    parentRules.AppendObject(parentRule);
    parentContext = styleSet->ResolveStyleForRules(nullptr, parentRules);
  }

  if (!parentContext) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  nsRefPtr<nsStyleContext> sc =
      styleSet->ResolveStyleForRules(parentContext, rules);
  if (!sc) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  const nsStyleFont* fontStyle = sc->GetStyleFont();

  NS_ASSERTION(fontStyle, "Could not obtain font style");

  nsIAtom* language = sc->GetStyleFont()->mLanguage;
  if (!language) {
    language = presShell->GetPresContext()->GetLanguageFromCharset();
  }

  
  const uint32_t aupcp = nsPresContext::AppUnitsPerCSSPixel();
  
  
  
  
  
  
  const nscoord fontSize = nsStyleFont::UnZoomText(parentContext->PresContext(), fontStyle->mSize);

  bool printerFont = (presShell->GetPresContext()->Type() == nsPresContext::eContext_PrintPreview ||
                        presShell->GetPresContext()->Type() == nsPresContext::eContext_Print);

  gfxFontStyle style(fontStyle->mFont.style,
                      fontStyle->mFont.weight,
                      fontStyle->mFont.stretch,
                      NSAppUnitsToFloatPixels(fontSize, float(aupcp)),
                      language,
                      fontStyle->mFont.sizeAdjust,
                      fontStyle->mFont.systemFont,
                      printerFont,
                      fontStyle->mFont.languageOverride);

  fontStyle->mFont.AddFontFeaturesToStyle(&style);

  CurrentState().fontGroup =
      gfxPlatform::GetPlatform()->CreateFontGroup(fontStyle->mFont.name,
                                                  &style,
                                                  presShell->GetPresContext()->GetUserFontSet());
  NS_ASSERTION(CurrentState().fontGroup, "Could not get font group");

  
  
  
  
  declaration->GetValue(eCSSProperty_font, CurrentState().font);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozFont(const nsAString& font)
{
  ErrorResult rv;
  SetFont(font, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozFont(nsAString& font)
{
  font = GetFont();
  return NS_OK;
}

void
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
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozTextAlign(const nsAString& ta)
{
  SetTextAlign(ta);
  return NS_OK;
}

void
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
  }
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozTextAlign(nsAString& ta)
{
  GetTextAlign(ta);
  return NS_OK;
}

void
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
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozTextBaseline(const nsAString& tb)
{
  SetTextBaseline(tb);
  return NS_OK;
}

void
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
  }
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozTextBaseline(nsAString& tb)
{
  GetTextBaseline(tb);
  return NS_OK;
}








static inline void
TextReplaceWhitespaceCharacters(nsAutoString& str)
{
  str.ReplaceChar("\x09\x0A\x0B\x0C\x0D", PRUnichar(' '));
}

void
nsCanvasRenderingContext2DAzure::FillText(const nsAString& text, double x,
                                          double y,
                                          const Optional<double>& maxWidth,
                                          ErrorResult& error)
{
  error = DrawOrMeasureText(text, x, y, maxWidth, TEXT_DRAW_OPERATION_FILL, nullptr);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::FillText(const nsAString& text, float x, float y, float maxWidth)
{
  ErrorResult rv;
  Optional<double> optionalMaxWidth;
  optionalMaxWidth.Construct();
  optionalMaxWidth.Value() = maxWidth;
  FillText(text, x, y, optionalMaxWidth, rv);
  return rv.ErrorCode();
}

void
nsCanvasRenderingContext2DAzure::StrokeText(const nsAString& text, double x,
                                            double y,
                                            const Optional<double>& maxWidth,
                                            ErrorResult& error)
{
  error = DrawOrMeasureText(text, x, y, maxWidth, TEXT_DRAW_OPERATION_STROKE, nullptr);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::StrokeText(const nsAString& text, float x, float y, float maxWidth)
{
  ErrorResult rv;
  Optional<double> optionalMaxWidth;
  optionalMaxWidth.Construct();
  optionalMaxWidth.Value() = maxWidth;
  StrokeText(text, x, y, optionalMaxWidth, rv);
  return rv.ErrorCode();
}

already_AddRefed<nsIDOMTextMetrics>
nsCanvasRenderingContext2DAzure::MeasureText(const nsAString& rawText,
                                             ErrorResult& error)
{
  float width;
  Optional<double> maxWidth;
  error = DrawOrMeasureText(rawText, 0, 0, maxWidth, TEXT_DRAW_OPERATION_MEASURE, &width);
  if (error.Failed()) {
    return NULL;
  }

  nsRefPtr<nsIDOMTextMetrics> textMetrics = new nsTextMetricsAzure(width);

  return textMetrics.forget();
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::MeasureText(const nsAString& rawText,
                                             nsIDOMTextMetrics** _retval)
{
  ErrorResult rv;
  *_retval = MeasureText(rawText, rv).get();
  return rv.ErrorCode();
}




struct NS_STACK_CLASS nsCanvasBidiProcessorAzure : public nsBidiPresUtils::BidiProcessor
{
  typedef nsCanvasRenderingContext2DAzure::ContextState ContextState;

  virtual void SetText(const PRUnichar* text, int32_t length, nsBidiDirection direction)
  {
    mFontgrp->UpdateFontList(); 
    mTextRun = mFontgrp->MakeTextRun(text,
                                     length,
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
                                                               nullptr);

    
    
    if (mDoMeasureBoundingBox) {
      textRunMetrics.mBoundingBox.Scale(1.0 / mAppUnitsPerDevPixel);
      mBoundingBox = mBoundingBox.Union(textRunMetrics.mBoundingBox);
    }

    return NSToCoordRound(textRunMetrics.mAdvanceWidth);
  }

  virtual void DrawText(nscoord xOffset, nscoord width)
  {
    gfxPoint point = mPt;
    point.x += xOffset;

    
    if (mTextRun->IsRightToLeft()) {
      
      
      
      
      
      gfxTextRun::Metrics textRunMetrics =
        mTextRun->MeasureText(0,
                              mTextRun->GetLength(),
                              mDoMeasureBoundingBox ?
                                  gfxFont::TIGHT_INK_EXTENTS :
                                  gfxFont::LOOSE_INK_EXTENTS,
                              mThebes,
                              nullptr);
      point.x += textRunMetrics.mAdvanceWidth;
      
      
      
      
    }

    uint32_t numRuns;
    const gfxTextRun::GlyphRun *runs = mTextRun->GetGlyphRuns(&numRuns);
    const uint32_t appUnitsPerDevUnit = mAppUnitsPerDevPixel;
    const double devUnitsPerAppUnit = 1.0/double(appUnitsPerDevUnit);
    Point baselineOrigin =
      Point(point.x * devUnitsPerAppUnit, point.y * devUnitsPerAppUnit);

    float advanceSum = 0;

    for (uint32_t c = 0; c < numRuns; c++) {
      gfxFont *font = runs[c].mFont;
      uint32_t endRun = 0;
      if (c + 1 < numRuns) {
        endRun = runs[c + 1].mCharacterOffset;
      } else {
        endRun = mTextRun->GetLength();
      }

      const gfxTextRun::CompressedGlyph *glyphs = mTextRun->GetCharacterGlyphs();

      RefPtr<ScaledFont> scaledFont =
        gfxPlatform::GetPlatform()->GetScaledFontForFont(mCtx->mTarget, font);

      if (!scaledFont) {
        
        return;
      }

      GlyphBuffer buffer;

      std::vector<Glyph> glyphBuf;

      for (uint32_t i = runs[c].mCharacterOffset; i < endRun; i++) {
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

        for (uint32_t c = 0; c < glyphs[i].GetGlyphCount(); c++) {
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

      Rect bounds(mBoundingBox.x, mBoundingBox.y, mBoundingBox.width, mBoundingBox.height);
      if (mOp == nsCanvasRenderingContext2DAzure::TEXT_DRAW_OPERATION_FILL) {
        AdjustedTarget(mCtx, &bounds)->
          FillGlyphs(scaledFont, buffer,
                     CanvasGeneralPattern().
                       ForStyle(mCtx, nsCanvasRenderingContext2DAzure::STYLE_FILL, mCtx->mTarget),
                     DrawOptions(mState->globalAlpha, mCtx->UsedOperation()));
      } else if (mOp == nsCanvasRenderingContext2DAzure::TEXT_DRAW_OPERATION_STROKE) {
        RefPtr<Path> path = scaledFont->GetPathForGlyphs(buffer, mCtx->mTarget);

        const ContextState& state = *mState;
        AdjustedTarget(mCtx, &bounds)->
          Stroke(path, CanvasGeneralPattern().
                   ForStyle(mCtx, nsCanvasRenderingContext2DAzure::STYLE_STROKE, mCtx->mTarget),
                 StrokeOptions(state.lineWidth, state.lineJoin,
                               state.lineCap, state.miterLimit,
                               state.dash.Length(),
                               state.dash.Elements(),
                               state.dashOffset),
                 DrawOptions(state.globalAlpha, mCtx->UsedOperation()));

      }
    }
  }

  
  nsAutoPtr<gfxTextRun> mTextRun;

  
  nsRefPtr<gfxContext> mThebes;

  
  nsCanvasRenderingContext2DAzure *mCtx;

  
  gfxPoint mPt;

  
  gfxFontGroup* mFontgrp;

  
  uint32_t mAppUnitsPerDevPixel;

  
  nsCanvasRenderingContext2DAzure::TextDrawOperation mOp;

  
  ContextState *mState;

  
  gfxRect mBoundingBox;

  
  bool mDoMeasureBoundingBox;
};

nsresult
nsCanvasRenderingContext2DAzure::DrawOrMeasureText(const nsAString& aRawText,
                                                   float aX,
                                                   float aY,
                                                   const Optional<double>& aMaxWidth,
                                                   TextDrawOperation aOp,
                                                   float* aWidth)
{
  nsresult rv;

  if (!FloatValidate(aX, aY) ||
      (aMaxWidth.WasPassed() && !FloatValidate(aMaxWidth.Value())))
      return NS_ERROR_DOM_SYNTAX_ERR;

  
  
  
  
  if (aMaxWidth.WasPassed() && aMaxWidth.Value() < 0)
    return NS_ERROR_INVALID_ARG;

  if (!mCanvasElement && !mDocShell) {
    NS_WARNING("Canvas element must be non-null or a docshell must be provided");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  if (!presShell)
    return NS_ERROR_FAILURE;

  nsIDocument* document = presShell->GetDocument();

  
  nsAutoString textToDraw(aRawText);
  TextReplaceWhitespaceCharacters(textToDraw);

  
  bool isRTL = false;

  if (mCanvasElement && mCanvasElement->IsInDoc()) {
    
    nsRefPtr<nsStyleContext> canvasStyle =
      nsComputedDOMStyle::GetStyleContextForElement(mCanvasElement,
                                                    nullptr,
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

  
  bool doDrawShadow = aOp == TEXT_DRAW_OPERATION_FILL && NeedToDrawShadow();

  nsCanvasBidiProcessorAzure processor;

  GetAppUnitsValues(&processor.mAppUnitsPerDevPixel, nullptr);
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

  nscoord totalWidthCoord;

  
  
  nsBidi bidiEngine;
  rv = nsBidiPresUtils::ProcessText(textToDraw.get(),
                                    textToDraw.Length(),
                                    isRTL ? NSBIDI_RTL : NSBIDI_LTR,
                                    presShell->GetPresContext(),
                                    processor,
                                    nsBidiPresUtils::MODE_MEASURE,
                                    nullptr,
                                    0,
                                    &totalWidthCoord,
                                    &bidiEngine);
  if (NS_FAILED(rv)) {
    return rv;
  }

  float totalWidth = float(totalWidthCoord) / processor.mAppUnitsPerDevPixel;
  if (aWidth) {
    *aWidth = totalWidth;
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

  
  processor.mFontgrp->UpdateFontList(); 
  NS_ASSERTION(processor.mFontgrp->FontListLength()>0, "font group contains no fonts");
  const gfxFont::Metrics& fontMetrics = processor.mFontgrp->GetFontAt(0)->GetMetrics();

  gfxFloat anchorY;

  switch (state.textBaseline)
  {
  case TEXT_BASELINE_HANGING:
      
  case TEXT_BASELINE_TOP:
    anchorY = fontMetrics.emAscent;
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
      MOZ_NOT_REACHED("unexpected TextBaseline");
  }

  processor.mPt.y += anchorY;

  
  processor.mBoundingBox.width = totalWidth;
  processor.mBoundingBox.MoveBy(processor.mPt);

  processor.mPt.x *= processor.mAppUnitsPerDevPixel;
  processor.mPt.y *= processor.mAppUnitsPerDevPixel;

  Matrix oldTransform = mTarget->GetTransform();
  
  
  if (aMaxWidth.WasPassed() && aMaxWidth.Value() > 0 &&
      totalWidth > aMaxWidth.Value()) {
    Matrix newTransform = oldTransform;

    
    
    newTransform.Translate(aX, 0);
    newTransform.Scale(aMaxWidth.Value() / totalWidth, 1);
    newTransform.Translate(-aX, 0);
    
    Matrix androidCompilerBug = newTransform;
    mTarget->SetTransform(androidCompilerBug);
  }

  
  gfxRect boundingBox = processor.mBoundingBox;

  
  processor.mDoMeasureBoundingBox = false;

  rv = nsBidiPresUtils::ProcessText(textToDraw.get(),
                                    textToDraw.Length(),
                                    isRTL ? NSBIDI_RTL : NSBIDI_LTR,
                                    presShell->GetPresContext(),
                                    processor,
                                    nsBidiPresUtils::MODE_DRAW,
                                    nullptr,
                                    0,
                                    nullptr,
                                    &bidiEngine);


  mTarget->SetTransform(oldTransform);

  if (aOp == nsCanvasRenderingContext2DAzure::TEXT_DRAW_OPERATION_FILL &&
      !doDrawShadow) {
    RedrawUser(boundingBox);
    return NS_OK;
  }

  Redraw();
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetTextStyle(const nsAString& textStyle)
{
  ErrorResult rv;
  SetMozTextStyle(textStyle, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetTextStyle(nsAString& textStyle)
{
  GetMozTextStyle(textStyle);
  return NS_OK;
}

gfxFontGroup *nsCanvasRenderingContext2DAzure::GetCurrentFontStyle()
{
  
  if (!CurrentState().fontGroup) {
    nsresult rv = SetMozFont(kDefaultFontStyle);
    if (NS_FAILED(rv)) {
      gfxFontStyle style;
      style.size = kDefaultFontSize;
      CurrentState().fontGroup =
        gfxPlatform::GetPlatform()->CreateFontGroup(kDefaultFontName,
                                                    &style,
                                                    nullptr);
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
nsCanvasRenderingContext2DAzure::SetLineWidth(float width)
{
  SetLineWidth((double)width);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetLineWidth(float *width)
{
  *width = GetLineWidth();
  return NS_OK;
}

void
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
    
    return;
  }

  CurrentState().lineCap = cap;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozLineCap(const nsAString& capstyle)
{
  SetLineCap(capstyle);
  return NS_OK;
}

void
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
  }
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozLineCap(nsAString& capstyle)
{
  GetLineCap(capstyle);
  return NS_OK;
}

void
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
    
    return;
  }

  CurrentState().lineJoin = j;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozLineJoin(const nsAString& joinstyle)
{
  SetLineJoin(joinstyle);
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::GetLineJoin(nsAString& joinstyle, ErrorResult& error)
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
    error.Throw(NS_ERROR_FAILURE);
  }
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozLineJoin(nsAString& joinstyle)
{
  ErrorResult rv;
  nsString linejoin;
  GetLineJoin(linejoin, rv);
  if (!rv.Failed()) {
    joinstyle = linejoin;
  }
  return rv.ErrorCode();
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMiterLimit(float miter)
{
  SetMiterLimit((double)miter);
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMiterLimit(float *miter)
{
  *miter = GetMiterLimit();
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::SetMozDash(JSContext* cx,
                                            const JS::Value& mozDash,
                                            ErrorResult& error)
{
  FallibleTArray<Float> dash;
  error = JSValToDashArray(cx, mozDash, dash);
  if (!error.Failed()) {
    ContextState& state = CurrentState();
    state.dash = dash;
    if (state.dash.IsEmpty()) {
      state.dashOffset = 0;
    }
  }
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozDash(JSContext *cx, const jsval& patternArray)
{
  ErrorResult rv;
  SetMozDash(cx, patternArray, rv);
  return rv.ErrorCode();
}

JS::Value
nsCanvasRenderingContext2DAzure::GetMozDash(JSContext* cx, ErrorResult& error)
{
  JS::Value mozDash;
  error = DashArrayToJSVal(CurrentState().dash, cx, &mozDash);
  return mozDash;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozDash(JSContext* cx, jsval* dashArray)
{
  ErrorResult rv;
  *dashArray = GetMozDash(cx, rv);
  return rv.ErrorCode();
}
 
NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozDashOffset(float offset)
{
  if (!FloatValidate(offset)) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  ContextState& state = CurrentState();
  if (!state.dash.IsEmpty()) {
    state.dashOffset = offset;
  }
  return NS_OK;
}
 
NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozDashOffset(float* offset)
{
  *offset = GetMozDashOffset();
  return NS_OK;
}

bool
nsCanvasRenderingContext2DAzure::IsPointInPath(double x, double y)
{
  if (!FloatValidate(x,y)) {
    return false;
  }

  EnsureUserSpacePath(false);
  if (!mPath) {
    return false;
  }
  if (mPathTransformWillUpdate) {
    return mPath->ContainsPoint(Point(x, y), mPathToDS);
  }
  return mPath->ContainsPoint(Point(x, y), mTarget->GetTransform());
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::IsPointInPath(float x, float y, bool *retVal)
{
  *retVal = IsPointInPath(x, y);
  return NS_OK;
}

















void
nsCanvasRenderingContext2DAzure::DrawImage(const HTMLImageOrCanvasOrVideoElement& image,
                                           double sx, double sy, double sw,
                                           double sh, double dx, double dy,
                                           double dw, double dh, 
                                           uint8_t optional_argc,
                                           ErrorResult& error)
{
  MOZ_ASSERT(optional_argc == 0 || optional_argc == 2 || optional_argc == 6);

  RefPtr<SourceSurface> srcSurf;
  gfxIntSize imgSize;

  Element* element;
  if (image.IsHTMLCanvasElement()) {
    nsHTMLCanvasElement* canvas = image.GetAsHTMLCanvasElement();
    element = canvas;
    nsIntSize size = canvas->GetSize();
    if (size.width == 0 || size.height == 0) {
      error.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
      return;
    }

    
    nsICanvasRenderingContextInternal *srcCanvas = canvas->GetContextAtIndex(0);
    if (srcCanvas == this) {
      
      srcSurf = mTarget->Snapshot();
      imgSize = gfxIntSize(mWidth, mHeight);
    } else if (srcCanvas) {
      
      srcSurf = srcCanvas->GetSurfaceSnapshot();

      if (srcSurf) {
        if (mCanvasElement) {
          
          CanvasUtils::DoDrawImageSecurityCheck(mCanvasElement,
                                                element->NodePrincipal(),
                                               canvas->IsWriteOnly(),
                                                false);
        }
        imgSize = gfxIntSize(srcSurf->GetSize().width, srcSurf->GetSize().height);
      }
    }
  } else {
    if (image.IsHTMLImageElement()) {
      nsHTMLImageElement* img = image.GetAsHTMLImageElement();
      element = img;
    } else {
      nsHTMLVideoElement* video = image.GetAsHTMLVideoElement();
      element = video;
    }

    gfxASurface* imgsurf =
      CanvasImageCache::Lookup(element, mCanvasElement, &imgSize);
    if (imgsurf) {
      srcSurf = gfxPlatform::GetPlatform()->GetSourceSurfaceForSurface(mTarget, imgsurf);
    }
  }

  if (!srcSurf) {
    
    
    uint32_t sfeFlags = nsLayoutUtils::SFE_WANT_FIRST_FRAME;
    nsLayoutUtils::SurfaceFromElementResult res =
      nsLayoutUtils::SurfaceFromElement(element, sfeFlags);

    if (!res.mSurface) {
      
      if (!res.mIsStillLoading) {
        error.Throw(NS_ERROR_NOT_AVAILABLE);
      }
      return;
    }

    
    if (res.mSurface->CairoStatus()) {
      return;
    }

    imgSize = res.mSize;

    if (mCanvasElement) {
      CanvasUtils::DoDrawImageSecurityCheck(mCanvasElement,
                                            res.mPrincipal, res.mIsWriteOnly,
                                            res.mCORSUsed);
    }

    if (res.mImageRequest) {
      CanvasImageCache::NotifyDrawImage(element, mCanvasElement,
                                        res.mImageRequest, res.mSurface, imgSize);
    }

    srcSurf = gfxPlatform::GetPlatform()->GetSourceSurfaceForSurface(mTarget, res.mSurface);
  }

  if (optional_argc == 0) {
    sx = sy = 0.0;
    dw = sw = (double) imgSize.width;
    dh = sh = (double) imgSize.height;
  } else if (optional_argc == 2) {
    sx = sy = 0.0;
    sw = (double) imgSize.width;
    sh = (double) imgSize.height;
  }

  if (dw == 0.0 || dh == 0.0) {
    
    
    return;
  }

  if (sx < 0.0 || sy < 0.0 ||
      sw < 0.0 || sw > (double) imgSize.width ||
      sh < 0.0 || sh > (double) imgSize.height ||
      dw < 0.0 || dh < 0.0) {
    
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  Filter filter;

  if (CurrentState().imageSmoothingEnabled)
    filter = mgfx::FILTER_LINEAR;
  else
    filter = mgfx::FILTER_POINT;

  mgfx::Rect bounds;
  
  if (NeedToDrawShadow()) {
    bounds = mgfx::Rect(dx, dy, dw, dh);
    bounds = mTarget->GetTransform().TransformBounds(bounds);
  }

  AdjustedTarget(this, bounds.IsEmpty() ? nullptr : &bounds)->
    DrawSurface(srcSurf,
                mgfx::Rect(dx, dy, dw, dh),
                mgfx::Rect(sx, sy, sw, sh),
                DrawSurfaceOptions(filter),
                DrawOptions(CurrentState().globalAlpha, UsedOperation()));

  RedrawUser(gfxRect(dx, dy, dw, dh));
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::DrawImage(nsIDOMElement *imgElt, float a1,
                                           float a2, float a3, float a4, float a5,
                                           float a6, float a7, float a8,
                                           uint8_t optional_argc)
{
  if (!(optional_argc == 0 || optional_argc == 2 || optional_argc == 6)) {
    return NS_ERROR_INVALID_ARG;
  }

  HTMLImageOrCanvasOrVideoElement element;
  if (!ToHTMLImageOrCanvasOrVideoElement(imgElt, element)) {
    return NS_ERROR_DOM_TYPE_MISMATCH_ERR;
  }

  ErrorResult rv;
  if (optional_argc == 0) {
    if (!FloatValidate(a1, a2)) {
      return NS_OK;
    }
    DrawImage(element, 0, 0, 0, 0, a1, a2, 0, 0, 0, rv);
  } else if (optional_argc == 2) {
    if (!FloatValidate(a1, a2, a3, a4)) {
      return NS_OK;
    }
    DrawImage(element, 0, 0, 0, 0, a1, a2, a3, a4, 2, rv);
  } else if (optional_argc == 6) {
    if (!FloatValidate(a1, a2, a3, a4) || !FloatValidate(a5, a6, a7, a8)) {
      return NS_OK;
    }
    DrawImage(element, a1, a2, a3, a4, a5, a6, a7, a8, 6, rv);
  }
  return rv.ErrorCode();
}

void
nsCanvasRenderingContext2DAzure::SetGlobalCompositeOperation(const nsAString& op,
                                                             ErrorResult& error)
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
  
  else return;

#undef CANVAS_OP_TO_GFX_OP
  CurrentState().op = comp_op;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetGlobalCompositeOperation(const nsAString& op)
{
  ErrorResult rv;
  SetGlobalCompositeOperation(op, rv);
  return rv.ErrorCode();
}

void
nsCanvasRenderingContext2DAzure::GetGlobalCompositeOperation(nsAString& op,
                                                             ErrorResult& error)
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
  else {
    error.Throw(NS_ERROR_FAILURE);
  }

#undef CANVAS_OP_TO_GFX_OP
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetGlobalCompositeOperation(nsAString& op)
{
  nsString globalCompositeOperation;
  ErrorResult rv;
  GetGlobalCompositeOperation(globalCompositeOperation, rv);
  if (!rv.Failed()) {
    op = globalCompositeOperation;
  }
  return rv.ErrorCode();
}

void
nsCanvasRenderingContext2DAzure::DrawWindow(nsIDOMWindow* window, double x,
                                            double y, double w, double h,
                                            const nsAString& bgColor,
                                            uint32_t flags, ErrorResult& error)
{
  
  
  if (!gfxASurface::CheckSurfaceSize(gfxIntSize(int32_t(w), int32_t(h)),
                                     0xffff)) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  nsRefPtr<gfxASurface> drawSurf;
  GetThebesSurface(getter_AddRefs(drawSurf));

  nsRefPtr<gfxContext> thebes = new gfxContext(drawSurf);

  Matrix matrix = mTarget->GetTransform();
  thebes->SetMatrix(gfxMatrix(matrix._11, matrix._12, matrix._21,
                              matrix._22, matrix._31, matrix._32));

  
  
  
  
  
  
  if (!nsContentUtils::IsCallerTrustedForRead()) {
    
    
    error.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

  
  if (!(flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DO_NOT_FLUSH))
    nsContentUtils::FlushLayoutForTree(window);

  nsRefPtr<nsPresContext> presContext;
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(window);
  if (win) {
    nsIDocShell* docshell = win->GetDocShell();
    if (docshell) {
      docshell->GetPresContext(getter_AddRefs(presContext));
    }
  }
  if (!presContext) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  nscolor backgroundColor;
  if (!ParseColor(bgColor, &backgroundColor)) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  nsRect r(nsPresContext::CSSPixelsToAppUnits((float)x),
           nsPresContext::CSSPixelsToAppUnits((float)y),
           nsPresContext::CSSPixelsToAppUnits((float)w),
           nsPresContext::CSSPixelsToAppUnits((float)h));
  uint32_t renderDocFlags = (nsIPresShell::RENDER_IGNORE_VIEWPORT_SCROLLING |
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

  unused << presContext->PresShell()->
    RenderDocument(r, renderDocFlags, backgroundColor, thebes);

  
  
  
  RedrawUser(gfxRect(0, 0, w, h));
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::DrawWindow(nsIDOMWindow* aWindow, float aX, float aY,
                                            float aW, float aH,
                                            const nsAString& aBGColor,
                                            uint32_t flags)
{
  NS_ENSURE_ARG(aWindow);

  ErrorResult rv;
  DrawWindow(aWindow, aX, aY, aW, aH, aBGColor, flags, rv);
  return rv.ErrorCode();
}

void
nsCanvasRenderingContext2DAzure::AsyncDrawXULElement(nsIDOMXULElement* elem,
                                                     double x, double y,
                                                     double w, double h,
                                                     const nsAString& bgColor,
                                                     uint32_t flags,
                                                     ErrorResult& error)
{
  
  
  
  
  
  
  if (!nsContentUtils::IsCallerTrustedForRead()) {
      
      
    error.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

#if 0
  nsCOMPtr<nsIFrameLoaderOwner> loaderOwner = do_QueryInterface(elem);
  if (!loaderOwner) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  nsRefPtr<nsFrameLoader> frameloader = loaderOwner->GetFrameLoader();
  if (!frameloader) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  PBrowserParent *child = frameloader->GetRemoteBrowser();
  if (!child) {
    nsCOMPtr<nsIDOMWindow> window =
      do_GetInterface(frameloader->GetExistingDocShell());
    if (!window) {
      error.Throw(NS_ERROR_FAILURE);
      return;
    }

    return DrawWindow(window, x, y, w, h, bgColor, flags);
  }

  
  
  if (!gfxASurface::CheckSurfaceSize(gfxIntSize(w, h), 0xffff)) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  bool flush =
    (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DO_NOT_FLUSH) == 0;

  uint32_t renderDocFlags = nsIPresShell::RENDER_IGNORE_VIEWPORT_SCROLLING;
  if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DRAW_CARET) {
    renderDocFlags |= nsIPresShell::RENDER_CARET;
  }
  if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DRAW_VIEW) {
    renderDocFlags &= ~nsIPresShell::RENDER_IGNORE_VIEWPORT_SCROLLING;
  }

  nsRect rect(nsPresContext::CSSPixelsToAppUnits(x),
              nsPresContext::CSSPixelsToAppUnits(y),
              nsPresContext::CSSPixelsToAppUnits(w),
              nsPresContext::CSSPixelsToAppUnits(h));
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
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::AsyncDrawXULElement(nsIDOMXULElement* aElem,
                                                     float aX, float aY,
                                                     float aW, float aH,
                                                     const nsAString& aBGColor,
                                                     uint32_t flags)
{
  NS_ENSURE_ARG(aElem);

  ErrorResult rv;
  AsyncDrawXULElement(aElem, aX, aY, aW, aH, aBGColor, flags, rv);
  return rv.ErrorCode();
}





void
nsCanvasRenderingContext2DAzure::EnsureUnpremultiplyTable() {
  if (sUnpremultiplyTable)
    return;

  
  sUnpremultiplyTable = new uint8_t[256][256];

  
  
  
  
  

  
  for (uint32_t c = 0; c <= 255; c++) {
    sUnpremultiplyTable[0][c] = c;
  }

  for (int a = 1; a <= 255; a++) {
    for (int c = 0; c <= 255; c++) {
      sUnpremultiplyTable[a][c] = (uint8_t)((c * 255) / a);
    }
  }
}


already_AddRefed<ImageData>
nsCanvasRenderingContext2DAzure::GetImageData(JSContext* aCx, double aSx,
                                              double aSy, double aSw,
                                              double aSh, ErrorResult& error)
{
  if (!mValid) {
    error.Throw(NS_ERROR_FAILURE);
    return NULL;
  }

  if (!mCanvasElement && !mDocShell) {
    NS_ERROR("No canvas element and no docshell in GetImageData!!!");
    error.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return NULL;
  }

  
  
  if (mCanvasElement && mCanvasElement->IsWriteOnly() &&
      !nsContentUtils::IsCallerTrustedForRead())
  {
    
    error.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return NULL;
  }

  if (!NS_finite(aSx) || !NS_finite(aSy) ||
      !NS_finite(aSw) || !NS_finite(aSh)) {
    error.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return NULL;
  }

  if (!aSw || !aSh) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return NULL;
  }

  int32_t x = JS_DoubleToInt32(aSx);
  int32_t y = JS_DoubleToInt32(aSy);
  int32_t wi = JS_DoubleToInt32(aSw);
  int32_t hi = JS_DoubleToInt32(aSh);

  
  
  uint32_t w, h;
  if (aSw < 0) {
    w = -wi;
    x -= w;
  } else {
    w = wi;
  }
  if (aSh < 0) {
    h = -hi;
    y -= h;
  } else {
    h = hi;
  }

  if (w == 0) {
    w = 1;
  }
  if (h == 0) {
    h = 1;
  }

  JSObject* array;
  error = GetImageDataArray(aCx, x, y, w, h, &array);
  if (error.Failed()) {
    return NULL;
  }
  MOZ_ASSERT(array);

  nsRefPtr<ImageData> imageData = new ImageData(w, h, *array);
  return imageData.forget();
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetImageData(double aSx, double aSy,
                                              double aSw, double aSh,
                                              JSContext* aCx,
                                              nsIDOMImageData** aRetval)
{
  ErrorResult rv;
  *aRetval = GetImageData(aCx, aSx, aSy, aSw, aSh, rv).get();
  return rv.ErrorCode();
}

nsresult
nsCanvasRenderingContext2DAzure::GetImageDataArray(JSContext* aCx,
                                                   int32_t aX,
                                                   int32_t aY,
                                                   uint32_t aWidth,
                                                   uint32_t aHeight,
                                                   JSObject** aRetval)
{
  MOZ_ASSERT(aWidth && aHeight);

  CheckedInt<uint32_t> len = CheckedInt<uint32_t>(aWidth) * aHeight * 4;
  if (!len.isValid()) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  CheckedInt<int32_t> rightMost = CheckedInt<int32_t>(aX) + aWidth;
  CheckedInt<int32_t> bottomMost = CheckedInt<int32_t>(aY) + aHeight;

  if (!rightMost.isValid() || !bottomMost.isValid()) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  JSObject* darray = JS_NewUint8ClampedArray(aCx, len.value());
  if (!darray) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (mZero) {
    *aRetval = darray;
    return NS_OK;
  }

  uint8_t* data = JS_GetUint8ClampedArrayData(darray, aCx);

  IntRect srcRect(0, 0, mWidth, mHeight);
  IntRect destRect(aX, aY, aWidth, aHeight);

  IntRect srcReadRect = srcRect.Intersect(destRect);
  IntRect dstWriteRect = srcReadRect;
  dstWriteRect.MoveBy(-aX, -aY);

  uint8_t* src = data;
  uint32_t srcStride = aWidth * 4;
  
  RefPtr<DataSourceSurface> readback;
  if (!srcReadRect.IsEmpty()) {
    RefPtr<SourceSurface> snapshot = mTarget->Snapshot();
    if (snapshot) {
      readback = snapshot->GetDataSurface();

      srcStride = readback->Stride();
      src = readback->GetData() + srcReadRect.y * srcStride + srcReadRect.x * 4;
    }
  }

  
  EnsureUnpremultiplyTable();

  
  
  
  
  uint8_t* dst = data + dstWriteRect.y * (aWidth * 4) + dstWriteRect.x * 4;

  for (int32_t j = 0; j < dstWriteRect.height; ++j) {
    for (int32_t i = 0; i < dstWriteRect.width; ++i) {
      
#ifdef IS_LITTLE_ENDIAN
      uint8_t b = *src++;
      uint8_t g = *src++;
      uint8_t r = *src++;
      uint8_t a = *src++;
#else
      uint8_t a = *src++;
      uint8_t r = *src++;
      uint8_t g = *src++;
      uint8_t b = *src++;
#endif
      
      *dst++ = sUnpremultiplyTable[a][r];
      *dst++ = sUnpremultiplyTable[a][g];
      *dst++ = sUnpremultiplyTable[a][b];
      *dst++ = a;
    }
    src += srcStride - (dstWriteRect.width * 4);
    dst += (aWidth * 4) - (dstWriteRect.width * 4);
  }

  *aRetval = darray;
  return NS_OK;
}

void
nsCanvasRenderingContext2DAzure::EnsurePremultiplyTable() {
  if (sPremultiplyTable)
    return;

  
  sPremultiplyTable = new uint8_t[256][256];

  
  
  

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
    mPath = nullptr;
  }
}

void
nsCanvasRenderingContext2DAzure::PutImageData(JSContext* cx,
                                              ImageData* imageData, double dx,
                                              double dy, ErrorResult& error)
{
  if (!FloatValidate(dx, dy)) {
    error.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  dom::Uint8ClampedArray arr(cx, imageData->GetDataObject());

  error = PutImageData_explicit(JS_DoubleToInt32(dx), JS_DoubleToInt32(dy),
                                imageData->GetWidth(), imageData->GetHeight(),
                                arr.Data(), arr.Length(), false, 0, 0, 0, 0);
}

void
nsCanvasRenderingContext2DAzure::PutImageData(JSContext* cx,
                                              ImageData* imageData, double dx,
                                              double dy, double dirtyX,
                                              double dirtyY, double dirtyWidth,
                                              double dirtyHeight,
                                              ErrorResult& error)
{
  if (!FloatValidate(dx, dy, dirtyX, dirtyY, dirtyWidth, dirtyHeight)) {
    error.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  dom::Uint8ClampedArray arr(cx, imageData->GetDataObject());

  error = PutImageData_explicit(JS_DoubleToInt32(dx), JS_DoubleToInt32(dy),
                                imageData->GetWidth(), imageData->GetHeight(),
                                arr.Data(), arr.Length(), true,
                                JS_DoubleToInt32(dirtyX),
                                JS_DoubleToInt32(dirtyY),
                                JS_DoubleToInt32(dirtyWidth),
                                JS_DoubleToInt32(dirtyHeight));
}



NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::PutImageData(const JS::Value&, double, double,
                                              double, double, double, double,
                                              JSContext*, uint8_t)
{
  

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::PutImageData_explicit(int32_t x, int32_t y, uint32_t w, uint32_t h,
                                                       unsigned char *aData, uint32_t aDataLen,
                                                       bool hasDirtyRect, int32_t dirtyX, int32_t dirtyY,
                                                       int32_t dirtyWidth, int32_t dirtyHeight)
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

      if (!checkedDirtyX.isValid())
          return NS_ERROR_DOM_INDEX_SIZE_ERR;

      dirtyX = checkedDirtyX.value();
      dirtyWidth = -dirtyWidth;
    }

    if (dirtyHeight < 0) {
      NS_ENSURE_TRUE(dirtyHeight != INT_MIN, NS_ERROR_DOM_INDEX_SIZE_ERR);

      CheckedInt32 checkedDirtyY = CheckedInt32(dirtyY) + dirtyHeight;

      if (!checkedDirtyY.isValid())
          return NS_ERROR_DOM_INDEX_SIZE_ERR;

      dirtyY = checkedDirtyY.value();
      dirtyHeight = -dirtyHeight;
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

  uint32_t len = w * h * 4;
  if (aDataLen != len) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  nsRefPtr<gfxImageSurface> imgsurf = new gfxImageSurface(gfxIntSize(w, h),
                                                          gfxASurface::ImageFormatARGB32);
  if (!imgsurf || imgsurf->CairoStatus()) {
    return NS_ERROR_FAILURE;
  }

  
  EnsurePremultiplyTable();

  uint8_t *src = aData;
  uint8_t *dst = imgsurf->Data();

  for (uint32_t j = 0; j < h; j++) {
    for (uint32_t i = 0; i < w; i++) {
      uint8_t r = *src++;
      uint8_t g = *src++;
      uint8_t b = *src++;
      uint8_t a = *src++;

      
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

  if (!mThebesSurface) {
    mThebesSurface =
      gfxPlatform::GetPlatform()->GetThebesSurfaceForDrawTarget(mTarget);    

    if (!mThebesSurface) {
      return NS_ERROR_FAILURE;
    }
  } else {
    
    
    mTarget->Flush();
  }

  *surface = mThebesSurface;
  NS_ADDREF(*surface);

  return NS_OK;
}

static already_AddRefed<ImageData>
CreateImageData(JSContext* cx, nsCanvasRenderingContext2DAzure* context,
                uint32_t w, uint32_t h, ErrorResult& error)
{
  if (w == 0)
      w = 1;
  if (h == 0)
      h = 1;

  CheckedInt<uint32_t> len = CheckedInt<uint32_t>(w) * h * 4;
  if (!len.isValid()) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return NULL;
  }

  
  JSObject* darray = Uint8ClampedArray::Create(cx, context, len.value());
  if (!darray) {
    error.Throw(NS_ERROR_OUT_OF_MEMORY);
    return NULL;
  }

  nsRefPtr<mozilla::dom::ImageData> imageData =
    new mozilla::dom::ImageData(w, h, *darray);
  return imageData.forget();
}

already_AddRefed<ImageData>
nsCanvasRenderingContext2DAzure::CreateImageData(JSContext* cx, double sw,
                                                 double sh, ErrorResult& error)
{
  if (!FloatValidate(sw, sh)) {
    error.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return NULL;
  }

  if (!sw || !sh) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return NULL;
  }

  int32_t wi = JS_DoubleToInt32(sw);
  int32_t hi = JS_DoubleToInt32(sh);

  uint32_t w = NS_ABS(wi);
  uint32_t h = NS_ABS(hi);
  return ::CreateImageData(cx, this, w, h, error);
}

already_AddRefed<ImageData>
nsCanvasRenderingContext2DAzure::CreateImageData(JSContext* cx,
                                                 ImageData* imagedata,
                                                 ErrorResult& error)
{
  return ::CreateImageData(cx, this, imagedata->GetWidth(),
                           imagedata->GetHeight(), error);
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::CreateImageData(const JS::Value &arg1,
                                                 const JS::Value &arg2,
                                                 JSContext* cx,
                                                 uint8_t optional_argc,
                                                 nsIDOMImageData** retval)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::GetMozImageSmoothingEnabled(bool *retVal)
{
  *retVal = GetImageSmoothingEnabled();
  return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2DAzure::SetMozImageSmoothingEnabled(bool val)
{
  SetImageSmoothingEnabled(val);
  return NS_OK;
}

static uint8_t g2DContextLayerUserData;

already_AddRefed<CanvasLayer>
nsCanvasRenderingContext2DAzure::GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                                CanvasLayer *aOldLayer,
                                                LayerManager *aManager)
{
  if (!mValid) {
    return nullptr;
  }

  if (mTarget) {
    mTarget->Flush();
  }

  if (!mResetLayer && aOldLayer) {
      CanvasRenderingContext2DUserDataAzure* userData =
      static_cast<CanvasRenderingContext2DUserDataAzure*>(
        aOldLayer->GetUserData(&g2DContextLayerUserData));
    if (userData && userData->IsForContext(this)) {
      NS_ADDREF(aOldLayer);
      return aOldLayer;
    }
  }

  nsRefPtr<CanvasLayer> canvasLayer = aManager->CreateCanvasLayer();
  if (!canvasLayer) {
    NS_WARNING("CreateCanvasLayer returned null!");
    return nullptr;
  }
  CanvasRenderingContext2DUserDataAzure *userData = nullptr;
  
  
  
  
  

  
  
  
  
  
  userData = new CanvasRenderingContext2DUserDataAzure(this);
  canvasLayer->SetDidTransactionCallback(
          CanvasRenderingContext2DUserDataAzure::DidTransactionCallback, userData);
  canvasLayer->SetUserData(&g2DContextLayerUserData, userData);

  CanvasLayer::Data data;

  data.mDrawTarget = mTarget;
  data.mSize = nsIntSize(mWidth, mHeight);

  canvasLayer->Initialize(data);
  uint32_t flags = mOpaque ? Layer::CONTENT_OPAQUE : 0;
  canvasLayer->SetContentFlags(flags);
  canvasLayer->Updated();

  mResetLayer = false;

  return canvasLayer.forget();
}

void
nsCanvasRenderingContext2DAzure::MarkContextClean()
{
  if (mInvalidateCount > 0) {
    mPredictManyRedrawCalls = mInvalidateCount > kCanvasMaxInvalidateCount;
  }
  mIsEntireFrameInvalid = false;
  mInvalidateCount = 0;
}


bool
nsCanvasRenderingContext2DAzure::ShouldForceInactiveLayer(LayerManager *aManager)
{
    return !aManager->CanUseCanvasLayerForSize(gfxIntSize(mWidth, mHeight));
}
