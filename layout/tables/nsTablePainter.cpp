




#include "nsTableFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsTableRowFrame.h"
#include "nsTableColGroupFrame.h"
#include "nsTableColFrame.h"
#include "nsTableCellFrame.h"
#include "nsTablePainter.h"
#include "nsCSSRendering.h"
#include "nsDisplayList.h"



















































































using namespace mozilla::image;

TableBackgroundPainter::TableBackgroundData::TableBackgroundData()
  : mFrame(nullptr)
  , mVisible(false)
  , mUsesSynthBorder(false)
{
}

TableBackgroundPainter::TableBackgroundData::TableBackgroundData(nsIFrame* aFrame)
  : mFrame(aFrame)
  , mRect(aFrame->GetRect())
  , mVisible(mFrame->IsVisibleForPainting())
  , mUsesSynthBorder(false)
{
}

inline bool
TableBackgroundPainter::TableBackgroundData::ShouldSetBCBorder() const
{
  
  if (!mVisible) {
    return false;
  }

  const nsStyleBackground *bg = mFrame->StyleBackground();
  NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(i, bg) {
    if (!bg->mLayers[i].mImage.IsEmpty())
      return true;
  }
  return false;
}

void
TableBackgroundPainter::TableBackgroundData::SetBCBorder(const nsMargin& aBorder)
{
  mUsesSynthBorder = true;
  mSynthBorderWidths = aBorder;
}

nsStyleBorder
TableBackgroundPainter::TableBackgroundData::StyleBorder(const nsStyleBorder& aZeroBorder) const
{
  MOZ_ASSERT(mVisible, "Don't call StyleBorder on an invisible TableBackgroundData");

  if (mUsesSynthBorder) {
    nsStyleBorder result = aZeroBorder;
    NS_FOR_CSS_SIDES(side) {
      result.SetBorderWidth(side, mSynthBorderWidths.Side(side));
    }
    return result;
  }

  MOZ_ASSERT(mFrame);

  return *mFrame->StyleBorder();
}

TableBackgroundPainter::TableBackgroundPainter(nsTableFrame*        aTableFrame,
                                               Origin               aOrigin,
                                               nsPresContext*       aPresContext,
                                               nsRenderingContext& aRenderingContext,
                                               const nsRect&        aDirtyRect,
                                               const nsPoint&       aRenderPt,
                                               uint32_t             aBGPaintFlags)
  : mPresContext(aPresContext),
    mRenderingContext(aRenderingContext),
    mRenderPt(aRenderPt),
    mDirtyRect(aDirtyRect),
    mOrigin(aOrigin),
    mZeroBorder(aPresContext),
    mBGPaintFlags(aBGPaintFlags)
{
  MOZ_COUNT_CTOR(TableBackgroundPainter);

  NS_FOR_CSS_SIDES(side) {
    mZeroBorder.SetBorderStyle(side, NS_STYLE_BORDER_STYLE_SOLID);
    mZeroBorder.SetBorderWidth(side, 0);
  }

  mIsBorderCollapse = aTableFrame->IsBorderCollapse();
#ifdef DEBUG
  mCompatMode = mPresContext->CompatibilityMode();
#endif
  mNumCols = aTableFrame->GetColCount();
}

TableBackgroundPainter::~TableBackgroundPainter()
{
  MOZ_COUNT_DTOR(TableBackgroundPainter);
}

static void
UpdateDrawResult(DrawResult* aCurrentResult, DrawResult aNewResult)
{
  MOZ_ASSERT(aCurrentResult);
  if (*aCurrentResult == DrawResult::SUCCESS) {
    *aCurrentResult = aNewResult;
  }
}

