



































#include "nsTableRowFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsIRenderingContext.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsTableFrame.h"
#include "nsTableCellFrame.h"
#include "nsCSSRendering.h"
#include "nsHTMLParts.h"
#include "nsTableColGroupFrame.h"
#include "nsTableColFrame.h"
#include "nsCOMPtr.h"
#include "nsDisplayList.h"

struct nsTableCellReflowState : public nsHTMLReflowState
{
  nsTableCellReflowState(nsPresContext*           aPresContext,
                         const nsHTMLReflowState& aParentReflowState,
                         nsIFrame*                aFrame,
                         const nsSize&            aAvailableSpace,
                         PRBool                   aInit = PR_TRUE)
    : nsHTMLReflowState(aPresContext, aParentReflowState, aFrame,
                        aAvailableSpace, -1, -1, aInit)
  {
  }

  void FixUp(const nsSize& aAvailSpace);
};

void nsTableCellReflowState::FixUp(const nsSize& aAvailSpace)
{
  
  NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aAvailSpace.width,
               "unconstrained available width in reflow");
  if (NS_UNCONSTRAINEDSIZE != ComputedWidth()) {
    nscoord computedWidth =
      aAvailSpace.width - mComputedBorderPadding.LeftRight();
    computedWidth = NS_MAX(0, computedWidth);
    SetComputedWidth(computedWidth);
  }
  if (NS_UNCONSTRAINEDSIZE != ComputedHeight() &&
      NS_UNCONSTRAINEDSIZE != aAvailSpace.height) {
    nscoord computedHeight =
      aAvailSpace.height - mComputedBorderPadding.TopBottom();
    computedHeight = NS_MAX(0, computedHeight);
    SetComputedHeight(computedHeight);
  }
}

void
nsTableRowFrame::InitChildReflowState(nsPresContext&         aPresContext,
                                      const nsSize&           aAvailSize,
                                      PRBool                  aBorderCollapse,
                                      nsTableCellReflowState& aReflowState)
{
  nsMargin collapseBorder;
  nsMargin* pCollapseBorder = nsnull;
  if (aBorderCollapse) {
    
    nsBCTableCellFrame* bcCellFrame = (nsBCTableCellFrame*)aReflowState.frame;
    if (bcCellFrame) {
      pCollapseBorder = bcCellFrame->GetBorderWidth(collapseBorder);
    }
  }
  aReflowState.Init(&aPresContext, -1, -1, pCollapseBorder);
  aReflowState.FixUp(aAvailSize);
}

void 
nsTableRowFrame::SetFixedHeight(nscoord aValue)
{
  nscoord height = NS_MAX(0, aValue);
  if (HasFixedHeight()) {
    if (height > mStyleFixedHeight) {
      mStyleFixedHeight = height;
    }
  }
  else {
    mStyleFixedHeight = height;
    if (height > 0) {
      SetHasFixedHeight(PR_TRUE);
    }
  }
}

void 
nsTableRowFrame::SetPctHeight(float  aPctValue,
                              PRBool aForce)
{
  nscoord height = NS_MAX(0, NSToCoordRound(aPctValue * 100.0f));
  if (HasPctHeight()) {
    if ((height > mStylePctHeight) || aForce) {
      mStylePctHeight = height;
    }
  }
  else {
    mStylePctHeight = height;
    if (height > 0.0f) {
      SetHasPctHeight(PR_TRUE);
    }
  }
}



NS_QUERYFRAME_HEAD(nsTableRowFrame)
  NS_QUERYFRAME_ENTRY(nsTableRowFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsHTMLContainerFrame)

nsTableRowFrame::nsTableRowFrame(nsStyleContext* aContext)
  : nsHTMLContainerFrame(aContext)
{
  mBits.mRowIndex = mBits.mFirstInserted = 0;
  ResetHeight(0);
}

nsTableRowFrame::~nsTableRowFrame()
{
}

NS_IMETHODIMP
nsTableRowFrame::Init(nsIContent*      aContent,
                      nsIFrame*        aParent,
                      nsIFrame*        aPrevInFlow)
{
  nsresult  rv;

  
  rv = nsHTMLContainerFrame::Init(aContent, aParent, aPrevInFlow);

  NS_ASSERTION(NS_STYLE_DISPLAY_TABLE_ROW == GetStyleDisplay()->mDisplay,
               "wrong display on table row frame");

  if (aPrevInFlow) {
    
    nsTableRowFrame* rowFrame = (nsTableRowFrame*)aPrevInFlow;
    
    SetRowIndex(rowFrame->GetRowIndex());
  }

  return rv;
}

 void
nsTableRowFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  if (!aOldStyleContext) 
    return;
     
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
    
  if (tableFrame->IsBorderCollapse() &&
      tableFrame->BCRecalcNeeded(aOldStyleContext, GetStyleContext())) {
    nsRect damageArea(0, GetRowIndex(), tableFrame->GetColCount(), 1);
    tableFrame->SetBCDamageArea(damageArea);
  }
  return;
}

NS_IMETHODIMP
nsTableRowFrame::AppendFrames(nsIAtom*        aListName,
                              nsFrameList&    aFrameList)
{
  NS_ASSERTION(!aListName, "unexpected child list");

  
  
  
  const nsFrameList::Slice& newCells = mFrames.AppendFrames(nsnull, aFrameList);

  
  nsTableFrame *tableFrame =  nsTableFrame::GetTableFrame(this);
  for (nsFrameList::Enumerator e(newCells) ; !e.AtEnd(); e.Next()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(e.get());
    NS_ASSERTION(cellFrame, "Unexpected frame");
    if (cellFrame) {
      
      tableFrame->AppendCell(*cellFrame, GetRowIndex());
    }
  }

  PresContext()->PresShell()->FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
  tableFrame->SetGeometryDirty();

  return NS_OK;
}


