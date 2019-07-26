






#include "mozilla/DebugOnly.h"

#include "nsHTMLParts.h"
#include "nsCOMPtr.h"
#include "nsImageFrame.h"
#include "nsIImageLoadingContent.h"
#include "nsString.h"
#include "nsPrintfCString.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsIPresShell.h"
#include "nsGkAtoms.h"
#include "nsIDocument.h"
#include "nsINodeInfo.h"
#include "nsContentUtils.h"
#include "nsCSSAnonBoxes.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsStyleCoord.h"
#include "nsTransform2D.h"
#include "nsImageMap.h"
#include "nsILinkHandler.h"
#include "nsIURL.h"
#include "nsIIOService.h"
#include "nsILoadGroup.h"
#include "nsISupportsPriority.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"
#include "nsContainerFrame.h"
#include "prprf.h"
#include "nsCSSRendering.h"
#include "nsILink.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsINameSpaceManager.h"
#include "nsTextFragment.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIScriptSecurityManager.h"
#include <algorithm>
#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif
#include "nsIDOMNode.h"
#include "nsGUIEvent.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"

#include "imgIContainer.h"
#include "imgLoader.h"
#include "imgRequestProxy.h"

#include "nsCSSFrameConstructor.h"
#include "nsIDOMRange.h"

#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsEventStates.h"
#include "nsError.h"
#include "nsBidiUtils.h"
#include "nsBidiPresUtils.h"

#include "gfxRect.h"
#include "ImageLayers.h"
#include "ImageContainer.h"

#include "mozilla/Preferences.h"

using namespace mozilla;


#define ICON_SIZE        (16)
#define ICON_PADDING     (3)
#define ALT_BORDER_WIDTH (1)



#define IMAGE_EDITOR_CHECK 1


#define ALIGN_UNSET uint8_t(-1)

using namespace mozilla::layers;
using namespace mozilla::dom;


nsImageFrame::IconLoad* nsImageFrame::gIconLoad = nullptr;


nsIIOService* nsImageFrame::sIOService;


static bool HaveFixedSize(const nsStylePosition* aStylePosition)
{
  
  
  
  return aStylePosition->mWidth.IsCoordPercentCalcUnit() &&
         aStylePosition->mHeight.IsCoordPercentCalcUnit();
}



inline bool HaveFixedSize(const nsHTMLReflowState& aReflowState)
{ 
  NS_ASSERTION(aReflowState.mStylePosition, "crappy reflowState - null stylePosition");
  
  
  
  
  
  
  
  const nsStyleCoord &height = aReflowState.mStylePosition->mHeight;
  const nsStyleCoord &width = aReflowState.mStylePosition->mWidth;
  return ((height.HasPercent() &&
           NS_UNCONSTRAINEDSIZE == aReflowState.ComputedHeight()) ||
          (width.HasPercent() &&
           (NS_UNCONSTRAINEDSIZE == aReflowState.ComputedWidth() ||
            0 == aReflowState.ComputedWidth())))
          ? false
          : HaveFixedSize(aReflowState.mStylePosition); 
}

nsIFrame*
NS_NewImageFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsImageFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsImageFrame)


nsImageFrame::nsImageFrame(nsStyleContext* aContext) :
  ImageFrameSuper(aContext),
  mComputedSize(0, 0),
  mIntrinsicRatio(0, 0),
  mDisplayingIcon(false),
  mFirstFrameComplete(false)
{
  
  
  mIntrinsicSize.width.SetCoordValue(0);
  mIntrinsicSize.height.SetCoordValue(0);
}

nsImageFrame::~nsImageFrame()
{
}

NS_QUERYFRAME_HEAD(nsImageFrame)
  NS_QUERYFRAME_ENTRY(nsImageFrame)
NS_QUERYFRAME_TAIL_INHERITING(ImageFrameSuper)

#ifdef ACCESSIBILITY
a11y::AccType
nsImageFrame::AccessibleType()
{
  
  if (HasImageMap()) {
    return a11y::eHTMLImageMapType;
  }

  return a11y::eImageType;
}
#endif

void
nsImageFrame::DisconnectMap()
{
  if (mImageMap) {
    mImageMap->Destroy();
    NS_RELEASE(mImageMap);

#ifdef ACCESSIBILITY
  nsAccessibilityService* accService = GetAccService();
  if (accService) {
    accService->RecreateAccessible(PresContext()->PresShell(), mContent);
  }
#endif
  }
}

void
nsImageFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  
  
  DisconnectMap();

  
  if (mListener) {
    nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mContent);
    if (imageLoader) {
      
      
      
      nsCxPusher pusher;
      pusher.PushNull();

      
      
      imageLoader->FrameDestroyed(this);

      imageLoader->RemoveObserver(mListener);
    }
    
    reinterpret_cast<nsImageListener*>(mListener.get())->SetFrame(nullptr);
  }
  
  mListener = nullptr;

  
  if (mDisplayingIcon)
    gIconLoad->RemoveIconObserver(this);

  nsSplittableFrame::DestroyFrom(aDestructRoot);
}



NS_IMETHODIMP
nsImageFrame::Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsSplittableFrame::Init(aContent, aParent, aPrevInFlow);
  NS_ENSURE_SUCCESS(rv, rv);

  mListener = new nsImageListener(this);

  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(aContent);
  NS_ENSURE_TRUE(imageLoader, NS_ERROR_UNEXPECTED);

  {
    
    
    
    nsCxPusher pusher;
    pusher.PushNull();

    imageLoader->AddObserver(mListener);
  }

  nsPresContext *aPresContext = PresContext();
  
  if (!gIconLoad)
    LoadIcons(aPresContext);

  
  
  imageLoader->FrameCreated(this);

  
  nsCOMPtr<imgIRequest> currentRequest;
  imageLoader->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                          getter_AddRefs(currentRequest));
  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(currentRequest);
  if (p)
    p->AdjustPriority(-1);

  
  
  if (currentRequest) {
    nsCOMPtr<imgIContainer> image;
    currentRequest->GetImage(getter_AddRefs(image));
    if (image) {
      image->SetAnimationMode(aPresContext->ImageAnimationMode());
    }
  }

  return rv;
}

bool
nsImageFrame::UpdateIntrinsicSize(imgIContainer* aImage)
{
  NS_PRECONDITION(aImage, "null image");
  if (!aImage)
    return false;

  nsIFrame::IntrinsicSize oldIntrinsicSize = mIntrinsicSize;

  nsIFrame* rootFrame = aImage->GetRootLayoutFrame();
  if (rootFrame) {
    
    mIntrinsicSize = rootFrame->GetIntrinsicSize();
  } else {
    
    nsIntSize imageSizeInPx;
    if (NS_FAILED(aImage->GetWidth(&imageSizeInPx.width)) ||
        NS_FAILED(aImage->GetHeight(&imageSizeInPx.height))) {
      imageSizeInPx.SizeTo(0, 0);
    }
    mIntrinsicSize.width.SetCoordValue(
      nsPresContext::CSSPixelsToAppUnits(imageSizeInPx.width));
    mIntrinsicSize.height.SetCoordValue(
      nsPresContext::CSSPixelsToAppUnits(imageSizeInPx.height));
  }

  return mIntrinsicSize != oldIntrinsicSize;
}

