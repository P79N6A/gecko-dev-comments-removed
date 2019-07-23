




































#include "nsTableFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsTableRowFrame.h"
#include "nsTableColGroupFrame.h"
#include "nsTableColFrame.h"
#include "nsTableCellFrame.h"
#include "nsTablePainter.h"
#include "nsCSSRendering.h"
#include "nsDisplayList.h"

























































































TableBackgroundPainter::TableBackgroundData::TableBackgroundData()
  : mFrame(nsnull),
    mBackground(nsnull),
    mBorder(nsnull),
    mSynthBorder(nsnull)
{
  MOZ_COUNT_CTOR(TableBackgroundData);
}

TableBackgroundPainter::TableBackgroundData::~TableBackgroundData()
{
  NS_ASSERTION(!mSynthBorder, "must call Destroy before dtor");
  MOZ_COUNT_DTOR(TableBackgroundData);
}

void
TableBackgroundPainter::TableBackgroundData::Destroy(nsPresContext* aPresContext)
{
  NS_PRECONDITION(aPresContext, "null prescontext");
  if (mSynthBorder) {
    mSynthBorder->Destroy(aPresContext);
    mSynthBorder = nsnull;
  }
}

void
TableBackgroundPainter::TableBackgroundData::Clear()
{
  mRect.Empty();
  mFrame = nsnull;
  mBorder = nsnull;
  mBackground = nsnull;
}

void
TableBackgroundPainter::TableBackgroundData::SetFrame(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null frame");
  mFrame = aFrame;
  mRect = aFrame->GetRect();
}

void
TableBackgroundPainter::TableBackgroundData::SetData()
{
  NS_PRECONDITION(mFrame, "null frame");
  if (mFrame->IsVisibleForPainting()) {
    mBackground = mFrame->GetStyleBackground();
    mBorder = mFrame->GetStyleBorder();
  }
}

void
TableBackgroundPainter::TableBackgroundData::SetFull(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null frame");
  SetFrame(aFrame);
  SetData();
}

inline PRBool
TableBackgroundPainter::TableBackgroundData::ShouldSetBCBorder()
{
  
  if (!mBackground) {
    return PR_FALSE;
  }

  NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(i, mBackground) {
    if (!mBackground->mLayers[i].mImage.IsEmpty())
      return PR_TRUE;
  }
  return PR_FALSE;
}

nsresult
TableBackgroundPainter::TableBackgroundData::SetBCBorder(nsMargin& aBorder,
                                                         TableBackgroundPainter* aPainter)
{
  NS_PRECONDITION(aPainter, "null painter");
  if (!mSynthBorder) {
    mSynthBorder = new (aPainter->mPresContext)
                        nsStyleBorder(aPainter->mZeroBorder);
    if (!mSynthBorder) return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_FOR_CSS_SIDES(side) {
    mSynthBorder->SetBorderWidth(side, aBorder.side(side));
  }
  
  mBorder = mSynthBorder;
  return NS_OK;
}

TableBackgroundPainter::TableBackgroundPainter(nsTableFrame*        aTableFrame,
                                               Origin               aOrigin,
                                               nsPresContext*       aPresContext,
                                               nsIRenderingContext& aRenderingContext,
                                               const nsRect&        aDirtyRect,
                                               const nsPoint&       aRenderPt)
  : mPresContext(aPresContext),
    mRenderingContext(aRenderingContext),
    mRenderPt(aRenderPt),
    mDirtyRect(aDirtyRect),
    mOrigin(aOrigin),
    mCols(nsnull),
    mZeroBorder(aPresContext)
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
  if (mCols) {
    TableBackgroundData* lastColGroup = nsnull;
    for (PRUint32 i = 0; i < mNumCols; i++) {
      if (mCols[i].mColGroup != lastColGroup) {
        lastColGroup = mCols[i].mColGroup;
        NS_ASSERTION(mCols[i].mColGroup, "colgroup data should not be null - bug 237421");
        
        if(lastColGroup)
          lastColGroup->Destroy(mPresContext);
        delete lastColGroup;
      }
      mCols[i].mColGroup = nsnull;
      mCols[i].mCol.Destroy(mPresContext);
    }
    delete [] mCols;
  }
  mRowGroup.Destroy(mPresContext);
  mRow.Destroy(mPresContext);
  MOZ_COUNT_DTOR(TableBackgroundPainter);
}

