






#include "nsBulletFrame.h"

#include "mozilla/MathAlgorithms.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValueInlines.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsRenderingContext.h"
#include "nsDisplayList.h"
#include "nsCounterManager.h"
#include "nsBidiUtils.h"
#include "CounterStyleManager.h"

#include "imgIContainer.h"
#include "imgRequestProxy.h"
#include "nsIURI.h"

#include <algorithm>

#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

using namespace mozilla;

NS_DECLARE_FRAME_PROPERTY(FontSizeInflationProperty, nullptr)

NS_IMPL_FRAMEARENA_HELPERS(nsBulletFrame)

#ifdef DEBUG
NS_QUERYFRAME_HEAD(nsBulletFrame)
  NS_QUERYFRAME_ENTRY(nsBulletFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsFrame)
#endif

nsBulletFrame::~nsBulletFrame()
{
}

void
nsBulletFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  if (mImageRequest) {
    
    nsLayoutUtils::DeregisterImageRequest(PresContext(),
                                          mImageRequest,
                                          &mRequestRegistered);
    mImageRequest->CancelAndForgetObserver(NS_ERROR_FAILURE);
    mImageRequest = nullptr;
  }

  if (mListener) {
    mListener->SetFrame(nullptr);
  }

  
  nsFrame::DestroyFrom(aDestructRoot);
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsBulletFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Bullet"), aResult);
}
#endif

nsIAtom*
nsBulletFrame::GetType() const
{
  return nsGkAtoms::bulletFrame;
}

bool
nsBulletFrame::IsEmpty()
{
  return IsSelfEmpty();
}

bool
nsBulletFrame::IsSelfEmpty() 
{
  return StyleList()->GetCounterStyle()->IsNone();
}

 void
nsBulletFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsFrame::DidSetStyleContext(aOldStyleContext);

  imgRequestProxy *newRequest = StyleList()->GetListStyleImage();

  if (newRequest) {

    if (!mListener) {
      mListener = new nsBulletListener();
      mListener->SetFrame(this);
    }

    bool needNewRequest = true;

    if (mImageRequest) {
      
      nsCOMPtr<nsIURI> oldURI;
      mImageRequest->GetURI(getter_AddRefs(oldURI));
      nsCOMPtr<nsIURI> newURI;
      newRequest->GetURI(getter_AddRefs(newURI));
      if (oldURI && newURI) {
        bool same;
        newURI->Equals(oldURI, &same);
        if (same) {
          needNewRequest = false;
        }
      }
    }

    if (needNewRequest) {
      nsRefPtr<imgRequestProxy> oldRequest = mImageRequest;
      newRequest->Clone(mListener, getter_AddRefs(mImageRequest));

      
      
      
      if (oldRequest) {
        nsLayoutUtils::DeregisterImageRequest(PresContext(), oldRequest,
                                              &mRequestRegistered);
        oldRequest->CancelAndForgetObserver(NS_ERROR_FAILURE);
        oldRequest = nullptr;
      }

      
      if (mImageRequest) {
        nsLayoutUtils::RegisterImageRequestIfAnimated(PresContext(),
                                                      mImageRequest,
                                                      &mRequestRegistered);
      }
    }
  } else {
    
    if (mImageRequest) {
      nsLayoutUtils::DeregisterImageRequest(PresContext(), mImageRequest,
                                            &mRequestRegistered);

      mImageRequest->CancelAndForgetObserver(NS_ERROR_FAILURE);
      mImageRequest = nullptr;
    }
  }

#ifdef ACCESSIBILITY
  
  
  if (aOldStyleContext) {
    nsAccessibilityService* accService = nsIPresShell::AccService();
    if (accService) {
      const nsStyleList* oldStyleList = aOldStyleContext->PeekStyleList();
      if (oldStyleList) {
        bool hadBullet = oldStyleList->GetListStyleImage() ||
          !oldStyleList->GetCounterStyle()->IsNone();

        const nsStyleList* newStyleList = StyleList();
        bool hasBullet = newStyleList->GetListStyleImage() ||
          !newStyleList->GetCounterStyle()->IsNone();

        if (hadBullet != hasBullet) {
          accService->UpdateListBullet(PresContext()->GetPresShell(), mContent,
                                       hasBullet);
        }
      }
    }
  }
