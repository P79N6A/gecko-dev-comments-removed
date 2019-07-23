



































#include "nsTableColGroupFrame.h"
#include "nsTableColFrame.h"
#include "nsTableFrame.h"
#include "nsIDOMHTMLTableColElement.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsCOMPtr.h"
#include "nsCSSRendering.h"
#include "nsIPresShell.h"

#define COL_GROUP_TYPE_BITS          0xC0000000 // uses bits 31-32 from mState
#define COL_GROUP_TYPE_OFFSET        30

nsTableColGroupType 
nsTableColGroupFrame::GetColType() const 
{
  return (nsTableColGroupType)((mState & COL_GROUP_TYPE_BITS) >> COL_GROUP_TYPE_OFFSET);
}

void nsTableColGroupFrame::SetColType(nsTableColGroupType aType) 
{
  PRUint32 type = aType - eColGroupContent;
  mState |= (type << COL_GROUP_TYPE_OFFSET);
}

void nsTableColGroupFrame::ResetColIndices(nsIFrame*       aFirstColGroup,
                                           PRInt32         aFirstColIndex,
                                           nsIFrame*       aStartColFrame)
{
  nsTableColGroupFrame* colGroupFrame = (nsTableColGroupFrame*)aFirstColGroup;
  PRInt32 colIndex = aFirstColIndex;
  while (colGroupFrame) {
    if (nsGkAtoms::tableColGroupFrame == colGroupFrame->GetType()) {
      
      
      
      if ((colIndex != aFirstColIndex) ||
          (colIndex < colGroupFrame->GetStartColumnIndex()) ||
          !aStartColFrame) {
        colGroupFrame->SetStartColumnIndex(colIndex);
      }
      nsIFrame* colFrame = aStartColFrame; 
      if (!colFrame || (colIndex != aFirstColIndex)) {
        colFrame = colGroupFrame->GetFirstChild(nsnull);
      }
      while (colFrame) {
        if (nsGkAtoms::tableColFrame == colFrame->GetType()) {
          ((nsTableColFrame*)colFrame)->SetColIndex(colIndex);
          colIndex++;
        }
        colFrame = colFrame->GetNextSibling();
      }
    }
    colGroupFrame = static_cast<nsTableColGroupFrame*>
                               (colGroupFrame->GetNextSibling());
  }
}


nsresult
nsTableColGroupFrame::AddColsToTable(PRInt32                   aFirstColIndex,
                                     PRBool                    aResetSubsequentColIndices,
                                     const nsFrameList::Slice& aCols)
{
  nsresult rv = NS_OK;
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (!tableFrame)
    return NS_ERROR_NULL_POINTER;

  
  PRInt32 colIndex = aFirstColIndex;
  nsFrameList::Enumerator e(aCols);
  for (; !e.AtEnd(); e.Next()) {
    ((nsTableColFrame*)e.get())->SetColIndex(colIndex);
    mColCount++;
    tableFrame->InsertCol((nsTableColFrame &)*e.get(), colIndex);
    colIndex++;
  }

  for (nsFrameList::Enumerator eTail = e.GetUnlimitedEnumerator();
       !eTail.AtEnd();
       eTail.Next()) {
    ((nsTableColFrame*)eTail.get())->SetColIndex(colIndex);
    colIndex++;
  }

  
  
  
  
  if (aResetSubsequentColIndices && GetNextSibling()) {
    ResetColIndices(GetNextSibling(), colIndex);
  }

  return rv;
}


nsTableColGroupFrame*
nsTableColGroupFrame::GetLastRealColGroup(nsTableFrame* aTableFrame)
{
  nsFrameList colGroups = aTableFrame->GetColGroups();

  nsIFrame* nextToLastColGroup = nsnull;
  nsFrameList::FrameLinkEnumerator link(colGroups);
  for ( ; !link.AtEnd(); link.Next()) {
    nextToLastColGroup = link.PrevFrame();
  }

  if (!link.PrevFrame()) {
    return nsnull; 
  }
 
  nsTableColGroupType lastColGroupType =
    static_cast<nsTableColGroupFrame*>(link.PrevFrame())->GetColType();
  if (eColGroupAnonymousCell == lastColGroupType) {
    return static_cast<nsTableColGroupFrame*>(nextToLastColGroup);
  }
 
  return static_cast<nsTableColGroupFrame*>(link.PrevFrame());
}


