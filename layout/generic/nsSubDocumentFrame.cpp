









#include "nsSubDocumentFrame.h"

#include "mozilla/layout/RenderFrameParent.h"

#include "nsCOMPtr.h"
#include "nsGenericHTMLElement.h"
#include "nsGenericHTMLFrameElement.h"
#include "nsAttrValueInlines.h"
#include "nsIDocShell.h"
#include "nsIContentViewer.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsView.h"
#include "nsViewManager.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsFrameSetFrame.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIScrollable.h"
#include "nsNameSpaceManager.h"
#include "nsDisplayList.h"
#include "nsIScrollableFrame.h"
#include "nsIObjectLoadingContent.h"
#include "nsLayoutUtils.h"
#include "FrameLayerBuilder.h"
#include "nsObjectFrame.h"
#include "nsContentUtils.h"
#include "nsIPermissionManager.h"
#include "nsServiceManagerUtils.h"

using namespace mozilla;
using mozilla::layout::RenderFrameParent;

static nsIDocument*
GetDocumentFromView(nsView* aView)
{
  NS_PRECONDITION(aView, "");

  nsIFrame* f = aView->GetFrame();
  nsIPresShell* ps =  f ? f->PresContext()->PresShell() : nullptr;
  return ps ? ps->GetDocument() : nullptr;
}

nsSubDocumentFrame::nsSubDocumentFrame(nsStyleContext* aContext)
  : nsLeafFrame(aContext)
  , mIsInline(false)
  , mPostedReflowCallback(false)
  , mDidCreateDoc(false)
  , mCallingShow(false)
{
}

#ifdef ACCESSIBILITY
a11y::AccType
nsSubDocumentFrame::AccessibleType()
{
  return a11y::eOuterDocType;
}
#endif

NS_QUERYFRAME_HEAD(nsSubDocumentFrame)
  NS_QUERYFRAME_ENTRY(nsSubDocumentFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsLeafFrame)

class AsyncFrameInit : public nsRunnable
{
public:
  explicit AsyncFrameInit(nsIFrame* aFrame) : mFrame(aFrame) {}
  NS_IMETHOD Run()
  {
    if (mFrame.IsAlive()) {
      static_cast<nsSubDocumentFrame*>(mFrame.GetFrame())->ShowViewer();
    }
    return NS_OK;
  }
private:
  nsWeakFrame mFrame;
};

static void
InsertViewsInReverseOrder(nsView* aSibling, nsView* aParent);

static void
EndSwapDocShellsForViews(nsView* aView);

void
nsSubDocumentFrame::Init(nsIContent*       aContent,
                         nsContainerFrame* aParent,
                         nsIFrame*         aPrevInFlow)
{
  
  if (aContent) {
    nsCOMPtr<nsIDOMHTMLFrameElement> frameElem = do_QueryInterface(aContent);
    mIsInline = frameElem ? false : true;
  }

  nsLeafFrame::Init(aContent, aParent, aPrevInFlow);

  
  
  
  
  
  
  
  
  if (!HasView()) {
    nsContainerFrame::CreateViewForFrame(this, true);
  }
  EnsureInnerView();

  
  
  aContent->SetPrimaryFrame(this);

  
  
  
  
  nsRefPtr<nsFrameLoader> frameloader = FrameLoader();
  if (frameloader) {
    nsCOMPtr<nsIDocument> oldContainerDoc;
    nsView* detachedViews =
      frameloader->GetDetachedSubdocView(getter_AddRefs(oldContainerDoc));
    if (detachedViews) {
      if (oldContainerDoc == aContent->OwnerDoc()) {
        
        ::InsertViewsInReverseOrder(detachedViews, mInnerView);
        ::EndSwapDocShellsForViews(mInnerView->GetFirstChild());
      } else {
        
        frameloader->Hide();
      }
    }
    frameloader->SetDetachedSubdocView(nullptr, nullptr);
  }

  nsContentUtils::AddScriptRunner(new AsyncFrameInit(this));
}

void
nsSubDocumentFrame::ShowViewer()
{
  if (mCallingShow) {
    return;
  }

  if (!PresContext()->IsDynamic()) {
    
    
    (void) EnsureInnerView();
  } else {
    nsRefPtr<nsFrameLoader> frameloader = FrameLoader();
    if (frameloader) {
      nsIntSize margin = GetMarginAttributes();
      nsWeakFrame weakThis(this);
      mCallingShow = true;
      const nsAttrValue* attrValue =
        GetContent()->AsElement()->GetParsedAttr(nsGkAtoms::scrolling);
      int32_t scrolling =
        nsGenericHTMLFrameElement::MapScrollingAttribute(attrValue);
      bool didCreateDoc =
        frameloader->Show(margin.width, margin.height,
                          scrolling, scrolling, this);
      if (!weakThis.IsAlive()) {
        return;
      }
      mCallingShow = false;
      mDidCreateDoc = didCreateDoc;
    }
  }
}