bool
nsImageFrame::UpdateIntrinsicRatio(imgIContainer* aImage)
{
  NS_PRECONDITION(aImage, "null image");

  if (!aImage)
    return false;

  nsSize oldIntrinsicRatio = mIntrinsicRatio;

  nsIFrame* rootFrame = aImage->GetRootLayoutFrame();
  if (rootFrame) {
    
    mIntrinsicRatio = rootFrame->GetIntrinsicRatio();
  } else {
    NS_ABORT_IF_FALSE(mIntrinsicSize.width.GetUnit() == eStyleUnit_Coord &&
                      mIntrinsicSize.height.GetUnit() == eStyleUnit_Coord,
                      "since aImage doesn't have a rootFrame, our intrinsic "
                      "dimensions must have coord units (not percent units)");
    mIntrinsicRatio.width = mIntrinsicSize.width.GetCoordValue();
    mIntrinsicRatio.height = mIntrinsicSize.height.GetCoordValue();
  }

  return mIntrinsicRatio != oldIntrinsicRatio;
}

bool
nsImageFrame::GetSourceToDestTransform(nsTransform2D& aTransform)
{
  
  
  
  
  nsRect innerArea = GetInnerArea();
  aTransform.SetToTranslate(float(innerArea.x),
                            float(innerArea.y - GetContinuationOffset()));

  
  if (mIntrinsicSize.width.GetUnit() == eStyleUnit_Coord &&
      mIntrinsicSize.width.GetCoordValue() != 0 &&
      mIntrinsicSize.height.GetUnit() == eStyleUnit_Coord &&
      mIntrinsicSize.height.GetCoordValue() != 0 &&
      mIntrinsicSize.width.GetCoordValue() != mComputedSize.width &&
      mIntrinsicSize.height.GetCoordValue() != mComputedSize.height) {

    aTransform.SetScale(float(mComputedSize.width)  /
                        float(mIntrinsicSize.width.GetCoordValue()),
                        float(mComputedSize.height) /
                        float(mIntrinsicSize.height.GetCoordValue()));
    return true;
  }

  return false;
}








bool
nsImageFrame::IsPendingLoad(imgIRequest* aRequest) const
{
  
  nsCOMPtr<nsIImageLoadingContent> imageLoader(do_QueryInterface(mContent));
  NS_ASSERTION(imageLoader, "No image loading content?");

  int32_t requestType = nsIImageLoadingContent::UNKNOWN_REQUEST;
  imageLoader->GetRequestType(aRequest, &requestType);

  return requestType != nsIImageLoadingContent::CURRENT_REQUEST;
}

bool
nsImageFrame::IsPendingLoad(imgIContainer* aContainer) const
{
  
  if (!aContainer) {
    NS_ERROR("No image container!");
    return true;
  }

  nsCOMPtr<nsIImageLoadingContent> imageLoader(do_QueryInterface(mContent));
  NS_ASSERTION(imageLoader, "No image loading content?");
  
  nsCOMPtr<imgIRequest> currentRequest;
  imageLoader->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                          getter_AddRefs(currentRequest));
  if (!currentRequest) {
    NS_ERROR("No current request");
    return true;
  }

  nsCOMPtr<imgIContainer> currentContainer;
  currentRequest->GetImage(getter_AddRefs(currentContainer));

  return currentContainer != aContainer;
  
}

nsRect
nsImageFrame::SourceRectToDest(const nsIntRect& aRect)
{
  
  
  
  
  

  nsRect r(nsPresContext::CSSPixelsToAppUnits(aRect.x - 1),
           nsPresContext::CSSPixelsToAppUnits(aRect.y - 1),
           nsPresContext::CSSPixelsToAppUnits(aRect.width + 2),
           nsPresContext::CSSPixelsToAppUnits(aRect.height + 2));

  nsTransform2D sourceToDest;
  if (!GetSourceToDestTransform(sourceToDest)) {
    
    
    
    return GetInnerArea();
  }

  sourceToDest.TransformCoord(&r.x, &r.y, &r.width, &r.height);

  
  nscoord scale = nsPresContext::CSSPixelsToAppUnits(1);
  nscoord right = r.x + r.width;
  nscoord bottom = r.y + r.height;

  r.x -= (scale + (r.x % scale)) % scale;
  r.y -= (scale + (r.y % scale)) % scale;
  r.width = right + ((scale - (right % scale)) % scale) - r.x;
  r.height = bottom + ((scale - (bottom % scale)) % scale) - r.y;

  return r;
}





#define BAD_STATES (NS_EVENT_STATE_BROKEN | NS_EVENT_STATE_USERDISABLED | \
                    NS_EVENT_STATE_LOADING)



#define IMAGE_OK(_state, _loadingOK)                                           \
   (!(_state).HasAtLeastOneOfStates(BAD_STATES) ||                                    \
    (!(_state).HasAtLeastOneOfStates(NS_EVENT_STATE_BROKEN | NS_EVENT_STATE_USERDISABLED) && \
     (_state).HasState(NS_EVENT_STATE_LOADING) && (_loadingOK)))


bool
nsImageFrame::ShouldCreateImageFrameFor(Element* aElement,
                                        nsStyleContext* aStyleContext)
{
  nsEventStates state = aElement->State();
  if (IMAGE_OK(state,
               HaveFixedSize(aStyleContext->GetStylePosition()))) {
    
    return true;
  }

  
  
  
  
  
  
  
  
  
  
  
  bool useSizedBox;
  
  if (aStyleContext->GetStyleUIReset()->mForceBrokenImageIcon) {
    useSizedBox = true;
  }
  else if (gIconLoad && gIconLoad->mPrefForceInlineAltText) {
    useSizedBox = false;
  }
  else {
    if (aStyleContext->PresContext()->CompatibilityMode() !=
        eCompatibility_NavQuirks) {
      useSizedBox = false;
    }
    else {
      
      
      nsIAtom *localName = aElement->Tag();

      
      
      
      if (!aElement->HasAttr(kNameSpaceID_None, nsGkAtoms::alt) &&
          localName != nsGkAtoms::object &&
          localName != nsGkAtoms::input) {
        useSizedBox = true;
      }
      else {
        
        useSizedBox = HaveFixedSize(aStyleContext->GetStylePosition());
      }
    }
  }
  
  return useSizedBox;
}

nsresult
nsImageFrame::Notify(imgIRequest* aRequest, int32_t aType, const nsIntRect* aData)
{
  if (aType == imgINotificationObserver::SIZE_AVAILABLE) {
    nsCOMPtr<imgIContainer> image;
    aRequest->GetImage(getter_AddRefs(image));
    return OnStartContainer(aRequest, image);
  }

  if (aType == imgINotificationObserver::FRAME_UPDATE) {
    return OnDataAvailable(aRequest, aData);
  }

  if (aType == imgINotificationObserver::FRAME_COMPLETE) {
    mFirstFrameComplete = true;
  }

  if (aType == imgINotificationObserver::LOAD_COMPLETE) {
    uint32_t imgStatus;
    aRequest->GetImageStatus(&imgStatus);
    nsresult status =
        imgStatus & imgIRequest::STATUS_ERROR ? NS_ERROR_FAILURE : NS_OK;
    return OnStopRequest(aRequest, status);
  }

  return NS_OK;
}

nsresult
nsImageFrame::OnStartContainer(imgIRequest *aRequest, imgIContainer *aImage)
{
  if (!aImage) return NS_ERROR_INVALID_ARG;

  




  nsPresContext *presContext = PresContext();
  aImage->SetAnimationMode(presContext->ImageAnimationMode());

  if (IsPendingLoad(aRequest)) {
    
    return NS_OK;
  }
  
  bool intrinsicSizeChanged = UpdateIntrinsicSize(aImage);
  intrinsicSizeChanged = UpdateIntrinsicRatio(aImage) || intrinsicSizeChanged;

  if (intrinsicSizeChanged && (mState & IMAGE_GOTINITIALREFLOW)) {
    
    
    if (!(mState & IMAGE_SIZECONSTRAINED)) { 
      nsIPresShell *presShell = presContext->GetPresShell();
      NS_ASSERTION(presShell, "No PresShell.");
      if (presShell) { 
        presShell->FrameNeedsReflow(this, nsIPresShell::eStyleChange,
                                    NS_FRAME_IS_DIRTY);
      }
    }
  }

  return NS_OK;
}

