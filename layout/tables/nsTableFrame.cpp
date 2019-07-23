






































#include "nsCOMPtr.h"
#include "nsTableFrame.h"
#include "nsIRenderingContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIContent.h"
#include "nsCellMap.h"
#include "nsTableCellFrame.h"
#include "nsHTMLParts.h"
#include "nsTableColFrame.h"
#include "nsTableColGroupFrame.h"
#include "nsTableRowFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsTableOuterFrame.h"
#include "nsTablePainter.h"

#include "BasicTableLayoutStrategy.h"
#include "FixedTableLayoutStrategy.h"

#include "nsPresContext.h"
#include "nsCSSRendering.h"
#include "nsStyleConsts.h"
#include "nsGkAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsIPresShell.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsIScrollableFrame.h"
#include "nsFrameManager.h"
#include "nsCSSRendering.h"
#include "nsLayoutErrors.h"
#include "nsAutoPtr.h"
#include "nsCSSFrameConstructor.h"
#include "nsStyleSet.h"
#include "nsDisplayList.h"





struct nsTableReflowState {

  
  const nsHTMLReflowState& reflowState;

  
  nsSize availSize;

  
  nscoord x;

  
  nscoord y;

  nsTableReflowState(nsPresContext&           aPresContext,
                     const nsHTMLReflowState& aReflowState,
                     nsTableFrame&            aTableFrame,
                     nscoord                  aAvailWidth,
                     nscoord                  aAvailHeight)
    : reflowState(aReflowState)
  {
    Init(aPresContext, aTableFrame, aAvailWidth, aAvailHeight);
  }

  void Init(nsPresContext&  aPresContext,
            nsTableFrame&   aTableFrame,
            nscoord         aAvailWidth,
            nscoord         aAvailHeight)
  {
    nsTableFrame* table = (nsTableFrame*)aTableFrame.GetFirstInFlow();
    nsMargin borderPadding = table->GetChildAreaOffset(&reflowState);
    nscoord cellSpacingX = table->GetCellSpacingX();

    x = borderPadding.left + cellSpacingX;
    y = borderPadding.top; 

    availSize.width  = aAvailWidth;
    if (NS_UNCONSTRAINEDSIZE != availSize.width) {
      availSize.width -= borderPadding.left + borderPadding.right
                         + (2 * cellSpacingX);
      availSize.width = PR_MAX(0, availSize.width);
    }

    availSize.height = aAvailHeight;
    if (NS_UNCONSTRAINEDSIZE != availSize.height) {
      availSize.height -= borderPadding.top + borderPadding.bottom
                          + (2 * table->GetCellSpacingY());
      availSize.height = PR_MAX(0, availSize.height);
    }
  }

  nsTableReflowState(nsPresContext&           aPresContext,
                     const nsHTMLReflowState& aReflowState,
                     nsTableFrame&            aTableFrame)
    : reflowState(aReflowState)
  {
    Init(aPresContext, aTableFrame, aReflowState.availableWidth, aReflowState.availableHeight);
  }

};





struct BCPropertyData
{
  BCPropertyData() { mDamageArea.x = mDamageArea.y = mDamageArea.width =
                     mDamageArea.height = mTopBorderWidth = mRightBorderWidth =
                     mBottomBorderWidth = mLeftBorderWidth =
                     mLeftCellBorderWidth = mRightCellBorderWidth = 0; }
  nsRect  mDamageArea;
  BCPixelSize mTopBorderWidth;
  BCPixelSize mRightBorderWidth;
  BCPixelSize mBottomBorderWidth;
  BCPixelSize mLeftBorderWidth;
  BCPixelSize mLeftCellBorderWidth;
  BCPixelSize mRightCellBorderWidth;
};

NS_IMETHODIMP 
nsTableFrame::GetParentStyleContextFrame(nsPresContext*  aPresContext,
                                         nsIFrame**      aProviderFrame,
                                         PRBool*         aIsChild)
{
  
  

  NS_PRECONDITION(mParent, "table constructed without outer table");
  if (!mContent->GetParent() && !GetStyleContext()->GetPseudoType()) {
    
    *aIsChild = PR_FALSE;
    *aProviderFrame = nsnull;
    return NS_OK;
  }
    
  return static_cast<nsFrame*>(mParent)->
          DoGetParentStyleContextFrame(aPresContext, aProviderFrame, aIsChild);
}


nsIAtom*
nsTableFrame::GetType() const
{
  return nsGkAtoms::tableFrame; 
}


nsTableFrame::nsTableFrame(nsStyleContext* aContext)
  : nsHTMLContainerFrame(aContext),
    mCellMap(nsnull),
    mTableLayoutStrategy(nsnull)
{
  mBits.mHaveReflowedColGroups  = PR_FALSE;
  mBits.mCellSpansPctCol        = PR_FALSE;
  mBits.mNeedToCalcBCBorders    = PR_FALSE;
  mBits.mIsBorderCollapse       = PR_FALSE;
  mBits.mResizedColumns         = PR_FALSE; 
  mBits.mGeometryDirty          = PR_FALSE;
}

NS_QUERYFRAME_HEAD(nsTableFrame)
  NS_QUERYFRAME_ENTRY(nsITableLayout)
NS_QUERYFRAME_TAIL_INHERITING(nsHTMLContainerFrame)

NS_IMETHODIMP
nsTableFrame::Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        aPrevInFlow)
{
  nsresult  rv;

  
  rv = nsHTMLContainerFrame::Init(aContent, aParent, aPrevInFlow);

  
  const nsStyleTableBorder* tableStyle = GetStyleTableBorder();
  PRBool borderCollapse = (NS_STYLE_BORDER_COLLAPSE == tableStyle->mBorderCollapse);
  SetBorderCollapse(borderCollapse);
  
  if (!aPrevInFlow) {
    mCellMap = new nsTableCellMap(*this, borderCollapse);
    if (!mCellMap)
      return NS_ERROR_OUT_OF_MEMORY;
  } else {
    mCellMap = nsnull;
  }

  if (aPrevInFlow) {
    
    
    mRect.width = aPrevInFlow->GetSize().width;
  }
  else {
    NS_ASSERTION(!mTableLayoutStrategy, "strategy was created before Init was called");
    
    if (IsAutoLayout())
      mTableLayoutStrategy = new BasicTableLayoutStrategy(this);
    else
      mTableLayoutStrategy = new FixedTableLayoutStrategy(this);
    if (!mTableLayoutStrategy)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  return rv;
}


nsTableFrame::~nsTableFrame()
{
  if (nsnull!=mCellMap) {
    delete mCellMap; 
    mCellMap = nsnull;
  }

  if (nsnull!=mTableLayoutStrategy) {
    delete mTableLayoutStrategy;
    mTableLayoutStrategy = nsnull;
  }
}

void
nsTableFrame::Destroy()
{
  mColGroups.DestroyFrames();
  nsHTMLContainerFrame::Destroy();
}


void
nsTableFrame::RePositionViews(nsIFrame* aFrame)
{
  nsContainerFrame::PositionFrameView(aFrame);
  nsContainerFrame::PositionChildViews(aFrame);
}

static PRBool
IsRepeatedFrame(nsIFrame* kidFrame)
{
  return (kidFrame->GetType() == nsGkAtoms::tableRowFrame ||
          kidFrame->GetType() == nsGkAtoms::tableRowGroupFrame) &&
         (kidFrame->GetStateBits() & NS_REPEATED_ROW_OR_ROWGROUP);
}

PRBool
nsTableFrame::PageBreakAfter(nsIFrame& aSourceFrame,
                             nsIFrame* aNextFrame)
{
  const nsStyleDisplay* display = aSourceFrame.GetStyleDisplay();
  
  if (display->mBreakAfter && !IsRepeatedFrame(&aSourceFrame)) {
    return !(aNextFrame && IsRepeatedFrame(aNextFrame)); 
  }

  if (aNextFrame) {
    display = aNextFrame->GetStyleDisplay();
    
    if (display->mBreakBefore && !IsRepeatedFrame(aNextFrame)) {
      return !IsRepeatedFrame(&aSourceFrame); 
    }
  }
  return PR_FALSE;
}



NS_IMETHODIMP
nsTableFrame::SetInitialChildList(nsIAtom*        aListName,
                                  nsFrameList&    aChildList)
{

  if (!mFrames.IsEmpty() || !mColGroups.IsEmpty()) {
    
    
    NS_NOTREACHED("unexpected second call to SetInitialChildList");
    return NS_ERROR_UNEXPECTED;
  }
  if (aListName) {
    
    NS_NOTREACHED("unknown frame list");
    return NS_ERROR_INVALID_ARG;
  } 

  
  
  
  
  
  
  nsIFrame *prevMainChild = nsnull;
  nsIFrame *prevColGroupChild = nsnull;
  while (aChildList.NotEmpty())
  {
    nsIFrame* childFrame = aChildList.FirstChild();
    aChildList.RemoveFrame(childFrame);
    const nsStyleDisplay* childDisplay = childFrame->GetStyleDisplay();

    if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == childDisplay->mDisplay)
    {
      NS_ASSERTION(nsGkAtoms::tableColGroupFrame == childFrame->GetType(),
                   "This is not a colgroup");
      mColGroups.InsertFrame(nsnull, prevColGroupChild, childFrame);
      prevColGroupChild = childFrame;
    }
    else
    { 
      mFrames.InsertFrame(nsnull, prevMainChild, childFrame);
      prevMainChild = childFrame;
    }
  }

  
  
  if (!GetPrevInFlow()) {
    
    
    InsertColGroups(0, mColGroups);
    InsertRowGroups(mFrames);
    
    if (IsBorderCollapse()) {
      nsRect damageArea(0, 0, GetColCount(), GetRowCount());
      SetBCDamageArea(damageArea);
    }
  }

  return NS_OK;
}

 PRBool
nsTableFrame::IsContainingBlock() const
{
  return PR_TRUE;
}

void nsTableFrame::AttributeChangedFor(nsIFrame*       aFrame,
                                       nsIContent*     aContent, 
                                       nsIAtom*        aAttribute)
{
  nsTableCellFrame *cellFrame = do_QueryFrame(aFrame);
  if (cellFrame) {
    if ((nsGkAtoms::rowspan == aAttribute) || 
        (nsGkAtoms::colspan == aAttribute)) {
      nsTableCellMap* cellMap = GetCellMap();
      if (cellMap) {
        
        PRInt32 rowIndex, colIndex;
        cellFrame->GetRowIndex(rowIndex);
        cellFrame->GetColIndex(colIndex);
        RemoveCell(cellFrame, rowIndex);
        nsAutoTArray<nsTableCellFrame*, 1> cells;
        cells.AppendElement(cellFrame);
        InsertCells(cells, rowIndex, colIndex - 1);

        
        
        PresContext()->PresShell()->
          FrameNeedsReflow(this, nsIPresShell::eTreeChange, NS_FRAME_IS_DIRTY);
      }
    }
  }
}





PRInt32 nsTableFrame::GetEffectiveColCount() const
{
  PRInt32 colCount = GetColCount();
  if (LayoutStrategy()->GetType() == nsITableLayoutStrategy::Auto) {
    nsTableCellMap* cellMap = GetCellMap();
    if (!cellMap) {
      return 0;
    }
    
    for (PRInt32 colX = colCount - 1; colX >= 0; colX--) {
      if (cellMap->GetNumCellsOriginatingInCol(colX) > 0) { 
        break;
      }
      colCount--;
    }
  }
  return colCount;
}

PRInt32 nsTableFrame::GetIndexOfLastRealCol()
{
  PRInt32 numCols = mColFrames.Length();
  if (numCols > 0) {
    for (PRInt32 colX = numCols - 1; colX >= 0; colX--) { 
      nsTableColFrame* colFrame = GetColFrame(colX);
      if (colFrame) {
        if (eColAnonymousCell != colFrame->GetColType()) {
          return colX;
        }
      }
    }
  }
  return -1; 
}

nsTableColFrame*
nsTableFrame::GetColFrame(PRInt32 aColIndex) const
{
  NS_ASSERTION(!GetPrevInFlow(), "GetColFrame called on next in flow");
  PRInt32 numCols = mColFrames.Length();
  if ((aColIndex >= 0) && (aColIndex < numCols)) {
    return mColFrames.ElementAt(aColIndex);
  }
  else {
    NS_ERROR("invalid col index");
    return nsnull;
  }
}

PRInt32 nsTableFrame::GetEffectiveRowSpan(PRInt32                 aRowIndex,
                                          const nsTableCellFrame& aCell) const
{
  nsTableCellMap* cellMap = GetCellMap();
  NS_PRECONDITION (nsnull != cellMap, "bad call, cellMap not yet allocated.");

  PRInt32 colIndex;
  aCell.GetColIndex(colIndex);
  return cellMap->GetEffectiveRowSpan(aRowIndex, colIndex);
}

PRInt32 nsTableFrame::GetEffectiveRowSpan(const nsTableCellFrame& aCell,
                                          nsCellMap*              aCellMap)
{
  nsTableCellMap* tableCellMap = GetCellMap(); if (!tableCellMap) ABORT1(1);

  PRInt32 colIndex, rowIndex;
  aCell.GetColIndex(colIndex);
  aCell.GetRowIndex(rowIndex);

  if (aCellMap) 
    return aCellMap->GetRowSpan(rowIndex, colIndex, PR_TRUE);
  else
    return tableCellMap->GetEffectiveRowSpan(rowIndex, colIndex);
}

PRInt32 nsTableFrame::GetEffectiveColSpan(const nsTableCellFrame& aCell,
                                          nsCellMap*              aCellMap) const
{
  nsTableCellMap* tableCellMap = GetCellMap(); if (!tableCellMap) ABORT1(1);

  PRInt32 colIndex, rowIndex;
  aCell.GetColIndex(colIndex);
  aCell.GetRowIndex(rowIndex);
  PRBool ignore;

  if (aCellMap) 
    return aCellMap->GetEffectiveColSpan(*tableCellMap, rowIndex, colIndex, ignore);
  else
    return tableCellMap->GetEffectiveColSpan(rowIndex, colIndex);
}

PRBool nsTableFrame::HasMoreThanOneCell(PRInt32 aRowIndex) const
{
  nsTableCellMap* tableCellMap = GetCellMap(); if (!tableCellMap) ABORT1(1);
  return tableCellMap->HasMoreThanOneCell(aRowIndex);
}

PRInt32 nsTableFrame::GetEffectiveCOLSAttribute()
{
  NS_PRECONDITION (GetCellMap(), "null cellMap.");

  PRInt32 result;
  result = GetStyleTable()->mCols;
  PRInt32 numCols = GetColCount();
  if (result > numCols)
    result = numCols;
  return result;
}

void nsTableFrame::AdjustRowIndices(PRInt32         aRowIndex,
                                    PRInt32         aAdjustment)
{
  
  
  RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);

  for (PRUint32 rgX = 0; rgX < rowGroups.Length(); rgX++) {
    rowGroups[rgX]->AdjustRowIndices(aRowIndex, aAdjustment);
  }
}


void nsTableFrame::ResetRowIndices(const nsFrameList::Slice& aRowGroupsToExclude)
{
  
  
  
  
  
  RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);

  PRInt32 rowIndex = 0;
  nsTableRowGroupFrame* newRgFrame = nsnull;
  nsFrameList::Enumerator excludeRowGroupsEnumerator(aRowGroupsToExclude);
  if (!excludeRowGroupsEnumerator.AtEnd()) {
    newRgFrame = GetRowGroupFrame(excludeRowGroupsEnumerator.get());
    excludeRowGroupsEnumerator.Next();
  }

  for (PRUint32 rgX = 0; rgX < rowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
    if (rgFrame == newRgFrame) {
      
      if (!excludeRowGroupsEnumerator.AtEnd()) {
        newRgFrame = GetRowGroupFrame(excludeRowGroupsEnumerator.get());
        excludeRowGroupsEnumerator.Next();
      }
    }
    else {
      const nsFrameList& rowFrames = rgFrame->GetChildList(nsnull);
      for (nsFrameList::Enumerator rows(rowFrames); !rows.AtEnd(); rows.Next()) {
        if (NS_STYLE_DISPLAY_TABLE_ROW==rows.get()->GetStyleDisplay()->mDisplay) {
          ((nsTableRowFrame *)rows.get())->SetRowIndex(rowIndex);
          rowIndex++;
        }
      }
    }
  }
}
void nsTableFrame::InsertColGroups(PRInt32                   aStartColIndex,
                                   const nsFrameList::Slice& aColGroups)
{
  PRInt32 colIndex = aStartColIndex;
  nsFrameList::Enumerator colGroups(aColGroups);
  for (; !colGroups.AtEnd(); colGroups.Next()) {
    nsTableColGroupFrame* cgFrame =
      static_cast<nsTableColGroupFrame*>(colGroups.get());
    cgFrame->SetStartColumnIndex(colIndex);
    
    
    
    

    
    
    
    
    cgFrame->AddColsToTable(colIndex, PR_FALSE,
                              colGroups.get()->GetChildList(nsnull));
    PRInt32 numCols = cgFrame->GetColCount();
    colIndex += numCols;
  }

  nsFrameList::Enumerator remainingColgroups = colGroups.GetUnlimitedEnumerator();
  if (!remainingColgroups.AtEnd()) {
    nsTableColGroupFrame::ResetColIndices(
      static_cast<nsTableColGroupFrame*>(remainingColgroups.get()), colIndex);
  }
}

void nsTableFrame::InsertCol(nsTableColFrame& aColFrame,
                             PRInt32          aColIndex)
{
  mColFrames.InsertElementAt(aColIndex, &aColFrame);
  nsTableColType insertedColType = aColFrame.GetColType();
  PRInt32 numCacheCols = mColFrames.Length();
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    PRInt32 numMapCols = cellMap->GetColCount();
    if (numCacheCols > numMapCols) {
      PRBool removedFromCache = PR_FALSE;
      if (eColAnonymousCell != insertedColType) {
        nsTableColFrame* lastCol = mColFrames.ElementAt(numCacheCols - 1);
        if (lastCol) {
          nsTableColType lastColType = lastCol->GetColType();
          if (eColAnonymousCell == lastColType) {
            
            mColFrames.RemoveElementAt(numCacheCols - 1);
            
            nsTableColGroupFrame* lastColGroup = (nsTableColGroupFrame *)mColGroups.LastChild();
            if (lastColGroup) {
              lastColGroup->RemoveChild(*lastCol, PR_FALSE);
            }
            
            if (lastColGroup->GetColCount() <= 0) {
              mColGroups.DestroyFrame((nsIFrame*)lastColGroup);
            }
            removedFromCache = PR_TRUE;
          }
        }
      }
      if (!removedFromCache) {
        cellMap->AddColsAtEnd(1);
      }
    }
  }
  
  if (IsBorderCollapse()) {
    nsRect damageArea(0, 0, PR_MAX(1, GetColCount()), PR_MAX(1, GetRowCount()));
    SetBCDamageArea(damageArea);
  }
}

void nsTableFrame::RemoveCol(nsTableColGroupFrame* aColGroupFrame,
                             PRInt32               aColIndex,
                             PRBool                aRemoveFromCache,
                             PRBool                aRemoveFromCellMap)
{
  if (aRemoveFromCache) {
    mColFrames.RemoveElementAt(aColIndex);
  }
  if (aRemoveFromCellMap) {
    nsTableCellMap* cellMap = GetCellMap();
    if (cellMap) {
      AppendAnonymousColFrames(1);
    }
  }
  
  if (IsBorderCollapse()) {
    nsRect damageArea(0, 0, GetColCount(), GetRowCount());
    SetBCDamageArea(damageArea);
  }
}




nsTableCellMap* nsTableFrame::GetCellMap() const
{
  nsTableFrame* firstInFlow = (nsTableFrame *)GetFirstInFlow();
  return firstInFlow->mCellMap;
}


nsTableColGroupFrame*
nsTableFrame::CreateAnonymousColGroupFrame(nsTableColGroupType aColGroupType)
{
  nsIContent* colGroupContent = GetContent();
  nsPresContext* presContext = PresContext();
  nsIPresShell *shell = presContext->PresShell();

  nsRefPtr<nsStyleContext> colGroupStyle;
  colGroupStyle = shell->StyleSet()->ResolvePseudoStyleFor(colGroupContent,
                                                           nsCSSAnonBoxes::tableColGroup,
                                                           mStyleContext);
  
  nsIFrame* newFrame = NS_NewTableColGroupFrame(shell, colGroupStyle);
  if (newFrame) {
    ((nsTableColGroupFrame *)newFrame)->SetColType(aColGroupType);
    newFrame->Init(colGroupContent, this, nsnull);
  }
  return (nsTableColGroupFrame *)newFrame;
}

void
nsTableFrame::AppendAnonymousColFrames(PRInt32 aNumColsToAdd)
{
  
  nsTableColGroupFrame* colGroupFrame =
    static_cast<nsTableColGroupFrame*>(mColGroups.LastChild());

  if (!colGroupFrame ||
      (colGroupFrame->GetColType() != eColGroupAnonymousCell)) {
    PRInt32 colIndex = (colGroupFrame) ?
                        colGroupFrame->GetStartColumnIndex() +
                        colGroupFrame->GetColCount() : 0;
    colGroupFrame = CreateAnonymousColGroupFrame(eColGroupAnonymousCell);
    if (!colGroupFrame) {
      return;
    }
    
    mColGroups.AppendFrame(this, colGroupFrame);
    colGroupFrame->SetStartColumnIndex(colIndex);
  }
  AppendAnonymousColFrames(colGroupFrame, aNumColsToAdd, eColAnonymousCell,
                           PR_TRUE);

}



void
nsTableFrame::AppendAnonymousColFrames(nsTableColGroupFrame* aColGroupFrame,
                                       PRInt32               aNumColsToAdd,
                                       nsTableColType        aColType,
                                       PRBool                aAddToTable)
{
  NS_PRECONDITION(aColGroupFrame, "null frame");
  NS_PRECONDITION(aColType != eColAnonymousCol, "Shouldn't happen");

  nsIPresShell *shell = PresContext()->PresShell();

  
  nsFrameItems newColFrames;

  PRInt32 startIndex = mColFrames.Length();
  PRInt32 lastIndex  = startIndex + aNumColsToAdd - 1; 

  for (PRInt32 childX = startIndex; childX <= lastIndex; childX++) {
    nsIContent* iContent;
    nsRefPtr<nsStyleContext> styleContext;
    nsStyleContext* parentStyleContext;

    
    
    iContent = aColGroupFrame->GetContent();
    parentStyleContext = aColGroupFrame->GetStyleContext();
    styleContext = shell->StyleSet()->ResolvePseudoStyleFor(iContent,
                                                            nsCSSAnonBoxes::tableCol,
                                                            parentStyleContext);
    
    NS_ASSERTION(iContent, "null content in CreateAnonymousColFrames");

    
    nsIFrame* colFrame = NS_NewTableColFrame(shell, styleContext);
    ((nsTableColFrame *) colFrame)->SetColType(aColType);
    colFrame->Init(iContent, aColGroupFrame, nsnull);

    newColFrames.AddChild(colFrame);
  }
  nsFrameList& cols = aColGroupFrame->GetWritableChildList();
  nsIFrame* oldLastCol = cols.LastChild();
  const nsFrameList::Slice& newCols =
    cols.InsertFrames(nsnull, oldLastCol, newColFrames);
  if (aAddToTable) {
    
    PRInt32 startColIndex;
    if (oldLastCol) {
      startColIndex =
        static_cast<nsTableColFrame*>(oldLastCol)->GetColIndex() + 1;
    } else {
      startColIndex = aColGroupFrame->GetStartColumnIndex();
    }

    aColGroupFrame->AddColsToTable(startColIndex, PR_TRUE, newCols);
  }
}

void
nsTableFrame::MatchCellMapToColCache(nsTableCellMap* aCellMap)
{
  PRInt32 numColsInMap   = GetColCount();
  PRInt32 numColsInCache = mColFrames.Length();
  PRInt32 numColsToAdd = numColsInMap - numColsInCache;
  if (numColsToAdd > 0) {
    
    AppendAnonymousColFrames(numColsToAdd);
  }
  if (numColsToAdd < 0) {
    PRInt32 numColsNotRemoved = DestroyAnonymousColFrames(-numColsToAdd);
    
    if (numColsNotRemoved > 0) {
      aCellMap->AddColsAtEnd(numColsNotRemoved);
    }
  }
  if (numColsToAdd && HasZeroColSpans()) {
    SetNeedColSpanExpansion(PR_TRUE);
  }
  if (NeedColSpanExpansion()) {
    
    
    
    
    
    
    
    

    aCellMap->ExpandZeroColSpans();
  }
}

void
nsTableFrame::DidResizeColumns()
{
  NS_PRECONDITION(!GetPrevInFlow(),
                  "should only be called on first-in-flow");
  if (mBits.mResizedColumns)
    return; 

  for (nsTableFrame *f = this; f;
       f = static_cast<nsTableFrame*>(f->GetNextInFlow()))
    f->mBits.mResizedColumns = PR_TRUE;
}

void
nsTableFrame::AppendCell(nsTableCellFrame& aCellFrame,
                         PRInt32           aRowIndex)
{
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    nsRect damageArea(0,0,0,0);
    cellMap->AppendCell(aCellFrame, aRowIndex, PR_TRUE, damageArea);
    MatchCellMapToColCache(cellMap);
    if (IsBorderCollapse()) {
      SetBCDamageArea(damageArea);
    }
  }
}

void nsTableFrame::InsertCells(nsTArray<nsTableCellFrame*>& aCellFrames,
                               PRInt32                      aRowIndex,
                               PRInt32                      aColIndexBefore)
{
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    nsRect damageArea(0,0,0,0);
    cellMap->InsertCells(aCellFrames, aRowIndex, aColIndexBefore, damageArea);
    MatchCellMapToColCache(cellMap);
    if (IsBorderCollapse()) {
      SetBCDamageArea(damageArea);
    }
  }
}


PRInt32 
nsTableFrame::DestroyAnonymousColFrames(PRInt32 aNumFrames)
{
  
  PRInt32 endIndex   = mColFrames.Length() - 1;
  PRInt32 startIndex = (endIndex - aNumFrames) + 1;
  PRInt32 numColsRemoved = 0;
  for (PRInt32 colX = endIndex; colX >= startIndex; colX--) {
    nsTableColFrame* colFrame = GetColFrame(colX);
    if (colFrame && (eColAnonymousCell == colFrame->GetColType())) {
      nsTableColGroupFrame* cgFrame =
        static_cast<nsTableColGroupFrame*>(colFrame->GetParent());
      
      cgFrame->RemoveChild(*colFrame, PR_FALSE);
      
      RemoveCol(nsnull, colX, PR_TRUE, PR_FALSE);
      numColsRemoved++;
    }
    else {
      break; 
    }
  }
  return (aNumFrames - numColsRemoved);
}

void nsTableFrame::RemoveCell(nsTableCellFrame* aCellFrame,
                              PRInt32           aRowIndex)
{
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    nsRect damageArea(0,0,0,0);
    cellMap->RemoveCell(aCellFrame, aRowIndex, damageArea);
    MatchCellMapToColCache(cellMap);
    if (IsBorderCollapse()) {
      SetBCDamageArea(damageArea);
    }
  }
}

PRInt32
nsTableFrame::GetStartRowIndex(nsTableRowGroupFrame& aRowGroupFrame)
{
  RowGroupArray orderedRowGroups;
  OrderRowGroups(orderedRowGroups);

  PRInt32 rowIndex = 0;
  for (PRUint32 rgIndex = 0; rgIndex < orderedRowGroups.Length(); rgIndex++) {
    nsTableRowGroupFrame* rgFrame = orderedRowGroups[rgIndex];
    if (rgFrame == &aRowGroupFrame) {
      break;
    }
    PRInt32 numRows = rgFrame->GetRowCount();
    rowIndex += numRows;
  }
  return rowIndex;
}


void nsTableFrame::AppendRows(nsTableRowGroupFrame&       aRowGroupFrame,
                              PRInt32                     aRowIndex,
                              nsTArray<nsTableRowFrame*>& aRowFrames)
{
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    PRInt32 absRowIndex = GetStartRowIndex(aRowGroupFrame) + aRowIndex;
    InsertRows(aRowGroupFrame, aRowFrames, absRowIndex, PR_TRUE);
  }
}

PRInt32
nsTableFrame::InsertRow(nsTableRowGroupFrame& aRowGroupFrame,
                        nsIFrame&             aRowFrame,
                        PRInt32               aRowIndex,
                        PRBool                aConsiderSpans)
{
  nsAutoTArray<nsTableRowFrame*, 1> rows;
  rows.AppendElement((nsTableRowFrame*)&aRowFrame);
  return InsertRows(aRowGroupFrame, rows, aRowIndex, aConsiderSpans);
}


PRInt32
nsTableFrame::InsertRows(nsTableRowGroupFrame&       aRowGroupFrame,
                         nsTArray<nsTableRowFrame*>& aRowFrames,
                         PRInt32                     aRowIndex,
                         PRBool                      aConsiderSpans)
{
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== insertRowsBefore firstRow=%d \n", aRowIndex);
  Dump(PR_TRUE, PR_FALSE, PR_TRUE);
#endif

  PRInt32 numColsToAdd = 0;
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    nsRect damageArea(0,0,0,0);
    PRInt32 origNumRows = cellMap->GetRowCount();
    PRInt32 numNewRows = aRowFrames.Length();
    cellMap->InsertRows(aRowGroupFrame, aRowFrames, aRowIndex, aConsiderSpans, damageArea);
    MatchCellMapToColCache(cellMap);
    if (aRowIndex < origNumRows) {
      AdjustRowIndices(aRowIndex, numNewRows);
    }
    
    
    for (PRInt32 rowY = 0; rowY < numNewRows; rowY++) {
      nsTableRowFrame* rowFrame = aRowFrames.ElementAt(rowY);
      rowFrame->SetRowIndex(aRowIndex + rowY);
    }
    if (IsBorderCollapse()) {
      SetBCDamageArea(damageArea);
    }
  }
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== insertRowsAfter \n");
  Dump(PR_TRUE, PR_FALSE, PR_TRUE);
#endif

  return numColsToAdd;
}


