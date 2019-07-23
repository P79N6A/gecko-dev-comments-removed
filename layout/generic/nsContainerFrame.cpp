








































#include "nsContainerFrame.h"
#include "nsHTMLContainerFrame.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsIRenderingContext.h"
#include "nsStyleContext.h"
#include "nsRect.h"
#include "nsPoint.h"
#include "nsGUIEvent.h"
#include "nsStyleConsts.h"
#include "nsIView.h"
#include "nsHTMLContainerFrame.h"
#include "nsFrameManager.h"
#include "nsIPresShell.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsIViewManager.h"
#include "nsIWidget.h"
#include "nsGfxCIID.h"
#include "nsIServiceManager.h"
#include "nsCSSRendering.h"
#include "nsTransform2D.h"
#include "nsRegion.h"
#include "nsLayoutErrors.h"
#include "nsDisplayList.h"
#include "nsContentErrors.h"
#include "nsIEventStateManager.h"
#include "nsListControlFrame.h"
#include "nsIBaseWindow.h"
#include "nsThemeConstants.h"
#include "nsCSSFrameConstructor.h"
#include "nsThemeConstants.h"

#ifdef NS_DEBUG
#undef NOISY
#else
#undef NOISY
#endif

NS_IMPL_FRAMEARENA_HELPERS(nsContainerFrame)

nsContainerFrame::~nsContainerFrame()
{
}

NS_IMETHODIMP
nsContainerFrame::Init(nsIContent* aContent,
                       nsIFrame*   aParent,
                       nsIFrame*   aPrevInFlow)
{
  nsresult rv = nsSplittableFrame::Init(aContent, aParent, aPrevInFlow);
  if (aPrevInFlow) {
    
    
    
    if (aPrevInFlow->GetStateBits() & NS_FRAME_HAS_CHILD_WITH_VIEW)
      AddStateBits(NS_FRAME_HAS_CHILD_WITH_VIEW);
  }
  return rv;
}

NS_IMETHODIMP
nsContainerFrame::SetInitialChildList(nsIAtom*     aListName,
                                      nsFrameList& aChildList)
{
  nsresult  result;
  if (!mFrames.IsEmpty()) {
    
    
    NS_NOTREACHED("unexpected second call to SetInitialChildList");
    result = NS_ERROR_UNEXPECTED;
  } else if (aListName) {
    
    NS_NOTREACHED("unknown frame list");
    result = NS_ERROR_INVALID_ARG;
  } else {
#ifdef NS_DEBUG
    nsFrame::VerifyDirtyBitSet(aChildList);
#endif
    mFrames.SetFrames(aChildList);
    result = NS_OK;
  }
  return result;
}

