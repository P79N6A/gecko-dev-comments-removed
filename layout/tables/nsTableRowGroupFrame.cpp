



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
using namespace mozilla::layout;

nsTableRowGroupFrame::nsTableRowGroupFrame(nsStyleContext* aContext):
  nsContainerFrame(aContext)
{
  SetRepeatable(false);
}

nsTableRowGroupFrame::~nsTableRowGroupFrame()
{
}

void
nsTableRowGroupFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  if (GetStateBits() & NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN) {
    nsTableFrame::UnregisterPositionedTablePart(this, aDestructRoot);
  }

  nsContainerFrame::DestroyFrom(aDestructRoot);
}

NS_QUERYFRAME_HEAD(nsTableRowGroupFrame)
  NS_QUERYFRAME_ENTRY(nsTableRowGroupFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

int32_t
nsTableRowGroupFrame::GetRowCount()
{
#ifdef DEBUG
  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    NS_ASSERTION(e.get()->StyleDisplay()->mDisplay ==
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
    return GetTableFrame()->GetStartRowIndex(this);
  }

  return result;
}

void  nsTableRowGroupFrame::AdjustRowIndices(int32_t aRowIndex,
                                             int32_t anAdjustment)
{
  nsIFrame* rowFrame = GetFirstPrincipalChild();
  for ( ; rowFrame; rowFrame = rowFrame->GetNextSibling()) {
    if (NS_STYLE_DISPLAY_TABLE_ROW==rowFrame->StyleDisplay()->mDisplay) {
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
                     nsRenderingContext* aCtx) override;

  NS_DISPLAY_DECL_NAME("TableRowGroupBackground", TYPE_TABLE_ROW_GROUP_BACKGROUND)
};

void
nsDisplayTableRowGroupBackground::Paint(nsDisplayListBuilder* aBuilder,
                                        nsRenderingContext* aCtx)
{
  auto rgFrame = static_cast<nsTableRowGroupFrame*>(mFrame);
  TableBackgroundPainter painter(rgFrame->GetTableFrame(),
                                 TableBackgroundPainter::eOrigin_TableRowGroup,
                                 mFrame->PresContext(), *aCtx,
                                 mVisibleRect, ToReferenceFrame(),
                                 aBuilder->GetBackgroundPaintFlags());

  DrawResult result = painter.PaintRowGroup(rgFrame);
  nsDisplayTableItemGeometry::UpdateDrawResult(this, result);
}


static void
DisplayRows(nsDisplayListBuilder* aBuilder, nsFrame* aFrame,
            const nsRect& aDirtyRect, const nsDisplayListSet& aLists)
{
  nscoord overflowAbove;
  nsTableRowGroupFrame* f = static_cast<nsTableRowGroupFrame*>(aFrame);
  
  
  
  
  
  
  
  
  nsIFrame* kid = aBuilder->ShouldDescendIntoFrame(f) ?
    nullptr : f->GetFirstRowContaining(aDirtyRect.y, &overflowAbove);

  if (kid) {
    
    while (kid) {
      if (kid->GetRect().y - overflowAbove >= aDirtyRect.YMost() &&
          kid->GetNormalRect().y - overflowAbove >= aDirtyRect.YMost())
        break;
      f->BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
      kid = kid->GetNextSibling();
    }
    return;
  }

  
  nsTableRowGroupFrame::FrameCursorData* cursor = f->SetupRowCursor();
  kid = f->GetFirstPrincipalChild();
  while (kid) {
    f->BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);

    if (cursor) {
      if (!cursor->AppendFrame(kid)) {
        f->ClearRowCursor();
        return;
      }
    }

    kid = kid->GetNextSibling();
  }
  if (cursor) {
    cursor->FinishBuildingCursor();
  }
}

void
nsTableRowGroupFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                       const nsRect&           aDirtyRect,
                                       const nsDisplayListSet& aLists)
{
  nsDisplayTableItem* item = nullptr;
  if (IsVisibleInSelection(aBuilder)) {
    bool isRoot = aBuilder->IsAtRootOfPseudoStackingContext();
    if (isRoot) {
      
      
      
      item = new (aBuilder) nsDisplayTableRowGroupBackground(aBuilder, this);
      aLists.BorderBackground()->AppendNewToTop(item);
    }
  }
  nsTableFrame::DisplayGenericTablePart(aBuilder, this, aDirtyRect,
                                        aLists, item, DisplayRows);
}