nsIFrame*
nsSubDocumentFrame::GetSubdocumentRootFrame()
{
  if (!mInnerView)
    return nullptr;
  nsView* subdocView = mInnerView->GetFirstChild();
  return subdocView ? subdocView->GetFrame() : nullptr;
}

nsIPresShell*
nsSubDocumentFrame::GetSubdocumentPresShellForPainting(uint32_t aFlags)
{
  if (!mInnerView)
    return nullptr;

  nsView* subdocView = mInnerView->GetFirstChild();
  if (!subdocView)
    return nullptr;

  nsIPresShell* presShell = nullptr;

  nsIFrame* subdocRootFrame = subdocView->GetFrame();
  if (subdocRootFrame) {
    presShell = subdocRootFrame->PresContext()->PresShell();
  }

  
  
  if (!presShell || (presShell->IsPaintingSuppressed() &&
                     !(aFlags & IGNORE_PAINT_SUPPRESSION))) {
    
    
    
    nsView* nextView = subdocView->GetNextSibling();
    nsIFrame* frame = nullptr;
    if (nextView) {
      frame = nextView->GetFrame();
    }
    if (frame) {
      nsIPresShell* ps = frame->PresContext()->PresShell();
      if (!presShell || (ps && !ps->IsPaintingSuppressed())) {
        subdocView = nextView;
        subdocRootFrame = frame;
        presShell = ps;
      }
    }
    if (!presShell) {
      
      if (!mFrameLoader)
        return nullptr;
      nsCOMPtr<nsIDocShell> docShell;
      mFrameLoader->GetDocShell(getter_AddRefs(docShell));
      if (!docShell)
        return nullptr;
      presShell = docShell->GetPresShell();
    }
  }

  return presShell;
}




nsIntSize
nsSubDocumentFrame::GetSubdocumentSize()
{
  if (GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    nsRefPtr<nsFrameLoader> frameloader = FrameLoader();
    if (frameloader) {
      nsCOMPtr<nsIDocument> oldContainerDoc;
      nsView* detachedViews =
        frameloader->GetDetachedSubdocView(getter_AddRefs(oldContainerDoc));
      if (detachedViews) {
        nsSize size = detachedViews->GetBounds().Size();
        nsPresContext* presContext = detachedViews->GetFrame()->PresContext();
        return nsIntSize(presContext->AppUnitsToDevPixels(size.width),
                         presContext->AppUnitsToDevPixels(size.height));
      }
    }
    
    
    return nsIntSize(10, 10);
  } else {
    nsSize docSizeAppUnits;
    nsPresContext* presContext = PresContext();
    nsCOMPtr<nsIDOMHTMLFrameElement> frameElem =
      do_QueryInterface(GetContent());
    if (frameElem) {
      docSizeAppUnits = GetSize();
    } else {
      docSizeAppUnits = GetContentRect().Size();
    }
    return nsIntSize(presContext->AppUnitsToDevPixels(docSizeAppUnits.width),
                     presContext->AppUnitsToDevPixels(docSizeAppUnits.height));
  }
}

bool
nsSubDocumentFrame::PassPointerEventsToChildren()
{
  
  
  
  
  
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::mozpasspointerevents)) {
      if (PresContext()->IsChrome()) {
        return true;
      }

      nsCOMPtr<nsIPermissionManager> permMgr =
        services::GetPermissionManager();
      if (permMgr) {
        uint32_t permission = nsIPermissionManager::DENY_ACTION;
        permMgr->TestPermissionFromPrincipal(GetContent()->NodePrincipal(),
                                             "embed-apps", &permission);

        return permission == nsIPermissionManager::ALLOW_ACTION;
      }
  }

  return false;
}

static void
WrapBackgroundColorInOwnLayer(nsDisplayListBuilder* aBuilder,
                              nsIFrame* aFrame,
                              nsDisplayList* aList)
{
  nsDisplayList tempItems;
  nsDisplayItem* item;
  while ((item = aList->RemoveBottom()) != nullptr) {
    if (item->GetType() == nsDisplayItem::TYPE_BACKGROUND_COLOR) {
      nsDisplayList tmpList;
      tmpList.AppendToTop(item);
      item = new (aBuilder) nsDisplayOwnLayer(aBuilder, aFrame, &tmpList);
    }
    tempItems.AppendToTop(item);
  }
  aList->AppendToTop(&tempItems);
}

