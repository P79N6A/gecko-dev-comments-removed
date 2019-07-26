



#include "nsCOMPtr.h"
#include "nsTableRowGroupFrame.h"
#include "nsTableRowFrame.h"
#include "nsTableFrame.h"
#include "nsTableCellFrame.h"
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
#include <algorithm>

using namespace mozilla;

nsTableRowGroupFrame::nsTableRowGroupFrame(nsStyleContext* aContext):
  nsContainerFrame(aContext)
{
  SetRepeatable(false);
}

nsTableRowGroupFrame::~nsTableRowGroupFrame()
{
}

NS_QUERYFRAME_HEAD(nsTableRowGroupFrame)
  NS_QUERYFRAME_ENTRY(nsTableRowGroupFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

int32_t
nsTableRowGroupFrame::GetRowCount()
{
#ifdef DEBUG
  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    NS_ASSERTION(e.get()->GetStyleDisplay()->mDisplay ==
                   NS_STYLE_DISPLAY_TABLE_ROW,
                 "Unexpected display");
    NS_ASSERTION(e.get()->GetType() == nsGkAtoms::tableRowFrame,
                 "Unexpected frame type");
  }
#endif
  
  return mFrames.GetLength();
}

int32_t nsTableRowGroupFrame::GetStartRowIndex()
{
  int32_t result = -1;
  if (mFrames.NotEmpty()) {
    NS_ASSERTION(mFrames.FirstChild()->GetType() == nsGkAtoms::tableRowFrame,
                 "Unexpected frame type");
    result = static_cast<nsTableRowFrame*>(mFrames.FirstChild())->GetRowIndex();
  }
  
  if (-1 == result) {
    nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
    return tableFrame->GetStartRowIndex(this);
  }
      
  return result;
}