NS_IMETHODIMP
nsTableRowFrame::InsertFrames(nsIAtom*        aListName,
                              nsIFrame*       aPrevFrame,
                              nsFrameList&    aFrameList)
{
  NS_ASSERTION(!aListName, "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  
  
  
  
  nsIAtom* cellFrameType = (tableFrame->IsBorderCollapse()) ? nsGkAtoms::bcTableCellFrame : nsGkAtoms::tableCellFrame;
  nsTableCellFrame* prevCellFrame = (nsTableCellFrame *)nsTableFrame::GetFrameAtOrBefore(this, aPrevFrame, cellFrameType);
  nsTArray<nsTableCellFrame*> cellChildren;
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(e.get());
    NS_ASSERTION(cellFrame, "Unexpected frame");
    if (cellFrame) {
      cellChildren.AppendElement(cellFrame);
    }
  }
  
  PRInt32 colIndex = -1;
  if (prevCellFrame) {
    prevCellFrame->GetColIndex(colIndex);
  }
  tableFrame->InsertCells(cellChildren, GetRowIndex(), colIndex);

  
  mFrames.InsertFrames(nsnull, aPrevFrame, aFrameList);
  
  PresContext()->PresShell()->FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
  tableFrame->SetGeometryDirty();

  return NS_OK;
}

NS_IMETHODIMP
nsTableRowFrame::RemoveFrame(nsIAtom*        aListName,
                             nsIFrame*       aOldFrame)
{
  NS_ASSERTION(!aListName, "unexpected child list");

  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (tableFrame) {
    nsTableCellFrame *cellFrame = do_QueryFrame(aOldFrame);
    if (cellFrame) {
      PRInt32 colIndex;
      cellFrame->GetColIndex(colIndex);
      
      tableFrame->RemoveCell(cellFrame, GetRowIndex());

      
      mFrames.DestroyFrame(aOldFrame);

      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
      tableFrame->SetGeometryDirty();
    }
    else {
      NS_ERROR("unexpected frame type");
      return NS_ERROR_INVALID_ARG;
    }
  }

  return NS_OK;
}

 nsMargin
nsTableRowFrame::GetUsedMargin() const
{
  return nsMargin(0,0,0,0);
}

 nsMargin
nsTableRowFrame::GetUsedBorder() const
{
  return nsMargin(0,0,0,0);
}

 nsMargin
nsTableRowFrame::GetUsedPadding() const
{
  return nsMargin(0,0,0,0);
}

nscoord 
GetHeightOfRowsSpannedBelowFirst(nsTableCellFrame& aTableCellFrame,
                                 nsTableFrame&     aTableFrame)
{
  nscoord height = 0;
  nscoord cellSpacingY = aTableFrame.GetCellSpacingY();
  PRInt32 rowSpan = aTableFrame.GetEffectiveRowSpan(aTableCellFrame);
  
  nsIFrame* nextRow = aTableCellFrame.GetParent()->GetNextSibling();
  for (PRInt32 rowX = 1; ((rowX < rowSpan) && nextRow);) {
    if (nsGkAtoms::tableRowFrame == nextRow->GetType()) {
      height += nextRow->GetSize().height;
      rowX++;
    }
    height += cellSpacingY;
    nextRow = nextRow->GetNextSibling();
  }
  return height;
}

nsTableCellFrame*  
nsTableRowFrame::GetFirstCell() 
{
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    nsTableCellFrame *cellFrame = do_QueryFrame(childFrame);
    if (cellFrame) {
      return cellFrame;
    }
    childFrame = childFrame->GetNextSibling();
  }
  return nsnull;
}




void
nsTableRowFrame::DidResize()
{
  
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (!tableFrame)
    return;
  
  nsTableIterator iter(*this);
  nsIFrame* childFrame = iter.First();
  
  nsHTMLReflowMetrics desiredSize;
  desiredSize.width = mRect.width;
  desiredSize.height = mRect.height;
  desiredSize.mOverflowArea = nsRect(0, 0, desiredSize.width,
                                     desiredSize.height);

  while (childFrame) {
    nsTableCellFrame *cellFrame = do_QueryFrame(childFrame);
    if (cellFrame) {
      nscoord cellHeight = mRect.height + GetHeightOfRowsSpannedBelowFirst(*cellFrame, *tableFrame);

      
      nsRect cellRect = cellFrame->GetRect();
      nsRect cellOverflowRect = cellFrame->GetOverflowRect();
      if (cellRect.height != cellHeight)
      {
        cellFrame->SetSize(nsSize(cellRect.width, cellHeight));
        nsTableFrame::InvalidateFrame(cellFrame, cellRect, cellOverflowRect,
                                      PR_FALSE);
      }

      
      
      cellFrame->VerticallyAlignChild(mMaxCellAscent);
      
      
      
      ConsiderChildOverflow(desiredSize.mOverflowArea, cellFrame);

      
      
    }
    
    childFrame = iter.Next();
  }
  FinishAndStoreOverflow(&desiredSize);
  if (HasView()) {
    nsContainerFrame::SyncFrameViewAfterReflow(PresContext(), this, GetView(), &desiredSize.mOverflowArea, 0);
  }
  
}



nscoord nsTableRowFrame::GetMaxCellAscent() const
{
  return mMaxCellAscent;
}