void
nsSubDocumentFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  if (!IsVisibleForPainting(aBuilder))
    return;

  nsFrameLoader* frameLoader = FrameLoader();
  RenderFrameParent* rfp = nullptr;
  if (frameLoader) {
    rfp = frameLoader->GetCurrentRemoteFrame();
  }

  
  bool pointerEventsNone = StyleVisibility()->mPointerEvents == NS_STYLE_POINTER_EVENTS_NONE;
  if (!aBuilder->IsForEventDelivery() || !pointerEventsNone) {
    nsDisplayListCollection decorations;
    DisplayBorderBackgroundOutline(aBuilder, decorations);
    if (rfp) {
      
      
      
      
      WrapBackgroundColorInOwnLayer(aBuilder, this, decorations.BorderBackground());
    }
    decorations.MoveTo(aLists);
  }

  bool passPointerEventsToChildren = false;
  if (aBuilder->IsForEventDelivery()) {
    passPointerEventsToChildren = PassPointerEventsToChildren();
    
    
    if (pointerEventsNone && !passPointerEventsToChildren) {
      return;
    }
  }

  
  
  
  if (!mInnerView ||
      (!aBuilder->GetDescendIntoSubdocuments() && !passPointerEventsToChildren)) {
    return;
  }

  if (rfp) {
    rfp->BuildDisplayList(aBuilder, this, aDirtyRect, aLists);
    return;
  }

  nsCOMPtr<nsIPresShell> presShell =
    GetSubdocumentPresShellForPainting(
      aBuilder->IsIgnoringPaintSuppression() ? IGNORE_PAINT_SUPPRESSION : 0);

  if (!presShell) {
    return;
  }

  nsIFrame* subdocRootFrame = presShell->GetRootFrame();

  nsPresContext* presContext = presShell->GetPresContext();

  int32_t parentAPD = PresContext()->AppUnitsPerDevPixel();
  int32_t subdocAPD = presContext->AppUnitsPerDevPixel();

  nsRect dirty;
  bool haveDisplayPort = false;
  bool ignoreViewportScrolling = false;
  nsIFrame* savedIgnoreScrollFrame = nullptr;
  if (subdocRootFrame) {
    
    dirty = aDirtyRect + GetOffsetToCrossDoc(subdocRootFrame);
    
    dirty = dirty.ConvertAppUnitsRoundOut(parentAPD, subdocAPD);

    if (nsIFrame* rootScrollFrame = presShell->GetRootScrollFrame()) {
      
      nsRect displayportBase = presContext->IsRootContentDocument() ?
          nsRect(nsPoint(0,0), nsLayoutUtils::CalculateCompositionSizeForFrame(rootScrollFrame)) :
          dirty.Intersect(nsRect(nsPoint(0,0), subdocRootFrame->GetSize()));
      nsRect displayPort;
      if (!aBuilder->IsForEventDelivery() &&
          nsLayoutUtils::GetOrMaybeCreateDisplayPort(
            *aBuilder, rootScrollFrame, displayportBase, &displayPort)) {
        haveDisplayPort = true;
        dirty = displayPort;
      }

      ignoreViewportScrolling = presShell->IgnoringViewportScrolling();
      if (ignoreViewportScrolling) {
        savedIgnoreScrollFrame = aBuilder->GetIgnoreScrollFrame();
        aBuilder->SetIgnoreScrollFrame(rootScrollFrame);

        if (aBuilder->IsForImageVisibility()) {
          
          
          nsIScrollableFrame* rootScrollableFrame = do_QueryFrame(rootScrollFrame);
          dirty = rootScrollableFrame->ExpandRectToNearlyVisible(dirty);
        }
      }
    }

    aBuilder->EnterPresShell(subdocRootFrame, dirty);
  } else {
    dirty = aDirtyRect;
  }

  DisplayListClipState::AutoSaveRestore clipState(aBuilder);
  if (ShouldClipSubdocument()) {
    clipState.ClipContainingBlockDescendantsToContentBox(aBuilder, this);
  }

  nsIScrollableFrame *sf = presShell->GetRootScrollFrameAsScrollable();
  bool constructResolutionItem = subdocRootFrame &&
    (presShell->GetXResolution() != 1.0 || presShell->GetYResolution() != 1.0);
  bool constructZoomItem = subdocRootFrame && parentAPD != subdocAPD;
  bool needsOwnLayer = constructResolutionItem || constructZoomItem ||
    haveDisplayPort ||
    presContext->IsRootContentDocument() || (sf && sf->IsScrollingActive());

  nsDisplayList childItems;

  {
    DisplayListClipState::AutoSaveRestore nestedClipState(aBuilder);
    if (needsOwnLayer) {
      
      
      
      
      nestedClipState.Clear();
    }

    if (subdocRootFrame) {
      nsIFrame* rootScrollFrame = presShell->GetRootScrollFrame();
      nsDisplayListBuilder::AutoCurrentScrollParentIdSetter idSetter(
          aBuilder,
          ignoreViewportScrolling && rootScrollFrame && rootScrollFrame->GetContent()
              ? nsLayoutUtils::FindOrCreateIDFor(rootScrollFrame->GetContent())
              : aBuilder->GetCurrentScrollParentId());

      aBuilder->SetAncestorHasTouchEventHandler(false);
      subdocRootFrame->
        BuildDisplayListForStackingContext(aBuilder, dirty, &childItems);
    }

    if (!aBuilder->IsForEventDelivery()) {
      
      
      
      
      nsRect bounds = GetContentRectRelativeToSelf() +
        aBuilder->ToReferenceFrame(this);
      if (subdocRootFrame) {
        bounds = bounds.ConvertAppUnitsRoundOut(parentAPD, subdocAPD);
      }

      
      
      
      if (nsLayoutUtils::NeedsPrintPreviewBackground(presContext)) {
        presShell->AddPrintPreviewBackgroundItem(
          *aBuilder, childItems, subdocRootFrame ? subdocRootFrame : this,
          bounds);
      } else {
        
        
        
        nsIFrame* frame = subdocRootFrame ? subdocRootFrame : this;
        nsDisplayListBuilder::AutoBuildingDisplayList
          building(aBuilder, frame, dirty, true);
        
        
        
        uint32_t flags = nsIPresShell::FORCE_DRAW;
        presShell->AddCanvasBackgroundColorItem(
          *aBuilder, childItems, frame, bounds, NS_RGBA(0,0,0,0), flags);
      }
    }
  }

  if (subdocRootFrame) {
    aBuilder->LeavePresShell(subdocRootFrame, dirty);

    if (ignoreViewportScrolling) {
      aBuilder->SetIgnoreScrollFrame(savedIgnoreScrollFrame);
    }
  }

  
  

  uint32_t flags = nsDisplayOwnLayer::GENERATE_SUBDOC_INVALIDATIONS;
  
  
  
  
  if (constructZoomItem) {
    uint32_t zoomFlags = flags;
    if (ignoreViewportScrolling && !constructResolutionItem) {
      zoomFlags |= nsDisplayOwnLayer::GENERATE_SCROLLABLE_LAYER;
    }
    nsDisplayZoom* zoomItem =
      new (aBuilder) nsDisplayZoom(aBuilder, subdocRootFrame, &childItems,
                                   subdocAPD, parentAPD, zoomFlags);
    childItems.AppendToTop(zoomItem);
    needsOwnLayer = false;
  }
  
  
  if (ignoreViewportScrolling) {
    flags |= nsDisplayOwnLayer::GENERATE_SCROLLABLE_LAYER;
  }
  if (constructResolutionItem) {
    nsDisplayResolution* resolutionItem =
      new (aBuilder) nsDisplayResolution(aBuilder, subdocRootFrame, &childItems,
                                         flags);
    childItems.AppendToTop(resolutionItem);
    needsOwnLayer = false;
  }
  if (needsOwnLayer) {
    
    nsDisplaySubDocument* layerItem = new (aBuilder) nsDisplaySubDocument(
      aBuilder, subdocRootFrame ? subdocRootFrame : this,
      &childItems, flags);
    childItems.AppendToTop(layerItem);
  }

  if (aBuilder->IsForImageVisibility()) {
    
    presShell->RebuildImageVisibilityDisplayList(childItems);
    childItems.DeleteAll();
  } else {
    aLists.Content()->AppendToTop(&childItems);
  }
}