DrawResult
TableBackgroundPainter::PaintTableFrame(nsTableFrame*         aTableFrame,
                                        nsTableRowGroupFrame* aFirstRowGroup,
                                        nsTableRowGroupFrame* aLastRowGroup,
                                        const nsMargin&       aDeflate)
{
  MOZ_ASSERT(aTableFrame, "null frame");
  TableBackgroundData tableData(aTableFrame);
  tableData.mRect.MoveTo(0,0); 
  tableData.mRect.Deflate(aDeflate);
  if (mIsBorderCollapse && tableData.ShouldSetBCBorder()) {
    if (aFirstRowGroup && aLastRowGroup && mNumCols > 0) {
      
      
      nsMargin border, tempBorder;
      nsTableColFrame* colFrame = aTableFrame->GetColFrame(mNumCols - 1);
      if (colFrame) {
        colFrame->GetContinuousBCBorderWidth(tempBorder);
      }
      border.right = tempBorder.right;

      aLastRowGroup->GetContinuousBCBorderWidth(tempBorder);
      border.bottom = tempBorder.bottom;

      nsTableRowFrame* rowFrame = aFirstRowGroup->GetFirstRow();
      if (rowFrame) {
        rowFrame->GetContinuousBCBorderWidth(tempBorder);
        border.top = tempBorder.top;
      }

      border.left = aTableFrame->GetContinuousLeftBCBorderWidth();

      tableData.SetBCBorder(border);
    }
  }

  DrawResult result = DrawResult::SUCCESS;

  if (tableData.IsVisible()) {
    result =
      nsCSSRendering::PaintBackgroundWithSC(mPresContext, mRenderingContext,
                                            tableData.mFrame, mDirtyRect,
                                            tableData.mRect + mRenderPt,
                                            tableData.mFrame->StyleContext(),
                                            tableData.StyleBorder(mZeroBorder),
                                            mBGPaintFlags);
  }

  return result;
}

void
TableBackgroundPainter::TranslateContext(nscoord aDX,
                                         nscoord aDY)
{
  mRenderPt += nsPoint(aDX, aDY);
  for (auto& col : mCols) {
    col.mCol.mRect.MoveBy(-aDX, -aDY);
  }
  for (auto& colGroup : mColGroups) {
    colGroup.mRect.MoveBy(-aDX, -aDY);
  }
}

TableBackgroundPainter::ColData::ColData(nsIFrame* aFrame, TableBackgroundData& aColGroupBGData)
  : mCol(aFrame)
  , mColGroup(aColGroupBGData)
{
}

DrawResult
TableBackgroundPainter::PaintTable(nsTableFrame*   aTableFrame,
                                   const nsMargin& aDeflate,
                                   bool            aPaintTableBackground)
{
  NS_PRECONDITION(aTableFrame, "null table frame");

  nsTableFrame::RowGroupArray rowGroups;
  aTableFrame->OrderRowGroups(rowGroups);

  DrawResult result = DrawResult::SUCCESS;

  if (rowGroups.Length() < 1) { 
    if (aPaintTableBackground) {
      PaintTableFrame(aTableFrame, nullptr, nullptr, nsMargin(0,0,0,0));
    }
    
    return result;
  }

  if (aPaintTableBackground) {
    PaintTableFrame(aTableFrame, rowGroups[0], rowGroups[rowGroups.Length() - 1],
                    aDeflate);
  }

  
  if (mNumCols > 0) {
    nsFrameList& colGroupList = aTableFrame->GetColGroups();
    NS_ASSERTION(colGroupList.FirstChild(), "table should have at least one colgroup");

    
    nsTArray<nsTableColGroupFrame*> colGroupFrames;
    for (nsTableColGroupFrame* cgFrame = static_cast<nsTableColGroupFrame*>(colGroupList.FirstChild());
         cgFrame; cgFrame = static_cast<nsTableColGroupFrame*>(cgFrame->GetNextSibling())) {

      if (cgFrame->GetColCount() < 1) {
        
        continue;
      }
      colGroupFrames.AppendElement(cgFrame);
    }

    
    
    
    mColGroups.SetCapacity(colGroupFrames.Length());

    nsMargin border;
    

    
    nscoord lastLeftBorder = aTableFrame->GetContinuousLeftBCBorderWidth();

    for (nsTableColGroupFrame* cgFrame : colGroupFrames) {
      
      TableBackgroundData& cgData = *mColGroups.AppendElement(TableBackgroundData(cgFrame));
      if (mIsBorderCollapse && cgData.ShouldSetBCBorder()) {
        border.left = lastLeftBorder;
        cgFrame->GetContinuousBCBorderWidth(border);
        cgData.SetBCBorder(border);
      }

      
      for (nsTableColFrame* col = cgFrame->GetFirstColumn(); col;
           col = static_cast<nsTableColFrame*>(col->GetNextSibling())) {
        MOZ_ASSERT(size_t(col->GetColIndex()) == mCols.Length());
        
        ColData& colData = *mCols.AppendElement(ColData(col, cgData));
        
        colData.mCol.mRect.MoveBy(cgData.mRect.x, cgData.mRect.y);
        if (mIsBorderCollapse) {
          border.left = lastLeftBorder;
          lastLeftBorder = col->GetContinuousBCBorderWidth(border);
          if (colData.mCol.ShouldSetBCBorder()) {
            colData.mCol.SetBCBorder(border);
          }
        }
      }
    }
  }

  for (uint32_t i = 0; i < rowGroups.Length(); i++) {
    nsTableRowGroupFrame* rg = rowGroups[i];
    TableBackgroundData rowGroupBGData(rg);
    
    
    rowGroupBGData.mRect.MoveTo(rg->GetOffsetTo(aTableFrame));

    
    
    
    nsRect rgVisualOverflow = rg->GetVisualOverflowRectRelativeToSelf();
    nsRect rgOverflowRect = rgVisualOverflow + rg->GetPosition();
    nsRect rgNormalRect = rgVisualOverflow + rg->GetNormalPosition();

    if (rgOverflowRect.Union(rgNormalRect).Intersects(mDirtyRect - mRenderPt)) {
      DrawResult rowGroupResult =
        PaintRowGroup(rg, rowGroupBGData, rg->IsPseudoStackingContextFromStyle());
      UpdateDrawResult(&result, rowGroupResult);
    }
  }

  return result;
}