#endif
}

class nsDisplayBulletGeometry : public nsDisplayItemGenericGeometry
{
public:
  nsDisplayBulletGeometry(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder)
    : nsDisplayItemGenericGeometry(aItem, aBuilder)
  {
    nsBulletFrame* f = static_cast<nsBulletFrame*>(aItem->Frame());
    mOrdinal = f->GetOrdinal();
  }

  int32_t mOrdinal;
};

class nsDisplayBullet : public nsDisplayItem {
public:
  nsDisplayBullet(nsDisplayListBuilder* aBuilder, nsBulletFrame* aFrame) :
    nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayBullet);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayBullet() {
    MOZ_COUNT_DTOR(nsDisplayBullet);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder,
                           bool* aSnap) MOZ_OVERRIDE
  {
    *aSnap = false;
    return mFrame->GetVisualOverflowRectRelativeToSelf() + ToReferenceFrame();
  }
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState,
                       nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE {
    aOutFrames->AppendElement(mFrame);
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("Bullet", TYPE_BULLET)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  {
    bool snap;
    return GetBounds(aBuilder, &snap);
  }

  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  {
    return new nsDisplayBulletGeometry(this, aBuilder);
  }

  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion *aInvalidRegion) MOZ_OVERRIDE
  {
    const nsDisplayBulletGeometry* geometry = static_cast<const nsDisplayBulletGeometry*>(aGeometry);
    nsBulletFrame* f = static_cast<nsBulletFrame*>(mFrame);

    if (f->GetOrdinal() != geometry->mOrdinal) {
      bool snap;
      aInvalidRegion->Or(geometry->mBounds, GetBounds(aBuilder, &snap));
      return;
    }

    nsCOMPtr<imgIContainer> image = f->GetImage();
    if (aBuilder->ShouldSyncDecodeImages() && image && !image->IsDecoded()) {
      
      
      
      bool snap;
      aInvalidRegion->Or(*aInvalidRegion, GetBounds(aBuilder, &snap));
    }

    return nsDisplayItem::ComputeInvalidationRegion(aBuilder, aGeometry, aInvalidRegion);
  }
};

void nsDisplayBullet::Paint(nsDisplayListBuilder* aBuilder,
                            nsRenderingContext* aCtx)
{
  uint32_t flags = imgIContainer::FLAG_NONE;
  if (aBuilder->ShouldSyncDecodeImages()) {
    flags |= imgIContainer::FLAG_SYNC_DECODE;
  }
  static_cast<nsBulletFrame*>(mFrame)->
    PaintBullet(*aCtx, ToReferenceFrame(), mVisibleRect, flags);
}

void
nsBulletFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  if (!IsVisibleForPainting(aBuilder))
    return;

  DO_GLOBAL_REFLOW_COUNT_DSP("nsBulletFrame");
  
  aLists.Content()->AppendNewToTop(
    new (aBuilder) nsDisplayBullet(aBuilder, this));
}

