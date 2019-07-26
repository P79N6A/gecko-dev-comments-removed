









#include "mozilla/layout/RenderFrameParent.h"

#include "nsSubDocumentFrame.h"
#include "nsCOMPtr.h"
#include "nsGenericHTMLElement.h"
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
#include "nsIView.h"
#include "nsIViewManager.h"
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
#include "nsIDOMDocument.h"
#include "nsDisplayList.h"
#include "nsUnicharUtils.h"
#include "nsIScrollableFrame.h"
#include "nsIObjectLoadingContent.h"
#include "nsLayoutUtils.h"
#include "FrameLayerBuilder.h"
#include "nsObjectFrame.h"
#include "nsIServiceManager.h"
#include "nsContentUtils.h"

#ifdef MOZ_XUL
#include "nsXULPopupManager.h"
#endif


#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

using namespace mozilla;
using mozilla::layout::RenderFrameParent;

static nsIDocument*
GetDocumentFromView(nsIView* aView)
{
  NS_PRECONDITION(aView, "");

  nsIFrame* f = aView->GetFrame();
  nsIPresShell* ps =  f ? f->PresContext()->PresShell() : nsnull;
  return ps ? ps->GetDocument() : nsnull;
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
already_AddRefed<Accessible>
nsSubDocumentFrame::CreateAccessible()
{
  nsAccessibilityService* accService = nsIPresShell::AccService();
  return accService ?
    accService->CreateOuterDocAccessible(mContent, PresContext()->PresShell()) :
    nsnull;
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

NS_IMETHODIMP
nsSubDocumentFrame::Init(nsIContent*     aContent,
                         nsIFrame*       aParent,
                         nsIFrame*       aPrevInFlow)
{
  
  if (aContent) {
    nsCOMPtr<nsIDOMHTMLFrameElement> frameElem = do_QueryInterface(aContent);
    mIsInline = frameElem ? false : true;
  }

  nsresult rv =  nsLeafFrame::Init(aContent, aParent, aPrevInFlow);
  if (NS_FAILED(rv))
    return rv;

  
  
  
  
  
  
  
  
  if (!HasView()) {
    rv = nsContainerFrame::CreateViewForFrame(this, true);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  EnsureInnerView();

  
  
  
  aContent->SetPrimaryFrame(this);

  nsContentUtils::AddScriptRunner(new AsyncFrameInit(this));
  return NS_OK;
}

inline PRInt32 ConvertOverflow(PRUint8 aOverflow)
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
      const nsStyleDisplay* disp = GetStyleDisplay();
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

PRIntn
nsSubDocumentFrame::GetSkipSides() const
{
  return 0;
}

nsIFrame*
nsSubDocumentFrame::GetSubdocumentRootFrame()
{
  if (!mInnerView)
    return nsnull;
  nsIView* subdocView = mInnerView->GetFirstChild();
  return subdocView ? subdocView->GetFrame() : nsnull;
}

NS_IMETHODIMP
nsSubDocumentFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  if (!IsVisibleForPainting(aBuilder))
    return NS_OK;

  if (aBuilder->IsForEventDelivery() &&
      GetStyleVisibility()->mPointerEvents == NS_STYLE_POINTER_EVENTS_NONE)
    return NS_OK;

  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mInnerView)
    return NS_OK;

  nsFrameLoader* frameLoader = FrameLoader();
  if (frameLoader) {
    RenderFrameParent* rfp = frameLoader->GetCurrentRemoteFrame();
    if (rfp) {
      return rfp->BuildDisplayList(aBuilder, this, aDirtyRect, aLists);
    }
  }

  nsIView* subdocView = mInnerView->GetFirstChild();
  if (!subdocView)
    return NS_OK;

  nsCOMPtr<nsIPresShell> presShell = nsnull;

  nsIFrame* subdocRootFrame = subdocView->GetFrame();
  if (subdocRootFrame) {
    presShell = subdocRootFrame->PresContext()->PresShell();
  }
  
  
  if (!presShell || (presShell->IsPaintingSuppressed() &&
                     !aBuilder->IsIgnoringPaintSuppression())) {
    
    
    
    nsIView* nextView = subdocView->GetNextSibling();
    nsIFrame* frame = nsnull;
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
        return NS_OK;
      nsCOMPtr<nsIDocShell> docShell;
      mFrameLoader->GetDocShell(getter_AddRefs(docShell));
      if (!docShell)
        return NS_OK;
      docShell->GetPresShell(getter_AddRefs(presShell));
      if (!presShell)
        return NS_OK;
    }
  }

  nsPresContext* presContext = presShell->GetPresContext();

  nsDisplayList childItems;

  PRInt32 parentAPD = PresContext()->AppUnitsPerDevPixel();
  PRInt32 subdocAPD = presContext->AppUnitsPerDevPixel();

  nsRect dirty;
  if (subdocRootFrame) {
    
    dirty = aDirtyRect + GetOffsetToCrossDoc(subdocRootFrame);
    
    dirty = dirty.ConvertAppUnitsRoundOut(parentAPD, subdocAPD);

    aBuilder->EnterPresShell(subdocRootFrame, dirty);
  }

  nsRect subdocBoundsInParentUnits =
    mInnerView->GetBounds() + GetOffsetToCrossDoc(aBuilder->ReferenceFrame());

  if (subdocRootFrame) {
    rv = subdocRootFrame->
           BuildDisplayListForStackingContext(aBuilder, dirty, &childItems);
  }

  if (!aBuilder->IsForEventDelivery()) {
    
    
    
    
    nsRect bounds;
    if (subdocRootFrame) {
      bounds = subdocBoundsInParentUnits.ConvertAppUnitsRoundOut(parentAPD, subdocAPD);
    } else {
      bounds = subdocBoundsInParentUnits;
    }

    
    
    
    if (nsLayoutUtils::NeedsPrintPreviewBackground(presContext)) {
      rv = presShell->AddPrintPreviewBackgroundItem(
             *aBuilder, childItems, subdocRootFrame ? subdocRootFrame : this,
             bounds);
    } else {
      
      
      
      PRUint32 flags = nsIPresShell::FORCE_DRAW;
      rv = presShell->AddCanvasBackgroundColorItem(
             *aBuilder, childItems, subdocRootFrame ? subdocRootFrame : this,
             bounds, NS_RGBA(0,0,0,0), flags);
    }
  }

  bool addedLayer = false;

  if (subdocRootFrame && parentAPD != subdocAPD) {
    NS_WARN_IF_FALSE(!addedLayer,
                     "Two container layers have been added. "
                      "Performance may suffer.");
    addedLayer = true;

    nsDisplayZoom* zoomItem =
      new (aBuilder) nsDisplayZoom(aBuilder, subdocRootFrame, &childItems,
                                   subdocAPD, parentAPD);
    childItems.AppendToTop(zoomItem);
  }

  if (!addedLayer && presContext->IsRootContentDocument()) {
    
    nsDisplayOwnLayer* layerItem = new (aBuilder) nsDisplayOwnLayer(
      aBuilder, subdocRootFrame ? subdocRootFrame : this, &childItems);
    childItems.AppendToTop(layerItem);
  }

  if (subdocRootFrame) {
    aBuilder->LeavePresShell(subdocRootFrame, dirty);
  }

  if (ShouldClipSubdocument()) {
    nsDisplayClip* item =
      new (aBuilder) nsDisplayClip(aBuilder, this, &childItems,
                                   subdocBoundsInParentUnits);
    
    childItems.AppendToTop(item);
  }

  if (mIsInline) {
    WrapReplacedContentForBorderRadius(aBuilder, &childItems, aLists);
  } else {
    aLists.Content()->AppendToTop(&childItems);
  }

  
  childItems.DeleteAll();

  return rv;
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

  NS_ASSERTION(ObtainIntrinsicSizeFrame() == nsnull,
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

  NS_ASSERTION(ObtainIntrinsicSizeFrame() == nsnull,
               "Intrinsic height should come from the embedded document.");

  
  return nsPresContext::CSSPixelsToAppUnits(150);
}

#ifdef DEBUG
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
                                PRUint32 aFlags)
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

  
  
  nsPoint offset(0, 0);
  
  if (IsInline()) {
    
    nsresult rv = nsLeafFrame::DoReflow(aPresContext, aDesiredSize, aReflowState,
                                        aStatus);
    NS_ENSURE_SUCCESS(rv, rv);

    offset = nsPoint(aReflowState.mComputedBorderPadding.left,
                     aReflowState.mComputedBorderPadding.top);
  } else {
    
    SizeToAvailSize(aReflowState, aDesiredSize);
  }

  nsSize innerSize(aDesiredSize.width, aDesiredSize.height);
  if (IsInline()) {
    innerSize.width  -= aReflowState.mComputedBorderPadding.LeftRight();
    innerSize.height -= aReflowState.mComputedBorderPadding.TopBottom();
  }

  if (mInnerView) {
    nsIViewManager* vm = mInnerView->GetViewManager();
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

  
  CheckInvalidateSizeChange(aDesiredSize);

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
nsSubDocumentFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                     nsIAtom* aAttribute,
                                     PRInt32 aModType)
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
  else if (aAttribute == nsGkAtoms::type) {
    if (!mFrameLoader) 
      return NS_OK;

    if (!mContent->IsXUL()) {
      return NS_OK;
    }

    if (mFrameLoader->GetRemoteBrowser()) {
      
      return NS_OK;
    }

    
    

    
    
    

    nsCOMPtr<nsIDocShell> docShell;
    mFrameLoader->GetDocShell(getter_AddRefs(docShell));
    nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(docShell));
    if (!docShellAsItem) {
      return NS_OK;
    }

    nsCOMPtr<nsIDocShellTreeItem> parentItem;
    docShellAsItem->GetParent(getter_AddRefs(parentItem));
    if (!parentItem) {
      return NS_OK;
    }

    PRInt32 parentType;
    parentItem->GetItemType(&parentType);

    if (parentType != nsIDocShellTreeItem::typeChrome) {
      return NS_OK;
    }

    nsCOMPtr<nsIDocShellTreeOwner> parentTreeOwner;
    parentItem->GetTreeOwner(getter_AddRefs(parentTreeOwner));
    if (parentTreeOwner) {
      nsAutoString value;
      mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::type, value);

      bool is_primary = value.LowerCaseEqualsLiteral("content-primary");

