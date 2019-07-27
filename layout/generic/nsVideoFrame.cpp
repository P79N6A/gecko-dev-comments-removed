







#include "nsVideoFrame.h"

#include "nsCOMPtr.h"
#include "nsGkAtoms.h"

#include "mozilla/dom/HTMLVideoElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsDisplayList.h"
#include "nsGenericHTMLElement.h"
#include "nsPresContext.h"
#include "nsContentCreatorFunctions.h"
#include "nsBoxLayoutState.h"
#include "nsBoxFrame.h"
#include "nsImageFrame.h"
#include "nsIImageLoadingContent.h"
#include "nsContentUtils.h"
#include "ImageContainer.h"
#include "ImageLayers.h"
#include "nsContentList.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::dom;
using namespace mozilla::gfx;

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
  NS_QUERYFRAME_ENTRY(nsVideoFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

nsresult
nsVideoFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  nsNodeInfoManager *nodeInfoManager = GetContent()->GetCurrentDoc()->NodeInfoManager();
  nsRefPtr<NodeInfo> nodeInfo;
  Element *element;

  if (HasVideoElement()) {
    
    
    
    nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::img,
                                            nullptr,
                                            kNameSpaceID_XHTML,
                                            nsIDOMNode::ELEMENT_NODE);
    NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
    element = NS_NewHTMLImageElement(nodeInfo.forget());
    mPosterImage = element;
    NS_ENSURE_TRUE(mPosterImage, NS_ERROR_OUT_OF_MEMORY);

    
    
    
    
    nsCOMPtr<nsIImageLoadingContent> imgContent = do_QueryInterface(mPosterImage);
    NS_ENSURE_TRUE(imgContent, NS_ERROR_FAILURE);

    imgContent->ForceImageState(true, 0);
    
    element->UpdateState(false);

    UpdatePosterSource(false);

    if (!aElements.AppendElement(mPosterImage))
      return NS_ERROR_OUT_OF_MEMORY;

    
    nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::div,
                                            nullptr,
                                            kNameSpaceID_XHTML,
                                            nsIDOMNode::ELEMENT_NODE);
    NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
    mCaptionDiv = NS_NewHTMLDivElement(nodeInfo.forget());
    NS_ENSURE_TRUE(mCaptionDiv, NS_ERROR_OUT_OF_MEMORY);
    nsGenericHTMLElement* div = static_cast<nsGenericHTMLElement*>(mCaptionDiv.get());
    div->SetClassName(NS_LITERAL_STRING("caption-box"));

    if (!aElements.AppendElement(mCaptionDiv))
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
nsVideoFrame::AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                       uint32_t aFliter)
{
  if (mPosterImage) {
    aElements.AppendElement(mPosterImage);
  }

  if (mVideoControls) {
    aElements.AppendElement(mVideoControls);
  }

  if (mCaptionDiv) {
    aElements.AppendElement(mCaptionDiv);
  }
}

