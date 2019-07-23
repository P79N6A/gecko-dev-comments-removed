







































#include "nsContainerFrame.h"
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

#ifdef NS_DEBUG
#undef NOISY
#else
#undef NOISY
#endif

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
nsContainerFrame::SetInitialChildList(nsIAtom*  aListName,
                                      nsIFrame* aChildList)
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
                               nsIFrame* aFrameList)
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
  if (aFrameList) {
    mFrames.AppendFrames(this, aFrameList);

    
#ifdef IBMBIDI
    if (nsnull == aListName)
#endif
    {
      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange, NS_FRAME_IS_DIRTY);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsContainerFrame::InsertFrames(nsIAtom*  aListName,
                               nsIFrame* aPrevFrame,
                               nsIFrame* aFrameList)
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
  if (aFrameList) {
    
    mFrames.InsertFrames(this, aPrevFrame, aFrameList);

#ifdef IBMBIDI
    if (nsnull == aListName)
#endif
    {
      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange, NS_FRAME_IS_DIRTY);
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
    
    
    
    
    PRBool generateReflowCommand =
      aOldFrame->GetType() == nsGkAtoms::brFrame;

    nsContainerFrame* parent = NS_STATIC_CAST(nsContainerFrame*, aOldFrame->GetParent());
    while (aOldFrame) {
#ifdef IBMBIDI
      if (nsGkAtoms::nextBidi != aListName) {
#endif


      nsRect bbox = aOldFrame->GetRect();
      if (bbox.width || bbox.height) {
        generateReflowCommand = PR_TRUE;
      }
#ifdef IBMBIDI
      }
#endif

      
      
      
      nsIFrame* oldFrameNextContinuation = aOldFrame->GetNextContinuation();
      parent->mFrames.DestroyFrame(aOldFrame);
      aOldFrame = oldFrameNextContinuation;
      if (aOldFrame) {
        parent = NS_STATIC_CAST(nsContainerFrame*, aOldFrame->GetParent());
      }
    }

    if (generateReflowCommand) {
      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange, NS_FRAME_IS_DIRTY);
    }
  }

  return NS_OK;
}

void
nsContainerFrame::CleanupGeneratedContentIn(nsIContent* aRealContent,
                                            nsIFrame* aRoot) {
  nsIAtom* frameList = nsnull;
  PRInt32 listIndex = 0;
  do {
    nsIFrame* child = aRoot->GetFirstChild(frameList);
    while (child) {
      nsIContent* content = child->GetContent();
      if (content && content != aRealContent) {
        
        
        aRoot->PresContext()->EventStateManager()->ContentRemoved(content);
        content->UnbindFromTree();
      }
      CleanupGeneratedContentIn(aRealContent, child);
      child = child->GetNextSibling();
    }
    frameList = aRoot->GetAdditionalChildListName(listIndex++);
  } while (frameList);
}

void
nsContainerFrame::Destroy()
{
  
  if (HasView()) {
    GetView()->SetClientData(nsnull);
  }

  
  mFrames.DestroyFrames();
  
  
  nsFrameList overflowFrames(GetOverflowFrames(PresContext(), PR_TRUE));
  overflowFrames.DestroyFrames();

  
  nsSplittableFrame::Destroy();
}




nsIFrame*
nsContainerFrame::GetFirstChild(nsIAtom* aListName) const
{
  
  
  if (nsnull == aListName) {
    return mFrames.FirstChild();
  } else if (nsGkAtoms::overflowList == aListName) {
    return GetOverflowFrames(PresContext(), PR_FALSE);
  } else {
    return nsnull;
  }
}

nsIAtom*
nsContainerFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  if (aIndex == 0) {
    return nsGkAtoms::overflowList;
  } else {
    return nsnull;
  }
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
HasNonZeroBorderRadius(nsStyleContext* aStyleContext) {
  const nsStyleBorder* border = aStyleContext->GetStyleBorder();
  return nsLayoutUtils::HasNonZeroSide(border->mBorderRadius);
}