nsresult
TableBackgroundPainter::PaintTableFrame(nsTableFrame*         aTableFrame,
                                        nsTableRowGroupFrame* aFirstRowGroup,
                                        nsTableRowGroupFrame* aLastRowGroup,
                                        const nsMargin&       aDeflate)
{
  NS_PRECONDITION(aTableFrame, "null frame");
  TableBackgroundData tableData;
  tableData.SetFull(aTableFrame);
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

      nsresult rv = tableData.SetBCBorder(border, this);
      if (NS_FAILED(rv)) {
        tableData.Destroy(mPresContext);
        return rv;
      }
    }
  }
  if (tableData.IsVisible()) {
    nsCSSRendering::PaintBackgroundWithSC(mPresContext, mRenderingContext,
                                          tableData.mFrame, mDirtyRect,
                                          tableData.mRect + mRenderPt,
                                          *tableData.mBackground,
                                          *tableData.mBorder,
                                          0);
  }
  tableData.Destroy(mPresContext);
  return NS_OK;
}

void
TableBackgroundPainter::TranslateContext(nscoord aDX,
                                         nscoord aDY)
{
  mRenderPt += nsPoint(aDX, aDY);
  if (mCols) {
    TableBackgroundData* lastColGroup = nsnull;
    for (PRUint32 i = 0; i < mNumCols; i++) {
      mCols[i].mCol.mRect.MoveBy(-aDX, -aDY);
      if (lastColGroup != mCols[i].mColGroup) {
        NS_ASSERTION(mCols[i].mColGroup, "colgroup data should not be null - bug 237421");
        
        if (!mCols[i].mColGroup)
          return;
        mCols[i].mColGroup->mRect.MoveBy(-aDX, -aDY);
        lastColGroup = mCols[i].mColGroup;
      }
    }
  }
}

nsresult
TableBackgroundPainter::PaintTable(nsTableFrame*   aTableFrame,
                                   const nsMargin& aDeflate,
                                   PRBool          aPaintTableBackground)
{
  NS_PRECONDITION(aTableFrame, "null table frame");

  nsTableFrame::RowGroupArray rowGroups;
  aTableFrame->OrderRowGroups(rowGroups);

  if (rowGroups.Length() < 1) { 
    if (aPaintTableBackground) {
      PaintTableFrame(aTableFrame, nsnull, nsnull, nsMargin(0,0,0,0));
    }
    
    return NS_OK;
  }

  if (aPaintTableBackground) {
    PaintTableFrame(aTableFrame, rowGroups[0], rowGroups[rowGroups.Length() - 1],
                    aDeflate);
  }

  
  if (mNumCols > 0) {
    nsFrameList& colGroupList = aTableFrame->GetColGroups();
    NS_ASSERTION(colGroupList.FirstChild(), "table should have at least one colgroup");

    mCols = new ColData[mNumCols];
    if (!mCols) return NS_ERROR_OUT_OF_MEMORY;

    TableBackgroundData* cgData = nsnull;
    nsMargin border;
    

    
    nscoord lastLeftBorder = aTableFrame->GetContinuousLeftBCBorderWidth();
    for (nsTableColGroupFrame* cgFrame = static_cast<nsTableColGroupFrame*>(colGroupList.FirstChild());
         cgFrame; cgFrame = static_cast<nsTableColGroupFrame*>(cgFrame->GetNextSibling())) {

      if (cgFrame->GetColCount() < 1) {
        
        continue;
      }

      
      cgData = new TableBackgroundData;
      if (!cgData) return NS_ERROR_OUT_OF_MEMORY;
      cgData->SetFull(cgFrame);
      if (mIsBorderCollapse && cgData->ShouldSetBCBorder()) {
        border.left = lastLeftBorder;
        cgFrame->GetContinuousBCBorderWidth(border);
        nsresult rv = cgData->SetBCBorder(border, this);
        if (NS_FAILED(rv)) {
          cgData->Destroy(mPresContext);
          delete cgData;
          return rv;
        }
      }

      
      PRBool cgDataOwnershipTaken = PR_FALSE;
      
      
      for (nsTableColFrame* col = cgFrame->GetFirstColumn(); col;
           col = static_cast<nsTableColFrame*>(col->GetNextSibling())) {
        
        PRUint32 colIndex = col->GetColIndex();
        NS_ASSERTION(colIndex < mNumCols, "prevent array boundary violation");
        if (mNumCols <= colIndex)
          break;
        mCols[colIndex].mCol.SetFull(col);
        
        mCols[colIndex].mCol.mRect.MoveBy(cgData->mRect.x, cgData->mRect.y);
        
        mCols[colIndex].mColGroup = cgData;
        cgDataOwnershipTaken = PR_TRUE;
        if (mIsBorderCollapse) {
          border.left = lastLeftBorder;
          lastLeftBorder = col->GetContinuousBCBorderWidth(border);
          if (mCols[colIndex].mCol.ShouldSetBCBorder()) {
            nsresult rv = mCols[colIndex].mCol.SetBCBorder(border, this);
            if (NS_FAILED(rv)) return rv;
          }
        }
      }

      if (!cgDataOwnershipTaken) {
        cgData->Destroy(mPresContext);
        delete cgData;
      }
    }
  }

  for (PRUint32 i = 0; i < rowGroups.Length(); i++) {
    nsTableRowGroupFrame* rg = rowGroups[i];
    mRowGroup.SetFrame(rg);
    
    
    mRowGroup.mRect.MoveTo(rg->GetOffsetTo(aTableFrame));
    if (mRowGroup.mRect.Intersects(mDirtyRect - mRenderPt)) {
      nsresult rv = PaintRowGroup(rg,
              rg->IsPseudoStackingContextFromStyle() || rg->IsScrolled());
      if (NS_FAILED(rv)) return rv;
    }
  }
  return NS_OK;
}

