









































#include "nsCOMPtr.h"
#include "nsAbsoluteContainingBlock.h"
#include "nsContainerFrame.h"
#include "nsIPresShell.h"
#include "nsHTMLParts.h"
#include "nsPresContext.h"

#ifdef DEBUG
#include "nsBlockFrame.h"
#endif


nsresult
nsAbsoluteContainingBlock::FirstChild(const nsIFrame* aDelegatingFrame,
                                      nsIAtom*        aListName,
                                      nsIFrame**      aFirstChild) const
{
  NS_PRECONDITION(GetChildListName() == aListName, "unexpected child list name");
  *aFirstChild = mAbsoluteFrames.FirstChild();
  return NS_OK;
}

nsresult
nsAbsoluteContainingBlock::SetInitialChildList(nsIFrame*       aDelegatingFrame,
                                               nsIAtom*        aListName,
                                               nsIFrame*       aChildList)
{
  NS_PRECONDITION(GetChildListName() == aListName, "unexpected child list name");
#ifdef NS_DEBUG
  nsFrame::VerifyDirtyBitSet(aChildList);
#endif
  mAbsoluteFrames.SetFrames(aChildList);
  return NS_OK;
}

nsresult
nsAbsoluteContainingBlock::AppendFrames(nsIFrame*      aDelegatingFrame,
                                        nsIAtom*       aListName,
                                        nsIFrame*      aFrameList)
{
  NS_ASSERTION(GetChildListName() == aListName, "unexpected child list");

  
#ifdef NS_DEBUG
  nsFrame::VerifyDirtyBitSet(aFrameList);
#endif
  mAbsoluteFrames.AppendFrames(nsnull, aFrameList);

  
  
  return aDelegatingFrame->PresContext()->PresShell()->
    FrameNeedsReflow(aDelegatingFrame, nsIPresShell::eResize,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
}

nsresult
nsAbsoluteContainingBlock::InsertFrames(nsIFrame*      aDelegatingFrame,
                                        nsIAtom*       aListName,
                                        nsIFrame*      aPrevFrame,
                                        nsIFrame*      aFrameList)
{
  NS_ASSERTION(GetChildListName() == aListName, "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == aDelegatingFrame,
               "inserting after sibling frame with different parent");

#ifdef NS_DEBUG
  nsFrame::VerifyDirtyBitSet(aFrameList);
#endif
  mAbsoluteFrames.InsertFrames(nsnull, aPrevFrame, aFrameList);

  
  
  return aDelegatingFrame->PresContext()->PresShell()->
    FrameNeedsReflow(aDelegatingFrame, nsIPresShell::eResize,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
}

nsresult
nsAbsoluteContainingBlock::RemoveFrame(nsIFrame*       aDelegatingFrame,
                                       nsIAtom*        aListName,
                                       nsIFrame*       aOldFrame)
{
  NS_ASSERTION(GetChildListName() == aListName, "unexpected child list");

  PRBool result = mAbsoluteFrames.DestroyFrame(aOldFrame);
  NS_ASSERTION(result, "didn't find frame to delete");
  
  
  
  return result ? NS_OK : NS_ERROR_FAILURE;
}

static void
AddFrameToChildBounds(nsIFrame* aKidFrame, nsRect* aChildBounds)
{
  NS_PRECONDITION(aKidFrame, "Must have kid frame");
  
  if (!aChildBounds) {
    return;
  }

  
  nsRect kidBounds = aKidFrame->GetRect();
  nsRect* kidOverflow = aKidFrame->GetOverflowAreaProperty();
  if (kidOverflow) {
    
    kidBounds = *kidOverflow + kidBounds.TopLeft();
  }
  aChildBounds->UnionRect(*aChildBounds, kidBounds);
}

nsresult
nsAbsoluteContainingBlock::Reflow(nsIFrame*                aDelegatingFrame,
                                  nsPresContext*          aPresContext,
                                  const nsHTMLReflowState& aReflowState,
                                  nscoord                  aContainingBlockWidth,
                                  nscoord                  aContainingBlockHeight,
                                  PRBool                   aCBWidthChanged,
                                  PRBool                   aCBHeightChanged,
                                  nsRect*                  aChildBounds)
{
  
  if (aChildBounds)
    aChildBounds->SetRect(0, 0, 0, 0);

  PRBool reflowAll = aReflowState.ShouldReflowAllKids();

  nsIFrame* kidFrame;
  for (kidFrame = mAbsoluteFrames.FirstChild(); kidFrame; kidFrame = kidFrame->GetNextSibling()) {
    if (reflowAll ||
        NS_SUBTREE_DIRTY(kidFrame) ||
        FrameDependsOnContainer(kidFrame, aCBWidthChanged, aCBHeightChanged)) {
      
      nsReflowStatus  kidStatus;
      ReflowAbsoluteFrame(aDelegatingFrame, aPresContext, aReflowState, aContainingBlockWidth,
                          aContainingBlockHeight, kidFrame, kidStatus);

    }
    AddFrameToChildBounds(kidFrame, aChildBounds);
  }
  return NS_OK;
}

static inline PRBool IsFixedPaddingSize(nsStyleUnit aUnit) {
  return aUnit == eStyleUnit_Coord || aUnit == eStyleUnit_Null ||
         aUnit == eStyleUnit_Chars;
}
static inline PRBool IsFixedMarginSize(nsStyleUnit aUnit) {
  return aUnit == eStyleUnit_Coord || aUnit == eStyleUnit_Null ||
         aUnit == eStyleUnit_Chars;
}
static inline PRBool IsFixedMaxSize(nsStyleUnit aUnit) {
  return aUnit == eStyleUnit_Null || aUnit == eStyleUnit_Coord ||
         aUnit == eStyleUnit_Chars;
}
static inline PRBool IsFixedOffset(nsStyleUnit aUnit) {
  return aUnit == eStyleUnit_Coord || aUnit == eStyleUnit_Chars;
}
static inline PRBool IsFixedHeight(nsStyleUnit aUnit) {
  return aUnit == eStyleUnit_Coord || aUnit == eStyleUnit_Chars;
}

static inline PRBool IsFixedWidth(const nsStyleCoord& aCoord)
{
  return aCoord.GetUnit() == eStyleUnit_Coord ||
         aCoord.GetUnit() == eStyleUnit_Chars ||
         (aCoord.GetUnit() == eStyleUnit_Enumerated &&
          aCoord.GetIntValue() == NS_STYLE_WIDTH_INTRINSIC ||
          aCoord.GetIntValue() == NS_STYLE_WIDTH_MIN_INTRINSIC);
}

static inline PRBool IsFixedMaxWidth(const nsStyleCoord& aCoord)
{
  return aCoord.GetUnit() == eStyleUnit_Null ||
         IsFixedWidth(aCoord);
}

PRBool
nsAbsoluteContainingBlock::FrameDependsOnContainer(nsIFrame* f,
                                                   PRBool aCBWidthChanged,
                                                   PRBool aCBHeightChanged)
{
  const nsStylePosition* pos = f->GetStylePosition();
  
  
  
  
  
  
  
  
  
  
  
  if ((pos->mOffset.GetTopUnit() == eStyleUnit_Auto &&
       pos->mOffset.GetBottomUnit() == eStyleUnit_Auto) ||
      (pos->mOffset.GetLeftUnit() == eStyleUnit_Auto &&
       pos->mOffset.GetRightUnit() == eStyleUnit_Auto)) {
    return PR_TRUE;
  }
  if (!aCBWidthChanged && !aCBHeightChanged) {
    
    return PR_FALSE;
  }
  const nsStylePadding* padding = f->GetStylePadding();
  const nsStyleMargin* margin = f->GetStyleMargin();
  if (aCBWidthChanged) {
    
    
    
    
    
    
    if (!IsFixedWidth(pos->mWidth) ||
        !IsFixedWidth(pos->mMinWidth) ||
        !IsFixedMaxWidth(pos->mMaxWidth) ||
        !IsFixedPaddingSize(padding->mPadding.GetLeftUnit()) ||
        !IsFixedPaddingSize(padding->mPadding.GetRightUnit())) {
      return PR_TRUE;
    }

    
    
    
    if (!IsFixedMarginSize(margin->mMargin.GetLeftUnit()) ||
        !IsFixedMarginSize(margin->mMargin.GetRightUnit())) {
      return PR_TRUE;
    }
    if (f->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
      
      
      
      
      
      
      if (!IsFixedOffset(pos->mOffset.GetLeftUnit()) ||
          pos->mOffset.GetRightUnit() != eStyleUnit_Auto) {
        return PR_TRUE;
      }
    } else {
      if (!IsFixedOffset(pos->mOffset.GetLeftUnit())) {
        return PR_TRUE;
      }
    }
  }
  if (aCBHeightChanged) {
    
    
    
    
    
    
    if (!(IsFixedHeight(pos->mHeight.GetUnit()) ||
          (pos->mHeight.GetUnit() == eStyleUnit_Auto &&
           pos->mOffset.GetBottomUnit() == eStyleUnit_Auto &&
           pos->mOffset.GetTopUnit() != eStyleUnit_Auto)) ||
        !IsFixedHeight(pos->mMinHeight.GetUnit()) ||
        !IsFixedMaxSize(pos->mMaxHeight.GetUnit()) ||
        !IsFixedPaddingSize(padding->mPadding.GetTopUnit()) ||
        !IsFixedPaddingSize(padding->mPadding.GetBottomUnit())) { 
      return PR_TRUE;
    }
      
    
    if (!IsFixedMarginSize(margin->mMargin.GetTopUnit()) ||
        !IsFixedMarginSize(margin->mMargin.GetBottomUnit())) {
      return PR_TRUE;
    }
    if (!IsFixedOffset(pos->mOffset.GetTopUnit())) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

void
nsAbsoluteContainingBlock::DestroyFrames(nsIFrame* aDelegatingFrame)
{
  mAbsoluteFrames.DestroyFrames();
}









nsresult
nsAbsoluteContainingBlock::ReflowAbsoluteFrame(nsIFrame*                aDelegatingFrame,
                                               nsPresContext*          aPresContext,
                                               const nsHTMLReflowState& aReflowState,
                                               nscoord                  aContainingBlockWidth,
                                               nscoord                  aContainingBlockHeight,
                                               nsIFrame*                aKidFrame,
                                               nsReflowStatus&          aStatus)
{
#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout,nsBlockFrame::gNoiseIndent);
    printf("abs pos ");
    if (nsnull != aKidFrame) {
      nsIFrameDebug*  frameDebug;
      if (NS_SUCCEEDED(CallQueryInterface(aKidFrame, &frameDebug))) {
        nsAutoString name;
        frameDebug->GetFrameName(name);
        printf("%s ", NS_LossyConvertUTF16toASCII(name).get());
      }
    }

    char width[16];
    char height[16];
    PrettyUC(aReflowState.availableWidth, width);
    PrettyUC(aReflowState.availableHeight, height);
    printf(" a=%s,%s ", width, height);
    PrettyUC(aReflowState.ComputedWidth(), width);
    PrettyUC(aReflowState.mComputedHeight, height);
    printf("c=%s,%s \n", width, height);
  }
  AutoNoisyIndenter indent(nsBlockFrame::gNoisy);
#endif 

  
  
  nsRect oldOverflowRect(aKidFrame->GetOverflowRect() +
                         aKidFrame->GetPosition());
  nsRect oldRect = aKidFrame->GetRect();

  nsresult  rv;
  
  const nsMargin& border = aReflowState.mStyleBorder->GetBorder();

  nscoord availWidth = aContainingBlockWidth;
  if (availWidth == -1) {
    NS_ASSERTION(aReflowState.ComputedWidth() != NS_UNCONSTRAINEDSIZE,
                 "Must have a useful width _somewhere_");
    availWidth =
      aReflowState.ComputedWidth() + aReflowState.mComputedPadding.LeftRight();
  }
    
  nsHTMLReflowMetrics kidDesiredSize;
  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, aKidFrame,
                                   nsSize(availWidth, NS_UNCONSTRAINEDSIZE),
                                   aContainingBlockWidth,
                                   aContainingBlockHeight);

  
  aKidFrame->WillReflow(aPresContext);

  
  
  nscoord x;
  if (NS_AUTOOFFSET == kidReflowState.mComputedOffsets.left) {
    
    x = aKidFrame->GetPosition().x;
  } else {
    x = border.left + kidReflowState.mComputedOffsets.left + kidReflowState.mComputedMargin.left;
  }
  aKidFrame->SetPosition(nsPoint(x, border.top +
                                    kidReflowState.mComputedOffsets.top +
                                    kidReflowState.mComputedMargin.top));

  
  rv = aKidFrame->Reflow(aPresContext, kidDesiredSize, kidReflowState, aStatus);

  
  
  if ((NS_AUTOOFFSET == kidReflowState.mComputedOffsets.left) ||
      (NS_AUTOOFFSET == kidReflowState.mComputedOffsets.top)) {
    if (-1 == aContainingBlockWidth) {
      
      kidReflowState.ComputeContainingBlockRectangle(aPresContext,
                                                     &aReflowState,
                                                     aContainingBlockWidth,
                                                     aContainingBlockHeight);
    }

    if (NS_AUTOOFFSET == kidReflowState.mComputedOffsets.left) {
      NS_ASSERTION(NS_AUTOOFFSET != kidReflowState.mComputedOffsets.right,
                   "Can't solve for both left and right");
      kidReflowState.mComputedOffsets.left = aContainingBlockWidth -
                                             kidReflowState.mComputedOffsets.right -
                                             kidReflowState.mComputedMargin.right -
                                             kidDesiredSize.width -
                                             kidReflowState.mComputedMargin.left;
    }
    if (NS_AUTOOFFSET == kidReflowState.mComputedOffsets.top) {
      kidReflowState.mComputedOffsets.top = aContainingBlockHeight -
                                            kidReflowState.mComputedOffsets.bottom -
                                            kidReflowState.mComputedMargin.bottom -
                                            kidDesiredSize.height -
                                            kidReflowState.mComputedMargin.top;
    }
  }

  
  nsRect  rect(border.left + kidReflowState.mComputedOffsets.left + kidReflowState.mComputedMargin.left,
               border.top + kidReflowState.mComputedOffsets.top + kidReflowState.mComputedMargin.top,
               kidDesiredSize.width, kidDesiredSize.height);
  aKidFrame->SetRect(rect);

  nsIView* view = aKidFrame->GetView();
  if (view) {
    
    
    nsContainerFrame::SyncFrameViewAfterReflow(aPresContext, aKidFrame,
                                               view,
                                               &kidDesiredSize.mOverflowArea);
  } else {
    nsContainerFrame::PositionChildViews(aKidFrame);
  }

  if (oldRect.TopLeft() != rect.TopLeft() || 
      (aDelegatingFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) ||
      ((aKidFrame->GetStyleDisplay()->mClipFlags & NS_STYLE_CLIP_RECT) && 
       (kidDesiredSize.mOverflowArea != oldOverflowRect))) {
    
    
    
    
    
    aKidFrame->GetParent()->Invalidate(oldOverflowRect);
    aKidFrame->GetParent()->Invalidate(kidDesiredSize.mOverflowArea +
                                       rect.TopLeft());
  } else if (oldRect.Size() != rect.Size()) {
    
    nscoord innerWidth = PR_MIN(oldRect.width, rect.width);
    nscoord innerHeight = PR_MIN(oldRect.height, rect.height);
    nscoord outerWidth = PR_MAX(oldRect.width, rect.width);
    nscoord outerHeight = PR_MAX(oldRect.height, rect.height);
    aKidFrame->GetParent()->Invalidate(
        nsRect(rect.x + innerWidth, rect.y, outerWidth - innerWidth, outerHeight));
    
    aKidFrame->GetParent()->Invalidate(
        nsRect(rect.x, rect.y + innerHeight, outerWidth, outerHeight - innerHeight));
  }
  aKidFrame->DidReflow(aPresContext, &kidReflowState, NS_FRAME_REFLOW_FINISHED);

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout,nsBlockFrame::gNoiseIndent - 1);
    printf("abs pos ");
    if (nsnull != aKidFrame) {
      nsIFrameDebug*  frameDebug;
      if (NS_SUCCEEDED(CallQueryInterface(aKidFrame, &frameDebug))) {
        nsAutoString name;
        frameDebug->GetFrameName(name);
        printf("%s ", NS_LossyConvertUTF16toASCII(name).get());
      }
    }
    printf("%p rect=%d,%d,%d,%d", aKidFrame, rect.x, rect.y, rect.width, rect.height);
    printf("\n");
  }
#endif

  return rv;
}

#ifdef DEBUG
 void nsAbsoluteContainingBlock::PrettyUC(nscoord aSize,
                        char*   aBuf)
{
  if (NS_UNCONSTRAINEDSIZE == aSize) {
    strcpy(aBuf, "UC");
  }
  else {
    if((PRInt32)0xdeadbeef == aSize)
    {
      strcpy(aBuf, "deadbeef");
    }
    else {
      sprintf(aBuf, "%d", aSize);
    }
  }
}
#endif