nsresult
nsImageFrame::OnDataAvailable(imgIRequest *aRequest,
                              const nsIntRect *aRect)
{
  if (mFirstFrameComplete) {
    nsCOMPtr<imgIContainer> container;
    aRequest->GetImage(getter_AddRefs(container));
    return FrameChanged(aRequest, container);
  }

  
  
  

  NS_ENSURE_ARG_POINTER(aRect);

  if (!(mState & IMAGE_GOTINITIALREFLOW)) {
    
    return NS_OK;
  }
  
  if (IsPendingLoad(aRequest)) {
    
    return NS_OK;
  }

#ifdef DEBUG_decode
  printf("Source rect (%d,%d,%d,%d)\n",
         aRect->x, aRect->y, aRect->width, aRect->height);
#endif

  if (aRect->IsEqualInterior(nsIntRect::GetMaxSizedIntRect())) {
    InvalidateFrame(nsDisplayItem::TYPE_IMAGE);
    InvalidateFrame(nsDisplayItem::TYPE_ALT_FEEDBACK);
  } else {
    nsRect invalid = SourceRectToDest(*aRect);
    InvalidateFrameWithRect(invalid, nsDisplayItem::TYPE_IMAGE);
    InvalidateFrameWithRect(invalid, nsDisplayItem::TYPE_ALT_FEEDBACK);
  }

  return NS_OK;
}

nsresult
nsImageFrame::OnStopRequest(imgIRequest *aRequest,
                            nsresult aStatus)
{
  
  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mContent);
  NS_ASSERTION(imageLoader, "Who's notifying us??");
  int32_t loadType = nsIImageLoadingContent::UNKNOWN_REQUEST;
  imageLoader->GetRequestType(aRequest, &loadType);
  if (loadType != nsIImageLoadingContent::CURRENT_REQUEST &&
      loadType != nsIImageLoadingContent::PENDING_REQUEST) {
    return NS_ERROR_FAILURE;
  }

  bool multipart = false;
  aRequest->GetMultipart(&multipart);

  if (loadType == nsIImageLoadingContent::PENDING_REQUEST || multipart) {
    NotifyNewCurrentRequest(aRequest, aStatus);
  }

  return NS_OK;
}

void
nsImageFrame::NotifyNewCurrentRequest(imgIRequest *aRequest,
                                      nsresult aStatus)
{
  
  bool intrinsicSizeChanged = true;
  if (NS_SUCCEEDED(aStatus)) {
    nsCOMPtr<imgIContainer> imageContainer;
    aRequest->GetImage(getter_AddRefs(imageContainer));
    NS_ASSERTION(imageContainer, "Successful load with no container?");
    intrinsicSizeChanged = UpdateIntrinsicSize(imageContainer);
    intrinsicSizeChanged = UpdateIntrinsicRatio(imageContainer) ||
      intrinsicSizeChanged;
  }
  else {
    
    mIntrinsicSize.width.SetCoordValue(0);
    mIntrinsicSize.height.SetCoordValue(0);
    mIntrinsicRatio.SizeTo(0, 0);
  }

  if (mState & IMAGE_GOTINITIALREFLOW) { 
    if (!(mState & IMAGE_SIZECONSTRAINED) && intrinsicSizeChanged) {
      nsIPresShell *presShell = PresContext()->GetPresShell();
      if (presShell) { 
        presShell->FrameNeedsReflow(this, nsIPresShell::eStyleChange,
                                    NS_FRAME_IS_DIRTY);
      }
    }
    
    InvalidateFrame();
  }
}

nsresult
nsImageFrame::FrameChanged(imgIRequest *aRequest,
                           imgIContainer *aContainer)
{
  if (!GetStyleVisibility()->IsVisible()) {
    return NS_OK;
  }

  if (IsPendingLoad(aContainer)) {
    
    return NS_OK;
  }

  InvalidateLayer(nsDisplayItem::TYPE_IMAGE);
  return NS_OK;
}

void
nsImageFrame::EnsureIntrinsicSizeAndRatio(nsPresContext* aPresContext)
{
  
  
  if (mIntrinsicSize.width.GetUnit() == eStyleUnit_Coord &&
      mIntrinsicSize.width.GetCoordValue() == 0 &&
      mIntrinsicSize.height.GetUnit() == eStyleUnit_Coord &&
      mIntrinsicSize.height.GetCoordValue() == 0) {

    
    nsCOMPtr<imgIRequest> currentRequest;
    nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mContent);
    if (imageLoader)
      imageLoader->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                              getter_AddRefs(currentRequest));
    uint32_t status = 0;
    if (currentRequest)
      currentRequest->GetImageStatus(&status);

    
    if (status & imgIRequest::STATUS_SIZE_AVAILABLE) {
      nsCOMPtr<imgIContainer> imgCon;
      currentRequest->GetImage(getter_AddRefs(imgCon));
      NS_ABORT_IF_FALSE(imgCon, "SIZE_AVAILABLE, but no imgContainer?");
      UpdateIntrinsicSize(imgCon);
      UpdateIntrinsicRatio(imgCon);
    } else {
      
      
      
      
      
      
      if (aPresContext->CompatibilityMode() == eCompatibility_NavQuirks) {
        nscoord edgeLengthToUse =
          nsPresContext::CSSPixelsToAppUnits(
            ICON_SIZE + (2 * (ICON_PADDING + ALT_BORDER_WIDTH)));
        mIntrinsicSize.width.SetCoordValue(edgeLengthToUse);
        mIntrinsicSize.height.SetCoordValue(edgeLengthToUse);
        mIntrinsicRatio.SizeTo(1, 1);
      }
    }
  }
}

 nsSize
nsImageFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                          nsSize aCBSize, nscoord aAvailableWidth,
                          nsSize aMargin, nsSize aBorder, nsSize aPadding,
                          uint32_t aFlags)
{
  nsPresContext *presContext = PresContext();
  EnsureIntrinsicSizeAndRatio(presContext);

  return nsLayoutUtils::ComputeSizeWithIntrinsicDimensions(
                            aRenderingContext, this,
                            mIntrinsicSize, mIntrinsicRatio, aCBSize,
                            aMargin, aBorder, aPadding);
}

nsRect 
nsImageFrame::GetInnerArea() const
{
  return GetContentRect() - GetPosition();
}


nscoord 
nsImageFrame::GetContinuationOffset() const
{
  nscoord offset = 0;
  for (nsIFrame *f = GetPrevInFlow(); f; f = f->GetPrevInFlow()) {
    offset += f->GetContentRect().height;
  }
  NS_ASSERTION(offset >= 0, "bogus GetContentRect");
  return offset;
}

 nscoord
nsImageFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  
  
  DebugOnly<nscoord> result;
  DISPLAY_MIN_WIDTH(this, result);
  nsPresContext *presContext = PresContext();
  EnsureIntrinsicSizeAndRatio(presContext);
  return mIntrinsicSize.width.GetUnit() == eStyleUnit_Coord ?
    mIntrinsicSize.width.GetCoordValue() : 0;
}

 nscoord
nsImageFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  
  
  DebugOnly<nscoord> result;
  DISPLAY_PREF_WIDTH(this, result);
  nsPresContext *presContext = PresContext();
  EnsureIntrinsicSizeAndRatio(presContext);
  
  return mIntrinsicSize.width.GetUnit() == eStyleUnit_Coord ?
    mIntrinsicSize.width.GetCoordValue() : 0;
}

 nsIFrame::IntrinsicSize
