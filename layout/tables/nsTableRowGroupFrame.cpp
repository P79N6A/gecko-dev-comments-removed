



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
                                 WritingMode            aWM,
                                 const LogicalPoint&    aKidPosition,
                                 nscoord                aContainerWidth,
                                 nsHTMLReflowMetrics&   aDesiredSize,
                                 const nsRect&          aOriginalKidRect,
                                 const nsRect&          aOriginalKidVisualOverflow)
{
  bool isFirstReflow =
    (aKidFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;

  
  FinishReflowChild(aKidFrame, aPresContext, aDesiredSize, nullptr,
                    aWM, aKidPosition, aContainerWidth, 0);

  nsTableFrame::InvalidateTableFrame(aKidFrame, aOriginalKidRect,
                                     aOriginalKidVisualOverflow, isFirstReflow);

  
  aReflowState.bCoord += aDesiredSize.BSize(aWM);

  
  if (NS_UNCONSTRAINEDSIZE != aReflowState.availSize.BSize(aWM)) {
    aReflowState.availSize.BSize(aWM) -= aDesiredSize.BSize(aWM);
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
      WritingMode wm = GetWritingMode();
      LogicalMargin border = rowFrame->GetBCBorderWidth(wm);
      collapseBorder = border.GetPhysicalMargin(wm);
      pCollapseBorder = &collapseBorder;
    }
  }
  aReflowState.Init(&aPresContext, nullptr, pCollapseBorder, &padding);
}