nscoord
nsSubDocumentFrame::GetIntrinsicISize()
{
  if (!IsInline()) {
    return 0;  
  }

  if (mContent->IsXUL()) {
    return 0;  
  }

  NS_ASSERTION(ObtainIntrinsicSizeFrame() == nullptr,
               "Intrinsic width should come from the embedded document.");

  
  
  return nsPresContext::CSSPixelsToAppUnits(300);
}

nscoord
nsSubDocumentFrame::GetIntrinsicBSize()
{
  
  NS_ASSERTION(IsInline(), "Shouldn't have been called");

  if (mContent->IsXUL()) {
    return 0;
  }

  NS_ASSERTION(ObtainIntrinsicSizeFrame() == nullptr,
               "Intrinsic height should come from the embedded document.");

  
  return nsPresContext::CSSPixelsToAppUnits(150);
}

#ifdef DEBUG_FRAME_DUMP
void
nsSubDocumentFrame::List(FILE* out, const char* aPrefix, uint32_t aFlags) const
{
  nsCString str;
  ListGeneric(str, aPrefix, aFlags);
  fprintf_stderr(out, "%s\n", str.get());

  if (aFlags & TRAVERSE_SUBDOCUMENT_FRAMES) {
    nsSubDocumentFrame* f = const_cast<nsSubDocumentFrame*>(this);
    nsIFrame* subdocRootFrame = f->GetSubdocumentRootFrame();
    if (subdocRootFrame) {
      nsCString pfx(aPrefix);
      pfx += "  ";
      subdocRootFrame->List(out, pfx.get(), aFlags);
    }
  }
}

nsresult nsSubDocumentFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("FrameOuter"), aResult);
}
#endif