nsImageFrame::GetIntrinsicSize()
{
  return mIntrinsicSize;
}

 nsSize
nsImageFrame::GetIntrinsicRatio()
{
  return mIntrinsicRatio;
}

NS_IMETHODIMP
nsImageFrame::Reflow(nsPresContext*          aPresContext,
                     nsHTMLReflowMetrics&     aMetrics,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsImageFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                  ("enter nsImageFrame::Reflow: availSize=%d,%d",
                  aReflowState.availableWidth, aReflowState.availableHeight));

  NS_PRECONDITION(mState & NS_FRAME_IN_REFLOW, "frame is not in reflow");

  aStatus = NS_FRAME_COMPLETE;

  
  if (HaveFixedSize(aReflowState)) {
    mState |= IMAGE_SIZECONSTRAINED;
  } else {
    mState &= ~IMAGE_SIZECONSTRAINED;
  }

  
  
  if (GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    mState |= IMAGE_GOTINITIALREFLOW;
  }

  mComputedSize = 
    nsSize(aReflowState.ComputedWidth(), aReflowState.ComputedHeight());

  aMetrics.width = mComputedSize.width;
  aMetrics.height = mComputedSize.height;

  
  aMetrics.width  += aReflowState.mComputedBorderPadding.LeftRight();
  aMetrics.height += aReflowState.mComputedBorderPadding.TopBottom();
  
  if (GetPrevInFlow()) {
    aMetrics.width = GetPrevInFlow()->GetSize().width;
    nscoord y = GetContinuationOffset();
    aMetrics.height -= y + aReflowState.mComputedBorderPadding.top;
    aMetrics.height = std::max(0, aMetrics.height);
  }


  
  
  uint32_t loadStatus = imgIRequest::STATUS_NONE;
  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mContent);
  NS_ASSERTION(imageLoader, "No content node??");
  if (imageLoader) {
    nsCOMPtr<imgIRequest> currentRequest;
    imageLoader->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                            getter_AddRefs(currentRequest));
    if (currentRequest) {
      currentRequest->GetImageStatus(&loadStatus);
    }
  }
  if (aPresContext->IsPaginated() &&
      ((loadStatus & imgIRequest::STATUS_SIZE_AVAILABLE) || (mState & IMAGE_SIZECONSTRAINED)) &&
      NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight && 
      aMetrics.height > aReflowState.availableHeight) { 
    
    
    aMetrics.height = std::max(nsPresContext::CSSPixelsToAppUnits(1), aReflowState.availableHeight);
    aStatus = NS_FRAME_NOT_COMPLETE;
  }

  aMetrics.SetOverflowAreasToDesiredBounds();
  nsEventStates contentState = mContent->AsElement()->State();
  bool imageOK = IMAGE_OK(contentState, true);

  
  bool haveSize = false;
  if (loadStatus & imgIRequest::STATUS_SIZE_AVAILABLE) {
    haveSize = true;
  }

  if (!imageOK || !haveSize) {
    nsRect altFeedbackSize(0, 0,
                           nsPresContext::CSSPixelsToAppUnits(ICON_SIZE+2*(ICON_PADDING+ALT_BORDER_WIDTH)),
                           nsPresContext::CSSPixelsToAppUnits(ICON_SIZE+2*(ICON_PADDING+ALT_BORDER_WIDTH)));
    
    
    
    MOZ_STATIC_ASSERT(eOverflowType_LENGTH == 2, "Unknown overflow types?");
    nsRect& visualOverflow = aMetrics.VisualOverflow();
    visualOverflow.UnionRect(visualOverflow, altFeedbackSize);
  }
  FinishAndStoreOverflow(&aMetrics);

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                  ("exit nsImageFrame::Reflow: size=%d,%d",
                  aMetrics.width, aMetrics.height));
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
  return NS_OK;
}






nscoord
nsImageFrame::MeasureString(const PRUnichar*     aString,
                            int32_t              aLength,
                            nscoord              aMaxWidth,
                            uint32_t&            aMaxFit,
                            nsRenderingContext& aContext)
{
  nscoord totalWidth = 0;
  aContext.SetTextRunRTL(false);
  nscoord spaceWidth = aContext.GetWidth(' ');

  aMaxFit = 0;
  while (aLength > 0) {
    
    uint32_t  len = aLength;
    bool      trailingSpace = false;
    for (int32_t i = 0; i < aLength; i++) {
      if (dom::IsSpaceCharacter(aString[i]) && (i > 0)) {
        len = i;  
        trailingSpace = true;
        break;
      }
    }
  
    
    nscoord width =
      nsLayoutUtils::GetStringWidth(this, &aContext, aString, len);
    bool    fits = (totalWidth + width) <= aMaxWidth;

    
    
    if (fits || (0 == totalWidth)) {
      
      totalWidth += width;

      
      if (trailingSpace) {
        if ((totalWidth + spaceWidth) <= aMaxWidth) {
          totalWidth += spaceWidth;
        } else {
          
          
          fits = false;
        }

        len++;
      }

      aMaxFit += len;
      aString += len;
      aLength -= len;
    }

    if (!fits) {
      break;
    }
  }
  return totalWidth;
}



void
nsImageFrame::DisplayAltText(nsPresContext*      aPresContext,
                             nsRenderingContext& aRenderingContext,
                             const nsString&      aAltText,
                             const nsRect&        aRect)
{
  
  aRenderingContext.SetColor(GetStyleColor()->mColor);
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
    nsLayoutUtils::FontSizeInflationFor(this));
  aRenderingContext.SetFont(fm);

  

  nscoord maxAscent = fm->MaxAscent();
  nscoord maxDescent = fm->MaxDescent();
  nscoord height = fm->MaxHeight();

  
  
  
  const PRUnichar* str = aAltText.get();
  int32_t          strLen = aAltText.Length();
  nscoord          y = aRect.y;

  if (!aPresContext->BidiEnabled() && HasRTLChars(aAltText)) {
    aPresContext->SetBidiEnabled();
  }

  
  bool firstLine = true;
  while ((strLen > 0) && (firstLine || (y + maxDescent) < aRect.YMost())) {
    
    uint32_t  maxFit;  
    nscoord strWidth = MeasureString(str, strLen, aRect.width, maxFit,
                                     aRenderingContext);
    
    
    nsresult rv = NS_ERROR_FAILURE;

    if (aPresContext->BidiEnabled()) {
      const nsStyleVisibility* vis = GetStyleVisibility();
      if (vis->mDirection == NS_STYLE_DIRECTION_RTL)
        rv = nsBidiPresUtils::RenderText(str, maxFit, NSBIDI_RTL,
                                         aPresContext, aRenderingContext,
                                         aRenderingContext,
                                         aRect.XMost() - strWidth, y + maxAscent);
      else
        rv = nsBidiPresUtils::RenderText(str, maxFit, NSBIDI_LTR,
                                         aPresContext, aRenderingContext,
                                         aRenderingContext,
                                         aRect.x, y + maxAscent);
    }
    if (NS_FAILED(rv))
      aRenderingContext.DrawString(str, maxFit, aRect.x, y + maxAscent);

    
    str += maxFit;
    strLen -= maxFit;
    y += height;
    firstLine = false;
  }
}

struct nsRecessedBorder : public nsStyleBorder {
  nsRecessedBorder(nscoord aBorderWidth, nsPresContext* aPresContext)
    : nsStyleBorder(aPresContext)
  {
    NS_FOR_CSS_SIDES(side) {
      
      
      SetBorderColor(side, NS_RGB(0, 0, 0));
      mBorder.Side(side) = aBorderWidth;
      
      
      SetBorderStyle(side, NS_STYLE_BORDER_STYLE_INSET);
    }
  }
};

