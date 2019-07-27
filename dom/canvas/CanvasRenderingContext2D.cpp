




#include "CanvasRenderingContext2D.h"

#include "nsXULElement.h"

#include "nsIServiceManager.h"
#include "nsMathUtils.h"
#include "SVGImageContext.h"

#include "nsContentUtils.h"

#include "nsIDocument.h"
#include "mozilla/dom/HTMLCanvasElement.h"
#include "nsSVGEffects.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"

#include "nsIInterfaceRequestorUtils.h"
#include "nsIFrame.h"
#include "nsError.h"

#include "nsCSSParser.h"
#include "mozilla/css/StyleRule.h"
#include "mozilla/css/Declaration.h"
#include "nsComputedDOMStyle.h"
#include "nsStyleSet.h"

#include "nsPrintfCString.h"

#include "nsReadableUtils.h"

#include "nsColor.h"
#include "nsGfxCIID.h"
#include "nsIDocShell.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsDisplayList.h"
#include "nsFocusManager.h"

#include "nsTArray.h"

#include "ImageEncoder.h"
#include "ImageRegion.h"

#include "gfxContext.h"
#include "gfxASurface.h"
#include "gfxImageSurface.h"
#include "gfxPlatform.h"
#include "gfxFont.h"
#include "gfxBlur.h"
#include "gfxUtils.h"

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

#include "mozilla/Alignment.h"
#include "mozilla/Assertions.h"
#include "mozilla/CheckedInt.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/ImageData.h"
#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/dom/ToJSValue.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/Endian.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/PathHelpers.h"
#include "mozilla/gfx/DataSurfaceHelpers.h"
#include "mozilla/ipc/DocumentRendererParent.h"
#include "mozilla/ipc/PDocumentRendererParent.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"
#include "mozilla/unused.h"
#include "nsCCUncollectableMarker.h"
#include "nsWrapperCacheInlines.h"
#include "mozilla/dom/CanvasRenderingContext2DBinding.h"
#include "mozilla/dom/HTMLImageElement.h"
#include "mozilla/dom/HTMLVideoElement.h"
#include "mozilla/dom/SVGMatrix.h"
#include "mozilla/dom/TextMetrics.h"
#include "mozilla/dom/UnionTypes.h"
#include "mozilla/dom/SVGMatrix.h"
#include "nsGlobalWindow.h"
#include "GLContext.h"
#include "GLContextProvider.h"
#include "SVGContentUtils.h"
#include "SVGImageContext.h"
#include "nsIScreenManager.h"

#undef free // apparently defined by some windows header, clashing with a free()
            
#ifdef USE_SKIA
#include "SkiaGLGlue.h"
#include "SurfaceStream.h"
#include "SurfaceTypes.h"
#endif

using mozilla::gl::GLContext;
using mozilla::gl::SkiaGLGlue;
using mozilla::gl::GLContextProvider;

#ifdef XP_WIN
#include "gfxWindowsPlatform.h"
#endif

#ifdef MOZ_WIDGET_GONK
#include "mozilla/layers/ShadowLayers.h"
#endif


#undef DrawText

using namespace mozilla;
using namespace mozilla::CanvasUtils;
using namespace mozilla::css;
using namespace mozilla::gfx;
using namespace mozilla::image;
using namespace mozilla::ipc;
using namespace mozilla::layers;

namespace mgfx = mozilla::gfx;

namespace mozilla {
namespace dom {


const Float SIGMA_MAX = 100;


static int64_t gCanvasAzureMemoryUsed = 0;




class Canvas2dPixelsReporter MOZ_FINAL : public nsIMemoryReporter
{
  ~Canvas2dPixelsReporter() {}
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD CollectReports(nsIHandleReportCallback* aHandleReport,
                            nsISupports* aData, bool aAnonymize)
  {
    return MOZ_COLLECT_REPORT(
      "canvas-2d-pixels", KIND_OTHER, UNITS_BYTES,
      gCanvasAzureMemoryUsed,
      "Memory used by 2D canvases. Each canvas requires "
      "(width * height * 4) bytes.");
  }
};

NS_IMPL_ISUPPORTS(Canvas2dPixelsReporter, nsIMemoryReporter)

class CanvasRadialGradient : public CanvasGradient
{
public:
  CanvasRadialGradient(CanvasRenderingContext2D* aContext,
                       const Point &aBeginOrigin, Float aBeginRadius,
                       const Point &aEndOrigin, Float aEndRadius)
    : CanvasGradient(aContext, Type::RADIAL)
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

class CanvasLinearGradient : public CanvasGradient
{
public:
  CanvasLinearGradient(CanvasRenderingContext2D* aContext,
                       const Point &aBegin, const Point &aEnd)
    : CanvasGradient(aContext, Type::LINEAR)
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
  typedef CanvasRenderingContext2D::Style Style;
  typedef CanvasRenderingContext2D::ContextState ContextState;

  CanvasGeneralPattern() : mPattern(nullptr) {}
  ~CanvasGeneralPattern()
  {
    if (mPattern) {
      mPattern->~Pattern();
    }
  }

