







































#include "nsHTMLParts.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsGkAtoms.h"

#include "nsVideoFrame.h"
#include "nsHTMLVideoElement.h"
#include "nsIDOMHTMLVideoElement.h"
#include "nsDisplayList.h"
#include "nsIRenderingContext.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "nsPresContext.h"
#include "nsTransform2D.h"
#include "nsContentCreatorFunctions.h"
#include "nsBoxLayoutState.h"
#include "nsBoxFrame.h"
#include "nsImageFrame.h"
#include "nsIImageLoadingContent.h"

#ifdef ACCESSIBILITY
#include "nsIServiceManager.h"
#include "nsIAccessible.h"
#include "nsIAccessibilityService.h"
#endif

nsIFrame*
NS_NewHTMLVideoFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsVideoFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsVideoFrame)

nsVideoFrame::nsVideoFrame(nsStyleContext* aContext) :
  nsContainerFrame(aContext)
{
}

nsVideoFrame::~nsVideoFrame()
{
}

NS_QUERYFRAME_HEAD(nsVideoFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

nsresult
nsVideoFrame::CreateAnonymousContent(nsTArray<nsIContent*>& aElements)
{
  nsNodeInfoManager *nodeInfoManager = GetContent()->GetCurrentDoc()->NodeInfoManager();
  nsCOMPtr<nsINodeInfo> nodeInfo;
  if (HasVideoElement()) {
    
    
    
    nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::img,
                                            nsnull,
                                            kNameSpaceID_XHTML);
    NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
    mPosterImage = NS_NewHTMLImageElement(nodeInfo);
    NS_ENSURE_TRUE(mPosterImage, NS_ERROR_OUT_OF_MEMORY);
    
    
    
    
    
    nsCOMPtr<nsIImageLoadingContent> imgContent = do_QueryInterface(mPosterImage);
    NS_ENSURE_TRUE(imgContent, NS_ERROR_FAILURE);

    imgContent->ForceImageState(PR_TRUE, 0);    

    nsresult res = UpdatePosterSource(PR_FALSE);
    NS_ENSURE_SUCCESS(res,res);
    
    if (!aElements.AppendElement(mPosterImage))
      return NS_ERROR_OUT_OF_MEMORY;
  } 

  
  
  nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::videocontrols,
                                          nsnull,
                                          kNameSpaceID_XUL);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = NS_NewElement(getter_AddRefs(mVideoControls),
                              kNameSpaceID_XUL,
                              nodeInfo,
                              PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!aElements.AppendElement(mVideoControls))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

void
nsVideoFrame::Destroy()
{
  nsContentUtils::DestroyAnonymousContent(&mVideoControls);
  nsContentUtils::DestroyAnonymousContent(&mPosterImage);
  nsContainerFrame::Destroy();
}

PRBool
nsVideoFrame::IsLeaf() const
{
  return PR_TRUE;
}



static gfxRect
CorrectForAspectRatio(const gfxRect& aRect, const nsIntSize& aRatio)
{
  NS_ASSERTION(aRatio.width > 0 && aRatio.height > 0 && !aRect.IsEmpty(),
               "Nothing to draw");
  
  gfxFloat scale =
    NS_MIN(aRect.Width()/aRatio.width, aRect.Height()/aRatio.height);
  gfxSize scaledRatio(scale*aRatio.width, scale*aRatio.height);
  gfxPoint topLeft((aRect.Width() - scaledRatio.width)/2,
                   (aRect.Height() - scaledRatio.height)/2);
  return gfxRect(aRect.TopLeft() + topLeft, scaledRatio);
}

void
nsVideoFrame::PaintVideo(nsIRenderingContext& aRenderingContext,
                         const nsRect& aDirtyRect, nsPoint aPt) 
{
  nsRect area = GetContentRect() - GetPosition() + aPt;
  nsHTMLVideoElement* element = static_cast<nsHTMLVideoElement*>(GetContent());
  nsIntSize videoSize = element->GetVideoSize(nsIntSize(0, 0));
  if (videoSize.width <= 0 || videoSize.height <= 0 || area.IsEmpty())
    return;

  gfxContext* ctx = static_cast<gfxContext*>(aRenderingContext.GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT));
  nsPresContext* presContext = PresContext();
  gfxRect r = gfxRect(presContext->AppUnitsToGfxUnits(area.x), 
                      presContext->AppUnitsToGfxUnits(area.y), 
                      presContext->AppUnitsToGfxUnits(area.width), 
                      presContext->AppUnitsToGfxUnits(area.height));

  r = CorrectForAspectRatio(r, videoSize);
  element->Paint(ctx, nsLayoutUtils::GetGraphicsFilterForFrame(this), r);
}