NS_IMETHODIMP
nsContainerFrame::AppendFrames(nsIAtom*  aListName,
                               nsFrameList& aFrameList)
{
  if (nsnull != aListName) {
#ifdef IBMBIDI
    if (aListName != nsGkAtoms::nextBidi)
#endif
    {
      NS_ERROR("unexpected child list");
      return NS_ERROR_INVALID_ARG;
    }
  }
  if (aFrameList.NotEmpty()) {
    mFrames.AppendFrames(this, aFrameList);

    
#ifdef IBMBIDI
    if (nsnull == aListName)
#endif
    {
      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsContainerFrame::InsertFrames(nsIAtom*  aListName,
                               nsIFrame* aPrevFrame,
                               nsFrameList& aFrameList)
{
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  if (nsnull != aListName) {
#ifdef IBMBIDI
    if (aListName != nsGkAtoms::nextBidi)
#endif
    {
      NS_ERROR("unexpected child list");
      return NS_ERROR_INVALID_ARG;
    }
  }
  if (aFrameList.NotEmpty()) {
    
    mFrames.InsertFrames(this, aPrevFrame, aFrameList);

#ifdef IBMBIDI
    if (nsnull == aListName)
#endif
    {
      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsContainerFrame::RemoveFrame(nsIAtom*  aListName,
                              nsIFrame* aOldFrame)
{
  if (nsnull != aListName) {
#ifdef IBMBIDI
    if (nsGkAtoms::nextBidi != aListName)
#endif
    {
      NS_ERROR("unexpected child list");
      return NS_ERROR_INVALID_ARG;
    }
  }

  if (aOldFrame) {
    
    
    
    
    PRBool generateReflowCommand = PR_TRUE;
#ifdef IBMBIDI
    if (nsGkAtoms::nextBidi == aListName) {
      generateReflowCommand = PR_FALSE;
    }
#endif
    nsContainerFrame* parent = static_cast<nsContainerFrame*>(aOldFrame->GetParent());
    while (aOldFrame) {
      
      
      
      nsIFrame* oldFrameNextContinuation = aOldFrame->GetNextContinuation();
      
      
      
      
      if (parent == this) {
        if (!parent->mFrames.DestroyFrame(aOldFrame)) {
          
          
#ifdef DEBUG
          nsresult rv =
#endif
            StealFrame(PresContext(), aOldFrame, PR_TRUE);
          NS_ASSERTION(NS_SUCCEEDED(rv), "Could not find frame to remove!");
          aOldFrame->Destroy();
        }
      } else {
        
        
        parent->RemoveFrame(nsnull, aOldFrame);
        break;
      }
      aOldFrame = oldFrameNextContinuation;
      if (aOldFrame) {
        parent = static_cast<nsContainerFrame*>(aOldFrame->GetParent());
      }
    }

    if (generateReflowCommand) {
      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }

  return NS_OK;
}

void
nsContainerFrame::Destroy()
{
  
  if (HasView()) {
    GetView()->SetClientData(nsnull);
  }

  
  mFrames.DestroyFrames();

  
  nsPresContext* prescontext = PresContext();

  DestroyOverflowList(prescontext);

  if (IsFrameOfType(nsIFrame::eCanContainOverflowContainers)) {
    nsFrameList* frameList = RemovePropTableFrames(prescontext,
                               nsGkAtoms::overflowContainersProperty);
    if (frameList)
      frameList->Destroy();

    frameList = RemovePropTableFrames(prescontext,
                  nsGkAtoms::excessOverflowContainersProperty);
    if (frameList)
      frameList->Destroy();
  }

  if (IsGeneratedContentFrame()) {
    
    
    
    
    nsCOMArray<nsIContent>* generatedContent =
      static_cast<nsCOMArray<nsIContent>*>(
        UnsetProperty(nsGkAtoms::generatedContent));

    if (generatedContent) {
      for (int i = generatedContent->Count() - 1; i >= 0; --i) {
        nsIContent* content = generatedContent->ObjectAt(i);
        
        
        PresContext()->EventStateManager()->
          ContentRemoved(content->GetCurrentDoc(), content);
        content->UnbindFromTree();
      }
      delete generatedContent;
    }
  }

  
  nsSplittableFrame::Destroy();
}




nsFrameList
nsContainerFrame::GetChildList(nsIAtom* aListName) const
{
  
  
  if (nsnull == aListName) {
    return mFrames;
  }

  if (nsGkAtoms::overflowList == aListName) {
    nsFrameList* frameList = GetOverflowFrames();
    return frameList ? *frameList : nsFrameList::EmptyList();
  }

  if (nsGkAtoms::overflowContainersList == aListName) {
    nsFrameList* list = GetPropTableFrames(PresContext(),
                          nsGkAtoms::overflowContainersProperty);
    return list ? *list : nsFrameList::EmptyList();
  }

  if (nsGkAtoms::excessOverflowContainersList == aListName) {
    nsFrameList* list = GetPropTableFrames(PresContext(),
                          nsGkAtoms::excessOverflowContainersProperty);
    return list ? *list : nsFrameList::EmptyList();

  }

  return nsFrameList::EmptyList();
}

#define NS_CONTAINER_FRAME_OVERFLOW_LIST_INDEX                   0
#define NS_CONTAINER_FRAME_OVERFLOW_CONTAINERS_LIST_INDEX        1
#define NS_CONTAINER_FRAME_EXCESS_OVERFLOW_CONTAINERS_LIST_INDEX 2



nsIAtom*
nsContainerFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  if (NS_CONTAINER_FRAME_OVERFLOW_LIST_INDEX == aIndex)
    return nsGkAtoms::overflowList;
  else if (IsFrameOfType(nsIFrame::eCanContainOverflowContainers)) {
    if (NS_CONTAINER_FRAME_OVERFLOW_CONTAINERS_LIST_INDEX == aIndex)
      return nsGkAtoms::overflowContainersList;
    else if (NS_CONTAINER_FRAME_EXCESS_OVERFLOW_CONTAINERS_LIST_INDEX == aIndex)
      return nsGkAtoms::excessOverflowContainersList;
  }
  return nsnull;
}




NS_IMETHODIMP
nsContainerFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                   const nsRect&           aDirtyRect,
                                   const nsDisplayListSet& aLists)
{
  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  return BuildDisplayListForNonBlockChildren(aBuilder, aDirtyRect, aLists);
}

nsresult
nsContainerFrame::BuildDisplayListForNonBlockChildren(nsDisplayListBuilder*   aBuilder,
                                                      const nsRect&           aDirtyRect,
                                                      const nsDisplayListSet& aLists,
                                                      PRUint32                aFlags)
{
  nsIFrame* kid = mFrames.FirstChild();
  
  nsDisplayListSet set(aLists, aLists.Content());
  
  while (kid) {
    nsresult rv = BuildDisplayListForChild(aBuilder, kid, aDirtyRect, set, aFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    kid = kid->GetNextSibling();
  }
  return NS_OK;
}

 void
nsContainerFrame::ChildIsDirty(nsIFrame* aChild)
{
  AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
}

PRBool
nsContainerFrame::IsLeaf() const
{
  return PR_FALSE;
}

PRBool
nsContainerFrame::PeekOffsetNoAmount(PRBool aForward, PRInt32* aOffset)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  
  return PR_FALSE;
}

PRBool
nsContainerFrame::PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  
  return PR_FALSE;
}









void
nsContainerFrame::PositionFrameView(nsIFrame* aKidFrame)
{
  nsIFrame* parentFrame = aKidFrame->GetParent();
  if (!aKidFrame->HasView() || !parentFrame)
    return;

  nsIView* view = aKidFrame->GetView();
  nsIViewManager* vm = view->GetViewManager();
  nsPoint pt;
  nsIView* ancestorView = parentFrame->GetClosestView(&pt);

  if (ancestorView != view->GetParent()) {
    NS_ASSERTION(ancestorView == view->GetParent()->GetParent(),
                 "Allowed only one anonymous view between frames");
    
    
    return;
  }

  pt += aKidFrame->GetPosition();
  vm->MoveViewTo(view, pt.x, pt.y);
}

static PRBool
IsMenuPopup(nsIFrame *aFrame)
{
  nsIAtom *frameType = aFrame->GetType();

  
  if (frameType == nsGkAtoms::listControlFrame) {
    nsListControlFrame *listControlFrame = static_cast<nsListControlFrame*>(aFrame);
      
    if (listControlFrame) {
      return listControlFrame->IsInDropDownMode();
    }
  }

  
  return (frameType == nsGkAtoms::menuPopupFrame);
}

static nsIWidget*
GetPresContextContainerWidget(nsPresContext* aPresContext)
{
  nsCOMPtr<nsISupports> container = aPresContext->Document()->GetContainer();
  nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(container);
  if (!baseWindow)
    return nsnull;

  nsCOMPtr<nsIWidget> mainWidget;
  baseWindow->GetMainWidget(getter_AddRefs(mainWidget));
  return mainWidget;
}

static PRBool
IsTopLevelWidget(nsIWidget* aWidget)
{
  nsWindowType windowType;
  aWidget->GetWindowType(windowType);
  return windowType == eWindowType_toplevel ||
         windowType == eWindowType_dialog || 
         windowType == eWindowType_sheet;
  
}

void
nsContainerFrame::SyncWindowProperties(nsPresContext*       aPresContext,
                                       nsIFrame*            aFrame,
                                       nsIView*             aView)
{
#ifdef MOZ_XUL
  if (!aView || !nsCSSRendering::IsCanvasFrame(aFrame) || !aView->HasWidget())
    return;

  nsIWidget* windowWidget = GetPresContextContainerWidget(aPresContext);
  if (!windowWidget || !IsTopLevelWidget(windowWidget))
    return;

  nsIViewManager* vm = aView->GetViewManager();
  nsIView* rootView;
  vm->GetRootView(rootView);

  if (aView != rootView)
    return;

  nsIContent* rootContent = aPresContext->Document()->GetRootContent();
  if (!rootContent || !rootContent->IsNodeOfType(nsINode::eXUL)) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    return;
  }

  nsIFrame *rootFrame = aPresContext->PresShell()->FrameConstructor()->GetRootElementStyleFrame();
  if (!rootFrame)
    return;

  nsTransparencyMode mode = nsLayoutUtils::GetFrameTransparency(aFrame, rootFrame);
  nsIWidget* viewWidget = aView->GetWidget();
  viewWidget->SetTransparencyMode(mode);
  windowWidget->SetWindowShadowStyle(rootFrame->GetStyleUIReset()->mWindowShadow);
#endif
}

void
nsContainerFrame::SyncFrameViewAfterReflow(nsPresContext* aPresContext,
                                           nsIFrame*       aFrame,
                                           nsIView*        aView,
                                           const nsRect*   aCombinedArea,
                                           PRUint32        aFlags)
{
  if (!aView) {
    return;
  }

  NS_ASSERTION(aCombinedArea, "Combined area must be passed in now");

  
  if (0 == (aFlags & NS_FRAME_NO_MOVE_VIEW)) {
    PositionFrameView(aFrame);
  }

  if (0 == (aFlags & NS_FRAME_NO_SIZE_VIEW)) {
    nsIViewManager* vm = aView->GetViewManager();

    vm->ResizeView(aView, *aCombinedArea, PR_TRUE);
  }
}

void
nsContainerFrame::SyncFrameViewProperties(nsPresContext*  aPresContext,
                                          nsIFrame*        aFrame,
                                          nsStyleContext*  aStyleContext,
                                          nsIView*         aView,
                                          PRUint32         aFlags)
{
  NS_ASSERTION(!aStyleContext || aFrame->GetStyleContext() == aStyleContext,
               "Wrong style context for frame?");

  if (!aView) {
    return;
  }

  nsIViewManager* vm = aView->GetViewManager();
 
  


  if (aFrame->GetStyleDisplay()->HasTransform())
    aView->SetInvalidateFrameOnScroll();

  if (nsnull == aStyleContext) {
    aStyleContext = aFrame->GetStyleContext();
  }

  
  if (0 == (aFlags & NS_FRAME_NO_VISIBILITY) &&
      !aFrame->SupportsVisibilityHidden()) {
    
    vm->SetViewVisibility(aView,
        aStyleContext->GetStyleVisibility()->IsVisible()
            ? nsViewVisibility_kShow : nsViewVisibility_kHide);
  }

  
  
  PRBool isPositioned = aStyleContext->GetStyleDisplay()->IsPositioned();

  PRInt32 zIndex = 0;
  PRBool  autoZIndex = PR_FALSE;

  if (!isPositioned) {
    autoZIndex = PR_TRUE;
  } else {
    
    const nsStylePosition* position = aStyleContext->GetStylePosition();

    if (position->mZIndex.GetUnit() == eStyleUnit_Integer) {
      zIndex = position->mZIndex.GetIntValue();
    } else if (position->mZIndex.GetUnit() == eStyleUnit_Auto) {
      autoZIndex = PR_TRUE;
    }
  }

  vm->SetViewZIndex(aView, autoZIndex, zIndex, isPositioned);
}

PRBool
nsContainerFrame::FrameNeedsView(nsIFrame* aFrame)
{
  
  
  if (aFrame->GetStyleContext()->GetPseudoType() ==
      nsCSSAnonBoxes::scrolledContent) {
    return PR_TRUE;
  }
  return aFrame->NeedsView() || aFrame->GetStyleDisplay()->HasTransform();
}

static nscoord GetCoord(const nsStyleCoord& aCoord, nscoord aIfNotCoord)
{
  return aCoord.GetUnit() == eStyleUnit_Coord
           ? aCoord.GetCoordValue()
           : aIfNotCoord;
}

void
nsContainerFrame::DoInlineIntrinsicWidth(nsIRenderingContext *aRenderingContext,
                                         InlineIntrinsicWidthData *aData,
                                         nsLayoutUtils::IntrinsicWidthType aType)
{
  if (GetPrevInFlow())
    return; 

  NS_PRECONDITION(aType == nsLayoutUtils::MIN_WIDTH ||
                  aType == nsLayoutUtils::PREF_WIDTH, "bad type");

  PRUint8 startSide, endSide;
  if (GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_LTR) {
    startSide = NS_SIDE_LEFT;
    endSide = NS_SIDE_RIGHT;
  } else {
    startSide = NS_SIDE_RIGHT;
    endSide = NS_SIDE_LEFT;
  }

  const nsStylePadding *stylePadding = GetStylePadding();
  const nsStyleBorder *styleBorder = GetStyleBorder();
  const nsStyleMargin *styleMargin = GetStyleMargin();

  
  
  
  
  
  
  
  if (!GetPrevContinuation()) {
    aData->currentLine +=
      GetCoord(stylePadding->mPadding.Get(startSide), 0) +
      styleBorder->GetActualBorderWidth(startSide) +
      GetCoord(styleMargin->mMargin.Get(startSide), 0);
  }

  const nsLineList_iterator* savedLine = aData->line;
  nsIFrame* const savedLineContainer = aData->lineContainer;

  nsContainerFrame *lastInFlow;
  for (nsContainerFrame *nif = this; nif;
       nif = static_cast<nsContainerFrame*>(nif->GetNextInFlow())) {
    for (nsIFrame *kid = nif->mFrames.FirstChild(); kid;
         kid = kid->GetNextSibling()) {
      if (aType == nsLayoutUtils::MIN_WIDTH)
        kid->AddInlineMinWidth(aRenderingContext,
                               static_cast<InlineMinWidthData*>(aData));
      else
        kid->AddInlinePrefWidth(aRenderingContext,
                                static_cast<InlinePrefWidthData*>(aData));
    }
    
    
    
    aData->line = nsnull;
    aData->lineContainer = nsnull;

    lastInFlow = nif;
  }
  
  aData->line = savedLine;
  aData->lineContainer = savedLineContainer;

  
  
  
  
  
  
  
  if (!lastInFlow->GetNextContinuation()) {
    aData->currentLine +=
      GetCoord(stylePadding->mPadding.Get(endSide), 0) +
      styleBorder->GetActualBorderWidth(endSide) +
      GetCoord(styleMargin->mMargin.Get(endSide), 0);
  }
}

 nsSize
nsContainerFrame::ComputeAutoSize(nsIRenderingContext *aRenderingContext,
                                  nsSize aCBSize, nscoord aAvailableWidth,
                                  nsSize aMargin, nsSize aBorder,
                                  nsSize aPadding, PRBool aShrinkWrap)
{
  nsSize result(0xdeadbeef, NS_UNCONSTRAINEDSIZE);
  nscoord availBased = aAvailableWidth - aMargin.width - aBorder.width -
                       aPadding.width;
  
  if (aShrinkWrap || IsFrameOfType(eReplaced)) {
    
    if (GetStylePosition()->mWidth.GetUnit() == eStyleUnit_Auto) {
      result.width = ShrinkWidthToFit(aRenderingContext, availBased);
    }
  } else {
    result.width = availBased;
  }
  return result;
}






nsresult
nsContainerFrame::ReflowChild(nsIFrame*                aKidFrame,
                              nsPresContext*           aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              nscoord                  aX,
                              nscoord                  aY,
                              PRUint32                 aFlags,
                              nsReflowStatus&          aStatus,
                              nsOverflowContinuationTracker* aTracker)
{
  NS_PRECONDITION(aReflowState.frame == aKidFrame, "bad reflow state");

  nsresult  result;

  
  
  aKidFrame->WillReflow(aPresContext);

  if (NS_FRAME_NO_MOVE_FRAME != (aFlags & NS_FRAME_NO_MOVE_FRAME)) {
    if ((aFlags & NS_FRAME_INVALIDATE_ON_MOVE) &&
        !(aKidFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) &&
        aKidFrame->GetPosition() != nsPoint(aX, aY)) {
      aKidFrame->InvalidateOverflowRect();
    }
    aKidFrame->SetPosition(nsPoint(aX, aY));
  }

  if (0 == (aFlags & NS_FRAME_NO_MOVE_VIEW)) {
    PositionFrameView(aKidFrame);
  }

  
  result = aKidFrame->Reflow(aPresContext, aDesiredSize, aReflowState,
                             aStatus);

  
  
  if (NS_SUCCEEDED(result) && NS_FRAME_IS_FULLY_COMPLETE(aStatus)) {
    nsIFrame* kidNextInFlow = aKidFrame->GetNextInFlow();
    if (nsnull != kidNextInFlow) {
      
      
      
      if (aTracker) aTracker->Finish(aKidFrame);
      static_cast<nsContainerFrame*>(kidNextInFlow->GetParent())
        ->DeleteNextInFlowChild(aPresContext, kidNextInFlow, PR_TRUE);
    }
  }
  return result;
}






void
nsContainerFrame::PositionChildViews(nsIFrame* aFrame)
{
  if (!(aFrame->GetStateBits() & NS_FRAME_HAS_CHILD_WITH_VIEW)) {
    return;
  }

  nsIAtom*  childListName = nsnull;
  PRInt32   childListIndex = 0;

  do {
    
    nsIFrame* childFrame = aFrame->GetFirstChild(childListName);
    while (childFrame) {
      
      
      if (childFrame->HasView()) {
        PositionFrameView(childFrame);
      } else {
        PositionChildViews(childFrame);
      }

      
      childFrame = childFrame->GetNextSibling();
    }

    
    
    
    do {
      childListName = aFrame->GetAdditionalChildListName(childListIndex++);
    } while (childListName == nsGkAtoms::popupList);
  } while (childListName);
}


















nsresult
nsContainerFrame::FinishReflowChild(nsIFrame*                  aKidFrame,
                                    nsPresContext*             aPresContext,
                                    const nsHTMLReflowState*   aReflowState,
                                    const nsHTMLReflowMetrics& aDesiredSize,
                                    nscoord                    aX,
                                    nscoord                    aY,
                                    PRUint32                   aFlags)
{
  nsPoint curOrigin = aKidFrame->GetPosition();
  nsRect  bounds(aX, aY, aDesiredSize.width, aDesiredSize.height);

  aKidFrame->SetRect(bounds);

  if (aKidFrame->HasView()) {
    nsIView* view = aKidFrame->GetView();
    
    
    SyncFrameViewAfterReflow(aPresContext, aKidFrame, view,
                             &aDesiredSize.mOverflowArea,
                             aFlags);
  }

  if (!(aFlags & NS_FRAME_NO_MOVE_VIEW) &&
      (curOrigin.x != aX || curOrigin.y != aY)) {
    if (!aKidFrame->HasView()) {
      
      
      PositionChildViews(aKidFrame);
    }

    
    
    
    
    
    aKidFrame->Invalidate(aDesiredSize.mOverflowArea);
  }
  
  return aKidFrame->DidReflow(aPresContext, aReflowState, NS_FRAME_REFLOW_FINISHED);
}

nsresult
nsContainerFrame::ReflowOverflowContainerChildren(nsPresContext*           aPresContext,
                                                  const nsHTMLReflowState& aReflowState,
                                                  nsRect&                  aOverflowRect,
                                                  PRUint32                 aFlags,
                                                  nsReflowStatus&          aStatus)
{
  NS_PRECONDITION(aPresContext, "null pointer");
  nsresult rv = NS_OK;

  nsFrameList* overflowContainers =
               GetPropTableFrames(aPresContext,
                                  nsGkAtoms::overflowContainersProperty);

  NS_ASSERTION(!(overflowContainers && GetPrevInFlow()
                 && static_cast<nsContainerFrame*>(GetPrevInFlow())
                      ->GetPropTableFrames(aPresContext,
                          nsGkAtoms::excessOverflowContainersProperty)),
               "conflicting overflow containers lists");

  if (!overflowContainers) {
    
    nsContainerFrame* prev = (nsContainerFrame*) GetPrevInFlow();
    if (prev) {
      nsFrameList* excessFrames =
        prev->RemovePropTableFrames(aPresContext,
                nsGkAtoms::excessOverflowContainersProperty);
      if (excessFrames) {
        nsFrameList reparenter;
        reparenter.InsertFrames(this, nsnull, excessFrames->FirstChild());
        nsHTMLContainerFrame::ReparentFrameViewList(aPresContext,
                                                    excessFrames->FirstChild(),
                                                    prev, this);
        overflowContainers = excessFrames;
        rv = SetPropTableFrames(aPresContext, overflowContainers,
                                nsGkAtoms::overflowContainersProperty);
        if (NS_FAILED(rv)) {
          excessFrames->DestroyFrames();
          delete excessFrames;
          return rv;
        }
      }
    }
  }

  if (!overflowContainers)
    return NS_OK; 

  nsOverflowContinuationTracker tracker(aPresContext, this, PR_FALSE, PR_FALSE);
  for (nsIFrame* frame = overflowContainers->FirstChild(); frame;
       frame = frame->GetNextSibling()) {
    if (frame->GetPrevInFlow()->GetParent() != GetPrevInFlow()) {
      
      
      continue;
    }
    if (NS_SUBTREE_DIRTY(frame)) {
      
      nsIFrame* prevInFlow = frame->GetPrevInFlow();
      NS_ASSERTION(prevInFlow,
                   "overflow container frame must have a prev-in-flow");
      NS_ASSERTION(frame->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER,
                   "overflow container frame must have overflow container bit set");
      nsRect prevRect = prevInFlow->GetRect();

      
      nsSize availSpace(prevRect.width, aReflowState.availableHeight);
      nsHTMLReflowMetrics desiredSize;
      nsHTMLReflowState frameState(aPresContext, aReflowState,
                                   frame, availSpace);
      nsReflowStatus frameStatus = NS_FRAME_COMPLETE;

      
      nsRect oldRect = frame->GetRect();
      nsRect oldOverflow = frame->GetOverflowRect();

      
      rv = ReflowChild(frame, aPresContext, desiredSize, frameState,
                       prevRect.x, 0, aFlags, frameStatus, &tracker);
      NS_ENSURE_SUCCESS(rv, rv);
      
      
      rv = FinishReflowChild(frame, aPresContext, &frameState, desiredSize,
                             prevRect.x, 0, aFlags);
      NS_ENSURE_SUCCESS(rv, rv);

      
      nsRect rect = frame->GetRect();
      if (rect != oldRect) {
        nsRect dirtyRect = oldOverflow;
        dirtyRect.MoveBy(oldRect.x, oldRect.y);
        Invalidate(dirtyRect);

        dirtyRect = frame->GetOverflowRect();
        dirtyRect.MoveBy(rect.x, rect.y);
        Invalidate(dirtyRect);
      }

      
      if (!NS_FRAME_IS_FULLY_COMPLETE(frameStatus)) {
        if (frame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
          
          
          NS_FRAME_SET_OVERFLOW_INCOMPLETE(frameStatus);
        }
        else {
          NS_ASSERTION(NS_FRAME_IS_COMPLETE(frameStatus),
                       "overflow container frames can't be incomplete, only overflow-incomplete");
        }

        
        nsIFrame* nif = frame->GetNextInFlow();
        if (!nif) {
          rv = nsHTMLContainerFrame::CreateNextInFlow(aPresContext, this,
                                                      frame, nif);
          NS_ENSURE_SUCCESS(rv, rv);
          NS_ASSERTION(frameStatus & NS_FRAME_REFLOW_NEXTINFLOW,
                       "Someone forgot a REFLOW_NEXTINFLOW flag");
          frame->SetNextSibling(nif->GetNextSibling());
          nif->SetNextSibling(nsnull);
        }
        else if (!(nif->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)) {
          
          rv = static_cast<nsContainerFrame*>(nif->GetParent())
                 ->StealFrame(aPresContext, nif);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        tracker.Insert(nif, frameStatus);
      }
      NS_MergeReflowStatusInto(&aStatus, frameStatus);
      
      
      
    }
    else {
      tracker.Skip(frame, aStatus);
      if (aReflowState.mFloatManager)
        nsBlockFrame::RecoverFloatsFor(frame, *aReflowState.mFloatManager);
    }
    ConsiderChildOverflow(aOverflowRect, frame);
  }

  return NS_OK;
}

void
nsContainerFrame::DisplayOverflowContainers(nsDisplayListBuilder*   aBuilder,
                                            const nsRect&           aDirtyRect,
                                            const nsDisplayListSet& aLists)
{
  nsFrameList* overflowconts = GetPropTableFrames(PresContext(),
                                 nsGkAtoms::overflowContainersProperty);
  if (overflowconts) {
    for (nsIFrame* frame = overflowconts->FirstChild(); frame;
         frame = frame->GetNextSibling()) {
      BuildDisplayListForChild(aBuilder, frame, aDirtyRect, aLists);
    }
  }
}

nsresult
nsContainerFrame::StealFrame(nsPresContext* aPresContext,
                             nsIFrame*      aChild,
                             PRBool         aForceNormal)
{
  PRBool removed = PR_TRUE;
  if ((aChild->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)
      && !aForceNormal) {
    
    if (!RemovePropTableFrame(aPresContext, aChild,
                              nsGkAtoms::overflowContainersProperty)) {
      
      removed = RemovePropTableFrame(aPresContext, aChild,
                  nsGkAtoms::excessOverflowContainersProperty);
    }
  }
  else {
    if (!mFrames.RemoveFrame(aChild)) {
      
      
      nsFrameList* frameList = GetOverflowFrames();
      if (frameList) {
        removed = frameList->RemoveFrame(aChild);
        if (frameList->IsEmpty()) {
          DestroyOverflowList(aPresContext);
        }
      }
    }
  }
  return (removed) ? NS_OK : NS_ERROR_UNEXPECTED;
}

void
nsContainerFrame::DestroyOverflowList(nsPresContext* aPresContext)
{
  nsFrameList* list =
    RemovePropTableFrames(aPresContext, nsGkAtoms::overflowProperty);
  if (list)
    list->Destroy();
}





void
nsContainerFrame::DeleteNextInFlowChild(nsPresContext* aPresContext,
                                        nsIFrame*      aNextInFlow,
                                        PRBool         aDeletingEmptyFrames)
{
#ifdef DEBUG
  nsIFrame* prevInFlow = aNextInFlow->GetPrevInFlow();
#endif
  NS_PRECONDITION(prevInFlow, "bad prev-in-flow");

  
  
  
  
  nsIFrame* nextNextInFlow = aNextInFlow->GetNextInFlow();
  if (nextNextInFlow) {
    nsAutoTArray<nsIFrame*, 8> frames;
    for (nsIFrame* f = nextNextInFlow; f; f = f->GetNextInFlow()) {
      frames.AppendElement(f);
    }
    for (PRInt32 i = frames.Length() - 1; i >= 0; --i) {
      nsIFrame* delFrame = frames.ElementAt(i);
      static_cast<nsContainerFrame*>(delFrame->GetParent())
        ->DeleteNextInFlowChild(aPresContext, delFrame, aDeletingEmptyFrames);
    }
  }

  aNextInFlow->Invalidate(aNextInFlow->GetOverflowRect());

  
#ifdef DEBUG
  nsresult rv = 
#endif
    StealFrame(aPresContext, aNextInFlow);
  NS_ASSERTION(NS_SUCCEEDED(rv), "StealFrame failure");

  
  
  aNextInFlow->Destroy();

  NS_POSTCONDITION(!prevInFlow->GetNextInFlow(), "non null next-in-flow");
}


static void
DestroyFrameList(void*           aFrame,
                 nsIAtom*        aPropertyName,
                 void*           aPropertyValue,
                 void*           aDtorData)
{
  if (aPropertyValue)
    static_cast<nsFrameList*>(aPropertyValue)->Destroy();
}




nsresult
nsContainerFrame::SetOverflowFrames(nsPresContext* aPresContext,
                                    const nsFrameList& aOverflowFrames)
{
  NS_PRECONDITION(aOverflowFrames.NotEmpty(), "Shouldn't be called");
  nsFrameList* newList = new nsFrameList(aOverflowFrames);
  if (!newList) {
    
    
    
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv =
    aPresContext->PropertyTable()->SetProperty(this,
                                               nsGkAtoms::overflowProperty,
                                               newList,
                                               DestroyFrameList,
                                               nsnull);
  if (NS_FAILED(rv)) {
    newList->Destroy();
  }

  
  NS_ASSERTION(rv != NS_PROPTABLE_PROP_OVERWRITTEN, "existing overflow list");

  return rv;
}

nsFrameList*
nsContainerFrame::GetPropTableFrames(nsPresContext*  aPresContext,
                                     nsIAtom*        aPropID) const
{
  nsPropertyTable* propTable = aPresContext->PropertyTable();
  return static_cast<nsFrameList*>(propTable->GetProperty(this, aPropID));
}

nsFrameList*
nsContainerFrame::RemovePropTableFrames(nsPresContext*  aPresContext,
                                        nsIAtom*        aPropID) const
{
  nsPropertyTable* propTable = aPresContext->PropertyTable();
  return static_cast<nsFrameList*>(propTable->UnsetProperty(this, aPropID));
}

PRBool
nsContainerFrame::RemovePropTableFrame(nsPresContext*  aPresContext,
                                       nsIFrame*       aFrame,
                                       nsIAtom*        aPropID) const
{
  nsFrameList* frameList = RemovePropTableFrames(aPresContext, aPropID);
  if (!frameList) {
    
    return PR_FALSE;
  }
  if (!frameList->RemoveFrame(aFrame)) {
    
    SetPropTableFrames(aPresContext, frameList, aPropID);
    return PR_FALSE;
  }

  if (frameList->IsEmpty()) {
    
    delete frameList;
  }
  else {
    
    SetPropTableFrames(aPresContext, frameList, aPropID);
  }
  return PR_TRUE;
}

nsresult
nsContainerFrame::SetPropTableFrames(nsPresContext*  aPresContext,
                                     nsFrameList*    aFrameList,
                                     nsIAtom*        aPropID) const
{
  NS_PRECONDITION(aPresContext && aPropID && aFrameList, "null ptr");
  nsresult rv = aPresContext->PropertyTable()->SetProperty(this, aPropID,
                  aFrameList, DestroyFrameList, nsnull);
  NS_ASSERTION(rv != NS_PROPTABLE_PROP_OVERWRITTEN, "existing framelist");
  return rv;
}















void
nsContainerFrame::PushChildren(nsPresContext* aPresContext,
                               nsIFrame*       aFromChild,
                               nsIFrame*       aPrevSibling)
{
  NS_PRECONDITION(nsnull != aFromChild, "null pointer");
  NS_PRECONDITION(nsnull != aPrevSibling, "pushing first child");
  NS_PRECONDITION(aPrevSibling->GetNextSibling() == aFromChild, "bad prev sibling");

  
  aPrevSibling->SetNextSibling(nsnull);

  if (nsnull != GetNextInFlow()) {
    
    
    
    nsContainerFrame* nextInFlow = (nsContainerFrame*)GetNextInFlow();
    
    
    for (nsIFrame* f = aFromChild; f; f = f->GetNextSibling()) {
      nsHTMLContainerFrame::ReparentFrameView(aPresContext, f, this, nextInFlow);
    }
    nextInFlow->mFrames.InsertFrames(nextInFlow, nsnull, aFromChild);
  }
  else {
    
    SetOverflowFrames(aPresContext, aFromChild);
  }
}









PRBool
nsContainerFrame::MoveOverflowToChildList(nsPresContext* aPresContext)
{
  PRBool result = PR_FALSE;

  
  nsContainerFrame* prevInFlow = (nsContainerFrame*)GetPrevInFlow();
  if (nsnull != prevInFlow) {
    nsAutoPtr<nsFrameList> prevOverflowFrames(prevInFlow->StealOverflowFrames());
    if (prevOverflowFrames) {
      
      
      NS_ASSERTION(mFrames.IsEmpty() || GetType() == nsGkAtoms::tableFrame,
                   "bad overflow list");
      
      
      nsHTMLContainerFrame::ReparentFrameViewList(aPresContext,
                                                  *prevOverflowFrames,
                                                  prevInFlow, this);
      mFrames.AppendFrames(this, *prevOverflowFrames);
      result = PR_TRUE;
    }
  }

  
  nsAutoPtr<nsFrameList> overflowFrames(StealOverflowFrames());
  if (overflowFrames) {
    NS_ASSERTION(mFrames.NotEmpty(), "overflow list w/o frames");
    mFrames.AppendFrames(nsnull, *overflowFrames);
    result = PR_TRUE;
  }
  return result;
}

nsOverflowContinuationTracker::nsOverflowContinuationTracker(nsPresContext*    aPresContext,
                                                             nsContainerFrame* aFrame,
                                                             PRBool            aWalkOOFFrames,
                                                             PRBool            aSkipOverflowContainerChildren)
  : mOverflowContList(nsnull),
    mPrevOverflowCont(nsnull),
    mSentry(nsnull),
    mParent(aFrame),
    mSkipOverflowContainerChildren(aSkipOverflowContainerChildren),
    mWalkOOFFrames(aWalkOOFFrames)
{
  NS_PRECONDITION(aFrame, "null frame pointer");
  nsContainerFrame* next = static_cast<nsContainerFrame*>
                             (aFrame->GetNextInFlow());
  if (next) {
    mOverflowContList =
      next->GetPropTableFrames(aPresContext,
                               nsGkAtoms::overflowContainersProperty);
    if (mOverflowContList) {
      mParent = next;
      SetUpListWalker();
    }
  }
  if (!mOverflowContList) {
    mOverflowContList =
      mParent->GetPropTableFrames(aPresContext,
                                  nsGkAtoms::excessOverflowContainersProperty);
    if (mOverflowContList) {
      SetUpListWalker();
    }
  }
}





void
nsOverflowContinuationTracker::SetUpListWalker()
{
  NS_ASSERTION(!mSentry && !mPrevOverflowCont,
               "forgot to reset mSentry or mPrevOverflowCont");
  if (mOverflowContList) {
    nsIFrame* cur = mOverflowContList->FirstChild();
    if (mSkipOverflowContainerChildren) {
      while (cur && (cur->GetPrevInFlow()->GetStateBits()
                     & NS_FRAME_IS_OVERFLOW_CONTAINER)) {
        mPrevOverflowCont = cur;
        cur = cur->GetNextSibling();
      }
      while (cur && (!(cur->GetStateBits() & NS_FRAME_OUT_OF_FLOW)
                     == mWalkOOFFrames)) {
        mPrevOverflowCont = cur;
        cur = cur->GetNextSibling();
      }
    }
    if (cur) {
      mSentry = cur->GetPrevInFlow();
    }
  }
}







void
nsOverflowContinuationTracker::StepForward()
{
  NS_PRECONDITION(mOverflowContList, "null list");

  
  if (mPrevOverflowCont) {
    mPrevOverflowCont = mPrevOverflowCont->GetNextSibling();
  }
  else {
    mPrevOverflowCont = mOverflowContList->FirstChild();
  }

  
  if (mSkipOverflowContainerChildren) {
    nsIFrame* cur = mPrevOverflowCont->GetNextSibling();
    while (cur && (!(cur->GetStateBits() & NS_FRAME_OUT_OF_FLOW)
                   == mWalkOOFFrames)) {
      mPrevOverflowCont = cur;
      cur = cur->GetNextSibling();
    }
  }

  
  mSentry = (mPrevOverflowCont->GetNextSibling())
            ? mPrevOverflowCont->GetNextSibling()->GetPrevInFlow()
            : nsnull;
}

nsresult
nsOverflowContinuationTracker::Insert(nsIFrame*       aOverflowCont,
                                      nsReflowStatus& aReflowStatus)
{
  NS_PRECONDITION(aOverflowCont, "null frame pointer");
  NS_PRECONDITION(!mSkipOverflowContainerChildren || mWalkOOFFrames ==
                  !!(aOverflowCont->GetStateBits() & NS_FRAME_OUT_OF_FLOW),
                  "shouldn't insert frame that doesn't match walker type");
  NS_PRECONDITION(aOverflowCont->GetPrevInFlow(),
                  "overflow containers must have a prev-in-flow");
  nsresult rv = NS_OK;
  PRBool convertedToOverflowContainer = PR_FALSE;
  nsPresContext* presContext = aOverflowCont->PresContext();
  if (!mSentry || aOverflowCont != mSentry->GetNextInFlow()) {
    
    if (aOverflowCont->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER) {
      
      
      NS_ASSERTION(!(mOverflowContList &&
                     mOverflowContList->ContainsFrame(aOverflowCont)),
                   "overflow containers out of order");
      rv = static_cast<nsContainerFrame*>(aOverflowCont->GetParent())
             ->StealFrame(presContext, aOverflowCont);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      aOverflowCont->AddStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
      convertedToOverflowContainer = PR_TRUE;
    }
    if (!mOverflowContList) {
      mOverflowContList = new nsFrameList();
      rv = mParent->SetPropTableFrames(presContext,
             mOverflowContList, nsGkAtoms::excessOverflowContainersProperty);
      NS_ENSURE_SUCCESS(rv, rv);
      SetUpListWalker();
    }
    if (aOverflowCont->GetParent() != mParent) {
      nsHTMLContainerFrame::ReparentFrameView(presContext, aOverflowCont,
                                              aOverflowCont->GetParent(),
                                              mParent);
    }
    mOverflowContList->InsertFrame(mParent, mPrevOverflowCont, aOverflowCont);
    aReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
  }

  
  if (aReflowStatus & NS_FRAME_REFLOW_NEXTINFLOW)
    aOverflowCont->AddStateBits(NS_FRAME_IS_DIRTY);

  
  StepForward();
  NS_ASSERTION(mPrevOverflowCont == aOverflowCont ||
               (mSkipOverflowContainerChildren &&
                (mPrevOverflowCont->GetStateBits() & NS_FRAME_OUT_OF_FLOW) !=
                (aOverflowCont->GetStateBits() & NS_FRAME_OUT_OF_FLOW)),
              "OverflowContTracker in unexpected state");

  if (convertedToOverflowContainer) {
    
    
    
    
    nsIFrame* f = aOverflowCont->GetNextContinuation();
    if (f && !(f->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)) {
      nsContainerFrame* parent = static_cast<nsContainerFrame*>(f->GetParent());
      rv = parent->StealFrame(presContext, f);
      NS_ENSURE_SUCCESS(rv, rv);
      Insert(f, aReflowStatus);
    }
  }
  return rv;
}

void
nsOverflowContinuationTracker::Finish(nsIFrame* aChild)
{
  NS_PRECONDITION(aChild, "null ptr");
  NS_PRECONDITION(aChild->GetNextInFlow(),
                "supposed to call Finish *before* deleting next-in-flow!");

  for (nsIFrame* f = aChild; f; f = f->GetNextInFlow()) {
    if (f == mSentry) {
      
      
      if (mOverflowContList->FirstChild() == f->GetNextInFlow()
          && !f->GetNextInFlow()->GetNextSibling()) {
        mOverflowContList = nsnull;
        mPrevOverflowCont = nsnull;
        mSentry = nsnull;
        mParent = static_cast<nsContainerFrame*>(f->GetParent());
        break;
      }
      else {
        
        nsIFrame* prevOverflowCont = mPrevOverflowCont;
        StepForward();
        if (mPrevOverflowCont == f->GetNextInFlow()) {
          
          
          mPrevOverflowCont = prevOverflowCont;
        }
      }
    }
  }
}




#ifdef NS_DEBUG
NS_IMETHODIMP
nsContainerFrame::List(FILE* out, PRInt32 aIndent) const
{
  IndentBy(out, aIndent);
  ListTag(out);
#ifdef DEBUG_waterson
  fprintf(out, " [parent=%p]", static_cast<void*>(mParent));
#endif
  if (HasView()) {
    fprintf(out, " [view=%p]", static_cast<void*>(GetView()));
  }
  if (nsnull != mNextSibling) {
    fprintf(out, " next=%p", static_cast<void*>(mNextSibling));
  }
  if (nsnull != GetPrevContinuation()) {
    fprintf(out, " prev-continuation=%p", static_cast<void*>(GetPrevContinuation()));
  }
  if (nsnull != GetNextContinuation()) {
    fprintf(out, " next-continuation=%p", static_cast<void*>(GetNextContinuation()));
  }
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%08x]", mState);
  }
  fprintf(out, " [content=%p]", static_cast<void*>(mContent));
  nsContainerFrame* f = const_cast<nsContainerFrame*>(this);
  if (f->HasOverflowRect()) {
    nsRect overflowArea = f->GetOverflowRect();
    fprintf(out, " [overflow=%d,%d,%d,%d]", overflowArea.x, overflowArea.y,
            overflowArea.width, overflowArea.height);
  }
  fprintf(out, " [sc=%p]", static_cast<void*>(mStyleContext));
  nsIAtom* pseudoTag = mStyleContext->GetPseudoType();
  if (pseudoTag) {
    nsAutoString atomString;
    pseudoTag->ToString(atomString);
    fprintf(out, " pst=%s",
            NS_LossyConvertUTF16toASCII(atomString).get());
  }

  
  nsIAtom* listName = nsnull;
  PRInt32 listIndex = 0;
  PRBool outputOneList = PR_FALSE;
  do {
    nsIFrame* kid = GetFirstChild(listName);
    if (nsnull != kid) {
      if (outputOneList) {
        IndentBy(out, aIndent);
      }
      outputOneList = PR_TRUE;
      nsAutoString tmp;
      if (nsnull != listName) {
        listName->ToString(tmp);
        fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);
      }
      fputs("<\n", out);
      while (nsnull != kid) {
        
        NS_ASSERTION(kid->GetParent() == (nsIFrame*)this, "bad parent frame pointer");

        
        kid->List(out, aIndent + 1);
        kid = kid->GetNextSibling();
      }
      IndentBy(out, aIndent);
      fputs(">\n", out);
    }
    listName = GetAdditionalChildListName(listIndex++);
  } while(nsnull != listName);

  if (!outputOneList) {
    fputs("<>\n", out);
  }

  return NS_OK;
}
#endif