void
nsVideoFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsContentUtils::DestroyAnonymousContent(&mCaptionDiv);
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
                         const ContainerLayerParameters& aContainerParameters)
{
  nsRect area = GetContentRect() - GetPosition() + aItem->ToReferenceFrame();
  HTMLVideoElement* element = static_cast<HTMLVideoElement*>(GetContent());
  nsIntSize videoSize;
  if (NS_FAILED(element->GetVideoSize(&videoSize)) || area.IsEmpty()) {
    return nullptr;
  }

  nsRefPtr<ImageContainer> container = element->GetImageContainer();
  if (!container)
    return nullptr;
  
  
  
  mozilla::gfx::IntSize frameSize = container->GetCurrentSize();
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
  IntSize scaleHint(static_cast<int32_t>(r.Width()),
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
  
  gfx::Matrix transform;
  gfxPoint p = r.TopLeft() + aContainerParameters.mOffset;
  transform.Translate(p.x, p.y);
  transform.Scale(r.Width()/frameSize.width, r.Height()/frameSize.height);
  layer->SetBaseTransform(gfx::Matrix4x4::From2D(transform));
  nsRefPtr<Layer> result = layer.forget();
  return result.forget();
}

class DispatchResizeToControls : public nsRunnable
{
public:
  explicit DispatchResizeToControls(nsIContent* aContent)
    : mContent(aContent) {}
  NS_IMETHOD Run() MOZ_OVERRIDE {
    nsContentUtils::DispatchTrustedEvent(mContent->OwnerDoc(), mContent,
                                         NS_LITERAL_STRING("resizevideocontrols"),
                                         false, false);
    return NS_OK;
  }
  nsCOMPtr<nsIContent> mContent;
};

void
nsVideoFrame::Reflow(nsPresContext*           aPresContext,
                     nsHTMLReflowMetrics&     aMetrics,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsVideoFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                  ("enter nsVideoFrame::Reflow: availSize=%d,%d",
                  aReflowState.AvailableWidth(), aReflowState.AvailableHeight()));

  NS_PRECONDITION(mState & NS_FRAME_IN_REFLOW, "frame is not in reflow");

  aStatus = NS_FRAME_COMPLETE;

  aMetrics.Width() = aReflowState.ComputedWidth();
  aMetrics.Height() = aReflowState.ComputedHeight();

  
  mBorderPadding   = aReflowState.ComputedPhysicalBorderPadding();

  aMetrics.Width() += mBorderPadding.left + mBorderPadding.right;
  aMetrics.Height() += mBorderPadding.top + mBorderPadding.bottom;

  
  
  for (nsIFrame *child = mFrames.FirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->GetContent() == mPosterImage) {
      
      nsImageFrame* imageFrame = static_cast<nsImageFrame*>(child);
      nsHTMLReflowMetrics kidDesiredSize(aReflowState);
      WritingMode wm = imageFrame->GetWritingMode();
      LogicalSize availableSize = aReflowState.AvailableSize(wm);
      nsHTMLReflowState kidReflowState(aPresContext,
                                       aReflowState,
                                       imageFrame,
                                       availableSize,
                                       aMetrics.Width(),
                                       aMetrics.Height());

      uint32_t posterHeight, posterWidth;
      nsSize scaledPosterSize(0, 0);
      nsSize computedArea(aReflowState.ComputedWidth(), aReflowState.ComputedHeight());
      nsPoint posterTopLeft(0, 0);

      nsCOMPtr<nsIDOMHTMLImageElement> posterImage = do_QueryInterface(mPosterImage);
      if (!posterImage) {
        return;
      }
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
      FinishReflowChild(imageFrame, aPresContext, kidDesiredSize, &kidReflowState,
                        posterTopLeft.x, posterTopLeft.y, 0);
    } else if (child->GetContent() == mVideoControls) {
      
      nsBoxLayoutState boxState(PresContext(), aReflowState.rendContext);
      nsSize size = child->GetSize();
      nsBoxFrame::LayoutChildAt(boxState,
                                child,
                                nsRect(mBorderPadding.left,
                                       mBorderPadding.top,
                                       aReflowState.ComputedWidth(),
                                       aReflowState.ComputedHeight()));
      if (child->GetSize() != size) {
        nsRefPtr<nsRunnable> event = new DispatchResizeToControls(child->GetContent());
        nsContentUtils::AddScriptRunner(event);
      }
    } else if (child->GetContent() == mCaptionDiv) {
      
      nsHTMLReflowMetrics kidDesiredSize(aReflowState);
      WritingMode wm = child->GetWritingMode();
      LogicalSize availableSize = aReflowState.AvailableSize(wm);
      nsHTMLReflowState kidReflowState(aPresContext,
                                       aReflowState,
                                       child,
                                       availableSize,
                                       aMetrics.Width(),
                                       aMetrics.Height());
      nsSize size(aReflowState.ComputedWidth(), aReflowState.ComputedHeight());
      size.width -= kidReflowState.ComputedPhysicalBorderPadding().LeftRight();
      size.height -= kidReflowState.ComputedPhysicalBorderPadding().TopBottom();

      kidReflowState.SetComputedWidth(std::max(size.width, 0));
      kidReflowState.SetComputedHeight(std::max(size.height, 0));

      ReflowChild(child, aPresContext, kidDesiredSize, kidReflowState,
                  mBorderPadding.left, mBorderPadding.top, 0, aStatus);
      FinishReflowChild(child, aPresContext,
                        kidDesiredSize, &kidReflowState,
                        mBorderPadding.left, mBorderPadding.top, 0);
    }
  }
  aMetrics.SetOverflowAreasToDesiredBounds();

  FinishAndStoreOverflow(&aMetrics);

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                  ("exit nsVideoFrame::Reflow: size=%d,%d",
                  aMetrics.Width(), aMetrics.Height()));
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
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

  
  
  
  
  
  
  

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE
  {
    *aSnap = true;
    nsIFrame* f = Frame();
    return f->GetContentRect() - f->GetPosition() + ToReferenceFrame();
  }

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE
  {
    return static_cast<nsVideoFrame*>(mFrame)->BuildLayer(aBuilder, aManager, this, aContainerParameters);
  }

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE
  {
    if (aManager->IsCompositingCheap()) {
      
      
      
      
      
      
      return LAYER_ACTIVE;
    }
    HTMLMediaElement* elem =
      static_cast<HTMLMediaElement*>(mFrame->GetContent());
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

  DisplayListClipState::AutoClipContainingBlockDescendantsToContentBox
    clip(aBuilder, this, DisplayListClipState::ASSUME_DRAWING_RESTRICTED_TO_CONTENT_RECT);

  if (HasVideoElement() && !ShouldDisplayPoster()) {
    aLists.Content()->AppendNewToTop(
      new (aBuilder) nsDisplayVideo(aBuilder, this));
  }

  
  
  
  for (nsIFrame *child = mFrames.FirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->GetContent() != mPosterImage || ShouldDisplayPoster()) {
      child->BuildDisplayListForStackingContext(aBuilder,
                                                aDirtyRect - child->GetOffsetTo(this),
                                                aLists.Content());
    } else if (child->GetType() == nsGkAtoms::boxFrame) {
      child->BuildDisplayListForStackingContext(aBuilder,
                                                aDirtyRect - child->GetOffsetTo(this),
                                                aLists.Content());
    }
  }
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