class nsDisplayAltFeedback : public nsDisplayItem {
public:
  nsDisplayAltFeedback(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame) {}

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
  {
    *aSnap = false;
    return mFrame->GetVisualOverflowRectRelativeToSelf() + ToReferenceFrame();
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx)
  {
    nsImageFrame* f = static_cast<nsImageFrame*>(mFrame);
    nsEventStates state = f->GetContent()->AsElement()->State();
    f->DisplayAltFeedback(*aCtx,
                          mVisibleRect,
                          IMAGE_OK(state, true)
                             ? nsImageFrame::gIconLoad->mLoadingImage
                             : nsImageFrame::gIconLoad->mBrokenImage,
                          ToReferenceFrame());

  }

  NS_DISPLAY_DECL_NAME("AltFeedback", TYPE_ALT_FEEDBACK)
};

void
nsImageFrame::DisplayAltFeedback(nsRenderingContext& aRenderingContext,
                                 const nsRect&        aDirtyRect,
                                 imgIRequest*         aRequest,
                                 nsPoint              aPt)
{
  
  NS_ABORT_IF_FALSE(gIconLoad, "How did we succeed in Init then?");

  
  nsRect  inner = GetInnerArea() + aPt;

  
  nscoord borderEdgeWidth = nsPresContext::CSSPixelsToAppUnits(ALT_BORDER_WIDTH);

  
  if (inner.IsEmpty()){
    inner.SizeTo(2*(nsPresContext::CSSPixelsToAppUnits(ICON_SIZE+ICON_PADDING+ALT_BORDER_WIDTH)),
                 2*(nsPresContext::CSSPixelsToAppUnits(ICON_SIZE+ICON_PADDING+ALT_BORDER_WIDTH)));
  }

  
  
  if ((inner.width < 2 * borderEdgeWidth) || (inner.height < 2 * borderEdgeWidth)) {
    return;
  }

  
  nsRecessedBorder recessedBorder(borderEdgeWidth, PresContext());
  nsCSSRendering::PaintBorderWithStyleBorder(PresContext(), aRenderingContext,
                                             this, inner, inner,
                                             recessedBorder, mStyleContext);

  
  
  inner.Deflate(nsPresContext::CSSPixelsToAppUnits(ICON_PADDING+ALT_BORDER_WIDTH), 
                nsPresContext::CSSPixelsToAppUnits(ICON_PADDING+ALT_BORDER_WIDTH));
  if (inner.IsEmpty()) {
    return;
  }

  
  aRenderingContext.PushState();
  aRenderingContext.IntersectClip(inner);

  
  if (gIconLoad->mPrefShowPlaceholders) {
    const nsStyleVisibility* vis = GetStyleVisibility();
    nscoord size = nsPresContext::CSSPixelsToAppUnits(ICON_SIZE);

    bool iconUsed = false;

    
    
    
    if (aRequest && !mDisplayingIcon) {
      gIconLoad->AddIconObserver(this);
      mDisplayingIcon = true;
    }


    
    uint32_t imageStatus = 0;
    if (aRequest)
      aRequest->GetImageStatus(&imageStatus);
    if (imageStatus & imgIRequest::STATUS_FRAME_COMPLETE) {
      nsCOMPtr<imgIContainer> imgCon;
      aRequest->GetImage(getter_AddRefs(imgCon));
      NS_ABORT_IF_FALSE(imgCon, "Frame Complete, but no image container?");
      nsRect dest((vis->mDirection == NS_STYLE_DIRECTION_RTL) ?
                  inner.XMost() - size : inner.x,
                  inner.y, size, size);
      nsLayoutUtils::DrawSingleImage(&aRenderingContext, imgCon,
        nsLayoutUtils::GetGraphicsFilterForFrame(this), dest, aDirtyRect,
        imgIContainer::FLAG_NONE);
      iconUsed = true;
    }

    
    
    if (!iconUsed) {
      nscoord iconXPos = (vis->mDirection ==   NS_STYLE_DIRECTION_RTL) ?
                         inner.XMost() - size : inner.x;
      nscoord twoPX = nsPresContext::CSSPixelsToAppUnits(2);
      aRenderingContext.DrawRect(iconXPos, inner.y,size,size);
      aRenderingContext.PushState();
      aRenderingContext.SetColor(NS_RGB(0xFF,0,0));
      aRenderingContext.FillEllipse(size/2 + iconXPos, size/2 + inner.y,
                                    size/2 - twoPX, size/2 - twoPX);
      aRenderingContext.PopState();
    }

    
    
    int32_t iconWidth = nsPresContext::CSSPixelsToAppUnits(ICON_SIZE + ICON_PADDING);
    if (vis->mDirection != NS_STYLE_DIRECTION_RTL)
      inner.x += iconWidth;
    inner.width -= iconWidth;
  }

  
  if (!inner.IsEmpty()) {
    nsIContent* content = GetContent();
    if (content) {
      nsXPIDLString altText;
      nsCSSFrameConstructor::GetAlternateTextFor(content, content->Tag(),
                                                 altText);
      DisplayAltText(PresContext(), aRenderingContext, altText, inner);
    }
  }

  aRenderingContext.PopState();
}

#ifdef DEBUG
static void PaintDebugImageMap(nsIFrame* aFrame, nsRenderingContext* aCtx,
     const nsRect& aDirtyRect, nsPoint aPt) {
  nsImageFrame* f = static_cast<nsImageFrame*>(aFrame);
  nsRect inner = f->GetInnerArea() + aPt;

  aCtx->SetColor(NS_RGB(0, 0, 0));
  aCtx->PushState();
  aCtx->Translate(inner.TopLeft());
  f->GetImageMap()->Draw(aFrame, *aCtx);
  aCtx->PopState();
}
#endif

void
nsDisplayImage::Paint(nsDisplayListBuilder* aBuilder,
                      nsRenderingContext* aCtx) {
  static_cast<nsImageFrame*>(mFrame)->
    PaintImage(*aCtx, ToReferenceFrame(), mVisibleRect, mImage,
               aBuilder->ShouldSyncDecodeImages()
                 ? (uint32_t) imgIContainer::FLAG_SYNC_DECODE
                 : (uint32_t) imgIContainer::FLAG_NONE);
}

already_AddRefed<ImageContainer>
nsDisplayImage::GetContainer(LayerManager* aManager,
                             nsDisplayListBuilder* aBuilder)
{
  nsRefPtr<ImageContainer> container;
  nsresult rv = mImage->GetImageContainer(aManager, getter_AddRefs(container));
  NS_ENSURE_SUCCESS(rv, nullptr);
  return container.forget();
}

gfxRect
nsDisplayImage::GetDestRect()
{
  int32_t factor = mFrame->PresContext()->AppUnitsPerDevPixel();
  nsImageFrame* imageFrame = static_cast<nsImageFrame*>(mFrame);

  nsRect dest = imageFrame->GetInnerArea() + ToReferenceFrame();
  gfxRect destRect(dest.x, dest.y, dest.width, dest.height);
  destRect.ScaleInverse(factor); 

  return destRect;
}