#ifdef MOZ_XUL
      
      if (!is_primary) {
        nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
        if (pm)
          pm->HidePopupsInDocShell(docShellAsItem);
      }
#endif

      parentTreeOwner->ContentShellRemoved(docShellAsItem);

      if (value.LowerCaseEqualsLiteral("content") ||
          StringBeginsWith(value, NS_LITERAL_STRING("content-"),
                           nsCaseInsensitiveStringComparator())) {
        bool is_targetable = is_primary ||
          value.LowerCaseEqualsLiteral("content-targetable");

        parentTreeOwner->ContentShellAdded(docShellAsItem, is_primary,
                                           is_targetable, value);
      }
    }
  }

  return NS_OK;
}

nsIFrame*
NS_NewSubDocumentFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSubDocumentFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSubDocumentFrame)

void
nsSubDocumentFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  if (mPostedReflowCallback) {
    PresContext()->PresShell()->CancelReflowCallback(this);
    mPostedReflowCallback = false;
  }
  
  HideViewer();

  nsLeafFrame::DestroyFrom(aDestructRoot);
}

void
nsSubDocumentFrame::HideViewer()
{
  if (mFrameLoader && (mDidCreateDoc || mCallingShow))
    mFrameLoader->Hide();
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
    return nsnull;

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
  *aDocShell = nsnull;

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
  nsIFrame* rootFrame = shell ? shell->GetRootFrame() : nsnull;
  if (rootFrame) {
    ::DestroyDisplayItemDataForFrames(rootFrame);
  }
  aDocument->EnumerateFreezableElements(
    nsObjectFrame::BeginSwapDocShells, nsnull);
  aDocument->EnumerateSubDocuments(BeginSwapDocShellsForDocument, nsnull);
  return true;
}