NS_IMETHODIMP
nsVideoFrame::Reflow(nsPresContext*           aPresContext,
                     nsHTMLReflowMetrics&     aMetrics,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsVideoFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                  ("enter nsVideoFrame::Reflow: availSize=%d,%d",
                  aReflowState.availableWidth, aReflowState.availableHeight));

  NS_PRECONDITION(mState & NS_FRAME_IN_REFLOW, "frame is not in reflow");

  aStatus = NS_FRAME_COMPLETE;

  aMetrics.width = aReflowState.ComputedWidth();
  aMetrics.height = aReflowState.ComputedHeight();

  
  mBorderPadding   = aReflowState.mComputedBorderPadding;

  aMetrics.width += mBorderPadding.left + mBorderPadding.right;
  aMetrics.height += mBorderPadding.top + mBorderPadding.bottom;

  
  
  for (nsIFrame *child = mFrames.FirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->GetType() == nsGkAtoms::imageFrame) {
      
      nsImageFrame* imageFrame = static_cast<nsImageFrame*>(child);
      nsHTMLReflowMetrics kidDesiredSize;
      nsSize availableSize = nsSize(aReflowState.availableWidth,
                                    aReflowState.availableHeight);
      nsHTMLReflowState kidReflowState(aPresContext,
                                       aReflowState,
                                       imageFrame,
                                       availableSize,
                                       aMetrics.width,
                                       aMetrics.height);
      if (ShouldDisplayPoster()) {
        kidReflowState.SetComputedWidth(aReflowState.ComputedWidth());
        kidReflowState.SetComputedHeight(aReflowState.ComputedHeight());
      } else {
        kidReflowState.SetComputedWidth(0);
        kidReflowState.SetComputedHeight(0);      
      }
      ReflowChild(imageFrame, aPresContext, kidDesiredSize, kidReflowState,
                  mBorderPadding.left, mBorderPadding.top, 0, aStatus);
      FinishReflowChild(imageFrame, aPresContext,
                        &kidReflowState, kidDesiredSize,
                        mBorderPadding.left, mBorderPadding.top, 0);
    } else if (child->GetType() == nsGkAtoms::boxFrame) {
      
      nsBoxLayoutState boxState(PresContext(), aReflowState.rendContext);
      nsBoxFrame::LayoutChildAt(boxState,
                                child,
                                nsRect(mBorderPadding.left,
                                       mBorderPadding.top,
                                       aReflowState.ComputedWidth(),
                                       aReflowState.ComputedHeight()));
    }
  }
  aMetrics.mOverflowArea.SetRect(0, 0, aMetrics.width, aMetrics.height);

  FinishAndStoreOverflow(&aMetrics);

  if (mRect.width != aMetrics.width || mRect.height != aMetrics.height) {
    Invalidate(nsRect(0, 0, mRect.width, mRect.height));
  }

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                  ("exit nsVideoFrame::Reflow: size=%d,%d",
                  aMetrics.width, aMetrics.height));
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);

  return NS_OK;
}

static void PaintVideo(nsIFrame* aFrame, nsIRenderingContext* aCtx,
                        const nsRect& aDirtyRect, nsPoint aPt)
{
#if 0
  double start = double(PR_IntervalToMilliseconds(PR_IntervalNow()))/1000.0;
#endif

  static_cast<nsVideoFrame*>(aFrame)->PaintVideo(*aCtx, aDirtyRect, aPt);
#if 0
  double end = double(PR_IntervalToMilliseconds(PR_IntervalNow()))/1000.0;
  printf("PaintVideo: %f\n", (float)end - (float)start);

#endif
}

NS_IMETHODIMP
nsVideoFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                               const nsRect&           aDirtyRect,
                               const nsDisplayListSet& aLists)
{
  if (!IsVisibleForPainting(aBuilder))
    return NS_OK;

  DO_GLOBAL_REFLOW_COUNT_DSP("nsVideoFrame");

  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ShouldDisplayPoster() && HasVideoData()) {
    rv = aLists.Content()->AppendNewToTop(new (aBuilder) nsDisplayGeneric(this, ::PaintVideo, "Video"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  for (nsIFrame *child = mFrames.FirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->GetType() == nsGkAtoms::imageFrame && ShouldDisplayPoster()) {
      rv = child->BuildDisplayListForStackingContext(aBuilder,
                                                     aDirtyRect - child->GetOffsetTo(this),
                                                     aLists.Content());
      NS_ENSURE_SUCCESS(rv,rv);
    } else if (child->GetType() == nsGkAtoms::boxFrame) {
      rv = child->BuildDisplayListForStackingContext(aBuilder,
                                                     aDirtyRect - child->GetOffsetTo(this),
                                                     aLists.Content());
      NS_ENSURE_SUCCESS(rv,rv);
    }
  }

  return NS_OK;
}

nsIAtom*
nsVideoFrame::GetType() const
{
  return nsGkAtoms::HTMLVideoFrame;
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP
nsVideoFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService =
    do_GetService("@mozilla.org/accessibilityService;1");
  NS_ENSURE_STATE(accService);

  return accService->CreateHTMLMediaAccessible(static_cast<nsIFrame*>(this),
                                               aAccessible);
}
#endif

#ifdef DEBUG
NS_IMETHODIMP
nsVideoFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("HTMLVideo"), aResult);
}
#endif