void  nsTableRowGroupFrame::AdjustRowIndices(int32_t aRowIndex,
                                             int32_t anAdjustment)
{
  nsIFrame* rowFrame = GetFirstPrincipalChild();
  for ( ; rowFrame; rowFrame = rowFrame->GetNextSibling()) {
    if (NS_STYLE_DISPLAY_TABLE_ROW==rowFrame->GetStyleDisplay()->mDisplay) {
      int32_t index = ((nsTableRowFrame*)rowFrame)->GetRowIndex();
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
      int32_t colIndex;
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
  nsDisplayTableRowGroupBackground(nsDisplayListBuilder* aBuilder,
                                   nsTableRowGroupFrame* aFrame) :
    nsDisplayTableItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayTableRowGroupBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTableRowGroupBackground() {
    MOZ_COUNT_DTOR(nsDisplayTableRowGroupBackground);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);

  NS_DISPLAY_DECL_NAME("TableRowGroupBackground", TYPE_TABLE_ROW_GROUP_BACKGROUND)
};

void
nsDisplayTableRowGroupBackground::Paint(nsDisplayListBuilder* aBuilder,
                                        nsRenderingContext* aCtx)
{
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(mFrame);
  TableBackgroundPainter painter(tableFrame,
                                 TableBackgroundPainter::eOrigin_TableRowGroup,
                                 mFrame->PresContext(), *aCtx,
                                 mVisibleRect, ToReferenceFrame(),
                                 aBuilder->GetBackgroundPaintFlags());
  painter.PaintRowGroup(static_cast<nsTableRowGroupFrame*>(mFrame));
}


static nsresult
DisplayRows(nsDisplayListBuilder* aBuilder, nsFrame* aFrame,
            const nsRect& aDirtyRect, const nsDisplayListSet& aLists)
{
  nscoord overflowAbove;
  nsTableRowGroupFrame* f = static_cast<nsTableRowGroupFrame*>(aFrame);
  
  
  
  
  
  
  
  
  nsIFrame* kid = aBuilder->ShouldDescendIntoFrame(f) ?
    nullptr : f->GetFirstRowContaining(aDirtyRect.y, &overflowAbove);
  
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
  kid = f->GetFirstPrincipalChild();
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
  nsDisplayTableItem* item = nullptr;
  if (IsVisibleInSelection(aBuilder)) {
    bool isRoot = aBuilder->IsAtRootOfPseudoStackingContext();
    if (isRoot) {
      
      
      
      item = new (aBuilder) nsDisplayTableRowGroupBackground(aBuilder, this);
      nsresult rv = aLists.BorderBackground()->AppendNewToTop(item);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }  
  return nsTableFrame::DisplayGenericTablePart(aBuilder, this, aDirtyRect,
                                               aLists, item, DisplayRows);
}

int
nsTableRowGroupFrame::GetSkipSides() const
{
  int skip = 0;
  if (nullptr != GetPrevInFlow()) {
    skip |= 1 << NS_SIDE_TOP;
  }
  if (nullptr != GetNextInFlow()) {
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
                                 const nsRect&          aOriginalKidVisualOverflow)
{
  bool isFirstReflow =
    (aKidFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;

  
  FinishReflowChild(aKidFrame, aPresContext, nullptr, aDesiredSize, 0,
                    aReflowState.y, 0);

  nsTableFrame::InvalidateTableFrame(aKidFrame, aOriginalKidRect,
                                     aOriginalKidVisualOverflow, isFirstReflow);

  
  aReflowState.y += aDesiredSize.height;

  
  if (NS_UNCONSTRAINEDSIZE != aReflowState.availSize.height) {
    aReflowState.availSize.height -= aDesiredSize.height;
  }
}

void
nsTableRowGroupFrame::InitChildReflowState(nsPresContext&     aPresContext, 
                                           bool               aBorderCollapse,
                                           nsHTMLReflowState& aReflowState)                                    
{
  nsMargin collapseBorder;
  nsMargin padding(0,0,0,0);
  nsMargin* pCollapseBorder = nullptr;
  if (aBorderCollapse) {
    nsTableRowFrame *rowFrame = do_QueryFrame(aReflowState.frame);
    if (rowFrame) {
      pCollapseBorder = rowFrame->GetBCBorderWidth(collapseBorder);
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
      row->SetHasUnpaginatedHeight(true);
      row->SetUnpaginatedHeight(aPresContext, row->GetSize().height);
    }
  }
}

nsresult
nsTableRowGroupFrame::ReflowChildren(nsPresContext*         aPresContext,
                                     nsHTMLReflowMetrics&   aDesiredSize,
                                     nsRowGroupReflowState& aReflowState,
                                     nsReflowStatus&        aStatus,
                                     bool*                aPageBreakBeforeEnd)
{
  if (aPageBreakBeforeEnd) 
    *aPageBreakBeforeEnd = false;

  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  nsresult rv = NS_OK;
  const bool borderCollapse = tableFrame->IsBorderCollapse();
  nscoord cellSpacingY = tableFrame->GetCellSpacingY();

  
  
  bool isPaginated = aPresContext->IsPaginated() && 
                       NS_UNCONSTRAINEDSIZE != aReflowState.availSize.height;

  bool haveRow = false;
  bool reflowAllKids = aReflowState.reflowState.ShouldReflowAllKids() ||
                         tableFrame->IsGeometryDirty();
  bool needToCalcRowHeights = reflowAllKids;

  nsIFrame *prevKidFrame = nullptr;
  for (nsIFrame* kidFrame = mFrames.FirstChild(); kidFrame;
       prevKidFrame = kidFrame, kidFrame = kidFrame->GetNextSibling()) {
    nsTableRowFrame *rowFrame = do_QueryFrame(kidFrame);
    if (!rowFrame) {
      
      NS_NOTREACHED("yikes, a non-row child");
      continue;
    }

    haveRow = true;

    
    if (reflowAllKids ||
        NS_SUBTREE_DIRTY(kidFrame) ||
        (aReflowState.reflowState.mFlags.mSpecialHeightReflow &&
         (isPaginated || (kidFrame->GetStateBits() &
                          NS_FRAME_CONTAINS_RELATIVE_HEIGHT)))) {
      nsRect oldKidRect = kidFrame->GetRect();
      nsRect oldKidVisualOverflow = kidFrame->GetVisualOverflowRect();

      
      
      nsHTMLReflowMetrics desiredSize(aDesiredSize.mFlags);
      desiredSize.width = desiredSize.height = 0;
  
      
      
      
      nsSize kidAvailSize(aReflowState.availSize.width, NS_UNCONSTRAINEDSIZE);
      nsHTMLReflowState kidReflowState(aPresContext, aReflowState.reflowState,
                                       kidFrame, kidAvailSize,
                                       -1, -1, false);
      InitChildReflowState(*aPresContext, borderCollapse, kidReflowState);

      
      if (aReflowState.reflowState.mFlags.mHResize)
        kidReflowState.mFlags.mHResize = true;
     
      NS_ASSERTION(kidFrame == mFrames.FirstChild() || prevKidFrame, 
                   "If we're not on the first frame, we should have a "
                   "previous sibling...");
      
      if (prevKidFrame && prevKidFrame->GetRect().YMost() > 0) {
        kidReflowState.mFlags.mIsTopOfPage = false;
      }

      rv = ReflowChild(kidFrame, aPresContext, desiredSize, kidReflowState,
                       0, aReflowState.y, NS_FRAME_INVALIDATE_ON_MOVE,
                       aStatus);

      
      PlaceChild(aPresContext, aReflowState, kidFrame, desiredSize,
                 oldKidRect, oldKidVisualOverflow);
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
            InvalidateFrame();
          }
          else if (oldKidRect.height != desiredSize.height)
            needToCalcRowHeights = true;
        } else {
          needToCalcRowHeights = true;
        }
      }

      if (isPaginated && aPageBreakBeforeEnd && !*aPageBreakBeforeEnd) {
        nsTableRowFrame* nextRow = rowFrame->GetNextRow();
        if (nextRow) {
          *aPageBreakBeforeEnd = nsTableFrame::PageBreakAfter(kidFrame, nextRow);
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
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, kidFrame);
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
      InvalidateFrame();
    }
  }

  return rv;
}

nsTableRowFrame*  
nsTableRowGroupFrame::GetFirstRow() 
{
  for (nsIFrame* childFrame = mFrames.FirstChild(); childFrame;
       childFrame = childFrame->GetNextSibling()) {
    nsTableRowFrame *rowFrame = do_QueryFrame(childFrame);
    if (rowFrame) {
      return rowFrame;
    }
  }
  return nullptr;
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
  
  
  
  aDesiredSize.mOverflowAreas.Clear();
  for (nsTableRowFrame* rowFrame = GetFirstRow();
       rowFrame; rowFrame = rowFrame->GetNextRow()) {
    rowFrame->DidResize();
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, rowFrame);
  }
}






void 
nsTableRowGroupFrame::CalculateRowHeights(nsPresContext*           aPresContext, 
                                          nsHTMLReflowMetrics&     aDesiredSize,
                                          const nsHTMLReflowState& aReflowState)
{
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  const bool isPaginated = aPresContext->IsPaginated();

  
  nscoord cellSpacingY = tableFrame->GetCellSpacingY();

  int32_t numEffCols = tableFrame->GetEffectiveColCount();

  int32_t startRowIndex = GetStartRowIndex();
  
  nsTableRowFrame* startRowFrame = GetFirstRow();

  if (!startRowFrame) return;

  
  nscoord startRowGroupHeight = startRowFrame->GetPosition().y;

  int32_t numRows = GetRowCount() - (startRowFrame->GetRowIndex() - GetStartRowIndex());
  
  if (numRows <= 0)
    return;

  nsTArray<RowInfo> rowInfo;
  if (!rowInfo.AppendElements(numRows)) {
    return;
  }

  bool    hasRowSpanningCell = false;
  nscoord heightOfRows = 0;
  nscoord heightOfUnStyledRows = 0;
  
  
  
  nscoord pctHeightBasis = GetHeightBasis(aReflowState);
  int32_t rowIndex; 
  nsTableRowFrame* rowFrame;
  for (rowFrame = startRowFrame, rowIndex = 0; rowFrame; rowFrame = rowFrame->GetNextRow(), rowIndex++) {
    nscoord nonPctHeight = rowFrame->GetContentHeight();
    if (isPaginated) {
      nonPctHeight = std::max(nonPctHeight, rowFrame->GetSize().height);
    }
    if (!rowFrame->GetPrevInFlow()) {
      if (rowFrame->HasPctHeight()) {
        rowInfo[rowIndex].hasPctHeight = true;
        rowInfo[rowIndex].pctHeight = rowFrame->GetHeight(pctHeightBasis);
      }
      rowInfo[rowIndex].hasStyleHeight = rowFrame->HasStyleHeight();
      nonPctHeight = std::max(nonPctHeight, rowFrame->GetFixedHeight());
    }
    UpdateHeights(rowInfo[rowIndex], nonPctHeight, heightOfRows, heightOfUnStyledRows);

    if (!rowInfo[rowIndex].hasStyleHeight) {
      if (isPaginated || tableFrame->HasMoreThanOneCell(rowIndex + startRowIndex)) {
        rowInfo[rowIndex].isSpecial = true;
        
        nsTableCellFrame* cellFrame = rowFrame->GetFirstCell();
        while (cellFrame) {
          int32_t rowSpan = tableFrame->GetEffectiveRowSpan(rowIndex + startRowIndex, *cellFrame);
          if (1 == rowSpan) { 
            rowInfo[rowIndex].isSpecial = false;
            break;
          }
          cellFrame = cellFrame->GetNextCell(); 
        }
      }
    }
    
    if (!hasRowSpanningCell) {
      if (tableFrame->RowIsSpannedInto(rowIndex + startRowIndex, numEffCols)) {
        hasRowSpanningCell = true;
      }
    }
  }

  if (hasRowSpanningCell) {
    
    
    for (rowFrame = startRowFrame, rowIndex = 0; rowFrame; rowFrame = rowFrame->GetNextRow(), rowIndex++) {
      
      
      
      if (GetPrevInFlow() || tableFrame->RowHasSpanningCells(startRowIndex + rowIndex, numEffCols)) {
        nsTableCellFrame* cellFrame = rowFrame->GetFirstCell();
        
        while (cellFrame) {
          int32_t rowSpan = tableFrame->GetEffectiveRowSpan(rowIndex + startRowIndex, *cellFrame);
          if ((rowIndex + rowSpan) > numRows) {
            
            rowSpan = numRows - rowIndex;
          }
          if (rowSpan > 1) { 
            nscoord heightOfRowsSpanned = 0;
            nscoord heightOfUnStyledRowsSpanned = 0;
            nscoord numSpecialRowsSpanned = 0; 
            nscoord cellSpacingTotal = 0;
            int32_t spanX;
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
            rowFrame->CalculateCellActualHeight(cellFrame, cellDesSize.height);
            cellFrameSize.height = cellDesSize.height;
            if (cellFrame->HasVerticalAlignBaseline()) {
              
              
              
              cellFrameSize.height += rowFrame->GetMaxCellAscent() -
                                      cellFrame->GetCellBaseline();
            }
  
            if (heightOfAreaSpanned < cellFrameSize.height) {
              
              
              nscoord extra     = cellFrameSize.height - heightOfAreaSpanned;
              nscoord extraUsed = 0;
              if (0 == numSpecialRowsSpanned) {
                
                bool haveUnStyledRowsSpanned = (heightOfUnStyledRowsSpanned > 0);
                nscoord divisor = (haveUnStyledRowsSpanned) 
                                  ? heightOfUnStyledRowsSpanned : heightOfRowsSpanned;
                if (divisor > 0) {
                  for (spanX = rowSpan - 1; spanX >= 0; spanX--) {
                    if (!haveUnStyledRowsSpanned || !rowInfo[rowIndex + spanX].hasStyleHeight) {
                      
                      float percent = ((float)rowInfo[rowIndex + spanX].height) / ((float)divisor);
                    
                      
                      nscoord extraForRow = (0 == spanX) ? extra - extraUsed  
                                                         : NSToCoordRound(((float)(extra)) * percent);
                      extraForRow = std::min(extraForRow, extra - extraUsed);
                      
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
                    extraForRow = std::min(extraForRow, extra - extraUsed);
                    
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
      rowExtra = std::min(rowExtra, extra);
      UpdateHeights(rInfo, rowExtra, heightOfRows, heightOfUnStyledRows);
      extra -= rowExtra;
    }
  }

  bool styleHeightAllocation = false;
  nscoord rowGroupHeight = startRowGroupHeight + heightOfRows + ((numRows - 1) * cellSpacingY);
  
  if ((aReflowState.ComputedHeight() > rowGroupHeight) && 
      (NS_UNCONSTRAINEDSIZE != aReflowState.ComputedHeight())) {
    nscoord extraComputedHeight = aReflowState.ComputedHeight() - rowGroupHeight;
    nscoord extraUsed = 0;
    bool haveUnStyledRows = (heightOfUnStyledRows > 0);
    nscoord divisor = (haveUnStyledRows) 
                      ? heightOfUnStyledRows : heightOfRows;
    if (divisor > 0) {
      styleHeightAllocation = true;
      for (rowIndex = 0; rowIndex < numRows; rowIndex++) {
        if (!haveUnStyledRows || !rowInfo[rowIndex].hasStyleHeight) {
          
          
          float percent = ((float)rowInfo[rowIndex].height) / ((float)divisor);
          
          nscoord extraForRow = (numRows - 1 == rowIndex) 
                                ? extraComputedHeight - extraUsed  
                                : NSToCoordRound(((float)extraComputedHeight) * percent);
          extraForRow = std::min(extraForRow, extraComputedHeight - extraUsed);
          
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
    nsRect rowVisualOverflow = rowFrame->GetVisualOverflowRect();

    bool movedFrame = (rowBounds.y != yOrigin);  
    nscoord rowHeight = (rowInfo[rowIndex].height > 0) ? rowInfo[rowIndex].height : 0;
    
    if (movedFrame || (rowHeight != rowBounds.height)) {
      
      if (movedFrame) {
        rowFrame->InvalidateFrameSubtree();
      }
      
      rowFrame->SetRect(nsRect(rowBounds.x, yOrigin, rowBounds.width,
                               rowHeight));

      nsTableFrame::InvalidateTableFrame(rowFrame, rowBounds, rowVisualOverflow,
                                         false);
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
  bool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE == groupVis->mVisible);
  if (collapseGroup) {
    tableFrame->SetNeedToCollapse(true);
  }

  nsOverflowAreas overflow;

  nsTableRowFrame* rowFrame= GetFirstRow();
  bool didCollapse = false;
  nscoord yGroupOffset = 0;
  while (rowFrame) {
    yGroupOffset += rowFrame->CollapseRowIfNecessary(yGroupOffset,
                                                     aWidth, collapseGroup,
                                                     didCollapse);
    ConsiderChildOverflow(overflow, rowFrame);
    rowFrame = rowFrame->GetNextRow();
  }

  nsRect groupRect = GetRect();
  nsRect oldGroupRect = groupRect;
  nsRect oldGroupVisualOverflow = GetVisualOverflowRect();
  
  groupRect.height -= yGroupOffset;
  if (didCollapse) {
    
    groupRect.height += tableFrame->GetCellSpacingY();
  }

  groupRect.y -= aYTotalOffset;
  groupRect.width = aWidth;

  if (aYTotalOffset != 0) {
    InvalidateFrameSubtree();
  }
  
  SetRect(groupRect);
  overflow.UnionAllWith(nsRect(0, 0, groupRect.width, groupRect.height));
  FinishAndStoreOverflow(overflow, nsSize(groupRect.width, groupRect.height));
  nsTableFrame::RePositionViews(this);
  nsTableFrame::InvalidateTableFrame(this, oldGroupRect, oldGroupVisualOverflow,
                                     false);

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
    aKidFrame->InvalidateFrameSubtree();
    aKidFrame->SetPosition(newPosition);
    nsTableFrame::RePositionViews(aKidFrame);
    aKidFrame->InvalidateFrameSubtree();
  }
}



void 
nsTableRowGroupFrame::CreateContinuingRowFrame(nsPresContext& aPresContext,
                                               nsIFrame&      aRowFrame,
                                               nsIFrame**     aContRowFrame)
{
  
  if (!aContRowFrame) {NS_ASSERTION(false, "bad call"); return;}
  
  nsresult rv = aPresContext.PresShell()->FrameConstructor()->
    CreateContinuingFrame(&aPresContext, &aRowFrame, this, aContRowFrame);
  if (NS_FAILED(rv)) {
    *aContRowFrame = nullptr;
    return;
  }

  
  mFrames.InsertFrame(nullptr, &aRowFrame, *aContRowFrame);

  
  PushChildren(&aPresContext, *aContRowFrame, &aRowFrame);
}




void
nsTableRowGroupFrame::SplitSpanningCells(nsPresContext&           aPresContext,
                                         const nsHTMLReflowState& aReflowState,
                                         nsTableFrame&            aTable,
                                         nsTableRowFrame&         aFirstRow, 
                                         nsTableRowFrame&         aLastRow,  
                                         bool                     aFirstRowIsTopOfPage,
                                         nscoord                  aSpanningRowBottom,
                                         nsTableRowFrame*&        aContRow,
                                         nsTableRowFrame*&        aFirstTruncatedRow,
                                         nscoord&                 aDesiredHeight)
{
  NS_ASSERTION(aSpanningRowBottom >= 0, "Can't split negative heights");
  aFirstTruncatedRow = nullptr;
  aDesiredHeight     = 0;

  const bool borderCollapse = aTable.IsBorderCollapse();
  int32_t lastRowIndex = aLastRow.GetRowIndex();
  bool wasLast = false;
  bool haveRowSpan = false;
  
  for (nsTableRowFrame* row = &aFirstRow; !wasLast; row = row->GetNextRow()) {
    wasLast = (row == &aLastRow);
    int32_t rowIndex = row->GetRowIndex();
    nsPoint rowPos = row->GetPosition();
    
    for (nsTableCellFrame* cell = row->GetFirstCell(); cell; cell = cell->GetNextCell()) {
      int32_t rowSpan = aTable.GetEffectiveRowSpan(rowIndex, *cell);
      
      
      if ((rowSpan > 1) && (rowIndex + rowSpan > lastRowIndex)) {
        haveRowSpan = true;
        nsReflowStatus status;
        
        
        nscoord cellAvailHeight = aSpanningRowBottom - rowPos.y;
        NS_ASSERTION(cellAvailHeight >= 0, "No space for cell?");
        bool isTopOfPage = (row == &aFirstRow) && aFirstRowIsTopOfPage;

        nsRect rowRect = row->GetRect();
        nsSize rowAvailSize(aReflowState.availableWidth,
                            std::max(aReflowState.availableHeight - rowRect.y,
                                   0));
        
        
        rowAvailSize.height = std::min(rowAvailSize.height, rowRect.height);
        nsHTMLReflowState rowReflowState(&aPresContext, aReflowState,
                                         row, rowAvailSize,
                                         -1, -1, false);
        InitChildReflowState(aPresContext, borderCollapse, rowReflowState);
        rowReflowState.mFlags.mIsTopOfPage = isTopOfPage; 

        nscoord cellHeight = row->ReflowCellFrame(&aPresContext, rowReflowState,
                                                  isTopOfPage, cell,
                                                  cellAvailHeight, status);
        aDesiredHeight = std::max(aDesiredHeight, rowPos.y + cellHeight);
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
              
              
              nsTableCellFrame* contCell = nullptr;
              aPresContext.PresShell()->FrameConstructor()->
                CreateContinuingFrame(&aPresContext, cell, &aLastRow,
                                      (nsIFrame**)&contCell);
              int32_t colIndex;
              cell->GetColIndex(colIndex);
              aContRow->InsertCellFrame(contCell, colIndex);
            }
          }
        }
      }
    }
  }
  if (!haveRowSpan) {
    aDesiredHeight = aLastRow.GetRect().YMost();
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

  if (overflows->IsEmpty())
    return;
  
  mFrames.InsertFrames(nullptr, rowBefore, *overflows);
}

static nsTableRowFrame* 
GetRowBefore(nsTableRowFrame& aStartRow,
             nsTableRowFrame& aRow)
{
  nsTableRowFrame* rowBefore = nullptr;
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
                                    nsReflowStatus&          aStatus,
                                    bool                     aRowForcedPageBreak)
{
  NS_PRECONDITION(aPresContext->IsPaginated(), "SplitRowGroup currently supports only paged media"); 

  nsresult rv = NS_OK;
  nsTableRowFrame* prevRowFrame = nullptr;
  aDesiredSize.height = 0;

  nscoord availWidth  = aReflowState.availableWidth;
  nscoord availHeight = aReflowState.availableHeight;
  
  const bool borderCollapse = aTableFrame->IsBorderCollapse();
  nscoord cellSpacingY = aTableFrame->GetCellSpacingY();
  
  
  nscoord pageHeight = aPresContext->GetPageSize().height;
  NS_ASSERTION(pageHeight != NS_UNCONSTRAINEDSIZE, 
               "The table shouldn't be split when there should be space");

  bool isTopOfPage = aReflowState.mFlags.mIsTopOfPage;
  nsTableRowFrame* firstRowThisPage = GetFirstRow();

  
  
  aTableFrame->SetGeometryDirty();

  
  
  for (nsTableRowFrame* rowFrame = firstRowThisPage; rowFrame; rowFrame = rowFrame->GetNextRow()) {
    bool rowIsOnPage = true;
    nsRect rowRect = rowFrame->GetRect();
    
    if (rowRect.YMost() > availHeight) {
      nsTableRowFrame* contRow = nullptr;
      
      
      
      if (!prevRowFrame || (availHeight - aDesiredSize.height > pageHeight / 20)) { 
        nsSize availSize(availWidth, std::max(availHeight - rowRect.y, 0));
        
        availSize.height = std::min(availSize.height, rowRect.height);

        nsHTMLReflowState rowReflowState(aPresContext, aReflowState,
                                         rowFrame, availSize,
                                         -1, -1, false);
                                         
        InitChildReflowState(*aPresContext, borderCollapse, rowReflowState);
        rowReflowState.mFlags.mIsTopOfPage = isTopOfPage; 
        nsHTMLReflowMetrics rowMetrics;

        
        nsRect oldRowRect = rowFrame->GetRect();
        nsRect oldRowVisualOverflow = rowFrame->GetVisualOverflowRect();

        
        
        rv = ReflowChild(rowFrame, aPresContext, rowMetrics, rowReflowState,
                         0, 0, NS_FRAME_NO_MOVE_FRAME, aStatus);
        if (NS_FAILED(rv)) return rv;
        rowFrame->SetSize(nsSize(rowMetrics.width, rowMetrics.height));
        rowFrame->DidReflow(aPresContext, nullptr, nsDidReflowStatus::FINISHED);
        rowFrame->DidResize();

        if (!aRowForcedPageBreak && !NS_FRAME_IS_FULLY_COMPLETE(aStatus) &&
            ShouldAvoidBreakInside(aReflowState)) {
          aStatus = NS_INLINE_LINE_BREAK_BEFORE();
          break;
        }

        nsTableFrame::InvalidateTableFrame(rowFrame, oldRowRect,
                                           oldRowVisualOverflow,
                                           false);

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
            
            rowIsOnPage = false;
          }
        } 
        else {
          
          
          
          
          
          if (rowMetrics.height > availSize.height ||
              (NS_INLINE_IS_BREAK_BEFORE(aStatus) && !aRowForcedPageBreak)) {
            
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
              
              rowIsOnPage = false;
            }
          }
        }
      } 
      else { 
        
        rowIsOnPage = false;
      }

      nsTableRowFrame* lastRowThisPage = rowFrame;
      nscoord spanningRowBottom = availHeight;
      if (!rowIsOnPage) {
        NS_ASSERTION(!contRow, "We should not have created a continuation if none of this row fits");
        if (!aRowForcedPageBreak && ShouldAvoidBreakInside(aReflowState)) {
          aStatus = NS_INLINE_LINE_BREAK_BEFORE();
          break;
        }
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
            contRow = nullptr;
          }
        }
        else { 
          
          nsTableRowFrame* rowBefore = ::GetRowBefore(*firstRowThisPage, *firstTruncatedRow);
          nscoord oldSpanningRowBottom = spanningRowBottom;
          spanningRowBottom = rowBefore->GetRect().YMost();

          UndoContinuedRow(aPresContext, contRow);
          contRow = nullptr;
          nsTableRowFrame* oldLastRowThisPage = lastRowThisPage;
          lastRowThisPage = rowBefore;
          aStatus = NS_FRAME_NOT_COMPLETE;

          
          SplitSpanningCells(*aPresContext, aReflowState, *aTableFrame, 
                             *firstRowThisPage, *rowBefore, aReflowState.mFlags.mIsTopOfPage, 
                             spanningRowBottom, contRow, firstTruncatedRow, aDesiredSize.height);
          if (firstTruncatedRow) {
            if (aReflowState.mFlags.mIsTopOfPage) {
              
              UndoContinuedRow(aPresContext, contRow);
              contRow = nullptr;
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
              contRow = nullptr;
            }
          }
        } 
      } 
      else {
        aDesiredSize.height = std::max(aDesiredSize.height, yMost);
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
      if (nextRow && nsTableFrame::PageBreakAfter(rowFrame, nextRow)) {
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

  
  ClearRowCursor();

  
  nsTableFrame::CheckRequestSpecialHeightReflow(aReflowState);

  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  nsRowGroupReflowState state(aReflowState, tableFrame);
  const nsStyleVisibility* groupVis = GetStyleVisibility();
  bool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE == groupVis->mVisible);
  if (collapseGroup) {
    tableFrame->SetNeedToCollapse(true);
  }

  
  MoveOverflowToChildList(aPresContext);

  
  bool splitDueToPageBreak = false;
  rv = ReflowChildren(aPresContext, aDesiredSize, state, aStatus,
                      &splitDueToPageBreak);

  
  
  if (aReflowState.mFlags.mTableIsSplittable &&
      NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight &&
      (NS_FRAME_NOT_COMPLETE == aStatus || splitDueToPageBreak || 
       aDesiredSize.height > aReflowState.availableHeight)) {
    
    bool specialReflow = (bool)aReflowState.mFlags.mSpecialHeightReflow;
    ((nsHTMLReflowState::ReflowStateFlags&)aReflowState.mFlags).mSpecialHeightReflow = false;

    SplitRowGroup(aPresContext, aDesiredSize, aReflowState, tableFrame, aStatus,
                  splitDueToPageBreak);

    ((nsHTMLReflowState::ReflowStateFlags&)aReflowState.mFlags).mSpecialHeightReflow = specialReflow;
  }

  
  
  
  if (GetNextInFlow() && GetNextInFlow()->GetFirstPrincipalChild()) {
    NS_FRAME_SET_INCOMPLETE(aStatus);
  }

  SetHasStyleHeight((NS_UNCONSTRAINEDSIZE != aReflowState.ComputedHeight()) &&
                    (aReflowState.ComputedHeight() > 0)); 
  
  
  aDesiredSize.width = aReflowState.availableWidth;

  aDesiredSize.UnionOverflowAreasWithDesiredBounds();

  
  
  if (!(GetParent()->GetStateBits() & NS_FRAME_FIRST_REFLOW) &&
      nsSize(aDesiredSize.width, aDesiredSize.height) != mRect.Size()) {
    InvalidateFrame();
  }
  
  FinishAndStoreOverflow(&aDesiredSize);
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return rv;
}

 void
nsTableRowGroupFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsContainerFrame::DidSetStyleContext(aOldStyleContext);

  if (!aOldStyleContext) 
    return;
     
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  if (tableFrame->IsBorderCollapse() &&
      tableFrame->BCRecalcNeeded(aOldStyleContext, StyleContext())) {
    nsIntRect damageArea(0, GetStartRowIndex(), tableFrame->GetColCount(),
                         GetRowCount());
    tableFrame->AddBCDamageArea(damageArea);
  }
}

NS_IMETHODIMP
nsTableRowGroupFrame::AppendFrames(ChildListID     aListID,
                                   nsFrameList&    aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");

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

  int32_t rowIndex = GetRowCount();
  
  mFrames.AppendFrames(nullptr, aFrameList);

  if (rows.Length() > 0) {
    nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
    tableFrame->AppendRows(this, rowIndex, rows);
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
    tableFrame->SetGeometryDirty();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTableRowGroupFrame::InsertFrames(ChildListID     aListID,
                                   nsIFrame*       aPrevFrame,
                                   nsFrameList&    aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  ClearRowCursor();

  
  
  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  nsTArray<nsTableRowFrame*> rows;
  bool gotFirstRow = false;
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    nsTableRowFrame *rowFrame = do_QueryFrame(e.get());
    NS_ASSERTION(rowFrame, "Unexpected frame; frame constructor screwed up");
    if (rowFrame) {
      NS_ASSERTION(NS_STYLE_DISPLAY_TABLE_ROW ==
                     e.get()->GetStyleDisplay()->mDisplay,
                   "wrong display type on rowframe");      
      rows.AppendElement(rowFrame);
      if (!gotFirstRow) {
        rowFrame->SetFirstInserted(true);
        gotFirstRow = true;
        tableFrame->SetRowInserted(true);
      }
    }
  }

  int32_t startRowIndex = GetStartRowIndex();
  
  mFrames.InsertFrames(nullptr, aPrevFrame, aFrameList);

  int32_t numRows = rows.Length();
  if (numRows > 0) {
    nsTableRowFrame* prevRow = (nsTableRowFrame *)nsTableFrame::GetFrameAtOrBefore(this, aPrevFrame, nsGkAtoms::tableRowFrame);
    int32_t rowIndex = (prevRow) ? prevRow->GetRowIndex() + 1 : startRowIndex;
    tableFrame->InsertRows(this, rows, rowIndex, true);

    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
    tableFrame->SetGeometryDirty();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsTableRowGroupFrame::RemoveFrame(ChildListID     aListID,
                                  nsIFrame*       aOldFrame)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");

  ClearRowCursor();

  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
  
  nsTableRowFrame* rowFrame = do_QueryFrame(aOldFrame);
  if (rowFrame) {
    
    tableFrame->RemoveRows(*rowFrame, 1, true);

    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
    tableFrame->SetGeometryDirty();
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
  if ((aReflowState.ComputedHeight() > 0) && (aReflowState.ComputedHeight() < NS_UNCONSTRAINEDSIZE)) {
    nscoord cellSpacing = std::max(0, GetRowCount() - 1) * tableFrame->GetCellSpacingY();
    result = aReflowState.ComputedHeight() - cellSpacing;
  }
  else {
    const nsHTMLReflowState* parentRS = aReflowState.parentReflowState;
    if (parentRS && (tableFrame != parentRS->frame)) {
      parentRS = parentRS->parentReflowState;
    }
    if (parentRS && (tableFrame == parentRS->frame) && 
        (parentRS->ComputedHeight() > 0) && (parentRS->ComputedHeight() < NS_UNCONSTRAINEDSIZE)) {
      nscoord cellSpacing = std::max(0, tableFrame->GetRowCount() + 1) * tableFrame->GetCellSpacingY();
      result = parentRS->ComputedHeight() - cellSpacing;
    }
  }

  return result;
}

bool
nsTableRowGroupFrame::IsSimpleRowFrame(nsTableFrame* aTableFrame,
                                       nsIFrame*     aFrame)
{
  
  nsTableRowFrame *rowFrame = do_QueryFrame(aFrame);
  if (rowFrame) {
    int32_t rowIndex = rowFrame->GetRowIndex();
    
    
    
    int32_t numEffCols = aTableFrame->GetEffectiveColCount();
    if (!aTableFrame->RowIsSpannedInto(rowIndex, numEffCols) &&
        !aTableFrame->RowHasSpanningCells(rowIndex, numEffCols)) {
      return true;
    }
  }

  return false;
}

nsIAtom*
nsTableRowGroupFrame::GetType() const
{
  return nsGkAtoms::tableRowGroupFrame;
}


bool 
nsTableRowGroupFrame::HasInternalBreakBefore() const
{
 nsIFrame* firstChild = mFrames.FirstChild(); 
  if (!firstChild)
    return false;
  return firstChild->GetStyleDisplay()->mBreakBefore;
}


bool 
nsTableRowGroupFrame::HasInternalBreakAfter() const
{
  nsIFrame* lastChild = mFrames.LastChild(); 
  if (!lastChild)
    return false;
  return lastChild->GetStyleDisplay()->mBreakAfter;
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

  nsTableRowFrame* firstRowFrame = nullptr;
  nsTableRowFrame* lastRowFrame = nullptr;
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

void nsTableRowGroupFrame::SetContinuousBCBorderWidth(uint8_t     aForSide,
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


int32_t
nsTableRowGroupFrame::GetNumLines()
{
  return GetRowCount();
}

bool
nsTableRowGroupFrame::GetDirection()
{
  nsTableFrame* table = nsTableFrame::GetTableFrame(this);
  return (NS_STYLE_DIRECTION_RTL ==
          table->GetStyleVisibility()->mDirection);
}
  
NS_IMETHODIMP
nsTableRowGroupFrame::GetLine(int32_t    aLineNumber, 
                              nsIFrame** aFirstFrameOnLine, 
                              int32_t*   aNumFramesOnLine,
                              nsRect&    aLineBounds, 
                              uint32_t*  aLineFlags)
{
  NS_ENSURE_ARG_POINTER(aFirstFrameOnLine);
  NS_ENSURE_ARG_POINTER(aNumFramesOnLine);
  NS_ENSURE_ARG_POINTER(aLineFlags);

  nsTableFrame* table = nsTableFrame::GetTableFrame(this);
  nsTableCellMap* cellMap = table->GetCellMap();

  *aLineFlags = 0;
  *aFirstFrameOnLine = nullptr;
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
  int32_t colCount = table->GetColCount();
  for (int32_t i = 0; i < colCount; i++) {
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
  
int32_t
nsTableRowGroupFrame::FindLineContaining(nsIFrame* aFrame, int32_t aStartLine)
{
  NS_ENSURE_TRUE(aFrame, -1);
  
  nsTableRowFrame *rowFrame = do_QueryFrame(aFrame);
  NS_ASSERTION(rowFrame, "RowGroup contains a frame that is not a row");

  int32_t rowIndexInGroup = rowFrame->GetRowIndex() - GetStartRowIndex();

  return rowIndexInGroup >= aStartLine ? rowIndexInGroup : -1;
}

#ifdef IBMBIDI
NS_IMETHODIMP
nsTableRowGroupFrame::CheckLineOrder(int32_t                  aLine,
                                     bool                     *aIsReordered,
                                     nsIFrame                 **aFirstVisual,
                                     nsIFrame                 **aLastVisual)
{
  *aIsReordered = false;
  *aFirstVisual = nullptr;
  *aLastVisual = nullptr;
  return NS_OK;
}
#endif 
  
NS_IMETHODIMP
nsTableRowGroupFrame::FindFrameAt(int32_t    aLineNumber, 
                                  nscoord    aX, 
                                  nsIFrame** aFrameFound,
                                  bool*    aXIsBeforeFirstFrame, 
                                  bool*    aXIsAfterLastFrame)
{
   nsTableFrame* table = nsTableFrame::GetTableFrame(this);
   nsTableCellMap* cellMap = table->GetCellMap();
   
   *aFrameFound = nullptr;
   *aXIsBeforeFirstFrame = true;
   *aXIsAfterLastFrame = false;

   aLineNumber += GetStartRowIndex();
   int32_t numCells = cellMap->GetNumCellsOriginatingInRow(aLineNumber);
   if (numCells == 0) {
     return NS_OK;
   }
  
   nsIFrame* frame = nullptr;
   int32_t colCount = table->GetColCount();
   for (int32_t i = 0; i < colCount; i++) {
     CellData* data = cellMap->GetDataAt(aLineNumber, i);
     if (data && data->IsOrig()) {
       frame = (nsIFrame*)data->GetCellFrame();
       break;
     }
   }
   NS_ASSERTION(frame, "cellmap is lying");
   bool isRTL = (NS_STYLE_DIRECTION_RTL ==
                   table->GetStyleVisibility()->mDirection);
   
   nsIFrame* closestFromLeft = nullptr;
   nsIFrame* closestFromRight = nullptr;
   int32_t n = numCells;
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
                                           int32_t    aLineNumber)
{
  NS_ENSURE_ARG_POINTER(aFrame);
  aFrame = aFrame->GetNextSibling();
  return NS_OK;
}



static void
DestroyFrameCursorData(void* aPropertyValue)
{
  delete static_cast<nsTableRowGroupFrame::FrameCursorData*>(aPropertyValue);
}

NS_DECLARE_FRAME_PROPERTY(RowCursorProperty, DestroyFrameCursorData)

void
nsTableRowGroupFrame::ClearRowCursor()
{
  if (!(GetStateBits() & NS_ROWGROUP_HAS_ROW_CURSOR))
    return;

  RemoveStateBits(NS_ROWGROUP_HAS_ROW_CURSOR);
  Properties().Delete(RowCursorProperty());
}

nsTableRowGroupFrame::FrameCursorData*
nsTableRowGroupFrame::SetupRowCursor()
{
  if (GetStateBits() & NS_ROWGROUP_HAS_ROW_CURSOR) {
    
    return nullptr;
  }

  nsIFrame* f = mFrames.FirstChild();
  int32_t count;
  for (count = 0; f && count < MIN_ROWS_NEEDING_CURSOR; ++count) {
    f = f->GetNextSibling();
  }
  if (!f) {
    
    return nullptr;
  }

  FrameCursorData* data = new FrameCursorData();
  if (!data)
    return nullptr;
  Properties().Set(RowCursorProperty(), data);
  AddStateBits(NS_ROWGROUP_HAS_ROW_CURSOR);
  return data;
}

nsIFrame*
nsTableRowGroupFrame::GetFirstRowContaining(nscoord aY, nscoord* aOverflowAbove)
{
  if (!(GetStateBits() & NS_ROWGROUP_HAS_ROW_CURSOR))
    return nullptr;

  FrameCursorData* property = static_cast<FrameCursorData*>
    (Properties().Get(RowCursorProperty()));
  uint32_t cursorIndex = property->mCursorIndex;
  uint32_t frameCount = property->mFrames.Length();
  if (cursorIndex >= frameCount)
    return nullptr;
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

bool
nsTableRowGroupFrame::FrameCursorData::AppendFrame(nsIFrame* aFrame)
{
  nsRect overflowRect = aFrame->GetVisualOverflowRect();
  if (overflowRect.IsEmpty())
    return true;
  nscoord overflowAbove = -overflowRect.y;
  nscoord overflowBelow = overflowRect.YMost() - aFrame->GetSize().height;
  mOverflowAbove = std::max(mOverflowAbove, overflowAbove);
  mOverflowBelow = std::max(mOverflowBelow, overflowBelow);
  return mFrames.AppendElement(aFrame) != nullptr;
}
  
void 
nsTableRowGroupFrame::InvalidateFrame(uint32_t aDisplayItemKey)
{
  nsIFrame::InvalidateFrame(aDisplayItemKey);
  GetParent()->InvalidateFrameWithRect(GetVisualOverflowRect() + GetPosition(), aDisplayItemKey);
}

void 
nsTableRowGroupFrame::InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey)
{
  nsIFrame::InvalidateFrameWithRect(aRect, aDisplayItemKey);
  
  
  
  GetParent()->InvalidateFrameWithRect(aRect + GetPosition(), aDisplayItemKey);
}