DrawResult
TableBackgroundPainter::PaintRowGroup(nsTableRowGroupFrame* aFrame)
{
  return PaintRowGroup(aFrame, TableBackgroundData(aFrame), false);
}

DrawResult
TableBackgroundPainter::PaintRowGroup(nsTableRowGroupFrame* aFrame,
                                      TableBackgroundData   aRowGroupBGData,
                                      bool                  aPassThrough)
{
  MOZ_ASSERT(aFrame, "null frame");

  nsTableRowFrame* firstRow = aFrame->GetFirstRow();

  
  if (aPassThrough) {
    aRowGroupBGData.MakeInvisible();
  } else {
    if (mIsBorderCollapse && aRowGroupBGData.ShouldSetBCBorder()) {
      nsMargin border;
      if (firstRow) {
        
        firstRow->GetContinuousBCBorderWidth(border);
        
      }
      
      aFrame->GetContinuousBCBorderWidth(border);
      aRowGroupBGData.SetBCBorder(border);
    }
    aPassThrough = !aRowGroupBGData.IsVisible();
  }

  
  if (eOrigin_TableRowGroup != mOrigin) {
    TranslateContext(aRowGroupBGData.mRect.x, aRowGroupBGData.mRect.y);
  }
  nsRect rgRect = aRowGroupBGData.mRect;
  aRowGroupBGData.mRect.MoveTo(0, 0);

  

  
  
  
  nscoord overflowAbove;
  nsIFrame* cursor = aFrame->GetFirstRowContaining(mDirtyRect.y - mRenderPt.y, &overflowAbove);

  
  
  while (cursor && cursor->GetType() != nsGkAtoms::tableRowFrame) {
    cursor = cursor->GetNextSibling();
  }

  
  nsTableRowFrame* row = static_cast<nsTableRowFrame*>(cursor);
  if (!row) {
    
    
    
    
    row = firstRow;
  }

  DrawResult result = DrawResult::SUCCESS;

  
  for (; row; row = row->GetNextRow()) {
    TableBackgroundData rowBackgroundData(row);

    
    
    nscoord rowY = std::min(rowBackgroundData.mRect.y, row->GetNormalPosition().y);

    
    if (cursor &&
        (mDirtyRect.YMost() - mRenderPt.y) <= (rowY - overflowAbove)) {
      
      break;
    }

    DrawResult rowResult =
      PaintRow(row, aRowGroupBGData, rowBackgroundData,
               aPassThrough || row->IsPseudoStackingContextFromStyle());

    UpdateDrawResult(&result, rowResult);
  }

  
  if (eOrigin_TableRowGroup != mOrigin) {
    TranslateContext(-rgRect.x, -rgRect.y);
  }

  return result;
}

