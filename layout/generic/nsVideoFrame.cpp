







#include "nsHTMLParts.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsGkAtoms.h"

#include "nsVideoFrame.h"
#include "nsHTMLVideoElement.h"
#include "nsIDOMHTMLVideoElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsDisplayList.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "nsPresContext.h"
#include "nsTransform2D.h"
#include "nsContentCreatorFunctions.h"
#include "nsBoxLayoutState.h"
#include "nsBoxFrame.h"
#include "nsImageFrame.h"
#include "nsIImageLoadingContent.h"
#include "nsCSSRendering.h"
#include "nsContentUtils.h"
#include "mozilla/layers/ShadowLayers.h"
#include "ImageContainer.h"
#include "ImageLayers.h"
#include "nsContentList.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::dom;

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
nsVideoFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  nsNodeInfoManager *nodeInfoManager = GetContent()->GetCurrentDoc()->NodeInfoManager();
  nsCOMPtr<nsINodeInfo> nodeInfo;
  if (HasVideoElement()) {
    
    
    
    nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::img,
                                            nullptr,
                                            kNameSpaceID_XHTML,
                                            nsIDOMNode::ELEMENT_NODE);
    NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
    Element* element = NS_NewHTMLImageElement(nodeInfo.forget());
    mPosterImage = element;
    NS_ENSURE_TRUE(mPosterImage, NS_ERROR_OUT_OF_MEMORY);

    
    
    
    nsCxPusher pusher;
    pusher.PushNull();

    
    
    
    
    nsCOMPtr<nsIImageLoadingContent> imgContent = do_QueryInterface(mPosterImage);
    NS_ENSURE_TRUE(imgContent, NS_ERROR_FAILURE);

    imgContent->ForceImageState(true, 0);
    
    element->UpdateState(false);

    nsresult res = UpdatePosterSource(false);
    NS_ENSURE_SUCCESS(res,res);

    if (!aElements.AppendElement(mPosterImage))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::videocontrols,
                                          nullptr,
                                          kNameSpaceID_XUL,
                                          nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  NS_TrustedNewXULElement(getter_AddRefs(mVideoControls), nodeInfo.forget());
  if (!aElements.AppendElement(mVideoControls))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

void
nsVideoFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                       uint32_t aFliter)
{
  aElements.MaybeAppendElement(mPosterImage);
  aElements.MaybeAppendElement(mVideoControls);
}

void
nsVideoFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsContentUtils::DestroyAnonymousContent(&mVideoControls);
  nsContentUtils::DestroyAnonymousContent(&mPosterImage);
  nsContainerFrame::DestroyFrom(aDestructRoot);
}

bool
nsVideoFrame::IsLeaf() const
{
  return true;
}



static gfxRect
CorrectForAspectRatio(const gfxRect& aRect, const nsIntSize& aRatio)
{
  NS_ASSERTION(aRatio.width > 0 && aRatio.height > 0 && !aRect.IsEmpty(),
               "Nothing to draw");
  
  gfxFloat scale =
    std::min(aRect.Width()/aRatio.width, aRect.Height()/aRatio.height);
  gfxSize scaledRatio(scale*aRatio.width, scale*aRatio.height);
  gfxPoint topLeft((aRect.Width() - scaledRatio.width)/2,
                   (aRect.Height() - scaledRatio.height)/2);
  return gfxRect(aRect.TopLeft() + topLeft, scaledRatio);
}