nscoord nsTableRowFrame::GetRowBaseline()
{
  if(mMaxCellAscent)
    return mMaxCellAscent;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsTableIterator iter(*this);
  nsIFrame* childFrame = iter.First();
  nscoord ascent = 0;
   while (childFrame) {
    if (IS_TABLE_CELL(childFrame->GetType())) {
      nsIFrame* firstKid = childFrame->GetFirstChild(nsnull);
      ascent = NS_MAX(ascent, firstKid->GetRect().YMost());
    }
    
    childFrame = iter.Next();
  }
  return ascent;
}
nscoord
nsTableRowFrame::GetHeight(nscoord aPctBasis) const
{
  nscoord height = 0;
  if ((aPctBasis > 0) && HasPctHeight()) {
    height = NSToCoordRound(GetPctHeight() * (float)aPctBasis);
  }
  if (HasFixedHeight()) {
    height = NS_MAX(height, GetFixedHeight());
  }
  return NS_MAX(height, GetContentHeight());
}

void 
nsTableRowFrame::ResetHeight(nscoord aFixedHeight)
{
  SetHasFixedHeight(PR_FALSE);
  SetHasPctHeight(PR_FALSE);
  SetFixedHeight(0);
  SetPctHeight(0);
  SetContentHeight(0);

  if (aFixedHeight > 0) {
    SetFixedHeight(aFixedHeight);
  }

  mMaxCellAscent = 0;
  mMaxCellDescent = 0;
}

void
nsTableRowFrame::UpdateHeight(nscoord           aHeight,
                              nscoord           aAscent,
                              nscoord           aDescent,
                              nsTableFrame*     aTableFrame,
                              nsTableCellFrame* aCellFrame)
{
  if (!aTableFrame || !aCellFrame) {
    NS_ASSERTION(PR_FALSE , "invalid call");
    return;
  }

  if (aHeight != NS_UNCONSTRAINEDSIZE) {
    if (!(aCellFrame->HasVerticalAlignBaseline())) { 
      if (GetHeight() < aHeight) {
        PRInt32 rowSpan = aTableFrame->GetEffectiveRowSpan(*aCellFrame);
        if (rowSpan == 1) {
          SetContentHeight(aHeight);
        }
      }
    }
    else { 
      NS_ASSERTION((aAscent != NS_UNCONSTRAINEDSIZE) && (aDescent != NS_UNCONSTRAINEDSIZE), "invalid call");
      
      if (mMaxCellAscent < aAscent) {
        mMaxCellAscent = aAscent;
      }
      
      if (mMaxCellDescent < aDescent) {
        PRInt32 rowSpan = aTableFrame->GetEffectiveRowSpan(*aCellFrame);
        if (rowSpan == 1) {
          mMaxCellDescent = aDescent;
        }
      }
      
      if (GetHeight() < mMaxCellAscent + mMaxCellDescent) {
        SetContentHeight(mMaxCellAscent + mMaxCellDescent);
      }
    }
  }
}

nscoord
nsTableRowFrame::CalcHeight(const nsHTMLReflowState& aReflowState)
{
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (!tableFrame)
    return 0;

  nscoord computedHeight = (NS_UNCONSTRAINEDSIZE == aReflowState.ComputedHeight())
                            ? 0 : aReflowState.ComputedHeight();
  ResetHeight(computedHeight);

  const nsStylePosition* position = GetStylePosition();
  if (eStyleUnit_Coord == position->mHeight.GetUnit()) {
    SetFixedHeight(position->mHeight.GetCoordValue());
  }
  else if (eStyleUnit_Percent == position->mHeight.GetUnit()) {
    SetPctHeight(position->mHeight.GetPercentValue());
  }

  for (nsIFrame* kidFrame = mFrames.FirstChild(); kidFrame;
       kidFrame = kidFrame->GetNextSibling()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
    if (cellFrame) {
      nscoord availWidth = cellFrame->GetPriorAvailWidth();
      nsSize desSize = cellFrame->GetDesiredSize();
      if ((NS_UNCONSTRAINEDSIZE == aReflowState.availableHeight) && !GetPrevInFlow()) {
        CalculateCellActualSize(kidFrame, desSize.width, desSize.height, availWidth);
      }
      
      nscoord ascent;
       if (!kidFrame->GetFirstChild(nsnull)->GetFirstChild(nsnull))
         ascent = desSize.height;
       else
         ascent = cellFrame->GetCellBaseline();
      nscoord descent = desSize.height - ascent;
      UpdateHeight(desSize.height, ascent, descent, tableFrame, cellFrame);
    }
  }
  return GetHeight();
}