nsIAtom*
nsSubDocumentFrame::GetType() const
{
  return nsGkAtoms::subDocumentFrame;
}

 nscoord
nsSubDocumentFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  nsIFrame* subDocRoot = ObtainIntrinsicSizeFrame();
  if (subDocRoot) {
    result = subDocRoot->GetMinISize(aRenderingContext);
  } else {
    result = GetIntrinsicISize();
  }

  return result;
}

 nscoord
nsSubDocumentFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);

  nsIFrame* subDocRoot = ObtainIntrinsicSizeFrame();
  if (subDocRoot) {
    result = subDocRoot->GetPrefISize(aRenderingContext);
  } else {
    result = GetIntrinsicISize();
  }

  return result;
}

 IntrinsicSize
nsSubDocumentFrame::GetIntrinsicSize()
{
  nsIFrame* subDocRoot = ObtainIntrinsicSizeFrame();
  if (subDocRoot) {
    return subDocRoot->GetIntrinsicSize();
  }
  return nsLeafFrame::GetIntrinsicSize();
}

 nsSize
nsSubDocumentFrame::GetIntrinsicRatio()
{
  nsIFrame* subDocRoot = ObtainIntrinsicSizeFrame();
  if (subDocRoot) {
    return subDocRoot->GetIntrinsicRatio();
  }
  return nsLeafFrame::GetIntrinsicRatio();
}


LogicalSize
nsSubDocumentFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                    WritingMode aWM,
                                    const LogicalSize& aCBSize,
                                    nscoord aAvailableISize,
                                    const LogicalSize& aMargin,
                                    const LogicalSize& aBorder,
                                    const LogicalSize& aPadding,
                                    bool aShrinkWrap)
{
  if (!IsInline()) {
    return nsFrame::ComputeAutoSize(aRenderingContext, aWM, aCBSize,
                                    aAvailableISize, aMargin, aBorder,
                                    aPadding, aShrinkWrap);
  }

  return nsLeafFrame::ComputeAutoSize(aRenderingContext, aWM, aCBSize,
                                      aAvailableISize, aMargin, aBorder,
                                      aPadding, aShrinkWrap);  
}



LogicalSize
nsSubDocumentFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                                WritingMode aWM,
                                const LogicalSize& aCBSize,
                                nscoord aAvailableISize,
                                const LogicalSize& aMargin,
                                const LogicalSize& aBorder,
                                const LogicalSize& aPadding,
                                uint32_t aFlags)
{
  nsIFrame* subDocRoot = ObtainIntrinsicSizeFrame();
  if (subDocRoot) {
    return nsLayoutUtils::ComputeSizeWithIntrinsicDimensions(aWM,
                            aRenderingContext, this,
                            subDocRoot->GetIntrinsicSize(),
                            subDocRoot->GetIntrinsicRatio(),
                            aCBSize,
                            aMargin,
                            aBorder,
                            aPadding);
  }
  return nsLeafFrame::ComputeSize(aRenderingContext, aWM,
                                  aCBSize, aAvailableISize,
                                  aMargin, aBorder, aPadding, aFlags);
}

void
nsSubDocumentFrame::Reflow(nsPresContext*           aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsSubDocumentFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
     ("enter nsSubDocumentFrame::Reflow: maxSize=%d,%d",
      aReflowState.AvailableWidth(), aReflowState.AvailableHeight()));

  aStatus = NS_FRAME_COMPLETE;

  NS_ASSERTION(mContent->GetPrimaryFrame() == this,
               "Shouldn't happen");

  
  nsLeafFrame::DoReflow(aPresContext, aDesiredSize, aReflowState, aStatus);

  
  
  nsPoint offset = nsPoint(aReflowState.ComputedPhysicalBorderPadding().left,
                           aReflowState.ComputedPhysicalBorderPadding().top);

  nsSize innerSize(aDesiredSize.Width(), aDesiredSize.Height());
  innerSize.width  -= aReflowState.ComputedPhysicalBorderPadding().LeftRight();
  innerSize.height -= aReflowState.ComputedPhysicalBorderPadding().TopBottom();

  if (mInnerView) {
    nsViewManager* vm = mInnerView->GetViewManager();
    vm->MoveViewTo(mInnerView, offset.x, offset.y);
    vm->ResizeView(mInnerView, nsRect(nsPoint(0, 0), innerSize), true);
  }

  aDesiredSize.SetOverflowAreasToDesiredBounds();
  if (!ShouldClipSubdocument()) {
    nsIFrame* subdocRootFrame = GetSubdocumentRootFrame();
    if (subdocRootFrame) {
      aDesiredSize.mOverflowAreas.UnionWith(subdocRootFrame->GetOverflowAreas() + offset);
    }
  }

  FinishAndStoreOverflow(&aDesiredSize);

  if (!aPresContext->IsPaginated() && !mPostedReflowCallback) {
    PresContext()->PresShell()->PostReflowCallback(this);
    mPostedReflowCallback = true;
  }

  
  

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
     ("exit nsSubDocumentFrame::Reflow: size=%d,%d status=%x",
      aDesiredSize.Width(), aDesiredSize.Height(), aStatus));

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