LayerState
nsDisplayImage::GetLayerState(nsDisplayListBuilder* aBuilder,
                              LayerManager* aManager,
                              const FrameLayerBuilder::ContainerParameters& aParameters)
{
  if (mImage->GetType() != imgIContainer::TYPE_RASTER ||
      !aManager->IsCompositingCheap() ||
      !nsLayoutUtils::GPUImageScalingEnabled()) {
    return LAYER_NONE;
  }

  int32_t imageWidth;
  int32_t imageHeight;
  mImage->GetWidth(&imageWidth);
  mImage->GetHeight(&imageHeight);

  NS_ASSERTION(imageWidth != 0 && imageHeight != 0, "Invalid image size!");

  gfxRect destRect = GetDestRect();

  destRect.width *= aParameters.mXScale;
  destRect.height *= aParameters.mYScale;

  
  gfxSize scale = gfxSize(destRect.width / imageWidth, destRect.height / imageHeight);

  
  if (scale.width == 1.0f && scale.height == 1.0f) {
    return LAYER_NONE;
  }

  
  if (destRect.width * destRect.height < 64 * 64) {
    return LAYER_NONE;
  }

  nsRefPtr<ImageContainer> container;
  mImage->GetImageContainer(aManager, getter_AddRefs(container));
  if (!container) {
    return LAYER_NONE;
  }

  return LAYER_ACTIVE;
}

already_AddRefed<Layer>
nsDisplayImage::BuildLayer(nsDisplayListBuilder* aBuilder,
                           LayerManager* aManager,
                           const ContainerParameters& aParameters)
{
  nsRefPtr<ImageContainer> container;
  nsresult rv = mImage->GetImageContainer(aManager, getter_AddRefs(container));
  NS_ENSURE_SUCCESS(rv, nullptr);

  nsRefPtr<ImageLayer> layer = aManager->CreateImageLayer();
  layer->SetContainer(container);
  ConfigureLayer(layer, aParameters.mOffset);
  return layer.forget();
}

void
nsDisplayImage::ConfigureLayer(ImageLayer *aLayer, const nsIntPoint& aOffset)
{
  aLayer->SetFilter(nsLayoutUtils::GetGraphicsFilterForFrame(mFrame));

  int32_t imageWidth;
  int32_t imageHeight;
  mImage->GetWidth(&imageWidth);
  mImage->GetHeight(&imageHeight);

  NS_ASSERTION(imageWidth != 0 && imageHeight != 0, "Invalid image size!");

  const gfxRect destRect = GetDestRect();

  gfxMatrix transform;
  transform.Translate(destRect.TopLeft() + aOffset);
  transform.Scale(destRect.Width()/imageWidth,
                  destRect.Height()/imageHeight);
  aLayer->SetBaseTransform(gfx3DMatrix::From2D(transform));
  aLayer->SetVisibleRegion(nsIntRect(0, 0, imageWidth, imageHeight));
}

void
nsImageFrame::PaintImage(nsRenderingContext& aRenderingContext, nsPoint aPt,
                         const nsRect& aDirtyRect, imgIContainer* aImage,
                         uint32_t aFlags)
{
  
  
  NS_ASSERTION(GetInnerArea().width == mComputedSize.width, "bad width");
  nsRect inner = GetInnerArea() + aPt;
  nsRect dest(inner.TopLeft(), mComputedSize);
  dest.y -= GetContinuationOffset();

  nsLayoutUtils::DrawSingleImage(&aRenderingContext, aImage,
    nsLayoutUtils::GetGraphicsFilterForFrame(this), dest, aDirtyRect,
    aFlags);

  nsImageMap* map = GetImageMap();
  if (nullptr != map) {
    aRenderingContext.PushState();
    aRenderingContext.SetColor(NS_RGB(0, 0, 0));
    aRenderingContext.SetLineStyle(nsLineStyle_kDotted);
    aRenderingContext.Translate(inner.TopLeft());
    map->Draw(this, aRenderingContext);
    aRenderingContext.PopState();
  }
}

void
nsImageFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                               const nsRect&           aDirtyRect,
                               const nsDisplayListSet& aLists)
{
  if (!IsVisibleForPainting(aBuilder))
    return;

  
  
  
  
  DisplayBorderBackgroundOutline(aBuilder, aLists);
  
  
  

  nsDisplayList replacedContent;
  if (mComputedSize.width != 0 && mComputedSize.height != 0) {
    nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mContent);
    NS_ASSERTION(imageLoader, "Not an image loading content?");

    nsCOMPtr<imgIRequest> currentRequest;
    if (imageLoader) {
      imageLoader->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                              getter_AddRefs(currentRequest));
    }

    nsEventStates contentState = mContent->AsElement()->State();
    bool imageOK = IMAGE_OK(contentState, true);

    nsCOMPtr<imgIContainer> imgCon;
    if (currentRequest) {
      currentRequest->GetImage(getter_AddRefs(imgCon));
    }

    
    bool haveSize = false;
    uint32_t imageStatus = 0;
    if (currentRequest)
      currentRequest->GetImageStatus(&imageStatus);
    if (imageStatus & imgIRequest::STATUS_SIZE_AVAILABLE)
      haveSize = true;

    
    NS_ABORT_IF_FALSE(!haveSize || imgCon, "Have size but not container?");

    if (!imageOK || !haveSize) {
      
      
      replacedContent.AppendNewToTop(new (aBuilder)
        nsDisplayAltFeedback(aBuilder, this));
    }
    else {
      replacedContent.AppendNewToTop(new (aBuilder)
        nsDisplayImage(aBuilder, this, imgCon));

      
      if (mDisplayingIcon) {
        gIconLoad->RemoveIconObserver(this);
        mDisplayingIcon = false;
      }

        
#ifdef DEBUG
      if (GetShowFrameBorders() && GetImageMap()) {
        aLists.Outlines()->AppendNewToTop(new (aBuilder)
          nsDisplayGeneric(aBuilder, this, PaintDebugImageMap, "DebugImageMap",
                           nsDisplayItem::TYPE_DEBUG_IMAGE_MAP));
      }
#endif
    }
  }

  if (ShouldDisplaySelection()) {
    DisplaySelectionOverlay(aBuilder, &replacedContent,
                            nsISelectionDisplay::DISPLAY_IMAGES);
  }

  WrapReplacedContentForBorderRadius(aBuilder, &replacedContent, aLists);
}

bool
nsImageFrame::ShouldDisplaySelection()
{
  
  nsresult result;
  nsPresContext* presContext = PresContext();
  int16_t displaySelection = presContext->PresShell()->GetSelectionFlags();
  if (!(displaySelection & nsISelectionDisplay::DISPLAY_IMAGES))
    return false;

#if IMAGE_EDITOR_CHECK
  
  
  if (displaySelection == nsISelectionDisplay::DISPLAY_ALL) 
  {
    nsCOMPtr<nsISelectionController> selCon;
    result = GetSelectionController(presContext, getter_AddRefs(selCon));
    if (NS_SUCCEEDED(result) && selCon)
    {
      nsCOMPtr<nsISelection> selection;
      result = selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));
      if (NS_SUCCEEDED(result) && selection)
      {
        int32_t rangeCount;
        selection->GetRangeCount(&rangeCount);
        if (rangeCount == 1) 
        {
          nsCOMPtr<nsIContent> parentContent = mContent->GetParent();
          if (parentContent)
          {
            int32_t thisOffset = parentContent->IndexOf(mContent);
            nsCOMPtr<nsIDOMNode> parentNode = do_QueryInterface(parentContent);
            nsCOMPtr<nsIDOMNode> rangeNode;
            int32_t rangeOffset;
            nsCOMPtr<nsIDOMRange> range;
            selection->GetRangeAt(0,getter_AddRefs(range));
            if (range)
            {
              range->GetStartContainer(getter_AddRefs(rangeNode));
              range->GetStartOffset(&rangeOffset);

              if (parentNode && rangeNode && (rangeNode == parentNode) && rangeOffset == thisOffset)
              {
                range->GetEndContainer(getter_AddRefs(rangeNode));
                range->GetEndOffset(&rangeOffset);
                if ((rangeNode == parentNode) && (rangeOffset == (thisOffset +1))) 
                  return false; 
              }
            }
          }
        }
      }
    }
  }
#endif
  return true;
}