void
nsBulletFrame::PaintBullet(nsRenderingContext& aRenderingContext, nsPoint aPt,
                           const nsRect& aDirtyRect, uint32_t aFlags)
{
  const nsStyleList* myList = StyleList();
  CounterStyle* listStyleType = myList->GetCounterStyle();

  if (myList->GetListStyleImage() && mImageRequest) {
    uint32_t status;
    mImageRequest->GetImageStatus(&status);
    if (status & imgIRequest::STATUS_LOAD_COMPLETE &&
        !(status & imgIRequest::STATUS_ERROR)) {
      nsCOMPtr<imgIContainer> imageCon;
      mImageRequest->GetImage(getter_AddRefs(imageCon));
      if (imageCon) {
        nsRect dest(mPadding.left, mPadding.top,
                    mRect.width - (mPadding.left + mPadding.right),
                    mRect.height - (mPadding.top + mPadding.bottom));
        nsLayoutUtils::DrawSingleImage(&aRenderingContext,
             imageCon, nsLayoutUtils::GetGraphicsFilterForFrame(this),
             dest + aPt, aDirtyRect, nullptr, aFlags);
        return;
      }
    }
  }

  nsRefPtr<nsFontMetrics> fm;
  aRenderingContext.SetColor(nsLayoutUtils::GetColor(this, eCSSProperty_color));

  nsAutoString text;
  switch (listStyleType->GetStyle()) {
  case NS_STYLE_LIST_STYLE_NONE:
    break;

  case NS_STYLE_LIST_STYLE_DISC:
    aRenderingContext.FillEllipse(mPadding.left + aPt.x, mPadding.top + aPt.y,
                                  mRect.width - (mPadding.left + mPadding.right),
                                  mRect.height - (mPadding.top + mPadding.bottom));
    break;

  case NS_STYLE_LIST_STYLE_CIRCLE:
    aRenderingContext.DrawEllipse(mPadding.left + aPt.x, mPadding.top + aPt.y,
                                  mRect.width - (mPadding.left + mPadding.right),
                                  mRect.height - (mPadding.top + mPadding.bottom));
    break;

  case NS_STYLE_LIST_STYLE_SQUARE:
    {
      nsRect rect(aPt, mRect.Size());
      rect.Deflate(mPadding);

      
      
      
      
      
      
      nsPresContext *pc = PresContext();
      nsRect snapRect(rect.x, rect.y, 
                      pc->RoundAppUnitsToNearestDevPixels(rect.width),
                      pc->RoundAppUnitsToNearestDevPixels(rect.height));
      snapRect.MoveBy((rect.width - snapRect.width) / 2,
                      (rect.height - snapRect.height) / 2);
      aRenderingContext.FillRect(snapRect.x, snapRect.y,
                                 snapRect.width, snapRect.height);
    }
    break;

  default:
    nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
                                          GetFontSizeInflation());
    GetListItemText(text);
    aRenderingContext.SetFont(fm);
    nscoord ascent = fm->MaxAscent();
    aPt.MoveBy(mPadding.left, mPadding.top);
    aPt.y = NSToCoordRound(nsLayoutUtils::GetSnappedBaselineY(
            this, aRenderingContext.ThebesContext(), aPt.y, ascent));
    nsPresContext* presContext = PresContext();
    if (!presContext->BidiEnabled() && HasRTLChars(text)) {
      presContext->SetBidiEnabled();
    }
    nsLayoutUtils::DrawString(this, &aRenderingContext,
                              text.get(), text.Length(), aPt);
    break;
  }
}

int32_t
nsBulletFrame::SetListItemOrdinal(int32_t aNextOrdinal,
                                  bool* aChanged,
                                  int32_t aIncrement)
{
  MOZ_ASSERT(aIncrement == 1 || aIncrement == -1,
             "We shouldn't have weird increments here");

  
  int32_t oldOrdinal = mOrdinal;
  mOrdinal = aNextOrdinal;

  
  
  
  nsIContent* parentContent = GetParent()->GetContent();
  if (parentContent) {
    nsGenericHTMLElement *hc =
      nsGenericHTMLElement::FromContent(parentContent);
    if (hc) {
      const nsAttrValue* attr = hc->GetParsedAttr(nsGkAtoms::value);
      if (attr && attr->Type() == nsAttrValue::eInteger) {
        
        mOrdinal = attr->GetIntegerValue();
      }
    }
  }

  *aChanged = oldOrdinal != mOrdinal;

  return nsCounterManager::IncrementCounter(mOrdinal, aIncrement);
}