nsresult
TableBackgroundPainter::PaintRowGroup(nsTableRowGroupFrame* aFrame,
                                      PRBool                aPassThrough)
{
  NS_PRECONDITION(aFrame, "null frame");

  if (!mRowGroup.mFrame) {
    mRowGroup.SetFrame(aFrame);
  }

  nsTableRowFrame* firstRow = aFrame->GetFirstRow();

  
  if (!aPassThrough) {
    mRowGroup.SetData();
    if (mIsBorderCollapse && mRowGroup.ShouldSetBCBorder()) {
      nsMargin border;
      if (firstRow) {
        
        firstRow->GetContinuousBCBorderWidth(border);
        
      }
      
      aFrame->GetContinuousBCBorderWidth(border);
      nsresult res = mRowGroup.SetBCBorder(border, this);
      if (!NS_SUCCEEDED(res)) {
        return res;
      }
    }
    aPassThrough = !mRowGroup.IsVisible();
  }

  
  if (eOrigin_TableRowGroup != mOrigin) {
    TranslateContext(mRowGroup.mRect.x, mRowGroup.mRect.y);
  }
  nsRect rgRect = mRowGroup.mRect;
  mRowGroup.mRect.MoveTo(0, 0);

  
  nscoord ignored; 
                   
                   
                   
                   

  
  
  
  nsIFrame* cursor = aFrame->GetFirstRowContaining(mDirtyRect.y - mRenderPt.y, &ignored);

  
  
  while (cursor && cursor->GetType() != nsGkAtoms::tableRowFrame) {
    cursor = cursor->GetNextSibling();
  }

  
  nsTableRowFrame* row = static_cast<nsTableRowFrame*>(cursor);  
  if (!row) {
    
    
    
    
    row = firstRow;
  }
  
  
  for (; row; row = row->GetNextRow()) {
    mRow.SetFrame(row);
    if (mDirtyRect.YMost() - mRenderPt.y < mRow.mRect.y) { 
                                             

      
      break;
    }
    
    nsresult rv = PaintRow(row, aPassThrough || row->IsPseudoStackingContextFromStyle());
    if (NS_FAILED(rv)) return rv;
  }

  
  if (eOrigin_TableRowGroup != mOrigin) {
    TranslateContext(-rgRect.x, -rgRect.y);
  }
  
  
  mRowGroup.Clear();

  return NS_OK;
}