static nsIView*
BeginSwapDocShellsForViews(nsIView* aSibling)
{
  
  nsIView* removedViews = nsnull;
  while (aSibling) {
    nsIDocument* doc = ::GetDocumentFromView(aSibling);
    if (doc) {
      ::BeginSwapDocShellsForDocument(doc, nsnull);
    }
    nsIView* next = aSibling->GetNextSibling();
    aSibling->GetViewManager()->RemoveChild(aSibling);
    aSibling->SetNextSibling(removedViews);
    removedViews = aSibling;
    aSibling = next;
  }
  return removedViews;
}

static void
InsertViewsInReverseOrder(nsIView* aSibling, nsIView* aParent)
{
  NS_PRECONDITION(aParent, "");
  NS_PRECONDITION(!aParent->GetFirstChild(), "inserting into non-empty list");

  nsIViewManager* vm = aParent->GetViewManager();
  while (aSibling) {
    nsIView* next = aSibling->GetNextSibling();
    aSibling->SetNextSibling(nsnull);
    
    
    vm->InsertChild(aParent, aSibling, nsnull, true);
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
    nsIView* ourSubdocViews = mInnerView->GetFirstChild();
    nsIView* ourRemovedViews = ::BeginSwapDocShellsForViews(ourSubdocViews);
    nsIView* otherSubdocViews = other->mInnerView->GetFirstChild();
    nsIView* otherRemovedViews = ::BeginSwapDocShellsForViews(otherSubdocViews);

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
      nsDeviceContext* dc = pc ? pc->DeviceContext() : nsnull;
      if (dc) {
        nsIView* v = cv->FindContainerView();
        dc->Init(v ? v->GetNearestWidget(nsnull) : nsnull);
      }
      nsCOMPtr<nsIContentViewer> prev;
      cv->GetPreviousViewer(getter_AddRefs(prev));
      cv = prev;
    }
  }

  aDocument->EnumerateFreezableElements(
    nsObjectFrame::EndSwapDocShells, nsnull);
  aDocument->EnumerateSubDocuments(EndSwapDocShellsForDocument, nsnull);
  return true;
}