static void
SyncFrameViewGeometryDependentProperties(nsPresContext*  aPresContext,
                                         nsIFrame*        aFrame,
                                         nsStyleContext*  aStyleContext,
                                         nsIView*         aView,
                                         PRUint32         aFlags)
{
  nsIViewManager* vm = aView->GetViewManager();

  PRBool isCanvas;
  const nsStyleBackground* bg;
  PRBool hasBG =
    nsCSSRendering::FindBackground(aPresContext, aFrame, &bg, &isCanvas);

  const nsStyleDisplay* display = aStyleContext->GetStyleDisplay();
  
  
  
  
  
  PRBool  viewHasTransparentContent =
    !(hasBG && !(bg->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT) &&
      !display->mAppearance && bg->mBackgroundClip == NS_STYLE_BG_CLIP_BORDER &&
      !HasNonZeroBorderRadius(aStyleContext));

  if (isCanvas) {
    nsIView* rootView;
    vm->GetRootView(rootView);
    nsIView* rootParent = rootView->GetParent();
    if (!rootParent) {
      
      
      viewHasTransparentContent = PR_FALSE;
    }

    nsIDocument *doc = aPresContext->PresShell()->GetDocument();
    if (doc) {
      nsIContent *rootElem = doc->GetRootContent();
      if (!doc->GetParentDocument() &&
          (nsCOMPtr<nsISupports>(doc->GetContainer())) &&
          rootElem && rootElem->IsNodeOfType(nsINode::eXUL)) {
        
        
        
        
        if (aView->HasWidget() && aView == rootView) {
          viewHasTransparentContent = nsLayoutUtils::FrameHasTransparency(aFrame);
          aView->GetWidget()->SetWindowTranslucency(viewHasTransparentContent);
        }
      }
    }
  }
  
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

    
    
    
    
    
    
    nsStyleContext* savedStyleContext = aFrame->GetStyleContext();
    SyncFrameViewGeometryDependentProperties(aPresContext, aFrame, savedStyleContext, aView, aFlags);
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

  if (nsnull == aStyleContext) {
    aStyleContext = aFrame->GetStyleContext();
  }

  
  if (0 == (aFlags & NS_FRAME_NO_VISIBILITY)) {
    
    PRBool  viewIsVisible = PR_TRUE;

    if (!aStyleContext->GetStyleVisibility()->IsVisible() &&
        !aFrame->SupportsVisibilityHidden()) {
      
      
      
      
      
      viewIsVisible = PR_FALSE;
    } else {
      
      nsIWidget* widget = aView->GetWidget();
      if (widget) {
        nsWindowType windowType;
        widget->GetWindowType(windowType);
        if (windowType == eWindowType_popup) {
          widget->IsVisible(viewIsVisible);
        }
      }
    }

    vm->SetViewVisibility(aView, viewIsVisible ? nsViewVisibility_kShow :
                          nsViewVisibility_kHide);
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

  SyncFrameViewGeometryDependentProperties(aPresContext, aFrame, aStyleContext, aView, aFlags);
}

PRBool
nsContainerFrame::FrameNeedsView(nsIFrame* aFrame)
{
  
  
  if (aFrame->GetStyleContext()->GetPseudoType() ==
      nsCSSAnonBoxes::scrolledContent) {
    return PR_TRUE;
  }
  return aFrame->NeedsView();
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
  nsStyleCoord tmp;

  
  
  
  
  aData->currentLine +=
    GetCoord(stylePadding->mPadding.Get(startSide, tmp), 0) +
    styleBorder->GetBorderWidth(startSide) +
    GetCoord(styleMargin->mMargin.Get(startSide, tmp), 0);

  for (nsContainerFrame *nif = this; nif;
       nif = (nsContainerFrame*) nif->GetNextInFlow()) {
    for (nsIFrame *kid = nif->mFrames.FirstChild(); kid;
         kid = kid->GetNextSibling()) {
      if (aType == nsLayoutUtils::MIN_WIDTH)
        kid->AddInlineMinWidth(aRenderingContext,
                               NS_STATIC_CAST(InlineMinWidthData*, aData));
      else
        kid->AddInlinePrefWidth(aRenderingContext,
                                NS_STATIC_CAST(InlinePrefWidthData*, aData));
    }
  }

  
  
  
  
  aData->currentLine +=
    GetCoord(stylePadding->mPadding.Get(endSide, tmp), 0) +
    styleBorder->GetBorderWidth(endSide) +
    GetCoord(styleMargin->mMargin.Get(endSide, tmp), 0);
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
                              nsReflowStatus&          aStatus)
{
  NS_PRECONDITION(aReflowState.frame == aKidFrame, "bad reflow state");

  nsresult  result;

  
  
  aKidFrame->WillReflow(aPresContext);

  if (0 == (aFlags & NS_FRAME_NO_MOVE_FRAME)) {
    aKidFrame->SetPosition(nsPoint(aX, aY));
  }

  if (0 == (aFlags & NS_FRAME_NO_MOVE_VIEW)) {
    PositionFrameView(aKidFrame);
  }

  
  result = aKidFrame->Reflow(aPresContext, aDesiredSize, aReflowState,
                             aStatus);

  
  
  if (NS_SUCCEEDED(result) && NS_FRAME_IS_COMPLETE(aStatus)) {
    nsIFrame* kidNextInFlow = aKidFrame->GetNextInFlow();
    if (nsnull != kidNextInFlow) {
      
      
      
      NS_STATIC_CAST(nsContainerFrame*, kidNextInFlow->GetParent())
        ->DeleteNextInFlowChild(aPresContext, kidNextInFlow);
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

    childListName = aFrame->GetAdditionalChildListName(childListIndex++);
  } while (childListName);
}


















nsresult
nsContainerFrame::FinishReflowChild(nsIFrame*                 aKidFrame,
                                    nsPresContext*            aPresContext,
                                    const nsHTMLReflowState*  aReflowState,
                                    nsHTMLReflowMetrics&      aDesiredSize,
                                    nscoord                   aX,
                                    nscoord                   aY,
                                    PRUint32                  aFlags)
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





void
nsContainerFrame::DeleteNextInFlowChild(nsPresContext* aPresContext,
                                        nsIFrame*       aNextInFlow)
{
  nsIFrame* prevInFlow = aNextInFlow->GetPrevInFlow();
  NS_PRECONDITION(prevInFlow, "bad prev-in-flow");

  
  
  
  
  nsIFrame* nextNextInFlow = aNextInFlow->GetNextInFlow();
  if (nextNextInFlow) {
    nsAutoVoidArray frames;
    for (nsIFrame* f = nextNextInFlow; f; f = f->GetNextInFlow()) {
      frames.AppendElement(f);
    }
    for (PRInt32 i = frames.Count() - 1; i >= 0; --i) {
      nsIFrame* delFrame = NS_STATIC_CAST(nsIFrame*, frames.ElementAt(i));
      NS_STATIC_CAST(nsContainerFrame*, delFrame->GetParent())
        ->DeleteNextInFlowChild(aPresContext, delFrame);
    }
  }

  
  nsSplittableFrame::BreakFromPrevFlow(aNextInFlow);

  
  PRBool result = mFrames.RemoveFrame(aNextInFlow);
  if (!result) {
    
    
    nsFrameList overflowFrames(GetOverflowFrames(aPresContext, PR_TRUE));

    if (overflowFrames.IsEmpty() || !overflowFrames.RemoveFrame(aNextInFlow)) {
      NS_ASSERTION(result, "failed to remove frame");
    }

    
    if (overflowFrames.NotEmpty()) {
      SetOverflowFrames(aPresContext, overflowFrames.FirstChild());
    }
  }

  
  aNextInFlow->Destroy();

  NS_POSTCONDITION(!prevInFlow->GetNextInFlow(), "non null next-in-flow");
}

nsIFrame*
nsContainerFrame::GetOverflowFrames(nsPresContext* aPresContext,
                                    PRBool          aRemoveProperty) const
{
  nsPropertyTable *propTable = aPresContext->PropertyTable();
  if (aRemoveProperty) {
    return (nsIFrame*) propTable->UnsetProperty(this,
                                              nsGkAtoms::overflowProperty);
  }

  return (nsIFrame*) propTable->GetProperty(this,
                                            nsGkAtoms::overflowProperty);
}


static void
DestroyOverflowFrames(void*           aFrame,
                      nsIAtom*        aPropertyName,
                      void*           aPropertyValue,
                      void*           aDtorData)
{
  if (aPropertyValue) {
    nsFrameList frames((nsIFrame*)aPropertyValue);

    frames.DestroyFrames();
  }
}

nsresult
nsContainerFrame::SetOverflowFrames(nsPresContext* aPresContext,
                                    nsIFrame*       aOverflowFrames)
{
  nsresult rv =
    aPresContext->PropertyTable()->SetProperty(this,
                                               nsGkAtoms::overflowProperty,
                                               aOverflowFrames,
                                               DestroyOverflowFrames,
                                               nsnull);

  
  NS_ASSERTION(rv != NS_PROPTABLE_PROP_OVERWRITTEN, "existing overflow list");

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
    nsIFrame* prevOverflowFrames = prevInFlow->GetOverflowFrames(aPresContext,
                                                                 PR_TRUE);
    if (prevOverflowFrames) {
      NS_ASSERTION(mFrames.IsEmpty(), "bad overflow list");
      
      
      for (nsIFrame* f = prevOverflowFrames; f; f = f->GetNextSibling()) {
        nsHTMLContainerFrame::ReparentFrameView(aPresContext, f, prevInFlow, this);
      }
      mFrames.InsertFrames(this, nsnull, prevOverflowFrames);
      result = PR_TRUE;
    }
  }

  
  nsIFrame* overflowFrames = GetOverflowFrames(aPresContext, PR_TRUE);
  if (overflowFrames) {
    NS_ASSERTION(mFrames.NotEmpty(), "overflow list w/o frames");
    mFrames.AppendFrames(nsnull, overflowFrames);
    result = PR_TRUE;
  }
  return result;
}




#ifdef NS_DEBUG
NS_IMETHODIMP
nsContainerFrame::List(FILE* out, PRInt32 aIndent) const
{
  IndentBy(out, aIndent);
  ListTag(out);
#ifdef DEBUG_waterson
  fprintf(out, " [parent=%p]", NS_STATIC_CAST(void*, mParent));
#endif
  if (HasView()) {
    fprintf(out, " [view=%p]", NS_STATIC_CAST(void*, GetView()));
  }
  if (nsnull != mNextSibling) {
    fprintf(out, " next=%p", NS_STATIC_CAST(void*, mNextSibling));
  }
  if (nsnull != GetPrevContinuation()) {
    fprintf(out, " prev-continuation=%p", NS_STATIC_CAST(void*, GetPrevContinuation()));
  }
  if (nsnull != GetNextContinuation()) {
    fprintf(out, " next-continuation=%p", NS_STATIC_CAST(void*, GetNextContinuation()));
  }
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%08x]", mState);
  }
  fprintf(out, " [content=%p]", NS_STATIC_CAST(void*, mContent));
  nsContainerFrame* f = NS_CONST_CAST(nsContainerFrame*, this);
  nsRect* overflowArea = f->GetOverflowAreaProperty(PR_FALSE);
  if (overflowArea) {
    fprintf(out, " [overflow=%d,%d,%d,%d]", overflowArea->x, overflowArea->y,
            overflowArea->width, overflowArea->height);
  }
  fprintf(out, " [sc=%p]", NS_STATIC_CAST(void*, mStyleContext));
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

        
        nsIFrameDebug*  frameDebug;
        if (NS_SUCCEEDED(kid->QueryInterface(NS_GET_IID(nsIFrameDebug), (void**)&frameDebug))) {
          frameDebug->List(out, aIndent + 1);
        }
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