NS_IMETHODIMP
nsTableColGroupFrame::SetInitialChildList(nsIAtom*        aListName,
                                          nsFrameList&    aChildList)
{
  if (!mFrames.IsEmpty()) {
    
    
    NS_NOTREACHED("unexpected second call to SetInitialChildList");
    return NS_ERROR_UNEXPECTED;
  }
  if (aListName) {
    
    NS_NOTREACHED("unknown frame list");
    return NS_ERROR_INVALID_ARG;
  } 
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (!tableFrame)
    return NS_ERROR_NULL_POINTER;

  if (aChildList.IsEmpty()) {
    tableFrame->AppendAnonymousColFrames(this, GetSpan(), eColAnonymousColGroup, 
                                         PR_FALSE);
    return NS_OK; 
  }

  mFrames.AppendFrames(this, aChildList);
  return NS_OK;
}

 void
nsTableColGroupFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  if (!aOldStyleContext) 
    return;
     
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);

  if (tableFrame->IsBorderCollapse() &&
      tableFrame->BCRecalcNeeded(aOldStyleContext, GetStyleContext())) {
    PRInt32 colCount = GetColCount();
    if (!colCount)
      return; 
    nsRect damageArea(GetFirstColumn()->GetColIndex(), 0, colCount,
                      tableFrame->GetRowCount());
    tableFrame->SetBCDamageArea(damageArea);
  }
  return;
}

NS_IMETHODIMP
nsTableColGroupFrame::AppendFrames(nsIAtom*        aListName,
                                   nsFrameList&    aFrameList)
{
  NS_ASSERTION(!aListName, "unexpected child list");

  nsTableColFrame* col = GetFirstColumn();
  nsTableColFrame* nextCol;
  while (col && col->GetColType() == eColAnonymousColGroup) {
    
    
    
    
    nextCol = col->GetNextCol();
    RemoveFrame(nsnull, col);
    col = nextCol;
  }

  const nsFrameList::Slice& newFrames =
    mFrames.AppendFrames(this, aFrameList);
  InsertColsReflow(GetStartColumnIndex() + mColCount, newFrames);
  return NS_OK;
}

