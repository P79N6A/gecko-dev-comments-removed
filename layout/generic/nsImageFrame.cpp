







































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
#include "nsIURL.h"
#include "nsILoadGroup.h"
#include "nsISupportsPriority.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"
#include "nsHTMLContainerFrame.h"
#include "prprf.h"
#include "nsCSSRendering.h"
#include "nsILink.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsINameSpaceManager.h"
#include "nsTextFragment.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIScriptSecurityManager.h"
#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsGUIEvent.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"

#include "imgIContainer.h"
#include "imgILoader.h"

#include "nsCSSFrameConstructor.h"
#include "nsIDOMRange.h"

#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsEventStates.h"
#include "nsLayoutErrors.h"
#include "nsBidiUtils.h"
#include "nsBidiPresUtils.h"

#include "gfxRect.h"
#include "ImageLayers.h"

#include "mozilla/Preferences.h"
#include "mozilla/Util.h" 

using namespace mozilla;


#define ICON_SIZE        (16)
#define ICON_PADDING     (3)
#define ALT_BORDER_WIDTH (1)



#define IMAGE_EDITOR_CHECK 1


#define ALIGN_UNSET PRUint8(-1)

using namespace mozilla::layers;
using namespace mozilla::dom;


nsImageFrame::IconLoad* nsImageFrame::gIconLoad = nsnull;


nsIIOService* nsImageFrame::sIOService;


static PRBool HaveFixedSize(const nsStylePosition* aStylePosition)
{
  
  
  
  return aStylePosition->mWidth.IsCoordPercentCalcUnit() &&
         aStylePosition->mHeight.IsCoordPercentCalcUnit();
}



inline PRBool HaveFixedSize(const nsHTMLReflowState& aReflowState)
{ 
  NS_ASSERTION(aReflowState.mStylePosition, "crappy reflowState - null stylePosition");
  
  
  
  
  
  
  
  const nsStyleCoord &height = aReflowState.mStylePosition->mHeight;
  const nsStyleCoord &width = aReflowState.mStylePosition->mWidth;
  return ((height.HasPercent() &&
           NS_UNCONSTRAINEDSIZE == aReflowState.ComputedHeight()) ||
          (width.HasPercent() &&
           (NS_UNCONSTRAINEDSIZE == aReflowState.ComputedWidth() ||
            0 == aReflowState.ComputedWidth())))
          ? PR_FALSE
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
  mDisplayingIcon(PR_FALSE)
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
already_AddRefed<nsAccessible>
nsImageFrame::CreateAccessible()
{
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (accService) {
    return accService->CreateHTMLImageAccessible(mContent,
                                                 PresContext()->PresShell());
  }

  return nsnull;
}
#endif

void
nsImageFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  
  
  if (mImageMap) {
    mImageMap->Destroy();
    NS_RELEASE(mImageMap);
  }

  
  if (mListener) {
    nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mContent);
    if (imageLoader) {
      
      
      
      nsCxPusher pusher;
      pusher.PushNull();

      imageLoader->RemoveObserver(mListener);
    }
    
    reinterpret_cast<nsImageListener*>(mListener.get())->SetFrame(nsnull);
  }
  
  mListener = nsnull;

  
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

PRBool
nsImageFrame::UpdateIntrinsicSize(imgIContainer* aImage)
{
  NS_PRECONDITION(aImage, "null image");
  if (!aImage)
    return PR_FALSE;

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

PRBool
nsImageFrame::UpdateIntrinsicRatio(imgIContainer* aImage)
{
  NS_PRECONDITION(aImage, "null image");

  if (!aImage)
    return PR_FALSE;

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

PRBool
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
    return PR_TRUE;
  }

  return PR_FALSE;
}








PRBool
nsImageFrame::IsPendingLoad(imgIRequest* aRequest) const
{
  
  nsCOMPtr<nsIImageLoadingContent> imageLoader(do_QueryInterface(mContent));
  NS_ASSERTION(imageLoader, "No image loading content?");

  PRInt32 requestType = nsIImageLoadingContent::UNKNOWN_REQUEST;
  imageLoader->GetRequestType(aRequest, &requestType);

  return requestType != nsIImageLoadingContent::CURRENT_REQUEST;
}