DrawResult
TableBackgroundPainter::PaintRow(nsTableRowFrame* aFrame)
{
  return PaintRow(aFrame, TableBackgroundData(), TableBackgroundData(aFrame), false);
}

DrawResult
TableBackgroundPainter::PaintRow(nsTableRowFrame* aFrame,
                                 const TableBackgroundData& aRowGroupBGData,
                                 TableBackgroundData aRowBGData,
                                 bool             aPassThrough)
{
  MOZ_ASSERT(aFrame, "null frame");

  
  if (aPassThrough) {
    aRowBGData.MakeInvisible();
  } else {
    if (mIsBorderCollapse && aRowBGData.ShouldSetBCBorder()) {
      nsMargin border;
      nsTableRowFrame* nextRow = aFrame->GetNextRow();
      if (nextRow) { 
        border.bottom = nextRow->GetOuterTopContBCBorderWidth();
      }
      else { 
        nsTableRowGroupFrame* rowGroup = static_cast<nsTableRowGroupFrame*>(aFrame->GetParent());
        rowGroup->GetContinuousBCBorderWidth(border);
      }
      
      aFrame->GetContinuousBCBorderWidth(border);

      aRowBGData.SetBCBorder(border);
    }
    aPassThrough = !aRowBGData.IsVisible();
  }

  
  if (eOrigin_TableRow == mOrigin) {
    
    aRowBGData.mRect.MoveTo(0, 0);
  }
  

  DrawResult result = DrawResult::SUCCESS;

  for (nsTableCellFrame* cell = aFrame->GetFirstCell(); cell; cell = cell->GetNextCell()) {
    nsRect cellBGRect, rowBGRect, rowGroupBGRect, colBGRect;
    ComputeCellBackgrounds(cell, aRowGroupBGData, aRowBGData,
                           cellBGRect, rowBGRect,
                           rowGroupBGRect, colBGRect);

    
    nsRect combinedRect(cellBGRect);
    combinedRect.UnionRect(combinedRect, rowBGRect);
    combinedRect.UnionRect(combinedRect, rowGroupBGRect);
    combinedRect.UnionRect(combinedRect, colBGRect);

    if (combinedRect.Intersects(mDirtyRect)) {
      bool passCell = aPassThrough || cell->IsPseudoStackingContextFromStyle();
      DrawResult cellResult =
        PaintCell(cell, aRowGroupBGData, aRowBGData,
                  cellBGRect, rowBGRect, rowGroupBGRect, colBGRect, passCell);

      UpdateDrawResult(&result, cellResult);
    }
  }

  return result;
}