bool
nsSubDocumentFrame::ReflowFinished()
{
  if (mFrameLoader) {
    nsWeakFrame weakFrame(this);

    mFrameLoader->UpdatePositionAndSize(this);

    if (weakFrame.IsAlive()) {
      
      mPostedReflowCallback = false;
    }
  } else {
    mPostedReflowCallback = false;
  }
  return false;
}

void
nsSubDocumentFrame::ReflowCallbackCanceled()
{
  mPostedReflowCallback = false;
}

nsresult
nsSubDocumentFrame::AttributeChanged(int32_t aNameSpaceID,
                                     nsIAtom* aAttribute,
                                     int32_t aModType)
{
  if (aNameSpaceID != kNameSpaceID_None) {
    return NS_OK;
  }
  
  
  if (aAttribute == nsGkAtoms::noresize) {
    
    
    if (mContent->GetParent()->Tag() == nsGkAtoms::frameset) {
      nsIFrame* parentFrame = GetParent();

      if (parentFrame) {
        
        
        nsHTMLFramesetFrame* framesetFrame = do_QueryFrame(parentFrame);
        if (framesetFrame) {
          framesetFrame->RecalculateBorderResize();
        }
      }
    }
  }
  else if (aAttribute == nsGkAtoms::showresizer) {
    nsIFrame* rootFrame = GetSubdocumentRootFrame();
    if (rootFrame) {
      rootFrame->PresContext()->PresShell()->
        FrameNeedsReflow(rootFrame, nsIPresShell::eResize, NS_FRAME_IS_DIRTY);
    }
  }
  else if (aAttribute == nsGkAtoms::marginwidth ||
           aAttribute == nsGkAtoms::marginheight) {

    
    nsIntSize margins = GetMarginAttributes();

    
    nsRefPtr<nsFrameLoader> frameloader = FrameLoader();
    if (frameloader)
      frameloader->MarginsChanged(margins.width, margins.height);
  }

  return NS_OK;
}

nsIFrame*
NS_NewSubDocumentFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSubDocumentFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSubDocumentFrame)

class nsHideViewer : public nsRunnable {
public:
  nsHideViewer(nsIContent* aFrameElement,
               nsFrameLoader* aFrameLoader,
               nsIPresShell* aPresShell,
               bool aHideViewerIfFrameless)
    : mFrameElement(aFrameElement),
      mFrameLoader(aFrameLoader),
      mPresShell(aPresShell),
      mHideViewerIfFrameless(aHideViewerIfFrameless)
  {
    NS_ASSERTION(mFrameElement, "Must have a frame element");
    NS_ASSERTION(mFrameLoader, "Must have a frame loader");
    NS_ASSERTION(mPresShell, "Must have a presshell");
  }

  NS_IMETHOD Run()
  {
    
    
    
    if (!mPresShell->IsDestroying()) {
      mPresShell->FlushPendingNotifications(Flush_Frames);
    }
    nsIFrame* frame = mFrameElement->GetPrimaryFrame();
    if ((!frame && mHideViewerIfFrameless) ||
        mPresShell->IsDestroying()) {
      
      
      
      mFrameLoader->SetDetachedSubdocView(nullptr, nullptr);
      mFrameLoader->Hide();
    }
    return NS_OK;
  }
private:
  nsCOMPtr<nsIContent> mFrameElement;
  nsRefPtr<nsFrameLoader> mFrameLoader;
  nsCOMPtr<nsIPresShell> mPresShell;
  bool mHideViewerIfFrameless;
};

static nsView*
BeginSwapDocShellsForViews(nsView* aSibling);

void
nsSubDocumentFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  if (mPostedReflowCallback) {
    PresContext()->PresShell()->CancelReflowCallback(this);
    mPostedReflowCallback = false;
  }

  
  
  
  nsFrameLoader* frameloader = FrameLoader();
  if (frameloader) {
    nsView* detachedViews = ::BeginSwapDocShellsForViews(mInnerView->GetFirstChild());
    frameloader->SetDetachedSubdocView(detachedViews, mContent->OwnerDoc());

    
    
    nsContentUtils::AddScriptRunner(
      new nsHideViewer(mContent,
                       mFrameLoader,
                       PresContext()->PresShell(),
                       (mDidCreateDoc || mCallingShow)));
  }

  nsLeafFrame::DestroyFrom(aDestructRoot);
}