PRBool
nsImageFrame::IsPendingLoad(imgIContainer* aContainer) const
{
  
  if (!aContainer) {
    NS_ERROR("No image container!");
    return PR_TRUE;
  }

  nsCOMPtr<nsIImageLoadingContent> imageLoader(do_QueryInterface(mContent));
  NS_ASSERTION(imageLoader, "No image loading content?");
  
  nsCOMPtr<imgIRequest> currentRequest;
  imageLoader->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                          getter_AddRefs(currentRequest));
  if (!currentRequest) {
    NS_ERROR("No current request");
    return PR_TRUE;
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


PRBool
nsImageFrame::ShouldCreateImageFrameFor(Element* aElement,
                                        nsStyleContext* aStyleContext)
{
  nsEventStates state = aElement->State();
  if (IMAGE_OK(state,
               HaveFixedSize(aStyleContext->GetStylePosition()))) {
    
    return PR_TRUE;
  }

  
  
  
  
  
  
  
  
  
  
  
  PRBool useSizedBox;
  
  if (aStyleContext->GetStyleUIReset()->mForceBrokenImageIcon) {
    useSizedBox = PR_TRUE;
  }
  else if (gIconLoad && gIconLoad->mPrefForceInlineAltText) {
    useSizedBox = PR_FALSE;
  }
  else {
    if (aStyleContext->PresContext()->CompatibilityMode() !=
        eCompatibility_NavQuirks) {
      useSizedBox = PR_FALSE;
    }
    else {
      
      
      nsIAtom *localName = aElement->Tag();

      
      
      
      if (!aElement->HasAttr(kNameSpaceID_None, nsGkAtoms::alt) &&
          localName != nsGkAtoms::object &&
          localName != nsGkAtoms::input) {
        useSizedBox = PR_TRUE;
      }
      else {
        
        useSizedBox = HaveFixedSize(aStyleContext->GetStylePosition());
      }
    }
  }
  
  return useSizedBox;
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
  
  UpdateIntrinsicSize(aImage);
  UpdateIntrinsicRatio(aImage);

  if (mState & IMAGE_GOTINITIALREFLOW) {
    
    
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
                              PRBool aCurrentFrame,
                              const nsIntRect *aRect)
{
  
  
  

  NS_ENSURE_ARG_POINTER(aRect);

  if (!(mState & IMAGE_GOTINITIALREFLOW)) {
    
    return NS_OK;
  }
  
  if (IsPendingLoad(aRequest)) {
    
    return NS_OK;
  }

  
  
  if (!aCurrentFrame)
    return NS_OK;

  
  
  nsRect r = aRect->IsEqualInterior(nsIntRect::GetMaxSizedIntRect()) ?
    GetInnerArea() :
    SourceRectToDest(*aRect);

#ifdef DEBUG_decode
  printf("Source rect (%d,%d,%d,%d) -> invalidate dest rect (%d,%d,%d,%d)\n",
         aRect->x, aRect->y, aRect->width, aRect->height,
         r.x, r.y, r.width, r.height);
#endif

  mImageContainer = nsnull;
  Invalidate(r);
  
  return NS_OK;
}

nsresult
nsImageFrame::OnStopDecode(imgIRequest *aRequest,
                           nsresult aStatus,
                           const PRUnichar *aStatusArg)
{
  nsPresContext *presContext = PresContext();
  nsIPresShell *presShell = presContext->GetPresShell();
  NS_ASSERTION(presShell, "No PresShell.");

  
  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mContent);
  NS_ASSERTION(imageLoader, "Who's notifying us??");
  PRInt32 loadType = nsIImageLoadingContent::UNKNOWN_REQUEST;
  imageLoader->GetRequestType(aRequest, &loadType);
  if (loadType != nsIImageLoadingContent::CURRENT_REQUEST &&
      loadType != nsIImageLoadingContent::PENDING_REQUEST) {
    return NS_ERROR_FAILURE;
  }

  if (loadType == nsIImageLoadingContent::PENDING_REQUEST) {
    
    PRBool intrinsicSizeChanged = PR_TRUE;
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
        if (presShell) { 
          presShell->FrameNeedsReflow(this, nsIPresShell::eStyleChange,
                                      NS_FRAME_IS_DIRTY);
        }
      } else {
        nsSize s = GetSize();
        nsRect r(0, 0, s.width, s.height);
        
        Invalidate(r);
      }
    }
  }

  return NS_OK;
}