#ifdef DEBUG_FRAME_DUMP
nsresult
nsVideoFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("HTMLVideo"), aResult);
}
#endif

LogicalSize
nsVideoFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                          WritingMode aWM,
                          const LogicalSize& aCBSize,
                          nscoord aAvailableISize,
                          const LogicalSize& aMargin,
                          const LogicalSize& aBorder,
                          const LogicalSize& aPadding,
                          uint32_t aFlags)
{
  nsSize size = GetVideoIntrinsicSize(aRenderingContext);

  IntrinsicSize intrinsicSize;
  intrinsicSize.width.SetCoordValue(size.width);
  intrinsicSize.height.SetCoordValue(size.height);

  
  nsSize intrinsicRatio = HasVideoElement() ? size : nsSize(0, 0);

  return nsLayoutUtils::ComputeSizeWithIntrinsicDimensions(aWM, aRenderingContext,
                                                           this,
                                                           intrinsicSize,
                                                           intrinsicRatio,
                                                           aCBSize,
                                                           aMargin,
                                                           aBorder,
                                                           aPadding);
}

nscoord nsVideoFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord result = GetVideoIntrinsicSize(aRenderingContext).width;
  DISPLAY_MIN_WIDTH(this, result);
  return result;
}

nscoord nsVideoFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nscoord result = GetVideoIntrinsicSize(aRenderingContext).width;
  DISPLAY_PREF_WIDTH(this, result);
  return result;
}

nsSize nsVideoFrame::GetIntrinsicRatio()
{
  if (!HasVideoElement()) {
    
    return nsSize(0, 0);
  }

  return GetVideoIntrinsicSize(nullptr);
}

bool nsVideoFrame::ShouldDisplayPoster()
{
  if (!HasVideoElement())
    return false;

  HTMLVideoElement* element = static_cast<HTMLVideoElement*>(GetContent());
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
    if (!mFrames.FirstChild()) {
      return nsSize(0, 0);
    }

    
    nsBoxLayoutState boxState(PresContext(), aRenderingContext, 0);
    nscoord prefHeight = mFrames.LastChild()->GetPrefSize(boxState).height;
    return nsSize(nsPresContext::CSSPixelsToAppUnits(size.width), prefHeight);
  }

  HTMLVideoElement* element = static_cast<HTMLVideoElement*>(GetContent());
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

void
nsVideoFrame::UpdatePosterSource(bool aNotify)
{
  NS_ASSERTION(HasVideoElement(), "Only call this on <video> elements.");
  HTMLVideoElement* element = static_cast<HTMLVideoElement*>(GetContent());

  if (element->HasAttr(kNameSpaceID_None, nsGkAtoms::poster)) {
    nsAutoString posterStr;
    element->GetPoster(posterStr);
    mPosterImage->SetAttr(kNameSpaceID_None,
                          nsGkAtoms::src,
                          posterStr,
                          aNotify);
  } else {
    mPosterImage->UnsetAttr(kNameSpaceID_None, nsGkAtoms::poster, aNotify);
  }
}

nsresult
nsVideoFrame::AttributeChanged(int32_t aNameSpaceID,
                               nsIAtom* aAttribute,
                               int32_t aModType)
{
  if (aAttribute == nsGkAtoms::poster && HasVideoElement()) {
    UpdatePosterSource(true);
  }
  return nsContainerFrame::AttributeChanged(aNameSpaceID,
                                            aAttribute,
                                            aModType);
}

bool nsVideoFrame::HasVideoElement() {
  nsCOMPtr<nsIDOMHTMLMediaElement> mediaDomElement = do_QueryInterface(mContent);
  return mediaDomElement->IsVideo();
}

bool nsVideoFrame::HasVideoData()
{
  if (!HasVideoElement())
    return false;
  HTMLVideoElement* element = static_cast<HTMLVideoElement*>(GetContent());
  nsIntSize size(0, 0);
  element->GetVideoSize(&size);
  return size != nsIntSize(0,0);
}