void
nsBulletFrame::GetListItemText(nsAString& aResult)
{
  CounterStyle* style = StyleList()->GetCounterStyle();
  NS_ASSERTION(style->GetStyle() != NS_STYLE_LIST_STYLE_NONE &&
               style->GetStyle() != NS_STYLE_LIST_STYLE_DISC &&
               style->GetStyle() != NS_STYLE_LIST_STYLE_CIRCLE &&
               style->GetStyle() != NS_STYLE_LIST_STYLE_SQUARE,
               "we should be using specialized code for these types");

  bool isRTL;
  nsAutoString counter, prefix, suffix;
  style->GetPrefix(prefix);
  style->GetSuffix(suffix);
  style->GetCounterText(mOrdinal, counter, isRTL);

  aResult.Truncate();
  aResult.Append(prefix);
  if (GetWritingMode().IsBidiLTR() != isRTL) {
    aResult.Append(counter);
  } else {
    
    char16_t mark = isRTL ? 0x200f : 0x200e;
    aResult.Append(mark);
    aResult.Append(counter);
    aResult.Append(mark);
  }
  aResult.Append(suffix);
}

#define MIN_BULLET_SIZE 1


void
nsBulletFrame::GetDesiredSize(nsPresContext*  aCX,
                              nsRenderingContext *aRenderingContext,
                              nsHTMLReflowMetrics& aMetrics,
                              float aFontSizeInflation)
{
  
  mPadding.SizeTo(0, 0, 0, 0);
  
  const nsStyleList* myList = StyleList();
  nscoord ascent;
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
                                        aFontSizeInflation);

  RemoveStateBits(BULLET_FRAME_IMAGE_LOADING);

  if (myList->GetListStyleImage() && mImageRequest) {
    uint32_t status;
    mImageRequest->GetImageStatus(&status);
    if (status & imgIRequest::STATUS_SIZE_AVAILABLE &&
        !(status & imgIRequest::STATUS_ERROR)) {
      
      aMetrics.Width() = mIntrinsicSize.width;
      aMetrics.SetBlockStartAscent(aMetrics.Height() = mIntrinsicSize.height);

      
      nscoord halfEm = fm->EmHeight() / 2;
      WritingMode wm = GetWritingMode();
      if (wm.IsVertical()) {
        mPadding.bottom += halfEm;
      } else if (wm.IsBidiLTR()) {
        mPadding.right += halfEm;
      } else {
        mPadding.left += halfEm;
      }

      AddStateBits(BULLET_FRAME_IMAGE_LOADING);

      return;
    }
  }

  
  
  
  
  
  
  mIntrinsicSize.SizeTo(0, 0);

  nscoord bulletSize;

  nsAutoString text;
  switch (myList->GetCounterStyle()->GetStyle()) {
    case NS_STYLE_LIST_STYLE_NONE:
      aMetrics.Width() = aMetrics.Height() = 0;
      aMetrics.SetBlockStartAscent(0);
      break;

    case NS_STYLE_LIST_STYLE_DISC:
    case NS_STYLE_LIST_STYLE_CIRCLE:
    case NS_STYLE_LIST_STYLE_SQUARE: {
      ascent = fm->MaxAscent();
      bulletSize = std::max(nsPresContext::CSSPixelsToAppUnits(MIN_BULLET_SIZE),
                          NSToCoordRound(0.8f * (float(ascent) / 2.0f)));
      mPadding.bottom = NSToCoordRound(float(ascent) / 8.0f);
      aMetrics.Width() = aMetrics.Height() = bulletSize;
      aMetrics.SetBlockStartAscent(bulletSize + mPadding.bottom);

      
      nscoord halfEm = fm->EmHeight() / 2;
      WritingMode wm = GetWritingMode();
      if (wm.IsVertical()) {
        mPadding.bottom += halfEm;
      } else if (wm.IsBidiLTR()) {
        mPadding.right += halfEm;
      } else {
        mPadding.left += halfEm;
      }
      break;
    }

    default:
      GetListItemText(text);
      aMetrics.Height() = fm->MaxHeight();
      aRenderingContext->SetFont(fm);
      aMetrics.Width() =
        nsLayoutUtils::GetStringWidth(this, aRenderingContext,
                                      text.get(), text.Length());
      aMetrics.SetBlockStartAscent(fm->MaxAscent());
      break;
  }
}