void nsTableFrame::RemoveRows(nsTableRowFrame& aFirstRowFrame,
                              PRInt32          aNumRowsToRemove,
                              PRBool           aConsiderSpans)
{
#ifdef TBD_OPTIMIZATION
  
  
  PRBool stopTelling = PR_FALSE;
  for (nsIFrame* kidFrame = aFirstFrame.FirstChild(); (kidFrame && !stopAsking);
       kidFrame = kidFrame->GetNextSibling()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
    if (cellFrame) {
      stopTelling = tableFrame->CellChangedWidth(*cellFrame, cellFrame->GetPass1MaxElementWidth(), 
                                                 cellFrame->GetMaximumWidth(), PR_TRUE);
    }
  }
  
  
#endif

  PRInt32 firstRowIndex = aFirstRowFrame.GetRowIndex();
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== removeRowsBefore firstRow=%d numRows=%d\n", firstRowIndex, aNumRowsToRemove);
  Dump(PR_TRUE, PR_FALSE, PR_TRUE);
#endif
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    nsRect damageArea(0,0,0,0);
    cellMap->RemoveRows(firstRowIndex, aNumRowsToRemove, aConsiderSpans, damageArea);
    MatchCellMapToColCache(cellMap);
    if (IsBorderCollapse()) {
      SetBCDamageArea(damageArea);
    }
  }
  AdjustRowIndices(firstRowIndex, -aNumRowsToRemove);
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== removeRowsAfter\n");
  Dump(PR_TRUE, PR_TRUE, PR_TRUE);
#endif
}

nsTableRowGroupFrame*
nsTableFrame::GetRowGroupFrame(nsIFrame* aFrame,
                               nsIAtom*  aFrameTypeIn)
{
  nsIFrame* rgFrame = nsnull;
  nsIAtom* frameType = aFrameTypeIn;
  if (!aFrameTypeIn) {
    frameType = aFrame->GetType();
  }
  if (nsGkAtoms::tableRowGroupFrame == frameType) {
    rgFrame = aFrame;
  }
  else if (nsGkAtoms::scrollFrame == frameType) {
    nsIScrollableFrame* scrollable = do_QueryFrame(aFrame);
    if (scrollable) {
      nsIFrame* scrolledFrame = scrollable->GetScrolledFrame();
      if (scrolledFrame) {
        if (nsGkAtoms::tableRowGroupFrame == scrolledFrame->GetType()) {
          rgFrame = scrolledFrame;
        }
      }
    }
  }
  return (nsTableRowGroupFrame*)rgFrame;
}


PRInt32
nsTableFrame::CollectRows(nsIFrame*                   aFrame,
                          nsTArray<nsTableRowFrame*>& aCollection)
{
  if (!aFrame) return 0;
  PRInt32 numRows = 0;
  nsTableRowGroupFrame* rgFrame = GetRowGroupFrame(aFrame);
  if (rgFrame) {
    nsIFrame* childFrame = rgFrame->GetFirstChild(nsnull);
    while (childFrame) {
      nsTableRowFrame *rowFrame = do_QueryFrame(childFrame);
      if (rowFrame) {
        aCollection.AppendElement(rowFrame);
        numRows++;
      }
      else {
        numRows += CollectRows(childFrame, aCollection);
      }
      childFrame = childFrame->GetNextSibling();
    }
  }
  return numRows;
}

void
nsTableFrame::InsertRowGroups(const nsFrameList::Slice& aRowGroups)
{
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== insertRowGroupsBefore\n");
  Dump(PR_TRUE, PR_FALSE, PR_TRUE);
#endif
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    RowGroupArray orderedRowGroups;
    OrderRowGroups(orderedRowGroups);

    nsAutoTArray<nsTableRowFrame*, 8> rows;
    
    
    
    
    PRUint32 rgIndex;
    for (rgIndex = 0; rgIndex < orderedRowGroups.Length(); rgIndex++) {
      for (nsFrameList::Enumerator rowgroups(aRowGroups); !rowgroups.AtEnd();
           rowgroups.Next()) {
        nsTableRowGroupFrame* rgFrame = GetRowGroupFrame(rowgroups.get());

        if (orderedRowGroups[rgIndex] == rgFrame) {
          nsTableRowGroupFrame* priorRG =
            (0 == rgIndex) ? nsnull : orderedRowGroups[rgIndex - 1]; 
          
          cellMap->InsertGroupCellMap(*rgFrame, priorRG);
        
          break;
        }
      }
    }
    cellMap->Synchronize(this);
    ResetRowIndices(aRowGroups);

    
    for (rgIndex = 0; rgIndex < orderedRowGroups.Length(); rgIndex++) {
      for (nsFrameList::Enumerator rowgroups(aRowGroups); !rowgroups.AtEnd();
           rowgroups.Next()) {
        nsTableRowGroupFrame* rgFrame = GetRowGroupFrame(rowgroups.get());

        if (orderedRowGroups[rgIndex] == rgFrame) {
          nsTableRowGroupFrame* priorRG =
            (0 == rgIndex) ? nsnull : orderedRowGroups[rgIndex - 1]; 
          
          PRInt32 numRows = CollectRows(rowgroups.get(), rows);
          if (numRows > 0) {
            PRInt32 rowIndex = 0;
            if (priorRG) {
              PRInt32 priorNumRows = priorRG->GetRowCount();
              rowIndex = priorRG->GetStartRowIndex() + priorNumRows;
            }
            InsertRows(*rgFrame, rows, rowIndex, PR_TRUE);
            rows.Clear();
          }
          break;
        }
      }
    }    
    
  }
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== insertRowGroupsAfter\n");
  Dump(PR_TRUE, PR_TRUE, PR_TRUE);
#endif
}





nsFrameList
nsTableFrame::GetChildList(nsIAtom* aListName) const
{
  if (aListName == nsGkAtoms::colGroupList) {
    return mColGroups;
  }

  return nsHTMLContainerFrame::GetChildList(aListName);
}

nsIAtom*
nsTableFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  if (aIndex == NS_TABLE_FRAME_COLGROUP_LIST_INDEX) {
    return nsGkAtoms::colGroupList;
  }
  if (aIndex == NS_TABLE_FRAME_OVERFLOW_LIST_INDEX) {
    return nsGkAtoms::overflowList;
  } 
  return nsnull;
}

nsRect
nsDisplayTableItem::GetBounds(nsDisplayListBuilder* aBuilder) {
  return mFrame->GetOverflowRect() + aBuilder->ToReferenceFrame(mFrame);
}

PRBool
nsDisplayTableItem::IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder)
{
  if (!mPartHasFixedBackground)
    return PR_FALSE;

  
  
  
  
  
  
  nsIFrame* rootMover = aBuilder->GetRootMovingFrame();
  return mFrame == rootMover ||
    nsLayoutUtils::IsProperAncestorFrame(rootMover, mFrame);
}

 void
nsDisplayTableItem::UpdateForFrameBackground(nsIFrame* aFrame)
{
  const nsStyleBackground* bg;
  if (!nsCSSRendering::FindBackground(aFrame->PresContext(), aFrame, &bg))
    return;
  if (!bg->HasFixedBackground())
    return;

  mPartHasFixedBackground = PR_TRUE;
}

class nsDisplayTableBorderBackground : public nsDisplayTableItem {
public:
  nsDisplayTableBorderBackground(nsTableFrame* aFrame) : nsDisplayTableItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayTableBorderBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTableBorderBackground() {
    MOZ_COUNT_DTOR(nsDisplayTableBorderBackground);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("TableBorderBackground")
};

void
nsDisplayTableBorderBackground::Paint(nsDisplayListBuilder* aBuilder,
    nsIRenderingContext* aCtx, const nsRect& aDirtyRect)
{
  static_cast<nsTableFrame*>(mFrame)->
    PaintTableBorderBackground(*aCtx, aDirtyRect,
                               aBuilder->ToReferenceFrame(mFrame),
                               aBuilder->GetBackgroundPaintFlags());
}

static PRInt32 GetTablePartRank(nsDisplayItem* aItem)
{
  nsIAtom* type = aItem->GetUnderlyingFrame()->GetType();
  if (type == nsGkAtoms::tableFrame)
    return 0;
  if (type == nsGkAtoms::tableRowGroupFrame)
    return 1;
  if (type == nsGkAtoms::tableRowFrame)
    return 2;
  return 3;
}

static PRBool CompareByTablePartRank(nsDisplayItem* aItem1, nsDisplayItem* aItem2,
                                     void* aClosure)
{
  return GetTablePartRank(aItem1) <= GetTablePartRank(aItem2);
}

 nsresult
nsTableFrame::GenericTraversal(nsDisplayListBuilder* aBuilder, nsFrame* aFrame,
                               const nsRect& aDirtyRect, const nsDisplayListSet& aLists)
{
  
  
  
  
  
  
  
  
  nsIFrame* kid = aFrame->GetFirstChild(nsnull);
  while (kid) {
    nsresult rv = aFrame->BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
    kid = kid->GetNextSibling();
  }
  return NS_OK;
}

 nsresult
nsTableFrame::DisplayGenericTablePart(nsDisplayListBuilder* aBuilder,
                                      nsFrame* aFrame,
                                      const nsRect& aDirtyRect,
                                      const nsDisplayListSet& aLists,
                                      nsDisplayTableItem* aDisplayItem,
                                      DisplayGenericTablePartTraversal aTraversal)
{
  nsDisplayList eventsBorderBackground;
  
  
  PRBool sortEventBackgrounds = aDisplayItem && aBuilder->IsForEventDelivery();
  nsDisplayListCollection separatedCollection;
  const nsDisplayListSet* lists = sortEventBackgrounds ? &separatedCollection : &aLists;
  
  nsAutoPushCurrentTableItem pushTableItem;
  if (aDisplayItem) {
    pushTableItem.Push(aBuilder, aDisplayItem);
  }
  nsDisplayTableItem* currentItem = aBuilder->GetCurrentTableItem();
  NS_ASSERTION(currentItem, "No current table item!");
  currentItem->UpdateForFrameBackground(aFrame);
  
  
  PRBool hasBoxShadow = aFrame->IsVisibleForPainting(aBuilder) &&
                        aFrame->GetStyleBorder()->mBoxShadow;
  if (hasBoxShadow) {
    nsDisplayItem* item = new (aBuilder) nsDisplayBoxShadowOuter(aFrame);
    nsresult rv = lists->BorderBackground()->AppendNewToTop(item);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  if (aBuilder->IsForEventDelivery() &&
      aFrame->IsVisibleForPainting(aBuilder)) {
    nsresult rv = lists->BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayBackground(aFrame));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (hasBoxShadow) {
    nsDisplayItem* item = new (aBuilder) nsDisplayBoxShadowInner(aFrame);
    nsresult rv = lists->BorderBackground()->AppendNewToTop(item);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsresult rv = aTraversal(aBuilder, aFrame, aDirtyRect, *lists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (sortEventBackgrounds) {
    
    
    
    separatedCollection.BorderBackground()->Sort(aBuilder, CompareByTablePartRank, nsnull);
    separatedCollection.MoveTo(aLists);
  }
  
  return aFrame->DisplayOutline(aBuilder, aLists);
}



NS_IMETHODIMP
nsTableFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                               const nsRect&           aDirtyRect,
                               const nsDisplayListSet& aLists)
{
  if (!IsVisibleInSelection(aBuilder))
    return NS_OK;

  DO_GLOBAL_REFLOW_COUNT_DSP_COLOR("nsTableFrame", NS_RGB(255,128,255));

  if (GetStyleVisibility()->IsVisible()) {
    nsMargin deflate = GetDeflationForBackground(PresContext());
    
    
    
    if (deflate.IsZero()) {
      nsresult rv = DisplayBackgroundUnconditional(aBuilder, aLists, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  
  
  
  nsDisplayTableItem* item = new (aBuilder) nsDisplayTableBorderBackground(this);
  nsresult rv = aLists.BorderBackground()->AppendNewToTop(item);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return DisplayGenericTablePart(aBuilder, this, aDirtyRect, aLists, item);
}

nsMargin
nsTableFrame::GetDeflationForBackground(nsPresContext* aPresContext) const
{
  if (eCompatibility_NavQuirks != aPresContext->CompatibilityMode() ||
      !IsBorderCollapse())
    return nsMargin(0,0,0,0);

  return GetOuterBCBorder();
}



void
nsTableFrame::PaintTableBorderBackground(nsIRenderingContext& aRenderingContext,
                                         const nsRect& aDirtyRect,
                                         nsPoint aPt, PRUint32 aBGPaintFlags)
{
  nsPresContext* presContext = PresContext();

  TableBackgroundPainter painter(this, TableBackgroundPainter::eOrigin_Table,
                                 presContext, aRenderingContext,
                                 aDirtyRect, aPt, aBGPaintFlags);
  nsMargin deflate = GetDeflationForBackground(presContext);
  
  
  nsresult rv = painter.PaintTable(this, deflate, !deflate.IsZero());
  if (NS_FAILED(rv)) return;

  if (GetStyleVisibility()->IsVisible()) {
    const nsStyleBorder* border = GetStyleBorder();
    if (!IsBorderCollapse()) {
      PRIntn skipSides = GetSkipSides();
      nsRect rect(aPt, mRect.Size());
      nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                  aDirtyRect, rect, *border, mStyleContext,
                                  skipSides);
    }
    else {
      
      
      nsIRenderingContext::AutoPushTranslation translate(&aRenderingContext, aPt.x, aPt.y);
      PaintBCBorders(aRenderingContext, aDirtyRect - aPt);
    }
  }
}

PRIntn
nsTableFrame::GetSkipSides() const
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
nsTableFrame::SetColumnDimensions(nscoord         aHeight,
                                  const nsMargin& aBorderPadding)
{
  nscoord cellSpacingX = GetCellSpacingX();
  nscoord cellSpacingY = GetCellSpacingY();
  nscoord colHeight = aHeight -= aBorderPadding.top + aBorderPadding.bottom +
                                 2* cellSpacingY;

  nsTableIterator iter(mColGroups); 
  nsIFrame* colGroupFrame = iter.First();
  PRBool tableIsLTR = GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_LTR;
  PRInt32 colX =tableIsLTR ? 0 : PR_MAX(0, GetColCount() - 1);
  PRInt32 tableColIncr = tableIsLTR ? 1 : -1; 
  nsPoint colGroupOrigin(aBorderPadding.left + cellSpacingX,
                         aBorderPadding.top + cellSpacingY);
  while (nsnull != colGroupFrame) {
    nscoord colGroupWidth = 0;
    nsTableIterator iterCol(*colGroupFrame);  
    nsIFrame* colFrame = iterCol.First();
    nsPoint colOrigin(0,0);
    while (nsnull != colFrame) {
      if (NS_STYLE_DISPLAY_TABLE_COLUMN ==
          colFrame->GetStyleDisplay()->mDisplay) {
        NS_ASSERTION(colX < GetColCount(), "invalid number of columns");
        nscoord colWidth = GetColumnWidth(colX);
        nsRect colRect(colOrigin.x, colOrigin.y, colWidth, colHeight);
        colFrame->SetRect(colRect);
        colOrigin.x += colWidth + cellSpacingX;
        colGroupWidth += colWidth + cellSpacingX;
        colX += tableColIncr;
      }
      colFrame = iterCol.Next();      
    }
    if (colGroupWidth) {
      colGroupWidth -= cellSpacingX;
    }

    nsRect colGroupRect(colGroupOrigin.x, colGroupOrigin.y, colGroupWidth, colHeight);
    colGroupFrame->SetRect(colGroupRect);
    colGroupFrame = iter.Next();
    colGroupOrigin.x += colGroupWidth + cellSpacingX;
  }
}





void
nsTableFrame::ProcessRowInserted(nscoord aNewHeight)
{
  SetRowInserted(PR_FALSE); 
  nsTableFrame::RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);
  
  for (PRUint32 rgX = 0; rgX < rowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
    NS_ASSERTION(rgFrame, "Must have rgFrame here");
    nsIFrame* childFrame = rgFrame->GetFirstChild(nsnull);
    
    while (childFrame) {
      nsTableRowFrame *rowFrame = do_QueryFrame(childFrame);
      if (rowFrame) {
        if (rowFrame->IsFirstInserted()) {
          rowFrame->SetFirstInserted(PR_FALSE);
          
          nscoord damageY = rgFrame->GetPosition().y + rowFrame->GetPosition().y;
          nsRect damageRect(0, damageY, GetSize().width, aNewHeight - damageY);

          Invalidate(damageRect);
          
          SetRowInserted(PR_FALSE);
          return; 
        }
      }
      childFrame = childFrame->GetNextSibling();
    }
  }
}

 void
nsTableFrame::MarkIntrinsicWidthsDirty()
{
  LayoutStrategy()->MarkIntrinsicWidthsDirty();

  

  nsHTMLContainerFrame::MarkIntrinsicWidthsDirty();
}

 nscoord
nsTableFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  if (NeedToCalcBCBorders())
    CalcBCBorders();

  ReflowColGroups(aRenderingContext);

  return LayoutStrategy()->GetMinWidth(aRenderingContext);
}

 nscoord
nsTableFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  if (NeedToCalcBCBorders())
    CalcBCBorders();

  ReflowColGroups(aRenderingContext);

  return LayoutStrategy()->GetPrefWidth(aRenderingContext, PR_FALSE);
}

 nsIFrame::IntrinsicWidthOffsetData
nsTableFrame::IntrinsicWidthOffsets(nsIRenderingContext* aRenderingContext)
{
  IntrinsicWidthOffsetData result =
    nsHTMLContainerFrame::IntrinsicWidthOffsets(aRenderingContext);

  if (IsBorderCollapse()) {
    result.hPadding = 0;
    result.hPctPadding = 0;

    nsMargin outerBC = GetIncludedOuterBCBorder();
    result.hBorder = outerBC.LeftRight();
  }

  return result;
}

 nsSize
nsTableFrame::ComputeSize(nsIRenderingContext *aRenderingContext,
                          nsSize aCBSize, nscoord aAvailableWidth,
                          nsSize aMargin, nsSize aBorder, nsSize aPadding,
                          PRBool aShrinkWrap)
{
  nsSize result =
    nsHTMLContainerFrame::ComputeSize(aRenderingContext, aCBSize,
                                      aAvailableWidth,
                                      aMargin, aBorder, aPadding, aShrinkWrap);

  
  nscoord minWidth = GetMinWidth(aRenderingContext);
  if (minWidth > result.width)
    result.width = minWidth;

  return result;
}

nscoord
nsTableFrame::TableShrinkWidthToFit(nsIRenderingContext *aRenderingContext,
                                    nscoord aWidthInCB)
{
  nscoord result;
  nscoord minWidth = GetMinWidth(aRenderingContext);
  if (minWidth > aWidthInCB) {
    result = minWidth;
  } else {
    
    
    
    
    
    
    
    nscoord prefWidth =
      LayoutStrategy()->GetPrefWidth(aRenderingContext, PR_TRUE);
    if (prefWidth > aWidthInCB) {
      result = aWidthInCB;
    } else {
      result = prefWidth;
    }
  }
  return result;
}

 nsSize
nsTableFrame::ComputeAutoSize(nsIRenderingContext *aRenderingContext,
                              nsSize aCBSize, nscoord aAvailableWidth,
                              nsSize aMargin, nsSize aBorder, nsSize aPadding,
                              PRBool aShrinkWrap)
{
  
  nscoord cbBased = aAvailableWidth - aMargin.width - aBorder.width -
                    aPadding.width;
  return nsSize(TableShrinkWidthToFit(aRenderingContext, cbBased),
                NS_UNCONSTRAINEDSIZE);
}



PRBool
nsTableFrame::AncestorsHaveStyleHeight(const nsHTMLReflowState& aParentReflowState)
{
  for (const nsHTMLReflowState* rs = &aParentReflowState;
       rs && rs->frame; rs = rs->parentReflowState) {
    nsIAtom* frameType = rs->frame->GetType();
    if (IS_TABLE_CELL(frameType)                     ||
        (nsGkAtoms::tableRowFrame      == frameType) ||
        (nsGkAtoms::tableRowGroupFrame == frameType)) {
      if (rs->mStylePosition->mHeight.GetUnit() != eStyleUnit_Auto) {
        return PR_TRUE;
      }
    }
    else if (nsGkAtoms::tableFrame == frameType) {
      
      if (rs->mStylePosition->mHeight.GetUnit() != eStyleUnit_Auto) {
        return PR_TRUE;
      }
      else return PR_FALSE;
    }
  }
  return PR_FALSE;
}


void
nsTableFrame::CheckRequestSpecialHeightReflow(const nsHTMLReflowState& aReflowState)
{
  if (!aReflowState.frame->GetPrevInFlow() &&  
      (NS_UNCONSTRAINEDSIZE == aReflowState.ComputedHeight() ||  
       0                    == aReflowState.ComputedHeight()) && 
      eStyleUnit_Percent == aReflowState.mStylePosition->mHeight.GetUnit() && 
      nsTableFrame::AncestorsHaveStyleHeight(*aReflowState.parentReflowState)) {
    nsTableFrame::RequestSpecialHeightReflow(aReflowState);
  }
}






void
nsTableFrame::RequestSpecialHeightReflow(const nsHTMLReflowState& aReflowState)
{
  
  for (const nsHTMLReflowState* rs = &aReflowState; rs && rs->frame; rs = rs->parentReflowState) {
    nsIAtom* frameType = rs->frame->GetType();
    NS_ASSERTION(IS_TABLE_CELL(frameType) ||
                 nsGkAtoms::tableRowFrame == frameType ||
                 nsGkAtoms::tableRowGroupFrame == frameType ||
                 nsGkAtoms::scrollFrame == frameType ||
                 nsGkAtoms::tableFrame == frameType,
                 "unexpected frame type");
                 
    rs->frame->AddStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
    if (nsGkAtoms::tableFrame == frameType) {
      NS_ASSERTION(rs != &aReflowState,
                   "should not request special height reflow for table");
      
      break;
    }
  }
}





























































NS_METHOD nsTableFrame::Reflow(nsPresContext*           aPresContext,
                               nsHTMLReflowMetrics&     aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsTableFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  PRBool isPaginated = aPresContext->IsPaginated();

  aStatus = NS_FRAME_COMPLETE; 
  if (!GetPrevInFlow() && !mTableLayoutStrategy) {
    NS_ASSERTION(PR_FALSE, "strategy should have been created in Init");
    return NS_ERROR_NULL_POINTER;
  }
  nsresult rv = NS_OK;

  
  if (!GetPrevInFlow() && IsBorderCollapse() && NeedToCalcBCBorders()) {
    CalcBCBorders();
  }

  aDesiredSize.width = aReflowState.availableWidth;

  
  MoveOverflowToChildList(aPresContext);

  PRBool haveDesiredHeight = PR_FALSE;
  SetHaveReflowedColGroups(PR_FALSE);

  
  
  
  
  if (NS_SUBTREE_DIRTY(this) ||
      aReflowState.ShouldReflowAllKids() ||
      IsGeometryDirty() ||
      aReflowState.mFlags.mVResize) {

    if (aReflowState.ComputedHeight() != NS_UNCONSTRAINEDSIZE ||
        
        
        
        
        aReflowState.mFlags.mVResize) {
      
      
      
      
      
      
      
      SetGeometryDirty();
    }

    PRBool needToInitiateSpecialReflow =
      !!(GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
    
    if (isPaginated && !GetPrevInFlow() && (NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight)) {
      nscoord tableSpecifiedHeight = CalcBorderBoxHeight(aReflowState);
      if ((tableSpecifiedHeight > 0) && 
          (tableSpecifiedHeight != NS_UNCONSTRAINEDSIZE)) {
        needToInitiateSpecialReflow = PR_TRUE;
      }
    }
    nsIFrame* lastChildReflowed = nsnull;

    NS_ASSERTION(!aReflowState.mFlags.mSpecialHeightReflow,
                 "Shouldn't be in special height reflow here!");

    
    
    

    
    
    nscoord availHeight = needToInitiateSpecialReflow 
                          ? NS_UNCONSTRAINEDSIZE : aReflowState.availableHeight;

    ReflowTable(aDesiredSize, aReflowState, availHeight,
                lastChildReflowed, aStatus);

    
    if (GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT)
      needToInitiateSpecialReflow = PR_TRUE;

    
    if (needToInitiateSpecialReflow && NS_FRAME_IS_COMPLETE(aStatus)) {
      

      nsHTMLReflowState &mutable_rs =
        const_cast<nsHTMLReflowState&>(aReflowState);

      
      CalcDesiredHeight(aReflowState, aDesiredSize); 
      mutable_rs.mFlags.mSpecialHeightReflow = PR_TRUE;

      ReflowTable(aDesiredSize, aReflowState, aReflowState.availableHeight, 
                  lastChildReflowed, aStatus);

      if (lastChildReflowed && NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
        
        nsMargin borderPadding = GetChildAreaOffset(&aReflowState);
        aDesiredSize.height = borderPadding.bottom + GetCellSpacingY() +
                              lastChildReflowed->GetRect().YMost();
      }
      haveDesiredHeight = PR_TRUE;

      mutable_rs.mFlags.mSpecialHeightReflow = PR_FALSE;
    }
  }
  else {
    
    for (nsIFrame* kid = GetFirstChild(nsnull); kid; kid = kid->GetNextSibling()) {
      ConsiderChildOverflow(aDesiredSize.mOverflowArea, kid);
    }
  }

  aDesiredSize.width = aReflowState.ComputedWidth() +
                       aReflowState.mComputedBorderPadding.LeftRight();
  if (!haveDesiredHeight) {
    CalcDesiredHeight(aReflowState, aDesiredSize); 
  }
  if (IsRowInserted()) {
    ProcessRowInserted(aDesiredSize.height);
  }

  nsMargin borderPadding = GetChildAreaOffset(&aReflowState);
  SetColumnDimensions(aDesiredSize.height, borderPadding);
  if (NeedToCollapse() &&
      (NS_UNCONSTRAINEDSIZE != aReflowState.availableWidth)) {
    AdjustForCollapsingRowsCols(aDesiredSize, borderPadding);
  }

  
  nsRect tableRect(0, 0, aDesiredSize.width, aDesiredSize.height) ;
  
  if (!aReflowState.mStyleDisplay->IsTableClip()) {
    
    nsMargin bcMargin = GetExcludedOuterBCBorder();
    tableRect.Inflate(bcMargin);
  }
  aDesiredSize.mOverflowArea.UnionRect(aDesiredSize.mOverflowArea, tableRect);
  
  if (GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    
    Invalidate(aDesiredSize.mOverflowArea);
  } else {
    CheckInvalidateSizeChange(aDesiredSize);
  }

  FinishAndStoreOverflow(&aDesiredSize);
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return rv;
}

nsresult 
nsTableFrame::ReflowTable(nsHTMLReflowMetrics&     aDesiredSize,
                          const nsHTMLReflowState& aReflowState,
                          nscoord                  aAvailHeight,
                          nsIFrame*&               aLastChildReflowed,
                          nsReflowStatus&          aStatus)
{
  nsresult rv = NS_OK;
  aLastChildReflowed = nsnull;

  if (!GetPrevInFlow()) {
    mTableLayoutStrategy->ComputeColumnWidths(aReflowState);
  }
  
  
  aDesiredSize.width = aReflowState.ComputedWidth() +
                       aReflowState.mComputedBorderPadding.LeftRight();
  nsTableReflowState reflowState(*PresContext(), aReflowState, *this,
                                 aDesiredSize.width, aAvailHeight);
  ReflowChildren(reflowState, aStatus, aLastChildReflowed,
                 aDesiredSize.mOverflowArea);

  ReflowColGroups(aReflowState.rendContext);
  return rv;
}

nsIFrame*
nsTableFrame::GetFirstBodyRowGroupFrame()
{
  nsIFrame* headerFrame = nsnull;
  nsIFrame* footerFrame = nsnull;

  for (nsIFrame* kidFrame = mFrames.FirstChild(); nsnull != kidFrame; ) {
    const nsStyleDisplay* childDisplay = kidFrame->GetStyleDisplay();

    
    
    if (NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == childDisplay->mDisplay) {
      if (headerFrame) {
        
        
        return kidFrame;
      }
      headerFrame = kidFrame;
    
    } else if (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == childDisplay->mDisplay) {
      if (footerFrame) {
        
        
        return kidFrame;
      }
      footerFrame = kidFrame;

    } else if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == childDisplay->mDisplay) {
      return kidFrame;
    }

    
    kidFrame = kidFrame->GetNextSibling();
  }

  return nsnull;
}



void
nsTableFrame::PushChildren(const FrameArray& aFrames,
                           PRInt32 aPushFrom)
{
  NS_PRECONDITION(aPushFrom > 0, "pushing first child");

  
  nsFrameList frames;
  nsIFrame* lastFrame = nsnull;
  PRUint32 childX;
  nsIFrame* prevSiblingHint = aFrames.SafeElementAt(aPushFrom - 1);
  for (childX = aPushFrom; childX < aFrames.Length(); ++childX) {
    nsIFrame* f = aFrames[childX];
    
    
    
    
    nsTableRowGroupFrame* rgFrame = GetRowGroupFrame(f);
    NS_ASSERTION(rgFrame, "Unexpected non-row-group frame");
    if (!rgFrame || !rgFrame->IsRepeatable()) {
      mFrames.RemoveFrame(f, prevSiblingHint);
      frames.InsertFrame(nsnull, lastFrame, f);
      lastFrame = f;
    }
  }

  if (nsnull != GetNextInFlow()) {
    nsTableFrame* nextInFlow = (nsTableFrame*)GetNextInFlow();

    
    nsIFrame* firstBodyFrame = nextInFlow->GetFirstBodyRowGroupFrame();
    nsIFrame* prevSibling = nsnull;
    if (firstBodyFrame) {
      prevSibling = nextInFlow->mFrames.GetPrevSiblingFor(firstBodyFrame);
    }
    
    
    ReparentFrameViewList(PresContext(), frames, this, nextInFlow);
    nextInFlow->mFrames.InsertFrames(nextInFlow, prevSibling,
                                     frames);
  }
  else if (frames.NotEmpty()) {
    
    SetOverflowFrames(PresContext(), frames);
  }
}



void
nsTableFrame::AdjustForCollapsingRowsCols(nsHTMLReflowMetrics& aDesiredSize,
                                          nsMargin             aBorderPadding)
{
  nscoord yTotalOffset = 0; 

  
  
  SetNeedToCollapse(PR_FALSE);

  
  
  RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);
  
  nsTableFrame* firstInFlow = static_cast<nsTableFrame*> (GetFirstInFlow());
  nscoord width = firstInFlow->GetCollapsedWidth(aBorderPadding);
  nscoord rgWidth = width - 2 * GetCellSpacingX();
  nsRect overflowArea(0, 0, 0, 0);
  
  for (PRUint32 childX = 0; childX < rowGroups.Length(); childX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[childX];
    NS_ASSERTION(rgFrame, "Must have row group frame here");
    yTotalOffset += rgFrame->CollapseRowGroupIfNecessary(yTotalOffset, rgWidth);
    ConsiderChildOverflow(overflowArea, rgFrame);
  } 

  aDesiredSize.height -= yTotalOffset;
  aDesiredSize.width   = width;
  overflowArea.UnionRect(nsRect(0, 0, aDesiredSize.width, aDesiredSize.height),
                         overflowArea);
  FinishAndStoreOverflow(&overflowArea,
                         nsSize(aDesiredSize.width, aDesiredSize.height));
}


nscoord
nsTableFrame::GetCollapsedWidth(nsMargin aBorderPadding)
{
  NS_ASSERTION(!GetPrevInFlow(), "GetCollapsedWidth called on next in flow");
  nscoord cellSpacingX = GetCellSpacingX();
  nscoord width = cellSpacingX;
  width += aBorderPadding.left + aBorderPadding.right;
  for (nsIFrame* groupFrame = mColGroups.FirstChild(); groupFrame;
         groupFrame = groupFrame->GetNextSibling()) {
    const nsStyleVisibility* groupVis = groupFrame->GetStyleVisibility();
    PRBool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE == groupVis->mVisible);
    nsTableColGroupFrame* cgFrame = (nsTableColGroupFrame*)groupFrame;
    for (nsTableColFrame* colFrame = cgFrame->GetFirstColumn(); colFrame;
         colFrame = colFrame->GetNextCol()) {
      const nsStyleDisplay* colDisplay = colFrame->GetStyleDisplay();
      PRInt32 colX = colFrame->GetColIndex();
      if (NS_STYLE_DISPLAY_TABLE_COLUMN == colDisplay->mDisplay) {
        const nsStyleVisibility* colVis = colFrame->GetStyleVisibility();
        PRBool collapseCol = (NS_STYLE_VISIBILITY_COLLAPSE == colVis->mVisible);
        PRInt32 colWidth = GetColumnWidth(colX);
        if (!collapseGroup && !collapseCol) {
          width += colWidth;
          if (ColumnHasCellSpacingBefore(colX))
            width += cellSpacingX;
        }
        else {
          SetNeedToCollapse(PR_TRUE);
        }
      }
    }
  }
  return width;
}

 void
nsTableFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
   if (!aOldStyleContext) 
     return;
   
   if (IsBorderCollapse() &&
       BCRecalcNeeded(aOldStyleContext, GetStyleContext())) {
     nsRect damageArea(0, 0, GetColCount(), GetRowCount());
     SetBCDamageArea(damageArea);
   }

   
   if (!mTableLayoutStrategy || GetPrevInFlow())
     return;
     
   PRBool isAuto = IsAutoLayout();
   if (isAuto != (LayoutStrategy()->GetType() == nsITableLayoutStrategy::Auto)) {
     nsITableLayoutStrategy* temp;
     if (isAuto)
       temp = new BasicTableLayoutStrategy(this);
     else
       temp = new FixedTableLayoutStrategy(this);
     
     if (temp) {
       delete mTableLayoutStrategy;
       mTableLayoutStrategy = temp;
     }
  }
}



NS_IMETHODIMP
nsTableFrame::AppendFrames(nsIAtom*        aListName,
                           nsFrameList&    aFrameList)
{
  NS_ASSERTION(!aListName || aListName == nsGkAtoms::colGroupList,
               "unexpected child list");

  
  
  
  
  while (!aFrameList.IsEmpty()) {
    nsIFrame* f = aFrameList.FirstChild();
    aFrameList.RemoveFrame(f);

    
    const nsStyleDisplay* display = f->GetStyleDisplay();

    if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == display->mDisplay) {
      nsTableColGroupFrame* lastColGroup =
        nsTableColGroupFrame::GetLastRealColGroup(this);
      PRInt32 startColIndex = (lastColGroup) 
        ? lastColGroup->GetStartColumnIndex() + lastColGroup->GetColCount() : 0;
      mColGroups.InsertFrame(nsnull, lastColGroup, f);
      
      InsertColGroups(startColIndex,
                      nsFrameList::Slice(mColGroups, f, f->GetNextSibling()));
    } else if (IsRowGroup(display->mDisplay)) {
      
      mFrames.AppendFrame(nsnull, f);

      
      InsertRowGroups(nsFrameList::Slice(mFrames, f, nsnull));
    } else {
      
      NS_NOTREACHED("How did we get here?  Frame construction screwed up");
      mFrames.AppendFrame(nsnull, f);
    }
  }

#ifdef DEBUG_TABLE_CELLMAP
  printf("=== TableFrame::AppendFrames\n");
  Dump(PR_TRUE, PR_TRUE, PR_TRUE);
#endif
  PresContext()->PresShell()->FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
  SetGeometryDirty();

  return NS_OK;
}

NS_IMETHODIMP
nsTableFrame::InsertFrames(nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsFrameList&    aFrameList)
{
  
  
  
  
  
 
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  if ((aPrevFrame && !aPrevFrame->GetNextSibling()) ||
      (!aPrevFrame && GetChildList(aListName).IsEmpty())) {
    
    return AppendFrames(aListName, aFrameList);
  }

  
  const nsStyleDisplay* display = aFrameList.FirstChild()->GetStyleDisplay();
#ifdef DEBUG
  
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    const nsStyleDisplay* nextDisplay = e.get()->GetStyleDisplay();
    NS_ASSERTION((display->mDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP) ==
        (nextDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP),
      "heterogenous childlist");  
  }
#endif  
  if (aPrevFrame) {
    const nsStyleDisplay* prevDisplay = aPrevFrame->GetStyleDisplay();
    
    if ((display->mDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP) !=
        (prevDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP)) {
      
      
      
      nsIFrame* pseudoFrame = aFrameList.FirstChild();
      nsIContent* parentContent = GetContent();
      nsIContent* content;
      aPrevFrame = nsnull;
      while (pseudoFrame  && (parentContent ==
                              (content = pseudoFrame->GetContent()))) {
        pseudoFrame = pseudoFrame->GetFirstChild(nsnull);
      }
      nsCOMPtr<nsIContent> container = content->GetParent();
      if (NS_LIKELY(container)) { 
        PRInt32 newIndex = container->IndexOf(content);
        nsIFrame* kidFrame;
        PRBool isColGroup = (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP ==
                             display->mDisplay);
        nsTableColGroupFrame* lastColGroup;
        if (isColGroup) {
          kidFrame = mColGroups.FirstChild();
          lastColGroup = nsTableColGroupFrame::GetLastRealColGroup(this);
        }
        else {
          kidFrame = mFrames.FirstChild();
        }
        
        PRInt32 lastIndex = -1;
        while (kidFrame) {
          if (isColGroup) {
            if (kidFrame == lastColGroup) {
              aPrevFrame = kidFrame; 
              break;
            }
          }
          pseudoFrame = kidFrame;
          while (pseudoFrame  && (parentContent ==
                                  (content = pseudoFrame->GetContent()))) {
            pseudoFrame = pseudoFrame->GetFirstChild(nsnull);
          }
          PRInt32 index = container->IndexOf(content);
          if (index > lastIndex && index < newIndex) {
            lastIndex = index;
            aPrevFrame = kidFrame;
          }
          kidFrame = kidFrame->GetNextSibling();
        }
      }
    }
  }
  if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == display->mDisplay) {
    NS_ASSERTION(!aListName || aListName == nsGkAtoms::colGroupList,
                 "unexpected child list");
    
    const nsFrameList::Slice& newColgroups =
      mColGroups.InsertFrames(nsnull, aPrevFrame, aFrameList);
    
    PRInt32 startColIndex = 0;
    if (aPrevFrame) {
      nsTableColGroupFrame* prevColGroup = 
        (nsTableColGroupFrame*)GetFrameAtOrBefore(this, aPrevFrame,
                                                  nsGkAtoms::tableColGroupFrame);
      if (prevColGroup) {
        startColIndex = prevColGroup->GetStartColumnIndex() + prevColGroup->GetColCount();
      }
    }
    InsertColGroups(startColIndex, newColgroups);
  } else if (IsRowGroup(display->mDisplay)) {
    NS_ASSERTION(!aListName, "unexpected child list");
    
    const nsFrameList::Slice& newRowGroups =
      mFrames.InsertFrames(nsnull, aPrevFrame, aFrameList);

    InsertRowGroups(newRowGroups);
  } else {
    NS_ASSERTION(!aListName, "unexpected child list");
    NS_NOTREACHED("How did we even get here?");
    
    mFrames.InsertFrames(nsnull, aPrevFrame, aFrameList);
    return NS_OK;
  }

  PresContext()->PresShell()->FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
  SetGeometryDirty();
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== TableFrame::InsertFrames\n");
  Dump(PR_TRUE, PR_TRUE, PR_TRUE);
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsTableFrame::RemoveFrame(nsIAtom*        aListName,
                          nsIFrame*       aOldFrame)
{
  
  const nsStyleDisplay* display = aOldFrame->GetStyleDisplay();

  
  
  if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == display->mDisplay) {
    NS_ASSERTION(!aListName || aListName == nsGkAtoms::colGroupList,
                 "unexpected child list");
    nsIFrame* nextColGroupFrame = aOldFrame->GetNextSibling();
    nsTableColGroupFrame* colGroup = (nsTableColGroupFrame*)aOldFrame;
    PRInt32 firstColIndex = colGroup->GetStartColumnIndex();
    PRInt32 lastColIndex  = firstColIndex + colGroup->GetColCount() - 1;
    mColGroups.DestroyFrame(aOldFrame);
    nsTableColGroupFrame::ResetColIndices(nextColGroupFrame, firstColIndex);
    
    PRInt32 colX;
    for (colX = lastColIndex; colX >= firstColIndex; colX--) {
      nsTableColFrame* colFrame = mColFrames.SafeElementAt(colX);
      if (colFrame) {
        RemoveCol(colGroup, colX, PR_TRUE, PR_FALSE);
      }
    }

    PRInt32 numAnonymousColsToAdd = GetColCount() - mColFrames.Length();
    if (numAnonymousColsToAdd > 0) {
      
      AppendAnonymousColFrames(numAnonymousColsToAdd);
    }

  } else {
    NS_ASSERTION(!aListName, "unexpected child list");
    nsTableRowGroupFrame* rgFrame = GetRowGroupFrame(aOldFrame);
    if (rgFrame) {
      
      nsTableCellMap* cellMap = GetCellMap();
      if (cellMap) {
        cellMap->RemoveGroupCellMap(rgFrame);
      }

       
      mFrames.DestroyFrame(aOldFrame);
     
      
      if (cellMap) {
        cellMap->Synchronize(this);
        
        ResetRowIndices(nsFrameList::Slice(mFrames, nsnull, nsnull));
        nsRect damageArea;
        cellMap->RebuildConsideringCells(nsnull, nsnull, 0, 0, PR_FALSE, damageArea);
      }

      MatchCellMapToColCache(cellMap);
    } else {
      
      mFrames.DestroyFrame(aOldFrame);
    }
  }
  
  
  if (IsBorderCollapse()) {
    nsRect damageArea(0, 0, PR_MAX(1, GetColCount()), PR_MAX(1, GetRowCount()));
    SetBCDamageArea(damageArea);
  }
  PresContext()->PresShell()->FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
  SetGeometryDirty();
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== TableFrame::RemoveFrame\n");
  Dump(PR_TRUE, PR_TRUE, PR_TRUE);
#endif
  return NS_OK;
}

 nsMargin
nsTableFrame::GetUsedBorder() const
{
  if (!IsBorderCollapse())
    return nsHTMLContainerFrame::GetUsedBorder();
  
  return GetIncludedOuterBCBorder();
}

 nsMargin
nsTableFrame::GetUsedPadding() const
{
  if (!IsBorderCollapse())
    return nsHTMLContainerFrame::GetUsedPadding();

  return nsMargin(0,0,0,0);
}

static void
DivideBCBorderSize(nscoord  aPixelSize,
                   nscoord& aSmallHalf,
                   nscoord& aLargeHalf)
{
  aSmallHalf = aPixelSize / 2;
  aLargeHalf = aPixelSize - aSmallHalf;
}

nsMargin
nsTableFrame::GetOuterBCBorder() const
{
  if (NeedToCalcBCBorders())
    const_cast<nsTableFrame*>(this)->CalcBCBorders();

  nsMargin border(0, 0, 0, 0);
  PRInt32 p2t = nsPresContext::AppUnitsPerCSSPixel();
  BCPropertyData* propData = 
    (BCPropertyData*)nsTableFrame::GetProperty((nsIFrame*)this, nsGkAtoms::tableBCProperty, PR_FALSE);
  if (propData) {
    border.top    = BC_BORDER_TOP_HALF_COORD(p2t, propData->mTopBorderWidth);
    border.right  = BC_BORDER_RIGHT_HALF_COORD(p2t, propData->mRightBorderWidth);
    border.bottom = BC_BORDER_BOTTOM_HALF_COORD(p2t, propData->mBottomBorderWidth);
    border.left   = BC_BORDER_LEFT_HALF_COORD(p2t, propData->mLeftBorderWidth);
  }
  return border;
}

nsMargin
nsTableFrame::GetIncludedOuterBCBorder() const
{
  if (NeedToCalcBCBorders())
    const_cast<nsTableFrame*>(this)->CalcBCBorders();

  nsMargin border(0, 0, 0, 0);
  PRInt32 p2t = nsPresContext::AppUnitsPerCSSPixel();
  BCPropertyData* propData =
    (BCPropertyData*)nsTableFrame::GetProperty((nsIFrame*)this,
                                                nsGkAtoms::tableBCProperty,
                                                PR_FALSE);
  if (propData) {
    border.top += BC_BORDER_TOP_HALF_COORD(p2t, propData->mTopBorderWidth);
    border.right += BC_BORDER_RIGHT_HALF_COORD(p2t, propData->mRightCellBorderWidth);
    border.bottom += BC_BORDER_BOTTOM_HALF_COORD(p2t, propData->mBottomBorderWidth);
    border.left += BC_BORDER_LEFT_HALF_COORD(p2t, propData->mLeftCellBorderWidth);
  }
  return border;
}

nsMargin
nsTableFrame::GetExcludedOuterBCBorder() const
{
  return GetOuterBCBorder() - GetIncludedOuterBCBorder();
}

static
void GetSeparateModelBorderPadding(const nsHTMLReflowState* aReflowState,
                                   nsStyleContext&          aStyleContext,
                                   nsMargin&                aBorderPadding)
{
  
  
  
  const nsStyleBorder* border = aStyleContext.GetStyleBorder();
  aBorderPadding = border->GetActualBorder();
  if (aReflowState) {
    aBorderPadding += aReflowState->mComputedPadding;
  }
}

nsMargin 
nsTableFrame::GetChildAreaOffset(const nsHTMLReflowState* aReflowState) const
{
  nsMargin offset(0,0,0,0);
  if (IsBorderCollapse()) {
    offset = GetIncludedOuterBCBorder();
  }
  else {
    GetSeparateModelBorderPadding(aReflowState, *mStyleContext, offset);
  }
  return offset;
}

void
nsTableFrame::InitChildReflowState(nsHTMLReflowState& aReflowState)                                    
{
  nsMargin collapseBorder;
  nsMargin padding(0,0,0,0);
  nsMargin* pCollapseBorder = nsnull;
  nsPresContext* presContext = PresContext();
  if (IsBorderCollapse()) {
    nsTableRowGroupFrame* rgFrame = GetRowGroupFrame(aReflowState.frame);
    if (rgFrame) {
      pCollapseBorder = rgFrame->GetBCBorderWidth(collapseBorder);
    }
  }
  aReflowState.Init(presContext, -1, -1, pCollapseBorder, &padding);

  NS_ASSERTION(!mBits.mResizedColumns ||
               !aReflowState.parentReflowState->mFlags.mSpecialHeightReflow,
               "should not resize columns on special height reflow");
  if (mBits.mResizedColumns) {
    aReflowState.mFlags.mHResize = PR_TRUE;
  }
}