class nsDisplayTableRowBackground : public nsDisplayTableItem {
public:
  nsDisplayTableRowBackground(nsTableRowFrame* aFrame) : nsDisplayTableItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayTableRowBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTableRowBackground() {
    MOZ_COUNT_DTOR(nsDisplayTableRowBackground);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsIRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("TableRowBackground")
};

void
nsDisplayTableRowBackground::Paint(nsDisplayListBuilder* aBuilder,
                                   nsIRenderingContext* aCtx) {
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(mFrame);

  nsPoint pt = aBuilder->ToReferenceFrame(mFrame);
  TableBackgroundPainter painter(tableFrame,
                                 TableBackgroundPainter::eOrigin_TableRow,
                                 mFrame->PresContext(), *aCtx,
                                 mVisibleRect, pt,
                                 aBuilder->GetBackgroundPaintFlags());
  painter.PaintRow(static_cast<nsTableRowFrame*>(mFrame));
}

NS_IMETHODIMP
nsTableRowFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists)
{
  if (!IsVisibleInSelection(aBuilder))
    return NS_OK;

  PRBool isRoot = aBuilder->IsAtRootOfPseudoStackingContext();
  nsDisplayTableItem* item = nsnull;
  if (isRoot) {
    
    
    
    
    
    
    item = new (aBuilder) nsDisplayTableRowBackground(this);
    nsresult rv = aLists.BorderBackground()->AppendNewToTop(item);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return nsTableFrame::DisplayGenericTablePart(aBuilder, this, aDirtyRect, aLists, item);
}

PRIntn
nsTableRowFrame::GetSkipSides() const
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





nsresult
nsTableRowFrame::CalculateCellActualSize(nsIFrame* aCellFrame,
                                         nscoord&  aDesiredWidth,
                                         nscoord&  aDesiredHeight,
                                         nscoord   aAvailWidth)
{
  nscoord specifiedHeight = 0;
  
  
  const nsStylePosition* position = aCellFrame->GetStylePosition();

  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (!tableFrame)
    return NS_ERROR_NULL_POINTER;
  
  PRInt32 rowSpan = tableFrame->GetEffectiveRowSpan((nsTableCellFrame&)*aCellFrame);
  
  switch (position->mHeight.GetUnit()) {
    case eStyleUnit_Coord:
      specifiedHeight = position->mHeight.GetCoordValue();
      if (1 == rowSpan) 
        SetFixedHeight(specifiedHeight);
      break;
    case eStyleUnit_Percent: {
      if (1 == rowSpan) 
        SetPctHeight(position->mHeight.GetPercentValue());
      
      break;
    }
    case eStyleUnit_Auto:
    default:
      break;
  }

  
  if (specifiedHeight > aDesiredHeight)
    aDesiredHeight = specifiedHeight;
 
  if ((0 == aDesiredWidth) && (NS_UNCONSTRAINEDSIZE != aAvailWidth)) { 
    aDesiredWidth = aAvailWidth;
  } 

  return NS_OK;
}



static nscoord
CalcAvailWidth(nsTableFrame&     aTableFrame,
               nsTableCellFrame& aCellFrame,
               nscoord           aCellSpacingX)
{
  nscoord cellAvailWidth = 0;
  PRInt32 colIndex;
  aCellFrame.GetColIndex(colIndex);
  PRInt32 colspan = aTableFrame.GetEffectiveColSpan(aCellFrame);
  NS_ASSERTION(colspan > 0, "effective colspan should be positive");

  for (PRInt32 spanX = 0; spanX < colspan; spanX++) {
    cellAvailWidth += aTableFrame.GetColumnWidth(colIndex + spanX);
    if (spanX > 0 &&
        aTableFrame.ColumnHasCellSpacingBefore(colIndex + spanX)) {
      cellAvailWidth += aCellSpacingX;
    }
  }
  return cellAvailWidth;
}

nscoord
GetSpaceBetween(PRInt32       aPrevColIndex,
                PRInt32       aColIndex,
                PRInt32       aColSpan,
                nsTableFrame& aTableFrame,
                nscoord       aCellSpacingX,
                PRBool        aIsLeftToRight,
                PRBool        aCheckVisibility)
{
  nscoord space = 0;
  PRInt32 colX;
  if (aIsLeftToRight) {
    for (colX = aPrevColIndex + 1; aColIndex > colX; colX++) {
      PRBool isCollapsed = PR_FALSE;
      if (!aCheckVisibility) {
        space += aTableFrame.GetColumnWidth(colX);
      }
      else {
        nsTableColFrame* colFrame = aTableFrame.GetColFrame(colX);
        const nsStyleVisibility* colVis = colFrame->GetStyleVisibility();
        PRBool collapseCol = (NS_STYLE_VISIBILITY_COLLAPSE == colVis->mVisible);
        nsIFrame* cgFrame = colFrame->GetParent();
        const nsStyleVisibility* groupVis = cgFrame->GetStyleVisibility();
        PRBool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE ==
                                groupVis->mVisible);
        isCollapsed = collapseCol || collapseGroup;
        if (!isCollapsed)
          space += aTableFrame.GetColumnWidth(colX);
      }
      if (!isCollapsed && aTableFrame.ColumnHasCellSpacingBefore(colX)) {
        space += aCellSpacingX;
      }
    }
  } 
  else {
    PRInt32 lastCol = aColIndex + aColSpan - 1;
    for (colX = aPrevColIndex - 1; colX > lastCol; colX--) {
      PRBool isCollapsed = PR_FALSE;
      if (!aCheckVisibility) {
        space += aTableFrame.GetColumnWidth(colX);
      }
      else {
        nsTableColFrame* colFrame = aTableFrame.GetColFrame(colX);
        const nsStyleVisibility* colVis = colFrame->GetStyleVisibility();
        PRBool collapseCol = (NS_STYLE_VISIBILITY_COLLAPSE == colVis->mVisible);
        nsIFrame* cgFrame = colFrame->GetParent();
        const nsStyleVisibility* groupVis = cgFrame->GetStyleVisibility();
        PRBool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE ==
                                groupVis->mVisible);
        isCollapsed = collapseCol || collapseGroup;
        if (!isCollapsed)
          space += aTableFrame.GetColumnWidth(colX);
      }
      if (!isCollapsed && aTableFrame.ColumnHasCellSpacingBefore(colX)) {
        space += aCellSpacingX;
      }
    }
  }
  return space;
}


static
nscoord CalcHeightFromUnpaginatedHeight(nsPresContext*   aPresContext,
                                        nsTableRowFrame& aRow)
{
  nscoord height = 0;
  nsTableRowFrame* firstInFlow = (nsTableRowFrame*)aRow.GetFirstInFlow();
  if (!firstInFlow) ABORT1(0);
  if (firstInFlow->HasUnpaginatedHeight()) {
    height = firstInFlow->GetUnpaginatedHeight(aPresContext);
    for (nsIFrame* prevInFlow = aRow.GetPrevInFlow(); prevInFlow;
         prevInFlow = prevInFlow->GetPrevInFlow()) {
      height -= prevInFlow->GetSize().height;
    }
  }
  return NS_MAX(height, 0);
}

