









#include "mozilla/layout/RenderFrameParent.h"

#include "nsSubDocumentFrame.h"
#include "nsCOMPtr.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValueInlines.h"
#include "nsIDocShell.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIBaseWindow.h"
#include "nsIContentViewer.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIComponentManager.h"
#include "nsFrameManager.h"
#include "nsIStreamListener.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIDocument.h"
#include "nsView.h"
#include "nsViewManager.h"
#include "nsGkAtoms.h"
#include "nsStyleCoord.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsFrameSetFrame.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMXULElement.h"
#include "nsIScriptSecurityManager.h"
#include "nsXPIDLString.h"
#include "nsIScrollable.h"
#include "nsINameSpaceManager.h"
#include "nsWeakReference.h"
#include "nsIDOMWindow.h"
#include "nsDisplayList.h"
#include "nsUnicharUtils.h"
#include "nsIScrollableFrame.h"
#include "nsIObjectLoadingContent.h"
#include "nsLayoutUtils.h"
#include "FrameLayerBuilder.h"
#include "nsObjectFrame.h"
#include "nsIServiceManager.h"
#include "nsContentUtils.h"
#include "LayerTreeInvalidation.h"
#include "nsIPermissionManager.h"

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

class AsyncFrameInit;

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
  AsyncFrameInit(nsIFrame* aFrame) : mFrame(aFrame) {}
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
nsSubDocumentFrame::Init(nsIContent*     aContent,
                         nsIFrame*       aParent,
                         nsIFrame*       aPrevInFlow)
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

inline int32_t ConvertOverflow(uint8_t aOverflow)
{
  switch (aOverflow) {
    case NS_STYLE_OVERFLOW_VISIBLE:
    case NS_STYLE_OVERFLOW_AUTO:
      return nsIScrollable::Scrollbar_Auto;
    case NS_STYLE_OVERFLOW_HIDDEN:
    case NS_STYLE_OVERFLOW_CLIP:
      return nsIScrollable::Scrollbar_Never;
    case NS_STYLE_OVERFLOW_SCROLL:
      return nsIScrollable::Scrollbar_Always;
  }
  NS_NOTREACHED("invalid overflow value passed to ConvertOverflow");
  return nsIScrollable::Scrollbar_Auto;
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
      const nsStyleDisplay* disp = StyleDisplay();
      nsWeakFrame weakThis(this);
      mCallingShow = true;
      bool didCreateDoc =
        frameloader->Show(margin.width, margin.height,
                          ConvertOverflow(disp->mOverflowX),
                          ConvertOverflow(disp->mOverflowY),
                          this);
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
  if (StyleVisibility()->mPointerEvents != NS_STYLE_POINTER_EVENTS_NONE) {
    return true;
  }
  
  
  
  
  
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::mozpasspointerevents)) {
      if (PresContext()->IsChrome()) {
        return true;
      }

      nsCOMPtr<nsIPermissionManager> permMgr =
        do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
      if (permMgr) {
        uint32_t permission = nsIPermissionManager::DENY_ACTION;
        permMgr->TestPermissionFromPrincipal(GetContent()->NodePrincipal(),
                                             "embed-apps", &permission);

        return permission == nsIPermissionManager::ALLOW_ACTION;
      }
  }

  return false;
}