nsIntSize
nsSubDocumentFrame::GetMarginAttributes()
{
  nsIntSize result(-1, -1);
  nsGenericHTMLElement *content = nsGenericHTMLElement::FromContent(mContent);
  if (content) {
    const nsAttrValue* attr = content->GetParsedAttr(nsGkAtoms::marginwidth);
    if (attr && attr->Type() == nsAttrValue::eInteger)
      result.width = attr->GetIntegerValue();
    attr = content->GetParsedAttr(nsGkAtoms::marginheight);
    if (attr && attr->Type() == nsAttrValue::eInteger)
      result.height = attr->GetIntegerValue();
  }
  return result;
}

nsFrameLoader*
nsSubDocumentFrame::FrameLoader()
{
  nsIContent* content = GetContent();
  if (!content)
    return nullptr;

  if (!mFrameLoader) {
    nsCOMPtr<nsIFrameLoaderOwner> loaderOwner = do_QueryInterface(content);
    if (loaderOwner) {
      nsCOMPtr<nsIFrameLoader> loader;
      loaderOwner->GetFrameLoader(getter_AddRefs(loader));
      mFrameLoader = static_cast<nsFrameLoader*>(loader.get());
    }
  }
  return mFrameLoader;
}



nsresult
nsSubDocumentFrame::GetDocShell(nsIDocShell **aDocShell)
{
  *aDocShell = nullptr;

  NS_ENSURE_STATE(FrameLoader());
  return mFrameLoader->GetDocShell(aDocShell);
}

static void
DestroyDisplayItemDataForFrames(nsIFrame* aFrame)
{
  FrameLayerBuilder::DestroyDisplayItemDataFor(aFrame);

  nsIFrame::ChildListIterator lists(aFrame);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      DestroyDisplayItemDataForFrames(childFrames.get());
    }
  }
}

static bool
BeginSwapDocShellsForDocument(nsIDocument* aDocument, void*)
{
  NS_PRECONDITION(aDocument, "");

  nsIPresShell* shell = aDocument->GetShell();
  if (shell) {
    
    shell->SetNeverPainting(true);

    nsIFrame* rootFrame = shell->GetRootFrame();
    if (rootFrame) {
      ::DestroyDisplayItemDataForFrames(rootFrame);
    }
  }
  aDocument->EnumerateActivityObservers(
    nsObjectFrame::BeginSwapDocShells, nullptr);
  aDocument->EnumerateSubDocuments(BeginSwapDocShellsForDocument, nullptr);
  return true;
}

static nsView*
BeginSwapDocShellsForViews(nsView* aSibling)
{
  
  nsView* removedViews = nullptr;
  while (aSibling) {
    nsIDocument* doc = ::GetDocumentFromView(aSibling);
    if (doc) {
      ::BeginSwapDocShellsForDocument(doc, nullptr);
    }
    nsView* next = aSibling->GetNextSibling();
    aSibling->GetViewManager()->RemoveChild(aSibling);
    aSibling->SetNextSibling(removedViews);
    removedViews = aSibling;
    aSibling = next;
  }
  return removedViews;
}

static void
InsertViewsInReverseOrder(nsView* aSibling, nsView* aParent)
{
  NS_PRECONDITION(aParent, "");
  NS_PRECONDITION(!aParent->GetFirstChild(), "inserting into non-empty list");

  nsViewManager* vm = aParent->GetViewManager();
  while (aSibling) {
    nsView* next = aSibling->GetNextSibling();
    aSibling->SetNextSibling(nullptr);
    
    
    vm->InsertChild(aParent, aSibling, nullptr, true);
    aSibling = next;
  }
}

nsresult
nsSubDocumentFrame::BeginSwapDocShells(nsIFrame* aOther)
{
  if (!aOther || aOther->GetType() != nsGkAtoms::subDocumentFrame) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsSubDocumentFrame* other = static_cast<nsSubDocumentFrame*>(aOther);
  if (!mFrameLoader || !mDidCreateDoc || mCallingShow ||
      !other->mFrameLoader || !other->mDidCreateDoc) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  if (mInnerView && other->mInnerView) {
    nsView* ourSubdocViews = mInnerView->GetFirstChild();
    nsView* ourRemovedViews = ::BeginSwapDocShellsForViews(ourSubdocViews);
    nsView* otherSubdocViews = other->mInnerView->GetFirstChild();
    nsView* otherRemovedViews = ::BeginSwapDocShellsForViews(otherSubdocViews);

    ::InsertViewsInReverseOrder(ourRemovedViews, other->mInnerView);
    ::InsertViewsInReverseOrder(otherRemovedViews, mInnerView);
  }
  mFrameLoader.swap(other->mFrameLoader);
  return NS_OK;
}