static void
CacheRowBSizesForPrinting(nsPresContext*   aPresContext,
                          nsTableRowFrame* aFirstRow,
                          WritingMode      aWM)
{
  for (nsTableRowFrame* row = aFirstRow; row; row = row->GetNextRow()) {
    if (!row->GetPrevInFlow()) {
      row->SetHasUnpaginatedBSize(true);
      row->SetUnpaginatedBSize(aPresContext, row->BSize(aWM));
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
  if (aPageBreakBeforeEnd) {
    *aPageBreakBeforeEnd = false;
  }

  WritingMode wm = aReflowState.reflowState.GetWritingMode();
  nsTableFrame* tableFrame = GetTableFrame();
  const bool borderCollapse = tableFrame->IsBorderCollapse();

  
  
  
  bool isPaginated = aPresContext->IsPaginated() &&
                     NS_UNCONSTRAINEDSIZE != aReflowState.availSize.BSize(wm);

  bool haveRow = false;
  bool reflowAllKids = aReflowState.reflowState.ShouldReflowAllKids() ||
                         tableFrame->IsGeometryDirty();
  bool needToCalcRowBSizes = reflowAllKids;

  nscoord containerWidth = aReflowState.reflowState.ComputedWidth();
  if (containerWidth == NS_UNCONSTRAINEDSIZE) {
    containerWidth = 0; 
                        
  } else {
    containerWidth +=
      aReflowState.reflowState.ComputedPhysicalBorderPadding().LeftRight();
  }

  nsIFrame *prevKidFrame = nullptr;
  for (nsIFrame* kidFrame = mFrames.FirstChild(); kidFrame;
       prevKidFrame = kidFrame, kidFrame = kidFrame->GetNextSibling()) {
    nsTableRowFrame *rowFrame = do_QueryFrame(kidFrame);
    if (!rowFrame) {
      
      NS_NOTREACHED("yikes, a non-row child");
      continue;
    }
    nscoord cellSpacingB = tableFrame->GetRowSpacing(rowFrame->GetRowIndex());
    haveRow = true;

    
    if (reflowAllKids ||
        NS_SUBTREE_DIRTY(kidFrame) ||
        (aReflowState.reflowState.mFlags.mSpecialBSizeReflow &&
         (isPaginated || (kidFrame->GetStateBits() &
                          NS_FRAME_CONTAINS_RELATIVE_BSIZE)))) {
      LogicalRect oldKidRect = kidFrame->GetLogicalRect(wm, containerWidth);
      nsRect oldKidVisualOverflow = kidFrame->GetVisualOverflowRect();

      
      
      nsHTMLReflowMetrics desiredSize(aReflowState.reflowState,
                                      aDesiredSize.mFlags);
      desiredSize.ClearSize();

      
      
      
      LogicalSize kidAvailSize = aReflowState.availSize;
      kidAvailSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
      nsHTMLReflowState kidReflowState(aPresContext, aReflowState.reflowState,
                                       kidFrame, kidAvailSize,
                                       nullptr,
                                       nsHTMLReflowState::CALLER_WILL_INIT);
      InitChildReflowState(*aPresContext, borderCollapse, kidReflowState);

      
      if (aReflowState.reflowState.IsIResize()) {
        kidReflowState.SetIResize(true);
      }

      NS_ASSERTION(kidFrame == mFrames.FirstChild() || prevKidFrame,
                   "If we're not on the first frame, we should have a "
                   "previous sibling...");
      
      if (prevKidFrame && prevKidFrame->GetNormalRect().YMost() > 0) {
        kidReflowState.mFlags.mIsTopOfPage = false;
      }

      LogicalPoint kidPosition(wm, 0, aReflowState.bCoord);
      ReflowChild(kidFrame, aPresContext, desiredSize, kidReflowState,
                  wm, kidPosition, containerWidth, 0, aStatus);
      kidReflowState.ApplyRelativePositioning(&kidPosition, containerWidth);

      
      PlaceChild(aPresContext, aReflowState, kidFrame,
                 wm, kidPosition, containerWidth,
                 desiredSize, oldKidRect.GetPhysicalRect(wm, containerWidth),
                 oldKidVisualOverflow);
      aReflowState.bCoord += cellSpacingB;

      if (!reflowAllKids) {
        if (IsSimpleRowFrame(aReflowState.tableFrame, kidFrame)) {
          
          rowFrame->DidResize();
          
          const nsStylePosition *stylePos = StylePosition();
          nsStyleUnit unit = stylePos->BSize(wm).GetUnit();
          if (aReflowState.tableFrame->IsAutoBSize(wm) &&
              unit != eStyleUnit_Coord) {
            
            
            InvalidateFrame();
          } else if (oldKidRect.BSize(wm) != desiredSize.BSize(wm)) {
            needToCalcRowBSizes = true;
          }
        } else {
          needToCalcRowBSizes = true;
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

      
      nscoord bSize = kidFrame->BSize(wm) + cellSpacingB;
      aReflowState.bCoord += bSize;

      if (NS_UNCONSTRAINEDSIZE != aReflowState.availSize.BSize(wm)) {
        aReflowState.availSize.BSize(wm) -= bSize;
      }
    }
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, kidFrame);
  }

  if (haveRow) {
    aReflowState.bCoord -= tableFrame->GetRowSpacing(GetStartRowIndex() +
                                                     GetRowCount());
  }

  
  aDesiredSize.ISize(wm) = aReflowState.reflowState.AvailableISize();
  aDesiredSize.BSize(wm) = aReflowState.bCoord;

  if (aReflowState.reflowState.mFlags.mSpecialBSizeReflow) {
    DidResizeRows(aDesiredSize);
    if (isPaginated) {
      CacheRowBSizesForPrinting(aPresContext, GetFirstRow(), wm);
    }
  }
  else if (needToCalcRowBSizes) {
    CalculateRowBSizes(aPresContext, aDesiredSize, aReflowState.reflowState);
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
  RowInfo() { bSize = pctBSize = hasStyleBSize = hasPctBSize = isSpecial = 0; }
  unsigned bSize;       
  unsigned pctBSize:29; 
  unsigned hasStyleBSize:1;
  unsigned hasPctBSize:1;
  unsigned isSpecial:1; 
                        
};

static void
UpdateBSizes(RowInfo& aRowInfo,
             nscoord  aAdditionalBSize,
             nscoord& aTotal,
             nscoord& aUnconstrainedTotal)
{
  aRowInfo.bSize += aAdditionalBSize;
  aTotal         += aAdditionalBSize;
  if (!aRowInfo.hasStyleBSize) {
    aUnconstrainedTotal += aAdditionalBSize;
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
nsTableRowGroupFrame::CalculateRowBSizes(nsPresContext*           aPresContext,
                                          nsHTMLReflowMetrics&     aDesiredSize,
                                          const nsHTMLReflowState& aReflowState)
{
  nsTableFrame* tableFrame = GetTableFrame();
  const bool isPaginated = aPresContext->IsPaginated();

  int32_t numEffCols = tableFrame->GetEffectiveColCount();

  int32_t startRowIndex = GetStartRowIndex();
  
  nsTableRowFrame* startRowFrame = GetFirstRow();

  if (!startRowFrame) {
    return;
  }

  
  
  WritingMode wm = aReflowState.GetWritingMode();
  nscoord containerWidth = 0; 
                              
  nscoord startRowGroupBSize =
    startRowFrame->GetLogicalNormalPosition(wm, containerWidth).B(wm);

  int32_t numRows = GetRowCount() - (startRowFrame->GetRowIndex() - GetStartRowIndex());
  
  if (numRows <= 0)
    return;

  nsTArray<RowInfo> rowInfo;
  if (!rowInfo.AppendElements(numRows)) {
    return;
  }

  bool    hasRowSpanningCell = false;
  nscoord bSizeOfRows = 0;
  nscoord bSizeOfUnStyledRows = 0;
  
  
  
  nscoord pctBSizeBasis = GetBSizeBasis(aReflowState);
  int32_t rowIndex; 
  nsTableRowFrame* rowFrame;
  for (rowFrame = startRowFrame, rowIndex = 0; rowFrame; rowFrame = rowFrame->GetNextRow(), rowIndex++) {
    nscoord nonPctBSize = rowFrame->GetContentBSize();
    if (isPaginated) {
      nonPctBSize = std::max(nonPctBSize, rowFrame->BSize(wm));
    }
    if (!rowFrame->GetPrevInFlow()) {
      if (rowFrame->HasPctBSize()) {
        rowInfo[rowIndex].hasPctBSize = true;
        rowInfo[rowIndex].pctBSize = rowFrame->GetBSize(pctBSizeBasis);
      }
      rowInfo[rowIndex].hasStyleBSize = rowFrame->HasStyleBSize();
      nonPctBSize = std::max(nonPctBSize, rowFrame->GetFixedBSize());
    }
    UpdateBSizes(rowInfo[rowIndex], nonPctBSize, bSizeOfRows, bSizeOfUnStyledRows);

    if (!rowInfo[rowIndex].hasStyleBSize) {
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
          nscoord cellSpacingB = tableFrame->GetRowSpacing(startRowIndex + rowIndex);
          int32_t rowSpan = tableFrame->GetEffectiveRowSpan(rowIndex + startRowIndex, *cellFrame);
          if ((rowIndex + rowSpan) > numRows) {
            
            rowSpan = numRows - rowIndex;
          }
          if (rowSpan > 1) { 
            nscoord bsizeOfRowsSpanned = 0;
            nscoord bsizeOfUnStyledRowsSpanned = 0;
            nscoord numSpecialRowsSpanned = 0;
            nscoord cellSpacingTotal = 0;
            int32_t spanX;
            for (spanX = 0; spanX < rowSpan; spanX++) {
              bsizeOfRowsSpanned += rowInfo[rowIndex + spanX].bSize;
              if (!rowInfo[rowIndex + spanX].hasStyleBSize) {
                bsizeOfUnStyledRowsSpanned += rowInfo[rowIndex + spanX].bSize;
              }
              if (0 != spanX) {
                cellSpacingTotal += cellSpacingB;
              }
              if (rowInfo[rowIndex + spanX].isSpecial) {
                numSpecialRowsSpanned++;
              }
            }
            nscoord bsizeOfAreaSpanned = bsizeOfRowsSpanned + cellSpacingTotal;
            
            LogicalSize cellFrameSize = cellFrame->GetLogicalSize(wm);
            LogicalSize cellDesSize = cellFrame->GetDesiredSize();
            rowFrame->CalculateCellActualBSize(cellFrame, cellDesSize.BSize(wm), wm);
            cellFrameSize.BSize(wm) = cellDesSize.BSize(wm);
            if (cellFrame->HasVerticalAlignBaseline()) {
              
              
              
              cellFrameSize.BSize(wm) += rowFrame->GetMaxCellAscent() -
                                         cellFrame->GetCellBaseline();
            }

            if (bsizeOfAreaSpanned < cellFrameSize.BSize(wm)) {
              
              
              nscoord extra     = cellFrameSize.BSize(wm) - bsizeOfAreaSpanned;
              nscoord extraUsed = 0;
              if (0 == numSpecialRowsSpanned) {
                
                bool haveUnStyledRowsSpanned = (bsizeOfUnStyledRowsSpanned > 0);
                nscoord divisor = (haveUnStyledRowsSpanned)
                                  ? bsizeOfUnStyledRowsSpanned : bsizeOfRowsSpanned;
                if (divisor > 0) {
                  for (spanX = rowSpan - 1; spanX >= 0; spanX--) {
                    if (!haveUnStyledRowsSpanned || !rowInfo[rowIndex + spanX].hasStyleBSize) {
                      
                      float percent = ((float)rowInfo[rowIndex + spanX].bSize) / ((float)divisor);

                      
                      nscoord extraForRow = (0 == spanX) ? extra - extraUsed
                                                         : NSToCoordRound(((float)(extra)) * percent);
                      extraForRow = std::min(extraForRow, extra - extraUsed);
                      
                      UpdateBSizes(rowInfo[rowIndex + spanX], extraForRow, bSizeOfRows, bSizeOfUnStyledRows);
                      extraUsed += extraForRow;
                      if (extraUsed >= extra) {
                        NS_ASSERTION((extraUsed == extra), "invalid row bsize calculation");
                        break;
                      }
                    }
                  }
                }
                else {
                  
                  UpdateBSizes(rowInfo[rowIndex + rowSpan - 1], extra, bSizeOfRows, bSizeOfUnStyledRows);
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
                    
                    UpdateBSizes(rowInfo[rowIndex + spanX], extraForRow, bSizeOfRows, bSizeOfUnStyledRows);
                    extraUsed += extraForRow;
                    if (extraUsed >= extra) {
                      NS_ASSERTION((extraUsed == extra), "invalid row bsize calculation");
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

  
  
  nscoord extra = pctBSizeBasis - bSizeOfRows;
  for (rowFrame = startRowFrame, rowIndex = 0; rowFrame && (extra > 0);
       rowFrame = rowFrame->GetNextRow(), rowIndex++) {
    RowInfo& rInfo = rowInfo[rowIndex];
    if (rInfo.hasPctBSize) {
      nscoord rowExtra = (rInfo.pctBSize > rInfo.bSize)
                         ? rInfo.pctBSize - rInfo.bSize: 0;
      rowExtra = std::min(rowExtra, extra);
      UpdateBSizes(rInfo, rowExtra, bSizeOfRows, bSizeOfUnStyledRows);
      extra -= rowExtra;
    }
  }

  bool styleBSizeAllocation = false;
  nscoord rowGroupBSize = startRowGroupBSize + bSizeOfRows +
                           tableFrame->GetRowSpacing(0, numRows-1);
  
  if ((aReflowState.ComputedBSize() > rowGroupBSize) &&
      (NS_UNCONSTRAINEDSIZE != aReflowState.ComputedBSize())) {
    nscoord extraComputedBSize = aReflowState.ComputedBSize() - rowGroupBSize;
    nscoord extraUsed = 0;
    bool haveUnStyledRows = (bSizeOfUnStyledRows > 0);
    nscoord divisor = (haveUnStyledRows)
                      ? bSizeOfUnStyledRows : bSizeOfRows;
    if (divisor > 0) {
      styleBSizeAllocation = true;
      for (rowIndex = 0; rowIndex < numRows; rowIndex++) {
        if (!haveUnStyledRows || !rowInfo[rowIndex].hasStyleBSize) {
          
          
          float percent = ((float)rowInfo[rowIndex].bSize) / ((float)divisor);
          
          nscoord extraForRow = (numRows - 1 == rowIndex)
                                ? extraComputedBSize - extraUsed
                                : NSToCoordRound(((float)extraComputedBSize) * percent);
          extraForRow = std::min(extraForRow, extraComputedBSize - extraUsed);
          
          UpdateBSizes(rowInfo[rowIndex], extraForRow, bSizeOfRows, bSizeOfUnStyledRows);
          extraUsed += extraForRow;
          if (extraUsed >= extraComputedBSize) {
            NS_ASSERTION((extraUsed == extraComputedBSize), "invalid row bsize calculation");
            break;
          }
        }
      }
    }
    rowGroupBSize = aReflowState.ComputedBSize();
  }

  if (wm.IsVertical()) {
    
    
    containerWidth = rowGroupBSize;
  }

  nscoord bOrigin = startRowGroupBSize;
  
  for (rowFrame = startRowFrame, rowIndex = 0; rowFrame;
       rowFrame = rowFrame->GetNextRow(), rowIndex++) {
    nsRect rowBounds = rowFrame->GetRect();
    LogicalSize rowBoundsSize(wm, rowBounds.Size());
    nsRect rowVisualOverflow = rowFrame->GetVisualOverflowRect();
    nscoord deltaB =
      bOrigin - rowFrame->GetLogicalNormalPosition(wm, containerWidth).B(wm);

    nscoord rowBSize = (rowInfo[rowIndex].bSize > 0) ? rowInfo[rowIndex].bSize : 0;

    if (deltaB != 0 || (rowBSize != rowBoundsSize.BSize(wm))) {
      
      if (deltaB != 0) {
        rowFrame->InvalidateFrameSubtree();
      }

      rowFrame->MovePositionBy(wm, LogicalPoint(wm, 0, deltaB));
      rowFrame->SetSize(LogicalSize(wm, rowBoundsSize.ISize(wm), rowBSize));

      nsTableFrame::InvalidateTableFrame(rowFrame, rowBounds, rowVisualOverflow,
                                         false);

      if (deltaB != 0) {
        nsTableFrame::RePositionViews(rowFrame);
        
      }
    }
    bOrigin += rowBSize + tableFrame->GetRowSpacing(startRowIndex + rowIndex);
  }

  if (isPaginated && styleBSizeAllocation) {
    
    
    CacheRowBSizesForPrinting(aPresContext, GetFirstRow(), wm);
  }

  DidResizeRows(aDesiredSize);

  aDesiredSize.BSize(wm) = rowGroupBSize; 
}

nscoord
nsTableRowGroupFrame::CollapseRowGroupIfNecessary(nscoord aBTotalOffset,
                                                  nscoord aISize,
                                                  WritingMode aWM)
{
  nsTableFrame* tableFrame = GetTableFrame();
  nscoord containerWidth = tableFrame->GetRect().width;
  const nsStyleVisibility* groupVis = StyleVisibility();
  bool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE == groupVis->mVisible);
  if (collapseGroup) {
    tableFrame->SetNeedToCollapse(true);
  }

  nsOverflowAreas overflow;

  nsTableRowFrame* rowFrame = GetFirstRow();
  bool didCollapse = false;
  nscoord bGroupOffset = 0;
  while (rowFrame) {
    bGroupOffset += rowFrame->CollapseRowIfNecessary(bGroupOffset,
                                                     aISize, collapseGroup,
                                                     didCollapse);
    ConsiderChildOverflow(overflow, rowFrame);
    rowFrame = rowFrame->GetNextRow();
  }

  LogicalRect groupRect = GetLogicalRect(aWM, containerWidth);
  nsRect oldGroupRect = GetRect();
  nsRect oldGroupVisualOverflow = GetVisualOverflowRect();

  groupRect.BSize(aWM) -= bGroupOffset;
  if (didCollapse) {
    
    groupRect.BSize(aWM) += tableFrame->GetRowSpacing(GetStartRowIndex() +
                                                      GetRowCount());
  }

  groupRect.BStart(aWM) -= aBTotalOffset;
  groupRect.ISize(aWM) = aISize;

  if (aBTotalOffset != 0) {
    InvalidateFrameSubtree();
  }

  SetRect(aWM, groupRect, containerWidth);
  overflow.UnionAllWith(nsRect(0, 0, groupRect.Width(aWM),
                               groupRect.Height(aWM)));
  FinishAndStoreOverflow(overflow, groupRect.Size(aWM).GetPhysicalSize(aWM));
  nsTableFrame::RePositionViews(this);
  nsTableFrame::InvalidateTableFrame(this, oldGroupRect, oldGroupVisualOverflow,
                                     false);

  return bGroupOffset;
}


void
nsTableRowGroupFrame::SlideChild(nsRowGroupReflowState& aReflowState,
                                 nsIFrame*              aKidFrame)
{
  
  WritingMode wm = aReflowState.reflowState.GetWritingMode();
  LogicalPoint oldPosition = aKidFrame->GetLogicalNormalPosition(wm, 0);
  LogicalPoint newPosition = oldPosition;
  newPosition.B(wm) = aReflowState.bCoord;
  if (oldPosition.B(wm) != newPosition.B(wm)) {
    aKidFrame->InvalidateFrameSubtree();
    aReflowState.reflowState.ApplyRelativePositioning(&newPosition, 0);
    aKidFrame->SetPosition(wm, newPosition, 0);
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
                                         nscoord                  aSpanningRowBEnd,
                                         nsTableRowFrame*&        aContRow,
                                         nsTableRowFrame*&        aFirstTruncatedRow,
                                         nscoord&                 aDesiredBSize)
{
  NS_ASSERTION(aSpanningRowBEnd >= 0, "Can't split negative bsizes");
  aFirstTruncatedRow = nullptr;
  aDesiredBSize     = 0;

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
        
        
        nscoord cellAvailBSize = aSpanningRowBEnd - rowPos.y;
        NS_ASSERTION(cellAvailBSize >= 0, "No space for cell?");
        bool isTopOfPage = (row == &aFirstRow) && aFirstRowIsTopOfPage;

        nsRect rowRect = row->GetNormalRect();
        nsSize rowAvailSize(aReflowState.AvailableWidth(),
                            std::max(aReflowState.AvailableHeight() - rowRect.y,
                                   0));
        
        
        rowAvailSize.height = std::min(rowAvailSize.height, rowRect.height);
        nsHTMLReflowState rowReflowState(&aPresContext, aReflowState, row,
                                         LogicalSize(row->GetWritingMode(),
                                                     rowAvailSize),
                                         nullptr,
                                         nsHTMLReflowState::CALLER_WILL_INIT);
        InitChildReflowState(aPresContext, borderCollapse, rowReflowState);
        rowReflowState.mFlags.mIsTopOfPage = isTopOfPage; 

        nscoord cellBSize = row->ReflowCellFrame(&aPresContext, rowReflowState,
                                                  isTopOfPage, cell,
                                                  cellAvailBSize, status);
        aDesiredBSize = std::max(aDesiredBSize, rowPos.y + cellBSize);
        if (NS_FRAME_IS_COMPLETE(status)) {
          if (cellBSize > cellAvailBSize) {
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
    aDesiredBSize = aLastRow.GetNormalRect().YMost();
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
    nscoord cellSpacingB = aTableFrame->GetRowSpacing(rowFrame->GetRowIndex());
    nsRect rowRect = rowFrame->GetNormalRect();
    
    if (rowRect.YMost() > availHeight) {
      nsTableRowFrame* contRow = nullptr;
      
      
      
      if (!prevRowFrame || (availHeight - aDesiredSize.Height() > pageHeight / 20)) {
        nsSize availSize(availWidth, std::max(availHeight - rowRect.y, 0));
        
        availSize.height = std::min(availSize.height, rowRect.height);

        nsHTMLReflowState rowReflowState(aPresContext, aReflowState, rowFrame,
                                         LogicalSize(rowFrame->GetWritingMode(),
                                                     availSize),
                                         nullptr,
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
                aDesiredSize.Height() += cellSpacingB;
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
                aDesiredSize.Height() += cellSpacingB;
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
      nscoord bMost;
      SplitSpanningCells(*aPresContext, aReflowState, *aTableFrame, *firstRowThisPage,
                         *lastRowThisPage, aReflowState.mFlags.mIsTopOfPage, spanningRowBottom, contRow,
                         firstTruncatedRow, bMost);
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
        aDesiredSize.Height() = std::max(aDesiredSize.Height(), bMost);
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

  
  nsTableFrame::CheckRequestSpecialBSizeReflow(aReflowState);

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
    
    bool specialReflow = (bool)aReflowState.mFlags.mSpecialBSizeReflow;
    ((nsHTMLReflowState::ReflowStateFlags&)aReflowState.mFlags).mSpecialBSizeReflow = false;

    SplitRowGroup(aPresContext, aDesiredSize, aReflowState, tableFrame, aStatus,
                  splitDueToPageBreak);

    ((nsHTMLReflowState::ReflowStateFlags&)aReflowState.mFlags).mSpecialBSizeReflow = specialReflow;
  }

  
  
  
  if (GetNextInFlow() && GetNextInFlow()->GetFirstPrincipalChild()) {
    NS_FRAME_SET_INCOMPLETE(aStatus);
  }

  SetHasStyleBSize((NS_UNCONSTRAINEDSIZE != aReflowState.ComputedBSize()) &&
                    (aReflowState.ComputedBSize() > 0));

  
  
  WritingMode wm = aReflowState.GetWritingMode();
  aDesiredSize.ISize(wm) = aReflowState.AvailableISize();

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
nsTableRowGroupFrame::GetBSizeBasis(const nsHTMLReflowState& aReflowState)
{
  nscoord result = 0;
  nsTableFrame* tableFrame = GetTableFrame();
  int32_t startRowIndex = GetStartRowIndex();
  if ((aReflowState.ComputedBSize() > 0) && (aReflowState.ComputedBSize() < NS_UNCONSTRAINEDSIZE)) {
    nscoord cellSpacing = tableFrame->GetRowSpacing(startRowIndex,
                                                    std::max(startRowIndex,
                                                             startRowIndex + GetRowCount() - 1));
    result = aReflowState.ComputedBSize() - cellSpacing;
  }
  else {
    const nsHTMLReflowState* parentRS = aReflowState.parentReflowState;
    if (parentRS && (tableFrame != parentRS->frame)) {
      parentRS = parentRS->parentReflowState;
    }
    if (parentRS && (tableFrame == parentRS->frame) &&
        (parentRS->ComputedBSize() > 0) && (parentRS->ComputedBSize() < NS_UNCONSTRAINEDSIZE)) {
      nscoord cellSpacing = tableFrame->GetRowSpacing(-1, tableFrame->GetRowCount());
      result = parentRS->ComputedBSize() - cellSpacing;
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

LogicalMargin
nsTableRowGroupFrame::GetBCBorderWidth(WritingMode aWM)
{
  LogicalMargin border(aWM);
  nsTableRowFrame* firstRowFrame = nullptr;
  nsTableRowFrame* lastRowFrame = nullptr;
  for (nsTableRowFrame* rowFrame = GetFirstRow(); rowFrame; rowFrame = rowFrame->GetNextRow()) {
    if (!firstRowFrame) {
      firstRowFrame = rowFrame;
    }
    lastRowFrame = rowFrame;
  }
  if (firstRowFrame) {
    border.BStart(aWM) = nsPresContext::
      CSSPixelsToAppUnits(firstRowFrame->GetBStartBCBorderWidth());
    border.BEnd(aWM) = nsPresContext::
      CSSPixelsToAppUnits(lastRowFrame->GetBEndBCBorderWidth());
  }
  return border;
}

void nsTableRowGroupFrame::SetContinuousBCBorderWidth(LogicalSide aForSide,
                                                      BCPixelSize aPixelValue)
{
  switch (aForSide) {
    case eLogicalSideIEnd:
      mIEndContBorderWidth = aPixelValue;
      return;
    case eLogicalSideBEnd:
      mBEndContBorderWidth = aPixelValue;
      return;
    case eLogicalSideIStart:
      mIStartContBorderWidth = aPixelValue;
      return;
    default:
      NS_ERROR("invalid LogicalSide argument");
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