void
nsSubDocumentFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  if (!IsVisibleForPainting(aBuilder))
    return;

  
  
  if (aBuilder->IsForEventDelivery() && !PassPointerEventsToChildren())
    return;

  DisplayBorderBackgroundOutline(aBuilder, aLists);

  if (!mInnerView)
    return;

  nsFrameLoader* frameLoader = FrameLoader();
  if (frameLoader) {
    RenderFrameParent* rfp = frameLoader->GetCurrentRemoteFrame();
    if (rfp) {
      rfp->BuildDisplayList(aBuilder, this, aDirtyRect, aLists);
      return;
    }
  }

  nsView* subdocView = mInnerView->GetFirstChild();
  if (!subdocView)
    return;

  nsCOMPtr<nsIPresShell> presShell = nullptr;

  nsIFrame* subdocRootFrame = subdocView->GetFrame();
  if (subdocRootFrame) {
    presShell = subdocRootFrame->PresContext()->PresShell();
  }
  
  
  if (!presShell || (presShell->IsPaintingSuppressed() &&
                     !aBuilder->IsIgnoringPaintSuppression())) {
    
    
    
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
        return;
      nsCOMPtr<nsIDocShell> docShell;
      mFrameLoader->GetDocShell(getter_AddRefs(docShell));
      if (!docShell)
        return;
      presShell = docShell->GetPresShell();
      if (!presShell)
        return;
    }
  }

  nsPresContext* presContext = presShell->GetPresContext();

  int32_t parentAPD = PresContext()->AppUnitsPerDevPixel();
  int32_t subdocAPD = presContext->AppUnitsPerDevPixel();

  nsRect dirty;
  if (subdocRootFrame) {
    nsIDocument* doc = subdocRootFrame->PresContext()->Document();
    nsIContent* root = doc ? doc->GetRootElement() : nullptr;
    nsRect displayPort;
    if (root && nsLayoutUtils::GetDisplayPort(root, &displayPort)) {
      dirty = displayPort;
    } else {
      
      dirty = aDirtyRect + GetOffsetToCrossDoc(subdocRootFrame);
      
      dirty = dirty.ConvertAppUnitsRoundOut(parentAPD, subdocAPD);
    }

    aBuilder->EnterPresShell(subdocRootFrame, dirty);
  }

  DisplayListClipState::AutoSaveRestore clipState(aBuilder);
  if (ShouldClipSubdocument()) {
    clipState.ClipContainingBlockDescendantsToContentBox(aBuilder, this);
  }

  nsIScrollableFrame *sf = presShell->GetRootScrollFrameAsScrollable();
  bool constructZoomItem = subdocRootFrame && parentAPD != subdocAPD;
  bool needsOwnLayer = constructZoomItem ||
    presContext->IsRootContentDocument() || (sf && sf->IsScrollingActive());

  nsDisplayList childItems;

  {
    DisplayListClipState::AutoSaveRestore nestedClipState(aBuilder);
    if (needsOwnLayer) {
      
      
      
      
      nestedClipState.Clear();
    }

    if (subdocRootFrame) {
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
        
        
        
        uint32_t flags = nsIPresShell::FORCE_DRAW;
        presShell->AddCanvasBackgroundColorItem(
          *aBuilder, childItems, subdocRootFrame ? subdocRootFrame : this,
          bounds, NS_RGBA(0,0,0,0), flags);
      }
    }
  }

  if (constructZoomItem) {
    nsDisplayZoom* zoomItem =
      new (aBuilder) nsDisplayZoom(aBuilder, subdocRootFrame, &childItems,
                                   subdocAPD, parentAPD,
                                   nsDisplayOwnLayer::GENERATE_SUBDOC_INVALIDATIONS);
    childItems.AppendToTop(zoomItem);
  } else if (needsOwnLayer) {
    
    nsDisplayOwnLayer* layerItem = new (aBuilder) nsDisplayOwnLayer(
      aBuilder, subdocRootFrame ? subdocRootFrame : this,
      &childItems, nsDisplayOwnLayer::GENERATE_SUBDOC_INVALIDATIONS);
    childItems.AppendToTop(layerItem);
  }

  if (subdocRootFrame) {
    aBuilder->LeavePresShell(subdocRootFrame, dirty);
  }

  if (aBuilder->IsForImageVisibility()) {
    
    presShell->RebuildImageVisibility(childItems);
    childItems.DeleteAll();
  } else {
    aLists.Content()->AppendToTop(&childItems);
  }
}

nscoord
nsSubDocumentFrame::GetIntrinsicWidth()
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
nsSubDocumentFrame::GetIntrinsicHeight()
{
  
  NS_ASSERTION(IsInline(), "Shouldn't have been called");

  if (mContent->IsXUL()) {
    return 0;
  }

  NS_ASSERTION(ObtainIntrinsicSizeFrame() == nullptr,
               "Intrinsic height should come from the embedded document.");

  
  return nsPresContext::CSSPixelsToAppUnits(150);
}