nsSize nsVideoFrame::ComputeSize(nsIRenderingContext *aRenderingContext,
                                     nsSize aCBSize, 
                                     nscoord aAvailableWidth,
                                     nsSize aMargin, 
                                     nsSize aBorder, 
                                     nsSize aPadding,
                                     PRBool aShrinkWrap)
{
  nsSize size = GetIntrinsicSize(aRenderingContext);

  IntrinsicSize intrinsicSize;
  intrinsicSize.width.SetCoordValue(size.width);
  intrinsicSize.height.SetCoordValue(size.height);

  nsSize& intrinsicRatio = size; 

  return nsLayoutUtils::ComputeSizeWithIntrinsicDimensions(aRenderingContext, 
                                                           this,
                                                           intrinsicSize,
                                                           intrinsicRatio,
                                                           aCBSize, 
                                                           aMargin, 
                                                           aBorder, 
                                                           aPadding);
}

nscoord nsVideoFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result = GetIntrinsicSize(aRenderingContext).width;
  DISPLAY_MIN_WIDTH(this, result);
  return result;
}

nscoord nsVideoFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result = GetIntrinsicSize(aRenderingContext).width;
  DISPLAY_PREF_WIDTH(this, result);
  return result;
}

nsSize nsVideoFrame::GetIntrinsicRatio()
{
  return GetIntrinsicSize(nsnull);
}

PRBool nsVideoFrame::ShouldDisplayPoster()
{
  if (!HasVideoElement())
    return PR_FALSE;

  nsHTMLVideoElement* element = static_cast<nsHTMLVideoElement*>(GetContent());
  if (element->GetPlayedOrSeeked() && HasVideoData())
    return PR_FALSE;

  nsCOMPtr<nsIImageLoadingContent> imgContent = do_QueryInterface(mPosterImage);
  NS_ENSURE_TRUE(imgContent, PR_FALSE);
  
  nsCOMPtr<imgIRequest> request;
  nsresult res = imgContent->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                                        getter_AddRefs(request));
  if (NS_FAILED(res) || !request) {
    return PR_FALSE;
  }

  PRUint32 status = 0;
  res = request->GetImageStatus(&status);
  if (NS_FAILED(res) || (status & imgIRequest::STATUS_ERROR))
    return PR_FALSE;  
  
  return PR_TRUE;
}

nsSize nsVideoFrame::GetIntrinsicSize(nsIRenderingContext *aRenderingContext)
{
  
  nsIntSize size(300,150);

  if (ShouldDisplayPoster()) {
    
    nsIFrame *child = mFrames.FirstChild();
    if (child && child->GetType() == nsGkAtoms::imageFrame) {
      nsImageFrame* imageFrame = static_cast<nsImageFrame*>(child);
      nsSize imgsize;
      imageFrame->GetIntrinsicImageSize(imgsize);
      return imgsize;
    }
  }

  if (!HasVideoData()) {
    if (!aRenderingContext || !mFrames.FirstChild()) {
      
      
      
      return nsSize(0, 0);
    }

    
    nsBoxLayoutState boxState(PresContext(), aRenderingContext, 0);
    nscoord prefHeight = mFrames.LastChild()->GetPrefSize(boxState).height;
    return nsSize(nsPresContext::CSSPixelsToAppUnits(size.width), prefHeight);
  }

  nsHTMLVideoElement* element = static_cast<nsHTMLVideoElement*>(GetContent());
  size = element->GetVideoSize(size);

  return nsSize(nsPresContext::CSSPixelsToAppUnits(size.width), 
                nsPresContext::CSSPixelsToAppUnits(size.height));
}

nsresult
nsVideoFrame::UpdatePosterSource(PRBool aNotify)
{
  NS_ASSERTION(HasVideoElement(), "Only call this on <video> elements.");
  nsHTMLVideoElement* element = static_cast<nsHTMLVideoElement*>(GetContent());

  nsAutoString posterStr;
  element->GetPoster(posterStr);
  nsresult res = mPosterImage->SetAttr(kNameSpaceID_None,
                                       nsGkAtoms::src,
                                       posterStr,
                                       aNotify);
  NS_ENSURE_SUCCESS(res,res);
  return NS_OK;
}

NS_IMETHODIMP
nsVideoFrame::AttributeChanged(PRInt32 aNameSpaceID,
                               nsIAtom* aAttribute,
                               PRInt32 aModType)
{
  if (aAttribute == nsGkAtoms::poster && HasVideoElement()) {
    nsresult res = UpdatePosterSource(PR_TRUE);
    NS_ENSURE_SUCCESS(res,res);
  }
  return nsContainerFrame::AttributeChanged(aNameSpaceID,
                                            aAttribute,
                                            aModType);
}

PRBool nsVideoFrame::HasVideoElement() {
  nsCOMPtr<nsIDOMHTMLVideoElement> videoDomElement = do_QueryInterface(mContent);
  return videoDomElement != nsnull;
}

PRBool nsVideoFrame::HasVideoData()
{
  if (!HasVideoElement())
    return PR_FALSE;
  nsHTMLVideoElement* element = static_cast<nsHTMLVideoElement*>(GetContent());
  nsIntSize size = element->GetVideoSize(nsIntSize(0,0));    
  return size != nsIntSize(0,0);
}