DrawResult
TableBackgroundPainter::PaintCell(nsTableCellFrame* aCell,
                                  const TableBackgroundData& aRowGroupBGData,
                                  const TableBackgroundData& aRowBGData,
                                  nsRect&           aCellBGRect,
                                  nsRect&           aRowBGRect,
                                  nsRect&           aRowGroupBGRect,
                                  nsRect&           aColBGRect,
                                  bool              aPassSelf)
{
  MOZ_ASSERT(aCell, "null frame");

  const nsStyleTableBorder* cellTableStyle;
  cellTableStyle = aCell->StyleTableBorder();
  if (NS_STYLE_TABLE_EMPTY_CELLS_SHOW != cellTableStyle->mEmptyCells &&
      aCell->GetContentEmpty() && !mIsBorderCollapse) {
    return DrawResult::SUCCESS;
  }

  int32_t colIndex;
  aCell->GetColIndex(colIndex);
  
  
  NS_ASSERTION(size_t(colIndex) < mNumCols, "out-of-bounds column index");
  if (size_t(colIndex) >= mNumCols) {
    return DrawResult::SUCCESS;
  }

  
  
  bool haveColumns = !mCols.IsEmpty();

  DrawResult result = DrawResult::SUCCESS;

  
  if (haveColumns && mCols[colIndex].mColGroup.IsVisible()) {
    DrawResult colGroupResult =
      nsCSSRendering::PaintBackgroundWithSC(mPresContext, mRenderingContext,
                                            mCols[colIndex].mColGroup.mFrame, mDirtyRect,
                                            mCols[colIndex].mColGroup.mRect + mRenderPt,
                                            mCols[colIndex].mColGroup.mFrame->StyleContext(),
                                            mCols[colIndex].mColGroup.StyleBorder(mZeroBorder),
                                            mBGPaintFlags, &aColBGRect);
    UpdateDrawResult(&result, colGroupResult);
  }

  
  if (haveColumns && mCols[colIndex].mCol.IsVisible()) {
    DrawResult colResult =
      nsCSSRendering::PaintBackgroundWithSC(mPresContext, mRenderingContext,
                                            mCols[colIndex].mCol.mFrame, mDirtyRect,
                                            mCols[colIndex].mCol.mRect + mRenderPt,
                                            mCols[colIndex].mCol.mFrame->StyleContext(),
                                            mCols[colIndex].mCol.StyleBorder(mZeroBorder),
                                            mBGPaintFlags, &aColBGRect);
    UpdateDrawResult(&result, colResult);
  }

  
  if (aRowGroupBGData.IsVisible()) {
    DrawResult rowGroupResult =
      nsCSSRendering::PaintBackgroundWithSC(mPresContext, mRenderingContext,
                                            aRowGroupBGData.mFrame, mDirtyRect,
                                            aRowGroupBGData.mRect + mRenderPt,
                                            aRowGroupBGData.mFrame->StyleContext(),
                                            aRowGroupBGData.StyleBorder(mZeroBorder),
                                            mBGPaintFlags, &aRowGroupBGRect);
    UpdateDrawResult(&result, rowGroupResult);
  }

  
  if (aRowBGData.IsVisible()) {
    DrawResult rowResult =
      nsCSSRendering::PaintBackgroundWithSC(mPresContext, mRenderingContext,
                                            aRowBGData.mFrame, mDirtyRect,
                                            aRowBGData.mRect + mRenderPt,
                                            aRowBGData.mFrame->StyleContext(),
                                            aRowBGData.StyleBorder(mZeroBorder),
                                            mBGPaintFlags, &aRowBGRect);
    UpdateDrawResult(&result, rowResult);
  }

  
  if (mIsBorderCollapse && !aPassSelf) {
    DrawResult cellResult =
      aCell->PaintCellBackground(mRenderingContext, mDirtyRect,
                                 aCellBGRect.TopLeft(), mBGPaintFlags);
    UpdateDrawResult(&result, cellResult);
  }

  return result;
}

void
TableBackgroundPainter::ComputeCellBackgrounds(nsTableCellFrame* aCell,
                                               const TableBackgroundData& aRowGroupBGData,
                                               const TableBackgroundData& aRowBGData,
                                               nsRect&           aCellBGRect,
                                               nsRect&           aRowBGRect,
                                               nsRect&           aRowGroupBGRect,
                                               nsRect&           aColBGRect)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  nsIFrame* rowGroupFrame =
    aRowGroupBGData.mFrame ? aRowGroupBGData.mFrame : aRowBGData.mFrame->GetParent();

  
  
  aCellBGRect = aCell->GetRect() + aRowBGData.mRect.TopLeft() + mRenderPt;

  
  
  aRowBGRect = aCellBGRect + (aCell->GetNormalPosition() - aCell->GetPosition());

  
  
  aRowGroupBGRect = aRowBGRect +
                    (aRowBGData.mFrame->GetNormalPosition() - aRowBGData.mFrame->GetPosition());

  
  
  
  
  aColBGRect = aRowGroupBGRect +
             (rowGroupFrame->GetNormalPosition() - rowGroupFrame->GetPosition());

}