NS_IMETHODIMP
nsTableColGroupFrame::InsertFrames(nsIAtom*        aListName,
                                   nsIFrame*       aPrevFrame,
                                   nsFrameList&    aFrameList)
{
  NS_ASSERTION(!aListName, "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  nsTableColFrame* col = GetFirstColumn();
  nsTableColFrame* nextCol;
  while (col && col->GetColType() == eColAnonymousColGroup) {
    
    
    
    
    nextCol = col->GetNextCol();
    if (col == aPrevFrame) {
      
      NS_ASSERTION(!nextCol || nextCol->GetColType() != eColAnonymousColGroup,
                   "Inserting in the middle of our anonymous cols?");
      
      aPrevFrame = nsnull;
    }
    RemoveFrame(nsnull, col);
    col = nextCol;
  }

  NS_ASSERTION(!aPrevFrame || aPrevFrame == aPrevFrame->GetLastContinuation(),
               "Prev frame should be last in continuation chain");
  NS_ASSERTION(!aPrevFrame || !GetNextColumn(aPrevFrame) ||
               GetNextColumn(aPrevFrame)->GetColType() != eColAnonymousCol,
               "Shouldn't be inserting before a spanned colframe");

  const nsFrameList::Slice& newFrames =
    mFrames.InsertFrames(this, aPrevFrame, aFrameList);
  nsIFrame* prevFrame = nsTableFrame::GetFrameAtOrBefore(this, aPrevFrame,
                                                         nsGkAtoms::tableColFrame);

  PRInt32 colIndex = (prevFrame) ? ((nsTableColFrame*)prevFrame)->GetColIndex() + 1 : GetStartColumnIndex();
  InsertColsReflow(colIndex, newFrames);

  return NS_OK;
}

void
nsTableColGroupFrame::InsertColsReflow(PRInt32                   aColIndex,
                                       const nsFrameList::Slice& aCols)
{
  AddColsToTable(aColIndex, PR_TRUE, aCols);

  PresContext()->PresShell()->FrameNeedsReflow(this,
                                               nsIPresShell::eTreeChange,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
}

void
nsTableColGroupFrame::RemoveChild(nsTableColFrame& aChild,
                                  PRBool           aResetSubsequentColIndices)
{
  PRInt32 colIndex = 0;
  nsIFrame* nextChild = nsnull;
  if (aResetSubsequentColIndices) {
    colIndex = aChild.GetColIndex();
    nextChild = aChild.GetNextSibling();
  }
  if (mFrames.DestroyFrame((nsIFrame*)&aChild)) {
    mColCount--;
    if (aResetSubsequentColIndices) {
      if (nextChild) { 
        ResetColIndices(this, colIndex, nextChild);
      }
      else {
        nsIFrame* nextGroup = GetNextSibling();
        if (nextGroup) 
          ResetColIndices(nextGroup, colIndex);
      }
    }
  }

  PresContext()->PresShell()->FrameNeedsReflow(this,
                                               nsIPresShell::eTreeChange,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
}

NS_IMETHODIMP
nsTableColGroupFrame::RemoveFrame(nsIAtom*        aListName,
                                  nsIFrame*       aOldFrame)
{
  NS_ASSERTION(!aListName, "unexpected child list");

  if (!aOldFrame) return NS_OK;

  if (nsGkAtoms::tableColFrame == aOldFrame->GetType()) {
    nsTableColFrame* colFrame = (nsTableColFrame*)aOldFrame;
    if (colFrame->GetColType() == eColContent) {
      
      nsTableColFrame* col = colFrame->GetNextCol();
      nsTableColFrame* nextCol;
      while (col && col->GetColType() == eColAnonymousCol) {
        NS_ASSERTION(col->GetStyleContext() == colFrame->GetStyleContext() &&
                     col->GetContent() == colFrame->GetContent(),
                     "How did that happen??");
        nextCol = col->GetNextCol();
        RemoveFrame(nsnull, col);
        col = nextCol;
      }
    }
    
    PRInt32 colIndex = colFrame->GetColIndex();
    
    RemoveChild(*colFrame, PR_TRUE);
    
    nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
    if (!tableFrame)
      return NS_ERROR_NULL_POINTER;

    tableFrame->RemoveCol(this, colIndex, PR_TRUE, PR_TRUE);
  }
  else {
    mFrames.DestroyFrame(aOldFrame);
  }

  return NS_OK;
}

PRIntn
nsTableColGroupFrame::GetSkipSides() const
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

NS_METHOD nsTableColGroupFrame::Reflow(nsPresContext*          aPresContext,
                                       nsHTMLReflowMetrics&     aDesiredSize,
                                       const nsHTMLReflowState& aReflowState,
                                       nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsTableColGroupFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  NS_ASSERTION(nsnull!=mContent, "bad state -- null content for frame");
  nsresult rv=NS_OK;
  
  const nsStyleVisibility* groupVis = GetStyleVisibility();
  PRBool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE == groupVis->mVisible);
  if (collapseGroup) {
    nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
    if (tableFrame)  {
      tableFrame->SetNeedToCollapse(PR_TRUE);;
    }
  }
  
  
  
  for (nsIFrame *kidFrame = mFrames.FirstChild(); kidFrame;
       kidFrame = kidFrame->GetNextSibling()) {
    
    nsHTMLReflowMetrics kidSize;
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, kidFrame,
                                     nsSize(0,0));

    nsReflowStatus status;
    ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState, 0, 0, 0, status);
    FinishReflowChild(kidFrame, aPresContext, nsnull, kidSize, 0, 0, 0);
  }

  aDesiredSize.width=0;
  aDesiredSize.height=0;
  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return rv;
}

 PRBool