void nsTableFrame::PlaceChild(nsTableReflowState&  aReflowState,
                              nsIFrame*            aKidFrame,
                              nsHTMLReflowMetrics& aKidDesiredSize,
                              const nsRect&        aOriginalKidRect,
                              const nsRect&        aOriginalKidOverflowRect)
{
  PRBool isFirstReflow =
    (aKidFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;
  
  
  FinishReflowChild(aKidFrame, PresContext(), nsnull, aKidDesiredSize,
                    aReflowState.x, aReflowState.y, 0);

  InvalidateFrame(aKidFrame, aOriginalKidRect, aOriginalKidOverflowRect,
                  isFirstReflow);

  
  aReflowState.y += aKidDesiredSize.height;

  
  if (NS_UNCONSTRAINEDSIZE != aReflowState.availSize.height) {
    aReflowState.availSize.height -= aKidDesiredSize.height;
  }
}

void
nsTableFrame::OrderRowGroups(RowGroupArray& aChildren) const
{
  aChildren.Clear();
  nsTableRowGroupFrame* head = nsnull;
  nsTableRowGroupFrame* foot = nsnull;
  
  nsIFrame* kidFrame = mFrames.FirstChild();
  while (kidFrame) {
    const nsStyleDisplay* kidDisplay = kidFrame->GetStyleDisplay();
    nsTableRowGroupFrame* rowGroup = GetRowGroupFrame(kidFrame);
    if (NS_LIKELY(rowGroup)) {
      switch(kidDisplay->mDisplay) {
      case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
        if (head) { 
          aChildren.AppendElement(rowGroup);
        }
        else {
          head = rowGroup;
        }
        break;
      case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
        if (foot) { 
          aChildren.AppendElement(rowGroup);
        }
        else {
          foot = rowGroup;
        }
        break;
      case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
        aChildren.AppendElement(rowGroup);
        break;
      default:
        NS_NOTREACHED("How did this produce an nsTableRowGroupFrame?");
        
        break;
      }
    }
    
    
    while (kidFrame) {
      nsIFrame* nif = kidFrame->GetNextInFlow();
      kidFrame = kidFrame->GetNextSibling();
      if (kidFrame != nif) 
        break;
    }
  }

  
  if (head) {
    aChildren.InsertElementAt(0, head);
  }

  
  if (foot) {
    aChildren.AppendElement(foot);
  }
}

PRUint32
nsTableFrame::OrderRowGroups(FrameArray& aChildren,
                             nsTableRowGroupFrame** aHead,
                             nsTableRowGroupFrame** aFoot) const
{
  aChildren.Clear();
  
  *aHead = nsnull;
  *aFoot = nsnull;

  FrameArray nonRowGroups;

  nsIFrame* head = nsnull;
  nsIFrame* foot = nsnull;
  
  nsIFrame* kidFrame = mFrames.FirstChild();
  while (kidFrame) {
    const nsStyleDisplay* kidDisplay = kidFrame->GetStyleDisplay();
    nsTableRowGroupFrame* rowGroup = GetRowGroupFrame(kidFrame);
    if (NS_LIKELY(rowGroup)) {
      switch(kidDisplay->mDisplay) {
      case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
        if (head) { 
          aChildren.AppendElement(kidFrame);
        }
        else {
          head = kidFrame;
          *aHead = rowGroup;
        }
        break;
      case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
        if (foot) { 
          aChildren.AppendElement(kidFrame);
        }
        else {
          foot = kidFrame;
          *aFoot = rowGroup;
        }
        break;
      case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
        aChildren.AppendElement(kidFrame);
        break;
      default:
        break;
      }
    } else {
      NS_NOTREACHED("Non-row-group primary frame list child of an "
                    "nsTableFrame?  How come?");
      nonRowGroups.AppendElement(kidFrame);
    }

    
    
    while (kidFrame) {
      nsIFrame* nif = kidFrame->GetNextInFlow();
      kidFrame = kidFrame->GetNextSibling();
      if (kidFrame != nif) 
        break;
    }
  }
  
  
  if (head) {
    aChildren.InsertElementAt(0, head);
  }

  
  if (foot) {
    aChildren.AppendElement(foot);
  }

  PRUint32 rowGroupCount = aChildren.Length();
  aChildren.AppendElements(nonRowGroups);

  return rowGroupCount;
}

nsTableRowGroupFrame*
nsTableFrame::GetTHead() const
{
  nsIFrame* kidFrame = mFrames.FirstChild();
  while (kidFrame) {
    if (kidFrame->GetStyleDisplay()->mDisplay ==
          NS_STYLE_DISPLAY_TABLE_HEADER_GROUP) {
      nsTableRowGroupFrame* rg = GetRowGroupFrame(kidFrame);
      if (rg) {
        return rg;
      }
    }

    
    
    while (kidFrame) {
      nsIFrame* nif = kidFrame->GetNextInFlow();
      kidFrame = kidFrame->GetNextSibling();
      if (kidFrame != nif) 
        break;
    }
  }

  return nsnull;
}

nsTableRowGroupFrame*
nsTableFrame::GetTFoot() const
{
  nsIFrame* kidFrame = mFrames.FirstChild();
  while (kidFrame) {
    if (kidFrame->GetStyleDisplay()->mDisplay ==
          NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP) {
      nsTableRowGroupFrame* rg = GetRowGroupFrame(kidFrame);
      if (rg) {
        return rg;
      }
    }

    
    
    while (kidFrame) {
      nsIFrame* nif = kidFrame->GetNextInFlow();
      kidFrame = kidFrame->GetNextSibling();
      if (kidFrame != nif) 
        break;
    }
  }

  return nsnull;
}

static PRBool
IsRepeatable(nscoord aFrameHeight, nscoord aPageHeight)
{
  return aFrameHeight < (aPageHeight / 4);
}

nsresult
nsTableFrame::SetupHeaderFooterChild(const nsTableReflowState& aReflowState,
                                     nsTableRowGroupFrame* aFrame,
                                     nscoord* aDesiredHeight)
{
  nsPresContext* presContext = PresContext();
  nscoord pageHeight = presContext->GetPageSize().height;

  if (aFrame->GetParent() != this || pageHeight == NS_UNCONSTRAINEDSIZE) {
    
    
    *aDesiredHeight = 0;
    return NS_OK;
  }

  
  nsHTMLReflowState kidReflowState(presContext, aReflowState.reflowState,
                                   aFrame,
                                   nsSize(aReflowState.availSize.width, NS_UNCONSTRAINEDSIZE),
                                   -1, -1, PR_FALSE);
  InitChildReflowState(kidReflowState);
  kidReflowState.mFlags.mIsTopOfPage = PR_TRUE;
  nsHTMLReflowMetrics desiredSize;
  desiredSize.width = desiredSize.height = 0;
  nsReflowStatus status;
  nsresult rv = ReflowChild(aFrame, presContext, desiredSize, kidReflowState,
                            aReflowState.x, aReflowState.y, 0, status);
  NS_ENSURE_SUCCESS(rv, rv);
  

  aFrame->SetRepeatable(IsRepeatable(desiredSize.height, pageHeight));
  *aDesiredHeight = desiredSize.height;
  return NS_OK;
}



NS_METHOD 
nsTableFrame::ReflowChildren(nsTableReflowState& aReflowState,
                             nsReflowStatus&     aStatus,
                             nsIFrame*&          aLastChildReflowed,
                             nsRect&             aOverflowArea)
{
  aStatus = NS_FRAME_COMPLETE;
  aLastChildReflowed = nsnull;

  nsIFrame* prevKidFrame = nsnull;
  nsresult  rv = NS_OK;
  nscoord   cellSpacingY = GetCellSpacingY();

  nsPresContext* presContext = PresContext();
  
  PRBool isPaginated = presContext->IsPaginated();

  aOverflowArea = nsRect (0, 0, 0, 0);

  PRBool reflowAllKids = aReflowState.reflowState.ShouldReflowAllKids() ||
                         mBits.mResizedColumns ||
                         IsGeometryDirty();

  FrameArray rowGroups;
  nsTableRowGroupFrame *thead, *tfoot;
  PRUint32 numRowGroups = OrderRowGroups(rowGroups, &thead, &tfoot);
  PRBool pageBreak = PR_FALSE;
  nscoord footerHeight = 0;

  
  
  
  
  
  
  
  
  if (isPaginated) {
    if (thead && !GetPrevInFlow()) {
      nscoord desiredHeight;
      rv = SetupHeaderFooterChild(aReflowState, thead, &desiredHeight);
      if (NS_FAILED(rv))
        return rv;
    }
    if (tfoot) {
      rv = SetupHeaderFooterChild(aReflowState, tfoot, &footerHeight);
      if (NS_FAILED(rv))
        return rv;
    }
  }

  for (PRUint32 childX = 0; childX < numRowGroups; childX++) {
    nsIFrame* kidFrame = rowGroups[childX];
    
    
    if (reflowAllKids ||
        NS_SUBTREE_DIRTY(kidFrame) ||
        (aReflowState.reflowState.mFlags.mSpecialHeightReflow &&
         (isPaginated || (kidFrame->GetStateBits() &
                          NS_FRAME_CONTAINS_RELATIVE_HEIGHT)))) {
      if (pageBreak) {
        PushChildren(rowGroups, childX);
        aStatus = NS_FRAME_NOT_COMPLETE;
        break;
      }

      nsSize kidAvailSize(aReflowState.availSize);
      
      PRBool allowRepeatedFooter = PR_FALSE;
      if (isPaginated && (NS_UNCONSTRAINEDSIZE != kidAvailSize.height)) {
        nsTableRowGroupFrame* kidRG = GetRowGroupFrame(kidFrame);
        if (kidRG != thead && kidRG != tfoot && tfoot && tfoot->IsRepeatable()) {
          
          NS_ASSERTION(tfoot == rowGroups[rowGroups.Length() - 1], "Missing footer!");
          if (footerHeight + cellSpacingY < kidAvailSize.height) {
            allowRepeatedFooter = PR_TRUE;
            kidAvailSize.height -= footerHeight + cellSpacingY;
          }
        }
      }

      nsRect oldKidRect = kidFrame->GetRect();
      nsRect oldKidOverflowRect = kidFrame->GetOverflowRect();

      nsHTMLReflowMetrics desiredSize;
      desiredSize.width = desiredSize.height = 0;
  
      
      nsHTMLReflowState kidReflowState(presContext, aReflowState.reflowState,
                                       kidFrame, kidAvailSize,
                                       -1, -1, PR_FALSE);
      InitChildReflowState(kidReflowState);

      
      
      
      
      
      if (childX > (thead ? 1 : 0) &&
          (rowGroups[childX - 1]->GetRect().YMost() > 0)) {
        kidReflowState.mFlags.mIsTopOfPage = PR_FALSE;
      }
      aReflowState.y += cellSpacingY;
      if (NS_UNCONSTRAINEDSIZE != aReflowState.availSize.height) {
        aReflowState.availSize.height -= cellSpacingY;
      }
      
      
      nsIFrame* kidNextInFlow = kidFrame->GetNextInFlow();
      PRBool reorder = PR_FALSE;
      if (kidFrame->GetNextInFlow())
        reorder = PR_TRUE;

      rv = ReflowChild(kidFrame, presContext, desiredSize, kidReflowState,
                       aReflowState.x, aReflowState.y,
                       NS_FRAME_INVALIDATE_ON_MOVE, aStatus);

      if (reorder) {
        
        numRowGroups = OrderRowGroups(rowGroups, &thead, &tfoot);
        childX = rowGroups.IndexOf(kidFrame);
        if (childX == RowGroupArray::NoIndex) {
          
          childX = numRowGroups;
        }
      }
      
      
      if (NS_FRAME_IS_COMPLETE(aStatus) && isPaginated &&
          (NS_UNCONSTRAINEDSIZE != kidReflowState.availableHeight) &&
          kidReflowState.availableHeight < desiredSize.height) {
        
        if (kidReflowState.mFlags.mIsTopOfPage) {
          if (childX+1 < rowGroups.Length()) {
            nsIFrame* nextRowGroupFrame = rowGroups[childX + 1];
            if (nextRowGroupFrame) {
              PlaceChild(aReflowState, kidFrame, desiredSize, oldKidRect,
                         oldKidOverflowRect);
              aStatus = NS_FRAME_NOT_COMPLETE;
              PushChildren(rowGroups, childX + 1);
              aLastChildReflowed = kidFrame;
              break;
            }
          }
        }
        else { 
          if (prevKidFrame) { 
            
            aStatus = NS_FRAME_NOT_COMPLETE;
            PushChildren(rowGroups, childX);
            aLastChildReflowed = prevKidFrame;
            break;
          }
        }
      }

      aLastChildReflowed   = kidFrame;

      pageBreak = PR_FALSE;
      
      if (NS_FRAME_IS_COMPLETE(aStatus) && isPaginated && 
          (NS_UNCONSTRAINEDSIZE != kidReflowState.availableHeight)) {
        nsIFrame* nextKid =
          (childX + 1 < numRowGroups) ? rowGroups[childX + 1] : nsnull;
        pageBreak = PageBreakAfter(*kidFrame, nextKid);
      }

      
      PlaceChild(aReflowState, kidFrame, desiredSize, oldKidRect,
                 oldKidOverflowRect);

      
      prevKidFrame = kidFrame;

      
      if (NS_FRAME_IS_NOT_COMPLETE(aStatus)) {         
        kidNextInFlow = kidFrame->GetNextInFlow();
        if (!kidNextInFlow) {
          
          
          nsIFrame*     continuingFrame;

          rv = presContext->PresShell()->FrameConstructor()->
            CreateContinuingFrame(presContext, kidFrame, this,
                                  &continuingFrame);
          if (NS_FAILED(rv)) {
            aStatus = NS_FRAME_COMPLETE;
            break;
          }

          
          continuingFrame->SetNextSibling(kidFrame->GetNextSibling());
          kidFrame->SetNextSibling(continuingFrame);
          
          
          
          
          rowGroups.InsertElementAt(childX + 1, continuingFrame);
        }
        else {
          
          rowGroups.InsertElementAt(childX + 1, kidNextInFlow);
        }
        
        
        nsIFrame* nextSibling = kidFrame->GetNextSibling();
        if (nsnull != nextSibling) {
          PushChildren(rowGroups, childX + 1);
        }
        if (allowRepeatedFooter) {
          kidAvailSize.height = footerHeight;
          nsHTMLReflowState footerReflowState(presContext,
                                              aReflowState.reflowState,
                                              tfoot, kidAvailSize,
                                              -1, -1, PR_FALSE);
          InitChildReflowState(footerReflowState);
          aReflowState.y += cellSpacingY;

          nsRect origTfootRect = tfoot->GetRect();
          nsRect origTfootOverflowRect = tfoot->GetOverflowRect();
          
          nsReflowStatus footerStatus;
          rv = ReflowChild(tfoot, presContext, desiredSize, footerReflowState,
                           aReflowState.x, aReflowState.y,
                           NS_FRAME_INVALIDATE_ON_MOVE, footerStatus);
          PlaceChild(aReflowState, tfoot, desiredSize, origTfootRect,
                     origTfootOverflowRect);
        }
        break;
      }
    }
    else { 
      aReflowState.y += cellSpacingY;
      nsRect kidRect = kidFrame->GetRect();
      if (kidRect.y != aReflowState.y) {
        
        kidFrame->InvalidateOverflowRect();
        kidRect.y = aReflowState.y;
        kidFrame->SetRect(kidRect);        
        RePositionViews(kidFrame);
        
        kidFrame->InvalidateOverflowRect();
      }
      aReflowState.y += kidRect.height;

      
      if (NS_UNCONSTRAINEDSIZE != aReflowState.availSize.height) {
        aReflowState.availSize.height -= cellSpacingY + kidRect.height;
      }
    }
    ConsiderChildOverflow(aOverflowArea, kidFrame);
  }
  
  
  
  mBits.mResizedColumns = PR_FALSE;
  ClearGeometryDirty();

  return rv;
}

void
nsTableFrame::ReflowColGroups(nsIRenderingContext *aRenderingContext)
{
  if (!GetPrevInFlow() && !HaveReflowedColGroups()) {
    nsHTMLReflowMetrics kidMet;
    nsPresContext *presContext = PresContext();
    for (nsIFrame* kidFrame = mColGroups.FirstChild(); kidFrame;
         kidFrame = kidFrame->GetNextSibling()) {
      if (NS_SUBTREE_DIRTY(kidFrame)) {
        
        nsHTMLReflowState kidReflowState(presContext, kidFrame,
                                       aRenderingContext, nsSize(0,0));
        nsReflowStatus cgStatus;
        ReflowChild(kidFrame, presContext, kidMet, kidReflowState, 0, 0, 0,
                    cgStatus);
        FinishReflowChild(kidFrame, presContext, nsnull, kidMet, 0, 0, 0);
      }
    }
    SetHaveReflowedColGroups(PR_TRUE);
  }
}

void 
nsTableFrame::CalcDesiredHeight(const nsHTMLReflowState& aReflowState, nsHTMLReflowMetrics& aDesiredSize) 
{
  nsTableCellMap* cellMap = GetCellMap();
  if (!cellMap) {
    NS_ASSERTION(PR_FALSE, "never ever call me until the cell map is built!");
    aDesiredSize.height = 0;
    return;
  }
  nscoord  cellSpacingY = GetCellSpacingY();
  nsMargin borderPadding = GetChildAreaOffset(&aReflowState);

  
  FrameArray rowGroups;
  PRUint32 numRowGroups;
  {
    
    nsTableRowGroupFrame *dummy1, *dummy2;
    numRowGroups = OrderRowGroups(rowGroups, &dummy1, &dummy2);
  }
  if (numRowGroups == 0) {
    
    nscoord tableSpecifiedHeight = CalcBorderBoxHeight(aReflowState);
    if ((NS_UNCONSTRAINEDSIZE != tableSpecifiedHeight) &&
        (tableSpecifiedHeight > 0) &&
        eCompatibility_NavQuirks != PresContext()->CompatibilityMode()) {
          
      aDesiredSize.height = tableSpecifiedHeight;
    } 
    else
      aDesiredSize.height = 0;
    return;
  }
  PRInt32 rowCount = cellMap->GetRowCount();
  PRInt32 colCount = cellMap->GetColCount();
  nscoord desiredHeight = borderPadding.top + borderPadding.bottom;
  if (rowCount > 0 && colCount > 0) {
    desiredHeight += cellSpacingY;
    for (PRUint32 rgX = 0; rgX < numRowGroups; rgX++) {
      desiredHeight += rowGroups[rgX]->GetSize().height + cellSpacingY;
    }
  }

  
  if (!GetPrevInFlow()) {
    nscoord tableSpecifiedHeight = CalcBorderBoxHeight(aReflowState);
    if ((tableSpecifiedHeight > 0) && 
        (tableSpecifiedHeight != NS_UNCONSTRAINEDSIZE) &&
        (tableSpecifiedHeight > desiredHeight)) {
      
      
      DistributeHeightToRows(aReflowState, tableSpecifiedHeight - desiredHeight);
      
      for (nsIFrame* kidFrame = mFrames.FirstChild(); kidFrame; kidFrame = kidFrame->GetNextSibling()) {
        ConsiderChildOverflow(aDesiredSize.mOverflowArea, kidFrame);
      } 
      desiredHeight = tableSpecifiedHeight;
    }
  }
  aDesiredSize.height = desiredHeight;
}

static
void ResizeCells(nsTableFrame& aTableFrame)
{
  nsTableFrame::RowGroupArray rowGroups;
  aTableFrame.OrderRowGroups(rowGroups);
  nsHTMLReflowMetrics tableDesiredSize;
  nsRect tableRect = aTableFrame.GetRect();
  tableDesiredSize.width = tableRect.width;
  tableDesiredSize.height = tableRect.height;
  tableDesiredSize.mOverflowArea = nsRect(0, 0, tableRect.width,
                                          tableRect.height);

  for (PRUint32 rgX = 0; rgX < rowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
   
    nsRect rowGroupRect = rgFrame->GetRect();
    nsHTMLReflowMetrics groupDesiredSize;
    groupDesiredSize.width = rowGroupRect.width;
    groupDesiredSize.height = rowGroupRect.height;
    groupDesiredSize.mOverflowArea = nsRect(0, 0, groupDesiredSize.width,
                                      groupDesiredSize.height);
    nsTableRowFrame* rowFrame = rgFrame->GetFirstRow();
    while (rowFrame) {
      rowFrame->DidResize();
      rgFrame->ConsiderChildOverflow(groupDesiredSize.mOverflowArea, rowFrame);
      rowFrame = rowFrame->GetNextRow();
    }
    rgFrame->FinishAndStoreOverflow(&groupDesiredSize.mOverflowArea,
                                    nsSize(groupDesiredSize.width, groupDesiredSize.height));
    
    
    groupDesiredSize.mOverflowArea.MoveBy(rgFrame->GetPosition());
    tableDesiredSize.mOverflowArea.UnionRect(tableDesiredSize.mOverflowArea, groupDesiredSize.mOverflowArea);
  }
  aTableFrame.FinishAndStoreOverflow(&tableDesiredSize.mOverflowArea,
                                     nsSize(tableDesiredSize.width, tableDesiredSize.height));
}

void
nsTableFrame::DistributeHeightToRows(const nsHTMLReflowState& aReflowState,
                                     nscoord                  aAmount)
{
  nscoord cellSpacingY = GetCellSpacingY();

  nsMargin borderPadding = GetChildAreaOffset(&aReflowState);
  
  RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);

  nscoord amountUsed = 0;
  
  
  
  nscoord pctBasis = aReflowState.ComputedHeight() - (GetCellSpacingY() * (GetRowCount() + 1));
  nscoord yOriginRG = borderPadding.top + GetCellSpacingY();
  nscoord yEndRG = yOriginRG;
  PRUint32 rgX;
  for (rgX = 0; rgX < rowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
    nscoord amountUsedByRG = 0;
    nscoord yOriginRow = 0;
    nsRect rgRect = rgFrame->GetRect();
    if (!rgFrame->HasStyleHeight()) {
      nsTableRowFrame* rowFrame = rgFrame->GetFirstRow();
      while (rowFrame) {
        nsRect rowRect = rowFrame->GetRect();
        if ((amountUsed < aAmount) && rowFrame->HasPctHeight()) {
          nscoord pctHeight = rowFrame->GetHeight(pctBasis);
          nscoord amountForRow = PR_MIN(aAmount - amountUsed, pctHeight - rowRect.height);
          if (amountForRow > 0) {
            nsRect oldRowRect = rowRect;
            rowRect.height += amountForRow;
            
            rowFrame->SetRect(rowRect);
            yOriginRow += rowRect.height + cellSpacingY;
            yEndRG += rowRect.height + cellSpacingY;
            amountUsed += amountForRow;
            amountUsedByRG += amountForRow;
            
            nsTableFrame::RePositionViews(rowFrame);

            rgFrame->InvalidateRectDifference(oldRowRect, rowRect);
          }
        }
        else {
          if (amountUsed > 0 && yOriginRow != rowRect.y &&
              !(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
            rowFrame->InvalidateOverflowRect();
            rowFrame->SetPosition(nsPoint(rowRect.x, yOriginRow));
            nsTableFrame::RePositionViews(rowFrame);
            rowFrame->InvalidateOverflowRect();
          }
          yOriginRow += rowRect.height + cellSpacingY;
          yEndRG += rowRect.height + cellSpacingY;
        }
        rowFrame = rowFrame->GetNextRow();
      }
      if (amountUsed > 0) {
        if (rgRect.y != yOriginRG) {
          rgFrame->InvalidateOverflowRect();
        }

        nsRect origRgRect = rgRect;
        nsRect origRgOverflowRect = rgFrame->GetOverflowRect();
        
        rgRect.y = yOriginRG;
        rgRect.height += amountUsedByRG;
        
        rgFrame->SetRect(rgRect);

        nsTableFrame::InvalidateFrame(rgFrame, origRgRect, origRgOverflowRect,
                                      PR_FALSE);
      }
    }
    else if (amountUsed > 0 && yOriginRG != rgRect.y) {
      rgFrame->InvalidateOverflowRect();
      rgFrame->SetPosition(nsPoint(rgRect.x, yOriginRG));
      
      nsTableFrame::RePositionViews(rgFrame);
      rgFrame->InvalidateOverflowRect();
    }
    yOriginRG = yEndRG;
  }

  if (amountUsed >= aAmount) {
    ResizeCells(*this);
    return;
  }

  
  
  nsTableRowGroupFrame* firstUnStyledRG  = nsnull;
  nsTableRowFrame*      firstUnStyledRow = nsnull;
  for (rgX = 0; rgX < rowGroups.Length() && !firstUnStyledRG; rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
    if (!rgFrame->HasStyleHeight()) {
      nsTableRowFrame* rowFrame = rgFrame->GetFirstRow();
      while (rowFrame) {
        if (!rowFrame->HasStyleHeight()) {
          firstUnStyledRG = rgFrame;
          firstUnStyledRow = rowFrame;
          break;
        }
        rowFrame = rowFrame->GetNextRow();
      }
    }
  }

  nsTableRowFrame* lastEligibleRow = nsnull;
  
  
  
  
  nscoord divisor = 0;
  PRInt32 eligibleRows = 0;
  PRBool expandEmptyRows = PR_FALSE;

  if (!firstUnStyledRow) {
    
    divisor = GetRowCount();
  }
  else {
    for (rgX = 0; rgX < rowGroups.Length(); rgX++) {
      nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
      if (!firstUnStyledRG || !rgFrame->HasStyleHeight()) {
        nsTableRowFrame* rowFrame = rgFrame->GetFirstRow();
        while (rowFrame) {
          if (!firstUnStyledRG || !rowFrame->HasStyleHeight()) {
            NS_ASSERTION(rowFrame->GetSize().height >= 0,
                         "negative row frame height");
            divisor += rowFrame->GetSize().height;
            eligibleRows++;
            lastEligibleRow = rowFrame;
          }
          rowFrame = rowFrame->GetNextRow();
        }
      }
    }
    if (divisor <= 0) {
      if (eligibleRows > 0) {
        expandEmptyRows = PR_TRUE;
      }
      else {
        NS_ERROR("invalid divisor");
        return;
      }
    }
  }
  
  nscoord heightToDistribute = aAmount - amountUsed;
  yOriginRG = borderPadding.top + cellSpacingY;
  yEndRG = yOriginRG;
  for (rgX = 0; rgX < rowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
    nscoord amountUsedByRG = 0;
    nscoord yOriginRow = 0;
    nsRect rgRect = rgFrame->GetRect();
    nsRect rgOverflowRect = rgFrame->GetOverflowRect();
    
    if (!firstUnStyledRG || !rgFrame->HasStyleHeight() || !eligibleRows) {
      nsTableRowFrame* rowFrame = rgFrame->GetFirstRow();
      while (rowFrame) {
        nsRect rowRect = rowFrame->GetRect();
        nsRect rowOverflowRect = rowFrame->GetOverflowRect();
        
        if (!firstUnStyledRow || !rowFrame->HasStyleHeight() || !eligibleRows) {          
          float ratio;
          if (eligibleRows) {
            if (!expandEmptyRows) {
              
              
              ratio = float(rowRect.height) / float(divisor);
            } else {
              
              ratio = 1.0f / float(eligibleRows);
            }
          }
          else {
            
            ratio = 1.0f / float(divisor);
          }
          
          
          nscoord amountForRow = (rowFrame == lastEligibleRow) 
                                 ? aAmount - amountUsed : NSToCoordRound(((float)(heightToDistribute)) * ratio);
          amountForRow = PR_MIN(amountForRow, aAmount - amountUsed);

          if (yOriginRow != rowRect.y) {
            rowFrame->InvalidateOverflowRect();
          }
          
          
          nsRect newRowRect(rowRect.x, yOriginRow, rowRect.width,
                            rowRect.height + amountForRow);
          rowFrame->SetRect(newRowRect);

          yOriginRow += newRowRect.height + cellSpacingY;
          yEndRG += newRowRect.height + cellSpacingY;

          amountUsed += amountForRow;
          amountUsedByRG += amountForRow;
          NS_ASSERTION((amountUsed <= aAmount), "invalid row allocation");
          
          nsTableFrame::RePositionViews(rowFrame);

          nsTableFrame::InvalidateFrame(rowFrame, rowRect, rowOverflowRect,
                                        PR_FALSE);
        }
        else {
          if (amountUsed > 0 && yOriginRow != rowRect.y) {
            rowFrame->InvalidateOverflowRect();
            rowFrame->SetPosition(nsPoint(rowRect.x, yOriginRow));
            nsTableFrame::RePositionViews(rowFrame);
            rowFrame->InvalidateOverflowRect();
          }
          yOriginRow += rowRect.height + cellSpacingY;
          yEndRG += rowRect.height + cellSpacingY;
        }
        rowFrame = rowFrame->GetNextRow();
      }
      if (amountUsed > 0) {
        if (rgRect.y != yOriginRG) {
          rgFrame->InvalidateOverflowRect();
        }
        
        rgFrame->SetRect(nsRect(rgRect.x, yOriginRG, rgRect.width,
                                rgRect.height + amountUsedByRG));

        nsTableFrame::InvalidateFrame(rgFrame, rgRect, rgOverflowRect,
                                      PR_FALSE);
      }
      
      
    }
    else if (amountUsed > 0 && yOriginRG != rgRect.y) {
      rgFrame->InvalidateOverflowRect();
      rgFrame->SetPosition(nsPoint(rgRect.x, yOriginRG));
      
      nsTableFrame::RePositionViews(rgFrame);
      rgFrame->InvalidateOverflowRect();
    }
    yOriginRG = yEndRG;
  }

  ResizeCells(*this);
}

PRBool 
nsTableFrame::IsPctHeight(nsStyleContext* aStyleContext) 
{
  PRBool result = PR_FALSE;
  if (aStyleContext) {
    result = (eStyleUnit_Percent ==
              aStyleContext->GetStylePosition()->mHeight.GetUnit());
  }
  return result;
}

PRInt32 nsTableFrame::GetColumnWidth(PRInt32 aColIndex)
{
  nsTableFrame * firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(firstInFlow, "illegal state -- no first in flow");
  PRInt32 result = 0;
  if (this == firstInFlow) {
    nsTableColFrame* colFrame = GetColFrame(aColIndex);
    if (colFrame) {
      result = colFrame->GetFinalWidth();
    }
  }
  else {
    result = firstInFlow->GetColumnWidth(aColIndex);
  }

  return result;
}

void nsTableFrame::SetColumnWidth(PRInt32 aColIndex, nscoord aWidth)
{
  nsTableFrame* firstInFlow = (nsTableFrame *)GetFirstInFlow();
  NS_ASSERTION(firstInFlow, "illegal state -- no first in flow");

  if (this == firstInFlow) {
    nsTableColFrame* colFrame = GetColFrame(aColIndex);
    if (colFrame) {
      colFrame->SetFinalWidth(aWidth);
    }
    else {
      NS_ASSERTION(PR_FALSE, "null col frame");
    }
  }
  else {
    firstInFlow->SetColumnWidth(aColIndex, aWidth);
  }
}


nscoord nsTableFrame::GetCellSpacingX()
{
  if (IsBorderCollapse())
    return 0;

  return GetStyleTableBorder()->mBorderSpacingX;
}


nscoord nsTableFrame::GetCellSpacingY()
{
  if (IsBorderCollapse())
    return 0;

  return GetStyleTableBorder()->mBorderSpacingY;
}


 nscoord
nsTableFrame::GetBaseline() const
{
  nscoord ascent = 0;
  RowGroupArray orderedRowGroups;
  OrderRowGroups(orderedRowGroups);
  nsTableRowFrame* firstRow = nsnull;
  for (PRUint32 rgIndex = 0; rgIndex < orderedRowGroups.Length(); rgIndex++) {
    
    
    nsTableRowGroupFrame* rgFrame = orderedRowGroups[rgIndex];
    if (rgFrame->GetRowCount()) {
      firstRow = rgFrame->GetFirstRow(); 
      ascent = rgFrame->GetRect().y + firstRow->GetRect().y + firstRow->GetRowBaseline();
      break;
    }
  }
  if (!firstRow)
    ascent = GetRect().height;
  return ascent;
}


nsIFrame*
NS_NewTableFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTableFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTableFrame)

nsTableFrame*
nsTableFrame::GetTableFrame(nsIFrame* aSourceFrame)
{
  if (aSourceFrame) {
    
    for (nsIFrame* parentFrame = aSourceFrame->GetParent(); parentFrame;
         parentFrame = parentFrame->GetParent()) {
      if (nsGkAtoms::tableFrame == parentFrame->GetType()) {
        return (nsTableFrame*)parentFrame;
      }
    }
  }
  NS_NOTREACHED("unable to find table parent");
  return nsnull;
}

PRBool 
nsTableFrame::IsAutoWidth(PRBool* aIsPctWidth)
{
  const nsStyleCoord& width = GetStylePosition()->mWidth;

  if (aIsPctWidth) {
    
    
    *aIsPctWidth = width.GetUnit() == eStyleUnit_Percent &&
                   width.GetPercentValue() > 0.0f;
    
  }
  return width.GetUnit() == eStyleUnit_Auto;
}

PRBool 
nsTableFrame::IsAutoHeight()
{
  PRBool isAuto = PR_TRUE;  

  const nsStylePosition* position = GetStylePosition();

  switch (position->mHeight.GetUnit()) {
    case eStyleUnit_Auto:         
      break;
    case eStyleUnit_Coord:
      isAuto = PR_FALSE;
      break;
    case eStyleUnit_Percent:
      if (position->mHeight.GetPercentValue() > 0.0f) {
        isAuto = PR_FALSE;
      }
      break;
    default:
      break;
  }

  return isAuto; 
}

nscoord 
nsTableFrame::CalcBorderBoxHeight(const nsHTMLReflowState& aState)
{
  nscoord height = aState.ComputedHeight();
  if (NS_AUTOHEIGHT != height) {
    nsMargin borderPadding = GetChildAreaOffset(&aState);
    height += borderPadding.top + borderPadding.bottom;
  }
  height = PR_MAX(0, height);

  return height;
}

PRBool 
nsTableFrame::IsAutoLayout()
{
  if (GetStyleTable()->mLayoutStrategy == NS_STYLE_TABLE_LAYOUT_AUTO)
    return PR_TRUE;
  
  
  
  
  const nsStyleCoord &width = GetStylePosition()->mWidth;
  return (width.GetUnit() == eStyleUnit_Auto) ||
         (width.GetUnit() == eStyleUnit_Enumerated &&
          width.GetIntValue() == NS_STYLE_WIDTH_MAX_CONTENT);
}

#ifdef DEBUG
NS_IMETHODIMP
nsTableFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Table"), aResult);
}
#endif



nsIFrame* 
nsTableFrame::GetFrameAtOrBefore(nsIFrame*       aParentFrame,
                                 nsIFrame*       aPriorChildFrame,
                                 nsIAtom*        aChildType)
{
  nsIFrame* result = nsnull;
  if (!aPriorChildFrame) {
    return result;
  }
  if (aChildType == aPriorChildFrame->GetType()) {
    return aPriorChildFrame;
  }

  
  
  nsIFrame* lastMatchingFrame = nsnull;
  nsIFrame* childFrame = aParentFrame->GetFirstChild(nsnull);
  while (childFrame && (childFrame != aPriorChildFrame)) {
    if (aChildType == childFrame->GetType()) {
      lastMatchingFrame = childFrame;
    }
    childFrame = childFrame->GetNextSibling();
  }
  return lastMatchingFrame;
}

#ifdef DEBUG
void 
nsTableFrame::DumpRowGroup(nsIFrame* aKidFrame)
{
  nsTableRowGroupFrame* rgFrame = GetRowGroupFrame(aKidFrame);
  if (rgFrame) {
    nsIFrame* cFrame = rgFrame->GetFirstChild(nsnull);
    while (cFrame) {
      nsTableRowFrame *rowFrame = do_QueryFrame(cFrame);
      if (rowFrame) {
        printf("row(%d)=%p ", rowFrame->GetRowIndex(), rowFrame);
        nsIFrame* childFrame = cFrame->GetFirstChild(nsnull);
        while (childFrame) {
          nsTableCellFrame *cellFrame = do_QueryFrame(childFrame);
          if (cellFrame) {
            PRInt32 colIndex;
            cellFrame->GetColIndex(colIndex);
            printf("cell(%d)=%p ", colIndex, childFrame);
          }
          childFrame = childFrame->GetNextSibling();
        }
        printf("\n");
      }
      else {
        DumpRowGroup(rowFrame);
      }
      cFrame = cFrame->GetNextSibling();
    }
  }
}

void 
nsTableFrame::Dump(PRBool          aDumpRows,
                   PRBool          aDumpCols, 
                   PRBool          aDumpCellMap)
{
  printf("***START TABLE DUMP*** \n");
  
  printf("mColWidths=");
  PRInt32 numCols = GetColCount();
  PRInt32 colX;
  for (colX = 0; colX < numCols; colX++) {
    printf("%d ", GetColumnWidth(colX));
  }
  printf("\n");

  if (aDumpRows) {
    nsIFrame* kidFrame = mFrames.FirstChild();
    while (kidFrame) {
      DumpRowGroup(kidFrame);
      kidFrame = kidFrame->GetNextSibling();
    }
  }

  if (aDumpCols) {
	  
    printf("\n col frame cache ->");
	   for (colX = 0; colX < numCols; colX++) {
      nsTableColFrame* colFrame = mColFrames.ElementAt(colX);
      if (0 == (colX % 8)) {
        printf("\n");
      }
      printf ("%d=%p ", colX, colFrame);
      nsTableColType colType = colFrame->GetColType();
      switch (colType) {
      case eColContent:
        printf(" content ");
        break;
      case eColAnonymousCol: 
        printf(" anonymous-column ");
        break;
      case eColAnonymousColGroup:
        printf(" anonymous-colgroup ");
        break;
      case eColAnonymousCell: 
        printf(" anonymous-cell ");
        break;
      }
    }
    printf("\n colgroups->");
    for (nsIFrame* childFrame = mColGroups.FirstChild(); childFrame;
         childFrame = childFrame->GetNextSibling()) {
      if (nsGkAtoms::tableColGroupFrame == childFrame->GetType()) {
        nsTableColGroupFrame* colGroupFrame = (nsTableColGroupFrame *)childFrame;
        colGroupFrame->Dump(1);
      }
    }
    for (colX = 0; colX < numCols; colX++) {
      printf("\n");
      nsTableColFrame* colFrame = GetColFrame(colX);
      colFrame->Dump(1);
    }
  }
  if (aDumpCellMap) {
    nsTableCellMap* cellMap = GetCellMap();
    cellMap->Dump();
  }
  printf(" ***END TABLE DUMP*** \n");
}
#endif


nsTableIterator::nsTableIterator(nsIFrame& aSource)
{
  nsIFrame* firstChild = aSource.GetFirstChild(nsnull);
  Init(firstChild);
}

nsTableIterator::nsTableIterator(nsFrameList& aSource)
{
  nsIFrame* firstChild = aSource.FirstChild();
  Init(firstChild);
}

void nsTableIterator::Init(nsIFrame* aFirstChild)
{
  mFirstListChild = aFirstChild;
  mFirstChild     = aFirstChild;
  mCurrentChild   = nsnull;
  mLeftToRight    = PR_TRUE;
  mCount          = -1;

  if (!mFirstChild) {
    return;
  }

  nsTableFrame* table = nsTableFrame::GetTableFrame(mFirstChild);
  if (table) {
    mLeftToRight = (NS_STYLE_DIRECTION_LTR ==
                    table->GetStyleVisibility()->mDirection);
  }
  else {
    NS_NOTREACHED("source of table iterator is not part of a table");
    return;
  }

  if (!mLeftToRight) {
    mCount = 0;
    nsIFrame* nextChild = mFirstChild->GetNextSibling();
    while (nsnull != nextChild) {
      mCount++;
      mFirstChild = nextChild;
      nextChild = nextChild->GetNextSibling();
    }
  } 
}

nsIFrame* nsTableIterator::First()
{
  mCurrentChild = mFirstChild;
  return mCurrentChild;
}
      
nsIFrame* nsTableIterator::Next()
{
  if (!mCurrentChild) {
    return nsnull;
  }

  if (mLeftToRight) {
    mCurrentChild = mCurrentChild->GetNextSibling();
    return mCurrentChild;
  }
  else {
    nsIFrame* targetChild = mCurrentChild;
    mCurrentChild = nsnull;
    nsIFrame* child = mFirstListChild;
    while (child && (child != targetChild)) {
      mCurrentChild = child;
      child = child->GetNextSibling();
    }
    return mCurrentChild;
  }
}

PRBool nsTableIterator::IsLeftToRight()
{
  return mLeftToRight;
}

PRInt32 nsTableIterator::Count()
{
  if (-1 == mCount) {
    mCount = 0;
    nsIFrame* child = mFirstListChild;
    while (nsnull != child) {
      mCount++;
      child = child->GetNextSibling();
    }
  }
  return mCount;
}


NS_IMETHODIMP 
nsTableFrame::GetCellDataAt(PRInt32        aRowIndex, 
                            PRInt32        aColIndex,
                            nsIDOMElement* &aCell,   
                            PRInt32&       aStartRowIndex, 
                            PRInt32&       aStartColIndex, 
                            PRInt32&       aRowSpan, 
                            PRInt32&       aColSpan,
                            PRInt32&       aActualRowSpan, 
                            PRInt32&       aActualColSpan,
                            PRBool&        aIsSelected)
{
  
  aCell = nsnull;
  aStartRowIndex = 0;
  aStartColIndex = 0;
  aRowSpan = 0;
  aColSpan = 0;
  aIsSelected = PR_FALSE;

  nsTableCellMap* cellMap = GetCellMap();
  if (!cellMap) { return NS_ERROR_NOT_INITIALIZED;}

  PRBool originates;
  PRInt32 colSpan; 

  nsTableCellFrame *cellFrame = cellMap->GetCellInfoAt(aRowIndex, aColIndex, &originates, &colSpan);
  if (!cellFrame) return NS_TABLELAYOUT_CELL_NOT_FOUND;

  nsresult result= cellFrame->GetRowIndex(aStartRowIndex);
  if (NS_FAILED(result)) return result;
  result = cellFrame->GetColIndex(aStartColIndex);
  if (NS_FAILED(result)) return result;
  
  aRowSpan = cellFrame->GetRowSpan();
  aColSpan = cellFrame->GetColSpan();
  aActualRowSpan = GetEffectiveRowSpan(*cellFrame);
  aActualColSpan = GetEffectiveColSpan(*cellFrame);

  
  if (aActualRowSpan == 0 || aActualColSpan == 0)
    return NS_ERROR_FAILURE;

  result = cellFrame->GetSelected(&aIsSelected);
  if (NS_FAILED(result)) return result;

  
  
  nsIContent* content = cellFrame->GetContent();
  if (!content) return NS_ERROR_FAILURE;   
  
  return CallQueryInterface(content, &aCell);                                      
}

NS_IMETHODIMP nsTableFrame::GetTableSize(PRInt32& aRowCount, PRInt32& aColCount)
{
  nsTableCellMap* cellMap = GetCellMap();
  
  aRowCount = 0;
  aColCount = 0;
  if (!cellMap) { return NS_ERROR_NOT_INITIALIZED;}

  aRowCount = cellMap->GetRowCount();
  aColCount = cellMap->GetColCount();
  return NS_OK;
}

