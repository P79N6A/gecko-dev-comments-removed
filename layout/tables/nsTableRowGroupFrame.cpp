




































#include "nsCOMPtr.h"
#include "nsTableRowGroupFrame.h"
#include "nsTableRowFrame.h"
#include "nsTableFrame.h"
#include "nsTableCellFrame.h"
#include "nsIRenderingContext.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIContent.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsCSSRendering.h"
#include "nsHTMLParts.h"
#include "nsCSSFrameConstructor.h"
#include "nsDisplayList.h"

#include "nsCellMap.h"

nsTableRowGroupFrame::nsTableRowGroupFrame(nsStyleContext* aContext):
  nsHTMLContainerFrame(aContext)
{
  SetRepeatable(PR_FALSE);
}

nsTableRowGroupFrame::~nsTableRowGroupFrame()
{
}

NS_QUERYFRAME_HEAD(nsTableRowGroupFrame)
  NS_QUERYFRAME_ENTRY(nsTableRowGroupFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsHTMLContainerFrame)

 PRBool
nsTableRowGroupFrame::IsContainingBlock() const
{
  return PR_TRUE;
}

PRInt32
nsTableRowGroupFrame::GetRowCount()
{  
  PRInt32 count = 0; 

  
  nsIFrame* childFrame = GetFirstFrame();
  while (PR_TRUE) {
    if (!childFrame)
      break;
    if (NS_STYLE_DISPLAY_TABLE_ROW == childFrame->GetStyleDisplay()->mDisplay)
      count++;
    GetNextFrame(childFrame, &childFrame);
  }
  return count;
}

PRInt32 nsTableRowGroupFrame::GetStartRowIndex()
{
  PRInt32 result = -1;
  nsIFrame* childFrame = GetFirstFrame();
  while (PR_TRUE) {
    if (!childFrame)
      break;
    if (NS_STYLE_DISPLAY_TABLE_ROW == childFrame->GetStyleDisplay()->mDisplay) {
      result = ((nsTableRowFrame *)childFrame)->GetRowIndex();
      break;
    }
    GetNextFrame(childFrame, &childFrame);
  }
  
  if (-1 == result) {
    nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
    if (tableFrame) {
      return tableFrame->GetStartRowIndex(*this);
    }
  }
      
  return result;
}

void  nsTableRowGroupFrame::AdjustRowIndices(PRInt32 aRowIndex,
                                             PRInt32 anAdjustment)
{
  nsIFrame* rowFrame = GetFirstChild(nsnull);
  for ( ; rowFrame; rowFrame = rowFrame->GetNextSibling()) {
    if (NS_STYLE_DISPLAY_TABLE_ROW==rowFrame->GetStyleDisplay()->mDisplay) {
      PRInt32 index = ((nsTableRowFrame*)rowFrame)->GetRowIndex();
      if (index >= aRowIndex)
        ((nsTableRowFrame *)rowFrame)->SetRowIndex(index+anAdjustment);
    }
  }
}
nsresult
nsTableRowGroupFrame::InitRepeatedFrame(nsPresContext*        aPresContext,
                                        nsTableRowGroupFrame* aHeaderFooterFrame)
{
  nsTableRowFrame* copyRowFrame = GetFirstRow();
  nsTableRowFrame* originalRowFrame = aHeaderFooterFrame->GetFirstRow();
  AddStateBits(NS_REPEATED_ROW_OR_ROWGROUP);
  while (copyRowFrame && originalRowFrame) {
    copyRowFrame->AddStateBits(NS_REPEATED_ROW_OR_ROWGROUP);
    int rowIndex = originalRowFrame->GetRowIndex();
    copyRowFrame->SetRowIndex(rowIndex);

    
    nsTableCellFrame* originalCellFrame = originalRowFrame->GetFirstCell();
    nsTableCellFrame* copyCellFrame     = copyRowFrame->GetFirstCell();
    while (copyCellFrame && originalCellFrame) {
      NS_ASSERTION(originalCellFrame->GetContent() == copyCellFrame->GetContent(),
                   "cell frames have different content");
      PRInt32 colIndex;
      originalCellFrame->GetColIndex(colIndex);
      copyCellFrame->SetColIndex(colIndex);
        
      
      copyCellFrame     = copyCellFrame->GetNextCell();
      originalCellFrame = originalCellFrame->GetNextCell();
    }
    
    
    originalRowFrame = originalRowFrame->GetNextRow();
    copyRowFrame = copyRowFrame->GetNextRow();
  }

  return NS_OK;
}







class nsDisplayTableRowGroupBackground : public nsDisplayTableItem {
public:
  nsDisplayTableRowGroupBackground(nsTableRowGroupFrame* aFrame) : nsDisplayTableItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayTableRowGroupBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTableRowGroupBackground() {
    MOZ_COUNT_DTOR(nsDisplayTableRowGroupBackground);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);

  NS_DISPLAY_DECL_NAME("TableRowGroupBackground")
};

void
nsDisplayTableRowGroupBackground::Paint(nsDisplayListBuilder* aBuilder,
    nsIRenderingContext* aCtx, const nsRect& aDirtyRect) {
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(mFrame);

  nsPoint pt = aBuilder->ToReferenceFrame(mFrame);
  TableBackgroundPainter painter(tableFrame,
                                 TableBackgroundPainter::eOrigin_TableRowGroup,
                                 mFrame->PresContext(), *aCtx,
                                 aDirtyRect, pt,
                                 aBuilder->GetBackgroundPaintFlags());
  painter.PaintRowGroup(static_cast<nsTableRowGroupFrame*>(mFrame));
}