static bool
EndSwapDocShellsForDocument(nsIDocument* aDocument, void*)
{
  NS_PRECONDITION(aDocument, "");

  
  
  
  nsCOMPtr<nsIDocShell> ds = aDocument->GetDocShell();
  if (ds) {
    nsCOMPtr<nsIContentViewer> cv;
    ds->GetContentViewer(getter_AddRefs(cv));
    while (cv) {
      nsRefPtr<nsPresContext> pc;
      cv->GetPresContext(getter_AddRefs(pc));
      if (pc && pc->GetPresShell()) {
        pc->GetPresShell()->SetNeverPainting(ds->IsInvisible());
      }
      nsDeviceContext* dc = pc ? pc->DeviceContext() : nullptr;
      if (dc) {
        nsView* v = cv->FindContainerView();
        dc->Init(v ? v->GetNearestWidget(nullptr) : nullptr);
      }
      nsCOMPtr<nsIContentViewer> prev;
      cv->GetPreviousViewer(getter_AddRefs(prev));
      cv = prev;
    }
  }

  aDocument->EnumerateActivityObservers(
    nsObjectFrame::EndSwapDocShells, nullptr);
  aDocument->EnumerateSubDocuments(EndSwapDocShellsForDocument, nullptr);
  return true;
}

static void
EndSwapDocShellsForViews(nsView* aSibling)
{
  for ( ; aSibling; aSibling = aSibling->GetNextSibling()) {
    nsIDocument* doc = ::GetDocumentFromView(aSibling);
    if (doc) {
      ::EndSwapDocShellsForDocument(doc, nullptr);
    }
    nsIFrame *frame = aSibling->GetFrame();
    if (frame) {
      nsIFrame* parent = nsLayoutUtils::GetCrossDocParentFrame(frame);
      if (parent->HasAnyStateBits(NS_FRAME_IN_POPUP)) {
        nsIFrame::AddInPopupStateBitToDescendants(frame);
      } else {
        nsIFrame::RemoveInPopupStateBitFromDescendants(frame);
      }
      if (frame->HasInvalidFrameInSubtree()) {
        while (parent && !parent->HasAnyStateBits(NS_FRAME_DESCENDANT_NEEDS_PAINT)) {
          parent->AddStateBits(NS_FRAME_DESCENDANT_NEEDS_PAINT);
          parent = nsLayoutUtils::GetCrossDocParentFrame(parent);
        }
      }
    }
  }
}

void
nsSubDocumentFrame::EndSwapDocShells(nsIFrame* aOther)
{
  nsSubDocumentFrame* other = static_cast<nsSubDocumentFrame*>(aOther);
  nsWeakFrame weakThis(this);
  nsWeakFrame weakOther(aOther);

  if (mInnerView) {
    ::EndSwapDocShellsForViews(mInnerView->GetFirstChild());
  }
  if (other->mInnerView) {
    ::EndSwapDocShellsForViews(other->mInnerView->GetFirstChild());
  }

  
  
  
  
  if (weakThis.IsAlive()) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange, NS_FRAME_IS_DIRTY);
    InvalidateFrameSubtree();
  }
  if (weakOther.IsAlive()) {
    other->PresContext()->PresShell()->
      FrameNeedsReflow(other, nsIPresShell::eTreeChange, NS_FRAME_IS_DIRTY);
    other->InvalidateFrameSubtree();
  }
}

nsView*
nsSubDocumentFrame::EnsureInnerView()
{
  if (mInnerView) {
    return mInnerView;
  }

  
  nsView* outerView = GetView();
  NS_ASSERTION(outerView, "Must have an outer view already");
  nsRect viewBounds(0, 0, 0, 0); 

  nsViewManager* viewMan = outerView->GetViewManager();
  nsView* innerView = viewMan->CreateView(viewBounds, outerView);
  if (!innerView) {
    NS_ERROR("Could not create inner view");
    return nullptr;
  }
  mInnerView = innerView;
  viewMan->InsertChild(outerView, innerView, nullptr, true);

  return mInnerView;
}

nsIFrame*
nsSubDocumentFrame::ObtainIntrinsicSizeFrame()
{
  nsCOMPtr<nsIObjectLoadingContent> olc = do_QueryInterface(GetContent());
  if (olc) {
    

    
    nsIFrame* subDocRoot = nullptr;

    nsCOMPtr<nsIDocShell> docShell;
    GetDocShell(getter_AddRefs(docShell));
    if (docShell) {
      nsCOMPtr<nsIPresShell> presShell = docShell->GetPresShell();
      if (presShell) {
        nsIScrollableFrame* scrollable = presShell->GetRootScrollFrameAsScrollable();
        if (scrollable) {
          nsIFrame* scrolled = scrollable->GetScrolledFrame();
          if (scrolled) {
            subDocRoot = scrolled->GetFirstPrincipalChild();
          }
        }
      }
    }

    if (subDocRoot && subDocRoot->GetContent() &&
        subDocRoot->GetContent()->NodeInfo()->Equals(nsGkAtoms::svg, kNameSpaceID_SVG)) {
      return subDocRoot; 
    }
  }
  return nullptr;
}