NS_IMETHODIMP
nsTableFrame::GetIndexByRowAndColumn(PRInt32 aRow, PRInt32 aColumn,
                                     PRInt32 *aIndex)
{
  NS_ENSURE_ARG_POINTER(aIndex);
  *aIndex = -1;

  nsTableCellMap* cellMap = GetCellMap();
  if (!cellMap)
    return NS_ERROR_NOT_INITIALIZED;

  *aIndex = cellMap->GetIndexByRowAndColumn(aRow, aColumn);
  return (*aIndex == -1) ? NS_TABLELAYOUT_CELL_NOT_FOUND : NS_OK;
}

NS_IMETHODIMP
nsTableFrame::GetRowAndColumnByIndex(PRInt32 aIndex,
                                    PRInt32 *aRow, PRInt32 *aColumn)
{
  NS_ENSURE_ARG_POINTER(aRow);
  *aRow = -1;

  NS_ENSURE_ARG_POINTER(aColumn);
  *aColumn = -1;

  nsTableCellMap* cellMap = GetCellMap();
  if (!cellMap)
    return NS_ERROR_NOT_INITIALIZED;

  cellMap->GetRowAndColumnByIndex(aIndex, aRow, aColumn);
  return NS_OK;
}



PRBool
nsTableFrame::ColumnHasCellSpacingBefore(PRInt32 aColIndex) const
{
  
  
  if (LayoutStrategy()->GetType() == nsITableLayoutStrategy::Fixed)
    return PR_TRUE;
  
  if (aColIndex == 0)
    return PR_TRUE;
  nsTableCellMap* cellMap = GetCellMap();
  if (!cellMap) 
    return PR_FALSE;
  return cellMap->GetNumCellsOriginatingInCol(aColIndex) > 0;
}

static void
CheckFixDamageArea(PRInt32 aNumRows,
                   PRInt32 aNumCols,
                   nsRect& aDamageArea)
{
  if (((aDamageArea.XMost() > aNumCols) && (aDamageArea.width  != 1) && (aNumCols != 0)) || 
      ((aDamageArea.YMost() > aNumRows) && (aDamageArea.height != 1) && (aNumRows != 0))) {
    
    NS_ASSERTION(PR_FALSE, "invalid BC damage area");
    aDamageArea.x      = 0;
    aDamageArea.y      = 0;
    aDamageArea.width  = aNumCols;
    aDamageArea.height = aNumRows;
  }
}














void 
nsTableFrame::SetBCDamageArea(const nsRect& aValue)
{
  nsRect newRect(aValue);
  newRect.width  = PR_MAX(1, newRect.width);
  newRect.height = PR_MAX(1, newRect.height);

  if (!IsBorderCollapse()) {
    NS_ASSERTION(PR_FALSE, "invalid call - not border collapse model");
    return;
  }
  SetNeedToCalcBCBorders(PR_TRUE);
  
  BCPropertyData* value = (BCPropertyData*)nsTableFrame::GetProperty(this, nsGkAtoms::tableBCProperty, PR_TRUE);
  if (value) {
    
    value->mDamageArea.UnionRect(value->mDamageArea, newRect);
    CheckFixDamageArea(GetRowCount(), GetColCount(), value->mDamageArea);
  }
}













struct BCCellBorder
{
  BCCellBorder() { Reset(0, 1); }
  void Reset(PRUint32 aRowIndex, PRUint32 aRowSpan);
  nscolor       color;    
  BCPixelSize   width;    
  PRUint8       style;    
                          
  BCBorderOwner owner;    
                          
                          
                          
                          
  PRInt32       rowIndex; 
                          
  PRInt32       rowSpan;  
                          
};

void
BCCellBorder::Reset(PRUint32 aRowIndex,
                    PRUint32 aRowSpan)
{
  style = NS_STYLE_BORDER_STYLE_NONE;
  color = 0;
  width = 0;
  owner = eTableOwner;
  rowIndex = aRowIndex;
  rowSpan  = aRowSpan;
}

class BCMapCellIterator;








struct BCMapCellInfo 
{
  BCMapCellInfo(nsTableFrame* aTableFrame);
  void ResetCellInfo();
  void SetInfo(nsTableRowFrame*   aNewRow,
               PRInt32            aColIndex,
               BCCellData*        aCellData,
               BCMapCellIterator* aIter,
               nsCellMap*         aCellMap = nsnull);
  
  
  
  
  
  
  void SetTableTopLeftContBCBorder();
  void SetRowGroupLeftContBCBorder();
  void SetRowGroupRightContBCBorder();
  void SetRowGroupBottomContBCBorder();
  void SetRowLeftContBCBorder();
  void SetRowRightContBCBorder();
  void SetColumnTopRightContBCBorder();
  void SetColumnBottomContBCBorder();
  void SetColGroupBottomContBCBorder();
  void SetInnerRowGroupBottomContBCBorder(const nsIFrame* aNextRowGroup,
                                          nsTableRowFrame* aNextRow);

  
  
  void SetTableTopBorderWidth(BCPixelSize aWidth);
  void SetTableLeftBorderWidth(PRInt32 aRowY, BCPixelSize aWidth);
  void SetTableRightBorderWidth(PRInt32 aRowY, BCPixelSize aWidth);
  void SetTableBottomBorderWidth(BCPixelSize aWidth);
  void SetLeftBorderWidths(BCPixelSize aWidth);
  void SetRightBorderWidths(BCPixelSize aWidth);
  void SetTopBorderWidths(BCPixelSize aWidth);
  void SetBottomBorderWidths(BCPixelSize aWidth);

  
  
  
  
  BCCellBorder GetTopEdgeBorder();
  BCCellBorder GetBottomEdgeBorder();
  BCCellBorder GetLeftEdgeBorder();
  BCCellBorder GetRightEdgeBorder();
  BCCellBorder GetRightInternalBorder();
  BCCellBorder GetLeftInternalBorder();
  BCCellBorder GetTopInternalBorder();
  BCCellBorder GetBottomInternalBorder();

  
  void SetColumn(PRInt32 aColX);
  
  void IncrementRow(PRBool aResetToTopRowOfCell = PR_FALSE);
  
  
  PRInt32 GetCellEndRowIndex() const;
  PRInt32 GetCellEndColIndex() const;

  
  nsTableFrame*         mTableFrame;
  PRInt32               mNumTableRows;
  PRInt32               mNumTableCols;
  BCPropertyData*       mTableBCData;

  
  
  PRPackedBool          mTableIsLTR;
  PRUint8               mStartSide;
  PRUint8               mEndSide;
  
  
  nsTableRowGroupFrame* mRowGroup;

  
  nsTableRowFrame*      mTopRow;
  nsTableRowFrame*      mBottomRow;
  nsTableRowFrame*      mCurrentRowFrame;

  
  
  nsTableColGroupFrame* mColGroup;
  nsTableColGroupFrame* mCurrentColGroupFrame;
 
  nsTableColFrame*      mLeftCol;
  nsTableColFrame*      mRightCol;
  nsTableColFrame*      mCurrentColFrame;
  
  
  BCCellData*           mCellData;
  nsBCTableCellFrame*   mCell;

  PRInt32               mRowIndex;
  PRInt32               mRowSpan;
  PRInt32               mColIndex;
  PRInt32               mColSpan;

  
  
  
  PRPackedBool          mRgAtTop;
  PRPackedBool          mRgAtBottom;
  PRPackedBool          mCgAtLeft;
  PRPackedBool          mCgAtRight;
 
};


BCMapCellInfo::BCMapCellInfo(nsTableFrame* aTableFrame)
{
  mTableFrame = aTableFrame;
  mTableIsLTR =
    aTableFrame->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_LTR;
  if (mTableIsLTR) {
    mStartSide = NS_SIDE_LEFT;
    mEndSide = NS_SIDE_RIGHT;
  }
  else {
    mStartSide = NS_SIDE_RIGHT;
    mEndSide = NS_SIDE_LEFT;
  }
  mNumTableRows = mTableFrame->GetRowCount();
  mNumTableCols = mTableFrame->GetColCount();
  mTableBCData =
    static_cast <BCPropertyData*>(nsTableFrame::GetProperty(mTableFrame,
                 nsGkAtoms::tableBCProperty, PR_FALSE));
                 
  ResetCellInfo();
}

void BCMapCellInfo::ResetCellInfo()
{
  mCellData  = nsnull;
  mRowGroup  = nsnull;
  mTopRow    = nsnull;
  mBottomRow = nsnull;
  mColGroup  = nsnull;
  mLeftCol   = nsnull;
  mRightCol  = nsnull;
  mCell      = nsnull;
  mRowIndex  = mRowSpan = mColIndex = mColSpan = 0;
  mRgAtTop = mRgAtBottom = mCgAtLeft = mCgAtRight = PR_FALSE;
}

inline PRInt32 BCMapCellInfo::GetCellEndRowIndex() const
{
  return mRowIndex + mRowSpan - 1;
}

inline PRInt32 BCMapCellInfo::GetCellEndColIndex() const
{
  return mColIndex + mColSpan - 1;
}


class BCMapCellIterator
{
public:
  BCMapCellIterator(nsTableFrame* aTableFrame,
                    const nsRect& aDamageArea);

  void First(BCMapCellInfo& aMapCellInfo);

  void Next(BCMapCellInfo& aMapCellInfo);

  void PeekRight(BCMapCellInfo& aRefInfo,
                 PRUint32     aRowIndex,
                 BCMapCellInfo& aAjaInfo);

  void PeekBottom(BCMapCellInfo& aRefInfo,
                  PRUint32     aColIndex,
                  BCMapCellInfo& aAjaInfo);

  PRBool IsNewRow() { return mIsNewRow; }

  nsTableRowFrame* GetPrevRow() const { return mPrevRow; }
  nsTableRowFrame* GetCurrentRow() const { return mRow; }
  nsTableRowGroupFrame* GetCurrentRowGroup() const { return mRowGroup;}

  PRInt32    mRowGroupStart;
  PRInt32    mRowGroupEnd;
  PRBool     mAtEnd;
  nsCellMap* mCellMap;

private:
  PRBool SetNewRow(nsTableRowFrame* row = nsnull);
  PRBool SetNewRowGroup(PRBool aFindFirstDamagedRow);

  nsTableFrame*         mTableFrame;
  nsTableCellMap*       mTableCellMap;
  nsTableFrame::RowGroupArray mRowGroups;
  nsTableRowGroupFrame* mRowGroup;
  PRInt32               mRowGroupIndex;
  PRUint32              mNumTableRows;
  nsTableRowFrame*      mRow;
  nsTableRowFrame*      mPrevRow;
  PRBool                mIsNewRow;
  PRInt32               mRowIndex;
  PRUint32              mNumTableCols;
  PRInt32               mColIndex;
  nsPoint               mAreaStart;
  nsPoint               mAreaEnd;
};

BCMapCellIterator::BCMapCellIterator(nsTableFrame* aTableFrame,
                                     const nsRect& aDamageArea)
:mTableFrame(aTableFrame)
{
  mTableCellMap  = aTableFrame->GetCellMap();

  mAreaStart.x   = aDamageArea.x;
  mAreaStart.y   = aDamageArea.y;
  mAreaEnd.y     = aDamageArea.y + aDamageArea.height - 1;
  mAreaEnd.x     = aDamageArea.x + aDamageArea.width - 1;

  mNumTableRows  = mTableFrame->GetRowCount();
  mRow           = nsnull;
  mRowIndex      = 0;
  mNumTableCols  = mTableFrame->GetColCount();
  mColIndex      = 0;
  mRowGroupIndex = -1;

  
  aTableFrame->OrderRowGroups(mRowGroups);

  mAtEnd = PR_TRUE; 
}


void
BCMapCellInfo::SetInfo(nsTableRowFrame*   aNewRow,
                       PRInt32            aColIndex,
                       BCCellData*        aCellData,
                       BCMapCellIterator* aIter,
                       nsCellMap*         aCellMap)
{
  
  mCellData = aCellData;
  mColIndex = aColIndex;

  
  
  mRowIndex = 0;
  if (aNewRow) {
    mTopRow = aNewRow;
    mRowIndex = aNewRow->GetRowIndex();
  }

  
  mCell      = nsnull;
  mRowSpan   = 1;
  mColSpan   = 1;
  if (aCellData) {
    mCell = static_cast<nsBCTableCellFrame*>(aCellData->GetCellFrame());
    if (mCell) {
      if (!mTopRow) {
        mTopRow = static_cast<nsTableRowFrame*>(mCell->GetParent());
        if (!mTopRow) ABORT0();
        mRowIndex = mTopRow->GetRowIndex();
      }
      mColSpan = mTableFrame->GetEffectiveColSpan(*mCell, aCellMap);
      mRowSpan = mTableFrame->GetEffectiveRowSpan(*mCell, aCellMap);
    }
  }
  
  if (!mTopRow) {
    mTopRow = aIter->GetCurrentRow();
  }
  if (1 == mRowSpan) {
    mBottomRow = mTopRow;
  }
  else {
    mBottomRow = mTopRow->GetNextRow();
    if (mBottomRow) {
      for (PRInt32 spanY = 2; mBottomRow && (spanY < mRowSpan); spanY++) {
        mBottomRow = mBottomRow->GetNextRow();
      }
      NS_ASSERTION(mBottomRow, "spanned row not found");
    }
    else {
      NS_ASSERTION(PR_FALSE, "error in cell map");
      mRowSpan = 1;
      mBottomRow = mTopRow;
    }
  }
  
  
  
  
  PRUint32 rgStart  = aIter->mRowGroupStart;
  PRUint32 rgEnd    = aIter->mRowGroupEnd;
  mRowGroup = mTableFrame->GetRowGroupFrame(mTopRow->GetParent());
  if (mRowGroup != aIter->GetCurrentRowGroup()) {
    rgStart = mRowGroup->GetStartRowIndex();
    rgEnd   = rgStart + mRowGroup->GetRowCount() - 1;
  }
  PRUint32 rowIndex = mTopRow->GetRowIndex();
  mRgAtTop    = (rgStart == rowIndex);
  mRgAtBottom = (rgEnd == rowIndex + mRowSpan - 1);
  
   
  mLeftCol = mTableFrame->GetColFrame(aColIndex);
  if (!mLeftCol) ABORT0();

  mRightCol = mLeftCol;
  if (mColSpan > 1) {
    nsTableColFrame* colFrame = mTableFrame->GetColFrame(aColIndex +
                                                         mColSpan -1);
    if (!colFrame) ABORT0();
    mRightCol = colFrame;
  }

  
  mColGroup = static_cast<nsTableColGroupFrame*>(mLeftCol->GetParent());
  PRInt32 cgStart = mColGroup->GetStartColumnIndex();
  PRInt32 cgEnd = PR_MAX(0, cgStart + mColGroup->GetColCount() - 1);
  mCgAtLeft  = (cgStart == aColIndex);
  mCgAtRight = (cgEnd == aColIndex + mColSpan - 1);
}

PRBool
BCMapCellIterator::SetNewRow(nsTableRowFrame* aRow)
{
  mAtEnd   = PR_TRUE;
  mPrevRow = mRow;
  if (aRow) {
    mRow = aRow;
  }
  else if (mRow) {
    mRow = mRow->GetNextRow();
  }
  if (mRow) {
    mRowIndex = mRow->GetRowIndex();
    
    PRInt32 rgRowIndex = mRowIndex - mRowGroupStart;
    if (PRUint32(rgRowIndex) >= mCellMap->mRows.Length()) 
      ABORT1(PR_FALSE);
    const nsCellMap::CellDataArray& row = mCellMap->mRows[rgRowIndex];

    for (mColIndex = mAreaStart.x; mColIndex <= mAreaEnd.x; mColIndex++) {
      CellData* cellData = row.SafeElementAt(mColIndex);
      if (!cellData) { 
        nsRect damageArea;
        cellData = mCellMap->AppendCell(*mTableCellMap, nsnull, rgRowIndex, PR_FALSE, damageArea); if (!cellData) ABORT1(PR_FALSE);
      }
      if (cellData && (cellData->IsOrig() || cellData->IsDead())) {
        break;
      }
    }
    mIsNewRow = PR_TRUE;
    mAtEnd    = PR_FALSE;
  }
  else ABORT1(PR_FALSE);

  return !mAtEnd;
}

PRBool
BCMapCellIterator::SetNewRowGroup(PRBool aFindFirstDamagedRow)
{
   mAtEnd = PR_TRUE;  
  PRInt32 numRowGroups = mRowGroups.Length();
  mCellMap = nsnull;
  for (mRowGroupIndex++; mRowGroupIndex < numRowGroups; mRowGroupIndex++) {
    mRowGroup = mRowGroups[mRowGroupIndex];
    PRInt32 rowCount = mRowGroup->GetRowCount();
    mRowGroupStart = mRowGroup->GetStartRowIndex();
    mRowGroupEnd   = mRowGroupStart + rowCount - 1;
    if (rowCount > 0) {
      mCellMap = mTableCellMap->GetMapFor(mRowGroup, mCellMap);
      if (!mCellMap) ABORT1(PR_FALSE);
      nsTableRowFrame* firstRow = mRowGroup->GetFirstRow();
      if (aFindFirstDamagedRow) {
        if ((mAreaStart.y >= mRowGroupStart) && (mAreaStart.y <= mRowGroupEnd)) {
          
          if (aFindFirstDamagedRow) {
            
            PRInt32 numRows = mAreaStart.y - mRowGroupStart;
            for (PRInt32 i = 0; i < numRows; i++) {
              firstRow = firstRow->GetNextRow();
              if (!firstRow) ABORT1(PR_FALSE);
            }
          }
        }
        else {     
          continue;
        }
      }
      if (SetNewRow(firstRow)) { 
        break;
      }
    }
  }
    
  return !mAtEnd;
}

void 
BCMapCellIterator::First(BCMapCellInfo& aMapInfo)
{
  aMapInfo.ResetCellInfo();

  SetNewRowGroup(PR_TRUE); 
  while (!mAtEnd) {
    if ((mAreaStart.y >= mRowGroupStart) && (mAreaStart.y <= mRowGroupEnd)) {
      BCCellData* cellData =
        static_cast<BCCellData*>(mCellMap->GetDataAt(mAreaStart.y -
                                                      mRowGroupStart,
                                                      mAreaStart.x));
      if (cellData && cellData->IsOrig()) {
        aMapInfo.SetInfo(mRow, mAreaStart.x, cellData, this);
      }
      else {
        NS_ASSERTION(((0 == mAreaStart.x) && (mRowGroupStart == mAreaStart.y)) ,
                     "damage area expanded incorrectly");
        mAtEnd = PR_TRUE;
      }
      break;
    }
    SetNewRowGroup(PR_TRUE); 
  } 
}

void 
BCMapCellIterator::Next(BCMapCellInfo& aMapInfo)
{
  if (mAtEnd) ABORT0();
  aMapInfo.ResetCellInfo();

  mIsNewRow = PR_FALSE;
  mColIndex++;
  while ((mRowIndex <= mAreaEnd.y) && !mAtEnd) {
    for (; mColIndex <= mAreaEnd.x; mColIndex++) {
      PRInt32 rgRowIndex = mRowIndex - mRowGroupStart;
      BCCellData* cellData =
         static_cast<BCCellData*>(mCellMap->GetDataAt(rgRowIndex, mColIndex));
      if (!cellData) { 
        nsRect damageArea;
        cellData =
          static_cast<BCCellData*>(mCellMap->AppendCell(*mTableCellMap, nsnull,
                                                         rgRowIndex, PR_FALSE,
                                                         damageArea));
        if (!cellData) ABORT0();
      }
      if (cellData && (cellData->IsOrig() || cellData->IsDead())) {
        aMapInfo.SetInfo(mRow, mColIndex, cellData, this);
        return;
      }
    }
    if (mRowIndex >= mRowGroupEnd) {
      SetNewRowGroup(PR_FALSE); 
    }
    else {
      SetNewRow(); 
    }
  }
  mAtEnd = PR_TRUE;
}

void 
BCMapCellIterator::PeekRight(BCMapCellInfo&   aRefInfo,
                             PRUint32         aRowIndex,
                             BCMapCellInfo&   aAjaInfo)
{
  aAjaInfo.ResetCellInfo();
  PRInt32 colIndex = aRefInfo.mColIndex + aRefInfo.mColSpan;
  PRUint32 rgRowIndex = aRowIndex - mRowGroupStart;

  BCCellData* cellData =
    static_cast<BCCellData*>(mCellMap->GetDataAt(rgRowIndex, colIndex));
  if (!cellData) { 
    NS_ASSERTION(colIndex < mTableCellMap->GetColCount(), "program error");
    nsRect damageArea;
    cellData =
      static_cast<BCCellData*>(mCellMap->AppendCell(*mTableCellMap, nsnull,
                                                     rgRowIndex, PR_FALSE,
                                                     damageArea));
    if (!cellData) ABORT0();
  }
  nsTableRowFrame* row = nsnull;
  if (cellData->IsRowSpan()) {
    rgRowIndex -= cellData->GetRowSpanOffset();
    cellData =
      static_cast<BCCellData*>(mCellMap->GetDataAt(rgRowIndex, colIndex));
    if (!cellData)
      ABORT0();
  }
  else {
    row = mRow;
  }
  aAjaInfo.SetInfo(row, colIndex, cellData, this);
}

void 
BCMapCellIterator::PeekBottom(BCMapCellInfo&   aRefInfo,
                              PRUint32         aColIndex,
                              BCMapCellInfo&   aAjaInfo)
{
  aAjaInfo.ResetCellInfo();
  PRInt32 rowIndex = aRefInfo.mRowIndex + aRefInfo.mRowSpan;
  PRInt32 rgRowIndex = rowIndex - mRowGroupStart;
  nsTableRowGroupFrame* rg = mRowGroup;
  nsCellMap* cellMap = mCellMap;
  nsTableRowFrame* nextRow = nsnull;
  if (rowIndex > mRowGroupEnd) {
    PRInt32 nextRgIndex = mRowGroupIndex;
    do {
      nextRgIndex++;
      rg = mRowGroups.SafeElementAt(nextRgIndex);
      if (rg) {
        cellMap = mTableCellMap->GetMapFor(rg, cellMap); if (!cellMap) ABORT0();
        rgRowIndex = 0;
        nextRow = rg->GetFirstRow();
      }
    }
    while (rg && !nextRow);
    if(!rg) return;
  }
  else {
    
    nextRow = mRow;
    for (PRInt32 i = 0; i < aRefInfo.mRowSpan; i++) {
      nextRow = nextRow->GetNextRow(); if (!nextRow) ABORT0();
    }
  }

  BCCellData* cellData =
    static_cast<BCCellData*>(cellMap->GetDataAt(rgRowIndex, aColIndex));
  if (!cellData) { 
    NS_ASSERTION(rgRowIndex < cellMap->GetRowCount(), "program error");
    nsRect damageArea;
    cellData =
      static_cast<BCCellData*>(cellMap->AppendCell(*mTableCellMap, nsnull,
                                                    rgRowIndex, PR_FALSE,
                                                    damageArea));
    if (!cellData) ABORT0();
  }
  if (cellData->IsColSpan()) {
    aColIndex -= cellData->GetColSpanOffset();
    cellData =
      static_cast<BCCellData*>(cellMap->GetDataAt(rgRowIndex, aColIndex));
  }
  aAjaInfo.SetInfo(nextRow, aColIndex, cellData, this, cellMap);
}



static PRUint8 styleToPriority[13] = { 0,  
                                       2,  
                                       4,  
                                       5,  
                                       6,  
                                       7,  
                                       8,  
                                       1,  
                                       3,  
                                       9 };



#define CELL_CORNER PR_TRUE










static void 
GetColorAndStyle(const nsIFrame*  aFrame,
                 PRUint8          aSide,
                 PRUint8&         aStyle,
                 nscolor&         aColor,
                 PRBool           aTableIsLTR,
                 PRBool           aIgnoreTableEdge)
{
  NS_PRECONDITION(aFrame, "null frame");
  
  aColor = 0;
  const nsStyleBorder* styleData = aFrame->GetStyleBorder();
  if(!aTableIsLTR) { 
    if (NS_SIDE_RIGHT == aSide) {
      aSide = NS_SIDE_LEFT;
    }
    else if (NS_SIDE_LEFT == aSide) {
      aSide = NS_SIDE_RIGHT;
    }
  }
  aStyle = styleData->GetBorderStyle(aSide);

  
  if (NS_STYLE_BORDER_STYLE_RULES_MARKER & aStyle) {
    if (aIgnoreTableEdge) {
      aStyle = NS_STYLE_BORDER_STYLE_NONE;
      return;
    }
    else {
      aStyle &= ~NS_STYLE_BORDER_STYLE_RULES_MARKER;
    }
  }

  if ((NS_STYLE_BORDER_STYLE_NONE == aStyle) ||
      (NS_STYLE_BORDER_STYLE_HIDDEN == aStyle)) {
    return;
  }
  PRBool foreground;
  styleData->GetBorderColor(aSide, aColor, foreground);
  if (foreground) {
    aColor = aFrame->GetStyleColor()->mColor;
  }
}










static void
GetPaintStyleInfo(const nsIFrame*  aFrame,
                  PRUint8          aSide,
                  PRUint8&         aStyle,
                  nscolor&         aColor,
                  PRBool           aTableIsLTR,
                  PRBool           aIgnoreTableEdge)
{
  GetColorAndStyle(aFrame, aSide, aStyle, aColor, aTableIsLTR, aIgnoreTableEdge);
  if (NS_STYLE_BORDER_STYLE_INSET    == aStyle) {
    aStyle = NS_STYLE_BORDER_STYLE_RIDGE;
  }
  else if (NS_STYLE_BORDER_STYLE_OUTSET    == aStyle) {
    aStyle = NS_STYLE_BORDER_STYLE_GROOVE;
  }
}













static void
GetColorAndStyle(const nsIFrame*  aFrame,
                 PRUint8          aSide,
                 PRUint8&         aStyle,
                 nscolor&         aColor,
                 PRBool           aTableIsLTR,
                 PRBool           aIgnoreTableEdge,
                 BCPixelSize&     aWidth)
{
  GetColorAndStyle(aFrame, aSide, aStyle, aColor, aTableIsLTR, aIgnoreTableEdge);
  if ((NS_STYLE_BORDER_STYLE_NONE == aStyle) ||
      (NS_STYLE_BORDER_STYLE_HIDDEN == aStyle)) {
    aWidth = 0;
    return;
  }
  const nsStyleBorder* styleData = aFrame->GetStyleBorder();
  nscoord width;
  if(!aTableIsLTR) { 
    if (NS_SIDE_RIGHT == aSide) {
      aSide = NS_SIDE_LEFT;
    }
    else if (NS_SIDE_LEFT == aSide) {
      aSide = NS_SIDE_RIGHT;
    }
  }
  width = styleData->GetActualBorderWidth(aSide);
  aWidth = nsPresContext::AppUnitsToIntCSSPixels(width);
}

class nsDelayedCalcBCBorders : public nsRunnable {
public:
  nsDelayedCalcBCBorders(nsIFrame* aFrame) :
    mFrame(aFrame) {}

  NS_IMETHOD Run() {
    if (mFrame) {
      nsTableFrame* tableFrame = static_cast <nsTableFrame*>(mFrame.GetFrame());
      if (tableFrame->NeedToCalcBCBorders()) {
        tableFrame->CalcBCBorders();
      }
    }
    return NS_OK;
  }
private:
  nsWeakFrame mFrame;
};
  
PRBool
nsTableFrame::BCRecalcNeeded(nsStyleContext* aOldStyleContext,
                             nsStyleContext* aNewStyleContext)
{
  
  
  

  const nsStyleBorder* oldStyleData = static_cast<const nsStyleBorder*>
                        (aOldStyleContext->PeekStyleData(eStyleStruct_Border));
  if (!oldStyleData)
    return PR_FALSE;

  const nsStyleBorder* newStyleData = aNewStyleContext->GetStyleBorder();
  nsChangeHint change = newStyleData->CalcDifference(*oldStyleData);
  if (!change)
    return PR_FALSE;
  if (change & nsChangeHint_ReflowFrame)
    return PR_TRUE; 
  if (change & nsChangeHint_RepaintFrame) {
    
    
    
    
    
    
    nsCOMPtr<nsIRunnable> evt = new nsDelayedCalcBCBorders(this);
    NS_DispatchToCurrentThread(evt);
    return PR_TRUE;
  }
  return PR_FALSE;
}





static const BCCellBorder&
CompareBorders(PRBool              aIsCorner, 
               const BCCellBorder& aBorder1,
               const BCCellBorder& aBorder2,
               PRBool              aSecondIsHorizontal,
               PRBool*             aFirstDominates = nsnull)
{
  PRBool firstDominates = PR_TRUE;
  
  if (NS_STYLE_BORDER_STYLE_HIDDEN == aBorder1.style) {
    firstDominates = (aIsCorner) ? PR_FALSE : PR_TRUE;
  }
  else if (NS_STYLE_BORDER_STYLE_HIDDEN == aBorder2.style) {
    firstDominates = (aIsCorner) ? PR_TRUE : PR_FALSE;
  }
  else if (aBorder1.width < aBorder2.width) {
    firstDominates = PR_FALSE;
  }
  else if (aBorder1.width == aBorder2.width) {
    if (styleToPriority[aBorder1.style] < styleToPriority[aBorder2.style]) {
      firstDominates = PR_FALSE;
    }
    else if (styleToPriority[aBorder1.style] == styleToPriority[aBorder2.style]) {
      if (aBorder1.owner == aBorder2.owner) {
        firstDominates = !aSecondIsHorizontal;
      }
      else if (aBorder1.owner < aBorder2.owner) {
        firstDominates = PR_FALSE;
      }
    }
  }

  if (aFirstDominates)
    *aFirstDominates = firstDominates;

  if (firstDominates)
    return aBorder1;
  return aBorder2;
}
