already_AddRefed<Layer>
nsVideoFrame::BuildLayer(nsDisplayListBuilder* aBuilder,
                         LayerManager* aManager,
                         nsDisplayItem* aItem,
                         const ContainerParameters& aContainerParameters)
{
  nsRect area = GetContentRect() - GetPosition() + aItem->ToReferenceFrame();
  nsHTMLVideoElement* element = static_cast<nsHTMLVideoElement*>(GetContent());
  nsIntSize videoSize;
  if (NS_FAILED(element->GetVideoSize(&videoSize)) || area.IsEmpty()) {
    return nullptr;
  }

  nsRefPtr<ImageContainer> container = element->GetImageContainer();
  if (!container)
    return nullptr;
  
  
  
  gfxIntSize frameSize = container->GetCurrentSize();
  if (frameSize.width == 0 || frameSize.height == 0) {
    
    return nullptr;
  }

  
  
  
  nsPresContext* presContext = PresContext();
  gfxRect r = gfxRect(presContext->AppUnitsToGfxUnits(area.x),
                      presContext->AppUnitsToGfxUnits(area.y),
                      presContext->AppUnitsToGfxUnits(area.width),
                      presContext->AppUnitsToGfxUnits(area.height));
  r = CorrectForAspectRatio(r, videoSize);
  r.Round();
  if (r.IsEmpty()) {
    return nullptr;
  }
  gfxIntSize scaleHint(static_cast<int32_t>(r.Width()),
                       static_cast<int32_t>(r.Height()));
  container->SetScaleHint(scaleHint);

  nsRefPtr<ImageLayer> layer = static_cast<ImageLayer*>
    (aManager->GetLayerBuilder()->GetLeafLayerFor(aBuilder, aItem));
  if (!layer) {
    layer = aManager->CreateImageLayer();
    if (!layer)
      return nullptr;
  }

  layer->SetContainer(container);
  layer->SetFilter(nsLayoutUtils::GetGraphicsFilterForFrame(this));
  layer->SetContentFlags(Layer::CONTENT_OPAQUE);
  
  gfxMatrix transform;
  transform.Translate(r.TopLeft() + aContainerParameters.mOffset);
  transform.Scale(r.Width()/frameSize.width, r.Height()/frameSize.height);
  layer->SetBaseTransform(gfx3DMatrix::From2D(transform));
  layer->SetVisibleRegion(nsIntRect(0, 0, frameSize.width, frameSize.height));
  nsRefPtr<Layer> result = layer.forget();
  return result.forget();
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

      uint32_t posterHeight, posterWidth;
      nsSize scaledPosterSize(0, 0);
      nsSize computedArea(aReflowState.ComputedWidth(), aReflowState.ComputedHeight());
      nsPoint posterTopLeft(0, 0);

      nsCOMPtr<nsIDOMHTMLImageElement> posterImage = do_QueryInterface(mPosterImage);
      NS_ENSURE_TRUE(posterImage, NS_ERROR_FAILURE);
      posterImage->GetNaturalHeight(&posterHeight);
      posterImage->GetNaturalWidth(&posterWidth);

      if (ShouldDisplayPoster() && posterHeight && posterWidth) {
        gfxFloat scale =
          std::min(static_cast<float>(computedArea.width)/nsPresContext::CSSPixelsToAppUnits(static_cast<float>(posterWidth)),
                 static_cast<float>(computedArea.height)/nsPresContext::CSSPixelsToAppUnits(static_cast<float>(posterHeight)));
        gfxSize scaledRatio = gfxSize(scale*posterWidth, scale*posterHeight);
        scaledPosterSize.width = nsPresContext::CSSPixelsToAppUnits(static_cast<float>(scaledRatio.width));
        scaledPosterSize.height = nsPresContext::CSSPixelsToAppUnits(static_cast<int32_t>(scaledRatio.height));
      }
      kidReflowState.SetComputedWidth(scaledPosterSize.width);
      kidReflowState.SetComputedHeight(scaledPosterSize.height);
      posterTopLeft.x = ((computedArea.width - scaledPosterSize.width) / 2) + mBorderPadding.left;
      posterTopLeft.y = ((computedArea.height - scaledPosterSize.height) / 2) + mBorderPadding.top;

      ReflowChild(imageFrame, aPresContext, kidDesiredSize, kidReflowState,
                        posterTopLeft.x, posterTopLeft.y, 0, aStatus);
      FinishReflowChild(imageFrame, aPresContext, &kidReflowState, kidDesiredSize,
                        posterTopLeft.x, posterTopLeft.y, 0);
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
  aMetrics.SetOverflowAreasToDesiredBounds();

  FinishAndStoreOverflow(&aMetrics);

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                  ("exit nsVideoFrame::Reflow: size=%d,%d",
                  aMetrics.width, aMetrics.height));
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);

  return NS_OK;
}

class nsDisplayVideo : public nsDisplayItem {
public:
  nsDisplayVideo(nsDisplayListBuilder* aBuilder, nsVideoFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame)
  {
    MOZ_COUNT_CTOR(nsDisplayVideo);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayVideo() {
    MOZ_COUNT_DTOR(nsDisplayVideo);
  }
#endif
  
  NS_DISPLAY_DECL_NAME("Video", TYPE_VIDEO)

  
  
  
  
  
  
  

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
  {
    *aSnap = true;
    nsIFrame* f = GetUnderlyingFrame();
    return f->GetContentRect() - f->GetPosition() + ToReferenceFrame();
  }

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters)
  {
    return static_cast<nsVideoFrame*>(mFrame)->BuildLayer(aBuilder, aManager, this, aContainerParameters);
  }

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const FrameLayerBuilder::ContainerParameters& aParameters)
  {
    if (aManager->IsCompositingCheap()) {
      
      
      
      
      
      
      return LAYER_ACTIVE;
    }
    nsHTMLMediaElement* elem =
      static_cast<nsHTMLMediaElement*>(mFrame->GetContent());
    return elem->IsPotentiallyPlaying() ? LAYER_ACTIVE_FORCE : LAYER_INACTIVE;
  }
};