  Pattern& ForStyle(CanvasRenderingContext2D *aCtx,
                    Style aStyle,
                    DrawTarget *aRT)
  {
    
    
    NS_ASSERTION(!mPattern, "ForStyle() should only be called once on CanvasGeneralPattern!");

    const ContextState &state = aCtx->CurrentState();

    if (state.StyleIsColor(aStyle)) {
      mPattern = new (mColorPattern.addr()) ColorPattern(Color::FromABGR(state.colorStyles[aStyle]));
    } else if (state.gradientStyles[aStyle] &&
               state.gradientStyles[aStyle]->GetType() == CanvasGradient::Type::LINEAR) {
      CanvasLinearGradient *gradient =
        static_cast<CanvasLinearGradient*>(state.gradientStyles[aStyle].get());

      mPattern = new (mLinearGradientPattern.addr())
        LinearGradientPattern(gradient->mBegin, gradient->mEnd,
                              gradient->GetGradientStopsForTarget(aRT));
    } else if (state.gradientStyles[aStyle] &&
               state.gradientStyles[aStyle]->GetType() == CanvasGradient::Type::RADIAL) {
      CanvasRadialGradient *gradient =
        static_cast<CanvasRadialGradient*>(state.gradientStyles[aStyle].get());

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
      if (state.patternStyles[aStyle]->mRepeat == CanvasPattern::RepeatMode::NOREPEAT) {
        mode = ExtendMode::CLAMP;
      } else {
        mode = ExtendMode::REPEAT;
      }
      mPattern = new (mSurfacePattern.addr())
        SurfacePattern(state.patternStyles[aStyle]->mSurface, mode,
                       state.patternStyles[aStyle]->mTransform);
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
  typedef CanvasRenderingContext2D::ContextState ContextState;

  explicit AdjustedTarget(CanvasRenderingContext2D* ctx,
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

    
    
    mTempRect.Inflate(Margin(blurRadius + std::max<Float>(state.shadowOffset.y, 0),
                             blurRadius + std::max<Float>(-state.shadowOffset.x, 0),
                             blurRadius + std::max<Float>(-state.shadowOffset.y, 0),
                             blurRadius + std::max<Float>(state.shadowOffset.x, 0)));

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
                                            SurfaceFormat::B8G8R8A8, mSigma);

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

  operator DrawTarget*()
  {
    return mTarget;
  }

  DrawTarget* operator->()
  {
    return mTarget;
  }

private:
  RefPtr<DrawTarget> mTarget;
  CanvasRenderingContext2D *mCtx;
  Float mSigma;
  mgfx::Rect mTempRect;
};

void
CanvasPattern::SetTransform(SVGMatrix& aMatrix)
{
  mTransform = ToMatrix(aMatrix.GetMatrix());
}

void
CanvasGradient::AddColorStop(float offset, const nsAString& colorstr, ErrorResult& rv)
{
  if (offset < 0.0 || offset > 1.0) {
    rv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  nsCSSValue value;
  nsCSSParser parser;
  if (!parser.ParseColorString(colorstr, nullptr, 0, value)) {
    rv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }

  nscolor color;
  if (!nsRuleNode::ComputeColor(value, nullptr, nullptr, color)) {
    rv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return;
  }

  mStops = nullptr;

  GradientStop newStop;

  newStop.offset = offset;
  newStop.color = Color::FromABGR(color);

  mRawStops.AppendElement(newStop);
}

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(CanvasGradient, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(CanvasGradient, Release)

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(CanvasGradient, mContext)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(CanvasPattern, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(CanvasPattern, Release)

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(CanvasPattern, mContext)

class CanvasRenderingContext2DUserData : public LayerUserData {
public:
  explicit CanvasRenderingContext2DUserData(CanvasRenderingContext2D *aContext)
    : mContext(aContext)
  {
    aContext->mUserDatas.AppendElement(this);
  }
  ~CanvasRenderingContext2DUserData()
  {
    if (mContext) {
      mContext->mUserDatas.RemoveElement(this);
    }
  }

  static void PreTransactionCallback(void* aData)
  {
    CanvasRenderingContext2DUserData* self =
      static_cast<CanvasRenderingContext2DUserData*>(aData);
    CanvasRenderingContext2D* context = self->mContext;
    if (!context || !context->mStream || !context->mTarget)
      return;

    
    
    context->mTarget->Flush();
  }

  static void DidTransactionCallback(void* aData)
  {
    CanvasRenderingContext2DUserData* self =
      static_cast<CanvasRenderingContext2DUserData*>(aData);
    if (self->mContext) {
      self->mContext->MarkContextClean();
    }
  }
  bool IsForContext(CanvasRenderingContext2D *aContext)
  {
    return mContext == aContext;
  }
  void Forget()
  {
    mContext = nullptr;
  }

private:
  CanvasRenderingContext2D *mContext;
};

NS_IMPL_CYCLE_COLLECTING_ADDREF(CanvasRenderingContext2D)
NS_IMPL_CYCLE_COLLECTING_RELEASE(CanvasRenderingContext2D)

NS_IMPL_CYCLE_COLLECTION_CLASS(CanvasRenderingContext2D)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(CanvasRenderingContext2D)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mCanvasElement)
  for (uint32_t i = 0; i < tmp->mStyleStack.Length(); i++) {
    ImplCycleCollectionUnlink(tmp->mStyleStack[i].patternStyles[Style::STROKE]);
    ImplCycleCollectionUnlink(tmp->mStyleStack[i].patternStyles[Style::FILL]);
    ImplCycleCollectionUnlink(tmp->mStyleStack[i].gradientStyles[Style::STROKE]);
    ImplCycleCollectionUnlink(tmp->mStyleStack[i].gradientStyles[Style::FILL]);
  }
  for (size_t x = 0 ; x < tmp->mHitRegionsOptions.Length(); x++) {
    RegionInfo& info = tmp->mHitRegionsOptions[x];
    if (info.mElement) {
      ImplCycleCollectionUnlink(info.mElement);
    }
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(CanvasRenderingContext2D)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCanvasElement)
  for (uint32_t i = 0; i < tmp->mStyleStack.Length(); i++) {
    ImplCycleCollectionTraverse(cb, tmp->mStyleStack[i].patternStyles[Style::STROKE], "Stroke CanvasPattern");
    ImplCycleCollectionTraverse(cb, tmp->mStyleStack[i].patternStyles[Style::FILL], "Fill CanvasPattern");
    ImplCycleCollectionTraverse(cb, tmp->mStyleStack[i].gradientStyles[Style::STROKE], "Stroke CanvasGradient");
    ImplCycleCollectionTraverse(cb, tmp->mStyleStack[i].gradientStyles[Style::FILL], "Fill CanvasGradient");
  }
  for (size_t x = 0 ; x < tmp->mHitRegionsOptions.Length(); x++) {
    RegionInfo& info = tmp->mHitRegionsOptions[x];
    if (info.mElement) {
      ImplCycleCollectionTraverse(cb, info.mElement, "Hit region fallback element");
    }
  }
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_WRAPPERCACHE(CanvasRenderingContext2D)

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(CanvasRenderingContext2D)
 if (nsCCUncollectableMarker::sGeneration && tmp->IsBlack()) {
   dom::Element* canvasElement = tmp->mCanvasElement;
    if (canvasElement) {
      if (canvasElement->IsPurple()) {
        canvasElement->RemovePurple();
      }
      dom::Element::MarkNodeChildren(canvasElement);
    }
    return true;
  }
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(CanvasRenderingContext2D)
  return nsCCUncollectableMarker::sGeneration && tmp->IsBlack();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(CanvasRenderingContext2D)
  return nsCCUncollectableMarker::sGeneration && tmp->IsBlack();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(CanvasRenderingContext2D)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextInternal)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END







uint32_t CanvasRenderingContext2D::sNumLivingContexts = 0;
DrawTarget* CanvasRenderingContext2D::sErrorTarget = nullptr;



CanvasRenderingContext2D::CanvasRenderingContext2D()
  : mRenderingMode(RenderingMode::OpenGLBackendMode)
  
  , mWidth(0), mHeight(0)
  , mZero(false), mOpaque(false)
  , mResetLayer(true)
  , mIPC(false)
  , mStream(nullptr)
  , mIsEntireFrameInvalid(false)
  , mPredictManyRedrawCalls(false), mPathTransformWillUpdate(false)
  , mInvalidateCount(0)
{
  sNumLivingContexts++;
  SetIsDOMBinding();

  
  if (!gfxPlatform::GetPlatform()->UseAcceleratedSkiaCanvas()) {
    mRenderingMode = RenderingMode::SoftwareBackendMode;
  }

}

CanvasRenderingContext2D::~CanvasRenderingContext2D()
{
  RemovePostRefreshObserver();
  Reset();
  
  for (uint32_t i = 0; i < mUserDatas.Length(); ++i) {
    mUserDatas[i]->Forget();
  }
  sNumLivingContexts--;
  if (!sNumLivingContexts) {
    NS_IF_RELEASE(sErrorTarget);
  }

  RemoveDemotableContext(this);
}

JSObject*
CanvasRenderingContext2D::WrapObject(JSContext *cx)
{
  return CanvasRenderingContext2DBinding::Wrap(cx, this);
}

bool
CanvasRenderingContext2D::ParseColor(const nsAString& aString,
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

  if (value.IsNumericColorUnit()) {
    
    *aColor = value.GetColorValue();
  } else {
    
    nsIPresShell* presShell = GetPresShell();
    nsRefPtr<nsStyleContext> parentContext;
    if (mCanvasElement && mCanvasElement->IsInDoc()) {
      
      parentContext = nsComputedDOMStyle::GetStyleContextForElement(
        mCanvasElement, nullptr, presShell);
    }

    unused << nsRuleNode::ComputeColor(
      value, presShell ? presShell->GetPresContext() : nullptr, parentContext,
      *aColor);
  }
  return true;
}

nsresult
CanvasRenderingContext2D::Reset()
{
  if (mCanvasElement) {
    mCanvasElement->InvalidateCanvas();
  }

  
  
  if (mTarget && IsTargetValid() && !mDocShell) {
    gCanvasAzureMemoryUsed -= mWidth * mHeight * 4;
  }

  mTarget = nullptr;
  mStream = nullptr;

  
  mHitRegionsOptions.ClearAndRetainStorage();

  
  
  mIsEntireFrameInvalid = false;
  mPredictManyRedrawCalls = false;

  return NS_OK;
}

void
CanvasRenderingContext2D::SetStyleFromString(const nsAString& str,
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
CanvasRenderingContext2D::GetStyleAsUnion(OwningStringOrCanvasGradientOrCanvasPattern& aValue,
                                          Style aWhichStyle)
{
  const ContextState &state = CurrentState();
  if (state.patternStyles[aWhichStyle]) {
    aValue.SetAsCanvasPattern() = state.patternStyles[aWhichStyle];
  } else if (state.gradientStyles[aWhichStyle]) {
    aValue.SetAsCanvasGradient() = state.gradientStyles[aWhichStyle];
  } else {
    StyleColorToString(state.colorStyles[aWhichStyle], aValue.SetAsString());
  }
}


void
CanvasRenderingContext2D::StyleColorToString(const nscolor& aColor, nsAString& aStr)
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
CanvasRenderingContext2D::Redraw()
{
  if (mIsEntireFrameInvalid) {
    return NS_OK;
  }

  mIsEntireFrameInvalid = true;

  if (!mCanvasElement) {
    NS_ASSERTION(mDocShell, "Redraw with no canvas element or docshell!");
    return NS_OK;
  }

  nsSVGEffects::InvalidateDirectRenderingObservers(mCanvasElement);

  mCanvasElement->InvalidateCanvasContent(nullptr);

  return NS_OK;
}

void
CanvasRenderingContext2D::Redraw(const mgfx::Rect &r)
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

  nsSVGEffects::InvalidateDirectRenderingObservers(mCanvasElement);

  mCanvasElement->InvalidateCanvasContent(&r);
}

void
CanvasRenderingContext2D::DidRefresh()
{
  if (mStream && mStream->GLContext()) {
    mStream->GLContext()->FlushIfHeavyGLCallsSinceLastFlush();
  }
}

void
CanvasRenderingContext2D::RedrawUser(const gfxRect& r)
{
  if (mIsEntireFrameInvalid) {
    ++mInvalidateCount;
    return;
  }

  mgfx::Rect newr =
    mTarget->GetTransform().TransformBounds(ToRect(r));
  Redraw(newr);
}

bool CanvasRenderingContext2D::SwitchRenderingMode(RenderingMode aRenderingMode)
{
  if (!IsTargetValid() || mRenderingMode == aRenderingMode) {
    return false;
  }

  RefPtr<SourceSurface> snapshot = mTarget->Snapshot();
  RefPtr<DrawTarget> oldTarget = mTarget;
  mTarget = nullptr;
  mStream = nullptr;
  mResetLayer = true;

  
  RenderingMode attemptedMode = EnsureTarget(aRenderingMode);
  if (!IsTargetValid())
    return false;

  
  mRenderingMode = attemptedMode;

  
  mgfx::Rect r(0, 0, mWidth, mHeight);
  mTarget->DrawSurface(snapshot, r, r);

  
  for (uint32_t i = 0; i < CurrentState().clipsPushed.size(); i++) {
    mTarget->PushClip(CurrentState().clipsPushed[i]);
  }

  mTarget->SetTransform(oldTarget->GetTransform());

  return true;
}

void CanvasRenderingContext2D::Demote()
{
  if (SwitchRenderingMode(RenderingMode::SoftwareBackendMode)) {
    RemoveDemotableContext(this);
  }
}

std::vector<CanvasRenderingContext2D*>&
CanvasRenderingContext2D::DemotableContexts()
{
  static std::vector<CanvasRenderingContext2D*> contexts;
  return contexts;
}

void
CanvasRenderingContext2D::DemoteOldestContextIfNecessary()
{
  const size_t kMaxContexts = 64;

  std::vector<CanvasRenderingContext2D*>& contexts = DemotableContexts();
  if (contexts.size() < kMaxContexts)
    return;

  CanvasRenderingContext2D* oldest = contexts.front();
  if (oldest->SwitchRenderingMode(RenderingMode::SoftwareBackendMode)) {
    RemoveDemotableContext(oldest);
  }
}

void
CanvasRenderingContext2D::AddDemotableContext(CanvasRenderingContext2D* context)
{
  std::vector<CanvasRenderingContext2D*>::iterator iter = std::find(DemotableContexts().begin(), DemotableContexts().end(), context);
  if (iter != DemotableContexts().end())
    return;

  DemotableContexts().push_back(context);
}

void
CanvasRenderingContext2D::RemoveDemotableContext(CanvasRenderingContext2D* context)
{
  std::vector<CanvasRenderingContext2D*>::iterator iter = std::find(DemotableContexts().begin(), DemotableContexts().end(), context);
  if (iter != DemotableContexts().end())
    DemotableContexts().erase(iter);
}

bool
CanvasRenderingContext2D::CheckSizeForSkiaGL(IntSize size) {
  MOZ_ASSERT(NS_IsMainThread());

  int minsize = Preferences::GetInt("gfx.canvas.min-size-for-skia-gl", 128);
  if (size.width < minsize || size.height < minsize) {
    return false;
  }

  
  
  
  
  int maxsize = Preferences::GetInt("gfx.canvas.max-size-for-skia-gl", 0);

  
  if (!maxsize) {
    return true;
  }

  
  if (maxsize > 0) {
    return size.width <= maxsize && size.height <= maxsize;
  }

  
  static int32_t gScreenPixels = -1;
  if (gScreenPixels < 0) {
    
    
    
    
    if (gfxPlatform::GetPlatform()->HasEnoughTotalSystemMemoryForSkiaGL()) {
      gScreenPixels = 980 * 480;
    }

    nsCOMPtr<nsIScreenManager> screenManager =
      do_GetService("@mozilla.org/gfx/screenmanager;1");
    if (screenManager) {
      nsCOMPtr<nsIScreen> primaryScreen;
      screenManager->GetPrimaryScreen(getter_AddRefs(primaryScreen));
      if (primaryScreen) {
        int32_t x, y, width, height;
        primaryScreen->GetRect(&x, &y, &width, &height);

        gScreenPixels = std::max(gScreenPixels, width * height);
      }
    }
  }

  
  static double gDefaultScale = 1.0;

  double scale = gDefaultScale > 0 ? gDefaultScale : 1.0;
  int32_t threshold = ceil(scale * scale * gScreenPixels);

  
  return threshold < 0 || (size.width * size.height) <= threshold;
}

CanvasRenderingContext2D::RenderingMode
CanvasRenderingContext2D::EnsureTarget(RenderingMode aRenderingMode)
{
  
  MOZ_ASSERT(mRenderingMode != RenderingMode::DefaultBackendMode);

  RenderingMode mode = (aRenderingMode == RenderingMode::DefaultBackendMode) ? mRenderingMode : aRenderingMode;

  if (mTarget && mode == mRenderingMode) {
    return mRenderingMode;
  }

   
  IntSize size(mWidth, mHeight);
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
      if (mode == RenderingMode::OpenGLBackendMode && CheckSizeForSkiaGL(size)) {
        DemoteOldestContextIfNecessary();

        SkiaGLGlue* glue = gfxPlatform::GetPlatform()->GetSkiaGLGlue();

#if USE_SKIA
        if (glue && glue->GetGrContext() && glue->GetGLContext()) {
          mTarget = Factory::CreateDrawTargetSkiaWithGrContext(glue->GetGrContext(), size, format);
          if (mTarget) {
            mStream = gl::SurfaceStream::CreateForType(gl::SurfaceStreamType::TripleBuffer,
                                                       glue->GetGLContext());
            AddDemotableContext(this);
          } else {
            printf_stderr("Failed to create a SkiaGL DrawTarget, falling back to software\n");
            mode = RenderingMode::SoftwareBackendMode;
          }
        }
#endif
        if (!mTarget) {
          mTarget = layerManager->CreateDrawTarget(size, format);
        }
      } else {
        mTarget = layerManager->CreateDrawTarget(size, format);
        mode = RenderingMode::SoftwareBackendMode;
      }
     } else {
        mTarget = gfxPlatform::GetPlatform()->CreateOffscreenCanvasDrawTarget(size, format);
        mode = RenderingMode::SoftwareBackendMode;
     }
  }

  if (mTarget) {
    static bool registered = false;
    if (!registered) {
      registered = true;
      RegisterStrongMemoryReporter(new Canvas2dPixelsReporter());
    }

    gCanvasAzureMemoryUsed += mWidth * mHeight * 4;
    JSContext* context = nsContentUtils::GetCurrentJSContext();
    if (context) {
      JS_updateMallocCounter(context, mWidth * mHeight * 4);
    }

    mTarget->ClearRect(mgfx::Rect(Point(0, 0), Size(mWidth, mHeight)));
    if (mTarget->GetBackendType() == mgfx::BackendType::CAIRO) {
      
      
      
      
      
      
      mTarget->PushClipRect(mgfx::Rect(Point(0, 0), Size(mWidth, mHeight)));
    }
    
    
    if (mCanvasElement) {
      mCanvasElement->InvalidateCanvas();
    }
    
    
    Redraw();
  } else {
    EnsureErrorTarget();
    mTarget = sErrorTarget;
  }

  return mode;
}

#ifdef DEBUG
int32_t
CanvasRenderingContext2D::GetWidth() const
{
  return mWidth;
}

int32_t
CanvasRenderingContext2D::GetHeight() const
{
  return mHeight;
}
#endif

NS_IMETHODIMP
CanvasRenderingContext2D::SetDimensions(int32_t width, int32_t height)
{
  ClearTarget();

  
  mZero = false;
  if (height == 0) {
    height = 1;
    mZero = true;
  }
  if (width == 0) {
    width = 1;
    mZero = true;
  }
  mWidth = width;
  mHeight = height;

  return NS_OK;
}

void
CanvasRenderingContext2D::ClearTarget()
{
  Reset();

  mResetLayer = true;

  
  mStyleStack.Clear();
  mPathBuilder = nullptr;
  mPath = nullptr;
  mDSPathBuilder = nullptr;

  ContextState *state = mStyleStack.AppendElement();
  state->globalAlpha = 1.0;

  state->colorStyles[Style::FILL] = NS_RGB(0,0,0);
  state->colorStyles[Style::STROKE] = NS_RGB(0,0,0);
  state->shadowColor = NS_RGBA(0,0,0,0);
}

NS_IMETHODIMP
CanvasRenderingContext2D::InitializeWithSurface(nsIDocShell *shell,
                                                gfxASurface *surface,
                                                int32_t width,
                                                int32_t height)
{
  RemovePostRefreshObserver();
  mDocShell = shell;
  AddPostRefreshObserverIfNecessary();

  SetDimensions(width, height);
  mTarget = gfxPlatform::GetPlatform()->
    CreateDrawTargetForSurface(surface, IntSize(width, height));

  if (!mTarget) {
    EnsureErrorTarget();
    mTarget = sErrorTarget;
  }

  if (mTarget->GetBackendType() == mgfx::BackendType::CAIRO) {
    
    mTarget->PushClipRect(mgfx::Rect(Point(0, 0), Size(mWidth, mHeight)));
  }

  return NS_OK;
}

NS_IMETHODIMP
CanvasRenderingContext2D::SetIsOpaque(bool isOpaque)
{
  if (isOpaque != mOpaque) {
    mOpaque = isOpaque;
    ClearTarget();
  }

  return NS_OK;
}

NS_IMETHODIMP
CanvasRenderingContext2D::SetIsIPC(bool isIPC)
{
  if (isIPC != mIPC) {
    mIPC = isIPC;
    ClearTarget();
  }

  return NS_OK;
}

NS_IMETHODIMP
CanvasRenderingContext2D::SetContextOptions(JSContext* aCx, JS::Handle<JS::Value> aOptions)
{
  if (aOptions.isNullOrUndefined()) {
    return NS_OK;
  }

  
  MOZ_ASSERT(!mTarget);

  ContextAttributes2D attributes;
  NS_ENSURE_TRUE(attributes.Init(aCx, aOptions), NS_ERROR_UNEXPECTED);

  if (Preferences::GetBool("gfx.canvas.willReadFrequently.enable", false)) {
    
    if (attributes.mWillReadFrequently) {
      mRenderingMode = RenderingMode::SoftwareBackendMode;
    }
  }

  if (!attributes.mAlpha) {
    SetIsOpaque(true);
  }

  return NS_OK;
}

void
CanvasRenderingContext2D::GetImageBuffer(uint8_t** aImageBuffer,
                                         int32_t* aFormat)
{
  *aImageBuffer = nullptr;
  *aFormat = 0;

  EnsureTarget();
  RefPtr<SourceSurface> snapshot = mTarget->Snapshot();
  if (!snapshot) {
    return;
  }

  RefPtr<DataSourceSurface> data = snapshot->GetDataSurface();
  if (!data || data->GetSize() != IntSize(mWidth, mHeight)) {
    return;
  }

  *aImageBuffer = SurfaceToPackedBGRA(data);
  *aFormat = imgIEncoder::INPUT_FORMAT_HOSTARGB;
}

nsString CanvasRenderingContext2D::GetHitRegion(const mozilla::gfx::Point& aPoint)
{
  for (size_t x = 0 ; x < mHitRegionsOptions.Length(); x++) {
    RegionInfo& info = mHitRegionsOptions[x];
    if (info.mPath->ContainsPoint(aPoint, Matrix())) {
      return info.mId;
    }
  }
  return nsString();
}

NS_IMETHODIMP
CanvasRenderingContext2D::GetInputStream(const char *aMimeType,
                                         const char16_t *aEncoderOptions,
                                         nsIInputStream **aStream)
{
  nsCString enccid("@mozilla.org/image/encoder;2?type=");
  enccid += aMimeType;
  nsCOMPtr<imgIEncoder> encoder = do_CreateInstance(enccid.get());
  if (!encoder) {
    return NS_ERROR_FAILURE;
  }

  nsAutoArrayPtr<uint8_t> imageBuffer;
  int32_t format = 0;
  GetImageBuffer(getter_Transfers(imageBuffer), &format);
  if (!imageBuffer) {
    return NS_ERROR_FAILURE;
  }

  return ImageEncoder::GetInputStream(mWidth, mHeight, imageBuffer, format,
                                      encoder, aEncoderOptions, aStream);
}

SurfaceFormat
CanvasRenderingContext2D::GetSurfaceFormat() const
{
  return mOpaque ? SurfaceFormat::B8G8R8X8 : SurfaceFormat::B8G8R8A8;
}





void
CanvasRenderingContext2D::Save()
{
  EnsureTarget();
  mStyleStack[mStyleStack.Length() - 1].transform = mTarget->GetTransform();
  mStyleStack.SetCapacity(mStyleStack.Length() + 1);
  mStyleStack.AppendElement(CurrentState());
}

void
CanvasRenderingContext2D::Restore()
{
  if (mStyleStack.Length() - 1 == 0)
    return;

  TransformWillUpdate();

  for (uint32_t i = 0; i < CurrentState().clipsPushed.size(); i++) {
    mTarget->PopClip();
  }

  mStyleStack.RemoveElementAt(mStyleStack.Length() - 1);

  mTarget->SetTransform(CurrentState().transform);
}





void
CanvasRenderingContext2D::Scale(double x, double y, ErrorResult& error)
{
  TransformWillUpdate();
  if (!IsTargetValid()) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  Matrix newMatrix = mTarget->GetTransform();
  mTarget->SetTransform(newMatrix.PreScale(x, y));
}

void
CanvasRenderingContext2D::Rotate(double angle, ErrorResult& error)
{
  TransformWillUpdate();
  if (!IsTargetValid()) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  Matrix rotation = Matrix::Rotation(angle);
  mTarget->SetTransform(rotation * mTarget->GetTransform());
}

void
CanvasRenderingContext2D::Translate(double x, double y, ErrorResult& error)
{
  TransformWillUpdate();
  if (!IsTargetValid()) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  mTarget->SetTransform(Matrix(mTarget->GetTransform()).PreTranslate(x, y));
}

void
CanvasRenderingContext2D::Transform(double m11, double m12, double m21,
                                    double m22, double dx, double dy,
                                    ErrorResult& error)
{
  TransformWillUpdate();
  if (!IsTargetValid()) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  Matrix matrix(m11, m12, m21, m22, dx, dy);
  mTarget->SetTransform(matrix * mTarget->GetTransform());
}

void
CanvasRenderingContext2D::SetTransform(double m11, double m12,
                                       double m21, double m22,
                                       double dx, double dy,
                                       ErrorResult& error)
{
  TransformWillUpdate();
  if (!IsTargetValid()) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  Matrix matrix(m11, m12, m21, m22, dx, dy);
  mTarget->SetTransform(matrix);
}

static void
MatrixToJSObject(JSContext* cx, const Matrix& matrix,
                 JS::MutableHandle<JSObject*> result, ErrorResult& error)
{
  double elts[6] = { matrix._11, matrix._12,
                     matrix._21, matrix._22,
                     matrix._31, matrix._32 };

  
  JS::Rooted<JS::Value> val(cx);
  if (!ToJSValue(cx, elts, &val)) {
    error.Throw(NS_ERROR_OUT_OF_MEMORY);
  } else {
    result.set(&val.toObject());
  }
}

static bool
ObjectToMatrix(JSContext* cx, JS::Handle<JSObject*> obj, Matrix& matrix,
               ErrorResult& error)
{
  uint32_t length;
  if (!JS_GetArrayLength(cx, obj, &length) || length != 6) {
    
    error.Throw(NS_ERROR_INVALID_ARG);
    return false;
  }

  Float* elts[] = { &matrix._11, &matrix._12, &matrix._21, &matrix._22,
                    &matrix._31, &matrix._32 };
  for (uint32_t i = 0; i < 6; ++i) {
    JS::Rooted<JS::Value> elt(cx);
    double d;
    if (!JS_GetElement(cx, obj, i, &elt)) {
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
CanvasRenderingContext2D::SetMozCurrentTransform(JSContext* cx,
                                                 JS::Handle<JSObject*> currentTransform,
                                                 ErrorResult& error)
{
  EnsureTarget();
  if (!IsTargetValid()) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  Matrix newCTM;
  if (ObjectToMatrix(cx, currentTransform, newCTM, error)) {
    mTarget->SetTransform(newCTM);
  }
}

void
CanvasRenderingContext2D::GetMozCurrentTransform(JSContext* cx,
                                                 JS::MutableHandle<JSObject*> result,
                                                 ErrorResult& error) const
{
  MatrixToJSObject(cx, mTarget ? mTarget->GetTransform() : Matrix(),
                   result, error);
}

void
CanvasRenderingContext2D::SetMozCurrentTransformInverse(JSContext* cx,
                                                        JS::Handle<JSObject*> currentTransform,
                                                        ErrorResult& error)
{
  EnsureTarget();
  if (!IsTargetValid()) {
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

void
CanvasRenderingContext2D::GetMozCurrentTransformInverse(JSContext* cx,
                                                        JS::MutableHandle<JSObject*> result,
                                                        ErrorResult& error) const
{
  if (!mTarget) {
    MatrixToJSObject(cx, Matrix(), result, error);
    return;
  }

  Matrix ctm = mTarget->GetTransform();

  if (!ctm.Invert()) {
    double NaN = JS_GetNaNValue(cx).toDouble();
    ctm = Matrix(NaN, NaN, NaN, NaN, NaN, NaN);
  }

  MatrixToJSObject(cx, ctm, result, error);
}





void
CanvasRenderingContext2D::SetStyleFromUnion(const StringOrCanvasGradientOrCanvasPattern& value,
                                            Style whichStyle)
{
  if (value.IsString()) {
    SetStyleFromString(value.GetAsString(), whichStyle);
    return;
  }

  if (value.IsCanvasGradient()) {
    SetStyleFromGradient(value.GetAsCanvasGradient(), whichStyle);
    return;
  }

  if (value.IsCanvasPattern()) {
    SetStyleFromPattern(value.GetAsCanvasPattern(), whichStyle);
    return;
  }

  MOZ_ASSERT_UNREACHABLE("Invalid union value");
}

void
CanvasRenderingContext2D::SetFillRule(const nsAString& aString)
{
  FillRule rule;

  if (aString.EqualsLiteral("evenodd"))
    rule = FillRule::FILL_EVEN_ODD;
  else if (aString.EqualsLiteral("nonzero"))
    rule = FillRule::FILL_WINDING;
  else
    return;

  CurrentState().fillRule = rule;
}

void
CanvasRenderingContext2D::GetFillRule(nsAString& aString)
{
  switch (CurrentState().fillRule) {
  case FillRule::FILL_WINDING:
    aString.AssignLiteral("nonzero"); break;
  case FillRule::FILL_EVEN_ODD:
    aString.AssignLiteral("evenodd"); break;
  }
}



already_AddRefed<CanvasGradient>
CanvasRenderingContext2D::CreateLinearGradient(double x0, double y0, double x1, double y1)
{
  nsRefPtr<CanvasGradient> grad =
    new CanvasLinearGradient(this, Point(x0, y0), Point(x1, y1));

  return grad.forget();
}

already_AddRefed<CanvasGradient>
CanvasRenderingContext2D::CreateRadialGradient(double x0, double y0, double r0,
                                               double x1, double y1, double r1,
                                               ErrorResult& aError)
{
  if (r0 < 0.0 || r1 < 0.0) {
    aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsRefPtr<CanvasGradient> grad =
    new CanvasRadialGradient(this, Point(x0, y0), r0, Point(x1, y1), r1);

  return grad.forget();
}

already_AddRefed<CanvasPattern>
CanvasRenderingContext2D::CreatePattern(const HTMLImageOrCanvasOrVideoElement& element,
                                        const nsAString& repeat,
                                        ErrorResult& error)
{
  CanvasPattern::RepeatMode repeatMode =
    CanvasPattern::RepeatMode::NOREPEAT;

  if (repeat.IsEmpty() || repeat.EqualsLiteral("repeat")) {
    repeatMode = CanvasPattern::RepeatMode::REPEAT;
  } else if (repeat.EqualsLiteral("repeat-x")) {
    repeatMode = CanvasPattern::RepeatMode::REPEATX;
  } else if (repeat.EqualsLiteral("repeat-y")) {
    repeatMode = CanvasPattern::RepeatMode::REPEATY;
  } else if (repeat.EqualsLiteral("no-repeat")) {
    repeatMode = CanvasPattern::RepeatMode::NOREPEAT;
  } else {
    error.Throw(NS_ERROR_DOM_SYNTAX_ERR);
    return nullptr;
  }

  Element* htmlElement;
  if (element.IsHTMLCanvasElement()) {
    HTMLCanvasElement* canvas = &element.GetAsHTMLCanvasElement();
    htmlElement = canvas;

    nsIntSize size = canvas->GetSize();
    if (size.width == 0 || size.height == 0) {
      error.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
      return nullptr;
    }

    
    nsICanvasRenderingContextInternal *srcCanvas = canvas->GetContextAtIndex(0);
    if (srcCanvas) {
      
      RefPtr<SourceSurface> srcSurf = srcCanvas->GetSurfaceSnapshot();

      nsRefPtr<CanvasPattern> pat =
        new CanvasPattern(this, srcSurf, repeatMode, htmlElement->NodePrincipal(), canvas->IsWriteOnly(), false);

      return pat.forget();
    }
  } else if (element.IsHTMLImageElement()) {
    HTMLImageElement* img = &element.GetAsHTMLImageElement();
    if (img->IntrinsicState().HasState(NS_EVENT_STATE_BROKEN)) {
      error.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
      return nullptr;
    }

    htmlElement = img;
  } else {
    htmlElement = &element.GetAsHTMLVideoElement();
  }

  EnsureTarget();

  
  
  nsLayoutUtils::SurfaceFromElementResult res =
    nsLayoutUtils::SurfaceFromElement(htmlElement,
      nsLayoutUtils::SFE_WANT_FIRST_FRAME, mTarget);

  if (!res.mSourceSurface) {
    error.Throw(NS_ERROR_NOT_AVAILABLE);
    return nullptr;
  }

  nsRefPtr<CanvasPattern> pat =
    new CanvasPattern(this, res.mSourceSurface, repeatMode, res.mPrincipal,
                             res.mIsWriteOnly, res.mCORSUsed);

  return pat.forget();
}




void
CanvasRenderingContext2D::SetShadowColor(const nsAString& shadowColor)
{
  nscolor color;
  if (!ParseColor(shadowColor, &color)) {
    return;
  }

  CurrentState().shadowColor = color;
}





void
CanvasRenderingContext2D::ClearRect(double x, double y, double w,
                                    double h)
{
  if (!mTarget) {
    return;
  }

  mTarget->ClearRect(mgfx::Rect(x, y, w, h));

  RedrawUser(gfxRect(x, y, w, h));
}

void
CanvasRenderingContext2D::FillRect(double x, double y, double w,
                                   double h)
{
  const ContextState &state = CurrentState();

  if (state.patternStyles[Style::FILL]) {
    CanvasPattern::RepeatMode repeat =
      state.patternStyles[Style::FILL]->mRepeat;
    
    bool limitx = repeat == CanvasPattern::RepeatMode::NOREPEAT || repeat == CanvasPattern::RepeatMode::REPEATY;
    bool limity = repeat == CanvasPattern::RepeatMode::NOREPEAT || repeat == CanvasPattern::RepeatMode::REPEATX;

    IntSize patternSize =
      state.patternStyles[Style::FILL]->mSurface->GetSize();

    
    
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

  EnsureTarget();
  if (NeedToDrawShadow()) {
    bounds = mgfx::Rect(x, y, w, h);
    bounds = mTarget->GetTransform().TransformBounds(bounds);
  }

  AdjustedTarget(this, bounds.IsEmpty() ? nullptr : &bounds)->
    FillRect(mgfx::Rect(x, y, w, h),
             CanvasGeneralPattern().ForStyle(this, Style::FILL, mTarget),
             DrawOptions(state.globalAlpha, UsedOperation()));

  RedrawUser(gfxRect(x, y, w, h));
}

void
CanvasRenderingContext2D::StrokeRect(double x, double y, double w,
                                     double h)
{
  const ContextState &state = CurrentState();

  mgfx::Rect bounds;

  if (!w && !h) {
    return;
  }

  EnsureTarget();
  if (!IsTargetValid()) {
    return;
  }

  if (NeedToDrawShadow()) {
    bounds = mgfx::Rect(x - state.lineWidth / 2.0f, y - state.lineWidth / 2.0f,
                        w + state.lineWidth, h + state.lineWidth);
    bounds = mTarget->GetTransform().TransformBounds(bounds);
  }

  if (!h) {
    CapStyle cap = CapStyle::BUTT;
    if (state.lineJoin == JoinStyle::ROUND) {
      cap = CapStyle::ROUND;
    }
    AdjustedTarget(this, bounds.IsEmpty() ? nullptr : &bounds)->
      StrokeLine(Point(x, y), Point(x + w, y),
                  CanvasGeneralPattern().ForStyle(this, Style::STROKE, mTarget),
                  StrokeOptions(state.lineWidth, state.lineJoin,
                                cap, state.miterLimit,
                                state.dash.Length(),
                                state.dash.Elements(),
                                state.dashOffset),
                  DrawOptions(state.globalAlpha, UsedOperation()));
    return;
  }

  if (!w) {
    CapStyle cap = CapStyle::BUTT;
    if (state.lineJoin == JoinStyle::ROUND) {
      cap = CapStyle::ROUND;
    }
    AdjustedTarget(this, bounds.IsEmpty() ? nullptr : &bounds)->
      StrokeLine(Point(x, y), Point(x, y + h),
                  CanvasGeneralPattern().ForStyle(this, Style::STROKE, mTarget),
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
                CanvasGeneralPattern().ForStyle(this, Style::STROKE, mTarget),
                StrokeOptions(state.lineWidth, state.lineJoin,
                              state.lineCap, state.miterLimit,
                              state.dash.Length(),
                              state.dash.Elements(),
                              state.dashOffset),
                DrawOptions(state.globalAlpha, UsedOperation()));

  Redraw();
}





void
CanvasRenderingContext2D::BeginPath()
{
  mPath = nullptr;
  mPathBuilder = nullptr;
  mDSPathBuilder = nullptr;
  mPathTransformWillUpdate = false;
}

void
CanvasRenderingContext2D::Fill(const CanvasWindingRule& winding)
{
  EnsureUserSpacePath(winding);

  if (!mPath) {
    return;
  }

  mgfx::Rect bounds;

  if (NeedToDrawShadow()) {
    bounds = mPath->GetBounds(mTarget->GetTransform());
  }

  AdjustedTarget(this, bounds.IsEmpty() ? nullptr : &bounds)->
    Fill(mPath, CanvasGeneralPattern().ForStyle(this, Style::FILL, mTarget),
         DrawOptions(CurrentState().globalAlpha, UsedOperation()));

  Redraw();
}

void CanvasRenderingContext2D::Fill(const CanvasPath& path, const CanvasWindingRule& winding)
{
  EnsureTarget();

  RefPtr<gfx::Path> gfxpath = path.GetPath(winding, mTarget);

  if (!gfxpath) {
    return;
  }

  mgfx::Rect bounds;

  if (NeedToDrawShadow()) {
    bounds = gfxpath->GetBounds(mTarget->GetTransform());
  }

  AdjustedTarget(this, bounds.IsEmpty() ? nullptr : &bounds)->
    Fill(gfxpath, CanvasGeneralPattern().ForStyle(this, Style::FILL, mTarget),
         DrawOptions(CurrentState().globalAlpha, UsedOperation()));

  Redraw();
}

void
CanvasRenderingContext2D::Stroke()
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
    Stroke(mPath, CanvasGeneralPattern().ForStyle(this, Style::STROKE, mTarget),
           strokeOptions, DrawOptions(state.globalAlpha, UsedOperation()));

  Redraw();
}

void
CanvasRenderingContext2D::Stroke(const CanvasPath& path)
{
  EnsureTarget();

  RefPtr<gfx::Path> gfxpath = path.GetPath(CanvasWindingRule::Nonzero, mTarget);

  if (!gfxpath) {
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
      gfxpath->GetStrokedBounds(strokeOptions, mTarget->GetTransform());
  }

  AdjustedTarget(this, bounds.IsEmpty() ? nullptr : &bounds)->
    Stroke(gfxpath, CanvasGeneralPattern().ForStyle(this, Style::STROKE, mTarget),
           strokeOptions, DrawOptions(state.globalAlpha, UsedOperation()));

  Redraw();
}

void CanvasRenderingContext2D::DrawFocusIfNeeded(mozilla::dom::Element& aElement)
{
  EnsureUserSpacePath();

  if (!mPath) {
    return;
  }

  if(DrawCustomFocusRing(aElement)) {
    Save();

    
    ContextState& state = CurrentState();
    state.globalAlpha = 1.0;
    state.shadowBlur = 0;
    state.shadowOffset.x = 0;
    state.shadowOffset.y = 0;
    state.op = mozilla::gfx::CompositionOp::OP_OVER;

    state.lineCap = CapStyle::BUTT;
    state.lineJoin = mozilla::gfx::JoinStyle::MITER_OR_BEVEL;
    state.lineWidth = 1;
    CurrentState().dash.Clear();

    
    
    CurrentState().SetColorStyle(Style::STROKE, NS_RGBA(255, 255, 255, 255));
    
    Stroke();

    
    FallibleTArray<mozilla::gfx::Float>& dash = CurrentState().dash;
    dash.AppendElement(1);
    dash.AppendElement(1);

    
    CurrentState().SetColorStyle(Style::STROKE, NS_RGBA(0,0,0, 255));
    
    Stroke();

    Restore();
  }
}

bool CanvasRenderingContext2D::DrawCustomFocusRing(mozilla::dom::Element& aElement)
{
  EnsureUserSpacePath();

  HTMLCanvasElement* canvas = GetCanvas();

  if (!canvas|| !nsContentUtils::ContentIsDescendantOf(&aElement, canvas)) {
    return false;
  }

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm) {
    
    nsCOMPtr<nsIDOMElement> focusedElement;
    fm->GetFocusedElement(getter_AddRefs(focusedElement));
    if (SameCOMIdentity(aElement.AsDOMNode(), focusedElement)) {
      nsPIDOMWindow *window = aElement.OwnerDoc()->GetWindow();
      if (window) {
        return window->ShouldShowFocusRing();
      }
    }
  }

  return false;
}

void
CanvasRenderingContext2D::Clip(const CanvasWindingRule& winding)
{
  EnsureUserSpacePath(winding);

  if (!mPath) {
    return;
  }

  mTarget->PushClip(mPath);
  CurrentState().clipsPushed.push_back(mPath);
}

void
CanvasRenderingContext2D::Clip(const CanvasPath& path, const CanvasWindingRule& winding)
{
  EnsureTarget();

  RefPtr<gfx::Path> gfxpath = path.GetPath(winding, mTarget);

  if (!gfxpath) {
    return;
  }

  mTarget->PushClip(gfxpath);
  CurrentState().clipsPushed.push_back(gfxpath);
}

void
CanvasRenderingContext2D::ArcTo(double x1, double y1, double x2,
                                double y2, double radius,
                                ErrorResult& error)
{
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

  Arc(cx, cy, radius, angle0, angle1, anticlockwise, error);
}

void
CanvasRenderingContext2D::Arc(double x, double y, double r,
                              double startAngle, double endAngle,
                              bool anticlockwise, ErrorResult& error)
{
  if (r < 0.0) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  EnsureWritablePath();

  ArcToBezier(this, Point(x, y), Size(r, r), startAngle, endAngle, anticlockwise);
}

void
CanvasRenderingContext2D::Rect(double x, double y, double w, double h)
{
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

void
CanvasRenderingContext2D::EnsureWritablePath()
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

  EnsureTarget();
  if (!mPath) {
    NS_ASSERTION(!mPathTransformWillUpdate, "mPathTransformWillUpdate should be false, if all paths are null");
    mPathBuilder = mTarget->CreatePathBuilder(fillRule);
  } else if (!mPathTransformWillUpdate) {
    mPathBuilder = mPath->CopyToBuilder(fillRule);
  } else {
    mDSPathBuilder =
      mPath->TransformedCopyToBuilder(mPathToDS, fillRule);
    mPathTransformWillUpdate = false;
    mPath = nullptr;
  }
}

void
CanvasRenderingContext2D::EnsureUserSpacePath(const CanvasWindingRule& winding)
{
  FillRule fillRule = CurrentState().fillRule;
  if(winding == CanvasWindingRule::Evenodd)
    fillRule = FillRule::FILL_EVEN_ODD;

  if (!mPath && !mPathBuilder && !mDSPathBuilder) {
    EnsureTarget();
    mPathBuilder = mTarget->CreatePathBuilder(fillRule);
  }

  if (mPathBuilder) {
    mPath = mPathBuilder->Finish();
    mPathBuilder = nullptr;
  }

  if (mPath &&
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
    mPathBuilder = nullptr;
  }

  NS_ASSERTION(mPath, "mPath should exist");
}

void
CanvasRenderingContext2D::TransformWillUpdate()
{
  EnsureTarget();

  
  
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
CanvasRenderingContext2D::SetFont(const nsAString& font,
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

  nsTArray< nsCOMPtr<nsIStyleRule> > rules;
  rules.AppendElement(rule);

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

    nsTArray< nsCOMPtr<nsIStyleRule> > parentRules;
    parentRules.AppendElement(parentRule);
    parentContext = styleSet->ResolveStyleForRules(nullptr, parentRules);
  }

  if (!parentContext) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  
  rules.AppendElement(new nsDisableTextZoomStyleRule);

  nsRefPtr<nsStyleContext> sc =
      styleSet->ResolveStyleForRules(parentContext, rules);
  if (!sc) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  const nsStyleFont* fontStyle = sc->StyleFont();

  NS_ASSERTION(fontStyle, "Could not obtain font style");

  nsIAtom* language = sc->StyleFont()->mLanguage;
  if (!language) {
    language = presShell->GetPresContext()->GetLanguageFromCharset();
  }

  
  const uint32_t aupcp = nsPresContext::AppUnitsPerCSSPixel();

  bool printerFont = (presShell->GetPresContext()->Type() == nsPresContext::eContext_PrintPreview ||
                      presShell->GetPresContext()->Type() == nsPresContext::eContext_Print);

  
  
  
  
  MOZ_ASSERT(!fontStyle->mAllowZoom,
             "expected text zoom to be disabled on this nsStyleFont");
  gfxFontStyle style(fontStyle->mFont.style,
                     fontStyle->mFont.weight,
                     fontStyle->mFont.stretch,
                     NSAppUnitsToFloatPixels(fontStyle->mSize, float(aupcp)),
                     language,
                     fontStyle->mFont.sizeAdjust,
                     fontStyle->mFont.systemFont,
                     printerFont,
                     fontStyle->mFont.synthesis & NS_FONT_SYNTHESIS_WEIGHT,
                     fontStyle->mFont.synthesis & NS_FONT_SYNTHESIS_STYLE,
                     fontStyle->mFont.languageOverride);

  fontStyle->mFont.AddFontFeaturesToStyle(&style);

  nsPresContext *c = presShell->GetPresContext();
  CurrentState().fontGroup =
      gfxPlatform::GetPlatform()->CreateFontGroup(fontStyle->mFont.fontlist,
                                                  &style,
                                                  c->GetUserFontSet());
  NS_ASSERTION(CurrentState().fontGroup, "Could not get font group");
  CurrentState().fontGroup->SetTextPerfMetrics(c->GetTextPerfMetrics());

  
  
  
  
  declaration->GetValue(eCSSProperty_font, CurrentState().font);
}

void
CanvasRenderingContext2D::SetTextAlign(const nsAString& ta)
{
  if (ta.EqualsLiteral("start"))
    CurrentState().textAlign = TextAlign::START;
  else if (ta.EqualsLiteral("end"))
    CurrentState().textAlign = TextAlign::END;
  else if (ta.EqualsLiteral("left"))
    CurrentState().textAlign = TextAlign::LEFT;
  else if (ta.EqualsLiteral("right"))
    CurrentState().textAlign = TextAlign::RIGHT;
  else if (ta.EqualsLiteral("center"))
    CurrentState().textAlign = TextAlign::CENTER;
}

void
CanvasRenderingContext2D::GetTextAlign(nsAString& ta)
{
  switch (CurrentState().textAlign)
  {
  case TextAlign::START:
    ta.AssignLiteral("start");
    break;
  case TextAlign::END:
    ta.AssignLiteral("end");
    break;
  case TextAlign::LEFT:
    ta.AssignLiteral("left");
    break;
  case TextAlign::RIGHT:
    ta.AssignLiteral("right");
    break;
  case TextAlign::CENTER:
    ta.AssignLiteral("center");
    break;
  }
}

void
CanvasRenderingContext2D::SetTextBaseline(const nsAString& tb)
{
  if (tb.EqualsLiteral("top"))
    CurrentState().textBaseline = TextBaseline::TOP;
  else if (tb.EqualsLiteral("hanging"))
    CurrentState().textBaseline = TextBaseline::HANGING;
  else if (tb.EqualsLiteral("middle"))
    CurrentState().textBaseline = TextBaseline::MIDDLE;
  else if (tb.EqualsLiteral("alphabetic"))
    CurrentState().textBaseline = TextBaseline::ALPHABETIC;
  else if (tb.EqualsLiteral("ideographic"))
    CurrentState().textBaseline = TextBaseline::IDEOGRAPHIC;
  else if (tb.EqualsLiteral("bottom"))
    CurrentState().textBaseline = TextBaseline::BOTTOM;
}

void
CanvasRenderingContext2D::GetTextBaseline(nsAString& tb)
{
  switch (CurrentState().textBaseline)
  {
  case TextBaseline::TOP:
    tb.AssignLiteral("top");
    break;
  case TextBaseline::HANGING:
    tb.AssignLiteral("hanging");
    break;
  case TextBaseline::MIDDLE:
    tb.AssignLiteral("middle");
    break;
  case TextBaseline::ALPHABETIC:
    tb.AssignLiteral("alphabetic");
    break;
  case TextBaseline::IDEOGRAPHIC:
    tb.AssignLiteral("ideographic");
    break;
  case TextBaseline::BOTTOM:
    tb.AssignLiteral("bottom");
    break;
  }
}








static inline void
TextReplaceWhitespaceCharacters(nsAutoString& str)
{
  str.ReplaceChar("\x09\x0A\x0B\x0C\x0D", char16_t(' '));
}

void
CanvasRenderingContext2D::FillText(const nsAString& text, double x,
                                   double y,
                                   const Optional<double>& maxWidth,
                                   ErrorResult& error)
{
  error = DrawOrMeasureText(text, x, y, maxWidth, TextDrawOperation::FILL, nullptr);
}

void
CanvasRenderingContext2D::StrokeText(const nsAString& text, double x,
                                     double y,
                                     const Optional<double>& maxWidth,
                                     ErrorResult& error)
{
  error = DrawOrMeasureText(text, x, y, maxWidth, TextDrawOperation::STROKE, nullptr);
}

TextMetrics*
CanvasRenderingContext2D::MeasureText(const nsAString& rawText,
                                      ErrorResult& error)
{
  float width;
  Optional<double> maxWidth;
  error = DrawOrMeasureText(rawText, 0, 0, maxWidth, TextDrawOperation::MEASURE, &width);
  if (error.Failed()) {
    return nullptr;
  }

  return new TextMetrics(width);
}

void
CanvasRenderingContext2D::AddHitRegion(const HitRegionOptions& options, ErrorResult& error)
{
  
  EnsureUserSpacePath(CanvasWindingRule::Nonzero);
  if(!mPath) {
    error.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  
  mgfx::Rect bounds(mPath->GetBounds(mTarget->GetTransform()));
  if ((bounds.width == 0) || (bounds.height == 0) || !bounds.IsFinite()) {
    
    error.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  
  RemoveHitRegion(options.mId);

  if (options.mControl) {
    
    for (size_t x = 0; x < mHitRegionsOptions.Length(); x++) {
      RegionInfo& info = mHitRegionsOptions[x];
      if (info.mElement == options.mControl) {
        mHitRegionsOptions.RemoveElementAt(x);
        break;
      }
    }
#ifdef ACCESSIBILITY
  options.mControl->SetProperty(nsGkAtoms::hitregion, new bool(true),
                                nsINode::DeleteProperty<bool>);
#endif
  }

  
  RegionInfo info;
  info.mId = options.mId;
  info.mElement = options.mControl;
  RefPtr<PathBuilder> pathBuilder = mPath->TransformedCopyToBuilder(mTarget->GetTransform());
  info.mPath = pathBuilder->Finish();

  mHitRegionsOptions.InsertElementAt(0, info);
}

void
CanvasRenderingContext2D::RemoveHitRegion(const nsAString& id)
{
  if (id.Length() == 0) {
     return;
   }

  for (size_t x = 0; x < mHitRegionsOptions.Length(); x++) {
    RegionInfo& info = mHitRegionsOptions[x];
    if (info.mId == id) {
      mHitRegionsOptions.RemoveElementAt(x);

      return;
    }
  }
}

bool
CanvasRenderingContext2D::GetHitRegionRect(Element* aElement, nsRect& aRect)
{
  for (unsigned int x = 0; x < mHitRegionsOptions.Length(); x++) {
    RegionInfo& info = mHitRegionsOptions[x];
    if (info.mElement == aElement) {
      mgfx::Rect bounds(info.mPath->GetBounds());
      gfxRect rect(bounds.x, bounds.y, bounds.width, bounds.height);
      aRect = nsLayoutUtils::RoundGfxRectToAppRect(rect, AppUnitsPerCSSPixel());

      return true;
    }
  }

  return false;
}




struct MOZ_STACK_CLASS CanvasBidiProcessor : public nsBidiPresUtils::BidiProcessor
{
  typedef CanvasRenderingContext2D::ContextState ContextState;

  virtual void SetText(const char16_t* text, int32_t length, nsBidiDirection direction)
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
    const int32_t appUnitsPerDevUnit = mAppUnitsPerDevPixel;
    const double devUnitsPerAppUnit = 1.0/double(appUnitsPerDevUnit);
    Point baselineOrigin =
      Point(point.x * devUnitsPerAppUnit, point.y * devUnitsPerAppUnit);

    float advanceSum = 0;

    mCtx->EnsureTarget();
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

      RefPtr<GlyphRenderingOptions> renderingOptions = font->GetGlyphRenderingOptions();

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

        if (glyphs[i].IsMissing()) {
          newGlyph.mIndex = 0;
          if (mTextRun->IsRightToLeft()) {
            newGlyph.mPosition.x = baselineOrigin.x - advanceSum -
              detailedGlyphs[0].mAdvance * devUnitsPerAppUnit;
          } else {
            newGlyph.mPosition.x = baselineOrigin.x + advanceSum;
          }
          newGlyph.mPosition.y = baselineOrigin.y;
          advanceSum += detailedGlyphs[0].mAdvance * devUnitsPerAppUnit;
          glyphBuf.push_back(newGlyph);
          continue;
        }

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

      Rect bounds = mCtx->mTarget->GetTransform().
        TransformBounds(Rect(mBoundingBox.x, mBoundingBox.y,
                             mBoundingBox.width, mBoundingBox.height));
      if (mOp == CanvasRenderingContext2D::TextDrawOperation::FILL) {
        AdjustedTarget(mCtx, &bounds)->
          FillGlyphs(scaledFont, buffer,
                     CanvasGeneralPattern().
                       ForStyle(mCtx, CanvasRenderingContext2D::Style::FILL, mCtx->mTarget),
                     DrawOptions(mState->globalAlpha, mCtx->UsedOperation()),
                     renderingOptions);
      } else if (mOp == CanvasRenderingContext2D::TextDrawOperation::STROKE) {
        
        
        buffer.mGlyphs = &glyphBuf.front();
        buffer.mNumGlyphs = 1;
        const ContextState& state = *mState;
        AdjustedTarget target(mCtx, &bounds);
        const StrokeOptions strokeOpts(state.lineWidth, state.lineJoin,
                                       state.lineCap, state.miterLimit,
                                       state.dash.Length(),
                                       state.dash.Elements(),
                                       state.dashOffset);
        CanvasGeneralPattern cgp;
        const Pattern& patForStyle
          (cgp.ForStyle(mCtx, CanvasRenderingContext2D::Style::STROKE, mCtx->mTarget));
        const DrawOptions drawOpts(state.globalAlpha, mCtx->UsedOperation());

        for (unsigned i = glyphBuf.size(); i > 0; --i) {
          RefPtr<Path> path = scaledFont->GetPathForGlyphs(buffer, mCtx->mTarget);
          target->Stroke(path, patForStyle, strokeOpts, drawOpts);
          buffer.mGlyphs++;
        }
      }
    }
  }

  
  nsAutoPtr<gfxTextRun> mTextRun;

  
  nsRefPtr<gfxContext> mThebes;

  
  CanvasRenderingContext2D *mCtx;

  
  gfxPoint mPt;

  
  gfxFontGroup* mFontgrp;

  
  int32_t mAppUnitsPerDevPixel;

  
  CanvasRenderingContext2D::TextDrawOperation mOp;

  
  ContextState *mState;

  
  gfxRect mBoundingBox;

  
  bool mDoMeasureBoundingBox;
};

nsresult
CanvasRenderingContext2D::DrawOrMeasureText(const nsAString& aRawText,
                                            float aX,
                                            float aY,
                                            const Optional<double>& aMaxWidth,
                                            TextDrawOperation aOp,
                                            float* aWidth)
{
  nsresult rv;

  
  
  
  
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

    isRTL = canvasStyle->StyleVisibility()->mDirection ==
      NS_STYLE_DIRECTION_RTL;
  } else {
    isRTL = GET_BIDI_OPTION_DIRECTION(document->GetBidiOptions()) == IBMBIDI_TEXTDIRECTION_RTL;
  }

  gfxFontGroup* currentFontStyle = GetCurrentFontStyle();
  NS_ASSERTION(currentFontStyle, "font group is null");

  
  currentFontStyle->
    SetUserFontSet(presShell->GetPresContext()->GetUserFontSet());

  if (currentFontStyle->GetStyle()->size == 0.0F) {
    if (aWidth) {
      *aWidth = 0;
    }
    return NS_OK;
  }

  const ContextState &state = CurrentState();

  
  bool doDrawShadow = NeedToDrawShadow();

  CanvasBidiProcessor processor;

  GetAppUnitsValues(&processor.mAppUnitsPerDevPixel, nullptr);
  processor.mPt = gfxPoint(aX, aY);
  processor.mThebes =
    new gfxContext(gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget());

  
  
  
  if (mTarget) {
    Matrix matrix = mTarget->GetTransform();
    processor.mThebes->SetMatrix(gfxMatrix(matrix._11, matrix._12, matrix._21, matrix._22, matrix._31, matrix._32));
  }
  processor.mCtx = this;
  processor.mOp = aOp;
  processor.mBoundingBox = gfxRect(0, 0, 0, 0);
  processor.mDoMeasureBoundingBox = doDrawShadow || !mIsEntireFrameInvalid;
  processor.mState = &CurrentState();
  processor.mFontgrp = currentFontStyle;

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

  
  if (aOp==TextDrawOperation::MEASURE) {
    return NS_OK;
  }

  
  gfxFloat anchorX;

  if (state.textAlign == TextAlign::CENTER) {
    anchorX = .5;
  } else if (state.textAlign == TextAlign::LEFT ||
            (!isRTL && state.textAlign == TextAlign::START) ||
            (isRTL && state.textAlign == TextAlign::END)) {
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
  case TextBaseline::HANGING:
      
  case TextBaseline::TOP:
    anchorY = fontMetrics.emAscent;
    break;
  case TextBaseline::MIDDLE:
    anchorY = (fontMetrics.emAscent - fontMetrics.emDescent) * .5f;
    break;
  case TextBaseline::IDEOGRAPHIC:
    
  case TextBaseline::ALPHABETIC:
    anchorY = 0;
    break;
  case TextBaseline::BOTTOM:
    anchorY = -fontMetrics.emDescent;
    break;
  default:
    MOZ_CRASH("unexpected TextBaseline");
  }

  processor.mPt.y += anchorY;

  
  processor.mBoundingBox.width = totalWidth;
  processor.mBoundingBox.MoveBy(processor.mPt);

  processor.mPt.x *= processor.mAppUnitsPerDevPixel;
  processor.mPt.y *= processor.mAppUnitsPerDevPixel;

  EnsureTarget();
  Matrix oldTransform = mTarget->GetTransform();
  
  
  if (aMaxWidth.WasPassed() && aMaxWidth.Value() > 0 &&
      totalWidth > aMaxWidth.Value()) {
    Matrix newTransform = oldTransform;

    
    
    newTransform.PreTranslate(aX, 0);
    newTransform.PreScale(aMaxWidth.Value() / totalWidth, 1);
    newTransform.PreTranslate(-aX, 0);
    
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

  if (aOp == CanvasRenderingContext2D::TextDrawOperation::FILL &&
      !doDrawShadow) {
    RedrawUser(boundingBox);
    return NS_OK;
  }

  Redraw();
  return NS_OK;
}

gfxFontGroup *CanvasRenderingContext2D::GetCurrentFontStyle()
{
  
  if (!CurrentState().fontGroup) {
    ErrorResult err;
    NS_NAMED_LITERAL_STRING(kDefaultFontStyle, "10px sans-serif");
    static float kDefaultFontSize = 10.0;
    SetFont(kDefaultFontStyle, err);
    if (err.Failed()) {
      gfxFontStyle style;
      style.size = kDefaultFontSize;
      CurrentState().fontGroup =
        gfxPlatform::GetPlatform()->CreateFontGroup(FontFamilyList(eFamily_sans_serif),
                                                    &style,
                                                    nullptr);
      if (CurrentState().fontGroup) {
        CurrentState().font = kDefaultFontStyle;

        nsIPresShell* presShell = GetPresShell();
        if (presShell) {
          CurrentState().fontGroup->SetTextPerfMetrics(
            presShell->GetPresContext()->GetTextPerfMetrics());
        }
      } else {
        NS_ERROR("Default canvas font is invalid");
      }
    }

  }

  return CurrentState().fontGroup;
}





void
CanvasRenderingContext2D::SetLineCap(const nsAString& capstyle)
{
  CapStyle cap;

  if (capstyle.EqualsLiteral("butt")) {
    cap = CapStyle::BUTT;
  } else if (capstyle.EqualsLiteral("round")) {
    cap = CapStyle::ROUND;
  } else if (capstyle.EqualsLiteral("square")) {
    cap = CapStyle::SQUARE;
  } else {
    
    return;
  }

  CurrentState().lineCap = cap;
}

void
CanvasRenderingContext2D::GetLineCap(nsAString& capstyle)
{
  switch (CurrentState().lineCap) {
  case CapStyle::BUTT:
    capstyle.AssignLiteral("butt");
    break;
  case CapStyle::ROUND:
    capstyle.AssignLiteral("round");
    break;
  case CapStyle::SQUARE:
    capstyle.AssignLiteral("square");
    break;
  }
}

void
CanvasRenderingContext2D::SetLineJoin(const nsAString& joinstyle)
{
  JoinStyle j;

  if (joinstyle.EqualsLiteral("round")) {
    j = JoinStyle::ROUND;
  } else if (joinstyle.EqualsLiteral("bevel")) {
    j = JoinStyle::BEVEL;
  } else if (joinstyle.EqualsLiteral("miter")) {
    j = JoinStyle::MITER_OR_BEVEL;
  } else {
    
    return;
  }

  CurrentState().lineJoin = j;
}

void
CanvasRenderingContext2D::GetLineJoin(nsAString& joinstyle, ErrorResult& error)
{
  switch (CurrentState().lineJoin) {
  case JoinStyle::ROUND:
    joinstyle.AssignLiteral("round");
    break;
  case JoinStyle::BEVEL:
    joinstyle.AssignLiteral("bevel");
    break;
  case JoinStyle::MITER_OR_BEVEL:
    joinstyle.AssignLiteral("miter");
    break;
  default:
    error.Throw(NS_ERROR_FAILURE);
  }
}

void
CanvasRenderingContext2D::SetMozDash(JSContext* cx,
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

void
CanvasRenderingContext2D::GetMozDash(JSContext* cx,
                                     JS::MutableHandle<JS::Value> retval,
                                     ErrorResult& error)
{
  DashArrayToJSVal(CurrentState().dash, cx, retval, error);
}

void
CanvasRenderingContext2D::SetMozDashOffset(double mozDashOffset)
{
  ContextState& state = CurrentState();
  if (!state.dash.IsEmpty()) {
    state.dashOffset = mozDashOffset;
  }
}

void
CanvasRenderingContext2D::SetLineDash(const Sequence<double>& aSegments)
{
  FallibleTArray<mozilla::gfx::Float> dash;

  for (uint32_t x = 0; x < aSegments.Length(); x++) {
    if (aSegments[x] < 0.0) {
      
      
      return;
    }
    dash.AppendElement(aSegments[x]);
  }
  if (aSegments.Length() % 2) { 
    for (uint32_t x = 0; x < aSegments.Length(); x++) {
      dash.AppendElement(aSegments[x]);
    }
  }

  CurrentState().dash = dash;
}

void
CanvasRenderingContext2D::GetLineDash(nsTArray<double>& aSegments) const {
  const FallibleTArray<mozilla::gfx::Float>& dash = CurrentState().dash;
  aSegments.Clear();

  for (uint32_t x = 0; x < dash.Length(); x++) {
    aSegments.AppendElement(dash[x]);
  }
}

void
CanvasRenderingContext2D::SetLineDashOffset(double mOffset) {
  CurrentState().dashOffset = mOffset;
}

double
CanvasRenderingContext2D::LineDashOffset() const {
  return CurrentState().dashOffset;
}

bool
CanvasRenderingContext2D::IsPointInPath(double x, double y, const CanvasWindingRule& winding)
{
  if (!FloatValidate(x,y)) {
    return false;
  }

  EnsureUserSpacePath(winding);
  if (!mPath) {
    return false;
  }

  if (mPathTransformWillUpdate) {
    return mPath->ContainsPoint(Point(x, y), mPathToDS);
  }

  return mPath->ContainsPoint(Point(x, y), mTarget->GetTransform());
}

bool CanvasRenderingContext2D::IsPointInPath(const CanvasPath& mPath, double x, double y, const CanvasWindingRule& mWinding)
{
  if (!FloatValidate(x,y)) {
    return false;
  }

  EnsureTarget();
  RefPtr<gfx::Path> tempPath = mPath.GetPath(mWinding, mTarget);

  return tempPath->ContainsPoint(Point(x, y), mTarget->GetTransform());
}

bool
CanvasRenderingContext2D::IsPointInStroke(double x, double y)
{
  if (!FloatValidate(x,y)) {
    return false;
  }

  EnsureUserSpacePath();
  if (!mPath) {
    return false;
  }

  const ContextState &state = CurrentState();

  StrokeOptions strokeOptions(state.lineWidth,
                              state.lineJoin,
                              state.lineCap,
                              state.miterLimit,
                              state.dash.Length(),
                              state.dash.Elements(),
                              state.dashOffset);

  if (mPathTransformWillUpdate) {
    return mPath->StrokeContainsPoint(strokeOptions, Point(x, y), mPathToDS);
  }
  return mPath->StrokeContainsPoint(strokeOptions, Point(x, y), mTarget->GetTransform());
}

bool CanvasRenderingContext2D::IsPointInStroke(const CanvasPath& mPath, double x, double y)
{
  if (!FloatValidate(x,y)) {
    return false;
  }

  EnsureTarget();
  RefPtr<gfx::Path> tempPath = mPath.GetPath(CanvasWindingRule::Nonzero, mTarget);

  const ContextState &state = CurrentState();

  StrokeOptions strokeOptions(state.lineWidth,
                              state.lineJoin,
                              state.lineCap,
                              state.miterLimit,
                              state.dash.Length(),
                              state.dash.Elements(),
                              state.dashOffset);

  return tempPath->StrokeContainsPoint(strokeOptions, Point(x, y), mTarget->GetTransform());
}

















void
CanvasRenderingContext2D::DrawImage(const HTMLImageOrCanvasOrVideoElement& image,
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

  EnsureTarget();
  if (image.IsHTMLCanvasElement()) {
    HTMLCanvasElement* canvas = &image.GetAsHTMLCanvasElement();
    element = canvas;
    nsIntSize size = canvas->GetSize();
    if (size.width == 0 || size.height == 0) {
      error.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
      return;
    }
  } else {
    if (image.IsHTMLImageElement()) {
      HTMLImageElement* img = &image.GetAsHTMLImageElement();
      element = img;
    } else {
      HTMLVideoElement* video = &image.GetAsHTMLVideoElement();
      element = video;
    }

    srcSurf =
      CanvasImageCache::Lookup(element, mCanvasElement, &imgSize);
  }

  nsLayoutUtils::DirectDrawInfo drawInfo;

  if (!srcSurf) {
    
    
    uint32_t sfeFlags = nsLayoutUtils::SFE_WANT_FIRST_FRAME |
                        nsLayoutUtils::SFE_NO_RASTERIZING_VECTORS;
    nsLayoutUtils::SurfaceFromElementResult res =
      nsLayoutUtils::SurfaceFromElement(element, sfeFlags, mTarget);

    if (!res.mSourceSurface && !res.mDrawInfo.mImgContainer) {
      
      if (!res.mIsStillLoading) {
        error.Throw(NS_ERROR_NOT_AVAILABLE);
      }
      return;
    }

    imgSize = res.mSize;

    
    if (image.IsHTMLVideoElement()) {
      HTMLVideoElement* video = &image.GetAsHTMLVideoElement();
      int32_t displayWidth = video->VideoWidth();
      int32_t displayHeight = video->VideoHeight();
      sw *= (double)imgSize.width / (double)displayWidth;
      sh *= (double)imgSize.height / (double)displayHeight;
    }

    if (mCanvasElement) {
      CanvasUtils::DoDrawImageSecurityCheck(mCanvasElement,
                                            res.mPrincipal, res.mIsWriteOnly,
                                            res.mCORSUsed);
    }

    if (res.mSourceSurface) {
      if (res.mImageRequest) {
        CanvasImageCache::NotifyDrawImage(element, mCanvasElement, res.mImageRequest,
                                          res.mSourceSurface, imgSize);
      }

      srcSurf = res.mSourceSurface;
    } else {
      drawInfo = res.mDrawInfo;
    }
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

  if (sw == 0.0 || sh == 0.0) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
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
    filter = mgfx::Filter::LINEAR;
  else
    filter = mgfx::Filter::POINT;

  mgfx::Rect bounds;

  if (NeedToDrawShadow()) {
    bounds = mgfx::Rect(dx, dy, dw, dh);
    bounds = mTarget->GetTransform().TransformBounds(bounds);
  }

  if (srcSurf) {
    AdjustedTarget(this, bounds.IsEmpty() ? nullptr : &bounds)->
      DrawSurface(srcSurf,
                  mgfx::Rect(dx, dy, dw, dh),
                  mgfx::Rect(sx, sy, sw, sh),
                  DrawSurfaceOptions(filter),
                  DrawOptions(CurrentState().globalAlpha, UsedOperation()));
  } else {
    DrawDirectlyToCanvas(drawInfo, &bounds,
                         mgfx::Rect(dx, dy, dw, dh),
                         mgfx::Rect(sx, sy, sw, sh),
                         imgSize);
  }

  RedrawUser(gfxRect(dx, dy, dw, dh));
}

void
CanvasRenderingContext2D::DrawDirectlyToCanvas(
                          const nsLayoutUtils::DirectDrawInfo& image,
                          mgfx::Rect* bounds,
                          mgfx::Rect dest,
                          mgfx::Rect src,
                          gfxIntSize imgSize)
{
  MOZ_ASSERT(src.width > 0 && src.height > 0,
             "Need positive source width and height");

  gfxMatrix contextMatrix;
  AdjustedTarget tempTarget(this, bounds->IsEmpty() ? nullptr: bounds);

  
  
  if (tempTarget) {
    Matrix matrix = tempTarget->GetTransform();
    contextMatrix = gfxMatrix(matrix._11, matrix._12, matrix._21,
                              matrix._22, matrix._31, matrix._32);
  }
  gfxSize contextScale(contextMatrix.ScaleFactors(true));

  
  dest.Scale(contextScale.width, contextScale.height);

  
  gfxSize scale(dest.width / src.width, dest.height / src.height);
  nsIntSize scaledImageSize(std::ceil(imgSize.width * scale.width),
                            std::ceil(imgSize.height * scale.height));
  src.Scale(scale.width, scale.height);

  nsRefPtr<gfxContext> context = new gfxContext(tempTarget);
  context->SetMatrix(contextMatrix.
                       Scale(1.0 / contextScale.width,
                             1.0 / contextScale.height).
                       Translate(dest.x - src.x, dest.y - src.y));

  
  uint32_t modifiedFlags = image.mDrawingFlags | imgIContainer::FLAG_CLAMP;

  SVGImageContext svgContext(scaledImageSize, Nothing(), CurrentState().globalAlpha);

  nsresult rv = image.mImgContainer->
    Draw(context, scaledImageSize,
         ImageRegion::Create(gfxRect(src.x, src.y, src.width, src.height)),
         image.mWhichFrame, GraphicsFilter::FILTER_GOOD,
         Some(svgContext), modifiedFlags);

  NS_ENSURE_SUCCESS_VOID(rv);
}

void
CanvasRenderingContext2D::SetGlobalCompositeOperation(const nsAString& op,
                                                      ErrorResult& error)
{
  CompositionOp comp_op;

#define CANVAS_OP_TO_GFX_OP(cvsop, op2d) \
  if (op.EqualsLiteral(cvsop))   \
    comp_op = CompositionOp::OP_##op2d;

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
  else CANVAS_OP_TO_GFX_OP("multiply", MULTIPLY)
  else CANVAS_OP_TO_GFX_OP("screen", SCREEN)
  else CANVAS_OP_TO_GFX_OP("overlay", OVERLAY)
  else CANVAS_OP_TO_GFX_OP("darken", DARKEN)
  else CANVAS_OP_TO_GFX_OP("lighten", LIGHTEN)
  else CANVAS_OP_TO_GFX_OP("color-dodge", COLOR_DODGE)
  else CANVAS_OP_TO_GFX_OP("color-burn", COLOR_BURN)
  else CANVAS_OP_TO_GFX_OP("hard-light", HARD_LIGHT)
  else CANVAS_OP_TO_GFX_OP("soft-light", SOFT_LIGHT)
  else CANVAS_OP_TO_GFX_OP("difference", DIFFERENCE)
  else CANVAS_OP_TO_GFX_OP("exclusion", EXCLUSION)
  else CANVAS_OP_TO_GFX_OP("hue", HUE)
  else CANVAS_OP_TO_GFX_OP("saturation", SATURATION)
  else CANVAS_OP_TO_GFX_OP("color", COLOR)
  else CANVAS_OP_TO_GFX_OP("luminosity", LUMINOSITY)
  
  else return;

#undef CANVAS_OP_TO_GFX_OP
  CurrentState().op = comp_op;
}

void
CanvasRenderingContext2D::GetGlobalCompositeOperation(nsAString& op,
                                                      ErrorResult& error)
{
  CompositionOp comp_op = CurrentState().op;

#define CANVAS_OP_TO_GFX_OP(cvsop, op2d) \
  if (comp_op == CompositionOp::OP_##op2d) \
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
  else CANVAS_OP_TO_GFX_OP("multiply", MULTIPLY)
  else CANVAS_OP_TO_GFX_OP("screen", SCREEN)
  else CANVAS_OP_TO_GFX_OP("overlay", OVERLAY)
  else CANVAS_OP_TO_GFX_OP("darken", DARKEN)
  else CANVAS_OP_TO_GFX_OP("lighten", LIGHTEN)
  else CANVAS_OP_TO_GFX_OP("color-dodge", COLOR_DODGE)
  else CANVAS_OP_TO_GFX_OP("color-burn", COLOR_BURN)
  else CANVAS_OP_TO_GFX_OP("hard-light", HARD_LIGHT)
  else CANVAS_OP_TO_GFX_OP("soft-light", SOFT_LIGHT)
  else CANVAS_OP_TO_GFX_OP("difference", DIFFERENCE)
  else CANVAS_OP_TO_GFX_OP("exclusion", EXCLUSION)
  else CANVAS_OP_TO_GFX_OP("hue", HUE)
  else CANVAS_OP_TO_GFX_OP("saturation", SATURATION)
  else CANVAS_OP_TO_GFX_OP("color", COLOR)
  else CANVAS_OP_TO_GFX_OP("luminosity", LUMINOSITY)
  else {
    error.Throw(NS_ERROR_FAILURE);
  }

#undef CANVAS_OP_TO_GFX_OP
}

void
CanvasRenderingContext2D::DrawWindow(nsGlobalWindow& window, double x,
                                     double y, double w, double h,
                                     const nsAString& bgColor,
                                     uint32_t flags, ErrorResult& error)
{
  
  
  if (!gfxASurface::CheckSurfaceSize(gfxIntSize(int32_t(w), int32_t(h)),
                                     0xffff)) {
    error.Throw(NS_ERROR_FAILURE);
    return;
  }

  EnsureTarget();
  
  
  
  
  
  
  if (!nsContentUtils::IsCallerChrome()) {
    
    
    error.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

  
  if (!(flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DO_NOT_FLUSH)) {
    nsContentUtils::FlushLayoutForTree(&window);
  }

  nsRefPtr<nsPresContext> presContext;
  nsIDocShell* docshell = window.GetDocShell();
  if (docshell) {
    docshell->GetPresContext(getter_AddRefs(presContext));
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
  if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DO_NOT_FLUSH) {
    renderDocFlags |= nsIPresShell::RENDER_DRAWWINDOW_NOT_FLUSHING;
  }

  
  
  Matrix matrix = mTarget->GetTransform();
  double sw = matrix._11 * w;
  double sh = matrix._22 * h;
  if (!sw || !sh) {
    return;
  }
  nsRefPtr<gfxContext> thebes;
  RefPtr<DrawTarget> drawDT;
  
  
  if (gfxPlatform::GetPlatform()->SupportsAzureContentForDrawTarget(mTarget) &&
      GlobalAlpha() == 1.0f)
  {
    thebes = new gfxContext(mTarget);
    thebes->SetMatrix(gfxMatrix(matrix._11, matrix._12, matrix._21,
                                matrix._22, matrix._31, matrix._32));
  } else {
    drawDT =
      gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(IntSize(ceil(sw), ceil(sh)),
                                                                   SurfaceFormat::B8G8R8A8);
    if (!drawDT) {
      error.Throw(NS_ERROR_FAILURE);
      return;
    }

    thebes = new gfxContext(drawDT);
    thebes->SetMatrix(gfxMatrix::Scaling(matrix._11, matrix._22));
  }

  nsCOMPtr<nsIPresShell> shell = presContext->PresShell();
  unused << shell->RenderDocument(r, renderDocFlags, backgroundColor, thebes);
  if (drawDT) {
    RefPtr<SourceSurface> snapshot = drawDT->Snapshot();
    RefPtr<DataSourceSurface> data = snapshot->GetDataSurface();

    RefPtr<SourceSurface> source =
      mTarget->CreateSourceSurfaceFromData(data->GetData(),
                                           data->GetSize(),
                                           data->Stride(),
                                           data->GetFormat());

    if (!source) {
      error.Throw(NS_ERROR_FAILURE);
      return;
    }

    mgfx::Rect destRect(0, 0, w, h);
    mgfx::Rect sourceRect(0, 0, sw, sh);
    mTarget->DrawSurface(source, destRect, sourceRect,
                         DrawSurfaceOptions(mgfx::Filter::POINT),
                         DrawOptions(GlobalAlpha(), CompositionOp::OP_OVER,
                                     AntialiasMode::NONE));
    mTarget->Flush();
  } else {
    mTarget->SetTransform(matrix);
  }

  
  
  
  RedrawUser(gfxRect(0, 0, w, h));
}

void
CanvasRenderingContext2D::AsyncDrawXULElement(nsXULElement& elem,
                                              double x, double y,
                                              double w, double h,
                                              const nsAString& bgColor,
                                              uint32_t flags,
                                              ErrorResult& error)
{
  
  
  
  
  
  
  if (!nsContentUtils::IsCallerChrome()) {
    
    
    error.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

#if 0
  nsCOMPtr<nsIFrameLoaderOwner> loaderOwner = do_QueryInterface(&elem);
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
    nsIDocShell* docShell = frameLoader->GetExistingDocShell();
    if (!docShell) {
      error.Throw(NS_ERROR_FAILURE);
      return;
    }

    nsCOMPtr<nsIDOMWindow> window = docShell->GetWindow();
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





already_AddRefed<ImageData>
CanvasRenderingContext2D::GetImageData(JSContext* aCx, double aSx,
                                       double aSy, double aSw,
                                       double aSh, ErrorResult& error)
{
  EnsureTarget();
  if (!IsTargetValid()) {
    error.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  if (!mCanvasElement && !mDocShell) {
    NS_ERROR("No canvas element and no docshell in GetImageData!!!");
    error.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return nullptr;
  }

  
  
  if (mCanvasElement && mCanvasElement->IsWriteOnly() &&
      !nsContentUtils::IsCallerChrome())
  {
    
    error.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return nullptr;
  }

  if (!NS_finite(aSx) || !NS_finite(aSy) ||
      !NS_finite(aSw) || !NS_finite(aSh)) {
    error.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return nullptr;
  }

  if (!aSw || !aSh) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
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

  JS::Rooted<JSObject*> array(aCx);
  error = GetImageDataArray(aCx, x, y, w, h, array.address());
  if (error.Failed()) {
    return nullptr;
  }
  MOZ_ASSERT(array);

  nsRefPtr<ImageData> imageData = new ImageData(w, h, *array);
  return imageData.forget();
}

nsresult
CanvasRenderingContext2D::GetImageDataArray(JSContext* aCx,
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

  IntRect srcRect(0, 0, mWidth, mHeight);
  IntRect destRect(aX, aY, aWidth, aHeight);
  IntRect srcReadRect = srcRect.Intersect(destRect);
  RefPtr<DataSourceSurface> readback;
  if (!srcReadRect.IsEmpty() && !mZero) {
    RefPtr<SourceSurface> snapshot = mTarget->Snapshot();
    if (snapshot) {
      readback = snapshot->GetDataSurface();
    }
    if (!readback || !readback->GetData()) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  JS::Rooted<JSObject*> darray(aCx, JS_NewUint8ClampedArray(aCx, len.value()));
  if (!darray) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (mZero) {
    *aRetval = darray;
    return NS_OK;
  }

  uint8_t* data = JS_GetUint8ClampedArrayData(darray);

  IntRect dstWriteRect = srcReadRect;
  dstWriteRect.MoveBy(-aX, -aY);

  uint8_t* src = data;
  uint32_t srcStride = aWidth * 4;
  if (readback) {
    srcStride = readback->Stride();
    src = readback->GetData() + srcReadRect.y * srcStride + srcReadRect.x * 4;
  }

  
  
  
  
  uint8_t* dst = data + dstWriteRect.y * (aWidth * 4) + dstWriteRect.x * 4;

  if (mOpaque) {
    for (int32_t j = 0; j < dstWriteRect.height; ++j) {
      for (int32_t i = 0; i < dstWriteRect.width; ++i) {
        
#if MOZ_LITTLE_ENDIAN
        uint8_t b = *src++;
        uint8_t g = *src++;
        uint8_t r = *src++;
        src++;
#else
        src++;
        uint8_t r = *src++;
        uint8_t g = *src++;
        uint8_t b = *src++;
#endif
        *dst++ = r;
        *dst++ = g;
        *dst++ = b;
        *dst++ = 255;
      }
      src += srcStride - (dstWriteRect.width * 4);
      dst += (aWidth * 4) - (dstWriteRect.width * 4);
    }
  } else
  for (int32_t j = 0; j < dstWriteRect.height; ++j) {
    for (int32_t i = 0; i < dstWriteRect.width; ++i) {
      
#if MOZ_LITTLE_ENDIAN
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
      
      *dst++ = gfxUtils::sUnpremultiplyTable[a * 256 + r];
      *dst++ = gfxUtils::sUnpremultiplyTable[a * 256 + g];
      *dst++ = gfxUtils::sUnpremultiplyTable[a * 256 + b];
      *dst++ = a;
    }
    src += srcStride - (dstWriteRect.width * 4);
    dst += (aWidth * 4) - (dstWriteRect.width * 4);
  }

  *aRetval = darray;
  return NS_OK;
}

void
CanvasRenderingContext2D::EnsureErrorTarget()
{
  if (sErrorTarget) {
    return;
  }

  RefPtr<DrawTarget> errorTarget = gfxPlatform::GetPlatform()->CreateOffscreenCanvasDrawTarget(IntSize(1, 1), SurfaceFormat::B8G8R8A8);
  MOZ_ASSERT(errorTarget, "Failed to allocate the error target!");

  sErrorTarget = errorTarget;
  NS_ADDREF(sErrorTarget);
}

void
CanvasRenderingContext2D::FillRuleChanged()
{
  if (mPath) {
    mPathBuilder = mPath->CopyToBuilder(CurrentState().fillRule);
    mPath = nullptr;
  }
}

void
CanvasRenderingContext2D::PutImageData(ImageData& imageData, double dx,
                                       double dy, ErrorResult& error)
{
  dom::Uint8ClampedArray arr;
  DebugOnly<bool> inited = arr.Init(imageData.GetDataObject());
  MOZ_ASSERT(inited);

  error = PutImageData_explicit(JS_DoubleToInt32(dx), JS_DoubleToInt32(dy),
                                imageData.Width(), imageData.Height(),
                                &arr, false, 0, 0, 0, 0);
}

void
CanvasRenderingContext2D::PutImageData(ImageData& imageData, double dx,
                                       double dy, double dirtyX,
                                       double dirtyY, double dirtyWidth,
                                       double dirtyHeight,
                                       ErrorResult& error)
{
  dom::Uint8ClampedArray arr;
  DebugOnly<bool> inited = arr.Init(imageData.GetDataObject());
  MOZ_ASSERT(inited);

  error = PutImageData_explicit(JS_DoubleToInt32(dx), JS_DoubleToInt32(dy),
                                imageData.Width(), imageData.Height(),
                                &arr, true,
                                JS_DoubleToInt32(dirtyX),
                                JS_DoubleToInt32(dirtyY),
                                JS_DoubleToInt32(dirtyWidth),
                                JS_DoubleToInt32(dirtyHeight));
}




nsresult
CanvasRenderingContext2D::PutImageData_explicit(int32_t x, int32_t y, uint32_t w, uint32_t h,
                                                dom::Uint8ClampedArray* aArray,
                                                bool hasDirtyRect, int32_t dirtyX, int32_t dirtyY,
                                                int32_t dirtyWidth, int32_t dirtyHeight)
{
  if (w == 0 || h == 0) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
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

  aArray->ComputeLengthAndData();

  uint32_t dataLen = aArray->Length();

  uint32_t len = w * h * 4;
  if (dataLen != len) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  nsRefPtr<gfxImageSurface> imgsurf = new gfxImageSurface(gfxIntSize(w, h),
                                                          gfxImageFormat::ARGB32,
                                                          false);
  if (!imgsurf || imgsurf->CairoStatus()) {
    return NS_ERROR_FAILURE;
  }

  uint8_t *src = aArray->Data();
  uint8_t *dst = imgsurf->Data();

  for (uint32_t j = 0; j < h; j++) {
    for (uint32_t i = 0; i < w; i++) {
      uint8_t r = *src++;
      uint8_t g = *src++;
      uint8_t b = *src++;
      uint8_t a = *src++;

      
#if MOZ_LITTLE_ENDIAN
      *dst++ = gfxUtils::sPremultiplyTable[a * 256 + b];
      *dst++ = gfxUtils::sPremultiplyTable[a * 256 + g];
      *dst++ = gfxUtils::sPremultiplyTable[a * 256 + r];
      *dst++ = a;
#else
      *dst++ = a;
      *dst++ = gfxUtils::sPremultiplyTable[a * 256 + r];
      *dst++ = gfxUtils::sPremultiplyTable[a * 256 + g];
      *dst++ = gfxUtils::sPremultiplyTable[a * 256 + b];
#endif
    }
  }

  EnsureTarget();
  if (!IsTargetValid()) {
    return NS_ERROR_FAILURE;
  }

  RefPtr<SourceSurface> sourceSurface =
    mTarget->CreateSourceSurfaceFromData(imgsurf->Data(), IntSize(w, h), imgsurf->Stride(), SurfaceFormat::B8G8R8A8);

  
  
  
  
  if (!sourceSurface) {
    return NS_ERROR_FAILURE;
  }

  mTarget->CopySurface(sourceSurface,
                       IntRect(dirtyRect.x - x, dirtyRect.y - y,
                               dirtyRect.width, dirtyRect.height),
                       IntPoint(dirtyRect.x, dirtyRect.y));

  Redraw(mgfx::Rect(dirtyRect.x, dirtyRect.y, dirtyRect.width, dirtyRect.height));

  return NS_OK;
}

static already_AddRefed<ImageData>
CreateImageData(JSContext* cx, CanvasRenderingContext2D* context,
                uint32_t w, uint32_t h, ErrorResult& error)
{
  if (w == 0)
      w = 1;
  if (h == 0)
      h = 1;

  CheckedInt<uint32_t> len = CheckedInt<uint32_t>(w) * h * 4;
  if (!len.isValid()) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  
  JSObject* darray = Uint8ClampedArray::Create(cx, context, len.value());
  if (!darray) {
    error.Throw(NS_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  nsRefPtr<mozilla::dom::ImageData> imageData =
    new mozilla::dom::ImageData(w, h, *darray);
  return imageData.forget();
}

already_AddRefed<ImageData>
CanvasRenderingContext2D::CreateImageData(JSContext* cx, double sw,
                                          double sh, ErrorResult& error)
{
  if (!sw || !sh) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  int32_t wi = JS_DoubleToInt32(sw);
  int32_t hi = JS_DoubleToInt32(sh);

  uint32_t w = Abs(wi);
  uint32_t h = Abs(hi);
  return mozilla::dom::CreateImageData(cx, this, w, h, error);
}

already_AddRefed<ImageData>
CanvasRenderingContext2D::CreateImageData(JSContext* cx,
                                          ImageData& imagedata,
                                          ErrorResult& error)
{
  return mozilla::dom::CreateImageData(cx, this, imagedata.Width(),
                                       imagedata.Height(), error);
}

static uint8_t g2DContextLayerUserData;

already_AddRefed<CanvasLayer>
CanvasRenderingContext2D::GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                         CanvasLayer *aOldLayer,
                                         LayerManager *aManager)
{
  if (mOpaque) {
    
    
    EnsureTarget();
  }

  
  
  
  
  if (!mTarget || !IsTargetValid()) {
    
    
    MarkContextClean();
    return nullptr;
  }

  mTarget->Flush();

  if (!mResetLayer && aOldLayer) {
    CanvasRenderingContext2DUserData* userData =
      static_cast<CanvasRenderingContext2DUserData*>(
        aOldLayer->GetUserData(&g2DContextLayerUserData));

    CanvasLayer::Data data;
    if (mStream) {
#ifdef USE_SKIA
      SkiaGLGlue* glue = gfxPlatform::GetPlatform()->GetSkiaGLGlue();

      if (glue) {
        data.mGLContext = glue->GetGLContext();
        data.mStream = mStream.get();
      }
#endif
    } else {
      data.mDrawTarget = mTarget;
    }

    if (userData && userData->IsForContext(this) && aOldLayer->IsDataValid(data)) {
      nsRefPtr<CanvasLayer> ret = aOldLayer;
      return ret.forget();
    }
  }

  nsRefPtr<CanvasLayer> canvasLayer = aManager->CreateCanvasLayer();
  if (!canvasLayer) {
    NS_WARNING("CreateCanvasLayer returned null!");
    
    
    MarkContextClean();
    return nullptr;
  }
  CanvasRenderingContext2DUserData *userData = nullptr;
  
  
  
  
  

  
  
  
  
  
  userData = new CanvasRenderingContext2DUserData(this);
  canvasLayer->SetDidTransactionCallback(
          CanvasRenderingContext2DUserData::DidTransactionCallback, userData);
  canvasLayer->SetUserData(&g2DContextLayerUserData, userData);

  CanvasLayer::Data data;
  if (mStream) {
    SkiaGLGlue* glue = gfxPlatform::GetPlatform()->GetSkiaGLGlue();

    if (glue) {
      canvasLayer->SetPreTransactionCallback(
              CanvasRenderingContext2DUserData::PreTransactionCallback, userData);
#if USE_SKIA
      data.mGLContext = glue->GetGLContext();
#endif
      data.mStream = mStream.get();
      data.mTexID = (uint32_t)((uintptr_t)mTarget->GetNativeSurface(NativeSurfaceType::OPENGL_TEXTURE));
    }
  } else {
    data.mDrawTarget = mTarget;
  }

  data.mSize = nsIntSize(mWidth, mHeight);
  data.mHasAlpha = !mOpaque;

  canvasLayer->Initialize(data);
  uint32_t flags = mOpaque ? Layer::CONTENT_OPAQUE : 0;
  canvasLayer->SetContentFlags(flags);
  canvasLayer->Updated();

  mResetLayer = false;

  return canvasLayer.forget();
}

void
CanvasRenderingContext2D::MarkContextClean()
{
  if (mInvalidateCount > 0) {
    mPredictManyRedrawCalls = mInvalidateCount > kCanvasMaxInvalidateCount;
  }
  mIsEntireFrameInvalid = false;
  mInvalidateCount = 0;
}


bool
CanvasRenderingContext2D::ShouldForceInactiveLayer(LayerManager *aManager)
{
  return !aManager->CanUseCanvasLayerForSize(IntSize(mWidth, mHeight));
}

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(CanvasPath, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(CanvasPath, Release)

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(CanvasPath, mParent)

CanvasPath::CanvasPath(nsISupports* aParent)
  : mParent(aParent)
{
  SetIsDOMBinding();

  mPathBuilder = gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget()->CreatePathBuilder();
}

CanvasPath::CanvasPath(nsISupports* aParent, TemporaryRef<PathBuilder> aPathBuilder)
  : mParent(aParent), mPathBuilder(aPathBuilder)
{
  SetIsDOMBinding();

  if (!mPathBuilder) {
    mPathBuilder = gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget()->CreatePathBuilder();
  }
}

JSObject*
CanvasPath::WrapObject(JSContext* aCx)
{
  return Path2DBinding::Wrap(aCx, this);
}

already_AddRefed<CanvasPath>
CanvasPath::Constructor(const GlobalObject& aGlobal, ErrorResult& aRv)
{
  nsRefPtr<CanvasPath> path = new CanvasPath(aGlobal.GetAsSupports());
  return path.forget();
}

already_AddRefed<CanvasPath>
CanvasPath::Constructor(const GlobalObject& aGlobal, CanvasPath& aCanvasPath, ErrorResult& aRv)
{
  RefPtr<gfx::Path> tempPath = aCanvasPath.GetPath(CanvasWindingRule::Nonzero,
                                                   gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget());

  nsRefPtr<CanvasPath> path = new CanvasPath(aGlobal.GetAsSupports(), tempPath->CopyToBuilder());
  return path.forget();
}

already_AddRefed<CanvasPath>
CanvasPath::Constructor(const GlobalObject& aGlobal, const nsAString& aPathString, ErrorResult& aRv)
{
  RefPtr<gfx::Path> tempPath = SVGContentUtils::GetPath(aPathString);
  if (!tempPath) {
    return Constructor(aGlobal, aRv);
  }

  nsRefPtr<CanvasPath> path = new CanvasPath(aGlobal.GetAsSupports(), tempPath->CopyToBuilder());
  return path.forget();
}

void
CanvasPath::ClosePath()
{
  EnsurePathBuilder();

  mPathBuilder->Close();
}

void
CanvasPath::MoveTo(double x, double y)
{
  EnsurePathBuilder();

  mPathBuilder->MoveTo(Point(ToFloat(x), ToFloat(y)));
}

void
CanvasPath::LineTo(double x, double y)
{
  EnsurePathBuilder();

  mPathBuilder->LineTo(Point(ToFloat(x), ToFloat(y)));
}

void
CanvasPath::QuadraticCurveTo(double cpx, double cpy, double x, double y)
{
  EnsurePathBuilder();

  mPathBuilder->QuadraticBezierTo(gfx::Point(ToFloat(cpx), ToFloat(cpy)),
                                  gfx::Point(ToFloat(x), ToFloat(y)));
}

void
CanvasPath::BezierCurveTo(double cp1x, double cp1y,
                          double cp2x, double cp2y,
                          double x, double y)
{
  BezierTo(gfx::Point(ToFloat(cp1x), ToFloat(cp1y)),
             gfx::Point(ToFloat(cp2x), ToFloat(cp2y)),
             gfx::Point(ToFloat(x), ToFloat(y)));
}

void
CanvasPath::ArcTo(double x1, double y1, double x2, double y2, double radius,
                  ErrorResult& error)
{
  if (radius < 0) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  EnsurePathBuilder();

  
  Point p0 = mPathBuilder->CurrentPoint();
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

  Arc(cx, cy, radius, angle0, angle1, anticlockwise, error);
}

void
CanvasPath::Rect(double x, double y, double w, double h)
{
  MoveTo(x, y);
  LineTo(x + w, y);
  LineTo(x + w, y + h);
  LineTo(x, y + h);
  ClosePath();
}

void
CanvasPath::Arc(double x, double y, double radius,
                double startAngle, double endAngle, bool anticlockwise,
                ErrorResult& error)
{
  if (radius < 0.0) {
    error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  EnsurePathBuilder();

  ArcToBezier(this, Point(x, y), Size(radius, radius), startAngle, endAngle, anticlockwise);
}

void
CanvasPath::LineTo(const gfx::Point& aPoint)
{
  EnsurePathBuilder();

  mPathBuilder->LineTo(aPoint);
}

void
CanvasPath::BezierTo(const gfx::Point& aCP1,
                     const gfx::Point& aCP2,
                     const gfx::Point& aCP3)
{
  EnsurePathBuilder();

  mPathBuilder->BezierTo(aCP1, aCP2, aCP3);
}

void
CanvasPath::AddPath(CanvasPath& aCanvasPath, const Optional<NonNull<SVGMatrix>>& aMatrix)
{
  RefPtr<gfx::Path> tempPath = aCanvasPath.GetPath(CanvasWindingRule::Nonzero,
                                                   gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget());

  if (aMatrix.WasPassed()) {
    const SVGMatrix& m = aMatrix.Value();
    Matrix transform(m.A(), m.B(), m.C(), m.D(), m.E(), m.F());

    if (!transform.IsIdentity()) {
      RefPtr<PathBuilder> tempBuilder = tempPath->TransformedCopyToBuilder(transform, FillRule::FILL_WINDING);
      tempPath = tempBuilder->Finish();
    }
  }

  EnsurePathBuilder(); 
  tempPath->StreamToSink(mPathBuilder);
}

TemporaryRef<gfx::Path>
CanvasPath::GetPath(const CanvasWindingRule& winding, const DrawTarget* aTarget) const
{
  FillRule fillRule = FillRule::FILL_WINDING;
  if (winding == CanvasWindingRule::Evenodd) {
    fillRule = FillRule::FILL_EVEN_ODD;
  }

  if (mPath &&
      (mPath->GetBackendType() == aTarget->GetBackendType()) &&
      (mPath->GetFillRule() == fillRule)) {
    return mPath;
  }

  if (!mPath) {
    
    MOZ_ASSERT(mPathBuilder);
    mPath = mPathBuilder->Finish();
    if (!mPath)
      return mPath;

    mPathBuilder = nullptr;
  }

  
  if (mPath->GetBackendType() != aTarget->GetBackendType()) {
    RefPtr<PathBuilder> tmpPathBuilder = aTarget->CreatePathBuilder(fillRule);
    mPath->StreamToSink(tmpPathBuilder);
    mPath = tmpPathBuilder->Finish();
  } else if (mPath->GetFillRule() != fillRule) {
    RefPtr<PathBuilder> tmpPathBuilder = mPath->CopyToBuilder(fillRule);
    mPath = tmpPathBuilder->Finish();
  }

  return mPath;
}

void
CanvasPath::EnsurePathBuilder() const
{
  if (mPathBuilder) {
    return;
  }

  
  MOZ_ASSERT(mPath);
  mPathBuilder = mPath->CopyToBuilder();
  mPath = nullptr;
}

}
}