static BCCellBorder
CompareBorders(const nsIFrame*  aTableFrame,
               const nsIFrame*  aColGroupFrame,
               const nsIFrame*  aColFrame,
               const nsIFrame*  aRowGroupFrame,
               const nsIFrame*  aRowFrame,
               const nsIFrame*  aCellFrame,
               PRBool           aTableIsLTR,
               PRBool           aIgnoreTableEdge,
               PRUint8          aSide,
               PRBool           aAja)
{
  BCCellBorder border, tempBorder;
  PRBool horizontal = (NS_SIDE_TOP == aSide) || (NS_SIDE_BOTTOM == aSide);

  
  if (aTableFrame) {
    GetColorAndStyle(aTableFrame, aSide, border.style, border.color, aTableIsLTR, aIgnoreTableEdge, border.width);
    border.owner = eTableOwner;
    if (NS_STYLE_BORDER_STYLE_HIDDEN == border.style) {
      return border;
    }
  }
  
  if (aColGroupFrame) {
    GetColorAndStyle(aColGroupFrame, aSide, tempBorder.style, tempBorder.color, aTableIsLTR, aIgnoreTableEdge, tempBorder.width);
    tempBorder.owner = (aAja && !horizontal) ? eAjaColGroupOwner : eColGroupOwner;
    
    border = CompareBorders(!CELL_CORNER, border, tempBorder, PR_FALSE);
    if (NS_STYLE_BORDER_STYLE_HIDDEN == border.style) {
      return border;
    }
  }
  
  if (aColFrame) {
    GetColorAndStyle(aColFrame, aSide, tempBorder.style, tempBorder.color, aTableIsLTR, aIgnoreTableEdge, tempBorder.width);
    tempBorder.owner = (aAja && !horizontal) ? eAjaColOwner : eColOwner;
    border = CompareBorders(!CELL_CORNER, border, tempBorder, PR_FALSE);
    if (NS_STYLE_BORDER_STYLE_HIDDEN == border.style) {
      return border;
    }
  }
  
  if (aRowGroupFrame) {
    GetColorAndStyle(aRowGroupFrame, aSide, tempBorder.style, tempBorder.color, aTableIsLTR, aIgnoreTableEdge, tempBorder.width);
    tempBorder.owner = (aAja && horizontal) ? eAjaRowGroupOwner : eRowGroupOwner;
    border = CompareBorders(!CELL_CORNER, border, tempBorder, PR_FALSE);
    if (NS_STYLE_BORDER_STYLE_HIDDEN == border.style) {
      return border;
    }
  }
  
  if (aRowFrame) {
    GetColorAndStyle(aRowFrame, aSide, tempBorder.style, tempBorder.color, aTableIsLTR, aIgnoreTableEdge, tempBorder.width);
    tempBorder.owner = (aAja && horizontal) ? eAjaRowOwner : eRowOwner;
    border = CompareBorders(!CELL_CORNER, border, tempBorder, PR_FALSE);
    if (NS_STYLE_BORDER_STYLE_HIDDEN == border.style) {
      return border;
    }
  }
  
  if (aCellFrame) {
    GetColorAndStyle(aCellFrame, aSide, tempBorder.style, tempBorder.color, aTableIsLTR, aIgnoreTableEdge, tempBorder.width);
    tempBorder.owner = (aAja) ? eAjaCellOwner : eCellOwner;
    border = CompareBorders(!CELL_CORNER, border, tempBorder, PR_FALSE);
  }
  return border;
}

static PRBool 
Perpendicular(PRUint8 aSide1, 
              PRUint8 aSide2)
{
  switch (aSide1) {
  case NS_SIDE_TOP:
    return (NS_SIDE_BOTTOM != aSide2);
  case NS_SIDE_RIGHT:
    return (NS_SIDE_LEFT != aSide2);
  case NS_SIDE_BOTTOM:
    return (NS_SIDE_TOP != aSide2);
  default: 
    return (NS_SIDE_RIGHT != aSide2);
  }
}


struct BCCornerInfo 
{
  BCCornerInfo() { ownerColor = 0; ownerWidth = subWidth = ownerSide = ownerElem = subSide = 
                   subElem = hasDashDot = numSegs = bevel = 0;
                   ownerStyle = 0xFF; subStyle = NS_STYLE_BORDER_STYLE_SOLID;  }
  void Set(PRUint8       aSide,
           BCCellBorder  border);

  void Update(PRUint8       aSide,
              BCCellBorder  border);

  nscolor   ownerColor;     
  PRUint16  ownerWidth;     
  PRUint16  subWidth;       
                            
  PRUint32  ownerSide:2;    
                            
  PRUint32  ownerElem:3;    
  PRUint32  ownerStyle:8;   
  PRUint32  subSide:2;      
  PRUint32  subElem:3;      
  PRUint32  subStyle:8;     
  PRUint32  hasDashDot:1;   
  PRUint32  numSegs:3;      
  PRUint32  bevel:1;        
  PRUint32  unused:1;
};

void 
BCCornerInfo::Set(PRUint8       aSide,
                  BCCellBorder  aBorder)
{
  ownerElem  = aBorder.owner;
  ownerStyle = aBorder.style;
  ownerWidth = aBorder.width;
  ownerColor = aBorder.color;
  ownerSide  = aSide;
  hasDashDot = 0;
  numSegs    = 0;
  if (aBorder.width > 0) {
    numSegs++;
    hasDashDot = (NS_STYLE_BORDER_STYLE_DASHED == aBorder.style) ||
                 (NS_STYLE_BORDER_STYLE_DOTTED == aBorder.style);
  }
  bevel      = 0;
  subWidth   = 0;
  
  subSide    = ((aSide == NS_SIDE_LEFT) || (aSide == NS_SIDE_RIGHT)) ? NS_SIDE_TOP : NS_SIDE_LEFT; 
  subElem    = eTableOwner;
  subStyle   = NS_STYLE_BORDER_STYLE_SOLID; 
}

void 
BCCornerInfo::Update(PRUint8       aSide,
                     BCCellBorder  aBorder)
{
  PRBool existingWins = PR_FALSE;
  if (0xFF == ownerStyle) { 
    Set(aSide, aBorder);
  }
  else {
    PRBool horizontal = (NS_SIDE_LEFT == aSide) || (NS_SIDE_RIGHT == aSide); 
    BCCellBorder oldBorder, tempBorder;
    oldBorder.owner  = (BCBorderOwner) ownerElem;
    oldBorder.style =  ownerStyle;
    oldBorder.width =  ownerWidth;
    oldBorder.color =  ownerColor;

    PRUint8 oldSide  = ownerSide;
    
    tempBorder = CompareBorders(CELL_CORNER, oldBorder, aBorder, horizontal, &existingWins); 
                         
    ownerElem  = tempBorder.owner;
    ownerStyle = tempBorder.style;
    ownerWidth = tempBorder.width;
    ownerColor = tempBorder.color;
    if (existingWins) { 
      if (::Perpendicular(ownerSide, aSide)) {
        
        BCCellBorder subBorder;
        subBorder.owner = (BCBorderOwner) subElem;
        subBorder.style =  subStyle;
        subBorder.width =  subWidth;
        subBorder.color = 0; 
        PRBool firstWins;

        tempBorder = CompareBorders(CELL_CORNER, subBorder, aBorder, horizontal, &firstWins);
        
        subElem  = tempBorder.owner;
        subStyle = tempBorder.style;
        subWidth = tempBorder.width;
        if (!firstWins) {
          subSide = aSide; 
        }
      }
    }
    else { 
      ownerSide = aSide;
      if (::Perpendicular(oldSide, ownerSide)) {
        subElem  = oldBorder.owner;
        subStyle = oldBorder.style;
        subWidth = oldBorder.width;
        subSide  = oldSide;
      }
    }
    if (aBorder.width > 0) {
      numSegs++;
      if (!hasDashDot && ((NS_STYLE_BORDER_STYLE_DASHED == aBorder.style) ||
                          (NS_STYLE_BORDER_STYLE_DOTTED == aBorder.style))) {
        hasDashDot = 1;
      }
    }
  
    
    bevel = (2 == numSegs) && (subWidth > 1) && (0 == hasDashDot);
  }
}

struct BCCorners
{
  BCCorners(PRInt32 aNumCorners,
            PRInt32 aStartIndex);

  ~BCCorners() { delete [] corners; }
  
  BCCornerInfo& operator [](PRInt32 i) const
  { NS_ASSERTION((i >= startIndex) && (i <= endIndex), "program error");
    return corners[PR_MAX(PR_MIN(i, endIndex), startIndex) - startIndex]; }

  PRInt32       startIndex;
  PRInt32       endIndex;
  BCCornerInfo* corners;
};
  
BCCorners::BCCorners(PRInt32 aNumCorners,
                     PRInt32 aStartIndex)
{
  NS_ASSERTION((aNumCorners > 0) && (aStartIndex >= 0), "program error");
  startIndex = aStartIndex;
  endIndex   = aStartIndex + aNumCorners - 1;
  corners    = new BCCornerInfo[aNumCorners]; 
}


struct BCCellBorders
{
  BCCellBorders(PRInt32 aNumBorders,
                PRInt32 aStartIndex);

  ~BCCellBorders() { delete [] borders; }
  
  BCCellBorder& operator [](PRInt32 i) const
  { NS_ASSERTION((i >= startIndex) && (i <= endIndex), "program error");
    return borders[PR_MAX(PR_MIN(i, endIndex), startIndex) - startIndex]; }

  PRInt32       startIndex;
  PRInt32       endIndex;
  BCCellBorder* borders;
};
  
BCCellBorders::BCCellBorders(PRInt32 aNumBorders,
                             PRInt32 aStartIndex)
{
  NS_ASSERTION((aNumBorders > 0) && (aStartIndex >= 0), "program error");
  startIndex = aStartIndex;
  endIndex   = aStartIndex + aNumBorders - 1;
  borders    = new BCCellBorder[aNumBorders]; 
}



static PRBool
SetBorder(const BCCellBorder&   aNewBorder,
          BCCellBorder&         aBorder)
{
  PRBool changed = (aNewBorder.style != aBorder.style) ||
                   (aNewBorder.width != aBorder.width) ||
                   (aNewBorder.color != aBorder.color);
  aBorder.color        = aNewBorder.color;
  aBorder.width        = aNewBorder.width;
  aBorder.style        = aNewBorder.style;
  aBorder.owner        = aNewBorder.owner;

  return changed;
}




static PRBool
SetHorBorder(const BCCellBorder& aNewBorder,
             const BCCornerInfo& aCorner,
             BCCellBorder&       aBorder)
{
  PRBool startSeg = ::SetBorder(aNewBorder, aBorder);
  if (!startSeg) {
    startSeg = ((NS_SIDE_LEFT != aCorner.ownerSide) && (NS_SIDE_RIGHT != aCorner.ownerSide));
  }
  return startSeg;
}





void
nsTableFrame::ExpandBCDamageArea(nsRect& aRect) const
{
  PRInt32 numRows = GetRowCount();
  PRInt32 numCols = GetColCount();

  PRInt32 dStartX = aRect.x;
  PRInt32 dEndX   = aRect.XMost() - 1;
  PRInt32 dStartY = aRect.y;
  PRInt32 dEndY   = aRect.YMost() - 1;

  
  if (dStartX > 0) {
    dStartX--;
  }
  if (dEndX < (numCols - 1)) {
    dEndX++;
  }
  if (dStartY > 0) {
    dStartY--;
  }
  if (dEndY < (numRows - 1)) {
    dEndY++;
  }
  
  
  
  
  
  PRBool haveSpanner = PR_FALSE;
  if ((dStartX > 0) || (dEndX < (numCols - 1)) || (dStartY > 0) || (dEndY < (numRows - 1))) {
    nsTableCellMap* tableCellMap = GetCellMap(); if (!tableCellMap) ABORT0();
    
    RowGroupArray rowGroups;
    OrderRowGroups(rowGroups);

    
    nsCellMap* cellMap = nsnull;
    for (PRUint32 rgX = 0; rgX < rowGroups.Length(); rgX++) {
      nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
      PRInt32 rgStartY = rgFrame->GetStartRowIndex();
      PRInt32 rgEndY   = rgStartY + rgFrame->GetRowCount() - 1;
      if (dEndY < rgStartY) 
        break;
      cellMap = tableCellMap->GetMapFor(rgFrame, cellMap);
      if (!cellMap) ABORT0();
      
      if ((dStartY > 0) && (dStartY >= rgStartY) && (dStartY <= rgEndY)) {
        if (PRUint32(dStartY - rgStartY) >= cellMap->mRows.Length()) 
          ABORT0();
        const nsCellMap::CellDataArray& row =
          cellMap->mRows[dStartY - rgStartY];
        for (PRInt32 x = dStartX; x <= dEndX; x++) {
          CellData* cellData = row.SafeElementAt(x);
          if (cellData && (cellData->IsRowSpan())) {
             haveSpanner = PR_TRUE;
             break;
          }
        }
        if (dEndY < rgEndY) {
          if (PRUint32(dEndY + 1 - rgStartY) >= cellMap->mRows.Length()) 
            ABORT0();
          const nsCellMap::CellDataArray& row2 =
            cellMap->mRows[dEndY + 1 - rgStartY];
          for (PRInt32 x = dStartX; x <= dEndX; x++) {
            CellData* cellData = row2.SafeElementAt(x);
            if (cellData && (cellData->IsRowSpan())) {
              haveSpanner = PR_TRUE;
              break;
            }
          }
        }
      }
      
      PRInt32 iterStartY = -1;
      PRInt32 iterEndY   = -1;
      if ((dStartY >= rgStartY) && (dStartY <= rgEndY)) {
        
        iterStartY = dStartY;
        iterEndY   = PR_MIN(dEndY, rgEndY);
      }
      else if ((dEndY >= rgStartY) && (dEndY <= rgEndY)) {
        
        iterStartY = rgStartY;
        iterEndY   = PR_MIN(dEndY, rgStartY);
      }
      else if ((rgStartY >= dStartY) && (rgEndY <= dEndY)) {
        
        iterStartY = rgStartY;
        iterEndY   = rgEndY;
      }
      if ((iterStartY >= 0) && (iterEndY >= 0)) {
        for (PRInt32 y = iterStartY; y <= iterEndY; y++) {
          if (PRUint32(y - rgStartY) >= cellMap->mRows.Length()) 
            ABORT0();
          const nsCellMap::CellDataArray& row =
            cellMap->mRows[y - rgStartY];
          CellData* cellData = row.SafeElementAt(dStartX);
          if (cellData && (cellData->IsColSpan())) {
            haveSpanner = PR_TRUE;
            break;
          }
          if (dEndX < (numCols - 1)) {
            cellData = row.SafeElementAt(dEndX + 1);
            if (cellData && (cellData->IsColSpan())) {
              haveSpanner = PR_TRUE;
              break;
            }
          }
        }
      }
    }
  }
  if (haveSpanner) {
    
    aRect.x      = 0;
    aRect.y      = 0;
    aRect.width  = numCols;
    aRect.height = numRows;
  }
  else {
    aRect.x      = dStartX;
    aRect.y      = dStartY;
    aRect.width  = 1 + dEndX - dStartX;
    aRect.height = 1 + dEndY - dStartY;
  }
}

#define MAX_TABLE_BORDER_WIDTH 255
static PRUint8
LimitBorderWidth(PRUint16 aWidth)
{
  return PR_MIN(MAX_TABLE_BORDER_WIDTH, aWidth);
}

#define TABLE_EDGE  PR_TRUE
#define ADJACENT    PR_TRUE
#define HORIZONTAL  PR_TRUE

void
BCMapCellInfo::SetTableTopLeftContBCBorder()
{
  BCCellBorder currentBorder;
  
  
  if (mTopRow) {
    currentBorder = CompareBorders(mTableFrame, nsnull, nsnull, mRowGroup,
                                   mTopRow, nsnull, mTableIsLTR,
                                   TABLE_EDGE, NS_SIDE_TOP, !ADJACENT);
    mTopRow->SetContinuousBCBorderWidth(NS_SIDE_TOP, currentBorder.width);
  }
  if (mCgAtRight && mColGroup) {
    
    currentBorder = CompareBorders(mTableFrame, mColGroup, nsnull, mRowGroup,
                                   mTopRow, nsnull, mTableIsLTR,
                                   TABLE_EDGE, NS_SIDE_TOP, !ADJACENT);
    mColGroup->SetContinuousBCBorderWidth(NS_SIDE_TOP, currentBorder.width);
  }
  if (0 == mColIndex) {
    currentBorder = CompareBorders(mTableFrame, mColGroup, mLeftCol, nsnull,
                                   nsnull, nsnull, mTableIsLTR, TABLE_EDGE,
                                   NS_SIDE_LEFT, !ADJACENT);
    mTableFrame->SetContinuousLeftBCBorderWidth(currentBorder.width);
  }
}

void
BCMapCellInfo::SetRowGroupLeftContBCBorder()
{
  BCCellBorder currentBorder;
  
  if (mRgAtBottom && mRowGroup) { 
     currentBorder = CompareBorders(mTableFrame, mColGroup, mLeftCol, mRowGroup,
                                    nsnull, nsnull, mTableIsLTR, TABLE_EDGE,
                                    NS_SIDE_LEFT, !ADJACENT);
     mRowGroup->SetContinuousBCBorderWidth(mStartSide, currentBorder.width);
  }
}

void
BCMapCellInfo::SetRowGroupRightContBCBorder()
{
  BCCellBorder currentBorder;
  
  if (mRgAtBottom && mRowGroup) { 
    currentBorder = CompareBorders(mTableFrame, mColGroup, mRightCol, mRowGroup,
                                   nsnull, nsnull, mTableIsLTR, TABLE_EDGE,
                                   NS_SIDE_RIGHT, ADJACENT);
    mRowGroup->SetContinuousBCBorderWidth(mEndSide, currentBorder.width);
  }
}

void
BCMapCellInfo::SetColumnTopRightContBCBorder()
{
  BCCellBorder currentBorder;
  
  
  currentBorder = CompareBorders(mTableFrame, mCurrentColGroupFrame,
                                 mCurrentColFrame, mRowGroup, mTopRow, nsnull,
                                 mTableIsLTR, TABLE_EDGE, NS_SIDE_TOP,
                                 !ADJACENT);
  ((nsTableColFrame*) mCurrentColFrame)->SetContinuousBCBorderWidth(NS_SIDE_TOP,
                                                           currentBorder.width);
  if (mNumTableCols == GetCellEndColIndex() + 1) {
    currentBorder = CompareBorders(mTableFrame, mCurrentColGroupFrame,
                                   mCurrentColFrame, nsnull, nsnull, nsnull,
                                   mTableIsLTR, TABLE_EDGE, NS_SIDE_RIGHT,
                                   !ADJACENT);
  }
  else {
    currentBorder = CompareBorders(nsnull, mCurrentColGroupFrame,
                                   mCurrentColFrame, nsnull,nsnull, nsnull,
                                   mTableIsLTR, !TABLE_EDGE, NS_SIDE_RIGHT,
                                   !ADJACENT);
  }
  mCurrentColFrame->SetContinuousBCBorderWidth(NS_SIDE_RIGHT,
                                               currentBorder.width);
}

void
BCMapCellInfo::SetColumnBottomContBCBorder()
{
  BCCellBorder currentBorder;
  
  currentBorder = CompareBorders(mTableFrame, mCurrentColGroupFrame,
                                 mCurrentColFrame, mRowGroup, mBottomRow,
                                 nsnull, mTableIsLTR, TABLE_EDGE,
                                 NS_SIDE_BOTTOM, ADJACENT);
  mCurrentColFrame->SetContinuousBCBorderWidth(NS_SIDE_BOTTOM,
                                               currentBorder.width);
}

void
BCMapCellInfo::SetColGroupBottomContBCBorder()
{
  BCCellBorder currentBorder;
  if (mColGroup) {
    currentBorder = CompareBorders(mTableFrame, mColGroup, nsnull, mRowGroup,
                                   mBottomRow, nsnull, mTableIsLTR, TABLE_EDGE,
                                   NS_SIDE_BOTTOM, ADJACENT);
    mColGroup->SetContinuousBCBorderWidth(NS_SIDE_BOTTOM, currentBorder.width);
  }
}

void
BCMapCellInfo::SetRowGroupBottomContBCBorder()
{
  BCCellBorder currentBorder;
  if (mRowGroup) {
    currentBorder = CompareBorders(mTableFrame, nsnull, nsnull, mRowGroup,
                                   mBottomRow, nsnull, mTableIsLTR, TABLE_EDGE,
                                   NS_SIDE_BOTTOM, ADJACENT);
    mRowGroup->SetContinuousBCBorderWidth(NS_SIDE_BOTTOM, currentBorder.width);
  }
}

void
BCMapCellInfo::SetInnerRowGroupBottomContBCBorder(const nsIFrame* aNextRowGroup,
                                                  nsTableRowFrame* aNextRow)
{
  BCCellBorder currentBorder, adjacentBorder;
  
  const nsIFrame* rowgroup = (mRgAtBottom) ? mRowGroup : nsnull;
  currentBorder = CompareBorders(nsnull, nsnull, nsnull, rowgroup, mBottomRow,
                                 nsnull, mTableIsLTR, !TABLE_EDGE,
                                 NS_SIDE_BOTTOM, ADJACENT);

  adjacentBorder = CompareBorders(nsnull, nsnull, nsnull, aNextRowGroup,
                                  aNextRow, nsnull, mTableIsLTR, !TABLE_EDGE,
                                  NS_SIDE_TOP, !ADJACENT);
  currentBorder = CompareBorders(PR_FALSE, currentBorder, adjacentBorder,
                                 HORIZONTAL);
  if (aNextRow) {
    aNextRow->SetContinuousBCBorderWidth(NS_SIDE_TOP, currentBorder.width);
  }
  if (mRgAtBottom && mRowGroup) {
    mRowGroup->SetContinuousBCBorderWidth(NS_SIDE_BOTTOM, currentBorder.width);
  }
}

void
BCMapCellInfo::SetRowLeftContBCBorder()
{
  
  if (mCurrentRowFrame) {
    BCCellBorder currentBorder;
    currentBorder = CompareBorders(mTableFrame, mColGroup, mLeftCol, mRowGroup,
                                   mCurrentRowFrame, nsnull, mTableIsLTR,
                                   TABLE_EDGE, NS_SIDE_LEFT, !ADJACENT);
    mCurrentRowFrame->SetContinuousBCBorderWidth(mStartSide,
                                                 currentBorder.width);
  }
}

void
BCMapCellInfo::SetRowRightContBCBorder()
{
  if (mCurrentRowFrame) {
    BCCellBorder currentBorder;
    currentBorder = CompareBorders(mTableFrame, mColGroup, mRightCol, mRowGroup,
                                   mCurrentRowFrame, nsnull, mTableIsLTR,
                                   TABLE_EDGE, NS_SIDE_RIGHT, ADJACENT);
    mCurrentRowFrame->SetContinuousBCBorderWidth(mEndSide,
                                                 currentBorder.width);
  }
}
void
BCMapCellInfo::SetTableTopBorderWidth(BCPixelSize aWidth)
{
  mTableBCData->mTopBorderWidth =
     LimitBorderWidth(PR_MAX(mTableBCData->mTopBorderWidth, (PRUint8) aWidth));
}

void
BCMapCellInfo::SetTableLeftBorderWidth(PRInt32 aRowY, BCPixelSize aWidth)
{
  
  if (aRowY == 0) {
    if (mTableIsLTR) {
      mTableBCData->mLeftCellBorderWidth = aWidth;
    }
    else {
      mTableBCData->mRightCellBorderWidth = aWidth;
    }
  }
  mTableBCData->mLeftBorderWidth =
               LimitBorderWidth(PR_MAX(mTableBCData->mLeftBorderWidth, aWidth));
}

void
BCMapCellInfo::SetTableRightBorderWidth(PRInt32 aRowY, BCPixelSize aWidth)
{
  
  if (aRowY == 0) {
    if (mTableIsLTR) {
      mTableBCData->mRightCellBorderWidth = aWidth;
    }
    else {
      mTableBCData->mLeftCellBorderWidth = aWidth;
    }
  }
  mTableBCData->mRightBorderWidth =
              LimitBorderWidth(PR_MAX(mTableBCData->mRightBorderWidth, aWidth));
}

void
BCMapCellInfo::SetRightBorderWidths(BCPixelSize aWidth)
{
   
  if (mCell) {
    mCell->SetBorderWidth(mEndSide, PR_MAX(aWidth,
                          mCell->GetBorderWidth(mEndSide)));
  }
  if (mRightCol) {
    BCPixelSize half = BC_BORDER_LEFT_HALF(aWidth);
    mRightCol->SetRightBorderWidth(PR_MAX(half,
                                   mRightCol->GetRightBorderWidth()));
  }
}

void
BCMapCellInfo::SetBottomBorderWidths(BCPixelSize aWidth)
{
  
  if (mCell) {
    mCell->SetBorderWidth(NS_SIDE_BOTTOM, PR_MAX(aWidth,
                          mCell->GetBorderWidth(NS_SIDE_BOTTOM)));
  }
  if (mBottomRow) {
    BCPixelSize half = BC_BORDER_TOP_HALF(aWidth);
    mBottomRow->SetBottomBCBorderWidth(PR_MAX(half,
                                       mBottomRow->GetBottomBCBorderWidth()));
  }
}
void
BCMapCellInfo::SetTopBorderWidths(BCPixelSize aWidth)
{
 if (mCell) {
     mCell->SetBorderWidth(NS_SIDE_TOP, PR_MAX(aWidth,
                           mCell->GetBorderWidth(NS_SIDE_TOP)));
  }
  if (mTopRow) {
    BCPixelSize half = BC_BORDER_BOTTOM_HALF(aWidth);
    mTopRow->SetTopBCBorderWidth(PR_MAX(half, mTopRow->GetTopBCBorderWidth()));
  }
}
void
BCMapCellInfo::SetLeftBorderWidths(BCPixelSize aWidth)
{
  if (mCell) {
    mCell->SetBorderWidth(mStartSide, PR_MAX(aWidth,
                          mCell->GetBorderWidth(mStartSide)));
  }
  if (mLeftCol) {
    BCPixelSize half = BC_BORDER_RIGHT_HALF(aWidth);
    mLeftCol->SetLeftBorderWidth(PR_MAX(half, mLeftCol->GetLeftBorderWidth()));
  }
}

void
BCMapCellInfo::SetTableBottomBorderWidth(BCPixelSize aWidth)
{
  mTableBCData->mBottomBorderWidth =
             LimitBorderWidth(PR_MAX(mTableBCData->mBottomBorderWidth, aWidth));
}

void
BCMapCellInfo::SetColumn(PRInt32 aColX)
{
  mCurrentColFrame = mTableFrame->GetColFrame(aColX);
  if (!mCurrentColFrame) {
    NS_ERROR("null mCurrentColFrame");
  }
  mCurrentColGroupFrame = static_cast<nsTableColGroupFrame*>
                            (mCurrentColFrame->GetParent());
  if (!mCurrentColGroupFrame) {
    NS_ERROR("null mCurrentColGroupFrame");
  }
}

void
BCMapCellInfo::IncrementRow(PRBool aResetToTopRowOfCell)
{
  mCurrentRowFrame = (aResetToTopRowOfCell) ? mTopRow :
                                                mCurrentRowFrame->GetNextRow();
}

BCCellBorder
BCMapCellInfo::GetTopEdgeBorder()
{
  return CompareBorders(mTableFrame, mCurrentColGroupFrame, mCurrentColFrame,
                        mRowGroup, mTopRow, mCell, mTableIsLTR, TABLE_EDGE,
                        NS_SIDE_TOP, !ADJACENT);
}

BCCellBorder
BCMapCellInfo::GetBottomEdgeBorder()
{
  return CompareBorders(mTableFrame, mCurrentColGroupFrame, mCurrentColFrame,
                        mRowGroup, mBottomRow, mCell, mTableIsLTR, TABLE_EDGE,
                        NS_SIDE_BOTTOM, ADJACENT);
}
BCCellBorder
BCMapCellInfo::GetLeftEdgeBorder()
{
  return CompareBorders(mTableFrame, mColGroup, mLeftCol, mRowGroup,
                        mCurrentRowFrame, mCell, mTableIsLTR, TABLE_EDGE,
                        NS_SIDE_LEFT, !ADJACENT);
}
BCCellBorder
BCMapCellInfo::GetRightEdgeBorder()
{
  return CompareBorders(mTableFrame, mColGroup, mRightCol, mRowGroup,
                        mCurrentRowFrame, mCell, mTableIsLTR, TABLE_EDGE,
                        NS_SIDE_RIGHT, ADJACENT);
}
BCCellBorder
BCMapCellInfo::GetRightInternalBorder()
{
  const nsIFrame* cg = (mCgAtRight) ? mColGroup : nsnull;
  return CompareBorders(nsnull, cg, mRightCol, nsnull, nsnull, mCell,
                        mTableIsLTR, !TABLE_EDGE, NS_SIDE_RIGHT, ADJACENT);
}

BCCellBorder
BCMapCellInfo::GetLeftInternalBorder()
{
  const nsIFrame* cg = (mCgAtLeft) ? mColGroup : nsnull;
  return CompareBorders(nsnull, cg, mLeftCol, nsnull, nsnull, mCell,
                        mTableIsLTR, !TABLE_EDGE, NS_SIDE_LEFT, !ADJACENT);
}

BCCellBorder
BCMapCellInfo::GetBottomInternalBorder()
{
  const nsIFrame* rg = (mRgAtBottom) ? mRowGroup : nsnull;
  return CompareBorders(nsnull, nsnull, nsnull, rg, mBottomRow, mCell,
                        mTableIsLTR, !TABLE_EDGE, NS_SIDE_BOTTOM, ADJACENT);
}

BCCellBorder
BCMapCellInfo::GetTopInternalBorder()
{
  const nsIFrame* rg = (mRgAtTop) ? mRowGroup : nsnull;
  return CompareBorders(nsnull, nsnull, nsnull, rg, mTopRow, mCell,
                        mTableIsLTR, !TABLE_EDGE, NS_SIDE_TOP, !ADJACENT);
}












