void
nsVideoFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                               const nsRect&           aDirtyRect,
                               const nsDisplayListSet& aLists)
{
  if (!IsVisibleForPainting(aBuilder))
    return;

  DO_GLOBAL_REFLOW_COUNT_DSP("nsVideoFrame");

  DisplayBorderBackgroundOutline(aBuilder, aLists);

  nsDisplayList replacedContent;

  if (HasVideoElement() && !ShouldDisplayPoster()) {
    replacedContent.AppendNewToTop(
      new (aBuilder) nsDisplayVideo(aBuilder, this));
  }

  
  
  for (nsIFrame *child = mFrames.FirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->GetType() == nsGkAtoms::imageFrame && ShouldDisplayPoster()) {
      child->BuildDisplayListForStackingContext(aBuilder,
                                                aDirtyRect - child->GetOffsetTo(this),
                                                &replacedContent);
    } else if (child->GetType() == nsGkAtoms::boxFrame) {
      child->BuildDisplayListForStackingContext(aBuilder,
                                                aDirtyRect - child->GetOffsetTo(this),
                                                &replacedContent);
    }
  }

  WrapReplacedContentForBorderRadius(aBuilder, &replacedContent, aLists);
}

nsIAtom*
nsVideoFrame::GetType() const
{
  return nsGkAtoms::HTMLVideoFrame;
}

#ifdef ACCESSIBILITY
a11y::AccType
nsVideoFrame::AccessibleType()
{
  return a11y::eHTMLMediaType;
}
#endif

#ifdef DEBUG
NS_IMETHODIMP
nsVideoFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("HTMLVideo"), aResult);
}
#endif

nsSize nsVideoFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                                     nsSize aCBSize,
                                     nscoord aAvailableWidth,
                                     nsSize aMargin,
                                     nsSize aBorder,
                                     nsSize aPadding,
                                     uint32_t aFlags)
{
  nsSize size = GetVideoIntrinsicSize(aRenderingContext);

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

nscoord nsVideoFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result = GetVideoIntrinsicSize(aRenderingContext).width;
  DISPLAY_MIN_WIDTH(this, result);
  return result;
}

nscoord nsVideoFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result = GetVideoIntrinsicSize(aRenderingContext).width;
  DISPLAY_PREF_WIDTH(this, result);
  return result;
}

nsSize nsVideoFrame::GetIntrinsicRatio()
{
  return GetVideoIntrinsicSize(nullptr);
}

bool nsVideoFrame::ShouldDisplayPoster()
{
  if (!HasVideoElement())
    return false;

  nsHTMLVideoElement* element = static_cast<nsHTMLVideoElement*>(GetContent());
  if (element->GetPlayedOrSeeked() && HasVideoData())
    return false;

  nsCOMPtr<nsIImageLoadingContent> imgContent = do_QueryInterface(mPosterImage);
  NS_ENSURE_TRUE(imgContent, false);

  nsCOMPtr<imgIRequest> request;
  nsresult res = imgContent->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                                        getter_AddRefs(request));
  if (NS_FAILED(res) || !request) {
    return false;
  }

  uint32_t status = 0;
  res = request->GetImageStatus(&status);
  if (NS_FAILED(res) || (status & imgIRequest::STATUS_ERROR))
    return false;

  return true;
}

nsSize
nsVideoFrame::GetVideoIntrinsicSize(nsRenderingContext *aRenderingContext)
{
  
  nsIntSize size(300, 150);
  
  if (!HasVideoElement()) {
    if (!aRenderingContext || !mFrames.FirstChild()) {
      
      
      
      return nsSize(0, 0);
    }

    
    nsBoxLayoutState boxState(PresContext(), aRenderingContext, 0);
    nscoord prefHeight = mFrames.LastChild()->GetPrefSize(boxState).height;
    return nsSize(nsPresContext::CSSPixelsToAppUnits(size.width), prefHeight);
  }

  nsHTMLVideoElement* element = static_cast<nsHTMLVideoElement*>(GetContent());
  if (NS_FAILED(element->GetVideoSize(&size)) && ShouldDisplayPoster()) {
    
    nsIFrame *child = mPosterImage->GetPrimaryFrame();
    nsImageFrame* imageFrame = do_QueryFrame(child);
    nsSize imgsize;
    if (NS_SUCCEEDED(imageFrame->GetIntrinsicImageSize(imgsize))) {
      return imgsize;
    }
  }

  return nsSize(nsPresContext::CSSPixelsToAppUnits(size.width),
                nsPresContext::CSSPixelsToAppUnits(size.height));
}

nsresult
nsVideoFrame::UpdatePosterSource(bool aNotify)
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
nsVideoFrame::AttributeChanged(int32_t aNameSpaceID,
                               nsIAtom* aAttribute,
                               int32_t aModType)
{
  if (aAttribute == nsGkAtoms::poster && HasVideoElement()) {
    nsresult res = UpdatePosterSource(true);
    NS_ENSURE_SUCCESS(res,res);
  }
  return nsContainerFrame::AttributeChanged(aNameSpaceID,
                                            aAttribute,
                                            aModType);
}

bool nsVideoFrame::HasVideoElement() {
  nsCOMPtr<nsIDOMHTMLVideoElement> videoDomElement = do_QueryInterface(mContent);
  return videoDomElement != nullptr;
}

bool nsVideoFrame::HasVideoData()
{
  if (!HasVideoElement())
    return false;
  nsHTMLVideoElement* element = static_cast<nsHTMLVideoElement*>(GetContent());
  nsIntSize size(0, 0);
  element->GetVideoSize(&size);
  return size != nsIntSize(0,0);
}