void
nsBulletFrame::Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aMetrics,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsBulletFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);

  float inflation = nsLayoutUtils::FontSizeInflationFor(this);
  SetFontSizeInflation(inflation);

  
  GetDesiredSize(aPresContext, aReflowState.rendContext, aMetrics, inflation);

  
  
  const nsMargin& borderPadding = aReflowState.ComputedPhysicalBorderPadding();
  mPadding.top += NSToCoordRound(borderPadding.top * inflation);
  mPadding.right += NSToCoordRound(borderPadding.right * inflation);
  mPadding.bottom += NSToCoordRound(borderPadding.bottom * inflation);
  mPadding.left += NSToCoordRound(borderPadding.left * inflation);
  aMetrics.Width() += mPadding.left + mPadding.right;
  aMetrics.Height() += mPadding.top + mPadding.bottom;
  aMetrics.SetBlockStartAscent(aMetrics.BlockStartAscent() + mPadding.top);

  
  
  
  
  aMetrics.SetOverflowAreasToDesiredBounds();

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
}

 nscoord
nsBulletFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nsHTMLReflowMetrics metrics(GetWritingMode());
  DISPLAY_MIN_WIDTH(this, metrics.Width());
  GetDesiredSize(PresContext(), aRenderingContext, metrics, 1.0f);
  return metrics.Width();
}

 nscoord
nsBulletFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  nsHTMLReflowMetrics metrics(GetWritingMode());
  DISPLAY_PREF_WIDTH(this, metrics.Width());
  GetDesiredSize(PresContext(), aRenderingContext, metrics, 1.0f);
  return metrics.Width();
}

NS_IMETHODIMP
nsBulletFrame::Notify(imgIRequest *aRequest, int32_t aType, const nsIntRect* aData)
{
  if (aType == imgINotificationObserver::SIZE_AVAILABLE) {
    nsCOMPtr<imgIContainer> image;
    aRequest->GetImage(getter_AddRefs(image));
    return OnStartContainer(aRequest, image);
  }

  if (aType == imgINotificationObserver::FRAME_UPDATE) {
    
    
    
    InvalidateFrame();
  }

  if (aType == imgINotificationObserver::IS_ANIMATED) {
    
    
    if (aRequest == mImageRequest) {
      nsLayoutUtils::RegisterImageRequest(PresContext(), mImageRequest,
                                          &mRequestRegistered);
    }
  }

  return NS_OK;
}

nsresult nsBulletFrame::OnStartContainer(imgIRequest *aRequest,
                                         imgIContainer *aImage)
{
  if (!aImage) return NS_ERROR_INVALID_ARG;
  if (!aRequest) return NS_ERROR_INVALID_ARG;

  uint32_t status;
  aRequest->GetImageStatus(&status);
  if (status & imgIRequest::STATUS_ERROR) {
    return NS_OK;
  }
  
  nscoord w, h;
  aImage->GetWidth(&w);
  aImage->GetHeight(&h);

  nsPresContext* presContext = PresContext();

  nsSize newsize(nsPresContext::CSSPixelsToAppUnits(w),
                 nsPresContext::CSSPixelsToAppUnits(h));

  if (mIntrinsicSize != newsize) {
    mIntrinsicSize = newsize;

    
    
    nsIPresShell *shell = presContext->GetPresShell();
    if (shell) {
      shell->FrameNeedsReflow(this, nsIPresShell::eStyleChange,
                              NS_FRAME_IS_DIRTY);
    }
  }

  
  aImage->SetAnimationMode(presContext->ImageAnimationMode());
  
  
  
  aRequest->IncrementAnimationConsumers();
  
  return NS_OK;
}