void 
nsTableFrame::CalcBCBorders()
{
  NS_ASSERTION(IsBorderCollapse(),
               "calling CalcBCBorders on separated-border table");
  nsTableCellMap* tableCellMap = GetCellMap(); if (!tableCellMap) ABORT0();
  PRInt32 numRows = GetRowCount();
  PRInt32 numCols = GetColCount();
  if (!numRows || !numCols)
    return; 

  
  BCPropertyData* propData =
    (BCPropertyData*)nsTableFrame::GetProperty(this, nsGkAtoms::tableBCProperty,
                                               PR_FALSE);
  if (!propData) ABORT0();

  
  
  CheckFixDamageArea(numRows, numCols, propData->mDamageArea);
  
  nsRect damageArea(propData->mDamageArea);
  ExpandBCDamageArea(damageArea);

  
  
  PRBool tableBorderReset[4];
  for (PRUint32 sideX = NS_SIDE_TOP; sideX <= NS_SIDE_LEFT; sideX++) {
    tableBorderReset[sideX] = PR_FALSE;
  }

  
  BCCellBorders lastVerBorders(damageArea.width + 1, damageArea.x);
  if (!lastVerBorders.borders) ABORT0();
  BCCellBorder  lastTopBorder, lastBottomBorder;
  
  BCCellBorders lastBottomBorders(damageArea.width + 1, damageArea.x);
  if (!lastBottomBorders.borders) ABORT0();
  PRBool startSeg;
  PRBool gotRowBorder = PR_FALSE;

  BCMapCellInfo  info(this), ajaInfo(this);

  BCCellBorder currentBorder, adjacentBorder;
  BCCorners topCorners(damageArea.width + 1, damageArea.x);
  if (!topCorners.corners) ABORT0();
  BCCorners bottomCorners(damageArea.width + 1, damageArea.x);
  if (!bottomCorners.corners) ABORT0();

  BCMapCellIterator iter(this, damageArea);
  for (iter.First(info); !iter.mAtEnd; iter.Next(info)) {
    PRBool bottomRowSpan = PR_FALSE;
    
    if (iter.IsNewRow()) { 
      gotRowBorder = PR_FALSE;
      lastTopBorder.Reset(info.mRowIndex, info.mRowSpan);
      lastBottomBorder.Reset(info.GetCellEndRowIndex() + 1, info.mRowSpan);
    }
    else if (info.mColIndex > damageArea.x) {
      lastBottomBorder = lastBottomBorders[info.mColIndex - 1];
      if (info.mRowIndex >
          (lastBottomBorder.rowIndex - lastBottomBorder.rowSpan)) {
        
        lastTopBorder.Reset(info.mRowIndex, info.mRowSpan);
      }
      if (lastBottomBorder.rowIndex > (info.GetCellEndRowIndex() + 1)) {
        
        lastBottomBorder.Reset(info.GetCellEndRowIndex() + 1, info.mRowSpan);
        bottomRowSpan = PR_TRUE;
      }
    }

    
    
    
    if (0 == info.mRowIndex) {
      if (!tableBorderReset[NS_SIDE_TOP]) {
        propData->mTopBorderWidth = 0;
        tableBorderReset[NS_SIDE_TOP] = PR_TRUE;
      }
      for (PRInt32 colX = info.mColIndex; colX <= info.GetCellEndColIndex();
           colX++) {
        info.SetColumn(colX);
        currentBorder = info.GetTopEdgeBorder();
        
        BCCornerInfo& tlCorner = topCorners[colX]; 
        if (0 == colX) {
          
          tlCorner.Set(NS_SIDE_RIGHT, currentBorder);
        }
        else {
          tlCorner.Update(NS_SIDE_RIGHT, currentBorder);
          tableCellMap->SetBCBorderCorner(eTopLeft, *iter.mCellMap, 0, 0, colX,
                                          tlCorner.ownerSide, tlCorner.subWidth,
                                          tlCorner.bevel);
        }
        topCorners[colX + 1].Set(NS_SIDE_LEFT, currentBorder); 
        
        startSeg = SetHorBorder(currentBorder, tlCorner, lastTopBorder);
        
        tableCellMap->SetBCBorderEdge(NS_SIDE_TOP, *iter.mCellMap, 0, 0, colX,
                                      1, currentBorder.owner,
                                      currentBorder.width, startSeg);
       
        info.SetTableTopBorderWidth(currentBorder.width);
        info.SetTopBorderWidths(currentBorder.width);
        info.SetColumnTopRightContBCBorder();
      }
      info.SetTableTopLeftContBCBorder();
    }
    else {
      
      
      if (info.mColIndex > 0) {
        BCData& data = info.mCellData->mData;
        if (!data.IsTopStart()) {
          PRUint8 cornerSide;
          PRPackedBool bevel;
          data.GetCorner(cornerSide, bevel);
          if ((NS_SIDE_TOP == cornerSide) || (NS_SIDE_BOTTOM == cornerSide)) {
            data.SetTopStart(PR_TRUE);
          }
        }
      }  
    }

    
    
    
    if (0 == info.mColIndex) {
      if (!tableBorderReset[NS_SIDE_LEFT]) {
        propData->mLeftBorderWidth = 0;
        tableBorderReset[NS_SIDE_LEFT] = PR_TRUE;
      }
      info.mCurrentRowFrame = nsnull;
      for (PRInt32 rowY = info.mRowIndex; rowY <= info.GetCellEndRowIndex();
           rowY++) {
        info.IncrementRow(rowY == info.mRowIndex);
        currentBorder = info.GetLeftEdgeBorder();
        BCCornerInfo& tlCorner = (0 == rowY) ? topCorners[0] : bottomCorners[0];
        tlCorner.Update(NS_SIDE_BOTTOM, currentBorder);
        tableCellMap->SetBCBorderCorner(eTopLeft, *iter.mCellMap,
                                        iter.mRowGroupStart, rowY, 0,
                                        tlCorner.ownerSide, tlCorner.subWidth,
                                        tlCorner.bevel);
        bottomCorners[0].Set(NS_SIDE_TOP, currentBorder); 
       
        
        startSeg = SetBorder(currentBorder, lastVerBorders[0]);
        
        tableCellMap->SetBCBorderEdge(NS_SIDE_LEFT, *iter.mCellMap,
                                      iter.mRowGroupStart, rowY, info.mColIndex,
                                      1, currentBorder.owner,
                                      currentBorder.width, startSeg);
        info.SetTableLeftBorderWidth(rowY , currentBorder.width);
        info.SetLeftBorderWidths(currentBorder.width);
        info.SetRowLeftContBCBorder();
      }
      info.SetRowGroupLeftContBCBorder();
    }

    
    
    if (info.mNumTableCols == info.GetCellEndColIndex() + 1) {
      
      if (!tableBorderReset[NS_SIDE_RIGHT]) {
        propData->mRightBorderWidth = 0;
        tableBorderReset[NS_SIDE_RIGHT] = PR_TRUE;
      }
      info.mCurrentRowFrame = nsnull;
      for (PRInt32 rowY = info.mRowIndex; rowY <= info.GetCellEndRowIndex();
           rowY++) {
        info.IncrementRow(rowY == info.mRowIndex);
        currentBorder = info.GetRightEdgeBorder();
        
        BCCornerInfo& trCorner = (0 == rowY) ?
                                 topCorners[info.GetCellEndColIndex() + 1] :
                                 bottomCorners[info.GetCellEndColIndex() + 1];
        trCorner.Update(NS_SIDE_BOTTOM, currentBorder);   
        tableCellMap->SetBCBorderCorner(eTopRight, *iter.mCellMap,
                                        iter.mRowGroupStart, rowY,
                                        info.GetCellEndColIndex(),
                                        trCorner.ownerSide, trCorner.subWidth,
                                        trCorner.bevel);
        BCCornerInfo& brCorner = bottomCorners[info.GetCellEndColIndex() + 1];
        brCorner.Set(NS_SIDE_TOP, currentBorder); 
        tableCellMap->SetBCBorderCorner(eBottomRight, *iter.mCellMap,
                                        iter.mRowGroupStart, rowY,
                                        info.GetCellEndColIndex(),
                                        brCorner.ownerSide, brCorner.subWidth,
                                        brCorner.bevel);
        
        startSeg = SetBorder(currentBorder,
                             lastVerBorders[info.GetCellEndColIndex() + 1]);
        
        tableCellMap->SetBCBorderEdge(NS_SIDE_RIGHT, *iter.mCellMap,
                                      iter.mRowGroupStart, rowY,
                                      info.GetCellEndColIndex(), 1,
                                      currentBorder.owner, currentBorder.width,
                                      startSeg);
        info.SetTableRightBorderWidth(rowY, currentBorder.width);
        info.SetRightBorderWidths(currentBorder.width);
        info.SetRowRightContBCBorder();
      }
      info.SetRowGroupRightContBCBorder();
    }
    else {
      PRInt32 segLength = 0;
      BCMapCellInfo priorAjaInfo(this);
      for (PRInt32 rowY = info.mRowIndex; rowY <= info.GetCellEndRowIndex();
           rowY += segLength) {
        iter.PeekRight(info, rowY, ajaInfo);
        currentBorder  = info.GetRightInternalBorder();
        adjacentBorder = ajaInfo.GetLeftInternalBorder();
        currentBorder = CompareBorders(!CELL_CORNER, currentBorder,
                                        adjacentBorder, !HORIZONTAL);
                          
        segLength = PR_MAX(1, ajaInfo.mRowIndex + ajaInfo.mRowSpan - rowY);
        segLength = PR_MIN(segLength, info.mRowIndex + info.mRowSpan - rowY);

        
        startSeg = SetBorder(currentBorder,
                             lastVerBorders[info.GetCellEndColIndex() + 1]);
        
        if (info.GetCellEndColIndex() < damageArea.XMost() &&
            rowY >= damageArea.y && rowY < damageArea.YMost()) {
          tableCellMap->SetBCBorderEdge(NS_SIDE_RIGHT, *iter.mCellMap,
                                        iter.mRowGroupStart, rowY,
                                        info.GetCellEndColIndex(), segLength,
                                        currentBorder.owner,
                                        currentBorder.width, startSeg);
          info.SetRightBorderWidths(currentBorder.width);
          ajaInfo.SetLeftBorderWidths(currentBorder.width);
        }
        
        PRBool hitsSpanOnRight = (rowY > ajaInfo.mRowIndex) &&
                                  (rowY < ajaInfo.mRowIndex + ajaInfo.mRowSpan);
        BCCornerInfo* trCorner = ((0 == rowY) || hitsSpanOnRight) ?
                                 &topCorners[info.GetCellEndColIndex() + 1] :
                                 &bottomCorners[info.GetCellEndColIndex() + 1];
        trCorner->Update(NS_SIDE_BOTTOM, currentBorder);
        
        
        if (rowY != info.mRowIndex) {
          currentBorder  = priorAjaInfo.GetBottomInternalBorder();
          adjacentBorder = ajaInfo.GetTopInternalBorder();
          currentBorder = CompareBorders(!CELL_CORNER, currentBorder,
                                          adjacentBorder, HORIZONTAL);
          trCorner->Update(NS_SIDE_RIGHT, currentBorder);
        }
        
        if (info.GetCellEndColIndex() < damageArea.XMost() &&
            rowY >= damageArea.y) {
          if (0 != rowY) {
            tableCellMap->SetBCBorderCorner(eTopRight, *iter.mCellMap,
                                            iter.mRowGroupStart, rowY,
                                            info.GetCellEndColIndex(),
                                            trCorner->ownerSide,
                                            trCorner->subWidth,
                                            trCorner->bevel);
          }
          
          for (PRInt32 rX = rowY + 1; rX < rowY + segLength; rX++) {
            tableCellMap->SetBCBorderCorner(eBottomRight, *iter.mCellMap,
                                            iter.mRowGroupStart, rX,
                                            info.GetCellEndColIndex(),
                                            trCorner->ownerSide,
                                            trCorner->subWidth, PR_FALSE);
          }
        }
        
        hitsSpanOnRight = (rowY + segLength <
                           ajaInfo.mRowIndex + ajaInfo.mRowSpan);
        BCCornerInfo& brCorner = (hitsSpanOnRight) ?
                                 topCorners[info.GetCellEndColIndex() + 1] :
                                 bottomCorners[info.GetCellEndColIndex() + 1];
        brCorner.Set(NS_SIDE_TOP, currentBorder);
        priorAjaInfo = ajaInfo;
      }
    }
    for (PRInt32 colX = info.mColIndex + 1; colX <= info.GetCellEndColIndex();
         colX++) {
      lastVerBorders[colX].Reset(0,1);
    }

    
    
    if (info.mNumTableRows == info.GetCellEndRowIndex() + 1) {
      
      if (!tableBorderReset[NS_SIDE_BOTTOM]) {
        propData->mBottomBorderWidth = 0;
        tableBorderReset[NS_SIDE_BOTTOM] = PR_TRUE;
      }
      for (PRInt32 colX = info.mColIndex; colX <= info.GetCellEndColIndex();
           colX++) {
        info.SetColumn(colX);
        currentBorder = info.GetBottomEdgeBorder();
        
        BCCornerInfo& blCorner = bottomCorners[colX]; 
        blCorner.Update(NS_SIDE_RIGHT, currentBorder);
        tableCellMap->SetBCBorderCorner(eBottomLeft, *iter.mCellMap,
                                        iter.mRowGroupStart,
                                        info.GetCellEndRowIndex(),
                                        colX, blCorner.ownerSide,
                                        blCorner.subWidth, blCorner.bevel);
        BCCornerInfo& brCorner = bottomCorners[colX + 1]; 
        brCorner.Update(NS_SIDE_LEFT, currentBorder);
        if (info.mNumTableCols == colX + 1) { 
          tableCellMap->SetBCBorderCorner(eBottomRight, *iter.mCellMap,
                                          iter.mRowGroupStart,
                                          info.GetCellEndRowIndex(),colX,
                                          brCorner.ownerSide, brCorner.subWidth,
                                          brCorner.bevel, PR_TRUE);
        }
        
        startSeg = SetHorBorder(currentBorder, blCorner, lastBottomBorder);
        if (!startSeg) { 
           
           
           
           
           startSeg = (lastBottomBorder.rowIndex !=
                       (info.GetCellEndRowIndex() + 1));
        }
        
        tableCellMap->SetBCBorderEdge(NS_SIDE_BOTTOM, *iter.mCellMap,
                                      iter.mRowGroupStart,
                                      info.GetCellEndRowIndex(),
                                      colX, 1, currentBorder.owner,
                                      currentBorder.width, startSeg);
        
        lastBottomBorder.rowIndex = info.GetCellEndRowIndex() + 1;
        lastBottomBorder.rowSpan = info.mRowSpan;
        lastBottomBorders[colX] = lastBottomBorder;
        
        info.SetBottomBorderWidths(currentBorder.width);
        info.SetTableBottomBorderWidth(currentBorder.width);
        info.SetColumnBottomContBCBorder();
      }
      info.SetRowGroupBottomContBCBorder();
      info.SetColGroupBottomContBCBorder();
    }
    else {
      PRInt32 segLength = 0;
      for (PRInt32 colX = info.mColIndex; colX <= info.GetCellEndColIndex();
           colX += segLength) {
        iter.PeekBottom(info, colX, ajaInfo);
        currentBorder  = info.GetBottomInternalBorder();
        adjacentBorder = ajaInfo.GetTopInternalBorder();
        currentBorder = CompareBorders(!CELL_CORNER, currentBorder,
                                        adjacentBorder, HORIZONTAL);
        segLength = PR_MAX(1, ajaInfo.mColIndex + ajaInfo.mColSpan - colX);
        segLength = PR_MIN(segLength, info.mColIndex + info.mColSpan - colX);

        
        BCCornerInfo& blCorner = bottomCorners[colX]; 
        PRBool hitsSpanBelow = (colX > ajaInfo.mColIndex) &&
                               (colX < ajaInfo.mColIndex + ajaInfo.mColSpan);
        PRBool update = PR_TRUE;
        if ((colX == info.mColIndex) && (colX > damageArea.x)) {
          PRInt32 prevRowIndex = lastBottomBorders[colX - 1].rowIndex;
          if (prevRowIndex > info.GetCellEndRowIndex() + 1) {
            
            update = PR_FALSE;
            
          }
          else if (prevRowIndex < info.GetCellEndRowIndex() + 1) {
            
            topCorners[colX] = blCorner;
            blCorner.Set(NS_SIDE_RIGHT, currentBorder);
            update = PR_FALSE;
          }
        }
        if (update) {
          blCorner.Update(NS_SIDE_RIGHT, currentBorder);
        }
        if (info.GetCellEndRowIndex() < damageArea.YMost() &&
            (colX >= damageArea.x)) {
          if (hitsSpanBelow) {
            tableCellMap->SetBCBorderCorner(eBottomLeft, *iter.mCellMap,
                                            iter.mRowGroupStart,
                                            info.GetCellEndRowIndex(), colX,
                                            blCorner.ownerSide,
                                            blCorner.subWidth, blCorner.bevel);
          }
          
          for (PRInt32 cX = colX + 1; cX < colX + segLength; cX++) {
            BCCornerInfo& corner = bottomCorners[cX];
            corner.Set(NS_SIDE_RIGHT, currentBorder);
            tableCellMap->SetBCBorderCorner(eBottomLeft, *iter.mCellMap,
                                            iter.mRowGroupStart,
                                            info.GetCellEndRowIndex(), cX,
                                            corner.ownerSide, corner.subWidth,
                                            PR_FALSE);
          }
        }
        
        startSeg = SetHorBorder(currentBorder, blCorner, lastBottomBorder);
        if (!startSeg) { 
           
           
           
           
           startSeg = (lastBottomBorder.rowIndex !=
                       info.GetCellEndRowIndex() + 1);
        }
        lastBottomBorder.rowIndex = info.GetCellEndRowIndex() + 1;
        lastBottomBorder.rowSpan = info.mRowSpan;
        for (PRInt32 cX = colX; cX < colX + segLength; cX++) {
          lastBottomBorders[cX] = lastBottomBorder;
        }

        
        if (info.GetCellEndRowIndex() < damageArea.YMost() &&
            (colX >= damageArea.x) &&
            (colX < damageArea.XMost())) {
          tableCellMap->SetBCBorderEdge(NS_SIDE_BOTTOM, *iter.mCellMap,
                                        iter.mRowGroupStart,
                                        info.GetCellEndRowIndex(),
                                        colX, segLength, currentBorder.owner,
                                        currentBorder.width, startSeg);
          info.SetBottomBorderWidths(currentBorder.width);
          ajaInfo.SetTopBorderWidths(currentBorder.width);
        }
        
        BCCornerInfo& brCorner = bottomCorners[colX + segLength];
        brCorner.Update(NS_SIDE_LEFT, currentBorder);
      }
      if (!gotRowBorder && 1 == info.mRowSpan &&
          (ajaInfo.mTopRow || info.mRgAtBottom)) {
        
        
        
        
        const nsIFrame* nextRowGroup = (ajaInfo.mRgAtTop) ? ajaInfo.mRowGroup :
                                                             nsnull;
        info.SetInnerRowGroupBottomContBCBorder(nextRowGroup, ajaInfo.mTopRow);
        gotRowBorder = PR_TRUE;
      }
    }

    
    
    
    if ((info.mNumTableCols != info.GetCellEndColIndex() + 1) &&
        (lastBottomBorders[info.GetCellEndColIndex() + 1].rowSpan > 1)) {
      BCCornerInfo& corner = bottomCorners[info.GetCellEndColIndex() + 1];
      if ((NS_SIDE_TOP != corner.ownerSide) &&
          (NS_SIDE_BOTTOM != corner.ownerSide)) {
        
        BCCellBorder& thisBorder = lastBottomBorder;
        BCCellBorder& nextBorder = lastBottomBorders[info.mColIndex + 1];
        if ((thisBorder.color == nextBorder.color) &&
            (thisBorder.width == nextBorder.width) &&
            (thisBorder.style == nextBorder.style)) {
          
          
          if (iter.mCellMap) {
            tableCellMap->SetNotTopStart(NS_SIDE_BOTTOM, *iter.mCellMap,
                                         info.GetCellEndRowIndex(),
                                         info.GetCellEndColIndex() + 1);
          }
        }
      }
    }
  } 

  
  SetNeedToCalcBCBorders(PR_FALSE);
  propData->mDamageArea.x = propData->mDamageArea.y = propData->mDamageArea.width = propData->mDamageArea.height = 0;
#ifdef DEBUG_TABLE_CELLMAP
  mCellMap->Dump();
#endif
}




class BCMapBorderIterator
{
public:
  BCMapBorderIterator(nsTableFrame&         aTableFrame,
                      nsTableRowGroupFrame& aRowGroupFrame,
                      nsTableRowFrame&      aRowFrame,
                      const nsRect&         aDamageArea);
  void Reset(nsTableFrame&         aTableFrame,
             nsTableRowGroupFrame& aRowGroupFrame,
             nsTableRowFrame&      aRowFrame,
             const nsRect&         aDamageArea);
  void First();
  void Next();

  nsTableFrame*         table;
  nsTableCellMap*       tableCellMap;
  nsCellMap*            cellMap;

  nsTableFrame::RowGroupArray rowGroups;
  nsTableRowGroupFrame* prevRg;
  nsTableRowGroupFrame* rg;
  PRInt32               rowGroupIndex;
  PRInt32               fifRowGroupStart;
  PRInt32               rowGroupStart;
  PRInt32               rowGroupEnd;
  PRInt32               numRows; 

  nsTableRowFrame*      prevRow;
  nsTableRowFrame*      row;
  PRInt32               numCols;
  PRInt32               x;
  PRInt32               y;

  nsTableCellFrame*     prevCell;
  nsTableCellFrame*     cell;  
  BCCellData*           prevCellData;
  BCCellData*           cellData;
  BCData*               bcData;

  PRBool                IsTopMostTable()    { return (y == 0) && !table->GetPrevInFlow(); }
  PRBool                IsRightMostTable()  { return (x >= numCols); }
  PRBool                IsBottomMostTable() { return (y >= numRows) && !table->GetNextInFlow(); }
  PRBool                IsLeftMostTable()   { return (x == 0); }
  PRBool                IsTopMost()    { return (y == startY); }
  PRBool                IsRightMost()  { return (x >= endX); }
  PRBool                IsBottomMost() { return (y >= endY); }
  PRBool                IsLeftMost()   { return (x == startX); }
  PRBool                isNewRow;

  PRInt32               startX;
  PRInt32               startY;
  PRInt32               endX;
  PRInt32               endY;
  PRBool                isRepeatedHeader;
  PRBool                isRepeatedFooter;
  PRBool                atEnd;

private:

  PRBool SetNewRow(nsTableRowFrame* aRow = nsnull);
  PRBool SetNewRowGroup();
  void   SetNewData(PRInt32 aY, PRInt32 aX);

};

BCMapBorderIterator::BCMapBorderIterator(nsTableFrame&         aTable,
                                         nsTableRowGroupFrame& aRowGroup,
                                         nsTableRowFrame&      aRow,
                                         const nsRect&         aDamageArea)
{
  Reset(aTable, aRowGroup, aRow, aDamageArea);
}

void
BCMapBorderIterator::Reset(nsTableFrame&         aTable,
                           nsTableRowGroupFrame& aRowGroup,
                           nsTableRowFrame&      aRow,
                           const nsRect&         aDamageArea)
{
  atEnd = PR_TRUE; 

  table   = &aTable;
  rg      = &aRowGroup; 
  prevRow = nsnull;
  row     = &aRow;                     

  nsTableFrame* tableFif = (nsTableFrame*)table->GetFirstInFlow(); if (!tableFif) ABORT0();
  tableCellMap = tableFif->GetCellMap();

  startX   = aDamageArea.x;
  startY   = aDamageArea.y;
  endY     = aDamageArea.y + aDamageArea.height;
  endX     = aDamageArea.x + aDamageArea.width;

  numRows       = tableFif->GetRowCount();
  y             = 0;
  numCols       = tableFif->GetColCount();
  x             = 0;
  rowGroupIndex = -1;
  prevCell      = nsnull;
  cell          = nsnull;
  prevCellData  = nsnull;
  cellData      = nsnull;
  bcData        = nsnull;

  
  table->OrderRowGroups(rowGroups);
}

void 
BCMapBorderIterator::SetNewData(PRInt32 aY,
                                PRInt32 aX)
{
  if (!tableCellMap || !tableCellMap->mBCInfo) ABORT0();

  x            = aX;
  y            = aY;
  prevCellData = cellData;
  if (IsRightMost() && IsBottomMost()) {
    cell = nsnull;
    bcData = &tableCellMap->mBCInfo->mLowerRightCorner;
  }
  else if (IsRightMost()) {
    cellData = nsnull;
    bcData = &tableCellMap->mBCInfo->mRightBorders.ElementAt(aY);
  }
  else if (IsBottomMost()) {
    cellData = nsnull;
    bcData = &tableCellMap->mBCInfo->mBottomBorders.ElementAt(aX);
  }
  else {
    if (PRUint32(y - fifRowGroupStart) < cellMap->mRows.Length()) { 
      bcData = nsnull;
      cellData =
        (BCCellData*)cellMap->mRows[y - fifRowGroupStart].SafeElementAt(x);
      if (cellData) {
        bcData = &cellData->mData;
        if (!cellData->IsOrig()) {
          if (cellData->IsRowSpan()) {
            aY -= cellData->GetRowSpanOffset();
          }
          if (cellData->IsColSpan()) {
            aX -= cellData->GetColSpanOffset();
          }
          if ((aX >= 0) && (aY >= 0)) {
            cellData = (BCCellData*)cellMap->mRows[aY - fifRowGroupStart][aX];
          }
        }
        if (cellData->IsOrig()) {
          prevCell = cell;
          cell = cellData->GetCellFrame();
        }
      }
    }
  }
}

PRBool
BCMapBorderIterator::SetNewRow(nsTableRowFrame* aRow)
{
  prevRow = row;
  row      = (aRow) ? aRow : row->GetNextRow();
 
  if (row) {
    isNewRow = PR_TRUE;
    y = row->GetRowIndex();
    x = startX;
  }
  else {
    atEnd = PR_TRUE;
  }
  return !atEnd;
}


PRBool
BCMapBorderIterator::SetNewRowGroup()
{
  rowGroupIndex++;

  isRepeatedHeader = PR_FALSE;
  isRepeatedFooter = PR_FALSE;

  if (PRUint32(rowGroupIndex) < rowGroups.Length()) {
    prevRg = rg;
    rg = rowGroups[rowGroupIndex];
    fifRowGroupStart = ((nsTableRowGroupFrame*)rg->GetFirstInFlow())->GetStartRowIndex();
    rowGroupStart    = rg->GetStartRowIndex(); 
    rowGroupEnd      = rowGroupStart + rg->GetRowCount() - 1;

    if (SetNewRow(rg->GetFirstRow())) {
      cellMap =
        tableCellMap->GetMapFor((nsTableRowGroupFrame*)rg->GetFirstInFlow(),
                                nsnull);
      if (!cellMap) ABORT1(PR_FALSE);
    }
    if (rg && table->GetPrevInFlow() && !rg->GetPrevInFlow()) {
      
      const nsStyleDisplay* display = rg->GetStyleDisplay();
      if (y == startY) {
        isRepeatedHeader = (NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == display->mDisplay);
      }
      else {
        isRepeatedFooter = (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == display->mDisplay);
      }
    }
  }
  else {
    atEnd = PR_TRUE;
  }
  return !atEnd;
}

void 
BCMapBorderIterator::First()
{
  if (!table || (startX >= numCols) || (startY >= numRows)) ABORT0();

  atEnd = PR_FALSE;

  PRUint32 numRowGroups = rowGroups.Length();
  for (PRUint32 rgX = 0; rgX < numRowGroups; rgX++) { 
    nsTableRowGroupFrame* rowG = rowGroups[rgX];
    PRInt32 start = rowG->GetStartRowIndex();
    PRInt32 end   = start + rowG->GetRowCount() - 1;
    if ((startY >= start) && (startY <= end)) {
      rowGroupIndex = rgX - 1; 
      if (SetNewRowGroup()) { 
        while ((y < startY) && !atEnd) {
          SetNewRow();
        }
        if (!atEnd) {
          SetNewData(startY, startX);
        }
      }
      return;
    }
  }
  atEnd = PR_TRUE;
}

void 
BCMapBorderIterator::Next()
{
  if (atEnd) ABORT0();
  isNewRow = PR_FALSE;

  x++;
  if (x > endX) {
    y++;
    if (y == endY) {
      x = startX;
    }
    else if (y < endY) {
      if (y <= rowGroupEnd) {
        SetNewRow();
      }
      else {
        SetNewRowGroup();
      }
    }
    else {
      atEnd = PR_TRUE;
    }
  }
  if (!atEnd) {
    SetNewData(y, x);
  }
}


static nscoord
CalcVerCornerOffset(PRUint8 aCornerOwnerSide,
                    nscoord aCornerSubWidth,
                    nscoord aHorWidth,
                    PRBool  aIsStartOfSeg,
                    PRBool  aIsBevel)
{
  nscoord offset = 0;
  
  nscoord smallHalf, largeHalf;
  if ((NS_SIDE_TOP == aCornerOwnerSide) || (NS_SIDE_BOTTOM == aCornerOwnerSide)) {
    DivideBCBorderSize(aCornerSubWidth, smallHalf, largeHalf);
    if (aIsBevel) {
      offset = (aIsStartOfSeg) ? -largeHalf : smallHalf;
    }
    else {
      offset = (NS_SIDE_TOP == aCornerOwnerSide) ? smallHalf : -largeHalf;
    }
  }
  else {
    DivideBCBorderSize(aHorWidth, smallHalf, largeHalf);
    if (aIsBevel) {
      offset = (aIsStartOfSeg) ? -largeHalf : smallHalf;
    }
    else {
      offset = (aIsStartOfSeg) ? smallHalf : -largeHalf;
    }
  }
  return nsPresContext::CSSPixelsToAppUnits(offset);
}