nsresult
nsImageFrame::FrameChanged(imgIContainer *aContainer,
                           const nsIntRect *aDirtyRect)
{
  if (!GetStyleVisibility()->IsVisible()) {
    return NS_OK;
  }

  if (IsPendingLoad(aContainer)) {
    
    return NS_OK;
  }

  nsRect r = aDirtyRect->IsEqualInterior(nsIntRect::GetMaxSizedIntRect()) ?
    GetInnerArea() :
    SourceRectToDest(*aDirtyRect);

  
  Invalidate(r);
  mImageContainer = nsnull;
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
    PRUint32 status = 0;
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
                          PRBool aShrinkWrap)
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
    aMetrics.height = NS_MAX(0, aMetrics.height);
  }


  
  
  PRUint32 loadStatus = imgIRequest::STATUS_NONE;
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
    
    
    aMetrics.height = NS_MAX(nsPresContext::CSSPixelsToAppUnits(1), aReflowState.availableHeight);
    aStatus = NS_FRAME_NOT_COMPLETE;
  }

  aMetrics.SetOverflowAreasToDesiredBounds();
  FinishAndStoreOverflow(&aMetrics);

  
  
  
  
  
  if (mRect.width != aMetrics.width || mRect.height != aMetrics.height) {
    Invalidate(nsRect(0, 0, mRect.width, mRect.height));
  }

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                  ("exit nsImageFrame::Reflow: size=%d,%d",
                  aMetrics.width, aMetrics.height));
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
  return NS_OK;
}