nsImageMap*
nsImageFrame::GetImageMap()
{
  if (!mImageMap) {
    nsIContent* map = GetMapElement();
    if (map) {
      mImageMap = new nsImageMap();
      NS_ADDREF(mImageMap);
      mImageMap->Init(this, map);
    }
  }

  return mImageMap;
}

bool
nsImageFrame::IsServerImageMap()
{
  return mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::ismap);
}




void
nsImageFrame::TranslateEventCoords(const nsPoint& aPoint,
                                   nsIntPoint&     aResult)
{
  nscoord x = aPoint.x;
  nscoord y = aPoint.y;

  
  
  nsRect inner = GetInnerArea();
  x -= inner.x;
  y -= inner.y;

  aResult.x = nsPresContext::AppUnitsToIntCSSPixels(x);
  aResult.y = nsPresContext::AppUnitsToIntCSSPixels(y);
}

bool
nsImageFrame::GetAnchorHREFTargetAndNode(nsIURI** aHref, nsString& aTarget,
                                         nsIContent** aNode)
{
  bool status = false;
  aTarget.Truncate();
  *aHref = nullptr;
  *aNode = nullptr;

  
  for (nsIContent* content = mContent->GetParent();
       content; content = content->GetParent()) {
    nsCOMPtr<nsILink> link(do_QueryInterface(content));
    if (link) {
      nsCOMPtr<nsIURI> href = content->GetHrefURI();
      if (href) {
        href->Clone(aHref);
      }
      status = (*aHref != nullptr);

      nsCOMPtr<nsIDOMHTMLAnchorElement> anchor(do_QueryInterface(content));
      if (anchor) {
        anchor->GetTarget(aTarget);
      }
      NS_ADDREF(*aNode = content);
      break;
    }
  }
  return status;
}

NS_IMETHODIMP  
nsImageFrame::GetContentForEvent(nsEvent* aEvent,
                                 nsIContent** aContent)
{
  NS_ENSURE_ARG_POINTER(aContent);

  nsIFrame* f = nsLayoutUtils::GetNonGeneratedAncestor(this);
  if (f != this) {
    return f->GetContentForEvent(aEvent, aContent);
  }

  
  
  nsIContent* capturingContent =
    NS_IS_MOUSE_EVENT(aEvent) ? nsIPresShell::GetCapturingContent() : nullptr;
  if (capturingContent && capturingContent->GetPrimaryFrame() == this) {
    *aContent = capturingContent;
    NS_IF_ADDREF(*aContent);
    return NS_OK;
  }

  nsImageMap* map = GetImageMap();

  if (nullptr != map) {
    nsIntPoint p;
    TranslateEventCoords(
      nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this), p);
    nsCOMPtr<nsIContent> area = map->GetArea(p.x, p.y);
    if (area) {
      area.forget(aContent);
      return NS_OK;
    }
  }

  *aContent = GetContent();
  NS_IF_ADDREF(*aContent);
  return NS_OK;
}


NS_IMETHODIMP
nsImageFrame::HandleEvent(nsPresContext* aPresContext,
                          nsGUIEvent* aEvent,
                          nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);

  if ((aEvent->eventStructType == NS_MOUSE_EVENT &&
       aEvent->message == NS_MOUSE_BUTTON_UP && 
       static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton) ||
      aEvent->message == NS_MOUSE_MOVE) {
    nsImageMap* map = GetImageMap();
    bool isServerMap = IsServerImageMap();
    if ((nullptr != map) || isServerMap) {
      nsIntPoint p;
      TranslateEventCoords(
        nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this), p);
      bool inside = false;
      
      
      
      
      if (nullptr != map) {
        inside = !!map->GetArea(p.x, p.y);
      }

      if (!inside && isServerMap) {

        
        
        nsCOMPtr<nsIURI> uri;
        nsAutoString target;
        nsCOMPtr<nsIContent> anchorNode;
        if (GetAnchorHREFTargetAndNode(getter_AddRefs(uri), target,
                                       getter_AddRefs(anchorNode))) {
          
          
          
          
          
          if (p.x < 0) p.x = 0;
          if (p.y < 0) p.y = 0;
          nsAutoCString spec;
          uri->GetSpec(spec);
          spec += nsPrintfCString("?%d,%d", p.x, p.y);
          uri->SetSpec(spec);                
          
          bool clicked = false;
          if (aEvent->message == NS_MOUSE_BUTTON_UP) {
            *aEventStatus = nsEventStatus_eConsumeDoDefault; 
            clicked = true;
          }
          nsContentUtils::TriggerLink(anchorNode, aPresContext, uri, target,
                                      clicked, true, true);
        }
      }
    }
  }

  return nsSplittableFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}

NS_IMETHODIMP
nsImageFrame::GetCursor(const nsPoint& aPoint,
                        nsIFrame::Cursor& aCursor)
{
  nsImageMap* map = GetImageMap();
  if (nullptr != map) {
    nsIntPoint p;
    TranslateEventCoords(aPoint, p);
    nsCOMPtr<nsIContent> area = map->GetArea(p.x, p.y);
    if (area) {
      
      
      
      
      
      nsRefPtr<nsStyleContext> areaStyle = 
        PresContext()->PresShell()->StyleSet()->
          ResolveStyleFor(area->AsElement(), StyleContext());
      if (areaStyle) {
        FillCursorInformationFromStyle(areaStyle->GetStyleUserInterface(),
                                       aCursor);
        if (NS_STYLE_CURSOR_AUTO == aCursor.mCursor) {
          aCursor.mCursor = NS_STYLE_CURSOR_DEFAULT;
        }
        return NS_OK;
      }
    }
  }
  return nsFrame::GetCursor(aPoint, aCursor);
}

NS_IMETHODIMP
nsImageFrame::AttributeChanged(int32_t aNameSpaceID,
                               nsIAtom* aAttribute,
                               int32_t aModType)
{
  nsresult rv = nsSplittableFrame::AttributeChanged(aNameSpaceID,
                                                    aAttribute, aModType);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (nsGkAtoms::alt == aAttribute)
  {
    PresContext()->PresShell()->FrameNeedsReflow(this,
                                                 nsIPresShell::eStyleChange,
                                                 NS_FRAME_IS_DIRTY);
  }

  return NS_OK;
}

nsIAtom*
nsImageFrame::GetType() const
{
  return nsGkAtoms::imageFrame;
}

#ifdef DEBUG
NS_IMETHODIMP
nsImageFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ImageFrame"), aResult);
}

NS_IMETHODIMP
nsImageFrame::List(FILE* out, int32_t aIndent, uint32_t aFlags) const
{
  IndentBy(out, aIndent);
  ListTag(out);
#ifdef DEBUG_waterson
  fprintf(out, " [parent=%p]", mParent);
#endif
  if (HasView()) {
    fprintf(out, " [view=%p]", (void*)GetView());
  }
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%016llx]", (unsigned long long)mState);
  }
  fprintf(out, " [content=%p]", (void*)mContent);
  fprintf(out, " [sc=%p]", static_cast<void*>(mStyleContext));

  
  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mContent);
  if (imageLoader) {
    nsCOMPtr<imgIRequest> currentRequest;
    imageLoader->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                            getter_AddRefs(currentRequest));
    if (currentRequest) {
      nsCOMPtr<nsIURI> uri;
      currentRequest->GetURI(getter_AddRefs(uri));
      nsAutoCString uristr;
      uri->GetAsciiSpec(uristr);
      fprintf(out, " [src=%s]", uristr.get());
    }
  }
  fputs("\n", out);
  return NS_OK;
}
#endif