nsresult
TableBackgroundPainter::PaintRow(nsTableRowFrame* aFrame,
                                 PRBool           aPassThrough)
{
  NS_PRECONDITION(aFrame, "null frame");

  if (!mRow.mFrame) {
    mRow.SetFrame(aFrame);
  }

  
  if (!aPassThrough) {
    mRow.SetData();
    if (mIsBorderCollapse && mRow.ShouldSetBCBorder()) {
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

      nsresult res = mRow.SetBCBorder(border, this);
      if (!NS_SUCCEEDED(res)) {
        return res;
      }
    }
    aPassThrough = !mRow.IsVisible();
  }

  
  if (eOrigin_TableRow == mOrigin) {
    
    mRow.mRect.MoveTo(0, 0);
  }
  

  for (nsTableCellFrame* cell = aFrame->GetFirstCell(); cell; cell = cell->GetNextCell()) {
    
    mCellRect = cell->GetRect() + mRow.mRect.TopLeft() + mRenderPt;
    if (mCellRect.Intersects(mDirtyRect)) {
      nsresult rv = PaintCell(cell, aPassThrough || cell->IsPseudoStackingContextFromStyle());
      if (NS_FAILED(rv)) return rv;
    }
  }

  
  mRow.Clear();
  return NS_OK;
}

nsresult
TableBackgroundPainter::PaintCell(nsTableCellFrame* aCell,
                                  PRBool aPassSelf)
{
  NS_PRECONDITION(aCell, "null frame");

  const nsStyleTableBorder* cellTableStyle;
  cellTableStyle = aCell->GetStyleTableBorder();
  if (!(NS_STYLE_TABLE_EMPTY_CELLS_SHOW == cellTableStyle->mEmptyCells ||
        NS_STYLE_TABLE_EMPTY_CELLS_SHOW_BACKGROUND == cellTableStyle->mEmptyCells)
      && aCell->GetContentEmpty() && !mIsBorderCollapse) {
    return NS_OK;
  }

  PRInt32 colIndex;
  aCell->GetColIndex(colIndex);
  NS_ASSERTION(colIndex < PRInt32(mNumCols), "prevent array boundary violation");
  if (PRInt32(mNumCols) <= colIndex)
    return NS_OK;

  
  if (mCols && mCols[colIndex].mColGroup && mCols[colIndex].mColGroup->IsVisible()) {
    nsCSSRendering::PaintBackgroundWithSC(mPresContext, mRenderingContext,
                                          mCols[colIndex].mColGroup->mFrame, mDirtyRect,
                                          mCols[colIndex].mColGroup->mRect + mRenderPt,
                                          *mCols[colIndex].mColGroup->mBackground,
                                          *mCols[colIndex].mColGroup->mBorder,
                                          0, &mCellRect);
  }

  
  if (mCols && mCols[colIndex].mCol.IsVisible()) {
    nsCSSRendering::PaintBackgroundWithSC(mPresContext, mRenderingContext,
                                          mCols[colIndex].mCol.mFrame, mDirtyRect,
                                          mCols[colIndex].mCol.mRect + mRenderPt,
                                          *mCols[colIndex].mCol.mBackground,
                                          *mCols[colIndex].mCol.mBorder,
                                          0, &mCellRect);
  }

  
  if (mRowGroup.IsVisible()) {
    nsCSSRendering::PaintBackgroundWithSC(mPresContext, mRenderingContext,
                                          mRowGroup.mFrame, mDirtyRect,
                                          mRowGroup.mRect + mRenderPt,
                                          *mRowGroup.mBackground, *mRowGroup.mBorder,
                                          0, &mCellRect);
  }

  
  if (mRow.IsVisible()) {
    nsCSSRendering::PaintBackgroundWithSC(mPresContext, mRenderingContext,
                                          mRow.mFrame, mDirtyRect,
                                          mRow.mRect + mRenderPt,
                                          *mRow.mBackground, *mRow.mBorder,
                                          0, &mCellRect);
  }

  
  if (mIsBorderCollapse && !aPassSelf) {
    aCell->PaintCellBackground(mRenderingContext, mDirtyRect,
                               mCellRect.TopLeft());
  }

  return NS_OK;
}
