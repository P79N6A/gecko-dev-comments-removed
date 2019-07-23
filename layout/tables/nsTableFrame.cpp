






































#include "nsCOMPtr.h"
#include "nsVoidArray.h"
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

  nsTableReflowState(nsPresContext&          aPresContext,
                     const nsHTMLReflowState& aReflowState,
                     nsTableFrame&            aTableFrame,
                     nscoord                  aAvailWidth,
                     nscoord                  aAvailHeight)
    : reflowState(aReflowState)
  {
    Init(aPresContext, aTableFrame, aAvailWidth, aAvailHeight);
  }

  void Init(nsPresContext& aPresContext,
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

  nsTableReflowState(nsPresContext&          aPresContext,
                     const nsHTMLReflowState& aReflowState,
                     nsTableFrame&            aTableFrame)
    : reflowState(aReflowState)
  {
    Init(aPresContext, aTableFrame, aReflowState.availableWidth, aReflowState.availableHeight);
  }

};





struct BCPropertyData
{
  BCPropertyData() { mDamageArea.x = mDamageArea.y = mDamageArea.width = mDamageArea.height =
                     mTopBorderWidth = mRightBorderWidth = mBottomBorderWidth = mLeftBorderWidth = 0; }
  nsRect  mDamageArea;
  BCPixelSize mTopBorderWidth;
  BCPixelSize mRightBorderWidth;
  BCPixelSize mBottomBorderWidth;
  BCPixelSize mLeftBorderWidth;
};