nsIFrame::LogicalSides
nsTableRowGroupFrame::GetLogicalSkipSides(const nsHTMLReflowState* aReflowState) const
{
  if (MOZ_UNLIKELY(StyleBorder()->mBoxDecorationBreak ==
                     NS_STYLE_BOX_DECORATION_BREAK_CLONE)) {
    return LogicalSides();
  }

  LogicalSides skip;
  if (nullptr != GetPrevInFlow()) {
    skip |= eLogicalSideBitsBStart;
  }
  if (nullptr != GetNextInFlow()) {
    skip |= eLogicalSideBitsBEnd;
  }
  return skip;
}



void
nsTableRowGroupFrame::PlaceChild(nsPresContext*         aPresContext,
                                 nsRowGroupReflowState& aReflowState,
                                 nsIFrame*              aKidFrame,
                                 nsPoint                aKidPosition,
                                 nsHTMLReflowMetrics&   aDesiredSize,
                                 const nsRect&          aOriginalKidRect,
                                 const nsRect&          aOriginalKidVisualOverflow)
{
  bool isFirstReflow =
    (aKidFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;

  
  FinishReflowChild(aKidFrame, aPresContext, aDesiredSize, nullptr,
                    aKidPosition.x, aKidPosition.y, 0);

  nsTableFrame::InvalidateTableFrame(aKidFrame, aOriginalKidRect,
                                     aOriginalKidVisualOverflow, isFirstReflow);

  
  aReflowState.y += aDesiredSize.Height();

  
  if (NS_UNCONSTRAINEDSIZE != aReflowState.availSize.height) {
    aReflowState.availSize.height -= aDesiredSize.Height();
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

void
nsTableRowGroupFrame::ReflowChildren(nsPresContext*         aPresContext,
                                     nsHTMLReflowMetrics&   aDesiredSize,
                                     nsRowGroupReflowState& aReflowState,
                                     nsReflowStatus&        aStatus,
                                     bool*                aPageBreakBeforeEnd)
{
  if (aPageBreakBeforeEnd)
    *aPageBreakBeforeEnd = false;

  nsTableFrame* tableFrame = GetTableFrame();
  const bool borderCollapse = tableFrame->IsBorderCollapse();

  
  
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
    nscoord cellSpacingY = tableFrame->GetRowSpacing(rowFrame->GetRowIndex());
    haveRow = true;

    
    if (reflowAllKids ||
        NS_SUBTREE_DIRTY(kidFrame) ||
        (aReflowState.reflowState.mFlags.mSpecialHeightReflow &&
         (isPaginated || (kidFrame->GetStateBits() &
                          NS_FRAME_CONTAINS_RELATIVE_HEIGHT)))) {
      nsRect oldKidRect = kidFrame->GetRect();
      nsRect oldKidVisualOverflow = kidFrame->GetVisualOverflowRect();

      
      
      nsHTMLReflowMetrics desiredSize(aReflowState.reflowState,
                                      aDesiredSize.mFlags);
      desiredSize.ClearSize();

      
      
      
      WritingMode wm = kidFrame->GetWritingMode();
      LogicalSize kidAvailSize(wm, aReflowState.availSize);
      kidAvailSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
      nsHTMLReflowState kidReflowState(aPresContext, aReflowState.reflowState,
                                       kidFrame, kidAvailSize,
                                       -1, -1,
                                       nsHTMLReflowState::CALLER_WILL_INIT);
      InitChildReflowState(*aPresContext, borderCollapse, kidReflowState);

      
      if (aReflowState.reflowState.IsHResize()) {
        kidReflowState.SetHResize(true);
      }

      NS_ASSERTION(kidFrame == mFrames.FirstChild() || prevKidFrame,
                   "If we're not on the first frame, we should have a "
                   "previous sibling...");
      
      if (prevKidFrame && prevKidFrame->GetNormalRect().YMost() > 0) {
        kidReflowState.mFlags.mIsTopOfPage = false;
      }

      ReflowChild(kidFrame, aPresContext, desiredSize, kidReflowState,
                  0, aReflowState.y, 0, aStatus);
      nsPoint kidPosition(0, aReflowState.y);
      kidReflowState.ApplyRelativePositioning(&kidPosition);

      
      PlaceChild(aPresContext, aReflowState, kidFrame, kidPosition,
                 desiredSize, oldKidRect, oldKidVisualOverflow);
      aReflowState.y += cellSpacingY;

      if (!reflowAllKids) {
        if (IsSimpleRowFrame(aReflowState.tableFrame, kidFrame)) {
          
          rowFrame->DidResize();
          
          const nsStylePosition *stylePos = StylePosition();
          nsStyleUnit unit = stylePos->mHeight.GetUnit();
          if (aReflowState.tableFrame->IsAutoHeight() &&
              unit != eStyleUnit_Coord) {
            
            
            InvalidateFrame();
          }
          else if (oldKidRect.height != desiredSize.Height())
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
    aReflowState.y -= tableFrame->GetRowSpacing(GetStartRowIndex() +
                                                GetRowCount());

  
  aDesiredSize.Width() = aReflowState.reflowState.AvailableWidth();
  aDesiredSize.Height() = aReflowState.y;

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
  nsTableFrame* tableFrame = GetTableFrame();
  const bool isPaginated = aPresContext->IsPaginated();

  int32_t numEffCols = tableFrame->GetEffectiveColCount();

  int32_t startRowIndex = GetStartRowIndex();
  
  nsTableRowFrame* startRowFrame = GetFirstRow();

  if (!startRowFrame) return;

  
  nscoord startRowGroupHeight = startRowFrame->GetNormalPosition().y;

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
          nscoord cellSpacingY = tableFrame->GetRowSpacing(startRowIndex + rowIndex);
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
            nsSize cellDesSize =
              cellFrame->GetDesiredSize().GetPhysicalSize(cellFrame->GetWritingMode());
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
  nscoord rowGroupHeight = startRowGroupHeight + heightOfRows +
                           tableFrame->GetRowSpacing(0, numRows-1);
  
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
    nscoord deltaY = yOrigin - rowFrame->GetNormalPosition().y;

    nscoord rowHeight = (rowInfo[rowIndex].height > 0) ? rowInfo[rowIndex].height : 0;

    if (deltaY != 0 || (rowHeight != rowBounds.height)) {
      
      if (deltaY != 0) {
        rowFrame->InvalidateFrameSubtree();
      }

      rowFrame->MovePositionBy(nsPoint(0, deltaY));
      rowFrame->SetSize(nsSize(rowBounds.width, rowHeight));

      nsTableFrame::InvalidateTableFrame(rowFrame, rowBounds, rowVisualOverflow,
                                         false);

      if (deltaY != 0) {
        nsTableFrame::RePositionViews(rowFrame);
        
      }
    }
    yOrigin += rowHeight + tableFrame->GetRowSpacing(startRowIndex + rowIndex);
  }

  if (isPaginated && styleHeightAllocation) {
    
    CacheRowHeightsForPrinting(aPresContext, GetFirstRow());
  }

  DidResizeRows(aDesiredSize);

  aDesiredSize.Height() = rowGroupHeight; 
}

nscoord
nsTableRowGroupFrame::CollapseRowGroupIfNecessary(nscoord aYTotalOffset,
                                                  nscoord aWidth)
{
  nsTableFrame* tableFrame = GetTableFrame();
  const nsStyleVisibility* groupVis = StyleVisibility();
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
    
    groupRect.height += tableFrame->GetRowSpacing(GetStartRowIndex() +
                                                  GetRowCount());
  }

  groupRect.y -= aYTotalOffset;
  groupRect.width = aWidth;

  if (aYTotalOffset != 0) {
    InvalidateFrameSubtree();
  }

  SetRect(groupRect);
  overflow.UnionAllWith(nsRect(0, 0, groupRect.width, groupRect.height));
  FinishAndStoreOverflow(overflow, groupRect.Size());
  nsTableFrame::RePositionViews(this);
  nsTableFrame::InvalidateTableFrame(this, oldGroupRect, oldGroupVisualOverflow,
                                     false);

  return yGroupOffset;
}


void
nsTableRowGroupFrame::SlideChild(nsRowGroupReflowState& aReflowState,
                                 nsIFrame*              aKidFrame)
{
  
  nsPoint oldPosition = aKidFrame->GetNormalPosition();
  nsPoint newPosition = oldPosition;
  newPosition.y = aReflowState.y;
  if (oldPosition.y != newPosition.y) {
    aKidFrame->InvalidateFrameSubtree();
    aReflowState.reflowState.ApplyRelativePositioning(&newPosition);
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
  
  *aContRowFrame = aPresContext.PresShell()->FrameConstructor()->
    CreateContinuingFrame(&aPresContext, &aRowFrame, this);

  
  mFrames.InsertFrame(nullptr, &aRowFrame, *aContRowFrame);

  
  PushChildren(*aContRowFrame, &aRowFrame);
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
    nsPoint rowPos = row->GetNormalPosition();
    
    for (nsTableCellFrame* cell = row->GetFirstCell(); cell; cell = cell->GetNextCell()) {
      int32_t rowSpan = aTable.GetEffectiveRowSpan(rowIndex, *cell);
      
      
      if ((rowSpan > 1) && (rowIndex + rowSpan > lastRowIndex)) {
        haveRowSpan = true;
        nsReflowStatus status;
        
        
        nscoord cellAvailHeight = aSpanningRowBottom - rowPos.y;
        NS_ASSERTION(cellAvailHeight >= 0, "No space for cell?");
        bool isTopOfPage = (row == &aFirstRow) && aFirstRowIsTopOfPage;

        nsRect rowRect = row->GetNormalRect();
        nsSize rowAvailSize(aReflowState.AvailableWidth(),
                            std::max(aReflowState.AvailableHeight() - rowRect.y,
                                   0));
        
        
        rowAvailSize.height = std::min(rowAvailSize.height, rowRect.height);
        nsHTMLReflowState rowReflowState(&aPresContext, aReflowState, row,
                                         LogicalSize(row->GetWritingMode(),
                                                     rowAvailSize),
                                         -1, -1,
                                         nsHTMLReflowState::CALLER_WILL_INIT);
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
              
              
              nsTableCellFrame* contCell = static_cast<nsTableCellFrame*>(
                aPresContext.PresShell()->FrameConstructor()->
                  CreateContinuingFrame(&aPresContext, cell, &aLastRow));
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
    aDesiredHeight = aLastRow.GetNormalRect().YMost();
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

  AutoFrameListPtr overflows(aPresContext, StealOverflowFrames());
  if (!rowBefore || !overflows || overflows->IsEmpty() ||
      overflows->FirstChild() != aRow) {
    NS_ERROR("invalid continued row");
    return;
  }

  
  
  overflows->DestroyFrame(aRow);

  
  if (!overflows->IsEmpty()) {
    mFrames.InsertFrames(nullptr, rowBefore, *overflows);
  }
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

  nsTableRowFrame* prevRowFrame = nullptr;
  aDesiredSize.Height() = 0;

  nscoord availWidth  = aReflowState.AvailableWidth();
  nscoord availHeight = aReflowState.AvailableHeight();

  const bool borderCollapse = aTableFrame->IsBorderCollapse();

  
  nscoord pageHeight = aPresContext->GetPageSize().height;
  NS_ASSERTION(pageHeight != NS_UNCONSTRAINEDSIZE,
               "The table shouldn't be split when there should be space");

  bool isTopOfPage = aReflowState.mFlags.mIsTopOfPage;
  nsTableRowFrame* firstRowThisPage = GetFirstRow();

  
  
  aTableFrame->SetGeometryDirty();

  
  
  for (nsTableRowFrame* rowFrame = firstRowThisPage; rowFrame; rowFrame = rowFrame->GetNextRow()) {
    bool rowIsOnPage = true;
    nscoord cellSpacingY = aTableFrame->GetRowSpacing(rowFrame->GetRowIndex());
    nsRect rowRect = rowFrame->GetNormalRect();
    
    if (rowRect.YMost() > availHeight) {
      nsTableRowFrame* contRow = nullptr;
      
      
      
      if (!prevRowFrame || (availHeight - aDesiredSize.Height() > pageHeight / 20)) {
        nsSize availSize(availWidth, std::max(availHeight - rowRect.y, 0));
        
        availSize.height = std::min(availSize.height, rowRect.height);

        nsHTMLReflowState rowReflowState(aPresContext, aReflowState, rowFrame,
                                         LogicalSize(rowFrame->GetWritingMode(),
                                                     availSize),
                                         -1, -1,
                                         nsHTMLReflowState::CALLER_WILL_INIT);

        InitChildReflowState(*aPresContext, borderCollapse, rowReflowState);
        rowReflowState.mFlags.mIsTopOfPage = isTopOfPage; 
        nsHTMLReflowMetrics rowMetrics(aReflowState);

        
        nsRect oldRowRect = rowFrame->GetRect();
        nsRect oldRowVisualOverflow = rowFrame->GetVisualOverflowRect();

        
        
        ReflowChild(rowFrame, aPresContext, rowMetrics, rowReflowState,
                    0, 0, NS_FRAME_NO_MOVE_FRAME, aStatus);
        rowFrame->SetSize(nsSize(rowMetrics.Width(), rowMetrics.Height()));
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
          
          if ((rowMetrics.Height() <= rowReflowState.AvailableHeight()) || isTopOfPage) {
            
            
            NS_ASSERTION(rowMetrics.Height() <= rowReflowState.AvailableHeight(),
                         "data loss - incomplete row needed more height than available, on top of page");
            CreateContinuingRowFrame(*aPresContext, *rowFrame, (nsIFrame**)&contRow);
            if (contRow) {
              aDesiredSize.Height() += rowMetrics.Height();
              if (prevRowFrame)
                aDesiredSize.Height() += cellSpacingY;
            }
            else return NS_ERROR_NULL_POINTER;
          }
          else {
            
            rowIsOnPage = false;
          }
        }
        else {
          
          
          
          
          
          if (rowMetrics.Height() > availSize.height ||
              (NS_INLINE_IS_BREAK_BEFORE(aStatus) && !aRowForcedPageBreak)) {
            
            if (isTopOfPage) {
              
              
              nsTableRowFrame* nextRowFrame = rowFrame->GetNextRow();
              if (nextRowFrame) {
                aStatus = NS_FRAME_NOT_COMPLETE;
              }
              aDesiredSize.Height() += rowMetrics.Height();
              if (prevRowFrame)
                aDesiredSize.Height() += cellSpacingY;
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
          spanningRowBottom = prevRowFrame->GetNormalRect().YMost();
          lastRowThisPage = prevRowFrame;
          isTopOfPage = (lastRowThisPage == firstRowThisPage) && aReflowState.mFlags.mIsTopOfPage;
          aStatus = NS_FRAME_NOT_COMPLETE;
        }
        else {
          
          aDesiredSize.Height() = rowRect.YMost();
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
            
            aDesiredSize.Height() = rowRect.YMost();
            aStatus = NS_FRAME_COMPLETE;
            UndoContinuedRow(aPresContext, contRow);
            contRow = nullptr;
          }
        }
        else { 
          
          nsTableRowFrame* rowBefore = ::GetRowBefore(*firstRowThisPage, *firstTruncatedRow);
          nscoord oldSpanningRowBottom = spanningRowBottom;
          spanningRowBottom = rowBefore->GetNormalRect().YMost();

          UndoContinuedRow(aPresContext, contRow);
          contRow = nullptr;
          nsTableRowFrame* oldLastRowThisPage = lastRowThisPage;
          lastRowThisPage = rowBefore;
          aStatus = NS_FRAME_NOT_COMPLETE;

          
          SplitSpanningCells(*aPresContext, aReflowState, *aTableFrame,
                             *firstRowThisPage, *rowBefore, aReflowState.mFlags.mIsTopOfPage,
                             spanningRowBottom, contRow, firstTruncatedRow, aDesiredSize.Height());
          if (firstTruncatedRow) {
            if (aReflowState.mFlags.mIsTopOfPage) {
              
              UndoContinuedRow(aPresContext, contRow);
              contRow = nullptr;
              lastRowThisPage = oldLastRowThisPage;
              spanningRowBottom = oldSpanningRowBottom;
              SplitSpanningCells(*aPresContext, aReflowState, *aTableFrame, *firstRowThisPage,
                                 *lastRowThisPage, aReflowState.mFlags.mIsTopOfPage, spanningRowBottom, contRow,
                                 firstTruncatedRow, aDesiredSize.Height());
              NS_WARNING("data loss in a row spanned cell");
            }
            else {
              
              aDesiredSize.Height() = rowRect.YMost();
              aStatus = NS_FRAME_COMPLETE;
              UndoContinuedRow(aPresContext, contRow);
              contRow = nullptr;
            }
          }
        } 
      } 
      else {
        aDesiredSize.Height() = std::max(aDesiredSize.Height(), yMost);
        if (contRow) {
          aStatus = NS_FRAME_NOT_COMPLETE;
        }
      }
      if (NS_FRAME_IS_NOT_COMPLETE(aStatus) && !contRow) {
        nsTableRowFrame* nextRow = lastRowThisPage->GetNextRow();
        if (nextRow) {
          PushChildren(nextRow, lastRowThisPage);
        }
      }
      break;
    } 
    else {
      aDesiredSize.Height() = rowRect.YMost();
      prevRowFrame = rowFrame;
      
      nsTableRowFrame* nextRow = rowFrame->GetNextRow();
      if (nextRow && nsTableFrame::PageBreakAfter(rowFrame, nextRow)) {
        PushChildren(nextRow, rowFrame);
        aStatus = NS_FRAME_NOT_COMPLETE;
        break;
      }
    }
    
    
    isTopOfPage = isTopOfPage && rowRect.YMost() == 0;
  }
  return NS_OK;
}





void
nsTableRowGroupFrame::Reflow(nsPresContext*           aPresContext,
                             nsHTMLReflowMetrics&     aDesiredSize,
                             const nsHTMLReflowState& aReflowState,
                             nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsTableRowGroupFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  aStatus     = NS_FRAME_COMPLETE;

  
  ClearRowCursor();

  
  nsTableFrame::CheckRequestSpecialHeightReflow(aReflowState);

  nsTableFrame* tableFrame = GetTableFrame();
  nsRowGroupReflowState state(aReflowState, tableFrame);
  const nsStyleVisibility* groupVis = StyleVisibility();
  bool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE == groupVis->mVisible);
  if (collapseGroup) {
    tableFrame->SetNeedToCollapse(true);
  }

  
  MoveOverflowToChildList();

  
  bool splitDueToPageBreak = false;
  ReflowChildren(aPresContext, aDesiredSize, state, aStatus,
                 &splitDueToPageBreak);

  
  
  if (aReflowState.mFlags.mTableIsSplittable &&
      NS_UNCONSTRAINEDSIZE != aReflowState.AvailableHeight() &&
      (NS_FRAME_NOT_COMPLETE == aStatus || splitDueToPageBreak ||
       aDesiredSize.Height() > aReflowState.AvailableHeight())) {
    
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

  
  aDesiredSize.Width() = aReflowState.AvailableWidth();

  aDesiredSize.UnionOverflowAreasWithDesiredBounds();

  
  
  if (!(GetParent()->GetStateBits() & NS_FRAME_FIRST_REFLOW) &&
      nsSize(aDesiredSize.Width(), aDesiredSize.Height()) != mRect.Size()) {
    InvalidateFrame();
  }

  FinishAndStoreOverflow(&aDesiredSize);

  
  
  
  PushDirtyBitToAbsoluteFrames();

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

bool
nsTableRowGroupFrame::UpdateOverflow()
{
  
  
  ClearRowCursor();
  return nsContainerFrame::UpdateOverflow();
}

 void
nsTableRowGroupFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsContainerFrame::DidSetStyleContext(aOldStyleContext);

  if (!aOldStyleContext) 
    return;

  nsTableFrame* tableFrame = GetTableFrame();
  if (tableFrame->IsBorderCollapse() &&
      tableFrame->BCRecalcNeeded(aOldStyleContext, StyleContext())) {
    TableArea damageArea(0, GetStartRowIndex(), tableFrame->GetColCount(),
                         GetRowCount());
    tableFrame->AddBCDamageArea(damageArea);
  }
}

void
nsTableRowGroupFrame::AppendFrames(ChildListID     aListID,
                                   nsFrameList&    aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");

  DrainSelfOverflowList(); 
  ClearRowCursor();

  
  
  nsAutoTArray<nsTableRowFrame*, 8> rows;
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    nsTableRowFrame *rowFrame = do_QueryFrame(e.get());
    NS_ASSERTION(rowFrame, "Unexpected frame; frame constructor screwed up");
    if (rowFrame) {
      NS_ASSERTION(NS_STYLE_DISPLAY_TABLE_ROW ==
                     e.get()->StyleDisplay()->mDisplay,
                   "wrong display type on rowframe");
      rows.AppendElement(rowFrame);
    }
  }

  int32_t rowIndex = GetRowCount();
  
  mFrames.AppendFrames(nullptr, aFrameList);

  if (rows.Length() > 0) {
    nsTableFrame* tableFrame = GetTableFrame();
    tableFrame->AppendRows(this, rowIndex, rows);
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
    tableFrame->SetGeometryDirty();
  }
}

void
nsTableRowGroupFrame::InsertFrames(ChildListID     aListID,
                                   nsIFrame*       aPrevFrame,
                                   nsFrameList&    aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  DrainSelfOverflowList(); 
  ClearRowCursor();

  
  
  nsTableFrame* tableFrame = GetTableFrame();
  nsTArray<nsTableRowFrame*> rows;
  bool gotFirstRow = false;
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    nsTableRowFrame *rowFrame = do_QueryFrame(e.get());
    NS_ASSERTION(rowFrame, "Unexpected frame; frame constructor screwed up");
    if (rowFrame) {
      NS_ASSERTION(NS_STYLE_DISPLAY_TABLE_ROW ==
                     e.get()->StyleDisplay()->mDisplay,
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
}

void
nsTableRowGroupFrame::RemoveFrame(ChildListID     aListID,
                                  nsIFrame*       aOldFrame)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");

  ClearRowCursor();

  
  nsTableRowFrame* rowFrame = do_QueryFrame(aOldFrame);
  if (rowFrame) {
    nsTableFrame* tableFrame = GetTableFrame();
    
    tableFrame->RemoveRows(*rowFrame, 1, true);

    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
    tableFrame->SetGeometryDirty();
  }
  mFrames.DestroyFrame(aOldFrame);
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
  nsTableFrame* tableFrame = GetTableFrame();
  int32_t startRowIndex = GetStartRowIndex();
  if ((aReflowState.ComputedHeight() > 0) && (aReflowState.ComputedHeight() < NS_UNCONSTRAINEDSIZE)) {
    nscoord cellSpacing = tableFrame->GetRowSpacing(startRowIndex,
                                                    std::max(startRowIndex,
                                                             startRowIndex + GetRowCount() - 1));
    result = aReflowState.ComputedHeight() - cellSpacing;
  }
  else {
    const nsHTMLReflowState* parentRS = aReflowState.parentReflowState;
    if (parentRS && (tableFrame != parentRS->frame)) {
      parentRS = parentRS->parentReflowState;
    }
    if (parentRS && (tableFrame == parentRS->frame) &&
        (parentRS->ComputedHeight() > 0) && (parentRS->ComputedHeight() < NS_UNCONSTRAINEDSIZE)) {
      nscoord cellSpacing = tableFrame->GetRowSpacing(-1, tableFrame->GetRowCount());
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
  return firstChild->StyleDisplay()->mBreakBefore;
}


bool
nsTableRowGroupFrame::HasInternalBreakAfter() const
{
  nsIFrame* lastChild = mFrames.LastChild();
  if (!lastChild)
    return false;
  return lastChild->StyleDisplay()->mBreakAfter;
}


nsTableRowGroupFrame*
NS_NewTableRowGroupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTableRowGroupFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTableRowGroupFrame)

#ifdef DEBUG_FRAME_DUMP
nsresult
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
  return (NS_STYLE_DIRECTION_RTL ==
          GetTableFrame()->StyleVisibility()->mDirection);
}

NS_IMETHODIMP
nsTableRowGroupFrame::GetLine(int32_t    aLineNumber,
                              nsIFrame** aFirstFrameOnLine,
                              int32_t*   aNumFramesOnLine,
                              nsRect&    aLineBounds)
{
  NS_ENSURE_ARG_POINTER(aFirstFrameOnLine);
  NS_ENSURE_ARG_POINTER(aNumFramesOnLine);

  nsTableFrame* table = GetTableFrame();
  nsTableCellMap* cellMap = table->GetCellMap();

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

NS_IMETHODIMP
nsTableRowGroupFrame::FindFrameAt(int32_t    aLineNumber,
                                  nsPoint    aPos,
                                  nsIFrame** aFrameFound,
                                  bool*    aPosIsBeforeFirstFrame,
                                  bool*    aPosIsAfterLastFrame)
{
  nsTableFrame* table = GetTableFrame();
  nsTableCellMap* cellMap = table->GetCellMap();

  WritingMode wm = table->GetWritingMode();
  nscoord cw = table->GetRect().width;
  LogicalPoint pos(wm, aPos, cw);

  *aFrameFound = nullptr;
  *aPosIsBeforeFirstFrame = true;
  *aPosIsAfterLastFrame = false;

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
                  table->StyleVisibility()->mDirection);

  nsIFrame* closestFromStart = nullptr;
  nsIFrame* closestFromEnd = nullptr;
  int32_t n = numCells;
  nsIFrame* firstFrame = frame;
  while (n--) {
    LogicalRect rect = frame->GetLogicalRect(wm, cw);
    if (rect.ISize(wm) > 0) {
      
      if (rect.IStart(wm) <= pos.I(wm) && rect.IEnd(wm) > pos.I(wm)) {
        closestFromStart = closestFromEnd = frame;
        break;
      }
      if (rect.IStart(wm) < pos.I(wm)) {
        if (!closestFromStart ||
            rect.IEnd(wm) > closestFromStart->GetLogicalRect(wm, cw).IEnd(wm))
          closestFromStart = frame;
      }
      else {
        if (!closestFromEnd ||
            rect.IStart(wm) < closestFromEnd->GetLogicalRect(wm, cw).IStart(wm))
          closestFromEnd = frame;
      }
    }
    frame = frame->GetNextSibling();
  }
  if (!closestFromStart && !closestFromEnd) {
    
    closestFromStart = closestFromEnd = firstFrame;
  }
  *aPosIsBeforeFirstFrame = isRTL ? !closestFromEnd : !closestFromStart;
  *aPosIsAfterLastFrame =   isRTL ? !closestFromStart : !closestFromEnd;
  if (closestFromStart == closestFromEnd) {
    *aFrameFound = closestFromStart;
  }
  else if (!closestFromStart) {
    *aFrameFound = closestFromEnd;
  }
  else if (!closestFromEnd) {
    *aFrameFound = closestFromStart;
  }
  else { 
    nscoord delta = closestFromEnd->GetLogicalRect(wm, cw).IStart(wm) -
                    closestFromStart->GetLogicalRect(wm, cw).IEnd(wm);
    if (pos.I(wm) < closestFromStart->GetLogicalRect(wm, cw).IEnd(wm) + delta/2) {
      *aFrameFound = closestFromStart;
    } else {
      *aFrameFound = closestFromEnd;
    }
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



NS_DECLARE_FRAME_PROPERTY(RowCursorProperty,
                          DeleteValue<nsTableRowGroupFrame::FrameCursorData>)

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
         cursorFrame->GetNormalRect().YMost() + property->mOverflowBelow > aY) {
    --cursorIndex;
    cursorFrame = property->mFrames[cursorIndex];
  }
  while (cursorIndex + 1 < frameCount &&
         cursorFrame->GetNormalRect().YMost() + property->mOverflowBelow <= aY) {
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
  
  
  
  
  
  
  
  nsRect positionedOverflowRect = aFrame->GetVisualOverflowRect();
  nsPoint positionedToNormal = aFrame->GetNormalPosition() - aFrame->GetPosition();
  nsRect normalOverflowRect = positionedOverflowRect + positionedToNormal;

  nsRect overflowRect = positionedOverflowRect.Union(normalOverflowRect);
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