#ifdef DEBUG
NS_IMETHODIMP
nsSubDocumentFrame::List(FILE* out, int32_t aIndent, uint32_t aFlags) const
{
  IndentBy(out, aIndent);
  ListTag(out);
#ifdef DEBUG_waterson
  fprintf(out, " [parent=%p]", static_cast<void*>(mParent));
#endif
  if (HasView()) {
    fprintf(out, " [view=%p]", static_cast<void*>(GetView()));
  }
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%016llx]", (unsigned long long)mState);
  }
  nsIFrame* prevInFlow = GetPrevInFlow();
  nsIFrame* nextInFlow = GetNextInFlow();
  if (nullptr != prevInFlow) {
    fprintf(out, " prev-in-flow=%p", static_cast<void*>(prevInFlow));
  }
  if (nullptr != nextInFlow) {
    fprintf(out, " next-in-flow=%p", static_cast<void*>(nextInFlow));
  }
  fprintf(out, " [content=%p]", static_cast<void*>(mContent));
  nsSubDocumentFrame* f = const_cast<nsSubDocumentFrame*>(this);
  if (f->HasOverflowAreas()) {
    nsRect overflowArea = f->GetVisualOverflowRect();
    fprintf(out, " [vis-overflow=%d,%d,%d,%d]", overflowArea.x, overflowArea.y,
            overflowArea.width, overflowArea.height);
    overflowArea = f->GetScrollableOverflowRect();
    fprintf(out, " [scr-overflow=%d,%d,%d,%d]", overflowArea.x, overflowArea.y,
            overflowArea.width, overflowArea.height);
  }
  fprintf(out, " [sc=%p]", static_cast<void*>(mStyleContext));
  fputs("\n", out);

  if (aFlags & TRAVERSE_SUBDOCUMENT_FRAMES) {
    nsIFrame* subdocRootFrame = f->GetSubdocumentRootFrame();
    if (subdocRootFrame) {
      subdocRootFrame->List(out, aIndent + 1);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsSubDocumentFrame::GetFrameName(nsAString& aResult) const
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
nsSubDocumentFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  nsIFrame* subDocRoot = ObtainIntrinsicSizeFrame();
  if (subDocRoot) {
    result = subDocRoot->GetMinWidth(aRenderingContext);
  } else {
    result = GetIntrinsicWidth();
  }

  return result;
}

 nscoord
nsSubDocumentFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);

  nsIFrame* subDocRoot = ObtainIntrinsicSizeFrame();
  if (subDocRoot) {
    result = subDocRoot->GetPrefWidth(aRenderingContext);
  } else {
    result = GetIntrinsicWidth();
  }

  return result;
}

 nsIFrame::IntrinsicSize
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

 nsSize
nsSubDocumentFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                    nsSize aCBSize, nscoord aAvailableWidth,
                                    nsSize aMargin, nsSize aBorder,
                                    nsSize aPadding, bool aShrinkWrap)
{
  if (!IsInline()) {
    return nsFrame::ComputeAutoSize(aRenderingContext, aCBSize,
                                    aAvailableWidth, aMargin, aBorder,
                                    aPadding, aShrinkWrap);
  }

  return nsLeafFrame::ComputeAutoSize(aRenderingContext, aCBSize,
                                      aAvailableWidth, aMargin, aBorder,
                                      aPadding, aShrinkWrap);  
}


 nsSize
nsSubDocumentFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                                nsSize aCBSize, nscoord aAvailableWidth,
                                nsSize aMargin, nsSize aBorder, nsSize aPadding,
                                uint32_t aFlags)
{
  nsIFrame* subDocRoot = ObtainIntrinsicSizeFrame();
  if (subDocRoot) {
    return nsLayoutUtils::ComputeSizeWithIntrinsicDimensions(
                            aRenderingContext, this,
                            subDocRoot->GetIntrinsicSize(),
                            subDocRoot->GetIntrinsicRatio(),
                            aCBSize, aMargin, aBorder, aPadding);
  }
  return nsLeafFrame::ComputeSize(aRenderingContext, aCBSize, aAvailableWidth,
                                  aMargin, aBorder, aPadding, aFlags);
}

NS_IMETHODIMP
nsSubDocumentFrame::Reflow(nsPresContext*           aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsSubDocumentFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
     ("enter nsSubDocumentFrame::Reflow: maxSize=%d,%d",
      aReflowState.availableWidth, aReflowState.availableHeight));

  aStatus = NS_FRAME_COMPLETE;

  NS_ASSERTION(mContent->GetPrimaryFrame() == this,
               "Shouldn't happen");

  
  nsresult rv = nsLeafFrame::DoReflow(aPresContext, aDesiredSize, aReflowState,
                                      aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsPoint offset = nsPoint(aReflowState.mComputedBorderPadding.left,
                           aReflowState.mComputedBorderPadding.top);

  nsSize innerSize(aDesiredSize.width, aDesiredSize.height);
  innerSize.width  -= aReflowState.mComputedBorderPadding.LeftRight();
  innerSize.height -= aReflowState.mComputedBorderPadding.TopBottom();

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
      aDesiredSize.width, aDesiredSize.height, aStatus));

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
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

NS_IMETHODIMP
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
  nsIFrame* rootFrame = shell ? shell->GetRootFrame() : nullptr;
  if (rootFrame) {
    ::DestroyDisplayItemDataForFrames(rootFrame);
  }
  aDocument->EnumerateFreezableElements(
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

  
  
  
  nsCOMPtr<nsISupports> container = aDocument->GetContainer();
  nsCOMPtr<nsIDocShell> ds = do_QueryInterface(container);
  if (ds) {
    nsCOMPtr<nsIContentViewer> cv;
    ds->GetContentViewer(getter_AddRefs(cv));
    while (cv) {
      nsCOMPtr<nsPresContext> pc;
      cv->GetPresContext(getter_AddRefs(pc));
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

  aDocument->EnumerateFreezableElements(
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