NS_METHOD 
nsTableRowFrame::ReflowChildren(nsPresContext*          aPresContext,
                                nsHTMLReflowMetrics&     aDesiredSize,
                                const nsHTMLReflowState& aReflowState,
                                nsTableFrame&            aTableFrame,
                                nsReflowStatus&          aStatus)
{
  aStatus = NS_FRAME_COMPLETE;

  PRBool borderCollapse = (((nsTableFrame*)aTableFrame.GetFirstInFlow())->IsBorderCollapse());

  
  PRBool isPaginated = aPresContext->IsPaginated();

  nsresult rv = NS_OK;

  nscoord cellSpacingX = aTableFrame.GetCellSpacingX();
  PRInt32 cellColSpan = 1;  
  
  nsTableIterator iter(*this);
  
  PRInt32 firstPrevColIndex = (iter.IsLeftToRight()) ? -1 : aTableFrame.GetColCount();
  PRInt32 prevColIndex  = firstPrevColIndex;
  nscoord x = 0; 

  
  nscoord cellMaxHeight = 0;

  
  for (nsIFrame* kidFrame = iter.First(); kidFrame; kidFrame = iter.Next()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
    if (!cellFrame) {
      
      NS_NOTREACHED("yikes, a non-row child");

      
      nsTableCellReflowState kidReflowState(aPresContext, aReflowState,
                                            kidFrame, nsSize(0,0), PR_FALSE);
      InitChildReflowState(*aPresContext, nsSize(0,0), PR_FALSE, kidReflowState);
      nsHTMLReflowMetrics desiredSize;
      nsReflowStatus  status;
      ReflowChild(kidFrame, aPresContext, desiredSize, kidReflowState, 0, 0, 0, status);
      kidFrame->DidReflow(aPresContext, nsnull, NS_FRAME_REFLOW_FINISHED);

      continue;
    }

    
    PRBool doReflowChild = PR_TRUE;
    if (!aReflowState.ShouldReflowAllKids() &&
        !aTableFrame.IsGeometryDirty() &&
        !NS_SUBTREE_DIRTY(kidFrame)) {
      if (!aReflowState.mFlags.mSpecialHeightReflow)
        doReflowChild = PR_FALSE;
    }
    else if ((NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight)) {
      
      
      if (aTableFrame.GetEffectiveRowSpan(*cellFrame) > 1) {
        doReflowChild = PR_FALSE;
      }
    }
    if (aReflowState.mFlags.mSpecialHeightReflow) {
      if (!isPaginated && !(cellFrame->GetStateBits() &
                            NS_FRAME_CONTAINS_RELATIVE_HEIGHT)) {
        continue;
      }
    }

    PRInt32 cellColIndex;
    cellFrame->GetColIndex(cellColIndex);
    cellColSpan = aTableFrame.GetEffectiveColSpan(*cellFrame);

    
    if ((iter.IsLeftToRight() && (prevColIndex != (cellColIndex - 1))) ||
        (!iter.IsLeftToRight() && (prevColIndex != cellColIndex + cellColSpan))) {
      x += GetSpaceBetween(prevColIndex, cellColIndex, cellColSpan, aTableFrame, 
                           cellSpacingX, iter.IsLeftToRight(), PR_FALSE);
    }

    
    prevColIndex = (iter.IsLeftToRight()) ? cellColIndex + (cellColSpan - 1) : cellColIndex;

    
    nsRect kidRect = kidFrame->GetRect();
    nsRect kidOverflowRect = kidFrame->GetOverflowRect();
    PRBool firstReflow =
      (kidFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;

    if (doReflowChild) {
      
      nscoord availCellWidth =
        CalcAvailWidth(aTableFrame, *cellFrame, cellSpacingX);

      nsHTMLReflowMetrics desiredSize;

      
      
      
      
      
      nsSize cellDesiredSize = cellFrame->GetDesiredSize();
      if ((availCellWidth != cellFrame->GetPriorAvailWidth())       ||
          (cellDesiredSize.width > cellFrame->GetPriorAvailWidth()) ||
          (GetStateBits() & NS_FRAME_IS_DIRTY)                      ||
          isPaginated                                               ||
          NS_SUBTREE_DIRTY(cellFrame)                               ||
          
          (cellFrame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT) ||
          HasPctHeight()) {
        
        
        nsSize  kidAvailSize(availCellWidth, aReflowState.availableHeight);

        
        nsTableCellReflowState kidReflowState(aPresContext, aReflowState, 
                                              kidFrame, kidAvailSize, PR_FALSE);
        InitChildReflowState(*aPresContext, kidAvailSize, borderCollapse,
                             kidReflowState);

        nsReflowStatus status;
        rv = ReflowChild(kidFrame, aPresContext, desiredSize, kidReflowState,
                         x, 0, NS_FRAME_INVALIDATE_ON_MOVE, status);

        
        
        if (NS_FRAME_IS_NOT_COMPLETE(status)) {
          aStatus = NS_FRAME_NOT_COMPLETE;
        }
      }
      else {
        if (x != kidRect.x) {
          kidFrame->InvalidateOverflowRect();
        }
        
        desiredSize.width = cellDesiredSize.width;
        desiredSize.height = cellDesiredSize.height;
        if (cellFrame->HasOverflowRect())
          desiredSize.mOverflowArea = cellFrame->GetOverflowRect();
        else
          desiredSize.mOverflowArea.SetRect(0, 0, cellDesiredSize.width,
                                            cellDesiredSize.height);
        
        
        
        if (!aTableFrame.GetStyleDisplay()->IsFloating()) {
          
          
          
          nsTableFrame::RePositionViews(kidFrame);
        }
      }
      
      if (NS_UNCONSTRAINEDSIZE == aReflowState.availableHeight) {
        if (!GetPrevInFlow()) {
          
          
          
          CalculateCellActualSize(kidFrame, desiredSize.width, 
                                  desiredSize.height, availCellWidth);
        }
        
        nscoord ascent;
        if (!kidFrame->GetFirstChild(nsnull)->GetFirstChild(nsnull))
          ascent = desiredSize.height;
        else
          ascent = ((nsTableCellFrame *)kidFrame)->GetCellBaseline();
        nscoord descent = desiredSize.height - ascent;
        UpdateHeight(desiredSize.height, ascent, descent, &aTableFrame, cellFrame);
      }
      else {
        cellMaxHeight = NS_MAX(cellMaxHeight, desiredSize.height);
        PRInt32 rowSpan = aTableFrame.GetEffectiveRowSpan((nsTableCellFrame&)*kidFrame);
        if (1 == rowSpan) {
          SetContentHeight(cellMaxHeight);
        }
      }

      
      desiredSize.width = availCellWidth;

      FinishReflowChild(kidFrame, aPresContext, nsnull, desiredSize, x, 0, 0);

      nsTableFrame::InvalidateFrame(kidFrame, kidRect, kidOverflowRect,
                                    firstReflow);
      
      x += desiredSize.width;  
    }
    else {
      if (kidRect.x != x) {
        
        kidFrame->InvalidateOverflowRect();
        
        kidFrame->SetPosition(nsPoint(x, kidRect.y));
        nsTableFrame::RePositionViews(kidFrame);
        
        kidFrame->InvalidateOverflowRect();
      }
      
      x += kidRect.width;

      if (kidFrame->GetNextInFlow()) {
        aStatus = NS_FRAME_NOT_COMPLETE;
      }
    }
    ConsiderChildOverflow(aDesiredSize.mOverflowArea, kidFrame);
    x += cellSpacingX;
  }

  
  aDesiredSize.width = aReflowState.availableWidth;

  if (aReflowState.mFlags.mSpecialHeightReflow) {
    aDesiredSize.height = mRect.height;
  }
  else if (NS_UNCONSTRAINEDSIZE == aReflowState.availableHeight) {
    aDesiredSize.height = CalcHeight(aReflowState);
    if (GetPrevInFlow()) {
      nscoord height = CalcHeightFromUnpaginatedHeight(aPresContext, *this);
      aDesiredSize.height = NS_MAX(aDesiredSize.height, height);
    }
    else {
      if (isPaginated && HasStyleHeight()) {
        
        SetHasUnpaginatedHeight(PR_TRUE);
        SetUnpaginatedHeight(aPresContext, aDesiredSize.height);
      }
      if (isPaginated && HasUnpaginatedHeight()) {
        aDesiredSize.height = NS_MAX(aDesiredSize.height, GetUnpaginatedHeight(aPresContext));
      }
    }
  }
  else { 
    
    
    nscoord styleHeight = CalcHeightFromUnpaginatedHeight(aPresContext, *this);
    if (styleHeight > aReflowState.availableHeight) {
      styleHeight = aReflowState.availableHeight;
      NS_FRAME_SET_INCOMPLETE(aStatus);
    }
    aDesiredSize.height = NS_MAX(cellMaxHeight, styleHeight);
  }
  nsRect rowRect(0, 0, aDesiredSize.width, aDesiredSize.height);
  aDesiredSize.mOverflowArea.UnionRect(aDesiredSize.mOverflowArea, rowRect);
  FinishAndStoreOverflow(&aDesiredSize);
  return rv;
}




NS_METHOD
nsTableRowFrame::Reflow(nsPresContext*          aPresContext,
                        nsHTMLReflowMetrics&     aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsTableRowFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  nsresult rv = NS_OK;

  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (!tableFrame)
    return NS_ERROR_NULL_POINTER;

  const nsStyleVisibility* rowVis = GetStyleVisibility();
  PRBool collapseRow = (NS_STYLE_VISIBILITY_COLLAPSE == rowVis->mVisible);
  if (collapseRow) {
    tableFrame->SetNeedToCollapse(PR_TRUE);
  }

  
  nsTableFrame::CheckRequestSpecialHeightReflow(aReflowState);

  
  InitHasCellWithStyleHeight(tableFrame);

  rv = ReflowChildren(aPresContext, aDesiredSize, aReflowState, *tableFrame,
                      aStatus);

  
  aDesiredSize.width = aReflowState.availableWidth;

  
  
  if (!(GetParent()->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    CheckInvalidateSizeChange(aDesiredSize);
  }

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return rv;
}






nscoord 
nsTableRowFrame::ReflowCellFrame(nsPresContext*          aPresContext,
                                 const nsHTMLReflowState& aReflowState,
                                 PRBool                   aIsTopOfPage,
                                 nsTableCellFrame*        aCellFrame,
                                 nscoord                  aAvailableHeight,
                                 nsReflowStatus&          aStatus)
{
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (!tableFrame)
    ABORT1(NS_ERROR_NULL_POINTER);

  
  nsRect cellRect = aCellFrame->GetRect();
  nsRect cellOverflowRect = aCellFrame->GetOverflowRect();
  
  nsSize  availSize(cellRect.width, aAvailableHeight);
  PRBool borderCollapse = ((nsTableFrame*)tableFrame->GetFirstInFlow())->IsBorderCollapse();
  nsTableCellReflowState cellReflowState(aPresContext, aReflowState,
                                         aCellFrame, availSize, PR_FALSE);
  InitChildReflowState(*aPresContext, availSize, borderCollapse, cellReflowState);
  cellReflowState.mFlags.mIsTopOfPage = aIsTopOfPage;

  nsHTMLReflowMetrics desiredSize;

  ReflowChild(aCellFrame, aPresContext, desiredSize, cellReflowState,
              0, 0, NS_FRAME_NO_MOVE_FRAME, aStatus);
  PRBool fullyComplete = NS_FRAME_IS_COMPLETE(aStatus) && !NS_FRAME_IS_TRUNCATED(aStatus);
  if (fullyComplete) {
    desiredSize.height = aAvailableHeight;
  }
  aCellFrame->SetSize(nsSize(cellRect.width, desiredSize.height));

  
  
  
  if (fullyComplete) {
    aCellFrame->VerticallyAlignChild(mMaxCellAscent);
  }
  
  nsTableFrame::InvalidateFrame(aCellFrame, cellRect,
                                cellOverflowRect,
                                (aCellFrame->GetStateBits() &
                                   NS_FRAME_FIRST_REFLOW) != 0);
  
  aCellFrame->DidReflow(aPresContext, nsnull, NS_FRAME_REFLOW_FINISHED);

  return desiredSize.height;
}

nscoord
nsTableRowFrame::CollapseRowIfNecessary(nscoord aRowOffset,
                                        nscoord aWidth,
                                        PRBool  aCollapseGroup,
                                        PRBool& aDidCollapse)
{
  const nsStyleVisibility* rowVis = GetStyleVisibility();
  PRBool collapseRow = (NS_STYLE_VISIBILITY_COLLAPSE == rowVis->mVisible);
  nsTableFrame* tableFrame = static_cast<nsTableFrame*>(nsTableFrame::GetTableFrame(this)->GetFirstInFlow());
  if (!tableFrame)
      return 0;
  if (collapseRow) {
    tableFrame->SetNeedToCollapse(PR_TRUE);
  }

  if (aRowOffset != 0) {
    
    InvalidateOverflowRect();
  }
  
  nsRect rowRect = GetRect();
  nsRect oldRect = rowRect;
  nsRect oldOverflowRect = GetOverflowRect();
  
  rowRect.y -= aRowOffset;
  rowRect.width  = aWidth;
  nsRect overflowArea(0, 0, 0, 0);
  nscoord shift = 0;
  nscoord cellSpacingX = tableFrame->GetCellSpacingX();
  nscoord cellSpacingY = tableFrame->GetCellSpacingY();

  if (aCollapseGroup || collapseRow) {
    nsTableCellFrame* cellFrame = GetFirstCell();
    aDidCollapse = PR_TRUE;
    shift = rowRect.height + cellSpacingY;
    while (cellFrame) {
      nsRect cRect = cellFrame->GetRect();
      
      
      
      
      if (aRowOffset == 0) {
        Invalidate(cRect);
      }
      cRect.height = 0;
      cellFrame->SetRect(cRect);
      cellFrame = cellFrame->GetNextCell();
    }
    rowRect.height = 0;
  }
  else { 
    nsTableIterator iter(*this);
    
    
    PRInt32 firstPrevColIndex = (iter.IsLeftToRight()) ? -1 :
                                tableFrame->GetColCount();
    PRInt32 prevColIndex  = firstPrevColIndex;
    nscoord x = 0; 

    PRInt32 colIncrement = iter.IsLeftToRight() ? 1 : -1;

    

    nsIFrame* kidFrame = iter.First();
    while (kidFrame) {
      nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
      if (cellFrame) {
        PRInt32 cellColIndex;
        cellFrame->GetColIndex(cellColIndex);
        PRInt32 cellColSpan = tableFrame->GetEffectiveColSpan(*cellFrame);

        
        
        if ((iter.IsLeftToRight() && (prevColIndex != (cellColIndex - 1))) ||
            (!iter.IsLeftToRight() &&
             (prevColIndex != cellColIndex + cellColSpan))) {
          x += GetSpaceBetween(prevColIndex, cellColIndex, cellColSpan,
                               *tableFrame, cellSpacingX, iter.IsLeftToRight(),
                               PR_TRUE);
        }
        nsRect cRect(x, 0, 0, rowRect.height);

        
        
        prevColIndex = (iter.IsLeftToRight()) ?
                       cellColIndex + (cellColSpan - 1) : cellColIndex;
        PRInt32 startIndex = (iter.IsLeftToRight()) ?
                             cellColIndex : cellColIndex + (cellColSpan - 1);
        PRInt32 actualColSpan = cellColSpan;
        PRBool isVisible = PR_FALSE;
        for (PRInt32 colX = startIndex; actualColSpan > 0;
             colX += colIncrement, actualColSpan--) {

          nsTableColFrame* colFrame = tableFrame->GetColFrame(colX);
          const nsStyleVisibility* colVis = colFrame->GetStyleVisibility();
          PRBool collapseCol = (NS_STYLE_VISIBILITY_COLLAPSE ==
                                colVis->mVisible);
          nsIFrame* cgFrame = colFrame->GetParent();
          const nsStyleVisibility* groupVis = cgFrame->GetStyleVisibility();
          PRBool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE ==
                                  groupVis->mVisible);
          PRBool isCollapsed = collapseCol || collapseGroup;
          if (!isCollapsed) {
            cRect.width += tableFrame->GetColumnWidth(colX);
            isVisible = PR_TRUE;
            if ((actualColSpan > 1)) {
              nsTableColFrame* nextColFrame =
                tableFrame->GetColFrame(colX + colIncrement);
              const nsStyleVisibility* nextColVis =
              nextColFrame->GetStyleVisibility();
              if ( (NS_STYLE_VISIBILITY_COLLAPSE != nextColVis->mVisible) &&
                  tableFrame->ColumnHasCellSpacingBefore(colX + colIncrement)) {
                cRect.width += cellSpacingX;
              }
            }
          }
        }
        x += cRect.width;
        if (isVisible)
          x += cellSpacingX;
        PRInt32 actualRowSpan = tableFrame->GetEffectiveRowSpan(*cellFrame);
        nsTableRowFrame* rowFrame = GetNextRow();
        for (actualRowSpan--; actualRowSpan > 0 && rowFrame; actualRowSpan--) {
          const nsStyleVisibility* nextRowVis = rowFrame->GetStyleVisibility();
          PRBool collapseNextRow = (NS_STYLE_VISIBILITY_COLLAPSE ==
                                    nextRowVis->mVisible);
          if (!collapseNextRow) {
            nsRect nextRect = rowFrame->GetRect();
            cRect.height += nextRect.height + cellSpacingY;
          }
          rowFrame = rowFrame->GetNextRow();
        }

        nsRect oldCellRect = cellFrame->GetRect();
        nsRect oldCellOverflowRect = cellFrame->GetOverflowRect();

        if (aRowOffset == 0 && cRect.TopLeft() != oldCellRect.TopLeft()) {
          
          cellFrame->InvalidateOverflowRect();
        }
        
        cellFrame->SetRect(cRect);

        
        
        nsRect cellOverflow = nsRect(0, 0, cRect.width, cRect.height);
        cellFrame->FinishAndStoreOverflow(&cellOverflow, nsSize(cRect.width,
                                              cRect.height));
        nsTableFrame::RePositionViews(cellFrame);
        ConsiderChildOverflow(overflowArea, cellFrame);
                
        if (aRowOffset == 0) {
          nsTableFrame::InvalidateFrame(cellFrame, oldCellRect,
                                        oldCellOverflowRect, PR_FALSE);
        }
      }
      kidFrame = iter.Next(); 
    }
  }

  SetRect(rowRect);
  overflowArea.UnionRect(nsRect(0,0,rowRect.width, rowRect.height),
                         overflowArea);
  FinishAndStoreOverflow(&overflowArea, nsSize(rowRect.width,
                                              rowRect.height));

  nsTableFrame::RePositionViews(this);
  nsTableFrame::InvalidateFrame(this, oldRect, oldOverflowRect, PR_FALSE);
  return shift;
}






void 
nsTableRowFrame::InsertCellFrame(nsTableCellFrame* aFrame,
                                 PRInt32           aColIndex)
{
  
  nsTableCellFrame* priorCell = nsnull;
  for (nsIFrame* child = mFrames.FirstChild(); child;
       child = child->GetNextSibling()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(child);
    if (cellFrame) {
      PRInt32 colIndex;
      cellFrame->GetColIndex(colIndex);
      if (colIndex < aColIndex) {
        priorCell = cellFrame;
      }
      else break;
    }
  }
  mFrames.InsertFrame(this, priorCell, aFrame);
}

nsIAtom*
nsTableRowFrame::GetType() const
{
  return nsGkAtoms::tableRowFrame;
}

nsTableRowFrame*  
nsTableRowFrame::GetNextRow() const
{
  nsIFrame* childFrame = GetNextSibling();
  while (childFrame) {
    nsTableRowFrame *rowFrame = do_QueryFrame(childFrame);
    if (rowFrame) {
	  NS_ASSERTION(NS_STYLE_DISPLAY_TABLE_ROW == childFrame->GetStyleDisplay()->mDisplay, "wrong display type on rowframe");
      return rowFrame;
    }
    childFrame = childFrame->GetNextSibling();
  }
  return nsnull;
}

void 
nsTableRowFrame::SetUnpaginatedHeight(nsPresContext* aPresContext,
                                      nscoord        aValue)
{
  NS_ASSERTION(!GetPrevInFlow(), "program error");
  
  nscoord* value = (nscoord*)nsTableFrame::GetProperty(this, nsGkAtoms::rowUnpaginatedHeightProperty, PR_TRUE);
  if (value) {
    *value = aValue;
  }
}

nscoord
nsTableRowFrame::GetUnpaginatedHeight(nsPresContext* aPresContext)
{
  
  nscoord* value = (nscoord*)nsTableFrame::GetProperty(GetFirstInFlow(), nsGkAtoms::rowUnpaginatedHeightProperty);
  if (value) 
    return *value;
  else 
    return 0;
}

void nsTableRowFrame::SetContinuousBCBorderWidth(PRUint8     aForSide,
                                                 BCPixelSize aPixelValue)
{
  switch (aForSide) {
    case NS_SIDE_RIGHT:
      mRightContBorderWidth = aPixelValue;
      return;
    case NS_SIDE_TOP:
      mTopContBorderWidth = aPixelValue;
      return;
    case NS_SIDE_LEFT:
      mLeftContBorderWidth = aPixelValue;
      return;
    default:
      NS_ERROR("invalid NS_SIDE arg");
  }
}






void nsTableRowFrame::InitHasCellWithStyleHeight(nsTableFrame* aTableFrame)
{
  nsTableIterator iter(*this);

  for (nsIFrame* kidFrame = iter.First(); kidFrame; kidFrame = iter.Next()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
    if (!cellFrame) {
      NS_NOTREACHED("Table row has a non-cell child.");
      continue;
    }
    
    if (aTableFrame->GetEffectiveRowSpan(*cellFrame) == 1 &&
        cellFrame->GetStylePosition()->mHeight.GetUnit() != eStyleUnit_Auto) {
      AddStateBits(NS_ROW_HAS_CELL_WITH_STYLE_HEIGHT);
      return;
    }
  }
  RemoveStateBits(NS_ROW_HAS_CELL_WITH_STYLE_HEIGHT);
}



nsIFrame* 
NS_NewTableRowFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTableRowFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTableRowFrame)

#ifdef DEBUG
NS_IMETHODIMP
nsTableRowFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("TableRow"), aResult);
}
#endif