static nscoord
CalcHorCornerOffset(PRUint8 aCornerOwnerSide,
                    nscoord aCornerSubWidth,
                    nscoord aVerWidth,
                    PRBool  aIsStartOfSeg,
                    PRBool  aIsBevel,
                    PRBool  aTableIsLTR)
{
  nscoord offset = 0;
  
  nscoord smallHalf, largeHalf;
  if ((NS_SIDE_LEFT == aCornerOwnerSide) || (NS_SIDE_RIGHT == aCornerOwnerSide)) {
    if (aTableIsLTR) {
      DivideBCBorderSize(aCornerSubWidth, smallHalf, largeHalf);
    }
    else {
      DivideBCBorderSize(aCornerSubWidth, largeHalf, smallHalf);
    }
    if (aIsBevel) {
      offset = (aIsStartOfSeg) ? -largeHalf : smallHalf;
    }
    else {
      offset = (NS_SIDE_LEFT == aCornerOwnerSide) ? smallHalf : -largeHalf;
    }
  }
  else {
    if (aTableIsLTR) {
      DivideBCBorderSize(aVerWidth, smallHalf, largeHalf);
    }
    else {
      DivideBCBorderSize(aVerWidth, largeHalf, smallHalf);
    }
    if (aIsBevel) {
      offset = (aIsStartOfSeg) ? -largeHalf : smallHalf;
    }
    else {
      offset = (aIsStartOfSeg) ? smallHalf : -largeHalf;
    }
  }
  return nsPresContext::CSSPixelsToAppUnits(offset);
}

struct BCVerticalSeg
{
  BCVerticalSeg();
 
  void Start(BCMapBorderIterator& aIter,
             BCBorderOwner        aBorderOwner,
             nscoord              aVerSegWidth,
             nscoord              aPrevHorSegHeight,
             nscoord              aHorSegHeight,
             BCVerticalSeg*       aVerInfoArray);
  
  union {
    nsTableColFrame*  col;
    PRInt32           colWidth;
  };
  PRInt32               colX;
  nsTableCellFrame*     ajaCell;
  nsTableCellFrame*     firstCell;  
  nsTableRowGroupFrame* firstRowGroup; 
  nsTableRowFrame*      firstRow; 
  nsTableCellFrame*     lastCell;   
  PRInt32               segY;
  PRInt32               segHeight;
  PRInt16               segWidth;   
  PRUint8               owner;
  PRUint8               bevelSide;
  PRUint16              bevelOffset;
};

BCVerticalSeg::BCVerticalSeg() 
{ 
  col = nsnull; firstCell = lastCell = ajaCell = nsnull; colX = segY = segHeight = 0;
  segWidth = bevelOffset = 0; bevelSide = 0; owner = eCellOwner; 
}
 
void
BCVerticalSeg::Start(BCMapBorderIterator& aIter,
                     BCBorderOwner        aBorderOwner,
                     nscoord              aVerSegWidth,
                     nscoord              aPrevHorSegHeight,
                     nscoord              aHorSegHeight,
                     BCVerticalSeg*       aVerInfoArray)
{
  PRUint8      ownerSide = 0;
  PRPackedBool bevel     = PR_FALSE;
  PRInt32      xAdj      = aIter.x - aIter.startX;

  nscoord cornerSubWidth  = (aIter.bcData) ? aIter.bcData->GetCorner(ownerSide, bevel) : 0;
  PRBool  topBevel        = (aVerSegWidth > 0) ? bevel : PR_FALSE;
  nscoord maxHorSegHeight = PR_MAX(aPrevHorSegHeight, aHorSegHeight);
  nscoord offset          = CalcVerCornerOffset(ownerSide, cornerSubWidth, maxHorSegHeight, 
                                                PR_TRUE, topBevel);

  bevelOffset   = (topBevel) ? maxHorSegHeight : 0;
  bevelSide     = (aHorSegHeight > 0) ? NS_SIDE_RIGHT : NS_SIDE_LEFT;
  segY         += offset;
  segHeight     = -offset;
  segWidth      = aVerSegWidth;
  owner         = aBorderOwner;
  firstCell     = aIter.cell;
  firstRowGroup = aIter.rg;
  firstRow      = aIter.row;
  if (xAdj > 0) {
    ajaCell = aVerInfoArray[xAdj - 1].lastCell;
  }
}

struct BCHorizontalSeg
{
  BCHorizontalSeg();

  void Start(BCMapBorderIterator& aIter,
             BCBorderOwner        aBorderOwner,
             PRUint8              aCornerOwnerSide,
             nscoord              aSubWidth,
             PRBool               aBevel,
             nscoord              aTopVerSegWidth,
             nscoord              aBottomVerSegWidth,
             nscoord              aHorSegHeight,
             nsTableCellFrame*    aLastCell,
             PRBool               aTableIsLTR);
  
  nscoord            x;
  nscoord            y;
  nscoord            width;
  nscoord            height;
  PRBool             leftBevel;
  nscoord            leftBevelOffset;
  PRUint8            leftBevelSide;
  PRUint8            owner;
  nsTableCellFrame*  firstCell; 
  nsTableCellFrame*  ajaCell;
};

BCHorizontalSeg::BCHorizontalSeg() 
{ 
  x = y = width = height = leftBevel = leftBevelOffset = leftBevelSide = 0; 
  firstCell = ajaCell = nsnull;
}
  













void
BCHorizontalSeg::Start(BCMapBorderIterator& aIter,
                       BCBorderOwner        aBorderOwner,
                       PRUint8              aCornerOwnerSide,
                       nscoord              aSubWidth,
                       PRBool               aBevel,
                       nscoord              aTopVerSegWidth,
                       nscoord              aBottomVerSegWidth,
                       nscoord              aHorSegHeight,
                       nsTableCellFrame*    aLastCell,
                       PRBool               aTableIsLTR)
{
  owner = aBorderOwner;
  leftBevel = (aHorSegHeight > 0) ? aBevel : PR_FALSE;
  nscoord maxVerSegWidth = PR_MAX(aTopVerSegWidth, aBottomVerSegWidth);
  nscoord offset = CalcHorCornerOffset(aCornerOwnerSide, aSubWidth, maxVerSegWidth, 
                                       PR_TRUE, leftBevel, aTableIsLTR);
  leftBevelOffset = (leftBevel && (aHorSegHeight > 0)) ? maxVerSegWidth : 0;
  leftBevelSide   = (aBottomVerSegWidth > 0) ? NS_SIDE_BOTTOM : NS_SIDE_TOP;
  if (aTableIsLTR) {
    x            += offset;
  }
  else {
    x            -= offset;
  }
  width           = -offset;
  height          = aHorSegHeight;
  firstCell       = aIter.cell;
  ajaCell         = (aIter.IsTopMost()) ? nsnull : aLastCell; 
}

void 
nsTableFrame::PaintBCBorders(nsIRenderingContext& aRenderingContext,
                             const nsRect&        aDirtyRect)
{
  nsMargin childAreaOffset = GetChildAreaOffset(nsnull);
  nsTableFrame* firstInFlow = (nsTableFrame*)GetFirstInFlow(); if (!firstInFlow) ABORT0();

  PRInt32 startRowY = (GetPrevInFlow()) ? 0 : childAreaOffset.top; 

  nsStyleContext* bgContext = nsCSSRendering::FindNonTransparentBackground(mStyleContext);
  const nsStyleBackground* bgColor = bgContext->GetStyleBackground();
  
  PRUint32 startRowIndex, endRowIndex, startColIndex, endColIndex;
  startRowIndex = endRowIndex = startColIndex = endColIndex = 0;

  RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);
  PRBool done = PR_FALSE;
  PRBool haveIntersect = PR_FALSE;
  nsTableRowGroupFrame* inFlowRG  = nsnull;
  nsTableRowFrame*      inFlowRow = nsnull;
  
  PRInt32 rowY = startRowY;
  for (PRUint32 rgX = 0; rgX < rowGroups.Length() && !done; rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
    for (nsTableRowFrame* rowFrame = rgFrame->GetFirstRow(); rowFrame;
         rowFrame = rowFrame->GetNextRow()) {
      
      nscoord topBorderHalf    = (GetPrevInFlow()) ? 0 : nsPresContext::CSSPixelsToAppUnits(rowFrame->GetTopBCBorderWidth() + 1); 
      nscoord bottomBorderHalf = (GetNextInFlow()) ? 0 : nsPresContext::CSSPixelsToAppUnits(rowFrame->GetBottomBCBorderWidth() + 1);
      
      nsSize rowSize = rowFrame->GetSize();
      if (haveIntersect) {
        if (aDirtyRect.YMost() >= (rowY - topBorderHalf)) {
          nsTableRowFrame* fifRow = (nsTableRowFrame*)rowFrame->GetFirstInFlow(); if (!fifRow) ABORT0();
          endRowIndex = fifRow->GetRowIndex();
        }
        else done = PR_TRUE;
      }
      else {
        if ((rowY + rowSize.height + bottomBorderHalf) >= aDirtyRect.y) {
          inFlowRG  = rgFrame;
          inFlowRow = rowFrame;
          nsTableRowFrame* fifRow = (nsTableRowFrame*)rowFrame->GetFirstInFlow(); if (!fifRow) ABORT0();
          startRowIndex = endRowIndex = fifRow->GetRowIndex();
          haveIntersect = PR_TRUE;
        }
        else {
          startRowY += rowSize.height;
        }
      }
      rowY += rowSize.height; 
    }
  }
  
  
  
  
  
  if (!haveIntersect)
    return;  
  if (!inFlowRG || !inFlowRow) ABORT0();

  PRInt32 startColX;
  
  haveIntersect = PR_FALSE;
  PRUint32 numCols = GetColCount();
  if (0 == numCols) return;

  PRInt32 leftCol, rightCol, colInc; 
  PRBool tableIsLTR = GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_LTR;
  if (tableIsLTR) {
    startColX = childAreaOffset.left; 
    leftCol = 0;
    rightCol = numCols;
    colInc = 1;
  } else {
    startColX = mRect.width - childAreaOffset.right; 
    leftCol = numCols-1;
    rightCol = -1;
    colInc = -1;
  }

  nscoord x = 0;
  PRInt32 colX;
  for (colX = leftCol; colX != rightCol; colX += colInc) {
    nsTableColFrame* colFrame = firstInFlow->GetColFrame(colX);
    if (!colFrame) ABORT0();
    
    nscoord leftBorderHalf    = nsPresContext::CSSPixelsToAppUnits(colFrame->GetLeftBorderWidth() + 1); 
    nscoord rightBorderHalf   = nsPresContext::CSSPixelsToAppUnits(colFrame->GetRightBorderWidth() + 1);
    
    nsSize size = colFrame->GetSize();
    if (haveIntersect) {
      if (aDirtyRect.XMost() >= (x - leftBorderHalf)) {
        endColIndex = colX;
      }
      else break;
    }
    else {
      if ((x + size.width + rightBorderHalf) >= aDirtyRect.x) {
        startColIndex = endColIndex = colX;
        haveIntersect = PR_TRUE;
      }
      else {
        startColX += colInc * size.width;
      }
    }
    x += size.width;
  }

  if (!tableIsLTR) {
    PRUint32 temp;
    startColX = mRect.width - childAreaOffset.right;
    temp = startColIndex; startColIndex = endColIndex; endColIndex = temp;
    for (PRUint32 column = 0; column < startColIndex; column++) {
      nsTableColFrame* colFrame = firstInFlow->GetColFrame(column);
      if (!colFrame) ABORT0();
      nsSize size = colFrame->GetSize();
      startColX += colInc * size.width;
    }
  }
  if (!haveIntersect)
    return;
  
  nsRect damageArea(startColIndex, startRowIndex,
                    1 + PR_ABS(PRInt32(endColIndex - startColIndex)), 
                    1 + endRowIndex - startRowIndex);
  BCVerticalSeg* verInfo = new BCVerticalSeg[damageArea.width + 1]; if (!verInfo) ABORT0();

  BCBorderOwner borderOwner, ignoreBorderOwner;
  PRUint8 ownerSide;
  nscoord cornerSubWidth, smallHalf, largeHalf;
  nsRect rowRect(0,0,0,0);
  PRBool isSegStart, ignoreSegStart;
  nscoord prevHorSegHeight = 0;
  PRPackedBool bevel;
  PRInt32 repeatedHeaderY = -99;
  PRBool  afterRepeatedHeader = PR_FALSE;
  PRBool  startRepeatedFooter = PR_FALSE;

  
  
  
  BCMapBorderIterator iter(*this, *inFlowRG, *inFlowRow, damageArea); 
  for (iter.First(); !iter.atEnd; iter.Next()) {
    nscoord verSegWidth = (iter.bcData) ? iter.bcData->GetLeftEdge(borderOwner, isSegStart) : 0;
    nscoord horSegHeight = (iter.bcData) ? iter.bcData->GetTopEdge(ignoreBorderOwner, ignoreSegStart) : 0;

    PRInt32 xAdj = iter.x - iter.startX;
    if (iter.isNewRow) {
      prevHorSegHeight = 0;
      rowRect = iter.row->GetRect();
      if (iter.isRepeatedHeader) {
        repeatedHeaderY = iter.y;
      }
      afterRepeatedHeader = !iter.isRepeatedHeader && (iter.y == (repeatedHeaderY + 1));
      startRepeatedFooter = iter.isRepeatedFooter && (iter.y == iter.rowGroupStart) && (iter.y != iter.startY);
    }
    BCVerticalSeg& info = verInfo[xAdj];
    if (!info.col) { 
      info.col = iter.IsRightMostTable() ? verInfo[xAdj - 1].col : firstInFlow->GetColFrame(iter.x);
      if (!info.col) ABORT0();
      if (0 == xAdj) {
        info.colX = startColX;
      }
      
      if (!iter.IsRightMost()) {
        verInfo[xAdj + 1].colX = info.colX + colInc * info.col->GetSize().width;
      }
      info.segY = startRowY; 
      info.Start(iter, borderOwner, verSegWidth, prevHorSegHeight, horSegHeight, verInfo);
      info.lastCell = iter.cell;
    }

    if (!iter.IsTopMost() && (isSegStart || iter.IsBottomMost() || afterRepeatedHeader || startRepeatedFooter)) {
      
      if (info.segHeight > 0) {
        if (iter.bcData) {
          cornerSubWidth = iter.bcData->GetCorner(ownerSide, bevel);
        } else {
          cornerSubWidth = 0;
          ownerSide = 0; 
          bevel = PR_FALSE; 
        }
        PRBool endBevel = (info.segWidth > 0) ? bevel : PR_FALSE; 
        nscoord bottomHorSegHeight = PR_MAX(prevHorSegHeight, horSegHeight); 
        nscoord endOffset = CalcVerCornerOffset(ownerSide, cornerSubWidth, bottomHorSegHeight, 
                                                PR_FALSE, endBevel);
        info.segHeight += endOffset;
        if (info.segWidth > 0) {     
          
          PRUint8 side = (iter.IsRightMost()) ? NS_SIDE_RIGHT : NS_SIDE_LEFT;
          nsTableRowFrame* row           = info.firstRow;
          nsTableRowGroupFrame* rowGroup = info.firstRowGroup;
          nsTableColFrame* col           = info.col; if (!col) ABORT0();
          nsTableCellFrame* cell         = info.firstCell; 
          PRUint8 style = NS_STYLE_BORDER_STYLE_SOLID;
          nscolor color = 0xFFFFFFFF;
          PRBool ignoreIfRules = (iter.IsRightMostTable() || iter.IsLeftMostTable());

          switch (info.owner) {
          case eTableOwner:
            ::GetPaintStyleInfo(this, side, style, color, tableIsLTR, PR_FALSE);
            break;
          case eAjaColGroupOwner: 
            side = NS_SIDE_RIGHT;
            if (!iter.IsRightMostTable() && (xAdj > 0)) {
              col = verInfo[xAdj - 1].col; 
            } 
          case eColGroupOwner:
            if (col) {
              nsIFrame* cg = col->GetParent();
              if (cg) {
                ::GetPaintStyleInfo(cg, side, style, color, tableIsLTR, ignoreIfRules);
              }
            }
            break;
          case eAjaColOwner: 
            side = NS_SIDE_RIGHT;
            if (!iter.IsRightMostTable() && (xAdj > 0)) {
              col = verInfo[xAdj - 1].col; 
            } 
          case eColOwner:
            if (col) {
              ::GetPaintStyleInfo(col, side, style, color, tableIsLTR, ignoreIfRules);
            }
            break;
          case eAjaRowGroupOwner:
            NS_ASSERTION(PR_FALSE, "program error"); 
          case eRowGroupOwner:
            NS_ASSERTION(iter.IsLeftMostTable() || iter.IsRightMostTable(), "program error");
            if (rowGroup) {
              ::GetPaintStyleInfo(rowGroup, side, style, color, tableIsLTR, ignoreIfRules);
            }
            break;
          case eAjaRowOwner:
            NS_ASSERTION(PR_FALSE, "program error"); 
          case eRowOwner: 
            NS_ASSERTION(iter.IsLeftMostTable() || iter.IsRightMostTable(), "program error");
            if (row) {
              ::GetPaintStyleInfo(row, side, style, color, tableIsLTR, ignoreIfRules);
            }
            break;
          case eAjaCellOwner:
            side = NS_SIDE_RIGHT;
            cell = info.ajaCell; 
          case eCellOwner:
            if (cell) {
              ::GetPaintStyleInfo(cell, side, style, color, tableIsLTR, PR_FALSE);
            }
            break;
          }
          DivideBCBorderSize(info.segWidth, smallHalf, largeHalf);
          nsRect segRect(info.colX - nsPresContext::CSSPixelsToAppUnits(largeHalf), info.segY, 
                         nsPresContext::CSSPixelsToAppUnits(info.segWidth), info.segHeight);
          nscoord bottomBevelOffset = (endBevel) ? nsPresContext::CSSPixelsToAppUnits(bottomHorSegHeight) : 0;
          PRUint8 bottomBevelSide = ((horSegHeight > 0) ^ !tableIsLTR) ? NS_SIDE_RIGHT : NS_SIDE_LEFT;
          PRUint8 topBevelSide = ((info.bevelSide == NS_SIDE_RIGHT) ^ !tableIsLTR)?  NS_SIDE_RIGHT : NS_SIDE_LEFT;
          nsCSSRendering::DrawTableBorderSegment(aRenderingContext, style, color, bgColor, segRect, nsPresContext::AppUnitsPerCSSPixel(), 
                                                 topBevelSide, nsPresContext::CSSPixelsToAppUnits(info.bevelOffset), 
                                                 bottomBevelSide, bottomBevelOffset);
        } 
        info.segY = info.segY + info.segHeight - endOffset;
      } 
      info.Start(iter, borderOwner, verSegWidth, prevHorSegHeight, horSegHeight, verInfo);
    } 

    info.lastCell   = iter.cell;
    info.segHeight += rowRect.height;
    prevHorSegHeight = horSegHeight;
  } 

  
  
  memset(verInfo, 0, damageArea.width * sizeof(BCVerticalSeg)); 
  for (PRInt32 xIndex = 0; xIndex < damageArea.width; xIndex++) {
    verInfo[xIndex].colWidth = -1;
  }
  PRInt32 nextY = startRowY;
  BCHorizontalSeg horSeg;

  iter.Reset(*this, *inFlowRG, *inFlowRow, damageArea);
  for (iter.First(); !iter.atEnd; iter.Next()) {
    nscoord leftSegWidth = (iter.bcData) ? iter.bcData->GetLeftEdge(ignoreBorderOwner, ignoreSegStart) : 0;
    nscoord topSegHeight = (iter.bcData) ? iter.bcData->GetTopEdge(borderOwner, isSegStart) : 0;

    PRInt32 xAdj = iter.x - iter.startX;
    
    if (verInfo[xAdj].colWidth < 0) {
      if (iter.IsRightMostTable()) {
        verInfo[xAdj].colWidth = verInfo[xAdj - 1].colWidth;
      }
      else {
        nsTableColFrame* col = firstInFlow->GetColFrame(iter.x); if (!col) ABORT0();
        verInfo[xAdj].colWidth = col->GetSize().width;
      }
    }
    cornerSubWidth = (iter.bcData) ? iter.bcData->GetCorner(ownerSide, bevel) : 0;
    nscoord verWidth = PR_MAX(verInfo[xAdj].segWidth, leftSegWidth);
    if (iter.isNewRow || (iter.IsLeftMost() && iter.IsBottomMost())) {
      horSeg.y = nextY;
      nextY    = nextY + iter.row->GetSize().height;
      horSeg.x = startColX;
      horSeg.Start(iter, borderOwner, ownerSide, cornerSubWidth, bevel, verInfo[xAdj].segWidth, 
                   leftSegWidth, topSegHeight, verInfo[xAdj].lastCell, tableIsLTR);
    }
    PRBool verOwnsCorner = (NS_SIDE_TOP == ownerSide) || (NS_SIDE_BOTTOM == ownerSide);
    if (!iter.IsLeftMost() && (isSegStart || iter.IsRightMost() || verOwnsCorner)) {
      
      if (horSeg.width > 0) {
        PRBool endBevel = (horSeg.height > 0) ? bevel : 0;
        nscoord endOffset = CalcHorCornerOffset(ownerSide, cornerSubWidth, verWidth, PR_FALSE, endBevel, tableIsLTR);
        horSeg.width += endOffset;
        if (horSeg.height > 0) {
          
          PRUint8 side = (iter.IsBottomMost()) ? NS_SIDE_BOTTOM : NS_SIDE_TOP;
          nsIFrame* rg   = iter.rg;          if (!rg) ABORT0();
          nsIFrame* row  = iter.row;         if (!row) ABORT0();
          nsIFrame* cell = horSeg.firstCell; if (!cell) ABORT0();
          nsIFrame* col;

          PRUint8 style = NS_STYLE_BORDER_STYLE_SOLID; 
          nscolor color = 0xFFFFFFFF;
          PRBool ignoreIfRules = (iter.IsTopMostTable() || iter.IsBottomMostTable());

          switch (horSeg.owner) {
          case eTableOwner:
            ::GetPaintStyleInfo(this, side, style, color, tableIsLTR, PR_FALSE);
            break;
          case eAjaColGroupOwner: 
            NS_ASSERTION(PR_FALSE, "program error"); 
          case eColGroupOwner: {
            NS_ASSERTION(iter.IsTopMostTable() || iter.IsBottomMostTable(), "program error");
            col = firstInFlow->GetColFrame(iter.x - 1); if (!col) ABORT0();
            nsIFrame* cg = col->GetParent(); if (!cg) ABORT0();
            ::GetPaintStyleInfo(cg, side, style, color, tableIsLTR, ignoreIfRules);
            break;
          }
          case eAjaColOwner: 
            NS_ASSERTION(PR_FALSE, "program error"); 
          case eColOwner:
            NS_ASSERTION(iter.IsTopMostTable() || iter.IsBottomMostTable(), "program error");
            col = firstInFlow->GetColFrame(iter.x - 1); if (!col) ABORT0();
            ::GetPaintStyleInfo(col, side, style, color, tableIsLTR, ignoreIfRules);
            break;
          case eAjaRowGroupOwner: 
            side = NS_SIDE_BOTTOM;
            rg = (iter.IsBottomMostTable()) ? iter.rg : iter.prevRg; 
          case eRowGroupOwner:
            if (rg) {
              ::GetPaintStyleInfo(rg, side, style, color, tableIsLTR, ignoreIfRules);
            }
            break;
          case eAjaRowOwner: 
            side = NS_SIDE_BOTTOM;
            row = (iter.IsBottomMostTable()) ? iter.row : iter.prevRow; 
          case eRowOwner:
            if (row) {
              ::GetPaintStyleInfo(row, side, style, color, tableIsLTR, iter.IsBottomMostTable());
            }
            break;
          case eAjaCellOwner:
            side = NS_SIDE_BOTTOM;
            
            cell = horSeg.ajaCell; 
            
          case eCellOwner:
            if (cell) {
              ::GetPaintStyleInfo(cell, side, style, color, tableIsLTR, PR_FALSE);
            }
            break;
          }
          
          DivideBCBorderSize(horSeg.height, smallHalf, largeHalf);
          nsRect segRect(horSeg.x, horSeg.y - nsPresContext::CSSPixelsToAppUnits(largeHalf), horSeg.width, 
                         nsPresContext::CSSPixelsToAppUnits(horSeg.height));
           if (!tableIsLTR)
            segRect.x -= segRect.width;

          nscoord rightBevelOffset = (endBevel) ? nsPresContext::CSSPixelsToAppUnits(verWidth) : 0;
          PRUint8 rightBevelSide = (leftSegWidth > 0) ? NS_SIDE_BOTTOM : NS_SIDE_TOP;
          if (tableIsLTR) {
            nsCSSRendering::DrawTableBorderSegment(aRenderingContext, style, color, bgColor, segRect, nsPresContext::AppUnitsPerCSSPixel(), horSeg.leftBevelSide,
                                                 nsPresContext::CSSPixelsToAppUnits(horSeg.leftBevelOffset), 
                                                 rightBevelSide, rightBevelOffset);
          }
          else {
            nsCSSRendering::DrawTableBorderSegment(aRenderingContext, style, color, bgColor, segRect, nsPresContext::AppUnitsPerCSSPixel(), rightBevelSide, rightBevelOffset,
                                                 horSeg.leftBevelSide, nsPresContext::CSSPixelsToAppUnits(horSeg.leftBevelOffset));
          }

        } 
        horSeg.x += colInc * (horSeg.width - endOffset);
      } 
      horSeg.Start(iter, borderOwner, ownerSide, cornerSubWidth, bevel, verInfo[xAdj].segWidth, 
                   leftSegWidth, topSegHeight, verInfo[xAdj].lastCell, tableIsLTR);
    } 
    horSeg.width += verInfo[xAdj].colWidth;
    verInfo[xAdj].segWidth = leftSegWidth;
    verInfo[xAdj].lastCell = iter.cell;
  }
  delete [] verInfo;
}

PRBool nsTableFrame::RowHasSpanningCells(PRInt32 aRowIndex, PRInt32 aNumEffCols)
{
  PRBool result = PR_FALSE;
  nsTableCellMap* cellMap = GetCellMap();
  NS_PRECONDITION (cellMap, "bad call, cellMap not yet allocated.");
  if (cellMap) {
    result = cellMap->RowHasSpanningCells(aRowIndex, aNumEffCols);
  }
  return result;
}

PRBool nsTableFrame::RowIsSpannedInto(PRInt32 aRowIndex, PRInt32 aNumEffCols)
{
  PRBool result = PR_FALSE;
  nsTableCellMap* cellMap = GetCellMap();
  NS_PRECONDITION (cellMap, "bad call, cellMap not yet allocated.");
  if (cellMap) {
    result = cellMap->RowIsSpannedInto(aRowIndex, aNumEffCols);
  }
  return result;
}

PRBool nsTableFrame::ColHasSpanningCells(PRInt32 aColIndex)
{
  PRBool result = PR_FALSE;
  nsTableCellMap * cellMap = GetCellMap();
  NS_PRECONDITION (cellMap, "bad call, cellMap not yet allocated.");
  if (cellMap) {
    result = cellMap->ColHasSpanningCells(aColIndex);
  }
  return result;
}

PRBool nsTableFrame::ColIsSpannedInto(PRInt32 aColIndex)
{
  PRBool result = PR_FALSE;
  nsTableCellMap * cellMap = GetCellMap();
  NS_PRECONDITION (cellMap, "bad call, cellMap not yet allocated.");
  if (cellMap) {
    result = cellMap->ColIsSpannedInto(aColIndex);
  }
  return result;
}


static void
DestroyCoordFunc(void*           aFrame,
                 nsIAtom*        aPropertyName,
                 void*           aPropertyValue,
                 void*           aDtorData)
{
  delete static_cast<nscoord*>(aPropertyValue);
}


static void
DestroyPointFunc(void*           aFrame,
                 nsIAtom*        aPropertyName,
                 void*           aPropertyValue,
                 void*           aDtorData)
{
  delete static_cast<nsPoint*>(aPropertyValue);
}


static void
DestroyBCPropertyDataFunc(void*           aFrame,
                          nsIAtom*        aPropertyName,
                          void*           aPropertyValue,
                          void*           aDtorData)
{
  delete static_cast<BCPropertyData*>(aPropertyValue);
}

void*
nsTableFrame::GetProperty(nsIFrame*            aFrame,
                          nsIAtom*             aPropertyName,
                          PRBool               aCreateIfNecessary)
{
  nsPropertyTable *propTable = aFrame->PresContext()->PropertyTable();
  void *value = propTable->GetProperty(aFrame, aPropertyName);
  if (value) {
    return (nsPoint*)value;  
  }
  if (aCreateIfNecessary) {
    
    
    NSPropertyDtorFunc dtorFunc = nsnull;
    if (aPropertyName == nsGkAtoms::collapseOffsetProperty) {
      value = new nsPoint(0, 0);
      dtorFunc = DestroyPointFunc;
    }
    else if (aPropertyName == nsGkAtoms::rowUnpaginatedHeightProperty) {
      value = new nscoord;
      dtorFunc = DestroyCoordFunc;
    }
    else if (aPropertyName == nsGkAtoms::tableBCProperty) {
      value = new BCPropertyData;
      dtorFunc = DestroyBCPropertyDataFunc;
    }
    if (value) {
      propTable->SetProperty(aFrame, aPropertyName, value, dtorFunc, nsnull);
    }
    return value;
  }
  return nsnull;
}


void
nsTableFrame::InvalidateFrame(nsIFrame* aFrame,
                              const nsRect& aOrigRect,
                              const nsRect& aOrigOverflowRect,
                              PRBool aIsFirstReflow)
{
  nsIFrame* parent = aFrame->GetParent();
  NS_ASSERTION(parent, "What happened here?");

  if (parent->GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    
    
    return;
  }

  
  
  
  nsRect overflowRect = aFrame->GetOverflowRect();
  if (aIsFirstReflow ||
      aOrigRect.TopLeft() != aFrame->GetPosition() ||
      aOrigOverflowRect.TopLeft() != overflowRect.TopLeft()) {
    
    
    
    
    
    
    aFrame->Invalidate(overflowRect);
    parent->Invalidate(aOrigOverflowRect + aOrigRect.TopLeft());
  } else {
    nsRect rect = aFrame->GetRect();
    aFrame->CheckInvalidateSizeChange(aOrigRect, aOrigOverflowRect,
                                      rect.Size());
    aFrame->InvalidateRectDifference(aOrigOverflowRect, overflowRect);
    parent->InvalidateRectDifference(aOrigRect, rect);
  }    
}