static void
EndSwapDocShellsForViews(nsIView* aSibling)
{
  for ( ; aSibling; aSibling = aSibling->GetNextSibling()) {
    nsIDocument* doc = ::GetDocumentFromView(aSibling);
    if (doc) {
      ::EndSwapDocShellsForDocument(doc, nsnull);
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

nsIView*
nsSubDocumentFrame::EnsureInnerView()
{
  if (mInnerView) {
    return mInnerView;
  }

  
  nsIView* outerView = GetView();
  NS_ASSERTION(outerView, "Must have an outer view already");
  nsRect viewBounds(0, 0, 0, 0); 

  nsIViewManager* viewMan = outerView->GetViewManager();
  nsIView* innerView = viewMan->CreateView(viewBounds, outerView);
  if (!innerView) {
    NS_ERROR("Could not create inner view");
    return nsnull;
  }
  mInnerView = innerView;
  viewMan->InsertChild(outerView, innerView, nsnull, true);

  return mInnerView;
}

nsIFrame*
nsSubDocumentFrame::ObtainIntrinsicSizeFrame()
{
  nsCOMPtr<nsIObjectLoadingContent> olc = do_QueryInterface(GetContent());
  if (olc) {
    

    
    nsIFrame* subDocRoot = nsnull;

    nsCOMPtr<nsIDocShell> docShell;
    GetDocShell(getter_AddRefs(docShell));
    if (docShell) {
      nsCOMPtr<nsIPresShell> presShell;
      docShell->GetPresShell(getter_AddRefs(presShell));
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
  return nsnull;
}