nsTableColGroupFrame::IsContainingBlock() const
{
  return PR_TRUE;
}

nsTableColFrame * nsTableColGroupFrame::GetFirstColumn()
{
  return GetNextColumn(nsnull);
}

nsTableColFrame * nsTableColGroupFrame::GetNextColumn(nsIFrame *aChildFrame)
{
  nsTableColFrame *result = nsnull;
  nsIFrame *childFrame = aChildFrame;
  if (!childFrame) {
    childFrame = mFrames.FirstChild();
  }
  else {
    childFrame = childFrame->GetNextSibling();
  }
  while (childFrame)
  {
    if (NS_STYLE_DISPLAY_TABLE_COLUMN ==
        childFrame->GetStyleDisplay()->mDisplay)
    {
      result = (nsTableColFrame *)childFrame;
      break;
    }
    childFrame = childFrame->GetNextSibling();
  }
  return result;
}

PRInt32 nsTableColGroupFrame::GetSpan()
{
  return GetStyleTable()->mSpan;
}

void nsTableColGroupFrame::SetContinuousBCBorderWidth(PRUint8     aForSide,
                                                      BCPixelSize aPixelValue)
{
  switch (aForSide) {
    case NS_SIDE_TOP:
      mTopContBorderWidth = aPixelValue;
      return;
    case NS_SIDE_BOTTOM:
      mBottomContBorderWidth = aPixelValue;
      return;
    default:
      NS_ERROR("invalid side arg");
  }
}

void nsTableColGroupFrame::GetContinuousBCBorderWidth(nsMargin& aBorder)
{
  PRInt32 aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  nsTableFrame* table = nsTableFrame::GetTableFrame(this);
  nsTableColFrame* col = table->GetColFrame(mStartColIndex + mColCount - 1);
  col->GetContinuousBCBorderWidth(aBorder);
  aBorder.top = BC_BORDER_BOTTOM_HALF_COORD(aPixelsToTwips,
                                            mTopContBorderWidth);
  aBorder.bottom = BC_BORDER_TOP_HALF_COORD(aPixelsToTwips,
                                            mBottomContBorderWidth);
  return;
}



nsIFrame*
NS_NewTableColGroupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTableColGroupFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTableColGroupFrame)

nsIAtom*
nsTableColGroupFrame::GetType() const
{
  return nsGkAtoms::tableColGroupFrame;
}

#ifdef DEBUG
NS_IMETHODIMP
nsTableColGroupFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("TableColGroup"), aResult);
}

void nsTableColGroupFrame::Dump(PRInt32 aIndent)
{
  char* indent = new char[aIndent + 1];
  if (!indent) return;
  for (PRInt32 i = 0; i < aIndent + 1; i++) {
    indent[i] = ' ';
  }
  indent[aIndent] = 0;

  printf("%s**START COLGROUP DUMP**\n%s startcolIndex=%d  colcount=%d span=%d coltype=",
    indent, indent, GetStartColumnIndex(),  GetColCount(), GetSpan());
  nsTableColGroupType colType = GetColType();
  switch (colType) {
  case eColGroupContent:
    printf(" content ");
    break;
  case eColGroupAnonymousCol: 
    printf(" anonymous-column  ");
    break;
  case eColGroupAnonymousCell: 
    printf(" anonymous-cell ");
    break;
  }
  
  PRInt32 j = GetStartColumnIndex();
  nsTableColFrame* col = GetFirstColumn();
  while (col) {
    NS_ASSERTION(j == col->GetColIndex(), "wrong colindex on col frame");
    col = col->GetNextCol();
    j++;
  }
  NS_ASSERTION((j - GetStartColumnIndex()) == GetColCount(),
               "number of cols out of sync");
  printf("\n%s**END COLGROUP DUMP** ", indent);
  delete [] indent;
}
#endif