nscoord
nsImageFrame::MeasureString(const PRUnichar*     aString,
                            PRInt32              aLength,
                            nscoord              aMaxWidth,
                            PRUint32&            aMaxFit,
                            nsRenderingContext& aContext)
{
  nscoord totalWidth = 0;
  aContext.SetTextRunRTL(PR_FALSE);
  nscoord spaceWidth = aContext.GetWidth(' ');

  aMaxFit = 0;
  while (aLength > 0) {
    
    PRUint32  len = aLength;
    PRBool    trailingSpace = PR_FALSE;
    for (PRInt32 i = 0; i < aLength; i++) {
      if (XP_IS_SPACE(aString[i]) && (i > 0)) {
        len = i;  
        trailingSpace = PR_TRUE;
        break;
      }
    }
  
    
    nscoord width =
      nsLayoutUtils::GetStringWidth(this, &aContext, aString, len);
    PRBool  fits = (totalWidth + width) <= aMaxWidth;

    
    
    if (fits || (0 == totalWidth)) {
      
      totalWidth += width;

      
      if (trailingSpace) {
        if ((totalWidth + spaceWidth) <= aMaxWidth) {
          totalWidth += spaceWidth;
        } else {
          
          
          fits = PR_FALSE;
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
  nsLayoutUtils::SetFontFromStyle(&aRenderingContext, mStyleContext);

  
  nsFontMetrics* fm = aRenderingContext.FontMetrics();

  nscoord maxAscent = fm->MaxAscent();
  nscoord maxDescent = fm->MaxDescent();
  nscoord height = fm->MaxHeight();

  
  
  
  const PRUnichar* str = aAltText.get();
  PRInt32          strLen = aAltText.Length();
  nscoord          y = aRect.y;

  if (!aPresContext->BidiEnabled() && HasRTLChars(aAltText)) {
    aPresContext->SetBidiEnabled();
  }

  
  PRBool firstLine = PR_TRUE;
  while ((strLen > 0) && (firstLine || (y + maxDescent) < aRect.YMost())) {
    
    PRUint32  maxFit;  
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
    firstLine = PR_FALSE;
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

    PRBool iconUsed = PR_FALSE;

    
    
    
    if (aRequest && !mDisplayingIcon) {
      gIconLoad->AddIconObserver(this);
      mDisplayingIcon = PR_TRUE;
    }


    
    PRUint32 imageStatus = 0;
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
      iconUsed = PR_TRUE;
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

    
    
    PRInt32 iconWidth = nsPresContext::CSSPixelsToAppUnits(ICON_SIZE + ICON_PADDING);
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

static void PaintAltFeedback(nsIFrame* aFrame, nsRenderingContext* aCtx,
     const nsRect& aDirtyRect, nsPoint aPt)
{
  nsImageFrame* f = static_cast<nsImageFrame*>(aFrame);
  nsEventStates state = f->GetContent()->AsElement()->State();
  f->DisplayAltFeedback(*aCtx,
                        aDirtyRect,
                        IMAGE_OK(state, PR_TRUE)
                           ? nsImageFrame::gIconLoad->mLoadingImage
                           : nsImageFrame::gIconLoad->mBrokenImage,
                        aPt);
}

#ifdef NS_DEBUG
static void PaintDebugImageMap(nsIFrame* aFrame, nsRenderingContext* aCtx,
     const nsRect& aDirtyRect, nsPoint aPt) {
  nsImageFrame* f = static_cast<nsImageFrame*>(aFrame);
  nsRect inner = f->GetInnerArea() + aPt;
  nsPresContext* pc = f->PresContext();

  aCtx->SetColor(NS_RGB(0, 0, 0));
  aCtx->PushState();
  aCtx->Translate(inner.TopLeft());
  f->GetImageMap(pc)->Draw(aFrame, *aCtx);
  aCtx->PopState();
}
#endif

void
nsDisplayImage::Paint(nsDisplayListBuilder* aBuilder,
                      nsRenderingContext* aCtx) {
  static_cast<nsImageFrame*>(mFrame)->
    PaintImage(*aCtx, ToReferenceFrame(), mVisibleRect, mImage,
               aBuilder->ShouldSyncDecodeImages()
                 ? (PRUint32) imgIContainer::FLAG_SYNC_DECODE
                 : (PRUint32) imgIContainer::FLAG_NONE);
}

nsCOMPtr<imgIContainer>
nsDisplayImage::GetImage()
{
  return mImage;
}

nsRefPtr<ImageContainer>
nsDisplayImage::GetContainer(LayerManager* aManager)
{
  return static_cast<nsImageFrame*>(mFrame)->GetContainer(aManager, mImage);
}

void
nsDisplayImage::ConfigureLayer(ImageLayer* aLayer)
{
  aLayer->SetFilter(nsLayoutUtils::GetGraphicsFilterForFrame(mFrame));
  
  PRInt32 factor = nsPresContext::AppUnitsPerCSSPixel();
  nsImageFrame* imageFrame = static_cast<nsImageFrame*>(mFrame);

  nsRect dest = imageFrame->GetInnerArea() + ToReferenceFrame();
  gfxRect destRect(dest.x, dest.y, dest.width, dest.height);
  destRect.ScaleInverse(factor); 

  PRInt32 imageWidth;
  PRInt32 imageHeight;
  mImage->GetWidth(&imageWidth);
  mImage->GetHeight(&imageHeight);

  gfxMatrix transform;
  transform.Translate(destRect.TopLeft());
  transform.Scale(destRect.Width()/imageWidth,
                  destRect.Height()/imageHeight);
  aLayer->SetTransform(gfx3DMatrix::From2D(transform));

  aLayer->SetVisibleRegion(nsIntRect(0, 0, imageWidth, imageHeight));
}

nsRefPtr<ImageContainer>
nsImageFrame::GetContainer(LayerManager* aManager, imgIContainer* aImage)
{
  if (mImageContainer && mImageContainer->Manager() == aManager) {
    return mImageContainer;
  }

  if (aImage->GetType() != imgIContainer::TYPE_RASTER) {
    return nsnull;
  }
  
  CairoImage::Data cairoData;
  nsRefPtr<gfxASurface> imageSurface;
  aImage->GetFrame(imgIContainer::FRAME_CURRENT,
                   imgIContainer::FLAG_SYNC_DECODE,
                   getter_AddRefs(imageSurface));
  cairoData.mSurface = imageSurface;
  aImage->GetWidth(&cairoData.mSize.width);
  aImage->GetHeight(&cairoData.mSize.height);

  mImageContainer = aManager->CreateImageContainer();
  NS_ASSERTION(mImageContainer, "Failed to create ImageContainer!");
  
  
  Image::Format cairoFormat = Image::CAIRO_SURFACE;
  nsRefPtr<Image> image = mImageContainer->CreateImage(&cairoFormat, 1);
  NS_ASSERTION(image, "Failed to create Image");

  NS_ASSERTION(image->GetFormat() == cairoFormat, "Wrong format");
  static_cast<CairoImage*>(image.get())->SetData(cairoData);
  mImageContainer->SetCurrentImage(image);

  return mImageContainer;
}

void
nsImageFrame::PaintImage(nsRenderingContext& aRenderingContext, nsPoint aPt,
                         const nsRect& aDirtyRect, imgIContainer* aImage,
                         PRUint32 aFlags)
{
  
  
  NS_ASSERTION(GetInnerArea().width == mComputedSize.width, "bad width");
  nsRect inner = GetInnerArea() + aPt;
  nsRect dest(inner.TopLeft(), mComputedSize);
  dest.y -= GetContinuationOffset();

  nsLayoutUtils::DrawSingleImage(&aRenderingContext, aImage,
    nsLayoutUtils::GetGraphicsFilterForFrame(this), dest, aDirtyRect,
    aFlags);

  nsPresContext* presContext = PresContext();
  nsImageMap* map = GetImageMap(presContext);
  if (nsnull != map) {
    aRenderingContext.PushState();
    aRenderingContext.SetColor(NS_RGB(0, 0, 0));
    aRenderingContext.SetLineStyle(nsLineStyle_kDotted);
    aRenderingContext.Translate(inner.TopLeft());
    map->Draw(this, aRenderingContext);
    aRenderingContext.PopState();
  }
}

NS_IMETHODIMP
nsImageFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                               const nsRect&           aDirtyRect,
                               const nsDisplayListSet& aLists)
{
  if (!IsVisibleForPainting(aBuilder))
    return NS_OK;

  
  
  
  
  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  

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
    PRBool imageOK = IMAGE_OK(contentState, PR_TRUE);

    nsCOMPtr<imgIContainer> imgCon;
    if (currentRequest) {
      currentRequest->GetImage(getter_AddRefs(imgCon));
    }

    
    PRBool haveSize = PR_FALSE;
    PRUint32 imageStatus = 0;
    if (currentRequest)
      currentRequest->GetImageStatus(&imageStatus);
    if (imageStatus & imgIRequest::STATUS_SIZE_AVAILABLE)
      haveSize = PR_TRUE;

    
    NS_ABORT_IF_FALSE(!haveSize || imgCon, "Have size but not container?");

    if (!imageOK || !haveSize) {
      
      
      rv = replacedContent.AppendNewToTop(new (aBuilder)
          nsDisplayGeneric(aBuilder, this, PaintAltFeedback, "AltFeedback",
                           nsDisplayItem::TYPE_ALT_FEEDBACK));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = replacedContent.AppendNewToTop(new (aBuilder)
          nsDisplayImage(aBuilder, this, imgCon));
      NS_ENSURE_SUCCESS(rv, rv);

      
      if (mDisplayingIcon) {
        gIconLoad->RemoveIconObserver(this);
        mDisplayingIcon = PR_FALSE;
      }

        
#ifdef DEBUG
      if (GetShowFrameBorders() && GetImageMap(PresContext())) {
        rv = aLists.Outlines()->AppendNewToTop(new (aBuilder)
            nsDisplayGeneric(aBuilder, this, PaintDebugImageMap, "DebugImageMap",
                             nsDisplayItem::TYPE_DEBUG_IMAGE_MAP));
        NS_ENSURE_SUCCESS(rv, rv);
      }
#endif
    }
  }

  if (ShouldDisplaySelection()) {
    rv = DisplaySelectionOverlay(aBuilder, &replacedContent,
                                 nsISelectionDisplay::DISPLAY_IMAGES);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  WrapReplacedContentForBorderRadius(aBuilder, &replacedContent, aLists);

  return NS_OK;
}

bool
nsImageFrame::ShouldDisplaySelection()
{
  
  nsresult result;
  nsPresContext* presContext = PresContext();
  PRInt16 displaySelection = presContext->PresShell()->GetSelectionFlags();
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
        PRInt32 rangeCount;
        selection->GetRangeCount(&rangeCount);
        if (rangeCount == 1) 
        {
          nsCOMPtr<nsIContent> parentContent = mContent->GetParent();
          if (parentContent)
          {
            PRInt32 thisOffset = parentContent->IndexOf(mContent);
            nsCOMPtr<nsIDOMNode> parentNode = do_QueryInterface(parentContent);
            nsCOMPtr<nsIDOMNode> rangeNode;
            PRInt32 rangeOffset;
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
nsImageFrame::GetImageMap(nsPresContext* aPresContext)
{
  if (!mImageMap) {
    nsIDocument* doc = mContent->GetDocument();
    if (!doc) {
      return nsnull;
    }

    nsAutoString usemap;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::usemap, usemap);

    nsCOMPtr<nsIContent> map = doc->FindImageMap(usemap);
    if (map) {
      mImageMap = new nsImageMap();
      NS_ADDREF(mImageMap);
      mImageMap->Init(aPresContext->PresShell(), this, map);
    }
  }

  return mImageMap;
}

PRBool
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

PRBool
nsImageFrame::GetAnchorHREFTargetAndNode(nsIURI** aHref, nsString& aTarget,
                                         nsIContent** aNode)
{
  PRBool status = PR_FALSE;
  aTarget.Truncate();
  *aHref = nsnull;
  *aNode = nsnull;

  
  for (nsIContent* content = mContent->GetParent();
       content; content = content->GetParent()) {
    nsCOMPtr<nsILink> link(do_QueryInterface(content));
    if (link) {
      nsCOMPtr<nsIURI> href = content->GetHrefURI();
      if (href) {
        href->Clone(aHref);
      }
      status = (*aHref != nsnull);

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
nsImageFrame::GetContentForEvent(nsPresContext* aPresContext,
                                 nsEvent* aEvent,
                                 nsIContent** aContent)
{
  NS_ENSURE_ARG_POINTER(aContent);

  nsIFrame* f = nsLayoutUtils::GetNonGeneratedAncestor(this);
  if (f != this) {
    return f->GetContentForEvent(aPresContext, aEvent, aContent);
  }

  
  
  nsIContent* capturingContent =
    NS_IS_MOUSE_EVENT(aEvent) ? nsIPresShell::GetCapturingContent() : nsnull;
  if (capturingContent && capturingContent->GetPrimaryFrame() == this) {
    *aContent = capturingContent;
    NS_IF_ADDREF(*aContent);
    return NS_OK;
  }

  nsImageMap* map;
  map = GetImageMap(aPresContext);

  if (nsnull != map) {
    nsIntPoint p;
    TranslateEventCoords(
      nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this), p);
    PRBool inside = PR_FALSE;
    nsCOMPtr<nsIContent> area;
    inside = map->IsInside(p.x, p.y, getter_AddRefs(area));
    if (inside && area) {
      *aContent = area;
      NS_ADDREF(*aContent);
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
  nsImageMap* map;

  if ((aEvent->eventStructType == NS_MOUSE_EVENT &&
       aEvent->message == NS_MOUSE_BUTTON_UP && 
       static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton) ||
      aEvent->message == NS_MOUSE_MOVE) {
    map = GetImageMap(aPresContext);
    PRBool isServerMap = IsServerImageMap();
    if ((nsnull != map) || isServerMap) {
      nsIntPoint p;
      TranslateEventCoords(
        nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this), p);
      PRBool inside = PR_FALSE;
      
      
      
      
      if (nsnull != map) {
        nsCOMPtr<nsIContent> area;
        inside = map->IsInside(p.x, p.y, getter_AddRefs(area));
      }

      if (!inside && isServerMap) {

        
        
        nsCOMPtr<nsIURI> uri;
        nsAutoString target;
        nsCOMPtr<nsIContent> anchorNode;
        if (GetAnchorHREFTargetAndNode(getter_AddRefs(uri), target,
                                       getter_AddRefs(anchorNode))) {
          
          
          
          
          
          if (p.x < 0) p.x = 0;
          if (p.y < 0) p.y = 0;
          nsCAutoString spec;
          uri->GetSpec(spec);
          spec += nsPrintfCString("?%d,%d", p.x, p.y);
          uri->SetSpec(spec);                
          
          PRBool clicked = PR_FALSE;
          if (aEvent->message == NS_MOUSE_BUTTON_UP) {
            *aEventStatus = nsEventStatus_eConsumeDoDefault; 
            clicked = PR_TRUE;
          }
          nsContentUtils::TriggerLink(anchorNode, aPresContext, uri, target,
                                      clicked, PR_TRUE, PR_TRUE);
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
  nsPresContext* context = PresContext();
  nsImageMap* map = GetImageMap(context);
  if (nsnull != map) {
    nsIntPoint p;
    TranslateEventCoords(aPoint, p);
    nsCOMPtr<nsIContent> area;
    if (map->IsInside(p.x, p.y, getter_AddRefs(area))) {
      
      
      
      
      
      nsRefPtr<nsStyleContext> areaStyle = 
        PresContext()->PresShell()->StyleSet()->
          ResolveStyleFor(area->AsElement(), GetStyleContext());
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
nsImageFrame::AttributeChanged(PRInt32 aNameSpaceID,
                               nsIAtom* aAttribute,
                               PRInt32 aModType)
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
nsImageFrame::List(FILE* out, PRInt32 aIndent) const
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
    fprintf(out, " [state=%016llx]", mState);
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
      nsCAutoString uristr;
      uri->GetAsciiSpec(uristr);
      fprintf(out, " [src=%s]", uristr.get());
    }
  }
  fputs("\n", out);
  return NS_OK;
}
#endif

PRIntn
nsImageFrame::GetSkipSides() const
{
  PRIntn skip = 0;
  if (nsnull != GetPrevInFlow()) {
    skip |= 1 << NS_SIDE_TOP;
  }
  if (nsnull != GetNextInFlow()) {
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
                       imgIRequest** aRequest)
{
  nsresult rv = NS_OK;
  NS_PRECONDITION(!aSpec.IsEmpty(), "What happened??");

  if (!sIOService) {
    rv = CallGetService(NS_IOSERVICE_CONTRACTID, &sIOService);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIURI> realURI;
  SpecToURI(aSpec, sIOService, getter_AddRefs(realURI));
 
  nsCOMPtr<imgILoader> il(do_GetService("@mozilla.org/image/loader;1", &rv));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsILoadGroup> loadGroup;
  GetLoadGroup(aPresContext, getter_AddRefs(loadGroup));

  
  nsLoadFlags loadFlags = nsIRequest::LOAD_NORMAL;

  return il->LoadImage(realURI,     
                       nsnull,      


                       nsnull,      
                       loadGroup,
                       gIconLoad,
                       nsnull,      
                       loadFlags,
                       nsnull,
                       nsnull,
                       nsnull,      
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
  nsCAutoString charset;
  GetDocumentCharacterSet(charset);
  NS_NewURI(aURI, aSpec, 
            charset.IsEmpty() ? nsnull : charset.get(), 
            baseURI, aIOService);
}

void
nsImageFrame::GetLoadGroup(nsPresContext *aPresContext, nsILoadGroup **aLoadGroup)
{
  if (!aPresContext)
    return;

  NS_PRECONDITION(nsnull != aLoadGroup, "null OUT parameter pointer");

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
                   imgIDecoderObserver)

static const char* kIconLoadPrefs[] = {
  "browser.display.force_inline_alttext",
  "browser.display.show_image_placeholders",
  nsnull
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
    mLoadingImage = nsnull;
  }
  if (mBrokenImage) {
    mBrokenImage->CancelAndForgetObserver(NS_ERROR_FAILURE);
    mBrokenImage = nsnull;
  }
}

NS_IMETHODIMP
nsImageFrame::IconLoad::Observe(nsISupports *aSubject, const char* aTopic,
                                const PRUnichar* aData)
{
  NS_ASSERTION(!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID),
               "wrong topic");
#ifdef DEBUG
  
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(kIconLoadPrefs) ||
                       (NS_NOTREACHED("wrong pref"), PR_FALSE); ++i)
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
    Preferences::GetBool("browser.display.show_image_placeholders", PR_TRUE);
}



NS_IMETHODIMP
nsImageFrame::IconLoad::OnStartRequest(imgIRequest *aRequest)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageFrame::IconLoad::OnStartDecode(imgIRequest *aRequest)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageFrame::IconLoad::OnStartContainer(imgIRequest *aRequest,
                                         imgIContainer *aContainer)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageFrame::IconLoad::OnStartFrame(imgIRequest *aRequest,
                                     PRUint32 aFrame)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageFrame::IconLoad::OnDataAvailable(imgIRequest *aRequest,
                                        PRBool aCurrentFrame,
                                        const nsIntRect * aRect)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageFrame::IconLoad::OnStopFrame(imgIRequest *aRequest,
                                    PRUint32 aFrame)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageFrame::IconLoad::OnStopContainer(imgIRequest *aRequest,
                                        imgIContainer *aContainer)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageFrame::IconLoad::OnStopDecode(imgIRequest *aRequest,
                                     nsresult status,
                                     const PRUnichar *statusArg)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageFrame::IconLoad::OnStopRequest(imgIRequest *aRequest,
                                      PRBool aIsLastPart)
{
  nsTObserverArray<nsImageFrame*>::ForwardIterator iter(mIconObservers);
  nsImageFrame *frame;
  while (iter.HasMore()) {
    frame = iter.GetNext();
    frame->Invalidate(frame->GetRect());
  }

  return NS_OK;
}

NS_IMETHODIMP
nsImageFrame::IconLoad::OnDiscard(imgIRequest *aRequest)
{
  return NS_OK;
}

NS_IMETHODIMP
nsImageFrame::IconLoad::FrameChanged(imgIContainer *aContainer,
                                     const nsIntRect *aDirtyRect)
{
  nsTObserverArray<nsImageFrame*>::ForwardIterator iter(mIconObservers);
  nsImageFrame *frame;
  while (iter.HasMore()) {
    frame = iter.GetNext();
    frame->Invalidate(frame->GetRect());
  }

  return NS_OK;
}



NS_IMPL_ISUPPORTS2(nsImageListener, imgIDecoderObserver, imgIContainerObserver)

nsImageListener::nsImageListener(nsImageFrame *aFrame) :
  mFrame(aFrame)
{
}

nsImageListener::~nsImageListener()
{
}

NS_IMETHODIMP nsImageListener::OnStartContainer(imgIRequest *aRequest,
                                                imgIContainer *aImage)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->OnStartContainer(aRequest, aImage);
}

NS_IMETHODIMP nsImageListener::OnDataAvailable(imgIRequest *aRequest,
                                               PRBool aCurrentFrame,
                                               const nsIntRect *aRect)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->OnDataAvailable(aRequest, aCurrentFrame, aRect);
}

NS_IMETHODIMP nsImageListener::OnStopDecode(imgIRequest *aRequest,
                                            nsresult status,
                                            const PRUnichar *statusArg)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->OnStopDecode(aRequest, status, statusArg);
}

NS_IMETHODIMP nsImageListener::FrameChanged(imgIContainer *aContainer,
                                            const nsIntRect *aDirtyRect)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->FrameChanged(aContainer, aDirtyRect);
}

static PRBool
IsInAutoWidthTableCellForQuirk(nsIFrame *aFrame)
{
  if (eCompatibility_NavQuirks != aFrame->PresContext()->CompatibilityMode())
    return PR_FALSE;
  
  nsBlockFrame *ancestor = nsLayoutUtils::FindNearestBlockAncestor(aFrame);
  if (ancestor->GetStyleContext()->GetPseudo() == nsCSSAnonBoxes::cellContent) {
    
    nsFrame *grandAncestor = static_cast<nsFrame*>(ancestor->GetParent());
    return grandAncestor &&
      grandAncestor->GetStylePosition()->mWidth.GetUnit() == eStyleUnit_Auto;
  }
  return PR_FALSE;
}

 void
nsImageFrame::AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                                nsIFrame::InlineMinWidthData *aData)
{

  NS_ASSERTION(GetParent(), "Must have a parent if we get here!");
  
  PRBool canBreak =
    !CanContinueTextRun() &&
    GetParent()->GetStyleText()->WhiteSpaceCanWrap() &&
    !IsInAutoWidthTableCellForQuirk(this);

  if (canBreak)
    aData->OptionallyBreak(aRenderingContext);
 
  aData->trailingWhitespace = 0;
  aData->skipWhitespace = PR_FALSE;
  aData->trailingTextFrame = nsnull;
  aData->currentLine += nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                            this, nsLayoutUtils::MIN_WIDTH);
  aData->atStartOfLine = PR_FALSE;

  if (canBreak)
    aData->OptionallyBreak(aRenderingContext);

}