void
nsBulletFrame::GetLoadGroup(nsPresContext *aPresContext, nsILoadGroup **aLoadGroup)
{
  if (!aPresContext)
    return;

  NS_PRECONDITION(nullptr != aLoadGroup, "null OUT parameter pointer");

  nsIPresShell *shell = aPresContext->GetPresShell();

  if (!shell)
    return;

  nsIDocument *doc = shell->GetDocument();
  if (!doc)
    return;

  *aLoadGroup = doc->GetDocumentLoadGroup().take();
}

union VoidPtrOrFloat {
  VoidPtrOrFloat() : p(nullptr) {}

  void *p;
  float f;
};

float
nsBulletFrame::GetFontSizeInflation() const
{
  if (!HasFontSizeInflation()) {
    return 1.0f;
  }
  VoidPtrOrFloat u;
  u.p = Properties().Get(FontSizeInflationProperty());
  return u.f;
}

void
nsBulletFrame::SetFontSizeInflation(float aInflation)
{
  if (aInflation == 1.0f) {
    if (HasFontSizeInflation()) {
      RemoveStateBits(BULLET_FRAME_HAS_FONT_INFLATION);
      Properties().Delete(FontSizeInflationProperty());
    }
    return;
  }

  AddStateBits(BULLET_FRAME_HAS_FONT_INFLATION);
  VoidPtrOrFloat u;
  u.f = aInflation;
  Properties().Set(FontSizeInflationProperty(), u.p);
}

already_AddRefed<imgIContainer>
nsBulletFrame::GetImage() const
{
  if (mImageRequest && StyleList()->GetListStyleImage()) {
    nsCOMPtr<imgIContainer> imageCon;
    mImageRequest->GetImage(getter_AddRefs(imageCon));
    return imageCon.forget();
  }

  return nullptr;
}

nscoord
nsBulletFrame::GetBaseline() const
{
  nscoord ascent = 0, bottomPadding;
  if (GetStateBits() & BULLET_FRAME_IMAGE_LOADING) {
    ascent = GetRect().height;
  } else {
    nsRefPtr<nsFontMetrics> fm;
    nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
                                          GetFontSizeInflation());
    CounterStyle* listStyleType = StyleList()->GetCounterStyle();
    switch (listStyleType->GetStyle()) {
      case NS_STYLE_LIST_STYLE_NONE:
        break;

      case NS_STYLE_LIST_STYLE_DISC:
      case NS_STYLE_LIST_STYLE_CIRCLE:
      case NS_STYLE_LIST_STYLE_SQUARE:
        ascent = fm->MaxAscent();
        bottomPadding = NSToCoordRound(float(ascent) / 8.0f);
        ascent = std::max(nsPresContext::CSSPixelsToAppUnits(MIN_BULLET_SIZE),
                        NSToCoordRound(0.8f * (float(ascent) / 2.0f)));
        ascent += bottomPadding;
        break;

      default:
        ascent = fm->MaxAscent();
        break;
    }
  }
  return ascent + GetUsedBorderAndPadding().top;
}

void
nsBulletFrame::GetSpokenText(nsAString& aText)
{
  CounterStyle* style = StyleList()->GetCounterStyle();
  bool isBullet;
  style->GetSpokenCounterText(mOrdinal, aText, isBullet);
  if (isBullet) {
    aText.Append(' ');
  } else {
    nsAutoString prefix, suffix;
    style->GetPrefix(prefix);
    style->GetSuffix(suffix);
    aText = prefix + aText + suffix;
  }
}








NS_IMPL_ISUPPORTS(nsBulletListener, imgINotificationObserver)

nsBulletListener::nsBulletListener() :
  mFrame(nullptr)
{
}

nsBulletListener::~nsBulletListener()
{
}

NS_IMETHODIMP
nsBulletListener::Notify(imgIRequest *aRequest, int32_t aType, const nsIntRect* aData)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;
  return mFrame->Notify(aRequest, aType, aData);
}