static nsresult
DisplayRows(nsDisplayListBuilder* aBuilder, nsFrame* aFrame,
            const nsRect& aDirtyRect, const nsDisplayListSet& aLists)
{
  nscoord overflowAbove;
  nsTableRowGroupFrame* f = static_cast<nsTableRowGroupFrame*>(aFrame);
  
  
  
  
  nsIFrame* kid = f->GetStateBits() & NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO
    ? nsnull : f->GetFirstRowContaining(aDirtyRect.y, &overflowAbove);
  
  if (kid) {
    
    while (kid) {
      if (kid->GetRect().y - overflowAbove >= aDirtyRect.YMost())
        break;
      nsresult rv = f->BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
      NS_ENSURE_SUCCESS(rv, rv);
      kid = kid->GetNextSibling();
    }
    return NS_OK;
  }
  
  
  nsTableRowGroupFrame::FrameCursorData* cursor = f->SetupRowCursor();
  kid = f->GetFirstChild(nsnull);
  while (kid) {
    nsresult rv = f->BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
    if (NS_FAILED(rv)) {
      f->ClearRowCursor();
      return rv;
    }
    
    if (cursor) {
      if (!cursor->AppendFrame(kid)) {
        f->ClearRowCursor();
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
  
    kid = kid->GetNextSibling();
  }
  if (cursor) {
    cursor->FinishBuildingCursor();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTableRowGroupFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                       const nsRect&           aDirtyRect,
                                       const nsDisplayListSet& aLists)
{
  if (!IsVisibleInSelection(aBuilder))
    return NS_OK;

  PRBool isRoot = aBuilder->IsAtRootOfPseudoStackingContext() || IsScrolled();
  nsDisplayTableItem* item = nsnull;
  if (isRoot) {
    
    
    
    item = new (aBuilder) nsDisplayTableRowGroupBackground(this);
    nsresult rv = aLists.BorderBackground()->AppendNewToTop(item);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return nsTableFrame::DisplayGenericTablePart(aBuilder, this, aDirtyRect,
                                               aLists, item, DisplayRows);
}

PRIntn
nsTableRowGroupFrame::GetSkipSides() const
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



void 
nsTableRowGroupFrame::PlaceChild(nsPresContext*         aPresContext,
                                 nsRowGroupReflowState& aReflowState,
                                 nsIFrame*              aKidFrame,
                                 nsHTMLReflowMetrics&   aDesiredSize,
                                 const nsRect&          aOriginalKidRect,
                                 const nsRect&          aOriginalKidOverflowRect)
{
  PRBool isFirstReflow =
    (aKidFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;

  
  FinishReflowChild(aKidFrame, aPresContext, nsnull, aDesiredSize, 0,
                    aReflowState.y, 0);

  nsTableFrame::InvalidateFrame(aKidFrame, aOriginalKidRect,
                                aOriginalKidOverflowRect, isFirstReflow);

  
  aReflowState.y += aDesiredSize.height;

  
  if (NS_UNCONSTRAINEDSIZE != aReflowState.availSize.height) {
    aReflowState.availSize.height -= aDesiredSize.height;
  }
}

void
nsTableRowGroupFrame::InitChildReflowState(nsPresContext&     aPresContext, 
                                           PRBool             aBorderCollapse,
                                           nsHTMLReflowState& aReflowState)                                    
{
  nsMargin collapseBorder;
  nsMargin padding(0,0,0,0);
  nsMargin* pCollapseBorder = nsnull;
  if (aBorderCollapse) {
    if (aReflowState.frame) {
      nsTableRowFrame *rowFrame = do_QueryFrame(aReflowState.frame);
      if (rowFrame) {
        pCollapseBorder = rowFrame->GetBCBorderWidth(collapseBorder);
      }
    }
  }
  aReflowState.Init(&aPresContext, -1, -1, pCollapseBorder, &padding);
}

static void
CacheRowHeightsForPrinting(nsPresContext*   aPresContext,
                           nsTableRowFrame* aFirstRow)
{
  for (nsTableRowFrame* row = aFirstRow; row; row = row->GetNextRow()) {
    if (!row->GetPrevInFlow()) {
      row->SetHasUnpaginatedHeight(PR_TRUE);
      row->SetUnpaginatedHeight(aPresContext, row->GetSize().height);
    }
  }
}

NS_METHOD 
nsTableRowGroupFrame::ReflowChildren(nsPresContext*         aPresContext,
                                     nsHTMLReflowMetrics&   aDesiredSize,
                                     nsRowGroupReflowState& aReflowState,
                                     nsReflowStatus&        aStatus,
                                     PRBool*                aPageBreakBeforeEnd)
{
  if (aPageBreakBeforeEnd) 
    *aPageBreakBeforeEnd = PR_FALSE;

  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (!tableFrame)
    ABORT1(NS_ERROR_NULL_POINTER);

  nsresult rv = NS_OK;

  PRBool borderCollapse = tableFrame->IsBorderCollapse();

  nscoord cellSpacingY = tableFrame->GetCellSpacingY();

  
  
  PRBool isPaginated = aPresContext->IsPaginated();

  PRBool haveRow = PR_FALSE;
  PRBool reflowAllKids = aReflowState.reflowState.ShouldReflowAllKids() ||
                         tableFrame->IsGeometryDirty();
  PRBool needToCalcRowHeights = reflowAllKids;

  nsIFrame *prevKidFrame = nsnull;
  for (nsIFrame* kidFrame = GetFirstFrame(); kidFrame;
       prevKidFrame = kidFrame, kidFrame = kidFrame->GetNextSibling()) {
    nsTableRowFrame *rowFrame = do_QueryFrame(kidFrame);
    if (!rowFrame) {
      
      NS_NOTREACHED("yikes, a non-row child");
      continue;
    }

    haveRow = PR_TRUE;

    
    if (reflowAllKids ||
        NS_SUBTREE_DIRTY(kidFrame) ||
        (aReflowState.reflowState.mFlags.mSpecialHeightReflow &&
         (isPaginated || (kidFrame->GetStateBits() &
                          NS_FRAME_CONTAINS_RELATIVE_HEIGHT)))) {
      nsRect oldKidRect = kidFrame->GetRect();
      nsRect oldKidOverflowRect = kidFrame->GetOverflowRect();

      
      
      nsHTMLReflowMetrics desiredSize(aDesiredSize.mFlags);
      desiredSize.width = desiredSize.height = 0;
  
      
      
      
      nsSize kidAvailSize(aReflowState.availSize.width, NS_UNCONSTRAINEDSIZE);
      nsHTMLReflowState kidReflowState(aPresContext, aReflowState.reflowState,
                                       kidFrame, kidAvailSize,
                                       -1, -1, PR_FALSE);
      InitChildReflowState(*aPresContext, borderCollapse, kidReflowState);

      
      if (aReflowState.reflowState.mFlags.mHResize)
        kidReflowState.mFlags.mHResize = PR_TRUE;
     
      NS_ASSERTION(kidFrame == GetFirstFrame() || prevKidFrame, 
                   "If we're not on the first frame, we should have a "
                   "previous sibling...");
      
      if (prevKidFrame && prevKidFrame->GetRect().YMost() > 0) {
        kidReflowState.mFlags.mIsTopOfPage = PR_FALSE;
      }

      rv = ReflowChild(kidFrame, aPresContext, desiredSize, kidReflowState,
                       0, aReflowState.y, NS_FRAME_INVALIDATE_ON_MOVE,
                       aStatus);

      
      PlaceChild(aPresContext, aReflowState, kidFrame, desiredSize,
                 oldKidRect, oldKidOverflowRect);
      aReflowState.y += cellSpacingY;

      if (!reflowAllKids) {
        if (IsSimpleRowFrame(aReflowState.tableFrame, kidFrame)) {
          
          rowFrame->DidResize();
          
          const nsStylePosition *stylePos = GetStylePosition();
          nsStyleUnit unit = stylePos->mHeight.GetUnit();
          if (aReflowState.tableFrame->IsAutoHeight() &&
              unit != eStyleUnit_Coord) {
            
            
            nsRect kidRect(0, aReflowState.y,
                           desiredSize.width, desiredSize.height);
            Invalidate(kidRect);
            
            
            
            if (kidRect.YMost() < mRect.height) {
              nsRect  dirtyRect(0, kidRect.YMost(),
                                mRect.width, mRect.height - kidRect.YMost());
              Invalidate(dirtyRect);
            }
          }
          else if (oldKidRect.height != desiredSize.height)
            needToCalcRowHeights = PR_TRUE;
        } else {
          needToCalcRowHeights = PR_TRUE;
        }
      }

      if (isPaginated && aPageBreakBeforeEnd && !*aPageBreakBeforeEnd) {
        nsTableRowFrame* nextRow = rowFrame->GetNextRow();
        if (nextRow) {
          *aPageBreakBeforeEnd = nsTableFrame::PageBreakAfter(*kidFrame, nextRow);
        }
      }
    } else {
      SlideChild(aReflowState, kidFrame);

      
      nscoord height = kidFrame->GetSize().height + cellSpacingY;
      aReflowState.y += height;

      if (NS_UNCONSTRAINEDSIZE != aReflowState.availSize.height) {
        aReflowState.availSize.height -= height;
      }
    }
    ConsiderChildOverflow(aDesiredSize.mOverflowArea, kidFrame);
  }

  if (haveRow)
    aReflowState.y -= cellSpacingY;

  
  aDesiredSize.width = aReflowState.reflowState.availableWidth;
  aDesiredSize.height = aReflowState.y;

  if (aReflowState.reflowState.mFlags.mSpecialHeightReflow) {
    DidResizeRows(aDesiredSize);
    if (isPaginated) {
      CacheRowHeightsForPrinting(aPresContext, GetFirstRow());
    }
  }
  else if (needToCalcRowHeights) {
    CalculateRowHeights(aPresContext, aDesiredSize, aReflowState.reflowState);
    if (!reflowAllKids) {
      
      
      
      nsRect  dirtyRect(0, 0, mRect.width, mRect.height);
      Invalidate(dirtyRect);
    }
  }

  return rv;
}

nsTableRowFrame*  
nsTableRowGroupFrame::GetFirstRow() 
{
  for (nsIFrame* childFrame = GetFirstFrame(); childFrame;
       childFrame = childFrame->GetNextSibling()) {
    nsTableRowFrame *rowFrame = do_QueryFrame(childFrame);
    if (rowFrame) {
      return rowFrame;
    }
  }
  return nsnull;
}


struct RowInfo {
  RowInfo() { height = pctHeight = hasStyleHeight = hasPctHeight = isSpecial = 0; }
  unsigned height;       
  unsigned pctHeight:29; 
  unsigned hasStyleHeight:1; 
  unsigned hasPctHeight:1; 
  unsigned isSpecial:1; 
                        
};

static void
UpdateHeights(RowInfo& aRowInfo,
              nscoord  aAdditionalHeight,
              nscoord& aTotal,
              nscoord& aUnconstrainedTotal)
{
  aRowInfo.height += aAdditionalHeight;
  aTotal          += aAdditionalHeight;
  if (!aRowInfo.hasStyleHeight) {
    aUnconstrainedTotal += aAdditionalHeight;
  }
}

void 
nsTableRowGroupFrame::DidResizeRows(nsHTMLReflowMetrics& aDesiredSize)
{
  
  
  
  aDesiredSize.mOverflowArea = nsRect(0, 0, 0, 0);
  for (nsTableRowFrame* rowFrame = GetFirstRow();
       rowFrame; rowFrame = rowFrame->GetNextRow()) {
    rowFrame->DidResize();
    ConsiderChildOverflow(aDesiredSize.mOverflowArea, rowFrame);
  }
}






void 
nsTableRowGroupFrame::CalculateRowHeights(nsPresContext*           aPresContext, 
                                          nsHTMLReflowMetrics&     aDesiredSize,
                                          const nsHTMLReflowState& aReflowState)
{
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (!tableFrame) return;

  PRBool isPaginated = aPresContext->IsPaginated();

  
  nscoord cellSpacingY = tableFrame->GetCellSpacingY();

  PRInt32 numEffCols = tableFrame->GetEffectiveColCount();

  PRInt32 startRowIndex = GetStartRowIndex();
  
  nsTableRowFrame* startRowFrame = GetFirstRow();

  if (!startRowFrame) return;

  
  nscoord startRowGroupHeight = startRowFrame->GetPosition().y;

  PRInt32 numRows = GetRowCount() - (startRowFrame->GetRowIndex() - GetStartRowIndex());
  
  if (numRows <= 0)
    return;

  nsTArray<RowInfo> rowInfo;
  if (!rowInfo.AppendElements(numRows)) {
    return;
  }

  PRBool  hasRowSpanningCell = PR_FALSE;
  nscoord heightOfRows = 0;
  nscoord heightOfUnStyledRows = 0;
  
  
  
  nscoord pctHeightBasis = GetHeightBasis(aReflowState);
  PRInt32 rowIndex; 
  nsTableRowFrame* rowFrame;
  for (rowFrame = startRowFrame, rowIndex = 0; rowFrame; rowFrame = rowFrame->GetNextRow(), rowIndex++) {
    nscoord nonPctHeight = rowFrame->GetContentHeight();
    if (isPaginated) {
      nonPctHeight = NS_MAX(nonPctHeight, rowFrame->GetSize().height);
    }
    if (!rowFrame->GetPrevInFlow()) {
      if (rowFrame->HasPctHeight()) {
        rowInfo[rowIndex].hasPctHeight = PR_TRUE;
        rowInfo[rowIndex].pctHeight = rowFrame->GetHeight(pctHeightBasis);
      }
      rowInfo[rowIndex].hasStyleHeight = rowFrame->HasStyleHeight();
      nonPctHeight = NS_MAX(nonPctHeight, rowFrame->GetFixedHeight());
    }
    UpdateHeights(rowInfo[rowIndex], nonPctHeight, heightOfRows, heightOfUnStyledRows);

    if (!rowInfo[rowIndex].hasStyleHeight) {
      if (isPaginated || tableFrame->HasMoreThanOneCell(rowIndex + startRowIndex)) {
        rowInfo[rowIndex].isSpecial = PR_TRUE;
        
        nsTableCellFrame* cellFrame = rowFrame->GetFirstCell();
        while (cellFrame) {
          PRInt32 rowSpan = tableFrame->GetEffectiveRowSpan(rowIndex + startRowIndex, *cellFrame);
          if (1 == rowSpan) { 
            rowInfo[rowIndex].isSpecial = PR_FALSE;
            break;
          }
          cellFrame = cellFrame->GetNextCell(); 
        }
      }
    }
    
    if (!hasRowSpanningCell) {
      if (tableFrame->RowIsSpannedInto(rowIndex + startRowIndex, numEffCols)) {
        hasRowSpanningCell = PR_TRUE;
      }
    }
  }

  if (hasRowSpanningCell) {
    
    
    for (rowFrame = startRowFrame, rowIndex = 0; rowFrame; rowFrame = rowFrame->GetNextRow(), rowIndex++) {
      
      
      
      if (GetPrevInFlow() || tableFrame->RowHasSpanningCells(startRowIndex + rowIndex, numEffCols)) {
        nsTableCellFrame* cellFrame = rowFrame->GetFirstCell();
        
        while (cellFrame) {
          PRInt32 rowSpan = tableFrame->GetEffectiveRowSpan(rowIndex + startRowIndex, *cellFrame);
          if ((rowIndex + rowSpan) > numRows) {
            
            rowSpan = numRows - rowIndex;
          }
          if (rowSpan > 1) { 
            nscoord heightOfRowsSpanned = 0;
            nscoord heightOfUnStyledRowsSpanned = 0;
            nscoord numSpecialRowsSpanned = 0; 
            nscoord cellSpacingTotal = 0;
            PRInt32 spanX;
            for (spanX = 0; spanX < rowSpan; spanX++) {
              heightOfRowsSpanned += rowInfo[rowIndex + spanX].height;
              if (!rowInfo[rowIndex + spanX].hasStyleHeight) {
                heightOfUnStyledRowsSpanned += rowInfo[rowIndex + spanX].height;
              }
              if (0 != spanX) {
                cellSpacingTotal += cellSpacingY;
              }
              if (rowInfo[rowIndex + spanX].isSpecial) {
                numSpecialRowsSpanned++;
              }
            } 
            nscoord heightOfAreaSpanned = heightOfRowsSpanned + cellSpacingTotal;
            
            nsSize cellFrameSize = cellFrame->GetSize();
            nsSize cellDesSize = cellFrame->GetDesiredSize();
            rowFrame->CalculateCellActualSize(cellFrame, cellDesSize.width, 
                                              cellDesSize.height, cellDesSize.width);
            cellFrameSize.height = cellDesSize.height;
            if (cellFrame->HasVerticalAlignBaseline()) {
              
              
              
              cellFrameSize.height += rowFrame->GetMaxCellAscent() -
                                      cellFrame->GetCellBaseline();
            }
  
            if (heightOfAreaSpanned < cellFrameSize.height) {
              
              
              nscoord extra     = cellFrameSize.height - heightOfAreaSpanned;
              nscoord extraUsed = 0;
              if (0 == numSpecialRowsSpanned) {
                
                PRBool haveUnStyledRowsSpanned = (heightOfUnStyledRowsSpanned > 0);
                nscoord divisor = (haveUnStyledRowsSpanned) 
                                  ? heightOfUnStyledRowsSpanned : heightOfRowsSpanned;
                if (divisor > 0) {
                  for (spanX = rowSpan - 1; spanX >= 0; spanX--) {
                    if (!haveUnStyledRowsSpanned || !rowInfo[rowIndex + spanX].hasStyleHeight) {
                      
                      float percent = ((float)rowInfo[rowIndex + spanX].height) / ((float)divisor);
                    
                      
                      nscoord extraForRow = (0 == spanX) ? extra - extraUsed  
                                                         : NSToCoordRound(((float)(extra)) * percent);
                      extraForRow = NS_MIN(extraForRow, extra - extraUsed);
                      
                      UpdateHeights(rowInfo[rowIndex + spanX], extraForRow, heightOfRows, heightOfUnStyledRows);
                      extraUsed += extraForRow;
                      if (extraUsed >= extra) {
                        NS_ASSERTION((extraUsed == extra), "invalid row height calculation");
                        break;
                      }
                    }
                  }
                }
                else {
                  
                  UpdateHeights(rowInfo[rowIndex + rowSpan - 1], extra, heightOfRows, heightOfUnStyledRows);
                }
              }
              else {
                
                nscoord numSpecialRowsAllocated = 0;
                for (spanX = rowSpan - 1; spanX >= 0; spanX--) {
                  if (rowInfo[rowIndex + spanX].isSpecial) {
                    
                    float percent = 1.0f / ((float)numSpecialRowsSpanned);
                    
                    
                    nscoord extraForRow = (numSpecialRowsSpanned - 1 == numSpecialRowsAllocated) 
                                          ? extra - extraUsed  
                                          : NSToCoordRound(((float)(extra)) * percent);
                    extraForRow = NS_MIN(extraForRow, extra - extraUsed);
                    
                    UpdateHeights(rowInfo[rowIndex + spanX], extraForRow, heightOfRows, heightOfUnStyledRows);
                    extraUsed += extraForRow;
                    if (extraUsed >= extra) {
                      NS_ASSERTION((extraUsed == extra), "invalid row height calculation");
                      break;
                    }
                  }
                }
              }
            } 
          } 
          cellFrame = cellFrame->GetNextCell(); 
        } 
      } 
    } 
  }

  
  nscoord extra = pctHeightBasis - heightOfRows;
  for (rowFrame = startRowFrame, rowIndex = 0; rowFrame && (extra > 0); rowFrame = rowFrame->GetNextRow(), rowIndex++) {
    RowInfo& rInfo = rowInfo[rowIndex];
    if (rInfo.hasPctHeight) {
      nscoord rowExtra = (rInfo.pctHeight > rInfo.height)  
                         ? rInfo.pctHeight - rInfo.height: 0;
      rowExtra = NS_MIN(rowExtra, extra);
      UpdateHeights(rInfo, rowExtra, heightOfRows, heightOfUnStyledRows);
      extra -= rowExtra;
    }
  }

  PRBool styleHeightAllocation = PR_FALSE;
  nscoord rowGroupHeight = startRowGroupHeight + heightOfRows + ((numRows - 1) * cellSpacingY);
  
  if ((aReflowState.ComputedHeight() > rowGroupHeight) && 
      (NS_UNCONSTRAINEDSIZE != aReflowState.ComputedHeight())) {
    nscoord extraComputedHeight = aReflowState.ComputedHeight() - rowGroupHeight;
    nscoord extraUsed = 0;
    PRBool haveUnStyledRows = (heightOfUnStyledRows > 0);
    nscoord divisor = (haveUnStyledRows) 
                      ? heightOfUnStyledRows : heightOfRows;
    if (divisor > 0) {
      styleHeightAllocation = PR_TRUE;
      for (rowIndex = 0; rowIndex < numRows; rowIndex++) {
        if (!haveUnStyledRows || !rowInfo[rowIndex].hasStyleHeight) {
          
          
          float percent = ((float)rowInfo[rowIndex].height) / ((float)divisor);
          
          nscoord extraForRow = (numRows - 1 == rowIndex) 
                                ? extraComputedHeight - extraUsed  
                                : NSToCoordRound(((float)extraComputedHeight) * percent);
          extraForRow = NS_MIN(extraForRow, extraComputedHeight - extraUsed);
          
          UpdateHeights(rowInfo[rowIndex], extraForRow, heightOfRows, heightOfUnStyledRows);
          extraUsed += extraForRow;
          if (extraUsed >= extraComputedHeight) {
            NS_ASSERTION((extraUsed == extraComputedHeight), "invalid row height calculation");
            break;
          }
        }
      }
    }
    rowGroupHeight = aReflowState.ComputedHeight();
  }

  nscoord yOrigin = startRowGroupHeight;
  
  for (rowFrame = startRowFrame, rowIndex = 0; rowFrame; rowFrame = rowFrame->GetNextRow(), rowIndex++) {
    nsRect rowBounds = rowFrame->GetRect();
    nsRect rowOverflowRect = rowFrame->GetOverflowRect();

    PRBool movedFrame = (rowBounds.y != yOrigin);  
    nscoord rowHeight = (rowInfo[rowIndex].height > 0) ? rowInfo[rowIndex].height : 0;
    
    if (movedFrame || (rowHeight != rowBounds.height)) {
      
      if (movedFrame) {
        rowFrame->InvalidateOverflowRect();
      }
      
      rowFrame->SetRect(nsRect(rowBounds.x, yOrigin, rowBounds.width,
                               rowHeight));

      nsTableFrame::InvalidateFrame(rowFrame, rowBounds, rowOverflowRect,
                                    PR_FALSE);
    }
    if (movedFrame) {
      nsTableFrame::RePositionViews(rowFrame);
      
    }
    yOrigin += rowHeight + cellSpacingY;
  }

  if (isPaginated && styleHeightAllocation) {
    
    CacheRowHeightsForPrinting(aPresContext, GetFirstRow());
  }

  DidResizeRows(aDesiredSize);

  aDesiredSize.height = rowGroupHeight; 
}

nscoord
nsTableRowGroupFrame::CollapseRowGroupIfNecessary(nscoord aYTotalOffset,
                                                  nscoord aWidth)
{
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);

  const nsStyleVisibility* groupVis = GetStyleVisibility();
  PRBool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE == groupVis->mVisible);
  if (collapseGroup) {
    tableFrame->SetNeedToCollapse(PR_TRUE);
  }

  nsRect overflowArea(0, 0, 0, 0);

  nsTableRowFrame* rowFrame= GetFirstRow();
  PRBool didCollapse = PR_FALSE;
  nscoord yGroupOffset = 0;
  while (rowFrame) {
    yGroupOffset += rowFrame->CollapseRowIfNecessary(yGroupOffset,
                                                     aWidth, collapseGroup,
                                                     didCollapse);
    ConsiderChildOverflow(overflowArea, rowFrame);
    rowFrame = rowFrame->GetNextRow();
  }

  nsRect groupRect = GetRect();
  nsRect oldGroupRect = groupRect;
  nsRect oldGroupOverflowRect = GetOverflowRect();
  
  groupRect.height -= yGroupOffset;
  if (didCollapse) {
    
    groupRect.height += tableFrame->GetCellSpacingY();
  }

  groupRect.y -= aYTotalOffset;
  groupRect.width = aWidth;

  if (aYTotalOffset != 0) {
    InvalidateOverflowRect();
  }
  
  SetRect(groupRect);
  overflowArea.UnionRect(nsRect(0, 0, groupRect.width, groupRect.height),
                         overflowArea);
  FinishAndStoreOverflow(&overflowArea, nsSize(groupRect.width,
                                              groupRect.height));
  nsTableFrame::RePositionViews(this);
  nsTableFrame::InvalidateFrame(this, oldGroupRect, oldGroupOverflowRect,
                                PR_FALSE);

  return yGroupOffset;
}


void
nsTableRowGroupFrame::SlideChild(nsRowGroupReflowState& aReflowState,
                                 nsIFrame*              aKidFrame)
{
  
  nsPoint oldPosition = aKidFrame->GetPosition();
  nsPoint newPosition = oldPosition;
  newPosition.y = aReflowState.y;
  if (oldPosition.y != newPosition.y) {
    aKidFrame->InvalidateOverflowRect();
    aKidFrame->SetPosition(newPosition);
    nsTableFrame::RePositionViews(aKidFrame);
    aKidFrame->InvalidateOverflowRect();
  }
}



void 
nsTableRowGroupFrame::CreateContinuingRowFrame(nsPresContext& aPresContext,
                                               nsIFrame&      aRowFrame,
                                               nsIFrame**     aContRowFrame)
{
  
  if (!aContRowFrame) {NS_ASSERTION(PR_FALSE, "bad call"); return;}
  
  nsresult rv = aPresContext.PresShell()->FrameConstructor()->
    CreateContinuingFrame(&aPresContext, &aRowFrame, this, aContRowFrame);
  if (NS_FAILED(rv)) {
    *aContRowFrame = nsnull;
    return;
  }

  
  nsIFrame* nextRow;
  GetNextFrame(&aRowFrame, &nextRow);
  (*aContRowFrame)->SetNextSibling(nextRow);
  aRowFrame.SetNextSibling(*aContRowFrame);
          
  
  PushChildren(&aPresContext, *aContRowFrame, &aRowFrame);
}




void
nsTableRowGroupFrame::SplitSpanningCells(nsPresContext&           aPresContext,
                                         const nsHTMLReflowState& aReflowState,
                                         nsTableFrame&            aTable,
                                         nsTableRowFrame&         aFirstRow, 
                                         nsTableRowFrame&         aLastRow,  
                                         PRBool                   aFirstRowIsTopOfPage,
                                         nscoord                  aSpanningRowBottom,
                                         nsTableRowFrame*&        aContRow,
                                         nsTableRowFrame*&        aFirstTruncatedRow,
                                         nscoord&                 aDesiredHeight)
{
  NS_ASSERTION(aSpanningRowBottom >= 0, "Can't split negative heights");
  aFirstTruncatedRow = nsnull;
  aDesiredHeight     = 0;

  PRInt32 lastRowIndex = aLastRow.GetRowIndex();
  PRBool wasLast = PR_FALSE;
  
  for (nsTableRowFrame* row = &aFirstRow; !wasLast; row = row->GetNextRow()) {
    wasLast = (row == &aLastRow);
    PRInt32 rowIndex = row->GetRowIndex();
    nsPoint rowPos = row->GetPosition();
    
    for (nsTableCellFrame* cell = row->GetFirstCell(); cell; cell = cell->GetNextCell()) {
      PRInt32 rowSpan = aTable.GetEffectiveRowSpan(rowIndex, *cell);
      
      
      if ((rowSpan > 1) && (rowIndex + rowSpan > lastRowIndex)) {
        nsReflowStatus status;
        
        
        nscoord cellAvailHeight = aSpanningRowBottom - rowPos.y;
        NS_ASSERTION(cellAvailHeight >= 0, "No space for cell?");
        PRBool isTopOfPage = (row == &aFirstRow) && aFirstRowIsTopOfPage;
        nscoord cellHeight = row->ReflowCellFrame(&aPresContext, aReflowState,
                                                  isTopOfPage, cell,
                                                  cellAvailHeight, status);
        aDesiredHeight = NS_MAX(aDesiredHeight, rowPos.y + cellHeight);
        if (NS_FRAME_IS_COMPLETE(status)) {
          if (cellHeight > cellAvailHeight) {
            aFirstTruncatedRow = row;
            if ((row != &aFirstRow) || !aFirstRowIsTopOfPage) {
              
              
              return;
            }
          }
        }
        else {
          if (!aContRow) {
            CreateContinuingRowFrame(aPresContext, aLastRow, (nsIFrame**)&aContRow);
          }
          if (aContRow) {
            if (row != &aLastRow) {
              
              
              nsTableCellFrame* contCell = nsnull;
              aPresContext.PresShell()->FrameConstructor()->
                CreateContinuingFrame(&aPresContext, cell, &aLastRow,
                                      (nsIFrame**)&contCell);
              PRInt32 colIndex;
              cell->GetColIndex(colIndex);
              aContRow->InsertCellFrame(contCell, colIndex);
            }
          }
        }
      }
    }
  }
}




void
nsTableRowGroupFrame::UndoContinuedRow(nsPresContext*   aPresContext,
                                       nsTableRowFrame* aRow)
{
  if (!aRow) return; 

  
  nsTableRowFrame* rowBefore = (nsTableRowFrame*)aRow->GetPrevInFlow();
  NS_PRECONDITION(mFrames.ContainsFrame(rowBefore),
                  "rowBefore not in our frame list?");

  nsAutoPtr<nsFrameList> overflows(StealOverflowFrames());
  if (!rowBefore || !overflows || overflows->IsEmpty() ||
      overflows->FirstChild() != aRow) {
    NS_ERROR("invalid continued row");
    return;
  }

  
  
  overflows->DestroyFrame(aRow);

  
  mFrames.InsertFrames(nsnull, rowBefore, *overflows);
}

static nsTableRowFrame* 
GetRowBefore(nsTableRowFrame& aStartRow,
             nsTableRowFrame& aRow)
{
  nsTableRowFrame* rowBefore = nsnull;
  for (nsTableRowFrame* sib = &aStartRow; sib && (sib != &aRow); sib = sib->GetNextRow()) {
    rowBefore = sib;
  }
  return rowBefore;
}

nsresult
nsTableRowGroupFrame::SplitRowGroup(nsPresContext*           aPresContext,
                                    nsHTMLReflowMetrics&     aDesiredSize,
                                    const nsHTMLReflowState& aReflowState,
                                    nsTableFrame*            aTableFrame,
                                    nsReflowStatus&          aStatus)
{
  NS_PRECONDITION(aPresContext->IsPaginated(), "SplitRowGroup currently supports only paged media"); 

  nsresult rv = NS_OK;
  nsTableRowFrame* prevRowFrame = nsnull;
  aDesiredSize.height = 0;

  nscoord availWidth  = aReflowState.availableWidth;
  nscoord availHeight = aReflowState.availableHeight;
  
  PRBool  borderCollapse = ((nsTableFrame*)aTableFrame->GetFirstInFlow())->IsBorderCollapse();
  nscoord cellSpacingY   = aTableFrame->GetCellSpacingY();
  
  
  nscoord pageHeight = aPresContext->GetPageSize().height;
  NS_ASSERTION(pageHeight != NS_UNCONSTRAINEDSIZE, 
               "The table shouldn't be split when there should be space");

  PRBool isTopOfPage = aReflowState.mFlags.mIsTopOfPage;
  nsTableRowFrame* firstRowThisPage = GetFirstRow();

  
  
  aTableFrame->SetGeometryDirty();

  
  
  for (nsTableRowFrame* rowFrame = firstRowThisPage; rowFrame; rowFrame = rowFrame->GetNextRow()) {
    PRBool rowIsOnPage = PR_TRUE;
    nsRect rowRect = rowFrame->GetRect();
    
    if (rowRect.YMost() > availHeight) {
      nsTableRowFrame* contRow = nsnull;
      
      
      
      if (!prevRowFrame || (availHeight - aDesiredSize.height > pageHeight / 20)) { 
        nsSize availSize(availWidth, NS_MAX(availHeight - rowRect.y, 0));
        
        availSize.height = NS_MIN(availSize.height, rowRect.height);

        nsHTMLReflowState rowReflowState(aPresContext, aReflowState,
                                         rowFrame, availSize,
                                         -1, -1, PR_FALSE);
                                         
        InitChildReflowState(*aPresContext, borderCollapse, rowReflowState);
        rowReflowState.mFlags.mIsTopOfPage = isTopOfPage; 
        nsHTMLReflowMetrics rowMetrics;

        
        nsRect oldRowRect = rowFrame->GetRect();
        nsRect oldRowOverflowRect = rowFrame->GetOverflowRect();

        
        
        rv = ReflowChild(rowFrame, aPresContext, rowMetrics, rowReflowState,
                         0, 0, NS_FRAME_NO_MOVE_FRAME, aStatus);
        if (NS_FAILED(rv)) return rv;
        rowFrame->SetSize(nsSize(rowMetrics.width, rowMetrics.height));
        rowFrame->DidReflow(aPresContext, nsnull, NS_FRAME_REFLOW_FINISHED);
        rowFrame->DidResize();

        nsTableFrame::InvalidateFrame(rowFrame, oldRowRect, oldRowOverflowRect,
                                      PR_FALSE);

        if (NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
          
          if ((rowMetrics.height <= rowReflowState.availableHeight) || isTopOfPage) {
            
            
            NS_ASSERTION(rowMetrics.height <= rowReflowState.availableHeight, 
                         "data loss - incomplete row needed more height than available, on top of page");
            CreateContinuingRowFrame(*aPresContext, *rowFrame, (nsIFrame**)&contRow);
            if (contRow) {
              aDesiredSize.height += rowMetrics.height;
              if (prevRowFrame) 
                aDesiredSize.height += cellSpacingY;
            }
            else return NS_ERROR_NULL_POINTER;
          }
          else {
            
            rowIsOnPage = PR_FALSE;
          }
        } 
        else {
          
          
          
          
          
          if (rowMetrics.height > availSize.height) {
            
            if (isTopOfPage) { 
              
              
              nsTableRowFrame* nextRowFrame = rowFrame->GetNextRow();
              if (nextRowFrame) {
                aStatus = NS_FRAME_NOT_COMPLETE;
              }
              aDesiredSize.height += rowMetrics.height;
              if (prevRowFrame) 
                aDesiredSize.height += cellSpacingY;
              NS_WARNING("data loss - complete row needed more height than available, on top of page");
            }
            else {
              
              rowIsOnPage = PR_FALSE;
            }
          }
        }
      } 
      else { 
        
        rowIsOnPage = PR_FALSE;
      }

      nsTableRowFrame* lastRowThisPage = rowFrame;
      nscoord spanningRowBottom = availHeight;
      if (!rowIsOnPage) {
        NS_ASSERTION(!contRow, "We should not have created a continuation if none of this row fits");
        if (prevRowFrame) {
          spanningRowBottom = prevRowFrame->GetRect().YMost();
          lastRowThisPage = prevRowFrame;
          isTopOfPage = (lastRowThisPage == firstRowThisPage) && aReflowState.mFlags.mIsTopOfPage;
          aStatus = NS_FRAME_NOT_COMPLETE;
        }
        else {
          
          aDesiredSize.height = rowRect.YMost();
          aStatus = NS_FRAME_COMPLETE;
          break;
        }
      }
      

      nsTableRowFrame* firstTruncatedRow;
      nscoord yMost;
      SplitSpanningCells(*aPresContext, aReflowState, *aTableFrame, *firstRowThisPage,
                         *lastRowThisPage, aReflowState.mFlags.mIsTopOfPage, spanningRowBottom, contRow, 
                         firstTruncatedRow, yMost);
      if (firstTruncatedRow) {
        
        if (firstTruncatedRow == firstRowThisPage) {
          if (aReflowState.mFlags.mIsTopOfPage) {
            NS_WARNING("data loss in a row spanned cell");
          }
          else {
            
            aDesiredSize.height = rowRect.YMost();
            aStatus = NS_FRAME_COMPLETE;
            UndoContinuedRow(aPresContext, contRow);
            contRow = nsnull;
          }
        }
        else { 
          
          nsTableRowFrame* rowBefore = ::GetRowBefore(*firstRowThisPage, *firstTruncatedRow);
          nscoord oldSpanningRowBottom = spanningRowBottom;
          spanningRowBottom = rowBefore->GetRect().YMost();

          UndoContinuedRow(aPresContext, contRow);
          contRow = nsnull;
          nsTableRowFrame* oldLastRowThisPage = lastRowThisPage;
          lastRowThisPage = firstTruncatedRow;
          aStatus = NS_FRAME_NOT_COMPLETE;

          
          SplitSpanningCells(*aPresContext, aReflowState, *aTableFrame, 
                             *firstRowThisPage, *rowBefore, aReflowState.mFlags.mIsTopOfPage, 
                             spanningRowBottom, contRow, firstTruncatedRow, aDesiredSize.height);
          if (firstTruncatedRow) {
            if (aReflowState.mFlags.mIsTopOfPage) {
              
              UndoContinuedRow(aPresContext, contRow);
              contRow = nsnull;
              lastRowThisPage = oldLastRowThisPage;
              spanningRowBottom = oldSpanningRowBottom;
              SplitSpanningCells(*aPresContext, aReflowState, *aTableFrame, *firstRowThisPage,
                                 *lastRowThisPage, aReflowState.mFlags.mIsTopOfPage, spanningRowBottom, contRow, 
                                 firstTruncatedRow, aDesiredSize.height);
              NS_WARNING("data loss in a row spanned cell");
            }
            else {
              
              aDesiredSize.height = rowRect.YMost();
              aStatus = NS_FRAME_COMPLETE;
              UndoContinuedRow(aPresContext, contRow);
              contRow = nsnull;
            }
          }
        } 
      } 
      else {
        aDesiredSize.height = NS_MAX(aDesiredSize.height, yMost);
        if (contRow) {
          aStatus = NS_FRAME_NOT_COMPLETE;
        }
      }
      if (NS_FRAME_IS_NOT_COMPLETE(aStatus) && !contRow) {
        nsTableRowFrame* nextRow = lastRowThisPage->GetNextRow();
        if (nextRow) {
          PushChildren(aPresContext, nextRow, lastRowThisPage);
        }
      }
      break;
    } 
    else { 
      aDesiredSize.height = rowRect.YMost();
      prevRowFrame = rowFrame;
      
      nsTableRowFrame* nextRow = rowFrame->GetNextRow();
      if (nextRow && nsTableFrame::PageBreakAfter(*rowFrame, nextRow)) {
        PushChildren(aPresContext, nextRow, rowFrame);
        aStatus = NS_FRAME_NOT_COMPLETE;
        break;
      }
    }
    
    
    isTopOfPage = isTopOfPage && rowRect.YMost() == 0;
  }
  return NS_OK;
}





NS_METHOD
nsTableRowGroupFrame::Reflow(nsPresContext*           aPresContext,
                             nsHTMLReflowMetrics&     aDesiredSize,
                             const nsHTMLReflowState& aReflowState,
                             nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsTableRowGroupFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  nsresult rv = NS_OK;
  aStatus     = NS_FRAME_COMPLETE;
        
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (!tableFrame) return NS_ERROR_NULL_POINTER;

  
  ClearRowCursor();

  
  nsTableFrame::CheckRequestSpecialHeightReflow(aReflowState);

  nsRowGroupReflowState state(aReflowState, tableFrame);
  const nsStyleVisibility* groupVis = GetStyleVisibility();
  PRBool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE == groupVis->mVisible);
  if (collapseGroup) {
    tableFrame->SetNeedToCollapse(PR_TRUE);
  }

  
  MoveOverflowToChildList(aPresContext);

  
  PRBool splitDueToPageBreak = PR_FALSE;
  rv = ReflowChildren(aPresContext, aDesiredSize, state, aStatus,
                      &splitDueToPageBreak);

  
  
  if (aReflowState.mFlags.mTableIsSplittable &&
      (NS_FRAME_NOT_COMPLETE == aStatus || splitDueToPageBreak || 
       (NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight &&
       aDesiredSize.height > aReflowState.availableHeight))) {
    
    PRBool specialReflow = (PRBool)aReflowState.mFlags.mSpecialHeightReflow;
    ((nsHTMLReflowState::ReflowStateFlags&)aReflowState.mFlags).mSpecialHeightReflow = PR_FALSE;

    SplitRowGroup(aPresContext, aDesiredSize, aReflowState, tableFrame, aStatus);

    ((nsHTMLReflowState::ReflowStateFlags&)aReflowState.mFlags).mSpecialHeightReflow = specialReflow;
  }

  
  
  if (GetNextInFlow()) {
    aStatus = NS_FRAME_NOT_COMPLETE;
  }

  SetHasStyleHeight((NS_UNCONSTRAINEDSIZE != aReflowState.ComputedHeight()) &&
                    (aReflowState.ComputedHeight() > 0)); 
  
  
  aDesiredSize.width = aReflowState.availableWidth;

  aDesiredSize.mOverflowArea.UnionRect(aDesiredSize.mOverflowArea, nsRect(0, 0, aDesiredSize.width,
	                                                                      aDesiredSize.height));

  
  
  if (!(GetParent()->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    CheckInvalidateSizeChange(aDesiredSize);
  }
  
  FinishAndStoreOverflow(&aDesiredSize);
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return rv;
}

 void
nsTableRowGroupFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  if (!aOldStyleContext) 
    return;
     
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
    
  if (tableFrame->IsBorderCollapse() &&
      tableFrame->BCRecalcNeeded(aOldStyleContext, GetStyleContext())) {
    nsRect damageArea(0, GetStartRowIndex(), tableFrame->GetColCount(),
                      GetRowCount());
    tableFrame->SetBCDamageArea(damageArea);
  }
  return;
}

NS_IMETHODIMP
nsTableRowGroupFrame::AppendFrames(nsIAtom*        aListName,
                                   nsFrameList&    aFrameList)
{
  NS_ASSERTION(!aListName, "unexpected child list");

  ClearRowCursor();

  
  
  nsAutoTArray<nsTableRowFrame*, 8> rows;
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    nsTableRowFrame *rowFrame = do_QueryFrame(e.get());
    NS_ASSERTION(rowFrame, "Unexpected frame; frame constructor screwed up");
    if (rowFrame) {
      NS_ASSERTION(NS_STYLE_DISPLAY_TABLE_ROW ==
                     e.get()->GetStyleDisplay()->mDisplay,
                   "wrong display type on rowframe");      
      rows.AppendElement(rowFrame);
    }
  }

  PRInt32 rowIndex = GetRowCount();
  
  mFrames.AppendFrames(nsnull, aFrameList);

  if (rows.Length() > 0) {
    nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
    if (tableFrame) {
      tableFrame->AppendRows(*this, rowIndex, rows);
      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
      tableFrame->SetGeometryDirty();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTableRowGroupFrame::InsertFrames(nsIAtom*        aListName,
                                   nsIFrame*       aPrevFrame,
                                   nsFrameList&    aFrameList)
{
  NS_ASSERTION(!aListName, "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  ClearRowCursor();

  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (!tableFrame)
    return NS_ERROR_NULL_POINTER;

  
  
  nsTArray<nsTableRowFrame*> rows;
  PRBool gotFirstRow = PR_FALSE;
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    nsTableRowFrame *rowFrame = do_QueryFrame(e.get());
    NS_ASSERTION(rowFrame, "Unexpected frame; frame constructor screwed up");
    if (rowFrame) {
      NS_ASSERTION(NS_STYLE_DISPLAY_TABLE_ROW ==
                     e.get()->GetStyleDisplay()->mDisplay,
                   "wrong display type on rowframe");      
      rows.AppendElement(rowFrame);
      if (!gotFirstRow) {
        rowFrame->SetFirstInserted(PR_TRUE);
        gotFirstRow = PR_TRUE;
        tableFrame->SetRowInserted(PR_TRUE);
      }
    }
  }

  PRInt32 startRowIndex = GetStartRowIndex();
  
  mFrames.InsertFrames(nsnull, aPrevFrame, aFrameList);

  PRInt32 numRows = rows.Length();
  if (numRows > 0) {
    nsTableRowFrame* prevRow = (nsTableRowFrame *)nsTableFrame::GetFrameAtOrBefore(this, aPrevFrame, nsGkAtoms::tableRowFrame);
    PRInt32 rowIndex = (prevRow) ? prevRow->GetRowIndex() + 1 : startRowIndex;
    tableFrame->InsertRows(*this, rows, rowIndex, PR_TRUE);

    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
    tableFrame->SetGeometryDirty();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsTableRowGroupFrame::RemoveFrame(nsIAtom*        aListName,
                                  nsIFrame*       aOldFrame)
{
  NS_ASSERTION(!aListName, "unexpected child list");

  ClearRowCursor();

  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (tableFrame) {
    nsTableRowFrame *rowFrame = do_QueryFrame(aOldFrame);
    if (rowFrame) {
      
      tableFrame->RemoveRows(*rowFrame, 1, PR_TRUE);

      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
      tableFrame->SetGeometryDirty();
    }
  }
  mFrames.DestroyFrame(aOldFrame);

  return NS_OK;
}

 nsMargin
nsTableRowGroupFrame::GetUsedMargin() const
{
  return nsMargin(0,0,0,0);
}

 nsMargin
nsTableRowGroupFrame::GetUsedBorder() const
{
  return nsMargin(0,0,0,0);
}

 nsMargin
nsTableRowGroupFrame::GetUsedPadding() const
{
  return nsMargin(0,0,0,0);
}

nscoord 
nsTableRowGroupFrame::GetHeightBasis(const nsHTMLReflowState& aReflowState)
{
  nscoord result = 0;
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (tableFrame) {
    if ((aReflowState.ComputedHeight() > 0) && (aReflowState.ComputedHeight() < NS_UNCONSTRAINEDSIZE)) {
      nscoord cellSpacing = NS_MAX(0, GetRowCount() - 1) * tableFrame->GetCellSpacingY();
      result = aReflowState.ComputedHeight() - cellSpacing;
    }
    else {
      const nsHTMLReflowState* parentRS = aReflowState.parentReflowState;
      if (parentRS && (tableFrame != parentRS->frame)) {
        parentRS = parentRS->parentReflowState;
      }
      if (parentRS && (tableFrame == parentRS->frame) && 
          (parentRS->ComputedHeight() > 0) && (parentRS->ComputedHeight() < NS_UNCONSTRAINEDSIZE)) {
        nscoord cellSpacing = NS_MAX(0, tableFrame->GetRowCount() + 1) * tableFrame->GetCellSpacingY();
        result = parentRS->ComputedHeight() - cellSpacing;
      }
    }
  }

  return result;
}

PRBool
nsTableRowGroupFrame::IsSimpleRowFrame(nsTableFrame* aTableFrame,
                                       nsIFrame*     aFrame)
{
  
  nsTableRowFrame *rowFrame = do_QueryFrame(aFrame);
  if (rowFrame) {
    PRInt32 rowIndex = rowFrame->GetRowIndex();
    
    
    
    PRInt32 numEffCols = aTableFrame->GetEffectiveColCount();
    if (!aTableFrame->RowIsSpannedInto(rowIndex, numEffCols) &&
        !aTableFrame->RowHasSpanningCells(rowIndex, numEffCols)) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

nsIAtom*
nsTableRowGroupFrame::GetType() const
{
  return nsGkAtoms::tableRowGroupFrame;
}




nsIFrame*
NS_NewTableRowGroupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTableRowGroupFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTableRowGroupFrame)

#ifdef DEBUG
NS_IMETHODIMP
nsTableRowGroupFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("TableRowGroup"), aResult);
}
#endif

nsMargin* 
nsTableRowGroupFrame::GetBCBorderWidth(nsMargin& aBorder)
{
  aBorder.left = aBorder.right = aBorder.top = aBorder.bottom = 0;

  nsTableRowFrame* firstRowFrame = nsnull;
  nsTableRowFrame* lastRowFrame = nsnull;
  for (nsTableRowFrame* rowFrame = GetFirstRow(); rowFrame; rowFrame = rowFrame->GetNextRow()) {
    if (!firstRowFrame) {
      firstRowFrame = rowFrame;
    }
    lastRowFrame = rowFrame;
  }
  if (firstRowFrame) {
    aBorder.top    = nsPresContext::CSSPixelsToAppUnits(firstRowFrame->GetTopBCBorderWidth());
    aBorder.bottom = nsPresContext::CSSPixelsToAppUnits(lastRowFrame->GetBottomBCBorderWidth());
  }

  return &aBorder;
}

void nsTableRowGroupFrame::SetContinuousBCBorderWidth(PRUint8     aForSide,
                                                      BCPixelSize aPixelValue)
{
  switch (aForSide) {
    case NS_SIDE_RIGHT:
      mRightContBorderWidth = aPixelValue;
      return;
    case NS_SIDE_BOTTOM:
      mBottomContBorderWidth = aPixelValue;
      return;
    case NS_SIDE_LEFT:
      mLeftContBorderWidth = aPixelValue;
      return;
    default:
      NS_ERROR("invalid NS_SIDE argument");
  }
}


PRInt32
nsTableRowGroupFrame::GetNumLines()
{
  return GetRowCount();
}

PRBool
nsTableRowGroupFrame::GetDirection()
{
  nsTableFrame* table = nsTableFrame::GetTableFrame(this);
  return (NS_STYLE_DIRECTION_RTL ==
          table->GetStyleVisibility()->mDirection);
}
  
NS_IMETHODIMP
nsTableRowGroupFrame::GetLine(PRInt32    aLineNumber, 
                              nsIFrame** aFirstFrameOnLine, 
                              PRInt32*   aNumFramesOnLine,
                              nsRect&    aLineBounds, 
                              PRUint32*  aLineFlags)
{
  NS_ENSURE_ARG_POINTER(aFirstFrameOnLine);
  NS_ENSURE_ARG_POINTER(aNumFramesOnLine);
  NS_ENSURE_ARG_POINTER(aLineFlags);

  nsTableFrame* table = nsTableFrame::GetTableFrame(this);
  nsTableCellMap* cellMap = table->GetCellMap();

  *aLineFlags = 0;
  *aFirstFrameOnLine = nsnull;
  *aNumFramesOnLine = 0;
  aLineBounds.SetRect(0, 0, 0, 0);
  
  if ((aLineNumber < 0) || (aLineNumber >=  GetRowCount())) {
    return NS_OK;
  }
  aLineNumber += GetStartRowIndex(); 

  *aNumFramesOnLine = cellMap->GetNumCellsOriginatingInRow(aLineNumber);
  if (*aNumFramesOnLine == 0) {
    return NS_OK;
  }
  PRInt32 colCount = table->GetColCount();
  for (PRInt32 i = 0; i < colCount; i++) {
    CellData* data = cellMap->GetDataAt(aLineNumber, i);
    if (data && data->IsOrig()) {
      *aFirstFrameOnLine = (nsIFrame*)data->GetCellFrame();
      nsIFrame* parent = (*aFirstFrameOnLine)->GetParent();
      aLineBounds = parent->GetRect();
      return NS_OK;
    }
  }
  NS_ERROR("cellmap is lying");
  return NS_ERROR_FAILURE;
}
  
PRInt32
nsTableRowGroupFrame::FindLineContaining(nsIFrame* aFrame)
{
  NS_ENSURE_ARG_POINTER(aFrame);
  
  nsTableRowFrame *rowFrame = do_QueryFrame(aFrame);
  NS_ASSERTION(rowFrame, "RowGroup contains a frame that is not a row");

  return rowFrame->GetRowIndex() - GetStartRowIndex();
}

PRInt32
nsTableRowGroupFrame::FindLineAt(nscoord  aY)
{
  NS_NOTREACHED("Not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

#ifdef IBMBIDI
NS_IMETHODIMP
nsTableRowGroupFrame::CheckLineOrder(PRInt32                  aLine,
                                     PRBool                   *aIsReordered,
                                     nsIFrame                 **aFirstVisual,
                                     nsIFrame                 **aLastVisual)
{
  *aIsReordered = PR_FALSE;
  *aFirstVisual = nsnull;
  *aLastVisual = nsnull;
  return NS_OK;
}
#endif 
  
NS_IMETHODIMP
nsTableRowGroupFrame::FindFrameAt(PRInt32    aLineNumber, 
                                  nscoord    aX, 
                                  nsIFrame** aFrameFound,
                                  PRBool*    aXIsBeforeFirstFrame, 
                                  PRBool*    aXIsAfterLastFrame)
{
   nsTableFrame* table = nsTableFrame::GetTableFrame(this);
   nsTableCellMap* cellMap = table->GetCellMap();
   
   *aFrameFound = nsnull;
   *aXIsBeforeFirstFrame = PR_TRUE;
   *aXIsAfterLastFrame = PR_FALSE;

   aLineNumber += GetStartRowIndex();
   PRInt32 numCells = cellMap->GetNumCellsOriginatingInRow(aLineNumber);
   if (numCells == 0) {
     return NS_OK;
   }
  
   nsIFrame* frame = nsnull;
   PRInt32 colCount = table->GetColCount();
   for (PRInt32 i = 0; i < colCount; i++) {
     CellData* data = cellMap->GetDataAt(aLineNumber, i);
     if (data && data->IsOrig()) {
       frame = (nsIFrame*)data->GetCellFrame();
       break;
     }
   }
   NS_ASSERTION(frame, "cellmap is lying");
   PRBool isRTL = (NS_STYLE_DIRECTION_RTL ==
                   table->GetStyleVisibility()->mDirection);
   
   nsIFrame* closestFromLeft = nsnull;
   nsIFrame* closestFromRight = nsnull;
   PRInt32 n = numCells;
   nsIFrame* firstFrame = frame;
   while (n--) {
     nsRect rect = frame->GetRect();
     if (rect.width > 0) {
       
       if (rect.x <= aX && rect.XMost() > aX) {
         closestFromLeft = closestFromRight = frame;
         break;
       }
       if (rect.x < aX) {
         if (!closestFromLeft ||
             rect.XMost() > closestFromLeft->GetRect().XMost())
           closestFromLeft = frame;
       }
       else {
         if (!closestFromRight ||
             rect.x < closestFromRight->GetRect().x)
           closestFromRight = frame;
       }
     }
     frame = frame->GetNextSibling();
   }
   if (!closestFromLeft && !closestFromRight) {
     
     closestFromLeft = closestFromRight = firstFrame;
   }
   *aXIsBeforeFirstFrame = isRTL ? !closestFromRight : !closestFromLeft;
   *aXIsAfterLastFrame =   isRTL ? !closestFromLeft : !closestFromRight;
   if (closestFromLeft == closestFromRight) {
     *aFrameFound = closestFromLeft;
   }
   else if (!closestFromLeft) {
     *aFrameFound = closestFromRight;
   }
   else if (!closestFromRight) {
     *aFrameFound = closestFromLeft;
   }
   else { 
     nscoord delta = closestFromRight->GetRect().x -
                     closestFromLeft->GetRect().XMost();
     if (aX < closestFromLeft->GetRect().XMost() + delta/2)
       *aFrameFound = closestFromLeft;
     else
       *aFrameFound = closestFromRight;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsTableRowGroupFrame::GetNextSiblingOnLine(nsIFrame*& aFrame, 
                                           PRInt32    aLineNumber)
{
  NS_ENSURE_ARG_POINTER(aFrame);
  aFrame = aFrame->GetNextSibling();
  return NS_OK;
}



static void
DestroyFrameCursorData(void* aObject, nsIAtom* aPropertyName,
                       void* aPropertyValue, void* aData)
{
  delete static_cast<nsTableRowGroupFrame::FrameCursorData*>(aPropertyValue);
}

void
nsTableRowGroupFrame::ClearRowCursor()
{
  if (!(GetStateBits() & NS_ROWGROUP_HAS_ROW_CURSOR))
    return;

  RemoveStateBits(NS_ROWGROUP_HAS_ROW_CURSOR);
  DeleteProperty(nsGkAtoms::rowCursorProperty);
}

nsTableRowGroupFrame::FrameCursorData*
nsTableRowGroupFrame::SetupRowCursor()
{
  if (GetStateBits() & NS_ROWGROUP_HAS_ROW_CURSOR) {
    
    return nsnull;
  }

  nsIFrame* f = mFrames.FirstChild();
  PRInt32 count;
  for (count = 0; f && count < MIN_ROWS_NEEDING_CURSOR; ++count) {
    f = f->GetNextSibling();
  }
  if (!f) {
    
    return nsnull;
  }

  FrameCursorData* data = new FrameCursorData();
  if (!data)
    return nsnull;
  nsresult rv = SetProperty(nsGkAtoms::rowCursorProperty, data,
                            DestroyFrameCursorData);
  if (NS_FAILED(rv)) {
    delete data;
    return nsnull;
  }
  AddStateBits(NS_ROWGROUP_HAS_ROW_CURSOR);
  return data;
}

nsIFrame*
nsTableRowGroupFrame::GetFirstRowContaining(nscoord aY, nscoord* aOverflowAbove)
{
  if (!(GetStateBits() & NS_ROWGROUP_HAS_ROW_CURSOR))
    return nsnull;

  FrameCursorData* property = static_cast<FrameCursorData*>
                                         (GetProperty(nsGkAtoms::rowCursorProperty));
  PRUint32 cursorIndex = property->mCursorIndex;
  PRUint32 frameCount = property->mFrames.Length();
  if (cursorIndex >= frameCount)
    return nsnull;
  nsIFrame* cursorFrame = property->mFrames[cursorIndex];

  
  
  
  
  
  
  
  
  while (cursorIndex > 0 &&
         cursorFrame->GetRect().YMost() + property->mOverflowBelow > aY) {
    --cursorIndex;
    cursorFrame = property->mFrames[cursorIndex];
  }
  while (cursorIndex + 1 < frameCount &&
         cursorFrame->GetRect().YMost() + property->mOverflowBelow <= aY) {
    ++cursorIndex;
    cursorFrame = property->mFrames[cursorIndex];
  }

  property->mCursorIndex = cursorIndex;
  *aOverflowAbove = property->mOverflowAbove;
  return cursorFrame;
}

PRBool
nsTableRowGroupFrame::FrameCursorData::AppendFrame(nsIFrame* aFrame)
{
  nsRect overflowRect = aFrame->GetOverflowRect();
  if (overflowRect.IsEmpty())
    return PR_TRUE;
  nscoord overflowAbove = -overflowRect.y;
  nscoord overflowBelow = overflowRect.YMost() - aFrame->GetSize().height;
  mOverflowAbove = NS_MAX(mOverflowAbove, overflowAbove);
  mOverflowBelow = NS_MAX(mOverflowBelow, overflowBelow);
  return mFrames.AppendElement(aFrame) != nsnull;
}