int
nsImageFrame::GetSkipSides() const
{
  int skip = 0;
  if (nullptr != GetPrevInFlow()) {
    skip |= 1 << NS_SIDE_TOP;
  }
  if (nullptr != GetNextInFlow()) {
    skip |= 1 << NS_SIDE_BOTTOM;
  }
  return skip;
}

nsresult
nsImageFrame::GetIntrinsicImageSize(nsSize& aSize)
{
  if (mIntrinsicSize.width.GetUnit() == eStyleUnit_Coord &&
      mIntrinsicSize.height.GetUnit() == eStyleUnit_Coord) {
    aSize.SizeTo(mIntrinsicSize.width.GetCoordValue(),
                 mIntrinsicSize.height.GetCoordValue());
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsImageFrame::LoadIcon(const nsAString& aSpec,
                       nsPresContext *aPresContext,
                       imgRequestProxy** aRequest)
{
  nsresult rv = NS_OK;
  NS_PRECONDITION(!aSpec.IsEmpty(), "What happened??");

  if (!sIOService) {
    rv = CallGetService(NS_IOSERVICE_CONTRACTID, &sIOService);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIURI> realURI;
  SpecToURI(aSpec, sIOService, getter_AddRefs(realURI));
 
  nsRefPtr<imgLoader> il =
    nsContentUtils::GetImgLoaderForDocument(aPresContext->Document());

  nsCOMPtr<nsILoadGroup> loadGroup;
  GetLoadGroup(aPresContext, getter_AddRefs(loadGroup));

  
  nsLoadFlags loadFlags = nsIRequest::LOAD_NORMAL;

  return il->LoadImage(realURI,     
                       nullptr,      


                       nullptr,      
                       nullptr,      
                       loadGroup,
                       gIconLoad,
                       nullptr,      
                       loadFlags,
                       nullptr,
                       nullptr,      
                       aRequest);
}

void
nsImageFrame::GetDocumentCharacterSet(nsACString& aCharset) const
{
  if (mContent) {
    NS_ASSERTION(mContent->GetDocument(),
                 "Frame still alive after content removed from document!");
    aCharset = mContent->GetDocument()->GetDocumentCharacterSet();
  }
}

void
nsImageFrame::SpecToURI(const nsAString& aSpec, nsIIOService *aIOService,
                         nsIURI **aURI)
{
  nsCOMPtr<nsIURI> baseURI;
  if (mContent) {
    baseURI = mContent->GetBaseURI();
  }
  nsAutoCString charset;
  GetDocumentCharacterSet(charset);
  NS_NewURI(aURI, aSpec, 
            charset.IsEmpty() ? nullptr : charset.get(), 
            baseURI, aIOService);
}

void
nsImageFrame::GetLoadGroup(nsPresContext *aPresContext, nsILoadGroup **aLoadGroup)
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

  *aLoadGroup = doc->GetDocumentLoadGroup().get();  
}

nsresult nsImageFrame::LoadIcons(nsPresContext *aPresContext)
{
  NS_ASSERTION(!gIconLoad, "called LoadIcons twice");

  NS_NAMED_LITERAL_STRING(loadingSrc,"resource://gre-resources/loading-image.png");
  NS_NAMED_LITERAL_STRING(brokenSrc,"resource://gre-resources/broken-image.png");

  gIconLoad = new IconLoad();
  NS_ADDREF(gIconLoad);

  nsresult rv;
  
  rv = LoadIcon(loadingSrc,
                aPresContext,
                getter_AddRefs(gIconLoad->mLoadingImage));
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = LoadIcon(brokenSrc,
                aPresContext,
                getter_AddRefs(gIconLoad->mBrokenImage));
  return rv;
}

NS_IMPL_ISUPPORTS2(nsImageFrame::IconLoad, nsIObserver,
                   imgINotificationObserver)

static const char* kIconLoadPrefs[] = {
  "browser.display.force_inline_alttext",
  "browser.display.show_image_placeholders",
  nullptr
};

nsImageFrame::IconLoad::IconLoad()
{
  
  Preferences::AddStrongObservers(this, kIconLoadPrefs);
  GetPrefs();
}

void
nsImageFrame::IconLoad::Shutdown()
{
  Preferences::RemoveObservers(this, kIconLoadPrefs);
  
  if (mLoadingImage) {
    mLoadingImage->CancelAndForgetObserver(NS_ERROR_FAILURE);
    mLoadingImage = nullptr;
  }
  if (mBrokenImage) {
    mBrokenImage->CancelAndForgetObserver(NS_ERROR_FAILURE);
    mBrokenImage = nullptr;
  }
}

NS_IMETHODIMP
nsImageFrame::IconLoad::Observe(nsISupports *aSubject, const char* aTopic,
                                const PRUnichar* aData)
{
  NS_ASSERTION(!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID),
               "wrong topic");
#ifdef DEBUG
  
  for (uint32_t i = 0; i < ArrayLength(kIconLoadPrefs) ||
                       (NS_NOTREACHED("wrong pref"), false); ++i)
    if (NS_ConvertASCIItoUTF16(kIconLoadPrefs[i]) == nsDependentString(aData))
      break;
#endif

  GetPrefs();
  return NS_OK;
}

void nsImageFrame::IconLoad::GetPrefs()
{
  mPrefForceInlineAltText =
    Preferences::GetBool("browser.display.force_inline_alttext");

  mPrefShowPlaceholders =
    Preferences::GetBool("browser.display.show_image_placeholders", true);
}

NS_IMETHODIMP
nsImageFrame::IconLoad::Notify(imgIRequest *aRequest, int32_t aType, const nsIntRect* aData)
{
  if (aType != imgINotificationObserver::LOAD_COMPLETE &&
      aType != imgINotificationObserver::FRAME_UPDATE) {
    return NS_OK;
  }

  nsTObserverArray<nsImageFrame*>::ForwardIterator iter(mIconObservers);
  nsImageFrame *frame;
  while (iter.HasMore()) {
    frame = iter.GetNext();
    frame->InvalidateFrame();
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsImageListener, imgINotificationObserver)

nsImageListener::nsImageListener(nsImageFrame *aFrame) :
  mFrame(aFrame)
{
}

nsImageListener::~nsImageListener()
{
}

NS_IMETHODIMP
nsImageListener::Notify(imgIRequest *aRequest, int32_t aType, const nsIntRect* aData)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->Notify(aRequest, aType, aData);
}

static bool
IsInAutoWidthTableCellForQuirk(nsIFrame *aFrame)
{
  if (eCompatibility_NavQuirks != aFrame->PresContext()->CompatibilityMode())
    return false;
  
  nsBlockFrame *ancestor = nsLayoutUtils::FindNearestBlockAncestor(aFrame);
  if (ancestor->StyleContext()->GetPseudo() == nsCSSAnonBoxes::cellContent) {
    
    nsFrame *grandAncestor = static_cast<nsFrame*>(ancestor->GetParent());
    return grandAncestor &&
      grandAncestor->GetStylePosition()->mWidth.GetUnit() == eStyleUnit_Auto;
  }
  return false;
}

 void
nsImageFrame::AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                                nsIFrame::InlineMinWidthData *aData)
{

  NS_ASSERTION(GetParent(), "Must have a parent if we get here!");
  
  bool canBreak =
    !CanContinueTextRun() &&
    GetParent()->GetStyleText()->WhiteSpaceCanWrap() &&
    !IsInAutoWidthTableCellForQuirk(this);

  if (canBreak)
    aData->OptionallyBreak(aRenderingContext);
 
  aData->trailingWhitespace = 0;
  aData->skipWhitespace = false;
  aData->trailingTextFrame = nullptr;
  aData->currentLine += nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                            this, nsLayoutUtils::MIN_WIDTH);
  aData->atStartOfLine = false;

  if (canBreak)
    aData->OptionallyBreak(aRenderingContext);

}