NS_IMETHODIMP 
nsTableFrame::GetParentStyleContextFrame(nsPresContext* aPresContext,
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

NS_IMPL_ADDREF_INHERITED(nsTableFrame, nsHTMLContainerFrame)
NS_IMPL_RELEASE_INHERITED(nsTableFrame, nsHTMLContainerFrame)

NS_IMETHODIMP
nsTableFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null out param");

  if (aIID.Equals(NS_GET_IID(nsITableLayout))) {
    *aInstancePtr = static_cast<nsITableLayout*>(this);
    return NS_OK;
  }

  return nsHTMLContainerFrame::QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP
nsTableFrame::Init(nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIFrame*        aPrevInFlow)
{
  nsresult  rv;

  
  rv = nsHTMLContainerFrame::Init(aContent, aParent, aPrevInFlow);

  
  mState |= NS_FRAME_EXCLUDE_IGNORABLE_WHITESPACE;

  
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
                                  nsIFrame*       aChildList)
{

  if (!mFrames.IsEmpty() || !mColGroups.IsEmpty()) {
    
    
    NS_NOTREACHED("unexpected second call to SetInitialChildList");
    return NS_ERROR_UNEXPECTED;
  }
  if (aListName) {
    
    NS_NOTREACHED("unknown frame list");
    return NS_ERROR_INVALID_ARG;
  } 
  
  nsIFrame *childFrame = aChildList;
  nsIFrame *prevMainChild = nsnull;
  nsIFrame *prevColGroupChild = nsnull;
  for ( ; nsnull!=childFrame; )
  {
    const nsStyleDisplay* childDisplay = childFrame->GetStyleDisplay();
    
    if (PR_TRUE==IsRowGroup(childDisplay->mDisplay))
    {
      if (mFrames.IsEmpty()) 
        mFrames.SetFrames(childFrame);
      else
        prevMainChild->SetNextSibling(childFrame);
      prevMainChild = childFrame;
    }
    else if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == childDisplay->mDisplay)
    {
      NS_ASSERTION(nsGkAtoms::tableColGroupFrame == childFrame->GetType(),
                   "This is not a colgroup");
      if (mColGroups.IsEmpty())
        mColGroups.SetFrames(childFrame);
      else
        prevColGroupChild->SetNextSibling(childFrame);
      prevColGroupChild = childFrame;
    }
    else
    { 
      if (mFrames.IsEmpty())
        mFrames.SetFrames(childFrame);
      else
        prevMainChild->SetNextSibling(childFrame);
      prevMainChild = childFrame;
    }
    nsIFrame *prevChild = childFrame;
    childFrame = childFrame->GetNextSibling();
    prevChild->SetNextSibling(nsnull);
  }
  if (nsnull!=prevMainChild)
    prevMainChild->SetNextSibling(nsnull);
  if (nsnull!=prevColGroupChild)
    prevColGroupChild->SetNextSibling(nsnull);

  
  
  if (!GetPrevInFlow()) {
    
    
    InsertColGroups(0, mColGroups.FirstChild());
    AppendRowGroups(mFrames.FirstChild());
    
    if (!aChildList && IsBorderCollapse()) {
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
  if (IS_TABLE_CELL(aFrame->GetType())) {
    if ((nsGkAtoms::rowspan == aAttribute) || 
        (nsGkAtoms::colspan == aAttribute)) {
      nsTableCellMap* cellMap = GetCellMap();
      if (cellMap) {
        
        nsTableCellFrame* cellFrame = (nsTableCellFrame*)aFrame;
        PRInt32 rowIndex, colIndex;
        cellFrame->GetRowIndex(rowIndex);
        cellFrame->GetColIndex(colIndex);
        RemoveCell(cellFrame, rowIndex);
        nsAutoVoidArray cells;
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
  
  for (PRInt32 colX = colCount - 1; colX >= 0; colX--) {
    if (GetNumCellsOriginatingInCol(colX) <= 0) { 
      colCount--;
    }
    else break;
  }
  return colCount;
}

PRInt32 nsTableFrame::GetIndexOfLastRealCol()
{
  PRInt32 numCols = mColFrames.Count();
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
  PRInt32 numCols = mColFrames.Count();
  if ((aColIndex >= 0) && (aColIndex < numCols)) {
    return (nsTableColFrame *)mColFrames.ElementAt(aColIndex);
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


void nsTableFrame::ResetRowIndices(nsIFrame* aFirstRowGroupFrame,
                                   nsIFrame* aLastRowGroupFrame)
{
  
  
  RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);

  PRInt32 rowIndex = 0;
  nsTableRowGroupFrame* newRgFrame = nsnull;
  nsIFrame* omitRgFrame = aFirstRowGroupFrame;
  if (omitRgFrame) {
    newRgFrame = GetRowGroupFrame(omitRgFrame);
    if (omitRgFrame == aLastRowGroupFrame)
      omitRgFrame = nsnull;
  }

  for (PRUint32 rgX = 0; rgX < rowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
    if (rgFrame == newRgFrame) {
      
      if (omitRgFrame) {
        omitRgFrame = omitRgFrame->GetNextSibling();
        if (omitRgFrame) {
          newRgFrame  = GetRowGroupFrame(omitRgFrame);
          if (omitRgFrame == aLastRowGroupFrame)
            omitRgFrame = nsnull;
        }
      }
    }
    else {
      nsIFrame* rowFrame = rgFrame->GetFirstChild(nsnull);
      for ( ; rowFrame; rowFrame = rowFrame->GetNextSibling()) {
        if (NS_STYLE_DISPLAY_TABLE_ROW==rowFrame->GetStyleDisplay()->mDisplay) {
          ((nsTableRowFrame *)rowFrame)->SetRowIndex(rowIndex);
          rowIndex++;
        }
      }
    }
  }
}
void nsTableFrame::InsertColGroups(PRInt32         aStartColIndex,
                                   nsIFrame*       aFirstFrame,
                                   nsIFrame*       aLastFrame)
{
  PRInt32 colIndex = aStartColIndex;
  nsTableColGroupFrame* firstColGroupToReset = nsnull;
  nsIFrame* kidFrame = aFirstFrame;
  PRBool didLastFrame = PR_FALSE;
  while (kidFrame) {
    if (nsGkAtoms::tableColGroupFrame == kidFrame->GetType()) {
      if (didLastFrame) {
        firstColGroupToReset = (nsTableColGroupFrame*)kidFrame;
        break;
      }
      else {
        nsTableColGroupFrame* cgFrame = (nsTableColGroupFrame*)kidFrame;
        cgFrame->SetStartColumnIndex(colIndex);
        nsIFrame* firstCol = kidFrame->GetFirstChild(nsnull);
        cgFrame->AddColsToTable(colIndex, PR_FALSE, firstCol);
        PRInt32 numCols = cgFrame->GetColCount();
        colIndex += numCols;
      }
    }
    if (kidFrame == aLastFrame) {
      didLastFrame = PR_TRUE;
    }
    kidFrame = kidFrame->GetNextSibling();
  }

  if (firstColGroupToReset) {
    nsTableColGroupFrame::ResetColIndices(firstColGroupToReset, colIndex);
  }
}

void nsTableFrame::InsertCol(nsTableColFrame& aColFrame,
                             PRInt32          aColIndex)
{
  mColFrames.InsertElementAt(&aColFrame, aColIndex);
  nsTableColType insertedColType = aColFrame.GetColType();
  PRInt32 numCacheCols = mColFrames.Count();
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    PRInt32 numMapCols = cellMap->GetColCount();
    if (numCacheCols > numMapCols) {
      PRBool removedFromCache = PR_FALSE;
      if (eColAnonymousCell != insertedColType) {
        nsTableColFrame* lastCol = (nsTableColFrame *)mColFrames.ElementAt(numCacheCols - 1);
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
      CreateAnonymousColFrames(1, eColAnonymousCell, PR_TRUE);
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
nsTableFrame::CreateAnonymousColFrames(PRInt32         aNumColsToAdd,
                                       nsTableColType  aColType,
                                       PRBool          aDoAppend,
                                       nsIFrame*       aPrevColIn)
{
  
  nsTableColGroupFrame* colGroupFrame = nsnull;
  nsIFrame* childFrame = mColGroups.FirstChild();
  while (childFrame) {
    if (nsGkAtoms::tableColGroupFrame == childFrame->GetType()) {
      colGroupFrame = (nsTableColGroupFrame *)childFrame;
    }
    childFrame = childFrame->GetNextSibling();
  }

  nsTableColGroupType lastColGroupType = eColGroupContent; 
  nsTableColGroupType newColGroupType  = eColGroupContent; 
  if (colGroupFrame) {
    lastColGroupType = colGroupFrame->GetColType();
  }
  if (eColAnonymousCell == aColType) {
    if (eColGroupAnonymousCell != lastColGroupType) {
      newColGroupType = eColGroupAnonymousCell;
    }
  }
  else if (eColAnonymousCol == aColType) {
    if (eColGroupAnonymousCol != lastColGroupType) {
      newColGroupType = eColGroupAnonymousCol;
    }
  }
  else {
    NS_ASSERTION(PR_FALSE, "CreateAnonymousColFrames called incorrectly");
    return;
  }

  if (eColGroupContent != newColGroupType) {
    PRInt32 colIndex = (colGroupFrame) ? colGroupFrame->GetStartColumnIndex() + colGroupFrame->GetColCount()
                                       : 0;
    colGroupFrame = CreateAnonymousColGroupFrame(newColGroupType);
    if (!colGroupFrame) {
      return;
    }
    mColGroups.AppendFrame(this, colGroupFrame); 
    colGroupFrame->SetStartColumnIndex(colIndex);
  }

  nsIFrame* prevCol = (aDoAppend) ? colGroupFrame->GetChildList().LastChild() : aPrevColIn;

  nsIFrame* firstNewFrame;
  CreateAnonymousColFrames(colGroupFrame, aNumColsToAdd, aColType,
                           PR_TRUE, prevCol, &firstNewFrame);
}



void
nsTableFrame::CreateAnonymousColFrames(nsTableColGroupFrame* aColGroupFrame,
                                       PRInt32               aNumColsToAdd,
                                       nsTableColType        aColType,
                                       PRBool                aAddToColGroupAndTable,         
                                       nsIFrame*             aPrevFrameIn,
                                       nsIFrame**            aFirstNewFrame)
{
  NS_PRECONDITION(aColGroupFrame, "null frame");
  *aFirstNewFrame = nsnull;
  nsIFrame* lastColFrame = nsnull;
  nsPresContext* presContext = PresContext();
  nsIPresShell *shell = presContext->PresShell();

  
  nsIFrame* childFrame = aColGroupFrame->GetFirstChild(nsnull);
  while (childFrame) {
    if (nsGkAtoms::tableColFrame == childFrame->GetType()) {
      lastColFrame = (nsTableColGroupFrame *)childFrame;
    }
    childFrame = childFrame->GetNextSibling();
  }

  PRInt32 startIndex = mColFrames.Count();
  PRInt32 lastIndex  = startIndex + aNumColsToAdd - 1; 

  for (PRInt32 childX = startIndex; childX <= lastIndex; childX++) {
    nsIContent* iContent;
    nsRefPtr<nsStyleContext> styleContext;
    nsStyleContext* parentStyleContext;

    if ((aColType == eColAnonymousCol) && aPrevFrameIn) {
      
      styleContext = aPrevFrameIn->GetStyleContext();
      
      iContent = aPrevFrameIn->GetContent();
    }
    else {
      
      iContent = aColGroupFrame->GetContent();
      parentStyleContext = aColGroupFrame->GetStyleContext();
      styleContext = shell->StyleSet()->ResolvePseudoStyleFor(iContent,
                                                              nsCSSAnonBoxes::tableCol,
                                                              parentStyleContext);
    }
    
    NS_ASSERTION(iContent, "null content in CreateAnonymousColFrames");

    
    nsIFrame* colFrame = NS_NewTableColFrame(shell, styleContext);
    ((nsTableColFrame *) colFrame)->SetColType(aColType);
    colFrame->Init(iContent, aColGroupFrame, nsnull);
    colFrame->SetInitialChildList(nsnull, nsnull);

    
    if (lastColFrame) {
      lastColFrame->SetNextSibling(colFrame);
    }
    lastColFrame = colFrame;
    if (childX == startIndex) {
      *aFirstNewFrame = colFrame;
    }
  }
  if (aAddToColGroupAndTable) {
    nsFrameList& cols = aColGroupFrame->GetChildList();
    
    if (!aPrevFrameIn) {
      cols.AppendFrames(aColGroupFrame, *aFirstNewFrame);
    }
    
    PRInt32 startColIndex = aColGroupFrame->GetStartColumnIndex();
    if (aPrevFrameIn) {
      nsTableColFrame* colFrame = 
        (nsTableColFrame*)nsTableFrame::GetFrameAtOrBefore((nsIFrame*) aColGroupFrame, aPrevFrameIn, 
                                                           nsGkAtoms::tableColFrame);
      if (colFrame) {
        startColIndex = colFrame->GetColIndex() + 1;
      }
    }
    aColGroupFrame->AddColsToTable(startColIndex, PR_TRUE, 
                                  *aFirstNewFrame, lastColFrame);
  }
}

void
nsTableFrame::MatchCellMapToColCache(nsTableCellMap* aCellMap)
{
  PRInt32 numColsInMap   = GetColCount();
  PRInt32 numColsInCache = mColFrames.Count();
  PRInt32 numColsToAdd = numColsInMap - numColsInCache;
  if (numColsToAdd > 0) {
    
    CreateAnonymousColFrames(numColsToAdd, eColAnonymousCell, PR_TRUE); 
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

void nsTableFrame::InsertCells(nsVoidArray&    aCellFrames, 
                               PRInt32         aRowIndex, 
                               PRInt32         aColIndexBefore)
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
  
  PRInt32 endIndex   = mColFrames.Count() - 1;
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


void nsTableFrame::AppendRows(nsTableRowGroupFrame& aRowGroupFrame,
                              PRInt32               aRowIndex,
                              nsVoidArray&          aRowFrames)
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
  nsAutoVoidArray rows;
  rows.AppendElement(&aRowFrame);
  return InsertRows(aRowGroupFrame, rows, aRowIndex, aConsiderSpans);
}


PRInt32
nsTableFrame::InsertRows(nsTableRowGroupFrame& aRowGroupFrame,
                         nsVoidArray&          aRowFrames,
                         PRInt32               aRowIndex,
                         PRBool                aConsiderSpans)
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
    PRInt32 numNewRows = aRowFrames.Count();
    cellMap->InsertRows(aRowGroupFrame, aRowFrames, aRowIndex, aConsiderSpans, damageArea);
    MatchCellMapToColCache(cellMap);
    if (aRowIndex < origNumRows) {
      AdjustRowIndices(aRowIndex, numNewRows);
    }
    
    
    for (PRInt32 rowX = 0; rowX < numNewRows; rowX++) {
      nsTableRowFrame* rowFrame = (nsTableRowFrame *) aRowFrames.ElementAt(rowX);
      rowFrame->SetRowIndex(aRowIndex + rowX);
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
    if (IS_TABLE_CELL(kidFrame->GetType())) {
      nsTableCellFrame* cellFrame = (nsTableCellFrame*)kidFrame;
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

void nsTableFrame::AppendRowGroups(nsIFrame* aFirstRowGroupFrame)
{
  if (aFirstRowGroupFrame) {
    nsTableCellMap* cellMap = GetCellMap();
    if (cellMap) {
      nsFrameList newList(aFirstRowGroupFrame);
      InsertRowGroups(aFirstRowGroupFrame, newList.LastChild());
    }
  }
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
    nsIScrollableFrame* scrollable = nsnull;
    nsresult rv = CallQueryInterface(aFrame, &scrollable);
    if (NS_SUCCEEDED(rv) && (scrollable)) {
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
nsTableFrame::CollectRows(nsIFrame*       aFrame,
                          nsVoidArray&    aCollection)
{
  if (!aFrame) return 0;
  PRInt32 numRows = 0;
  nsTableRowGroupFrame* rgFrame = GetRowGroupFrame(aFrame);
  if (rgFrame) {
    nsIFrame* childFrame = rgFrame->GetFirstChild(nsnull);
    while (childFrame) {
      if (nsGkAtoms::tableRowFrame == childFrame->GetType()) {
        aCollection.AppendElement(childFrame);
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
nsTableFrame::InsertRowGroups(nsIFrame* aFirstRowGroupFrame,
                              nsIFrame* aLastRowGroupFrame)
{
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== insertRowGroupsBefore\n");
  Dump(PR_TRUE, PR_FALSE, PR_TRUE);
#endif
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    RowGroupArray orderedRowGroups;
    OrderRowGroups(orderedRowGroups);

    nsAutoVoidArray rows;
    
    
    PRUint32 rgIndex;
    for (rgIndex = 0; rgIndex < orderedRowGroups.Length(); rgIndex++) {
      nsIFrame* kidFrame = aFirstRowGroupFrame;
      while (kidFrame) {
        nsTableRowGroupFrame* rgFrame = GetRowGroupFrame(kidFrame);

        if (orderedRowGroups[rgIndex] == rgFrame) {
          nsTableRowGroupFrame* priorRG =
            (0 == rgIndex) ? nsnull : orderedRowGroups[rgIndex - 1]; 
          
          cellMap->InsertGroupCellMap(*rgFrame, priorRG);
        
          break;
        }
        else {
          if (kidFrame == aLastRowGroupFrame) {
            break;
          }
          kidFrame = kidFrame->GetNextSibling();
        }
      }
    }
    cellMap->Synchronize(this);
    ResetRowIndices(aFirstRowGroupFrame, aLastRowGroupFrame);

    
    for (rgIndex = 0; rgIndex < orderedRowGroups.Length(); rgIndex++) {
      nsIFrame* kidFrame = aFirstRowGroupFrame;
      while (kidFrame) {
        nsTableRowGroupFrame* rgFrame = GetRowGroupFrame(kidFrame);

        if (orderedRowGroups[rgIndex] == rgFrame) {
          nsTableRowGroupFrame* priorRG =
            (0 == rgIndex) ? nsnull : orderedRowGroups[rgIndex - 1]; 
          
          PRInt32 numRows = CollectRows(kidFrame, rows);
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
        else {
          if (kidFrame == aLastRowGroupFrame) {
            break;
          }
          kidFrame = kidFrame->GetNextSibling();
        }
      }
    }    
    
  }
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== insertRowGroupsAfter\n");
  Dump(PR_TRUE, PR_TRUE, PR_TRUE);
#endif
}





nsIFrame*
nsTableFrame::GetFirstChild(nsIAtom* aListName) const
{
  if (aListName == nsGkAtoms::colGroupList) {
    return mColGroups.FirstChild();
  }

  return nsHTMLContainerFrame::GetFirstChild(aListName);
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

class nsDisplayTableBorderBackground : public nsDisplayItem {
public:
  nsDisplayTableBorderBackground(nsTableFrame* aFrame) : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayTableBorderBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTableBorderBackground() {
    MOZ_COUNT_DTOR(nsDisplayTableBorderBackground);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  
  
  
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder) {
    return static_cast<nsTableFrame*>(mFrame)->GetOverflowRect() +
      aBuilder->ToReferenceFrame(mFrame);
  }
  NS_DISPLAY_DECL_NAME("TableBorderBackground")
};

void
nsDisplayTableBorderBackground::Paint(nsDisplayListBuilder* aBuilder,
    nsIRenderingContext* aCtx, const nsRect& aDirtyRect)
{
  static_cast<nsTableFrame*>(mFrame)->
    PaintTableBorderBackground(*aCtx, aDirtyRect,
                               aBuilder->ToReferenceFrame(mFrame));
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
                                      PRBool aIsRoot,
                                      DisplayGenericTablePartTraversal aTraversal)
{
  nsDisplayList eventsBorderBackground;
  
  
  PRBool sortEventBackgrounds = aIsRoot && aBuilder->IsForEventDelivery();
  nsDisplayListCollection separatedCollection;
  const nsDisplayListSet* lists = sortEventBackgrounds ? &separatedCollection : &aLists;
  
  
  
  
  if (aBuilder->IsForEventDelivery() &&
      aFrame->IsVisibleForPainting(aBuilder)) {
    nsresult rv = lists->BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayBackground(aFrame));
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

  
  
  
  nsresult rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
      nsDisplayTableBorderBackground(this));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return DisplayGenericTablePart(aBuilder, this, aDirtyRect, aLists, PR_TRUE);
}



void
nsTableFrame::PaintTableBorderBackground(nsIRenderingContext& aRenderingContext,
                                         const nsRect& aDirtyRect,
                                         nsPoint aPt)
{
  nsPresContext* presContext = PresContext();
  nsRect dirtyRect = aDirtyRect - aPt;
  nsIRenderingContext::AutoPushTranslation
    translate(&aRenderingContext, aPt.x, aPt.y);

  TableBackgroundPainter painter(this, TableBackgroundPainter::eOrigin_Table,
                                 presContext, aRenderingContext, dirtyRect);
  nsresult rv;
  
  if (eCompatibility_NavQuirks == presContext->CompatibilityMode()) {
    nsMargin deflate(0,0,0,0);
    if (IsBorderCollapse()) {
      PRInt32 p2t = nsPresContext::AppUnitsPerCSSPixel();
      BCPropertyData* propData =
        (BCPropertyData*)nsTableFrame::GetProperty((nsIFrame*)this,
                                                   nsGkAtoms::tableBCProperty,
                                                   PR_FALSE);
      if (propData) {
        deflate.top    = BC_BORDER_TOP_HALF_COORD(p2t, propData->mTopBorderWidth);
        deflate.right  = BC_BORDER_RIGHT_HALF_COORD(p2t, propData->mRightBorderWidth);
        deflate.bottom = BC_BORDER_BOTTOM_HALF_COORD(p2t, propData->mBottomBorderWidth);
        deflate.left   = BC_BORDER_LEFT_HALF_COORD(p2t, propData->mLeftBorderWidth);
      }
    }
    rv = painter.PaintTable(this, &deflate);
    if (NS_FAILED(rv)) return;
  }
  else {
    rv = painter.PaintTable(this, nsnull);
    if (NS_FAILED(rv)) return;
  }

  if (GetStyleVisibility()->IsVisible()) {
    const nsStyleBorder* border = GetStyleBorder();
    nsRect  rect(0, 0, mRect.width, mRect.height);
    if (!IsBorderCollapse()) {
      PRIntn skipSides = GetSkipSides();
      nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                  dirtyRect, rect, *border, mStyleContext,
                                  skipSides);
    }
    else {
      PaintBCBorders(aRenderingContext, dirtyRect);
    }
  }
}


NS_IMETHODIMP
nsTableFrame::SetSelected(nsPresContext* aPresContext,
                          nsIDOMRange *aRange,
                          PRBool aSelected,
                          nsSpread aSpread)
{
#if 0
  
  if ((aSpread == eSpreadDown)){
    nsIFrame* kid = GetFirstChild(nsnull);
    while (kid) {
      kid->SetSelected(nsnull, aSelected, eSpreadDown);
      kid = kid->GetNextSibling();
    }
  }
#endif
  
  
  
  nsFrame::SetSelected(aPresContext, aRange, aSelected, aSpread);
  return NS_OK;
  
}

PRBool nsTableFrame::ParentDisablesSelection() const 
{
  PRBool returnval;
  if (NS_FAILED(GetSelected(&returnval)))
    return PR_FALSE;
  if (returnval)
    return PR_TRUE;
  return nsFrame::ParentDisablesSelection();
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

  nsIFrame* colGroupFrame = mColGroups.FirstChild();
  PRInt32 colX = 0;
  nsPoint colGroupOrigin(aBorderPadding.left + cellSpacingX,
                         aBorderPadding.top + cellSpacingY);
  while (nsnull != colGroupFrame) {
    nscoord colGroupWidth = 0;
    nsIFrame* colFrame = colGroupFrame->GetFirstChild(nsnull);
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
        colX++;
      }
      colFrame = colFrame->GetNextSibling();
    }
    if (colGroupWidth) {
      colGroupWidth -= cellSpacingX;
    }

    nsRect colGroupRect(colGroupOrigin.x, colGroupOrigin.y, colGroupWidth, colHeight);
    colGroupFrame->SetRect(colGroupRect);
    colGroupFrame = colGroupFrame->GetNextSibling();
    colGroupOrigin.x += colGroupWidth + cellSpacingX;
  }
}





static void
ProcessRowInserted(nsTableFrame&   aTableFrame,
                   PRBool          aInvalidate,
                   nscoord         aNewHeight)
{
  aTableFrame.SetRowInserted(PR_FALSE); 
  nsTableFrame::RowGroupArray rowGroups;
  aTableFrame.OrderRowGroups(rowGroups);
  
  for (PRUint32 rgX = 0; rgX < rowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
    NS_ASSERTION(rgFrame, "Must have rgFrame here");
    nsIFrame* childFrame = rgFrame->GetFirstChild(nsnull);
    
    while (childFrame) {
      if (nsGkAtoms::tableRowFrame == childFrame->GetType()) {
        nsTableRowFrame* rowFrame = (nsTableRowFrame*)childFrame;
        if (rowFrame->IsFirstInserted()) {
          rowFrame->SetFirstInserted(PR_FALSE);
          if (aInvalidate) {
            
            nscoord damageY = rgFrame->GetPosition().y + rowFrame->GetPosition().y;
            nsRect damageRect(0, damageY,
                              aTableFrame.GetSize().width, aNewHeight - damageY);

            aTableFrame.Invalidate(damageRect);
            aTableFrame.SetRowInserted(PR_FALSE);
          }
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
  static_cast<nsTableFrame*>(GetFirstInFlow())->
    mTableLayoutStrategy->MarkIntrinsicWidthsDirty();

  

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





























































NS_METHOD nsTableFrame::Reflow(nsPresContext*          aPresContext,
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
  PRBool reflowedChildren  = PR_FALSE;
  SetHaveReflowedColGroups(PR_FALSE);

  if (aReflowState.ComputedHeight() != NS_UNCONSTRAINEDSIZE ||
      
      
      
      
      aReflowState.mFlags.mVResize) {
    
    
    
    
    
    
    
    SetGeometryDirty();
  }

  
  
  
  
  PRBool needToInitiateSpecialReflow =
    !!(GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
  if (NS_SUBTREE_DIRTY(this) ||
      aReflowState.ShouldReflowAllKids() ||
      IsGeometryDirty() ||
      needToInitiateSpecialReflow) {
    
    if (isPaginated && !GetPrevInFlow() && (NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight)) {
      nscoord tableSpecifiedHeight = CalcBorderBoxHeight(aReflowState);
      if ((tableSpecifiedHeight > 0) && 
          (tableSpecifiedHeight != NS_UNCONSTRAINEDSIZE)) {
        needToInitiateSpecialReflow = PR_TRUE;
      }
    }
    nsIFrame* lastChildReflowed = nsnull;

    nsHTMLReflowState &mutable_rs =
      const_cast<nsHTMLReflowState&>(aReflowState);
    PRBool oldSpecialHeightReflow = mutable_rs.mFlags.mSpecialHeightReflow;
    mutable_rs.mFlags.mSpecialHeightReflow = PR_FALSE;

    
    
    

    
    
    nscoord availHeight = needToInitiateSpecialReflow 
                          ? NS_UNCONSTRAINEDSIZE : aReflowState.availableHeight;

    ReflowTable(aDesiredSize, aReflowState, availHeight,
                lastChildReflowed, aStatus);
    reflowedChildren = PR_TRUE;

    
    if (GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT)
      needToInitiateSpecialReflow = PR_TRUE;

    
    if (needToInitiateSpecialReflow && NS_FRAME_IS_COMPLETE(aStatus)) {
      

      
      CalcDesiredHeight(aReflowState, aDesiredSize); 
      mutable_rs.mFlags.mSpecialHeightReflow = PR_TRUE;
      
      nsIFrame* specialReflowInitiator = aReflowState.mPercentHeightReflowInitiator;
      mutable_rs.mPercentHeightReflowInitiator = this;

      ReflowTable(aDesiredSize, aReflowState, aReflowState.availableHeight, 
                  lastChildReflowed, aStatus);
      
      mutable_rs.mPercentHeightReflowInitiator = specialReflowInitiator;

      if (lastChildReflowed && NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
        
        nsMargin borderPadding = GetChildAreaOffset(&aReflowState);
        aDesiredSize.height = borderPadding.bottom + GetCellSpacingY() +
                              lastChildReflowed->GetRect().YMost();
      }
      haveDesiredHeight = PR_TRUE;
      reflowedChildren  = PR_TRUE;
    }

    mutable_rs.mFlags.mSpecialHeightReflow = oldSpecialHeightReflow;
  }

  aDesiredSize.width = aReflowState.ComputedWidth() +
                       aReflowState.mComputedBorderPadding.LeftRight();
  if (!haveDesiredHeight) {
    CalcDesiredHeight(aReflowState, aDesiredSize); 
  }
  if (IsRowInserted()) {
    ProcessRowInserted(*this, PR_TRUE, aDesiredSize.height);
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
  
  
  
  if (reflowedChildren) {
    nsRect damage(0, 0, PR_MAX(mRect.width, aDesiredSize.width),
                  PR_MAX(mRect.height, aDesiredSize.height));
    damage.UnionRect(damage, aDesiredSize.mOverflowArea);
    nsRect* oldOverflowArea = GetOverflowAreaProperty();
    if (oldOverflowArea) {
      damage.UnionRect(damage, *oldOverflowArea);
    }
    Invalidate(damage);
  } else {
    
     nsRect* oldOverflowArea = GetOverflowAreaProperty();
     if (oldOverflowArea) {
       aDesiredSize.mOverflowArea.UnionRect(aDesiredSize.mOverflowArea, *oldOverflowArea);
     }
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
    
    
    for (nsIFrame* f = frames.FirstChild(); f; f = f->GetNextSibling()) {
      nsHTMLContainerFrame::ReparentFrameView(PresContext(), f, this, nextInFlow);
    }
    nextInFlow->mFrames.InsertFrames(GetNextInFlow(), prevSibling, frames.FirstChild());
  }
  else {
    
    SetOverflowFrames(PresContext(), frames.FirstChild());
  }
}








PRBool
nsTableFrame::MoveOverflowToChildList(nsPresContext* aPresContext)
{
  PRBool result = PR_FALSE;

  
  nsTableFrame* prevInFlow = (nsTableFrame*)GetPrevInFlow();
  if (prevInFlow) {
    nsIFrame* prevOverflowFrames = prevInFlow->GetOverflowFrames(aPresContext, PR_TRUE);
    if (prevOverflowFrames) {
      
      
      for (nsIFrame* f = prevOverflowFrames; f; f = f->GetNextSibling()) {
        nsHTMLContainerFrame::ReparentFrameView(aPresContext, f, prevInFlow, this);
      }
      mFrames.AppendFrames(this, prevOverflowFrames);
      result = PR_TRUE;
    }
  }

  
  nsIFrame* overflowFrames = GetOverflowFrames(aPresContext, PR_TRUE);
  if (overflowFrames) {
    mFrames.AppendFrames(nsnull, overflowFrames);
    result = PR_TRUE;
  }
  return result;
}





void
nsTableFrame::AdjustForCollapsingRowsCols(nsHTMLReflowMetrics& aDesiredSize,
                                          nsMargin             aBorderPadding)
{
  nscoord yTotalOffset = 0; 

  
  SetNeedToCollapse(PR_FALSE);
  
  
  
  RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);
  nscoord width = GetCollapsedWidth(aBorderPadding);
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
          if (GetNumCellsOriginatingInCol(colX) > 0)
            width += cellSpacingX;
        }
      }
    }
  }
  return width;
}




NS_IMETHODIMP
nsTableFrame::AppendFrames(nsIAtom*        aListName,
                           nsIFrame*       aFrameList)
{
  NS_ASSERTION(!aListName || aListName == nsGkAtoms::colGroupList,
               "unexpected child list");

  
  
  
  
  nsIFrame* f = aFrameList;
  while (f) {
    
    nsIFrame* next = f->GetNextSibling();
    f->SetNextSibling(nsnull);

    
    const nsStyleDisplay* display = f->GetStyleDisplay();

    if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == display->mDisplay) {
      nsTableColGroupFrame* lastColGroup;
      PRBool doAppend = nsTableColGroupFrame::GetLastRealColGroup(this, (nsIFrame**) &lastColGroup);
      PRInt32 startColIndex = (lastColGroup) 
        ? lastColGroup->GetStartColumnIndex() + lastColGroup->GetColCount() : 0;
      if (doAppend) {
        
        mColGroups.AppendFrame(nsnull, f);
      }
      else {
        
          mColGroups.InsertFrame(nsnull, lastColGroup, f);
      }
      
      InsertColGroups(startColIndex, f, f);
    } else if (IsRowGroup(display->mDisplay)) {
      
      mFrames.AppendFrame(nsnull, f);

      
      InsertRowGroups(f, f);
    } else {
      
      mFrames.AppendFrame(nsnull, f);
    }

    
    f = next;
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
                           nsIFrame*       aFrameList)
{
  
  
  
  
  
  NS_PRECONDITION(!aFrameList->GetNextSibling(), "expected only one child frame");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  
  const nsStyleDisplay* display = aFrameList->GetStyleDisplay();
  if (aPrevFrame) {
    const nsStyleDisplay* prevDisplay = aPrevFrame->GetStyleDisplay();
    
    if ((display->mDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP) !=
        (prevDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP)) {
      
      
      
      nsIFrame* pseudoFrame = aFrameList;
      nsIContent* parentContent = GetContent();
      nsIContent* content;
      aPrevFrame = nsnull;
      while (pseudoFrame  && (parentContent ==
                              (content = pseudoFrame->GetContent()))) {
        pseudoFrame = pseudoFrame->GetFirstChild(nsnull);
      }
      nsCOMPtr<nsIContent> container = content->GetParent();
      PRInt32 newIndex = container->IndexOf(content);
      nsIFrame* kidFrame;
      PRBool isColGroup = (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP ==
                           display->mDisplay);
      if (isColGroup) {
        kidFrame = mColGroups.FirstChild();
      }
      else {
        kidFrame = mFrames.FirstChild();
      }
      
      PRInt32 lastIndex = -1;
      while (kidFrame) {
        if (isColGroup) {
          nsTableColGroupType groupType =
            ((nsTableColGroupFrame *)kidFrame)->GetColType();
          if (eColGroupAnonymousCell == groupType) {
            continue;
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
  if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == display->mDisplay) {
    NS_ASSERTION(!aListName || aListName == nsGkAtoms::colGroupList,
                 "unexpected child list");
    
    nsFrameList frames(aFrameList); 
    nsIFrame* lastFrame = frames.LastChild();
    mColGroups.InsertFrame(nsnull, aPrevFrame, aFrameList);
    
    PRInt32 startColIndex = 0;
    if (aPrevFrame) {
      nsTableColGroupFrame* prevColGroup = 
        (nsTableColGroupFrame*)GetFrameAtOrBefore(this, aPrevFrame,
                                                  nsGkAtoms::tableColGroupFrame);
      if (prevColGroup) {
        startColIndex = prevColGroup->GetStartColumnIndex() + prevColGroup->GetColCount();
      }
    }
    InsertColGroups(startColIndex, aFrameList, lastFrame);
  } else if (IsRowGroup(display->mDisplay)) {
    NS_ASSERTION(!aListName, "unexpected child list");
    nsFrameList newList(aFrameList);
    nsIFrame* lastSibling = newList.LastChild();
    
    mFrames.InsertFrame(nsnull, aPrevFrame, aFrameList);

    InsertRowGroups(aFrameList, lastSibling);
  } else {
    NS_ASSERTION(!aListName, "unexpected child list");
    
    mFrames.InsertFrame(nsnull, aPrevFrame, aFrameList);
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
      nsTableColFrame* colFrame = (nsTableColFrame*)mColFrames.SafeElementAt(colX);
      if (colFrame) {
        RemoveCol(colGroup, colX, PR_TRUE, PR_FALSE);
      }
    }

    PRInt32 numAnonymousColsToAdd = GetColCount() - mColFrames.Count();
    if (numAnonymousColsToAdd > 0) {
      
      CreateAnonymousColFrames(numAnonymousColsToAdd,
                               eColAnonymousCell, PR_TRUE);
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
        ResetRowIndices();
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
    border.top += BC_BORDER_TOP_HALF_COORD(p2t, propData->mTopBorderWidth);
    border.right += BC_BORDER_RIGHT_HALF_COORD(p2t, propData->mRightBorderWidth);
    border.bottom += BC_BORDER_BOTTOM_HALF_COORD(p2t, propData->mBottomBorderWidth);
    border.left += BC_BORDER_LEFT_HALF_COORD(p2t, propData->mLeftBorderWidth);
  }
  return border;
}

nsMargin
nsTableFrame::GetIncludedOuterBCBorder() const
{
  if (eCompatibility_NavQuirks == PresContext()->CompatibilityMode()) {
    return GetOuterBCBorder();
  }
  nsMargin border(0, 0, 0, 0);
  return border;
}

nsMargin
nsTableFrame::GetExcludedOuterBCBorder() const
{
  if (eCompatibility_NavQuirks != PresContext()->CompatibilityMode()) {
    return GetOuterBCBorder();
  }
  nsMargin border(0, 0, 0, 0);
  return border;
}
static
void GetSeparateModelBorderPadding(const nsHTMLReflowState* aReflowState,
                                   nsStyleContext&          aStyleContext,
                                   nsMargin&                aBorderPadding)
{
  
  
  
  const nsStyleBorder* border = aStyleContext.GetStyleBorder();
  aBorderPadding = border->GetBorder();
  if (aReflowState) {
    aBorderPadding += aReflowState->mComputedPadding;
  }
}

nsMargin 
nsTableFrame::GetChildAreaOffset(const nsHTMLReflowState* aReflowState) const
{
  nsMargin offset(0,0,0,0);
  if (IsBorderCollapse()) {
    nsPresContext* presContext = PresContext();
    if (eCompatibility_NavQuirks == presContext->CompatibilityMode()) {
      nsTableFrame* firstInFlow = (nsTableFrame*)GetFirstInFlow(); if (!firstInFlow) ABORT1(offset);
      PRInt32 p2t = nsPresContext::AppUnitsPerCSSPixel();
      BCPropertyData* propData = 
        (BCPropertyData*)nsTableFrame::GetProperty((nsIFrame*)firstInFlow, nsGkAtoms::tableBCProperty, PR_FALSE);
      if (!propData) ABORT1(offset);

      offset.top += BC_BORDER_TOP_HALF_COORD(p2t, propData->mTopBorderWidth);
      offset.right += BC_BORDER_RIGHT_HALF_COORD(p2t, propData->mRightBorderWidth);
      offset.bottom += BC_BORDER_BOTTOM_HALF_COORD(p2t, propData->mBottomBorderWidth);
      offset.left += BC_BORDER_LEFT_HALF_COORD(p2t, propData->mLeftBorderWidth);
    }
  }
  else {
    GetSeparateModelBorderPadding(aReflowState, *mStyleContext, offset);
  }
  return offset;
}

nsMargin 
nsTableFrame::GetContentAreaOffset(const nsHTMLReflowState* aReflowState) const
{
  nsMargin offset(0,0,0,0);
  if (IsBorderCollapse()) {
    
    
    offset = GetOuterBCBorder();
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
                              nsHTMLReflowMetrics& aKidDesiredSize)
{
  
  
  FinishReflowChild(aKidFrame, PresContext(), nsnull, aKidDesiredSize,
                    aReflowState.x, aReflowState.y, 0);

  
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
IsRepeatable(nsTableRowGroupFrame& aHeaderOrFooter,
             nscoord               aPageHeight)
{
  return aHeaderOrFooter.GetSize().height < (aPageHeight / 4);
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
      
      
      
      nsIFrame* repeatedFooter = nsnull;
      nscoord repeatedFooterHeight = 0;
      if (isPaginated && (NS_UNCONSTRAINEDSIZE != kidAvailSize.height)) {
        if (NS_STYLE_DISPLAY_TABLE_ROW_GROUP == kidFrame->GetStyleDisplay()->mDisplay) { 
          nsIFrame* lastChild = rowGroups[rowGroups.Length() - 1];
          if (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == lastChild->GetStyleDisplay()->mDisplay) { 
            
            
            if (((nsTableRowGroupFrame*)lastChild)->IsRepeatable()) {
              repeatedFooterHeight = lastChild->GetSize().height;
              if (repeatedFooterHeight + cellSpacingY < kidAvailSize.height) {
                repeatedFooter = lastChild;
                kidAvailSize.height -= repeatedFooterHeight + cellSpacingY;
              }
            }
          }
        }
      }

      nsRect oldKidRect = kidFrame->GetRect();

      nsHTMLReflowMetrics desiredSize;
      desiredSize.width = desiredSize.height = 0;
  
      
      nsHTMLReflowState kidReflowState(presContext, aReflowState.reflowState,
                                       kidFrame, kidAvailSize,
                                       -1, -1, PR_FALSE);
      InitChildReflowState(kidReflowState);

      
      
      
      
      if (childX > (thead ? 1 : 0)) {
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
                       aReflowState.x, aReflowState.y, 0, aStatus);

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
              PlaceChild(aReflowState, kidFrame, desiredSize);
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

      
      PlaceChild(aReflowState, kidFrame, desiredSize);

      
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
        if (repeatedFooter) {
          kidAvailSize.height = repeatedFooterHeight;
          nsHTMLReflowState footerReflowState(presContext,
                                              aReflowState.reflowState,
                                              repeatedFooter, kidAvailSize,
                                              -1, -1, PR_FALSE);
          InitChildReflowState(footerReflowState);
          aReflowState.y += cellSpacingY;
          nsReflowStatus footerStatus;
          rv = ReflowChild(repeatedFooter, presContext, desiredSize, footerReflowState,
                           aReflowState.x, aReflowState.y, 0, footerStatus);
          PlaceChild(aReflowState, repeatedFooter, desiredSize);
        }
        break;
      }
    }
    else { 
      aReflowState.y += cellSpacingY;
      nsRect kidRect = kidFrame->GetRect();
      if (kidRect.y != aReflowState.y) {
        Invalidate(kidRect); 
        kidRect.y = aReflowState.y;
        kidFrame->SetRect(kidRect);        
        RePositionViews(kidFrame);
        Invalidate(kidRect); 
      }
      aReflowState.y += kidRect.height;

      
      if (NS_UNCONSTRAINEDSIZE != aReflowState.availSize.height) {
        aReflowState.availSize.height -= cellSpacingY + kidRect.height;
      }
    }
    ConsiderChildOverflow(aOverflowArea, kidFrame);
  }
  
  
  
  if (isPaginated && !GetPrevInFlow() && (NS_UNCONSTRAINEDSIZE == aReflowState.availSize.height)) {
    nscoord height = presContext->GetPageSize().height;
    
    if (thead && height != NS_UNCONSTRAINEDSIZE) {
      thead->SetRepeatable(IsRepeatable(*thead, height));
    }
    if (tfoot && height != NS_UNCONSTRAINEDSIZE) {
      tfoot->SetRepeatable(IsRepeatable(*tfoot, height));
    }
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
            rowRect.height += amountForRow;
            rowFrame->SetRect(rowRect);
            yOriginRow += rowRect.height + cellSpacingY;
            yEndRG += rowRect.height + cellSpacingY;
            amountUsed += amountForRow;
            amountUsedByRG += amountForRow;
            
            nsTableFrame::RePositionViews(rowFrame);
          }
        }
        else {
          if (amountUsed > 0) {
            rowFrame->SetPosition(nsPoint(rowRect.x, yOriginRow));
            nsTableFrame::RePositionViews(rowFrame);
          }
          yOriginRow += rowRect.height + cellSpacingY;
          yEndRG += rowRect.height + cellSpacingY;
        }
        rowFrame = rowFrame->GetNextRow();
      }
      if (amountUsed > 0) {
        rgRect.y = yOriginRG;
        rgRect.height += amountUsedByRG;
        rgFrame->SetRect(rgRect);
      }
    }
    else if (amountUsed > 0) {
      rgFrame->SetPosition(nsPoint(0, yOriginRG));
      
      nsTableFrame::RePositionViews(rgFrame);
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

  nsTableRowFrame* lastElligibleRow = nsnull;
  
  
  nscoord divisor = 0;
  for (rgX = 0; rgX < rowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
    if (!firstUnStyledRG || !rgFrame->HasStyleHeight()) {
      nsTableRowFrame* rowFrame = rgFrame->GetFirstRow();
      while (rowFrame) {
        if (!firstUnStyledRG || !rowFrame->HasStyleHeight()) {
          divisor += rowFrame->GetSize().height;
          lastElligibleRow = rowFrame;
        }
        rowFrame = rowFrame->GetNextRow();
      }
    }
  }
  if (divisor <= 0) {
    NS_ERROR("invalid divisor");
    return;
  }

  
  pctBasis = aAmount - amountUsed;
  yOriginRG = borderPadding.top + cellSpacingY;
  yEndRG = yOriginRG;
  for (rgX = 0; rgX < rowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
    nscoord amountUsedByRG = 0;
    nscoord yOriginRow = 0;
    nsRect rgRect = rgFrame->GetRect();
    
    if (!firstUnStyledRG || !rgFrame->HasStyleHeight()) {
      nsTableRowFrame* rowFrame = rgFrame->GetFirstRow();
      while (rowFrame) {
        nsRect rowRect = rowFrame->GetRect();
        
        if (!firstUnStyledRow || !rowFrame->HasStyleHeight()) {
          
          float percent = rowRect.height / ((float)divisor);
          
          nscoord amountForRow = (rowFrame == lastElligibleRow) 
                                 ? aAmount - amountUsed : NSToCoordRound(((float)(pctBasis)) * percent);
          amountForRow = PR_MIN(amountForRow, aAmount - amountUsed);
          
          nsRect newRowRect(rowRect.x, yOriginRow, rowRect.width, rowRect.height + amountForRow);
          rowFrame->SetRect(newRowRect);
          yOriginRow += newRowRect.height + cellSpacingY;
          yEndRG += newRowRect.height + cellSpacingY;

          amountUsed += amountForRow;
          amountUsedByRG += amountForRow;
          NS_ASSERTION((amountUsed <= aAmount), "invalid row allocation");
          
          nsTableFrame::RePositionViews(rowFrame);
        }
        else {
          if (amountUsed > 0) {
            rowFrame->SetPosition(nsPoint(rowRect.x, yOriginRow));
            nsTableFrame::RePositionViews(rowFrame);
          }
          yOriginRow += rowRect.height + cellSpacingY;
          yEndRG += rowRect.height + cellSpacingY;
        }
        rowFrame = rowFrame->GetNextRow();
      }
      if (amountUsed > 0) {
        rgRect.y = yOriginRG;
        rgRect.height += amountUsedByRG;
        rgFrame->SetRect(rgRect);
      }
      
      
    }
    else if (amountUsed > 0) {
      rgFrame->SetPosition(nsPoint(0, yOriginRG));
      
      nsTableFrame::RePositionViews(rgFrame);
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

  NS_ASSERTION(GetStyleTableBorder()->mBorderSpacingX.GetUnit() == eStyleUnit_Coord,
               "Not a coord value!");
  return GetStyleTableBorder()->mBorderSpacingX.GetCoordValue();
}


nscoord nsTableFrame::GetCellSpacingY()
{
  if (IsBorderCollapse())
    return 0;

  NS_ASSERTION(GetStyleTableBorder()->mBorderSpacingY.GetUnit() == eStyleUnit_Coord,
               "Not a coord value!");
  return GetStyleTableBorder()->mBorderSpacingY.GetCoordValue();
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
    nsMargin borderPadding = GetContentAreaOffset(&aState);
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
  return (GetStyleDisplay()->mDisplay == NS_STYLE_DISPLAY_INLINE_TABLE &&
          width.GetUnit() == eStyleUnit_Auto) ||
         (width.GetUnit() == eStyleUnit_Enumerated &&
          width.GetIntValue() == NS_STYLE_WIDTH_INTRINSIC);
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
    nsIFrame* rowFrame = rgFrame->GetFirstChild(nsnull);
    while (rowFrame) {
      if (nsGkAtoms::tableRowFrame == rowFrame->GetType()) {
        printf("row(%d)=%p ", ((nsTableRowFrame*)rowFrame)->GetRowIndex(), rowFrame);
        nsIFrame* cellFrame = rowFrame->GetFirstChild(nsnull);
        while (cellFrame) {
          if (IS_TABLE_CELL(cellFrame->GetType())) {
            PRInt32 colIndex;
            ((nsTableCellFrame*)cellFrame)->GetColIndex(colIndex);
            printf("cell(%d)=%p ", colIndex, cellFrame);
          }
          cellFrame = cellFrame->GetNextSibling();
        }
        printf("\n");
      }
      else {
        DumpRowGroup(rowFrame);
      }
      rowFrame = rowFrame->GetNextSibling();
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
      nsTableColFrame* colFrame = (nsTableColFrame *)mColFrames.ElementAt(colX);
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



PRInt32 nsTableFrame::GetNumCellsOriginatingInCol(PRInt32 aColIndex) const
{
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) 
    return cellMap->GetNumCellsOriginatingInCol(aColIndex);
  else
    return 0;
}

PRInt32 nsTableFrame::GetNumCellsOriginatingInRow(PRInt32 aRowIndex) const
{
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) 
    return cellMap->GetNumCellsOriginatingInRow(aRowIndex);
  else
    return 0;
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



struct BCMapCellInfo 
{
  BCMapCellInfo();
  void Reset();

  CellData*             cellData;
  nsCellMap*            cellMap;

  nsTableRowGroupFrame* rg;

  nsTableRowFrame*      topRow;
  nsTableRowFrame*      bottomRow;

  nsTableColGroupFrame* cg;
 
  nsTableColFrame*      leftCol;
  nsTableColFrame*      rightCol;

  nsBCTableCellFrame*   cell;

  PRInt32               rowIndex;
  PRInt32               rowSpan;
  PRInt32               colIndex;
  PRInt32               colSpan;

  PRPackedBool          rgTop;
  PRPackedBool          rgBottom;
  PRPackedBool          cgLeft;
  PRPackedBool          cgRight;
};

BCMapCellInfo::BCMapCellInfo()
{
  Reset();
}

void BCMapCellInfo::Reset()
{
  cellData  = nsnull;
  rg        = nsnull;
  topRow    = nsnull;
  bottomRow = nsnull;
  cg        = nsnull;
  leftCol   = nsnull;
  rightCol  = nsnull;
  cell      = nsnull;
  rowIndex = rowSpan = colIndex = colSpan = 0;
  rgTop = rgBottom = cgLeft = cgRight = PR_FALSE;
}

class BCMapCellIterator
{
public:
  BCMapCellIterator(nsTableFrame& aTableFrame,
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

  PRInt32    mRowGroupStart;
  PRInt32    mRowGroupEnd;
  PRBool     mAtEnd;
  nsCellMap* mCellMap;

private:
  void SetInfo(nsTableRowFrame* aRow,
               PRInt32          aColIndex,
               CellData*        aCellData,
               BCMapCellInfo&   aMapInfo,
               nsCellMap*       aCellMap = nsnull);

  PRBool SetNewRow(nsTableRowFrame* row = nsnull);
  PRBool SetNewRowGroup(PRBool aFindFirstDamagedRow);

  nsTableFrame&         mTableFrame;
  nsTableCellMap*       mTableCellMap;
  nsTableFrame::RowGroupArray mRowGroups;
  nsTableRowGroupFrame* mRowGroup;
  PRInt32               mRowGroupIndex;
  PRUint32              mNumRows;
  nsTableRowFrame*      mRow;
  nsTableRowFrame*      mPrevRow;
  PRBool                mIsNewRow;
  PRInt32               mRowIndex;
  PRUint32              mNumCols;
  PRInt32               mColIndex;
  nsPoint               mAreaStart;
  nsPoint               mAreaEnd;
};

BCMapCellIterator::BCMapCellIterator(nsTableFrame& aTableFrame,
                                     const nsRect& aDamageArea)
:mTableFrame(aTableFrame)
{
  mTableCellMap  = aTableFrame.GetCellMap();

  mAreaStart.x   = aDamageArea.x;
  mAreaStart.y   = aDamageArea.y;
  mAreaEnd.y     = aDamageArea.y + aDamageArea.height - 1;
  mAreaEnd.x     = aDamageArea.x + aDamageArea.width - 1;

  mNumRows       = mTableFrame.GetRowCount();
  mRow           = nsnull;
  mRowIndex      = 0;
  mNumCols       = mTableFrame.GetColCount();
  mColIndex      = 0;
  mRowGroupIndex = -1;

  
  aTableFrame.OrderRowGroups(mRowGroups);

  mAtEnd = PR_TRUE; 
}

void 
BCMapCellIterator::SetInfo(nsTableRowFrame* aRow,
                           PRInt32          aColIndex,
                           CellData*        aCellData,
                           BCMapCellInfo&   aCellInfo,
                           nsCellMap*       aCellMap)
{
  aCellInfo.cellData = aCellData;
  aCellInfo.cellMap = (aCellMap) ? aCellMap : mCellMap;
  aCellInfo.colIndex = aColIndex;

  
  aCellInfo.rowIndex = 0;
  if (aRow) {
    aCellInfo.topRow = aRow; 
    aCellInfo.rowIndex = aRow->GetRowIndex();
  }

  
  aCellInfo.cell      = nsnull;
  aCellInfo.rowSpan   = 1;
  aCellInfo.colSpan  = 1;
  if (aCellData) {
    aCellInfo.cell = (nsBCTableCellFrame*)aCellData->GetCellFrame(); 
    if (aCellInfo.cell) {
      if (!aCellInfo.topRow) {
        aCellInfo.topRow = static_cast<nsTableRowFrame*>
                                      (aCellInfo.cell->GetParent());
        if (!aCellInfo.topRow) ABORT0();
        aCellInfo.rowIndex = aCellInfo.topRow->GetRowIndex();
      }
      aCellInfo.colSpan = mTableFrame.GetEffectiveColSpan(*aCellInfo.cell, aCellMap); 
      aCellInfo.rowSpan = mTableFrame.GetEffectiveRowSpan(*aCellInfo.cell, aCellMap);
    }
  }
  if (!aCellInfo.topRow) {
    aCellInfo.topRow = mRow;
  }

  if (1 == aCellInfo.rowSpan) {
    aCellInfo.bottomRow = aCellInfo.topRow;
  }
  else {
    aCellInfo.bottomRow = aCellInfo.topRow->GetNextRow();
    if (aCellInfo.bottomRow) {
      for (PRInt32 spanX = 2; aCellInfo.bottomRow && (spanX < aCellInfo.rowSpan); spanX++) {
        aCellInfo.bottomRow = aCellInfo.bottomRow->GetNextRow();
      }
      NS_ASSERTION(aCellInfo.bottomRow, "program error");
    }
    else {
      NS_ASSERTION(PR_FALSE, "error in cell map");
      aCellInfo.rowSpan = 1;
      aCellInfo.bottomRow = aCellInfo.topRow;
    }
  }

  
  PRUint32 rgStart  = mRowGroupStart;
  PRUint32 rgEnd    = mRowGroupEnd;
  aCellInfo.rg = mTableFrame.GetRowGroupFrame(aCellInfo.topRow->GetParent());
  if (aCellInfo.rg != mRowGroup) {
    rgStart = aCellInfo.rg->GetStartRowIndex();
    rgEnd   = rgStart + aCellInfo.rg->GetRowCount() - 1;
  }
  PRUint32 rowIndex  = aCellInfo.topRow->GetRowIndex();
  aCellInfo.rgTop    = (rgStart == rowIndex);
  aCellInfo.rgBottom = (rgEnd == rowIndex + aCellInfo.rowSpan - 1);

  
  aCellInfo.leftCol = mTableFrame.GetColFrame(aColIndex); if (!aCellInfo.leftCol) ABORT0();

  aCellInfo.rightCol = aCellInfo.leftCol;
  if (aCellInfo.colSpan > 1) {
    for (PRInt32 spanX = 1; spanX < aCellInfo.colSpan; spanX++) {
      nsTableColFrame* colFrame = mTableFrame.GetColFrame(aColIndex + spanX); if (!colFrame) ABORT0();
      aCellInfo.rightCol = colFrame;
    }
  }

  
  aCellInfo.cg = static_cast<nsTableColGroupFrame*>
                            (aCellInfo.leftCol->GetParent());
  PRInt32 cgStart  = aCellInfo.cg->GetStartColumnIndex();
  PRInt32 cgEnd    = PR_MAX(0, cgStart + aCellInfo.cg->GetColCount() - 1);
  aCellInfo.cgLeft  = (cgStart == aColIndex);
  aCellInfo.cgRight = (cgEnd == aColIndex + (PRInt32)aCellInfo.colSpan - 1);
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
  mRowGroupIndex++;
  PRInt32 numRowGroups = mRowGroups.Length();
  mCellMap = nsnull;
  for (PRInt32 rgX = mRowGroupIndex; rgX < numRowGroups; rgX++) {
    
    
    
    
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
          mRowGroupIndex++;
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
  aMapInfo.Reset();

  SetNewRowGroup(PR_TRUE); 
  while (!mAtEnd) {
    if ((mAreaStart.y >= mRowGroupStart) && (mAreaStart.y <= mRowGroupEnd)) {
      CellData* cellData = mCellMap->GetDataAt(mAreaStart.y - mRowGroupStart,
                                               mAreaStart.x);
      if (cellData && cellData->IsOrig()) {
        SetInfo(mRow, mAreaStart.x, cellData, aMapInfo);
      }
      else {
        NS_ASSERTION(((0 == mAreaStart.x) && (mRowGroupStart == mAreaStart.y)) , "damage area expanded incorrectly");
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
  aMapInfo.Reset();

  mIsNewRow = PR_FALSE;
  mColIndex++;
  while ((mRowIndex <= mAreaEnd.y) && !mAtEnd) {
    for (; mColIndex <= mAreaEnd.x; mColIndex++) {
      PRInt32 rgRowIndex = mRowIndex - mRowGroupStart;
      CellData* cellData = mCellMap->GetDataAt(rgRowIndex, mColIndex);
      if (!cellData) { 
        nsRect damageArea;
        cellData = mCellMap->AppendCell(*mTableCellMap, nsnull, rgRowIndex, PR_FALSE, damageArea); if (!cellData) ABORT0();
      }
      if (cellData && (cellData->IsOrig() || cellData->IsDead())) {
        SetInfo(mRow, mColIndex, cellData, aMapInfo);
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
  aAjaInfo.Reset();
  PRInt32 colIndex = aRefInfo.colIndex + aRefInfo.colSpan;
  PRUint32 rgRowIndex = aRowIndex - mRowGroupStart;

  CellData* cellData = mCellMap->GetDataAt(rgRowIndex, colIndex);
  if (!cellData) { 
    NS_ASSERTION(colIndex < mTableCellMap->GetColCount(), "program error");
    nsRect damageArea;
    cellData = mCellMap->AppendCell(*mTableCellMap, nsnull, rgRowIndex, PR_FALSE, damageArea); if (!cellData) ABORT0();
  }
  nsTableRowFrame* row = nsnull;
  if (cellData->IsRowSpan()) {
    rgRowIndex -= cellData->GetRowSpanOffset();
    cellData = mCellMap->GetDataAt(rgRowIndex, colIndex);
    if (!cellData)
      ABORT0();
  }
  else {
    row = mRow;
  }
  SetInfo(row, colIndex, cellData, aAjaInfo);
}

void 
BCMapCellIterator::PeekBottom(BCMapCellInfo&   aRefInfo,
                              PRUint32         aColIndex,
                              BCMapCellInfo&   aAjaInfo)
{
  aAjaInfo.Reset();
  PRInt32 rowIndex = aRefInfo.rowIndex + aRefInfo.rowSpan;
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
    for (PRInt32 i = 0; i < aRefInfo.rowSpan; i++) {
      nextRow = nextRow->GetNextRow(); if (!nextRow) ABORT0();
    }
  }

  CellData* cellData = cellMap->GetDataAt(rgRowIndex, aColIndex);
  if (!cellData) { 
    NS_ASSERTION(rgRowIndex < cellMap->GetRowCount(), "program error");
    nsRect damageArea;
    cellData = cellMap->AppendCell(*mTableCellMap, nsnull, rgRowIndex, PR_FALSE, damageArea); if (!cellData) ABORT0();
  }
  if (cellData->IsColSpan()) {
    aColIndex -= cellData->GetColSpanOffset();
    cellData = cellMap->GetDataAt(rgRowIndex, aColIndex);
  }
  SetInfo(nextRow, aColIndex, cellData, aAjaInfo, cellMap);
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
  PRBool transparent, foreground;
  styleData->GetBorderColor(aSide, aColor, transparent, foreground);
  if (transparent) { 
    aColor = 0;
  }
  else if (foreground) {
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
                 nscoord&         aWidth)
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
  width = styleData->GetBorderWidth(aSide);
  aWidth = nsPresContext::AppUnitsToIntCSSPixels(width);
}
 
 












struct BCCellBorder
{
  BCCellBorder() { Reset(0, 1); }
  void Reset(PRUint32 aRowIndex, PRUint32 aRowSpan);
  nscolor       color;    
  nscoord       width;    
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









































#define TOP_DAMAGED(aRowIndex)    ((aRowIndex) >= propData->mDamageArea.y) 
#define RIGHT_DAMAGED(aColIndex)  ((aColIndex) <  propData->mDamageArea.XMost()) 
#define BOTTOM_DAMAGED(aRowIndex) ((aRowIndex) <  propData->mDamageArea.YMost()) 
#define LEFT_DAMAGED(aColIndex)   ((aColIndex) >= propData->mDamageArea.x) 

#define TABLE_EDGE  PR_TRUE
#define ADJACENT    PR_TRUE
#define HORIZONTAL  PR_TRUE


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
    (BCPropertyData*)nsTableFrame::GetProperty(this, nsGkAtoms::tableBCProperty, PR_FALSE);
  if (!propData) ABORT0();

  PRBool tableIsLTR = GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_LTR;
  PRUint8 firstSide, secondSide;
  if (tableIsLTR) {
    firstSide  = NS_SIDE_LEFT;
    secondSide = NS_SIDE_RIGHT;
  }
  else {
    firstSide  = NS_SIDE_RIGHT;
    secondSide = NS_SIDE_LEFT;
  }
  CheckFixDamageArea(numRows, numCols, propData->mDamageArea);
  
  nsRect damageArea(propData->mDamageArea);
  ExpandBCDamageArea(damageArea);

  
  PRBool tableBorderReset[4];
  for (PRUint32 sideX = NS_SIDE_TOP; sideX <= NS_SIDE_LEFT; sideX++) {
    tableBorderReset[sideX] = PR_FALSE;
  }

  
  BCCellBorders lastVerBorders(damageArea.width + 1, damageArea.x); if (!lastVerBorders.borders) ABORT0();
  BCCellBorder  lastTopBorder, lastBottomBorder;
  
  BCCellBorders lastBottomBorders(damageArea.width + 1, damageArea.x); if (!lastBottomBorders.borders) ABORT0();
  PRBool startSeg;
  PRBool gotRowBorder = PR_FALSE;

  BCMapCellInfo  info, ajaInfo;
  BCCellBorder currentBorder, adjacentBorder;
  PRInt32   cellEndRowIndex = -1;
  PRInt32   cellEndColIndex = -1;
  BCCorners topCorners(damageArea.width + 1, damageArea.x); if (!topCorners.corners) ABORT0();
  BCCorners bottomCorners(damageArea.width + 1, damageArea.x); if (!bottomCorners.corners) ABORT0();

  BCMapCellIterator iter(*this, damageArea);
  for (iter.First(info); !iter.mAtEnd; iter.Next(info)) {

    cellEndRowIndex = info.rowIndex + info.rowSpan - 1;
    cellEndColIndex = info.colIndex + info.colSpan - 1;
    
    PRBool bottomRowSpan = PR_FALSE;
    
    if (iter.IsNewRow()) { 
      gotRowBorder = PR_FALSE;
      lastTopBorder.Reset(info.rowIndex, info.rowSpan);
      lastBottomBorder.Reset(cellEndRowIndex + 1, info.rowSpan);
    }
    else if (info.colIndex > damageArea.x) {
      lastBottomBorder = lastBottomBorders[info.colIndex - 1];
      if (info.rowIndex > lastBottomBorder.rowIndex - lastBottomBorder.rowSpan) { 
        
        lastTopBorder.Reset(info.rowIndex, info.rowSpan);
      }
      if (lastBottomBorder.rowIndex > (cellEndRowIndex + 1)) {
        
        lastBottomBorder.Reset(cellEndRowIndex + 1, info.rowSpan);
        bottomRowSpan = PR_TRUE;
      }
    }

    
    
    if (0 == info.rowIndex) {
      if (!tableBorderReset[NS_SIDE_TOP]) {
        propData->mTopBorderWidth = 0;
        tableBorderReset[NS_SIDE_TOP] = PR_TRUE;
      }
      for (PRInt32 colX = info.colIndex; colX <= cellEndColIndex; colX++) {
        nsIFrame* colFrame = GetColFrame(colX); if (!colFrame) ABORT0();
        nsIFrame* cgFrame = colFrame->GetParent(); if (!cgFrame) ABORT0();
        currentBorder = CompareBorders(this, cgFrame, colFrame, info.rg, info.topRow,
                                       info.cell, tableIsLTR, TABLE_EDGE, NS_SIDE_TOP,
                                       !ADJACENT);
        
        BCCornerInfo& tlCorner = topCorners[colX]; 
        if (0 == colX) {
          tlCorner.Set(NS_SIDE_RIGHT, currentBorder); 
        }
        else {
          tlCorner.Update(NS_SIDE_RIGHT, currentBorder);
          tableCellMap->SetBCBorderCorner(eTopLeft, *info.cellMap, 0, 0, colX,
                                          tlCorner.ownerSide, tlCorner.subWidth, tlCorner.bevel);
        }
        topCorners[colX + 1].Set(NS_SIDE_LEFT, currentBorder); 
        
        startSeg = SetHorBorder(currentBorder, tlCorner, lastTopBorder);
        
        tableCellMap->SetBCBorderEdge(NS_SIDE_TOP, *info.cellMap, 0, 0, colX,
                                      1, currentBorder.owner, currentBorder.width, startSeg);
        
        if (info.cell) {
          info.cell->SetBorderWidth(NS_SIDE_TOP, PR_MAX(currentBorder.width, info.cell->GetBorderWidth(NS_SIDE_TOP)));
        }
        if (info.topRow) {
          BCPixelSize half = BC_BORDER_BOTTOM_HALF(currentBorder.width);
          info.topRow->SetTopBCBorderWidth(PR_MAX(half, info.topRow->GetTopBCBorderWidth()));
        }
        propData->mTopBorderWidth = LimitBorderWidth(PR_MAX(propData->mTopBorderWidth, (PRUint8)currentBorder.width));
        
        
        currentBorder = CompareBorders(this, cgFrame, colFrame, info.rg,
                                       info.topRow, nsnull, tableIsLTR, 
                                       TABLE_EDGE, NS_SIDE_TOP, !ADJACENT);
        ((nsTableColFrame*)colFrame)->SetContinuousBCBorderWidth(NS_SIDE_TOP,
                                                                 currentBorder.width);
        if (numCols == cellEndColIndex + 1) {
          currentBorder = CompareBorders(this, cgFrame, colFrame, nsnull,
                                         nsnull, nsnull, tableIsLTR, TABLE_EDGE,
                                         NS_SIDE_RIGHT, !ADJACENT);
        }
        else {
          currentBorder = CompareBorders(nsnull, cgFrame, colFrame, nsnull,
                                         nsnull, nsnull, tableIsLTR, !TABLE_EDGE,
                                         NS_SIDE_RIGHT, !ADJACENT);
        }
        ((nsTableColFrame*)colFrame)->SetContinuousBCBorderWidth(NS_SIDE_RIGHT,
                                                                 currentBorder.width);
        
      }
      
      
      if (info.topRow) {
        currentBorder = CompareBorders(this, nsnull, nsnull, info.rg,
                                       info.topRow, nsnull, tableIsLTR,
                                       TABLE_EDGE, NS_SIDE_TOP, !ADJACENT);
        info.topRow->SetContinuousBCBorderWidth(NS_SIDE_TOP, currentBorder.width);
      }
      if (info.cgRight && info.cg) {
        
        currentBorder = CompareBorders(this, info.cg, nsnull, info.rg,
                                       info.topRow, nsnull, tableIsLTR, 
                                       TABLE_EDGE, NS_SIDE_TOP, !ADJACENT);
        info.cg->SetContinuousBCBorderWidth(NS_SIDE_TOP, currentBorder.width);
      }
      if (0 == info.colIndex) {
        currentBorder = CompareBorders(this, info.cg, info.leftCol, nsnull,
                                       nsnull, nsnull, tableIsLTR, TABLE_EDGE,
                                       NS_SIDE_LEFT, !ADJACENT);
        mBits.mLeftContBCBorder = currentBorder.width;
      }
    }
    else {
      
      if (info.colIndex > 0) {
        BCData& data = ((BCCellData*)info.cellData)->mData;
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

    
    
    if (0 == info.colIndex) {
      if (!tableBorderReset[NS_SIDE_LEFT]) {
        propData->mLeftBorderWidth = 0;
        tableBorderReset[NS_SIDE_LEFT] = PR_TRUE;
      }
      nsTableRowFrame* rowFrame = nsnull;
      for (PRInt32 rowX = info.rowIndex; rowX <= cellEndRowIndex; rowX++) {
        rowFrame = (rowX == info.rowIndex) ? info.topRow : rowFrame->GetNextRow();
        currentBorder = CompareBorders(this, info.cg, info.leftCol, info.rg, rowFrame, info.cell, 
                                       tableIsLTR, TABLE_EDGE, NS_SIDE_LEFT, !ADJACENT);
        BCCornerInfo& tlCorner = (0 == rowX) ? topCorners[0] : bottomCorners[0]; 
        tlCorner.Update(NS_SIDE_BOTTOM, currentBorder);
        tableCellMap->SetBCBorderCorner(eTopLeft, *info.cellMap, iter.mRowGroupStart, rowX, 
                                        0, tlCorner.ownerSide, tlCorner.subWidth, tlCorner.bevel);
        bottomCorners[0].Set(NS_SIDE_TOP, currentBorder); 
        
        startSeg = SetBorder(currentBorder, lastVerBorders[0]);
        
        tableCellMap->SetBCBorderEdge(NS_SIDE_LEFT, *info.cellMap, iter.mRowGroupStart, rowX, 
                                      info.colIndex, 1, currentBorder.owner, currentBorder.width, startSeg);
        
        if (info.cell) {
          info.cell->SetBorderWidth(firstSide, PR_MAX(currentBorder.width, info.cell->GetBorderWidth(firstSide)));
        }
        if (info.leftCol) {
          BCPixelSize half = BC_BORDER_RIGHT_HALF(currentBorder.width);
          info.leftCol->SetLeftBorderWidth(PR_MAX(half, info.leftCol->GetLeftBorderWidth()));
        }
        propData->mLeftBorderWidth = LimitBorderWidth(PR_MAX(propData->mLeftBorderWidth, currentBorder.width));
        
        if (rowFrame) {
          currentBorder = CompareBorders(this, info.cg, info.leftCol,
                                         info.rg, rowFrame, nsnull, tableIsLTR,
                                         TABLE_EDGE, NS_SIDE_LEFT, !ADJACENT);
          rowFrame->SetContinuousBCBorderWidth(firstSide, currentBorder.width);
        }
      }
      
      if (info.rgBottom && info.rg) { 
        currentBorder = CompareBorders(this, info.cg, info.leftCol, info.rg, nsnull,
                                       nsnull, tableIsLTR, TABLE_EDGE, NS_SIDE_LEFT,
                                       !ADJACENT);
        info.rg->SetContinuousBCBorderWidth(firstSide, currentBorder.width);
      }
    }

    
    if (numCols == cellEndColIndex + 1) { 
      if (!tableBorderReset[NS_SIDE_RIGHT]) {
        propData->mRightBorderWidth = 0;
        tableBorderReset[NS_SIDE_RIGHT] = PR_TRUE;
      }
      nsTableRowFrame* rowFrame = nsnull;
      for (PRInt32 rowX = info.rowIndex; rowX <= cellEndRowIndex; rowX++) {
        rowFrame = (rowX == info.rowIndex) ? info.topRow : rowFrame->GetNextRow();
        currentBorder = CompareBorders(this, info.cg, info.rightCol, info.rg, rowFrame, info.cell, 
                                       tableIsLTR, TABLE_EDGE, NS_SIDE_RIGHT, ADJACENT);
        
        BCCornerInfo& trCorner = (0 == rowX) ? topCorners[cellEndColIndex + 1] : bottomCorners[cellEndColIndex + 1]; 
        trCorner.Update(NS_SIDE_BOTTOM, currentBorder);   
        tableCellMap->SetBCBorderCorner(eTopRight, *info.cellMap, iter.mRowGroupStart, rowX, 
                                        cellEndColIndex, trCorner.ownerSide, trCorner.subWidth, trCorner.bevel);
        BCCornerInfo& brCorner = bottomCorners[cellEndColIndex + 1];
        brCorner.Set(NS_SIDE_TOP, currentBorder); 
        tableCellMap->SetBCBorderCorner(eBottomRight, *info.cellMap, iter.mRowGroupStart, rowX,
                                        cellEndColIndex, brCorner.ownerSide, brCorner.subWidth, brCorner.bevel);
        
        startSeg = SetBorder(currentBorder, lastVerBorders[cellEndColIndex + 1]);
        
        tableCellMap->SetBCBorderEdge(NS_SIDE_RIGHT, *info.cellMap, iter.mRowGroupStart, rowX,
                                      cellEndColIndex, 1, currentBorder.owner, currentBorder.width, startSeg);
        
        if (info.cell) {
          info.cell->SetBorderWidth(secondSide, PR_MAX(currentBorder.width, info.cell->GetBorderWidth(secondSide)));
        }
        if (info.rightCol) {
          BCPixelSize half = BC_BORDER_LEFT_HALF(currentBorder.width);
          info.rightCol->SetRightBorderWidth(PR_MAX(half, info.rightCol->GetRightBorderWidth()));
        }
        propData->mRightBorderWidth = LimitBorderWidth(PR_MAX(propData->mRightBorderWidth, currentBorder.width));
        
        if (rowFrame) {
          currentBorder = CompareBorders(this, info.cg, info.rightCol, info.rg,
                                         rowFrame, nsnull, tableIsLTR, TABLE_EDGE,
                                         NS_SIDE_RIGHT, ADJACENT);
          rowFrame->SetContinuousBCBorderWidth(secondSide, currentBorder.width);
        }
      }
      
      if (info.rgBottom && info.rg) { 
        currentBorder = CompareBorders(this, info.cg, info.rightCol, info.rg, 
                                       nsnull, nsnull, tableIsLTR, TABLE_EDGE,
                                       NS_SIDE_RIGHT, ADJACENT);
        info.rg->SetContinuousBCBorderWidth(secondSide, currentBorder.width);
      }
    }
    else {
      PRInt32 segLength = 0;
      BCMapCellInfo priorAjaInfo;
      for (PRInt32 rowX = info.rowIndex; rowX <= cellEndRowIndex; rowX += segLength) {
        iter.PeekRight(info, rowX, ajaInfo);
        const nsIFrame* cg = (info.cgRight) ? info.cg : nsnull;
        currentBorder = CompareBorders(nsnull, cg, info.rightCol, nsnull, nsnull, info.cell,
                                       tableIsLTR, !TABLE_EDGE, NS_SIDE_RIGHT, ADJACENT);
        cg = (ajaInfo.cgLeft) ? ajaInfo.cg : nsnull;
        adjacentBorder = CompareBorders(nsnull, cg, ajaInfo.leftCol, nsnull, nsnull, ajaInfo.cell, 
                                        tableIsLTR, !TABLE_EDGE, NS_SIDE_LEFT, !ADJACENT);
        currentBorder = CompareBorders(!CELL_CORNER, currentBorder, adjacentBorder, !HORIZONTAL);
                          
        segLength = PR_MAX(1, ajaInfo.rowIndex + ajaInfo.rowSpan - rowX);
        segLength = PR_MIN(segLength, info.rowIndex + info.rowSpan - rowX);

        
        startSeg = SetBorder(currentBorder, lastVerBorders[cellEndColIndex + 1]);
        
        if (RIGHT_DAMAGED(cellEndColIndex) && TOP_DAMAGED(rowX) && BOTTOM_DAMAGED(rowX)) {
          tableCellMap->SetBCBorderEdge(NS_SIDE_RIGHT, *info.cellMap, iter.mRowGroupStart, rowX, 
                                        cellEndColIndex, segLength, currentBorder.owner, currentBorder.width, startSeg);
          
          if (info.cell) {
            info.cell->SetBorderWidth(secondSide, PR_MAX(currentBorder.width, info.cell->GetBorderWidth(secondSide)));
          }
          if (info.rightCol) {
            BCPixelSize half = BC_BORDER_LEFT_HALF(currentBorder.width);
            info.rightCol->SetRightBorderWidth(PR_MAX(half, info.rightCol->GetRightBorderWidth()));
          }
          if (ajaInfo.cell) {
            ajaInfo.cell->SetBorderWidth(firstSide, PR_MAX(currentBorder.width, ajaInfo.cell->GetBorderWidth(firstSide)));
          }
          if (ajaInfo.leftCol) {
            BCPixelSize half = BC_BORDER_RIGHT_HALF(currentBorder.width);
            ajaInfo.leftCol->SetLeftBorderWidth(PR_MAX(half, ajaInfo.leftCol->GetLeftBorderWidth()));
          }
        }
        
        PRBool hitsSpanOnRight = (rowX > ajaInfo.rowIndex) && (rowX < ajaInfo.rowIndex + ajaInfo.rowSpan);
        BCCornerInfo* trCorner = ((0 == rowX) || hitsSpanOnRight) 
                                 ? &topCorners[cellEndColIndex + 1] : &bottomCorners[cellEndColIndex + 1]; 
        trCorner->Update(NS_SIDE_BOTTOM, currentBorder);
        
        if (rowX != info.rowIndex) {
          const nsIFrame* rg = (priorAjaInfo.rgBottom) ? priorAjaInfo.rg : nsnull;
          currentBorder = CompareBorders(nsnull, nsnull, nsnull, rg, priorAjaInfo.bottomRow, priorAjaInfo.cell,
                                         tableIsLTR, !TABLE_EDGE, NS_SIDE_BOTTOM, ADJACENT);
          rg = (ajaInfo.rgTop) ? ajaInfo.rg : nsnull;
          adjacentBorder = CompareBorders(nsnull, nsnull, nsnull, rg, ajaInfo.topRow, ajaInfo.cell,
                                          tableIsLTR, !TABLE_EDGE, NS_SIDE_TOP, !ADJACENT);
          currentBorder = CompareBorders(!CELL_CORNER, currentBorder, adjacentBorder, HORIZONTAL);
          trCorner->Update(NS_SIDE_RIGHT, currentBorder);
        }
        
        if (RIGHT_DAMAGED(cellEndColIndex) && TOP_DAMAGED(rowX)) {
          if (0 != rowX) {
            tableCellMap->SetBCBorderCorner(eTopRight, *info.cellMap, iter.mRowGroupStart, rowX, cellEndColIndex, 
                                            trCorner->ownerSide, trCorner->subWidth, trCorner->bevel);
          }
          
          for (PRInt32 rX = rowX + 1; rX < rowX + segLength; rX++) {
            tableCellMap->SetBCBorderCorner(eBottomRight, *info.cellMap, iter.mRowGroupStart, rX, 
                                            cellEndColIndex, trCorner->ownerSide, trCorner->subWidth, PR_FALSE);
          }
        }
        
        hitsSpanOnRight = (rowX + segLength < ajaInfo.rowIndex + ajaInfo.rowSpan);
        BCCornerInfo& brCorner = (hitsSpanOnRight) ? topCorners[cellEndColIndex + 1] 
                                                   : bottomCorners[cellEndColIndex + 1];
        brCorner.Set(NS_SIDE_TOP, currentBorder);
        priorAjaInfo = ajaInfo;
      }
    }
    for (PRInt32 colX = info.colIndex + 1; colX <= cellEndColIndex; colX++) {
      lastVerBorders[colX].Reset(0,1);
    }

    
    if (numRows == cellEndRowIndex + 1) { 
      if (!tableBorderReset[NS_SIDE_BOTTOM]) {
        propData->mBottomBorderWidth = 0;
        tableBorderReset[NS_SIDE_BOTTOM] = PR_TRUE;
      }
      for (PRInt32 colX = info.colIndex; colX <= cellEndColIndex; colX++) {
        nsIFrame* colFrame = GetColFrame(colX); if (!colFrame) ABORT0();
        nsIFrame* cgFrame = colFrame->GetParent(); if (!cgFrame) ABORT0();
        currentBorder = CompareBorders(this, cgFrame, colFrame, info.rg, info.bottomRow, info.cell,
                                       tableIsLTR, TABLE_EDGE, NS_SIDE_BOTTOM, ADJACENT);
        
        BCCornerInfo& blCorner = bottomCorners[colX]; 
        blCorner.Update(NS_SIDE_RIGHT, currentBorder);
        tableCellMap->SetBCBorderCorner(eBottomLeft, *info.cellMap, iter.mRowGroupStart, cellEndRowIndex,                
                                        colX, blCorner.ownerSide, blCorner.subWidth, blCorner.bevel); 
        BCCornerInfo& brCorner = bottomCorners[colX + 1]; 
        brCorner.Update(NS_SIDE_LEFT, currentBorder);
        if (numCols == colX + 1) { 
          tableCellMap->SetBCBorderCorner(eBottomRight, *info.cellMap, iter.mRowGroupStart, cellEndRowIndex,               
                                          colX, brCorner.ownerSide, brCorner.subWidth, brCorner.bevel, PR_TRUE);  
        }
        
        startSeg = SetHorBorder(currentBorder, blCorner, lastBottomBorder);
        if (!startSeg) { 
           
           
           
           startSeg = (lastBottomBorder.rowIndex != cellEndRowIndex + 1);
        }
        
        tableCellMap->SetBCBorderEdge(NS_SIDE_BOTTOM, *info.cellMap, iter.mRowGroupStart, cellEndRowIndex, 
                                      colX, 1, currentBorder.owner, currentBorder.width, startSeg);
        
        if (info.cell) {
          info.cell->SetBorderWidth(NS_SIDE_BOTTOM, PR_MAX(currentBorder.width, info.cell->GetBorderWidth(NS_SIDE_BOTTOM)));
        }
        if (info.bottomRow) {
          BCPixelSize half = BC_BORDER_TOP_HALF(currentBorder.width);
          info.bottomRow->SetBottomBCBorderWidth(PR_MAX(half, info.bottomRow->GetBottomBCBorderWidth()));
        }
        propData->mBottomBorderWidth = LimitBorderWidth(PR_MAX(propData->mBottomBorderWidth, currentBorder.width));
        
        lastBottomBorder.rowIndex = cellEndRowIndex + 1;
        lastBottomBorder.rowSpan = info.rowSpan;
        lastBottomBorders[colX] = lastBottomBorder;
        
        currentBorder = CompareBorders(this, cgFrame, colFrame, info.rg, info.bottomRow,
                                       nsnull, tableIsLTR, TABLE_EDGE, NS_SIDE_BOTTOM,
                                       ADJACENT);
        ((nsTableColFrame*)colFrame)->SetContinuousBCBorderWidth(NS_SIDE_BOTTOM,
                                                                currentBorder.width);
      }
      
      if (info.rg) {
        currentBorder = CompareBorders(this, nsnull, nsnull, info.rg, info.bottomRow,
                                       nsnull, tableIsLTR, TABLE_EDGE, NS_SIDE_BOTTOM,
                                       ADJACENT);
        info.rg->SetContinuousBCBorderWidth(NS_SIDE_BOTTOM, currentBorder.width);
      }
      if (info.cg) {
        currentBorder = CompareBorders(this, info.cg, nsnull, info.rg, info.bottomRow,
                                       nsnull, tableIsLTR, TABLE_EDGE, NS_SIDE_BOTTOM,
                                       ADJACENT);
        info.cg->SetContinuousBCBorderWidth(NS_SIDE_BOTTOM, currentBorder.width);
      }
    }
    else {
      PRInt32 segLength = 0;
      for (PRInt32 colX = info.colIndex; colX <= cellEndColIndex; colX += segLength) {
        iter.PeekBottom(info, colX, ajaInfo);
        const nsIFrame* rg = (info.rgBottom) ? info.rg : nsnull;
        currentBorder = CompareBorders(nsnull, nsnull, nsnull, rg, info.bottomRow, info.cell, 
                                       tableIsLTR, !TABLE_EDGE, NS_SIDE_BOTTOM, ADJACENT);
        rg = (ajaInfo.rgTop) ? ajaInfo.rg : nsnull;
        adjacentBorder = CompareBorders(nsnull, nsnull, nsnull, rg, ajaInfo.topRow, ajaInfo.cell, 
                                        tableIsLTR, !TABLE_EDGE, NS_SIDE_TOP, !ADJACENT);
        currentBorder = CompareBorders(!CELL_CORNER, currentBorder, adjacentBorder, HORIZONTAL);
        segLength = PR_MAX(1, ajaInfo.colIndex + ajaInfo.colSpan - colX);
        segLength = PR_MIN(segLength, info.colIndex + info.colSpan - colX);

        
        BCCornerInfo& blCorner = bottomCorners[colX]; 
        PRBool hitsSpanBelow = (colX > ajaInfo.colIndex) && (colX < ajaInfo.colIndex + ajaInfo.colSpan);
        PRBool update = PR_TRUE;
        if ((colX == info.colIndex) && (colX > damageArea.x)) {
          PRInt32 prevRowIndex = lastBottomBorders[colX - 1].rowIndex;
          if (prevRowIndex > cellEndRowIndex + 1) { 
            update = PR_FALSE; 
          }
          else if (prevRowIndex < cellEndRowIndex + 1) { 
            topCorners[colX] = blCorner;
            blCorner.Set(NS_SIDE_RIGHT, currentBorder);
            update = PR_FALSE;
          }
        }
        if (update) {
          blCorner.Update(NS_SIDE_RIGHT, currentBorder);
        }
        if (BOTTOM_DAMAGED(cellEndRowIndex) && LEFT_DAMAGED(colX)) {
          if (hitsSpanBelow) {
            tableCellMap->SetBCBorderCorner(eBottomLeft, *info.cellMap, iter.mRowGroupStart, cellEndRowIndex, colX,
                                            blCorner.ownerSide, blCorner.subWidth, blCorner.bevel);
          }
          
          for (PRInt32 cX = colX + 1; cX < colX + segLength; cX++) {
            BCCornerInfo& corner = bottomCorners[cX];
            corner.Set(NS_SIDE_RIGHT, currentBorder);
            tableCellMap->SetBCBorderCorner(eBottomLeft, *info.cellMap, iter.mRowGroupStart, cellEndRowIndex,
                                            cX, corner.ownerSide, corner.subWidth, PR_FALSE);
          }
        }
        
        startSeg = SetHorBorder(currentBorder, blCorner, lastBottomBorder);
        if (!startSeg) { 
           
           
           
           startSeg = (lastBottomBorder.rowIndex != cellEndRowIndex + 1);
        }
        lastBottomBorder.rowIndex = cellEndRowIndex + 1;
        lastBottomBorder.rowSpan = info.rowSpan;
        for (PRInt32 cX = colX; cX < colX + segLength; cX++) {
          lastBottomBorders[cX] = lastBottomBorder;
        }

        
        if (BOTTOM_DAMAGED(cellEndRowIndex) && LEFT_DAMAGED(colX) && RIGHT_DAMAGED(colX)) {
          tableCellMap->SetBCBorderEdge(NS_SIDE_BOTTOM, *info.cellMap, iter.mRowGroupStart, cellEndRowIndex,
                                        colX, segLength, currentBorder.owner, currentBorder.width, startSeg);
          
          if (info.cell) {
            info.cell->SetBorderWidth(NS_SIDE_BOTTOM, PR_MAX(currentBorder.width, info.cell->GetBorderWidth(NS_SIDE_BOTTOM)));
          }
          if (info.bottomRow) {
            BCPixelSize half = BC_BORDER_TOP_HALF(currentBorder.width);
            info.bottomRow->SetBottomBCBorderWidth(PR_MAX(half, info.bottomRow->GetBottomBCBorderWidth()));
          }
          if (ajaInfo.cell) {
            ajaInfo.cell->SetBorderWidth(NS_SIDE_TOP, PR_MAX(currentBorder.width, ajaInfo.cell->GetBorderWidth(NS_SIDE_TOP)));
          }
          if (ajaInfo.topRow) {
            BCPixelSize half = BC_BORDER_BOTTOM_HALF(currentBorder.width);
            ajaInfo.topRow->SetTopBCBorderWidth(PR_MAX(half, ajaInfo.topRow->GetTopBCBorderWidth()));
          }
        }
        
        BCCornerInfo& brCorner = bottomCorners[colX + segLength];
        brCorner.Update(NS_SIDE_LEFT, currentBorder);
      }
      if (!gotRowBorder && 1 == info.rowSpan && (ajaInfo.topRow || info.rgBottom)) {
        
        
        
        
        const nsIFrame* rg = (info.rgBottom) ? info.rg : nsnull;
        currentBorder = CompareBorders(nsnull, nsnull, nsnull, rg, info.bottomRow,
                                       nsnull, tableIsLTR, !TABLE_EDGE, NS_SIDE_BOTTOM,
                                       ADJACENT);
        rg = (ajaInfo.rgTop) ? ajaInfo.rg : nsnull;
        adjacentBorder = CompareBorders(nsnull, nsnull, nsnull, rg, ajaInfo.topRow,
                                        nsnull, tableIsLTR, !TABLE_EDGE, NS_SIDE_TOP,
                                        !ADJACENT);
        currentBorder = CompareBorders(PR_FALSE, currentBorder, adjacentBorder, HORIZONTAL);
        if (ajaInfo.topRow) {
          ajaInfo.topRow->SetContinuousBCBorderWidth(NS_SIDE_TOP, currentBorder.width);
        }
        if (info.rgBottom && info.rg) {
          info.rg->SetContinuousBCBorderWidth(NS_SIDE_BOTTOM, currentBorder.width);
        }
        gotRowBorder = PR_TRUE;
      }
    }

    
    if ((numCols != cellEndColIndex + 1) &&                  
        (lastBottomBorders[cellEndColIndex + 1].rowSpan > 1)) { 
      BCCornerInfo& corner = bottomCorners[cellEndColIndex + 1];
      if ((NS_SIDE_TOP != corner.ownerSide) && (NS_SIDE_BOTTOM != corner.ownerSide)) { 
        BCCellBorder& thisBorder = lastBottomBorder;
        BCCellBorder& nextBorder = lastBottomBorders[info.colIndex + 1];
        if ((thisBorder.color == nextBorder.color) && (thisBorder.width == nextBorder.width) &&
            (thisBorder.style == nextBorder.style)) {
          
          if (iter.mCellMap) {
            BCData* bcData = tableCellMap->GetBCData(NS_SIDE_BOTTOM, *iter.mCellMap, cellEndRowIndex, 
                                                     cellEndColIndex + 1);
            if (bcData) {
              bcData->SetTopStart(PR_FALSE);
            }
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
    bcData = (BCData*)tableCellMap->mBCInfo->mRightBorders.ElementAt(aY);
  }
  else if (IsBottomMost()) {
    cellData = nsnull;
    bcData = (BCData*)tableCellMap->mBCInfo->mBottomBorders.ElementAt(aX);
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

  if (rowGroupIndex < rowGroups.Length()) {
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

  const nsStyleBackground* bgColor = nsCSSRendering::FindNonTransparentBackground(mStyleContext);
  
  PRUint32 startRowIndex, endRowIndex, startColIndex, endColIndex;
  startRowIndex = endRowIndex = startColIndex = endColIndex = 0;

  RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);
  PRBool done = PR_FALSE;
  PRBool haveIntersect = PR_FALSE;
  nsTableRowGroupFrame* inFlowRG  = nsnull;
  nsTableRowFrame*      inFlowRow = nsnull;
  
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
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

#ifdef DEBUG

static PRBool 
GetFrameTypeName(nsIAtom* aFrameType,
                 char*    aName)
{
  PRBool isTable = PR_FALSE;
  if (nsGkAtoms::tableOuterFrame == aFrameType) 
    strcpy(aName, "Tbl");
  else if (nsGkAtoms::tableFrame == aFrameType) {
    strcpy(aName, "Tbl");
    isTable = PR_TRUE;
  }
  else if (nsGkAtoms::tableRowGroupFrame == aFrameType) 
    strcpy(aName, "RowG");
  else if (nsGkAtoms::tableRowFrame == aFrameType) 
    strcpy(aName, "Row");
  else if (IS_TABLE_CELL(aFrameType)) 
    strcpy(aName, "Cell");
  else if (nsGkAtoms::blockFrame == aFrameType) 
    strcpy(aName, "Block");
  else 
    NS_ASSERTION(PR_FALSE, "invalid call to GetFrameTypeName");

  return isTable;
}
#endif

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

#ifdef DEBUG
#define MAX_SIZE  128
#define MIN_INDENT 30

static 
void DumpTableFramesRecur(nsIFrame*       aFrame,
                          PRUint32        aIndent)
{
  char indent[MAX_SIZE + 1];
  aIndent = PR_MIN(aIndent, MAX_SIZE - MIN_INDENT);
  memset (indent, ' ', aIndent + MIN_INDENT);
  indent[aIndent + MIN_INDENT] = 0;

  char fName[MAX_SIZE];
  nsIAtom* fType = aFrame->GetType();
  GetFrameTypeName(fType, fName);

  printf("%s%s %p", indent, fName, aFrame);
  nsIFrame* flowFrame = aFrame->GetPrevInFlow();
  if (flowFrame) {
    printf(" pif=%p", flowFrame);
  }
  flowFrame = aFrame->GetNextInFlow();
  if (flowFrame) {
    printf(" nif=%p", flowFrame);
  }
  printf("\n");

  if (nsGkAtoms::tableFrame         == fType ||
      nsGkAtoms::tableRowGroupFrame == fType ||
      nsGkAtoms::tableRowFrame      == fType ||
      IS_TABLE_CELL(fType)) {
    nsIFrame* child = aFrame->GetFirstChild(nsnull);
    while(child) {
      DumpTableFramesRecur(child, aIndent+1);
      child = child->GetNextSibling();
    }
  }
}
  
void
nsTableFrame::DumpTableFrames(nsIFrame* aFrame)
{
  nsTableFrame* tableFrame = nsnull;

  if (nsGkAtoms::tableFrame == aFrame->GetType()) { 
    tableFrame = static_cast<nsTableFrame*>(aFrame);
  }
  else {
    tableFrame = nsTableFrame::GetTableFrame(aFrame);
  }
  tableFrame = static_cast<nsTableFrame*>(tableFrame->GetFirstInFlow());
  while (tableFrame) {
    DumpTableFramesRecur(tableFrame, 0);
    tableFrame = static_cast<nsTableFrame*>(tableFrame->GetNextInFlow());
  }
}
#endif
