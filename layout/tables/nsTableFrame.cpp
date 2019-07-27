





#include "mozilla/Likely.h"
#include "mozilla/MathAlgorithms.h"

#include "nsCOMPtr.h"
#include "nsTableFrame.h"
#include "nsRenderingContext.h"
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
#include "nsContentUtils.h"
#include "nsCSSRendering.h"
#include "nsGkAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsIPresShell.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLElement.h"
#include "nsIScriptError.h"
#include "nsFrameManager.h"
#include "nsError.h"
#include "nsAutoPtr.h"
#include "nsCSSFrameConstructor.h"
#include "nsStyleSet.h"
#include "nsDisplayList.h"
#include "nsIScrollableFrame.h"
#include "nsCSSProps.h"
#include "RestyleTracker.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::image;
using namespace mozilla::layout;





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
    nsTableFrame* table = static_cast<nsTableFrame*>(aTableFrame.FirstInFlow());
    nsMargin borderPadding = table->GetChildAreaOffset(&reflowState);

    x = borderPadding.left + table->GetColSpacing(-1);
    y = borderPadding.top; 

    availSize.width  = aAvailWidth;
    if (NS_UNCONSTRAINEDSIZE != availSize.width) {
      int32_t colCount = table->GetColCount();
      availSize.width -= borderPadding.left + borderPadding.right
                         + table->GetColSpacing(-1)
                         + table->GetColSpacing(colCount);
      availSize.width = std::max(0, availSize.width);
    }

    availSize.height = aAvailHeight;
    if (NS_UNCONSTRAINEDSIZE != availSize.height) {
      availSize.height -= borderPadding.top + borderPadding.bottom
                          + table->GetRowSpacing(-1)
                          + table->GetRowSpacing(table->GetRowCount());
      availSize.height = std::max(0, availSize.height);
    }
  }

  nsTableReflowState(nsPresContext&           aPresContext,
                     const nsHTMLReflowState& aReflowState,
                     nsTableFrame&            aTableFrame)
    : reflowState(aReflowState)
  {
    Init(aPresContext, aTableFrame, aReflowState.AvailableWidth(), aReflowState.AvailableHeight());
  }

};





struct BCPropertyData
{
  BCPropertyData() : mTopBorderWidth(0), mRightBorderWidth(0),
                     mBottomBorderWidth(0), mLeftBorderWidth(0),
                     mLeftCellBorderWidth(0), mRightCellBorderWidth(0) {}
  nsIntRect mDamageArea;
  BCPixelSize mTopBorderWidth;
  BCPixelSize mRightBorderWidth;
  BCPixelSize mBottomBorderWidth;
  BCPixelSize mLeftBorderWidth;
  BCPixelSize mLeftCellBorderWidth;
  BCPixelSize mRightCellBorderWidth;
};

nsStyleContext*
nsTableFrame::GetParentStyleContext(nsIFrame** aProviderFrame) const
{
  
  

  NS_PRECONDITION(GetParent(), "table constructed without outer table");
  if (!mContent->GetParent() && !StyleContext()->GetPseudo()) {
    
    *aProviderFrame = nullptr;
    return nullptr;
  }

  return GetParent()->DoGetParentStyleContext(aProviderFrame);
}


nsIAtom*
nsTableFrame::GetType() const
{
  return nsGkAtoms::tableFrame;
}


nsTableFrame::nsTableFrame(nsStyleContext* aContext)
  : nsContainerFrame(aContext),
    mCellMap(nullptr),
    mTableLayoutStrategy(nullptr)
{
  memset(&mBits, 0, sizeof(mBits));
}

void
nsTableFrame::Init(nsIContent*       aContent,
                   nsContainerFrame* aParent,
                   nsIFrame*         aPrevInFlow)
{
  NS_PRECONDITION(!mCellMap, "Init called twice");
  NS_PRECONDITION(!aPrevInFlow ||
                  aPrevInFlow->GetType() == nsGkAtoms::tableFrame,
                  "prev-in-flow must be of same type");

  
  nsContainerFrame::Init(aContent, aParent, aPrevInFlow);

  
  const nsStyleTableBorder* tableStyle = StyleTableBorder();
  bool borderCollapse = (NS_STYLE_BORDER_COLLAPSE == tableStyle->mBorderCollapse);
  SetBorderCollapse(borderCollapse);

  
  if (!aPrevInFlow) {
    mCellMap = new nsTableCellMap(*this, borderCollapse);
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
  }
}

nsTableFrame::~nsTableFrame()
{
  delete mCellMap;
  delete mTableLayoutStrategy;
}

void
nsTableFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  mColGroups.DestroyFramesFrom(aDestructRoot);
  nsContainerFrame::DestroyFrom(aDestructRoot);
}


void
nsTableFrame::RePositionViews(nsIFrame* aFrame)
{
  nsContainerFrame::PositionFrameView(aFrame);
  nsContainerFrame::PositionChildViews(aFrame);
}

static bool
IsRepeatedFrame(nsIFrame* kidFrame)
{
  return (kidFrame->GetType() == nsGkAtoms::tableRowFrame ||
          kidFrame->GetType() == nsGkAtoms::tableRowGroupFrame) &&
         (kidFrame->GetStateBits() & NS_REPEATED_ROW_OR_ROWGROUP);
}

bool
nsTableFrame::PageBreakAfter(nsIFrame* aSourceFrame,
                             nsIFrame* aNextFrame)
{
  const nsStyleDisplay* display = aSourceFrame->StyleDisplay();
  nsTableRowGroupFrame* prevRg = do_QueryFrame(aSourceFrame);
  
  if ((display->mBreakAfter || (prevRg && prevRg->HasInternalBreakAfter())) &&
      !IsRepeatedFrame(aSourceFrame)) {
    return !(aNextFrame && IsRepeatedFrame(aNextFrame)); 
  }

  if (aNextFrame) {
    display = aNextFrame->StyleDisplay();
    
     nsTableRowGroupFrame* nextRg = do_QueryFrame(aNextFrame);
    if ((display->mBreakBefore ||
        (nextRg && nextRg->HasInternalBreakBefore())) &&
        !IsRepeatedFrame(aNextFrame)) {
      return !IsRepeatedFrame(aSourceFrame); 
    }
  }
  return false;
}

typedef nsTArray<nsIFrame*> FrameTArray;

 void
nsTableFrame::RegisterPositionedTablePart(nsIFrame* aFrame)
{
  
  
  
  
  if (!IS_TABLE_CELL(aFrame->GetType())) {
    nsIContent* content = aFrame->GetContent();
    nsPresContext* presContext = aFrame->PresContext();
    if (content && !presContext->HasWarnedAboutPositionedTableParts()) {
      presContext->SetHasWarnedAboutPositionedTableParts();
      nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                      NS_LITERAL_CSTRING("Layout: Tables"),
                                      content->OwnerDoc(),
                                      nsContentUtils::eLAYOUT_PROPERTIES,
                                      "TablePartRelPosWarning");
    }
  }

  nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(aFrame);
  MOZ_ASSERT(tableFrame, "Should have a table frame here");
  tableFrame = static_cast<nsTableFrame*>(tableFrame->FirstContinuation());

  
  FrameProperties props = tableFrame->Properties();
  auto positionedParts =
    static_cast<FrameTArray*>(props.Get(PositionedTablePartArray()));

  
  if (!positionedParts) {
    positionedParts = new FrameTArray;
    props.Set(PositionedTablePartArray(), positionedParts);
  }

  
  positionedParts->AppendElement(aFrame);
}

 void
nsTableFrame::UnregisterPositionedTablePart(nsIFrame* aFrame,
                                            nsIFrame* aDestructRoot)
{
  
  
  
  nsTableFrame* tableFrame = GetTableFramePassingThrough(aDestructRoot, aFrame);
  if (!tableFrame) {
    return;
  }
  tableFrame = static_cast<nsTableFrame*>(tableFrame->FirstContinuation());

  
  FrameProperties props = tableFrame->Properties();
  auto positionedParts =
    static_cast<FrameTArray*>(props.Get(PositionedTablePartArray()));

  
  MOZ_ASSERT(positionedParts &&
             positionedParts->IndexOf(aFrame) != FrameTArray::NoIndex,
             "Asked to unregister a positioned table part that wasn't registered");
  if (positionedParts) {
    positionedParts->RemoveElement(aFrame);
  }
}



void
nsTableFrame::SetInitialChildList(ChildListID     aListID,
                                  nsFrameList&    aChildList)
{
  MOZ_ASSERT(mFrames.IsEmpty() && mColGroups.IsEmpty(),
             "unexpected second call to SetInitialChildList");
  MOZ_ASSERT(aListID == kPrincipalList, "unexpected child list");

  
  
  
  while (aChildList.NotEmpty()) {
    nsIFrame* childFrame = aChildList.FirstChild();
    aChildList.RemoveFirstChild();
    const nsStyleDisplay* childDisplay = childFrame->StyleDisplay();

    if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == childDisplay->mDisplay) {
      NS_ASSERTION(nsGkAtoms::tableColGroupFrame == childFrame->GetType(),
                   "This is not a colgroup");
      mColGroups.AppendFrame(nullptr, childFrame);
    }
    else { 
      mFrames.AppendFrame(nullptr, childFrame);
    }
  }

  
  
  if (!GetPrevInFlow()) {
    
    
    InsertColGroups(0, mColGroups);
    InsertRowGroups(mFrames);
    
    if (IsBorderCollapse()) {
      SetFullBCDamageArea();
    }
  }
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
        
        int32_t rowIndex, colIndex;
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





int32_t nsTableFrame::GetEffectiveColCount() const
{
  int32_t colCount = GetColCount();
  if (LayoutStrategy()->GetType() == nsITableLayoutStrategy::Auto) {
    nsTableCellMap* cellMap = GetCellMap();
    if (!cellMap) {
      return 0;
    }
    
    for (int32_t colX = colCount - 1; colX >= 0; colX--) {
      if (cellMap->GetNumCellsOriginatingInCol(colX) > 0) {
        break;
      }
      colCount--;
    }
  }
  return colCount;
}

int32_t nsTableFrame::GetIndexOfLastRealCol()
{
  int32_t numCols = mColFrames.Length();
  if (numCols > 0) {
    for (int32_t colX = numCols - 1; colX >= 0; colX--) {
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
nsTableFrame::GetColFrame(int32_t aColIndex) const
{
  NS_ASSERTION(!GetPrevInFlow(), "GetColFrame called on next in flow");
  int32_t numCols = mColFrames.Length();
  if ((aColIndex >= 0) && (aColIndex < numCols)) {
    return mColFrames.ElementAt(aColIndex);
  }
  else {
    NS_ERROR("invalid col index");
    return nullptr;
  }
}

int32_t nsTableFrame::GetEffectiveRowSpan(int32_t                 aRowIndex,
                                          const nsTableCellFrame& aCell) const
{
  nsTableCellMap* cellMap = GetCellMap();
  NS_PRECONDITION (nullptr != cellMap, "bad call, cellMap not yet allocated.");

  int32_t colIndex;
  aCell.GetColIndex(colIndex);
  return cellMap->GetEffectiveRowSpan(aRowIndex, colIndex);
}

int32_t nsTableFrame::GetEffectiveRowSpan(const nsTableCellFrame& aCell,
                                          nsCellMap*              aCellMap)
{
  nsTableCellMap* tableCellMap = GetCellMap(); if (!tableCellMap) ABORT1(1);

  int32_t colIndex, rowIndex;
  aCell.GetColIndex(colIndex);
  aCell.GetRowIndex(rowIndex);

  if (aCellMap)
    return aCellMap->GetRowSpan(rowIndex, colIndex, true);
  else
    return tableCellMap->GetEffectiveRowSpan(rowIndex, colIndex);
}

int32_t nsTableFrame::GetEffectiveColSpan(const nsTableCellFrame& aCell,
                                          nsCellMap*              aCellMap) const
{
  nsTableCellMap* tableCellMap = GetCellMap(); if (!tableCellMap) ABORT1(1);

  int32_t colIndex, rowIndex;
  aCell.GetColIndex(colIndex);
  aCell.GetRowIndex(rowIndex);
  bool ignore;

  if (aCellMap)
    return aCellMap->GetEffectiveColSpan(*tableCellMap, rowIndex, colIndex, ignore);
  else
    return tableCellMap->GetEffectiveColSpan(rowIndex, colIndex);
}

bool nsTableFrame::HasMoreThanOneCell(int32_t aRowIndex) const
{
  nsTableCellMap* tableCellMap = GetCellMap(); if (!tableCellMap) ABORT1(1);
  return tableCellMap->HasMoreThanOneCell(aRowIndex);
}

void nsTableFrame::AdjustRowIndices(int32_t         aRowIndex,
                                    int32_t         aAdjustment)
{
  
  
  RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);

  for (uint32_t rgX = 0; rgX < rowGroups.Length(); rgX++) {
    rowGroups[rgX]->AdjustRowIndices(aRowIndex, aAdjustment);
  }
}


void nsTableFrame::ResetRowIndices(const nsFrameList::Slice& aRowGroupsToExclude)
{
  
  
  RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);

  int32_t rowIndex = 0;
  nsTHashtable<nsPtrHashKey<nsTableRowGroupFrame> > excludeRowGroups;
  nsFrameList::Enumerator excludeRowGroupsEnumerator(aRowGroupsToExclude);
  while (!excludeRowGroupsEnumerator.AtEnd()) {
    excludeRowGroups.PutEntry(static_cast<nsTableRowGroupFrame*>(excludeRowGroupsEnumerator.get()));
    excludeRowGroupsEnumerator.Next();
  }

  for (uint32_t rgX = 0; rgX < rowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
    if (!excludeRowGroups.GetEntry(rgFrame)) {
      const nsFrameList& rowFrames = rgFrame->PrincipalChildList();
      for (nsFrameList::Enumerator rows(rowFrames); !rows.AtEnd(); rows.Next()) {
        if (NS_STYLE_DISPLAY_TABLE_ROW==rows.get()->StyleDisplay()->mDisplay) {
          ((nsTableRowFrame *)rows.get())->SetRowIndex(rowIndex);
          rowIndex++;
        }
      }
    }
  }
}
void nsTableFrame::InsertColGroups(int32_t                   aStartColIndex,
                                   const nsFrameList::Slice& aColGroups)
{
  int32_t colIndex = aStartColIndex;
  nsFrameList::Enumerator colGroups(aColGroups);
  for (; !colGroups.AtEnd(); colGroups.Next()) {
    MOZ_ASSERT(colGroups.get()->GetType() == nsGkAtoms::tableColGroupFrame);
    nsTableColGroupFrame* cgFrame =
      static_cast<nsTableColGroupFrame*>(colGroups.get());
    cgFrame->SetStartColumnIndex(colIndex);
    
    
    
    

    
    
    
    
    cgFrame->AddColsToTable(colIndex, false,
                              colGroups.get()->PrincipalChildList());
    int32_t numCols = cgFrame->GetColCount();
    colIndex += numCols;
  }

  nsFrameList::Enumerator remainingColgroups = colGroups.GetUnlimitedEnumerator();
  if (!remainingColgroups.AtEnd()) {
    nsTableColGroupFrame::ResetColIndices(
      static_cast<nsTableColGroupFrame*>(remainingColgroups.get()), colIndex);
  }
}

void nsTableFrame::InsertCol(nsTableColFrame& aColFrame,
                             int32_t          aColIndex)
{
  mColFrames.InsertElementAt(aColIndex, &aColFrame);
  nsTableColType insertedColType = aColFrame.GetColType();
  int32_t numCacheCols = mColFrames.Length();
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    int32_t numMapCols = cellMap->GetColCount();
    if (numCacheCols > numMapCols) {
      bool removedFromCache = false;
      if (eColAnonymousCell != insertedColType) {
        nsTableColFrame* lastCol = mColFrames.ElementAt(numCacheCols - 1);
        if (lastCol) {
          nsTableColType lastColType = lastCol->GetColType();
          if (eColAnonymousCell == lastColType) {
            
            mColFrames.RemoveElementAt(numCacheCols - 1);
            
            nsTableColGroupFrame* lastColGroup = (nsTableColGroupFrame *)mColGroups.LastChild();
            if (lastColGroup) {
              lastColGroup->RemoveChild(*lastCol, false);

              
              if (lastColGroup->GetColCount() <= 0) {
                mColGroups.DestroyFrame((nsIFrame*)lastColGroup);
              }
            }
            removedFromCache = true;
          }
        }
      }
      if (!removedFromCache) {
        cellMap->AddColsAtEnd(1);
      }
    }
  }
  
  if (IsBorderCollapse()) {
    nsIntRect damageArea(aColIndex, 0, 1, GetRowCount());
    AddBCDamageArea(damageArea);
  }
}

void nsTableFrame::RemoveCol(nsTableColGroupFrame* aColGroupFrame,
                             int32_t               aColIndex,
                             bool                  aRemoveFromCache,
                             bool                  aRemoveFromCellMap)
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
    nsIntRect damageArea(0, 0, GetColCount(), GetRowCount());
    AddBCDamageArea(damageArea);
  }
}




nsTableCellMap*
nsTableFrame::GetCellMap() const
{
  return static_cast<nsTableFrame*>(FirstInFlow())->mCellMap;
}


nsTableColGroupFrame*
nsTableFrame::CreateAnonymousColGroupFrame(nsTableColGroupType aColGroupType)
{
  nsIContent* colGroupContent = GetContent();
  nsPresContext* presContext = PresContext();
  nsIPresShell *shell = presContext->PresShell();

  nsRefPtr<nsStyleContext> colGroupStyle;
  colGroupStyle = shell->StyleSet()->
    ResolveAnonymousBoxStyle(nsCSSAnonBoxes::tableColGroup, mStyleContext);
  
  nsIFrame* newFrame = NS_NewTableColGroupFrame(shell, colGroupStyle);
  ((nsTableColGroupFrame *)newFrame)->SetColType(aColGroupType);
  newFrame->Init(colGroupContent, this, nullptr);
  return (nsTableColGroupFrame *)newFrame;
}

void
nsTableFrame::AppendAnonymousColFrames(int32_t aNumColsToAdd)
{
  
  nsTableColGroupFrame* colGroupFrame =
    static_cast<nsTableColGroupFrame*>(mColGroups.LastChild());

  if (!colGroupFrame ||
      (colGroupFrame->GetColType() != eColGroupAnonymousCell)) {
    int32_t colIndex = (colGroupFrame) ?
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
                           true);

}



void
nsTableFrame::AppendAnonymousColFrames(nsTableColGroupFrame* aColGroupFrame,
                                       int32_t               aNumColsToAdd,
                                       nsTableColType        aColType,
                                       bool                  aAddToTable)
{
  NS_PRECONDITION(aColGroupFrame, "null frame");
  NS_PRECONDITION(aColType != eColAnonymousCol, "Shouldn't happen");

  nsIPresShell *shell = PresContext()->PresShell();

  
  nsFrameList newColFrames;

  int32_t startIndex = mColFrames.Length();
  int32_t lastIndex  = startIndex + aNumColsToAdd - 1;

  for (int32_t childX = startIndex; childX <= lastIndex; childX++) {
    nsIContent* iContent;
    nsRefPtr<nsStyleContext> styleContext;
    nsStyleContext* parentStyleContext;

    
    
    iContent = aColGroupFrame->GetContent();
    parentStyleContext = aColGroupFrame->StyleContext();
    styleContext = shell->StyleSet()->
      ResolveAnonymousBoxStyle(nsCSSAnonBoxes::tableCol, parentStyleContext);
    
    NS_ASSERTION(iContent, "null content in CreateAnonymousColFrames");

    
    nsIFrame* colFrame = NS_NewTableColFrame(shell, styleContext);
    ((nsTableColFrame *) colFrame)->SetColType(aColType);
    colFrame->Init(iContent, aColGroupFrame, nullptr);

    newColFrames.AppendFrame(nullptr, colFrame);
  }
  nsFrameList& cols = aColGroupFrame->GetWritableChildList();
  nsIFrame* oldLastCol = cols.LastChild();
  const nsFrameList::Slice& newCols =
    cols.InsertFrames(nullptr, oldLastCol, newColFrames);
  if (aAddToTable) {
    
    int32_t startColIndex;
    if (oldLastCol) {
      startColIndex =
        static_cast<nsTableColFrame*>(oldLastCol)->GetColIndex() + 1;
    } else {
      startColIndex = aColGroupFrame->GetStartColumnIndex();
    }

    aColGroupFrame->AddColsToTable(startColIndex, true, newCols);
  }
}

void
nsTableFrame::MatchCellMapToColCache(nsTableCellMap* aCellMap)
{
  int32_t numColsInMap   = GetColCount();
  int32_t numColsInCache = mColFrames.Length();
  int32_t numColsToAdd = numColsInMap - numColsInCache;
  if (numColsToAdd > 0) {
    
    AppendAnonymousColFrames(numColsToAdd);
  }
  if (numColsToAdd < 0) {
    int32_t numColsNotRemoved = DestroyAnonymousColFrames(-numColsToAdd);
    
    if (numColsNotRemoved > 0) {
      aCellMap->AddColsAtEnd(numColsNotRemoved);
    }
  }
  if (numColsToAdd && HasZeroColSpans()) {
    SetNeedColSpanExpansion(true);
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
    f->mBits.mResizedColumns = true;
}

void
nsTableFrame::AppendCell(nsTableCellFrame& aCellFrame,
                         int32_t           aRowIndex)
{
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    nsIntRect damageArea(0,0,0,0);
    cellMap->AppendCell(aCellFrame, aRowIndex, true, damageArea);
    MatchCellMapToColCache(cellMap);
    if (IsBorderCollapse()) {
      AddBCDamageArea(damageArea);
    }
  }
}

void nsTableFrame::InsertCells(nsTArray<nsTableCellFrame*>& aCellFrames,
                               int32_t                      aRowIndex,
                               int32_t                      aColIndexBefore)
{
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    nsIntRect damageArea(0,0,0,0);
    cellMap->InsertCells(aCellFrames, aRowIndex, aColIndexBefore, damageArea);
    MatchCellMapToColCache(cellMap);
    if (IsBorderCollapse()) {
      AddBCDamageArea(damageArea);
    }
  }
}


int32_t
nsTableFrame::DestroyAnonymousColFrames(int32_t aNumFrames)
{
  
  int32_t endIndex   = mColFrames.Length() - 1;
  int32_t startIndex = (endIndex - aNumFrames) + 1;
  int32_t numColsRemoved = 0;
  for (int32_t colX = endIndex; colX >= startIndex; colX--) {
    nsTableColFrame* colFrame = GetColFrame(colX);
    if (colFrame && (eColAnonymousCell == colFrame->GetColType())) {
      nsTableColGroupFrame* cgFrame =
        static_cast<nsTableColGroupFrame*>(colFrame->GetParent());
      
      cgFrame->RemoveChild(*colFrame, false);
      
      RemoveCol(nullptr, colX, true, false);
      numColsRemoved++;
    }
    else {
      break;
    }
  }
  return (aNumFrames - numColsRemoved);
}

void nsTableFrame::RemoveCell(nsTableCellFrame* aCellFrame,
                              int32_t           aRowIndex)
{
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    nsIntRect damageArea(0,0,0,0);
    cellMap->RemoveCell(aCellFrame, aRowIndex, damageArea);
    MatchCellMapToColCache(cellMap);
    if (IsBorderCollapse()) {
      AddBCDamageArea(damageArea);
    }
  }
}

int32_t
nsTableFrame::GetStartRowIndex(nsTableRowGroupFrame* aRowGroupFrame)
{
  RowGroupArray orderedRowGroups;
  OrderRowGroups(orderedRowGroups);

  int32_t rowIndex = 0;
  for (uint32_t rgIndex = 0; rgIndex < orderedRowGroups.Length(); rgIndex++) {
    nsTableRowGroupFrame* rgFrame = orderedRowGroups[rgIndex];
    if (rgFrame == aRowGroupFrame) {
      break;
    }
    int32_t numRows = rgFrame->GetRowCount();
    rowIndex += numRows;
  }
  return rowIndex;
}


void nsTableFrame::AppendRows(nsTableRowGroupFrame*       aRowGroupFrame,
                              int32_t                     aRowIndex,
                              nsTArray<nsTableRowFrame*>& aRowFrames)
{
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    int32_t absRowIndex = GetStartRowIndex(aRowGroupFrame) + aRowIndex;
    InsertRows(aRowGroupFrame, aRowFrames, absRowIndex, true);
  }
}


int32_t
nsTableFrame::InsertRows(nsTableRowGroupFrame*       aRowGroupFrame,
                         nsTArray<nsTableRowFrame*>& aRowFrames,
                         int32_t                     aRowIndex,
                         bool                        aConsiderSpans)
{
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== insertRowsBefore firstRow=%d \n", aRowIndex);
  Dump(true, false, true);
#endif

  int32_t numColsToAdd = 0;
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    nsIntRect damageArea(0,0,0,0);
    int32_t origNumRows = cellMap->GetRowCount();
    int32_t numNewRows = aRowFrames.Length();
    cellMap->InsertRows(aRowGroupFrame, aRowFrames, aRowIndex, aConsiderSpans, damageArea);
    MatchCellMapToColCache(cellMap);
    if (aRowIndex < origNumRows) {
      AdjustRowIndices(aRowIndex, numNewRows);
    }
    
    
    for (int32_t rowY = 0; rowY < numNewRows; rowY++) {
      nsTableRowFrame* rowFrame = aRowFrames.ElementAt(rowY);
      rowFrame->SetRowIndex(aRowIndex + rowY);
    }
    if (IsBorderCollapse()) {
      AddBCDamageArea(damageArea);
    }
  }
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== insertRowsAfter \n");
  Dump(true, false, true);
#endif

  return numColsToAdd;
}


void nsTableFrame::RemoveRows(nsTableRowFrame& aFirstRowFrame,
                              int32_t          aNumRowsToRemove,
                              bool             aConsiderSpans)
{
#ifdef TBD_OPTIMIZATION
  
  
  bool stopTelling = false;
  for (nsIFrame* kidFrame = aFirstFrame.FirstChild(); (kidFrame && !stopAsking);
       kidFrame = kidFrame->GetNextSibling()) {
    nsTableCellFrame *cellFrame = do_QueryFrame(kidFrame);
    if (cellFrame) {
      stopTelling = tableFrame->CellChangedWidth(*cellFrame, cellFrame->GetPass1MaxElementWidth(),
                                                 cellFrame->GetMaximumWidth(), true);
    }
  }
  
  
#endif

  int32_t firstRowIndex = aFirstRowFrame.GetRowIndex();
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== removeRowsBefore firstRow=%d numRows=%d\n", firstRowIndex, aNumRowsToRemove);
  Dump(true, false, true);
#endif
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    nsIntRect damageArea(0,0,0,0);
    cellMap->RemoveRows(firstRowIndex, aNumRowsToRemove, aConsiderSpans, damageArea);
    MatchCellMapToColCache(cellMap);
    if (IsBorderCollapse()) {
      AddBCDamageArea(damageArea);
    }
  }
  AdjustRowIndices(firstRowIndex, -aNumRowsToRemove);
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== removeRowsAfter\n");
  Dump(true, true, true);
#endif
}


int32_t
nsTableFrame::CollectRows(nsIFrame*                   aFrame,
                          nsTArray<nsTableRowFrame*>& aCollection)
{
  NS_PRECONDITION(aFrame, "null frame");
  int32_t numRows = 0;
  nsIFrame* childFrame = aFrame->GetFirstPrincipalChild();
  while (childFrame) {
    aCollection.AppendElement(static_cast<nsTableRowFrame*>(childFrame));
    numRows++;
    childFrame = childFrame->GetNextSibling();
  }
  return numRows;
}

void
nsTableFrame::InsertRowGroups(const nsFrameList::Slice& aRowGroups)
{
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== insertRowGroupsBefore\n");
  Dump(true, false, true);
#endif
  nsTableCellMap* cellMap = GetCellMap();
  if (cellMap) {
    RowGroupArray orderedRowGroups;
    OrderRowGroups(orderedRowGroups);

    nsAutoTArray<nsTableRowFrame*, 8> rows;
    
    
    
    
    uint32_t rgIndex;
    for (rgIndex = 0; rgIndex < orderedRowGroups.Length(); rgIndex++) {
      for (nsFrameList::Enumerator rowgroups(aRowGroups); !rowgroups.AtEnd();
           rowgroups.Next()) {
        if (orderedRowGroups[rgIndex] == rowgroups.get()) {
          nsTableRowGroupFrame* priorRG =
            (0 == rgIndex) ? nullptr : orderedRowGroups[rgIndex - 1];
          
          cellMap->InsertGroupCellMap(orderedRowGroups[rgIndex], priorRG);

          break;
        }
      }
    }
    cellMap->Synchronize(this);
    ResetRowIndices(aRowGroups);

    
    for (rgIndex = 0; rgIndex < orderedRowGroups.Length(); rgIndex++) {
      for (nsFrameList::Enumerator rowgroups(aRowGroups); !rowgroups.AtEnd();
           rowgroups.Next()) {
        if (orderedRowGroups[rgIndex] == rowgroups.get()) {
          nsTableRowGroupFrame* priorRG =
            (0 == rgIndex) ? nullptr : orderedRowGroups[rgIndex - 1];
          
          int32_t numRows = CollectRows(rowgroups.get(), rows);
          if (numRows > 0) {
            int32_t rowIndex = 0;
            if (priorRG) {
              int32_t priorNumRows = priorRG->GetRowCount();
              rowIndex = priorRG->GetStartRowIndex() + priorNumRows;
            }
            InsertRows(orderedRowGroups[rgIndex], rows, rowIndex, true);
            rows.Clear();
          }
          break;
        }
      }
    }

  }
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== insertRowGroupsAfter\n");
  Dump(true, true, true);
#endif
}





const nsFrameList&
nsTableFrame::GetChildList(ChildListID aListID) const
{
  if (aListID == kColGroupList) {
    return mColGroups;
  }
  return nsContainerFrame::GetChildList(aListID);
}

void
nsTableFrame::GetChildLists(nsTArray<ChildList>* aLists) const
{
  nsContainerFrame::GetChildLists(aLists);
  mColGroups.AppendIfNonempty(aLists, kColGroupList);
}

nsRect
nsDisplayTableItem::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) {
  *aSnap = false;
  return mFrame->GetVisualOverflowRectRelativeToSelf() + ToReferenceFrame();
}

void
nsDisplayTableItem::UpdateForFrameBackground(nsIFrame* aFrame)
{
  nsStyleContext *bgSC;
  if (!nsCSSRendering::FindBackground(aFrame, &bgSC))
    return;
  if (!bgSC->StyleBackground()->HasFixedBackground())
    return;

  mPartHasFixedBackground = true;
}

nsDisplayItemGeometry*
nsDisplayTableItem::AllocateGeometry(nsDisplayListBuilder* aBuilder)
{
  return new nsDisplayTableItemGeometry(this, aBuilder,
      mFrame->GetOffsetTo(mFrame->PresContext()->PresShell()->GetRootFrame()));
}

void
nsDisplayTableItem::ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                              const nsDisplayItemGeometry* aGeometry,
                                              nsRegion *aInvalidRegion)
{
  auto geometry =
    static_cast<const nsDisplayTableItemGeometry*>(aGeometry);

  bool invalidateForAttachmentFixed = false;
  if (mPartHasFixedBackground) {
    nsPoint frameOffsetToViewport = mFrame->GetOffsetTo(
        mFrame->PresContext()->PresShell()->GetRootFrame());
    invalidateForAttachmentFixed =
        frameOffsetToViewport != geometry->mFrameOffsetToViewport;
  }

  if (invalidateForAttachmentFixed ||
      (aBuilder->ShouldSyncDecodeImages() &&
       geometry->ShouldInvalidateToSyncDecodeImages())) {
    bool snap;
    aInvalidRegion->Or(*aInvalidRegion, GetBounds(aBuilder, &snap));
  }

  nsDisplayItem::ComputeInvalidationRegion(aBuilder, aGeometry, aInvalidRegion);
}

class nsDisplayTableBorderBackground : public nsDisplayTableItem {
public:
  nsDisplayTableBorderBackground(nsDisplayListBuilder* aBuilder,
                                 nsTableFrame* aFrame) :
    nsDisplayTableItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayTableBorderBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTableBorderBackground() {
    MOZ_COUNT_DTOR(nsDisplayTableBorderBackground);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) override;
  NS_DISPLAY_DECL_NAME("TableBorderBackground", TYPE_TABLE_BORDER_BACKGROUND)
};

#ifdef DEBUG
static bool
IsFrameAllowedInTable(nsIAtom* aType)
{
  return IS_TABLE_CELL(aType) ||
         nsGkAtoms::tableRowFrame == aType ||
         nsGkAtoms::tableRowGroupFrame == aType ||
         nsGkAtoms::scrollFrame == aType ||
         nsGkAtoms::tableFrame == aType ||
         nsGkAtoms::tableColFrame == aType ||
         nsGkAtoms::tableColGroupFrame == aType;
}
#endif

void
nsDisplayTableBorderBackground::Paint(nsDisplayListBuilder* aBuilder,
                                      nsRenderingContext* aCtx)
{
  DrawResult result = static_cast<nsTableFrame*>(mFrame)->
    PaintTableBorderBackground(*aCtx, mVisibleRect,
                               ToReferenceFrame(),
                               aBuilder->GetBackgroundPaintFlags());

  nsDisplayTableItemGeometry::UpdateDrawResult(this, result);
}

static int32_t GetTablePartRank(nsDisplayItem* aItem)
{
  nsIAtom* type = aItem->Frame()->GetType();
  if (type == nsGkAtoms::tableFrame)
    return 0;
  if (type == nsGkAtoms::tableRowGroupFrame)
    return 1;
  if (type == nsGkAtoms::tableRowFrame)
    return 2;
  return 3;
}

static bool CompareByTablePartRank(nsDisplayItem* aItem1, nsDisplayItem* aItem2,
                                     void* aClosure)
{
  return GetTablePartRank(aItem1) <= GetTablePartRank(aItem2);
}

 void
nsTableFrame::GenericTraversal(nsDisplayListBuilder* aBuilder, nsFrame* aFrame,
                               const nsRect& aDirtyRect, const nsDisplayListSet& aLists)
{
  
  
  
  
  
  
  
  
  nsIFrame* kid = aFrame->GetFirstPrincipalChild();
  while (kid) {
    aFrame->BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
    kid = kid->GetNextSibling();
  }
}

 void
nsTableFrame::DisplayGenericTablePart(nsDisplayListBuilder* aBuilder,
                                      nsFrame* aFrame,
                                      const nsRect& aDirtyRect,
                                      const nsDisplayListSet& aLists,
                                      nsDisplayTableItem* aDisplayItem,
                                      DisplayGenericTablePartTraversal aTraversal)
{
  nsDisplayList eventsBorderBackground;
  
  
  bool sortEventBackgrounds = aDisplayItem && aBuilder->IsForEventDelivery();
  nsDisplayListCollection separatedCollection;
  const nsDisplayListSet* lists = sortEventBackgrounds ? &separatedCollection : &aLists;

  nsAutoPushCurrentTableItem pushTableItem;
  if (aDisplayItem) {
    pushTableItem.Push(aBuilder, aDisplayItem);
  }

  if (aFrame->IsVisibleForPainting(aBuilder)) {
    nsDisplayTableItem* currentItem = aBuilder->GetCurrentTableItem();
    
    
    if (currentItem) {
      currentItem->UpdateForFrameBackground(aFrame);
    }

    
    bool hasBoxShadow = aFrame->StyleBorder()->mBoxShadow != nullptr;
    if (hasBoxShadow) {
      lists->BorderBackground()->AppendNewToTop(
        new (aBuilder) nsDisplayBoxShadowOuter(aBuilder, aFrame));
    }

    
    
    
    if (aBuilder->IsForEventDelivery()) {
      nsDisplayBackgroundImage::AppendBackgroundItemsToTop(aBuilder, aFrame,
                                                           lists->BorderBackground());
    }

    
    if (hasBoxShadow) {
      lists->BorderBackground()->AppendNewToTop(
        new (aBuilder) nsDisplayBoxShadowInner(aBuilder, aFrame));
    }
  }

  aTraversal(aBuilder, aFrame, aDirtyRect, *lists);

  if (sortEventBackgrounds) {
    
    
    
    separatedCollection.BorderBackground()->Sort(aBuilder, CompareByTablePartRank, nullptr);
    separatedCollection.MoveTo(aLists);
  }

  aFrame->DisplayOutline(aBuilder, aLists);
}

static bool
AnyTablePartHasBorderOrBackground(nsIFrame* aStart, nsIFrame* aEnd)
{
  for (nsIFrame* f = aStart; f != aEnd; f = f->GetNextSibling()) {
    NS_ASSERTION(IsFrameAllowedInTable(f->GetType()), "unexpected frame type");

    if (FrameHasBorderOrBackground(f))
      return true;

    nsTableCellFrame *cellFrame = do_QueryFrame(f);
    if (cellFrame)
      continue;

    if (AnyTablePartHasBorderOrBackground(f->PrincipalChildList().FirstChild(), nullptr))
      return true;
  }

  return false;
}

static void
UpdateItemForColGroupBackgrounds(nsDisplayTableItem* item,
                                 const nsFrameList& aFrames) {
  for (nsFrameList::Enumerator e(aFrames); !e.AtEnd(); e.Next()) {
    nsTableColGroupFrame* cg = static_cast<nsTableColGroupFrame*>(e.get());
    item->UpdateForFrameBackground(cg);
    for (nsTableColFrame* colFrame = cg->GetFirstColumn(); colFrame;
         colFrame = colFrame->GetNextCol()) {
      item->UpdateForFrameBackground(colFrame);
    }
  }
}



void
nsTableFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                               const nsRect&           aDirtyRect,
                               const nsDisplayListSet& aLists)
{
  DO_GLOBAL_REFLOW_COUNT_DSP_COLOR("nsTableFrame", NS_RGB(255,128,255));

  nsDisplayTableItem* item = nullptr;
  if (IsVisibleInSelection(aBuilder)) {
    if (StyleVisibility()->IsVisible()) {
      nsMargin deflate = GetDeflationForBackground(PresContext());
      
      
      
      if (deflate == nsMargin(0, 0, 0, 0)) {
        DisplayBackgroundUnconditional(aBuilder, aLists, false);
      }
    }

    
    
    
    
    
    if (aBuilder->IsForEventDelivery() ||
        AnyTablePartHasBorderOrBackground(this, GetNextSibling()) ||
        AnyTablePartHasBorderOrBackground(mColGroups.FirstChild(), nullptr)) {
      item = new (aBuilder) nsDisplayTableBorderBackground(aBuilder, this);
      aLists.BorderBackground()->AppendNewToTop(item);
    }
  }
  DisplayGenericTablePart(aBuilder, this, aDirtyRect, aLists, item);
  if (item) {
    UpdateItemForColGroupBackgrounds(item, mColGroups);
  }
}

nsMargin
nsTableFrame::GetDeflationForBackground(nsPresContext* aPresContext) const
{
  if (eCompatibility_NavQuirks != aPresContext->CompatibilityMode() ||
      !IsBorderCollapse())
    return nsMargin(0,0,0,0);

  return GetOuterBCBorder();
}



DrawResult
nsTableFrame::PaintTableBorderBackground(nsRenderingContext& aRenderingContext,
                                         const nsRect& aDirtyRect,
                                         nsPoint aPt, uint32_t aBGPaintFlags)
{
  nsPresContext* presContext = PresContext();

  TableBackgroundPainter painter(this, TableBackgroundPainter::eOrigin_Table,
                                 presContext, aRenderingContext,
                                 aDirtyRect, aPt, aBGPaintFlags);
  nsMargin deflate = GetDeflationForBackground(presContext);
  
  
  DrawResult result =
    painter.PaintTable(this, deflate, deflate != nsMargin(0, 0, 0, 0));

  if (StyleVisibility()->IsVisible()) {
    if (!IsBorderCollapse()) {
      Sides skipSides = GetSkipSides();
      nsRect rect(aPt, mRect.Size());
      nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                  aDirtyRect, rect, mStyleContext, skipSides);
    }
    else {
      gfxContext* ctx = aRenderingContext.ThebesContext();

      gfxPoint devPixelOffset =
        nsLayoutUtils::PointToGfxPoint(aPt,
                                       PresContext()->AppUnitsPerDevPixel());

      
      
      gfxContextMatrixAutoSaveRestore autoSR(ctx);
      ctx->SetMatrix(ctx->CurrentMatrix().Translate(devPixelOffset));

      PaintBCBorders(aRenderingContext, aDirtyRect - aPt);
    }
  }

  return result;
}

nsIFrame::LogicalSides
nsTableFrame::GetLogicalSkipSides(const nsHTMLReflowState* aReflowState) const
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
nsTableFrame::SetColumnDimensions(nscoord         aHeight,
                                  const nsMargin& aBorderPadding)
{
  nscoord colHeight = aHeight -= aBorderPadding.top + aBorderPadding.bottom +
                                 GetRowSpacing(-1) +
                                 GetRowSpacing(GetRowCount());

  nsTableIterator iter(mColGroups);
  nsIFrame* colGroupFrame = iter.First();
  bool tableIsLTR = StyleVisibility()->mDirection == NS_STYLE_DIRECTION_LTR;
  int32_t colX =tableIsLTR ? 0 : std::max(0, GetColCount() - 1);
  nscoord cellSpacingX = GetColSpacing(colX);
  int32_t tableColIncr = tableIsLTR ? 1 : -1;
  nsPoint colGroupOrigin(aBorderPadding.left + GetColSpacing(-1),
                         aBorderPadding.top + GetRowSpacing(-1));
  while (colGroupFrame) {
    MOZ_ASSERT(colGroupFrame->GetType() == nsGkAtoms::tableColGroupFrame);
    nscoord colGroupWidth = 0;
    nsTableIterator iterCol(*colGroupFrame);
    nsIFrame* colFrame = iterCol.First();
    nsPoint colOrigin(0,0);
    while (colFrame) {
      if (NS_STYLE_DISPLAY_TABLE_COLUMN ==
          colFrame->StyleDisplay()->mDisplay) {
        NS_ASSERTION(colX < GetColCount(), "invalid number of columns");
        nscoord colWidth = GetColumnWidth(colX);
        nsRect colRect(colOrigin.x, colOrigin.y, colWidth, colHeight);
        colFrame->SetRect(colRect);
        cellSpacingX = GetColSpacing(colX);
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
  SetRowInserted(false); 
  nsTableFrame::RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);
  
  for (uint32_t rgX = 0; rgX < rowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
    NS_ASSERTION(rgFrame, "Must have rgFrame here");
    nsIFrame* childFrame = rgFrame->GetFirstPrincipalChild();
    
    while (childFrame) {
      nsTableRowFrame *rowFrame = do_QueryFrame(childFrame);
      if (rowFrame) {
        if (rowFrame->IsFirstInserted()) {
          rowFrame->SetFirstInserted(false);
          
          nsIFrame::InvalidateFrame();
          
          SetRowInserted(false);
          return; 
        }
      }
      childFrame = childFrame->GetNextSibling();
    }
  }
}

 void
nsTableFrame::MarkIntrinsicISizesDirty()
{
  nsITableLayoutStrategy* tls = LayoutStrategy();
  if (MOZ_UNLIKELY(!tls)) {
    
    
    
    
    
    
    return;
  }
  tls->MarkIntrinsicISizesDirty();

  

  nsContainerFrame::MarkIntrinsicISizesDirty();
}

 nscoord
nsTableFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  if (NeedToCalcBCBorders())
    CalcBCBorders();

  ReflowColGroups(aRenderingContext);

  return LayoutStrategy()->GetMinISize(aRenderingContext);
}

 nscoord
nsTableFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  if (NeedToCalcBCBorders())
    CalcBCBorders();

  ReflowColGroups(aRenderingContext);

  return LayoutStrategy()->GetPrefISize(aRenderingContext, false);
}

 nsIFrame::IntrinsicISizeOffsetData
nsTableFrame::IntrinsicISizeOffsets(nsRenderingContext* aRenderingContext)
{
  IntrinsicISizeOffsetData result =
    nsContainerFrame::IntrinsicISizeOffsets(aRenderingContext);

  result.hMargin = 0;
  result.hPctMargin = 0;

  if (IsBorderCollapse()) {
    result.hPadding = 0;
    result.hPctPadding = 0;

    nsMargin outerBC = GetIncludedOuterBCBorder();
    result.hBorder = outerBC.LeftRight();
  }

  return result;
}


LogicalSize
nsTableFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                          WritingMode aWM,
                          const LogicalSize& aCBSize,
                          nscoord aAvailableISize,
                          const LogicalSize& aMargin,
                          const LogicalSize& aBorder,
                          const LogicalSize& aPadding,
                          ComputeSizeFlags aFlags)
{
  LogicalSize result =
    nsContainerFrame::ComputeSize(aRenderingContext, aWM,
                                  aCBSize, aAvailableISize,
                                  aMargin, aBorder, aPadding, aFlags);

  
  
  
  if (aWM.IsVertical() != GetWritingMode().IsVertical()) {
    return result;
  }

  
  
  AutoMaybeDisableFontInflation an(this);

  
  nscoord minISize = GetMinISize(aRenderingContext);
  if (minISize > result.ISize(aWM)) {
    result.ISize(aWM) = minISize;
  }

  return result;
}

nscoord
nsTableFrame::TableShrinkWidthToFit(nsRenderingContext *aRenderingContext,
                                    nscoord aWidthInCB)
{
  
  
  AutoMaybeDisableFontInflation an(this);

  nscoord result;
  nscoord minWidth = GetMinISize(aRenderingContext);
  if (minWidth > aWidthInCB) {
    result = minWidth;
  } else {
    
    
    
    
    
    
    
    nscoord prefWidth =
      LayoutStrategy()->GetPrefISize(aRenderingContext, true);
    if (prefWidth > aWidthInCB) {
      result = aWidthInCB;
    } else {
      result = prefWidth;
    }
  }
  return result;
}


LogicalSize
nsTableFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                              WritingMode aWM,
                              const LogicalSize& aCBSize,
                              nscoord aAvailableISize,
                              const LogicalSize& aMargin,
                              const LogicalSize& aBorder,
                              const LogicalSize& aPadding,
                              bool aShrinkWrap)
{
  
  nscoord cbBased = aAvailableISize - aMargin.ISize(aWM) - aBorder.ISize(aWM) -
                    aPadding.ISize(aWM);
  return LogicalSize(aWM, TableShrinkWidthToFit(aRenderingContext, cbBased),
                     NS_UNCONSTRAINEDSIZE);
}



bool
nsTableFrame::AncestorsHaveStyleHeight(const nsHTMLReflowState& aParentReflowState)
{
  for (const nsHTMLReflowState* rs = &aParentReflowState;
       rs && rs->frame; rs = rs->parentReflowState) {
    nsIAtom* frameType = rs->frame->GetType();
    if (IS_TABLE_CELL(frameType)                     ||
        (nsGkAtoms::tableRowFrame      == frameType) ||
        (nsGkAtoms::tableRowGroupFrame == frameType)) {
      const nsStyleCoord &height = rs->mStylePosition->mHeight;
      
      if (height.GetUnit() != eStyleUnit_Auto &&
          (!height.IsCalcUnit() || !height.HasPercent())) {
        return true;
      }
    }
    else if (nsGkAtoms::tableFrame == frameType) {
      
      return rs->mStylePosition->mHeight.GetUnit() != eStyleUnit_Auto;
    }
  }
  return false;
}


void
nsTableFrame::CheckRequestSpecialHeightReflow(const nsHTMLReflowState& aReflowState)
{
  NS_ASSERTION(IS_TABLE_CELL(aReflowState.frame->GetType()) ||
               aReflowState.frame->GetType() == nsGkAtoms::tableRowFrame ||
               aReflowState.frame->GetType() == nsGkAtoms::tableRowGroupFrame ||
               aReflowState.frame->GetType() == nsGkAtoms::tableFrame,
               "unexpected frame type");
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





























































void
nsTableFrame::Reflow(nsPresContext*           aPresContext,
                     nsHTMLReflowMetrics&     aDesiredSize,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsTableFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  bool isPaginated = aPresContext->IsPaginated();

  aStatus = NS_FRAME_COMPLETE;
  if (!GetPrevInFlow() && !mTableLayoutStrategy) {
    NS_ASSERTION(false, "strategy should have been created in Init");
    return;
  }

  
  if (!GetPrevInFlow() && IsBorderCollapse() && NeedToCalcBCBorders()) {
    CalcBCBorders();
  }

  aDesiredSize.Width() = aReflowState.AvailableWidth();

  
  MoveOverflowToChildList();

  bool haveDesiredHeight = false;
  SetHaveReflowedColGroups(false);

  
  
  
  
  if (NS_SUBTREE_DIRTY(this) ||
      aReflowState.ShouldReflowAllKids() ||
      IsGeometryDirty() ||
      aReflowState.IsVResize()) {

    if (aReflowState.ComputedHeight() != NS_UNCONSTRAINEDSIZE ||
        
        
        
        
        aReflowState.IsVResize()) {
      
      
      
      
      
      
      
      SetGeometryDirty();
    }

    bool needToInitiateSpecialReflow =
      !!(GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
    
    if (isPaginated && !GetPrevInFlow() && (NS_UNCONSTRAINEDSIZE != aReflowState.AvailableHeight())) {
      nscoord tableSpecifiedHeight = CalcBorderBoxHeight(aReflowState);
      if ((tableSpecifiedHeight > 0) &&
          (tableSpecifiedHeight != NS_UNCONSTRAINEDSIZE)) {
        needToInitiateSpecialReflow = true;
      }
    }
    nsIFrame* lastChildReflowed = nullptr;

    NS_ASSERTION(!aReflowState.mFlags.mSpecialHeightReflow,
                 "Shouldn't be in special height reflow here!");

    
    
    

    
    
    nscoord availHeight = needToInitiateSpecialReflow
                          ? NS_UNCONSTRAINEDSIZE : aReflowState.AvailableHeight();

    ReflowTable(aDesiredSize, aReflowState, availHeight,
                lastChildReflowed, aStatus);

    
    if (GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT)
      needToInitiateSpecialReflow = true;

    
    if (needToInitiateSpecialReflow && NS_FRAME_IS_COMPLETE(aStatus)) {
      

      nsHTMLReflowState &mutable_rs =
        const_cast<nsHTMLReflowState&>(aReflowState);

      
      CalcDesiredHeight(aReflowState, aDesiredSize);
      mutable_rs.mFlags.mSpecialHeightReflow = true;

      ReflowTable(aDesiredSize, aReflowState, aReflowState.AvailableHeight(),
                  lastChildReflowed, aStatus);

      if (lastChildReflowed && NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
        
        nsMargin borderPadding = GetChildAreaOffset(&aReflowState);
        aDesiredSize.Height() = borderPadding.bottom + GetRowSpacing(GetRowCount()) +
                              lastChildReflowed->GetNormalRect().YMost();
      }
      haveDesiredHeight = true;

      mutable_rs.mFlags.mSpecialHeightReflow = false;
    }
  }
  else {
    
    for (nsIFrame* kid = GetFirstPrincipalChild(); kid; kid = kid->GetNextSibling()) {
      ConsiderChildOverflow(aDesiredSize.mOverflowAreas, kid);
    }
  }

  aDesiredSize.Width() = aReflowState.ComputedWidth() +
                       aReflowState.ComputedPhysicalBorderPadding().LeftRight();
  if (!haveDesiredHeight) {
    CalcDesiredHeight(aReflowState, aDesiredSize);
  }
  if (IsRowInserted()) {
    ProcessRowInserted(aDesiredSize.Height());
  }

  nsMargin borderPadding = GetChildAreaOffset(&aReflowState);
  SetColumnDimensions(aDesiredSize.Height(), borderPadding);
  if (NeedToCollapse() &&
      (NS_UNCONSTRAINEDSIZE != aReflowState.AvailableWidth())) {
    AdjustForCollapsingRowsCols(aDesiredSize, borderPadding);
  }

  
  
  FixupPositionedTableParts(aPresContext, aDesiredSize, aReflowState);

  
  nsRect tableRect(0, 0, aDesiredSize.Width(), aDesiredSize.Height()) ;

  if (!ShouldApplyOverflowClipping(this, aReflowState.mStyleDisplay)) {
    
    nsMargin bcMargin = GetExcludedOuterBCBorder();
    tableRect.Inflate(bcMargin);
  }
  aDesiredSize.mOverflowAreas.UnionAllWith(tableRect);

  if ((GetStateBits() & NS_FRAME_FIRST_REFLOW) ||
      nsSize(aDesiredSize.Width(), aDesiredSize.Height()) != mRect.Size()) {
      nsIFrame::InvalidateFrame();
  }

  FinishAndStoreOverflow(&aDesiredSize);
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

void
nsTableFrame::FixupPositionedTableParts(nsPresContext*           aPresContext,
                                        nsHTMLReflowMetrics&     aDesiredSize,
                                        const nsHTMLReflowState& aReflowState)
{
  auto positionedParts =
    static_cast<FrameTArray*>(Properties().Get(PositionedTablePartArray()));
  if (!positionedParts) {
    return;
  }

  OverflowChangedTracker overflowTracker;
  overflowTracker.SetSubtreeRoot(this);

  for (size_t i = 0; i < positionedParts->Length(); ++i) {
    nsIFrame* positionedPart = positionedParts->ElementAt(i);

    
    
    nsSize size(positionedPart->GetSize());
    nsHTMLReflowMetrics desiredSize(aReflowState.GetWritingMode());
    desiredSize.Width() = size.width;
    desiredSize.Height() = size.height;
    desiredSize.mOverflowAreas = positionedPart->GetOverflowAreasRelativeToSelf();

    
    
    
    
    WritingMode wm = positionedPart->GetWritingMode();
    LogicalSize availSize(wm, size);
    availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
    nsHTMLReflowState reflowState(aPresContext, positionedPart,
                                  aReflowState.rendContext, availSize,
                                  nsHTMLReflowState::DUMMY_PARENT_REFLOW_STATE);
    nsReflowStatus reflowStatus = NS_FRAME_COMPLETE;

    
    
    
    
    nsFrame* positionedFrame = static_cast<nsFrame*>(positionedPart);
    positionedFrame->FinishReflowWithAbsoluteFrames(PresContext(),
                                                    desiredSize,
                                                    reflowState,
                                                    reflowStatus,
                                                    true);

    
    
    
    nsIFrame* positionedFrameParent = positionedPart->GetParent();
    if (positionedFrameParent != this) {
      overflowTracker.AddFrame(positionedFrameParent,
        OverflowChangedTracker::CHILDREN_CHANGED);
    }
  }

  
  overflowTracker.Flush();

  
  
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  nsLayoutUtils::UnionChildOverflow(this, aDesiredSize.mOverflowAreas);
}

bool
nsTableFrame::UpdateOverflow()
{
  nsRect bounds(nsPoint(0, 0), GetSize());

  
  
  if (!ShouldApplyOverflowClipping(this, StyleDisplay())) {
    nsMargin bcMargin = GetExcludedOuterBCBorder();
    bounds.Inflate(bcMargin);
  }

  nsOverflowAreas overflowAreas(bounds, bounds);
  nsLayoutUtils::UnionChildOverflow(this, overflowAreas);

  return FinishAndStoreOverflow(overflowAreas, GetSize());
}

void
nsTableFrame::ReflowTable(nsHTMLReflowMetrics&     aDesiredSize,
                          const nsHTMLReflowState& aReflowState,
                          nscoord                  aAvailHeight,
                          nsIFrame*&               aLastChildReflowed,
                          nsReflowStatus&          aStatus)
{
  aLastChildReflowed = nullptr;

  if (!GetPrevInFlow()) {
    mTableLayoutStrategy->ComputeColumnISizes(aReflowState);
  }
  
  
  aDesiredSize.Width() = aReflowState.ComputedWidth() +
                       aReflowState.ComputedPhysicalBorderPadding().LeftRight();
  nsTableReflowState reflowState(*PresContext(), aReflowState, *this,
                                 aDesiredSize.Width(), aAvailHeight);
  ReflowChildren(reflowState, aStatus, aLastChildReflowed,
                 aDesiredSize.mOverflowAreas);

  ReflowColGroups(aReflowState.rendContext);
}

nsIFrame*
nsTableFrame::GetFirstBodyRowGroupFrame()
{
  nsIFrame* headerFrame = nullptr;
  nsIFrame* footerFrame = nullptr;

  for (nsIFrame* kidFrame = mFrames.FirstChild(); nullptr != kidFrame; ) {
    const nsStyleDisplay* childDisplay = kidFrame->StyleDisplay();

    
    
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

  return nullptr;
}



void
nsTableFrame::PushChildren(const RowGroupArray& aRowGroups,
                           int32_t aPushFrom)
{
  NS_PRECONDITION(aPushFrom > 0, "pushing first child");

  
  nsFrameList frames;
  uint32_t childX;
  for (childX = aPushFrom; childX < aRowGroups.Length(); ++childX) {
    nsTableRowGroupFrame* rgFrame = aRowGroups[childX];
    if (!rgFrame->IsRepeatable()) {
      mFrames.RemoveFrame(rgFrame);
      frames.AppendFrame(nullptr, rgFrame);
    }
  }

  if (frames.IsEmpty()) {
    return;
  }

  nsTableFrame* nextInFlow = static_cast<nsTableFrame*>(GetNextInFlow());
  if (nextInFlow) {
    
    nsIFrame* firstBodyFrame = nextInFlow->GetFirstBodyRowGroupFrame();
    nsIFrame* prevSibling = nullptr;
    if (firstBodyFrame) {
      prevSibling = firstBodyFrame->GetPrevSibling();
    }
    
    
    ReparentFrameViewList(frames, this, nextInFlow);
    nextInFlow->mFrames.InsertFrames(nextInFlow, prevSibling,
                                     frames);
  }
  else {
    
    SetOverflowFrames(frames);
  }
}



void
nsTableFrame::AdjustForCollapsingRowsCols(nsHTMLReflowMetrics& aDesiredSize,
                                          nsMargin             aBorderPadding)
{
  nscoord yTotalOffset = 0; 

  
  
  SetNeedToCollapse(false);

  
  
  RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);

  nsTableFrame* firstInFlow = static_cast<nsTableFrame*>(FirstInFlow());
  nscoord width = firstInFlow->GetCollapsedWidth(aBorderPadding);
  nscoord rgWidth = width - GetColSpacing(-1) -
                    GetColSpacing(GetColCount());
  nsOverflowAreas overflow;
  
  for (uint32_t childX = 0; childX < rowGroups.Length(); childX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[childX];
    NS_ASSERTION(rgFrame, "Must have row group frame here");
    yTotalOffset += rgFrame->CollapseRowGroupIfNecessary(yTotalOffset, rgWidth);
    ConsiderChildOverflow(overflow, rgFrame);
  }

  aDesiredSize.Height() -= yTotalOffset;
  aDesiredSize.Width() = width;
  overflow.UnionAllWith(nsRect(0, 0, aDesiredSize.Width(), aDesiredSize.Height()));
  FinishAndStoreOverflow(overflow,
                         nsSize(aDesiredSize.Width(), aDesiredSize.Height()));
}


nscoord
nsTableFrame::GetCollapsedWidth(nsMargin aBorderPadding)
{
  NS_ASSERTION(!GetPrevInFlow(), "GetCollapsedWidth called on next in flow");
  nscoord width = GetColSpacing(GetColCount());
  width += aBorderPadding.left + aBorderPadding.right;
  for (nsIFrame* groupFrame = mColGroups.FirstChild(); groupFrame;
         groupFrame = groupFrame->GetNextSibling()) {
    const nsStyleVisibility* groupVis = groupFrame->StyleVisibility();
    bool collapseGroup = (NS_STYLE_VISIBILITY_COLLAPSE == groupVis->mVisible);
    nsTableColGroupFrame* cgFrame = (nsTableColGroupFrame*)groupFrame;
    for (nsTableColFrame* colFrame = cgFrame->GetFirstColumn(); colFrame;
         colFrame = colFrame->GetNextCol()) {
      const nsStyleDisplay* colDisplay = colFrame->StyleDisplay();
      int32_t colX = colFrame->GetColIndex();
      if (NS_STYLE_DISPLAY_TABLE_COLUMN == colDisplay->mDisplay) {
        const nsStyleVisibility* colVis = colFrame->StyleVisibility();
        bool collapseCol = (NS_STYLE_VISIBILITY_COLLAPSE == colVis->mVisible);
        int32_t colWidth = GetColumnWidth(colX);
        if (!collapseGroup && !collapseCol) {
          width += colWidth;
          if (ColumnHasCellSpacingBefore(colX))
            width += GetColSpacing(colX-1);
        }
        else {
          SetNeedToCollapse(true);
        }
      }
    }
  }
  return width;
}

 void
nsTableFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsContainerFrame::DidSetStyleContext(aOldStyleContext);

  if (!aOldStyleContext) 
    return;

  if (IsBorderCollapse() &&
      BCRecalcNeeded(aOldStyleContext, StyleContext())) {
    SetFullBCDamageArea();
  }

  
  if (!mTableLayoutStrategy || GetPrevInFlow())
    return;

  bool isAuto = IsAutoLayout();
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



void
nsTableFrame::AppendFrames(ChildListID     aListID,
                           nsFrameList&    aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList || aListID == kColGroupList,
               "unexpected child list");

  
  
  
  
  while (!aFrameList.IsEmpty()) {
    nsIFrame* f = aFrameList.FirstChild();
    aFrameList.RemoveFrame(f);

    
    const nsStyleDisplay* display = f->StyleDisplay();

    if (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP == display->mDisplay) {
      nsTableColGroupFrame* lastColGroup =
        nsTableColGroupFrame::GetLastRealColGroup(this);
      int32_t startColIndex = (lastColGroup)
        ? lastColGroup->GetStartColumnIndex() + lastColGroup->GetColCount() : 0;
      mColGroups.InsertFrame(nullptr, lastColGroup, f);
      
      InsertColGroups(startColIndex,
                      nsFrameList::Slice(mColGroups, f, f->GetNextSibling()));
    } else if (IsRowGroup(display->mDisplay)) {
      DrainSelfOverflowList(); 
      
      mFrames.AppendFrame(nullptr, f);

      
      InsertRowGroups(nsFrameList::Slice(mFrames, f, nullptr));
    } else {
      
      NS_NOTREACHED("How did we get here?  Frame construction screwed up");
      mFrames.AppendFrame(nullptr, f);
    }
  }

#ifdef DEBUG_TABLE_CELLMAP
  printf("=== TableFrame::AppendFrames\n");
  Dump(true, true, true);
#endif
  PresContext()->PresShell()->FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
  SetGeometryDirty();
}


struct ChildListInsertions {
  nsIFrame::ChildListID mID;
  nsFrameList mList;
};

void
nsTableFrame::InsertFrames(ChildListID     aListID,
                           nsIFrame*       aPrevFrame,
                           nsFrameList&    aFrameList)
{
  
  
  
  
  

  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  if ((aPrevFrame && !aPrevFrame->GetNextSibling()) ||
      (!aPrevFrame && GetChildList(aListID).IsEmpty())) {
    
    AppendFrames(aListID, aFrameList);
    return;
  }

  
  
  ChildListInsertions insertions[2]; 
  const nsStyleDisplay* display = aFrameList.FirstChild()->StyleDisplay();
  nsFrameList::FrameLinkEnumerator e(aFrameList);
  for (; !aFrameList.IsEmpty(); e.Next()) {
    nsIFrame* next = e.NextFrame();
    if (!next || next->StyleDisplay()->mDisplay != display->mDisplay) {
      nsFrameList head = aFrameList.ExtractHead(e);
      if (display->mDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP) {
        insertions[0].mID = kColGroupList;
        insertions[0].mList.AppendFrames(nullptr, head);
      } else {
        insertions[1].mID = kPrincipalList;
        insertions[1].mList.AppendFrames(nullptr, head);
      }
      if (!next) {
        break;
      }
      display = next->StyleDisplay();
    }
  }
  for (uint32_t i = 0; i < ArrayLength(insertions); ++i) {
    
    
    
    if (!insertions[i].mList.IsEmpty()) {
      HomogenousInsertFrames(insertions[i].mID, aPrevFrame,
                             insertions[i].mList);
    }
  }
}

void
nsTableFrame::HomogenousInsertFrames(ChildListID     aListID,
                                     nsIFrame*       aPrevFrame,
                                     nsFrameList&    aFrameList)
{
  
  const nsStyleDisplay* display = aFrameList.FirstChild()->StyleDisplay();
#ifdef DEBUG
  
  
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    const nsStyleDisplay* nextDisplay = e.get()->StyleDisplay();
    MOZ_ASSERT((display->mDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP) ==
               (nextDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP),
               "heterogenous childlist");
  }
#endif
  if (aPrevFrame) {
    const nsStyleDisplay* prevDisplay = aPrevFrame->StyleDisplay();
    
    if ((display->mDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP) !=
        (prevDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP)) {
      
      
      
      nsIFrame* pseudoFrame = aFrameList.FirstChild();
      nsIContent* parentContent = GetContent();
      nsIContent* content;
      aPrevFrame = nullptr;
      while (pseudoFrame  && (parentContent ==
                              (content = pseudoFrame->GetContent()))) {
        pseudoFrame = pseudoFrame->GetFirstPrincipalChild();
      }
      nsCOMPtr<nsIContent> container = content->GetParent();
      if (MOZ_LIKELY(container)) { 
        int32_t newIndex = container->IndexOf(content);
        nsIFrame* kidFrame;
        bool isColGroup = (NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP ==
                             display->mDisplay);
        nsTableColGroupFrame* lastColGroup;
        if (isColGroup) {
          kidFrame = mColGroups.FirstChild();
          lastColGroup = nsTableColGroupFrame::GetLastRealColGroup(this);
        }
        else {
          kidFrame = mFrames.FirstChild();
        }
        
        int32_t lastIndex = -1;
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
            pseudoFrame = pseudoFrame->GetFirstPrincipalChild();
          }
          int32_t index = container->IndexOf(content);
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
    NS_ASSERTION(aListID == kColGroupList, "unexpected child list");
    
    const nsFrameList::Slice& newColgroups =
      mColGroups.InsertFrames(nullptr, aPrevFrame, aFrameList);
    
    int32_t startColIndex = 0;
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
    NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");
    DrainSelfOverflowList(); 
    
    const nsFrameList::Slice& newRowGroups =
      mFrames.InsertFrames(nullptr, aPrevFrame, aFrameList);

    InsertRowGroups(newRowGroups);
  } else {
    NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");
    NS_NOTREACHED("How did we even get here?");
    
    mFrames.InsertFrames(nullptr, aPrevFrame, aFrameList);
    return;
  }

  PresContext()->PresShell()->FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
  SetGeometryDirty();
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== TableFrame::InsertFrames\n");
  Dump(true, true, true);
#endif
  return;
}

void
nsTableFrame::DoRemoveFrame(ChildListID     aListID,
                            nsIFrame*       aOldFrame)
{
  if (aListID == kColGroupList) {
    nsIFrame* nextColGroupFrame = aOldFrame->GetNextSibling();
    nsTableColGroupFrame* colGroup = (nsTableColGroupFrame*)aOldFrame;
    int32_t firstColIndex = colGroup->GetStartColumnIndex();
    int32_t lastColIndex  = firstColIndex + colGroup->GetColCount() - 1;
    mColGroups.DestroyFrame(aOldFrame);
    nsTableColGroupFrame::ResetColIndices(nextColGroupFrame, firstColIndex);
    
    int32_t colX;
    for (colX = lastColIndex; colX >= firstColIndex; colX--) {
      nsTableColFrame* colFrame = mColFrames.SafeElementAt(colX);
      if (colFrame) {
        RemoveCol(colGroup, colX, true, false);
      }
    }

    int32_t numAnonymousColsToAdd = GetColCount() - mColFrames.Length();
    if (numAnonymousColsToAdd > 0) {
      
      AppendAnonymousColFrames(numAnonymousColsToAdd);
    }

  } else {
    NS_ASSERTION(aListID == kPrincipalList, "unexpected child list");
    nsTableRowGroupFrame* rgFrame =
      static_cast<nsTableRowGroupFrame*>(aOldFrame);
    
    nsTableCellMap* cellMap = GetCellMap();
    if (cellMap) {
      cellMap->RemoveGroupCellMap(rgFrame);
    }

    
    mFrames.DestroyFrame(aOldFrame);

    
    if (cellMap) {
      cellMap->Synchronize(this);
      
      ResetRowIndices(nsFrameList::Slice(mFrames, nullptr, nullptr));
      nsIntRect damageArea;
      cellMap->RebuildConsideringCells(nullptr, nullptr, 0, 0, false, damageArea);

      static_cast<nsTableFrame*>(FirstInFlow())->MatchCellMapToColCache(cellMap);
    }
  }
}

void
nsTableFrame::RemoveFrame(ChildListID     aListID,
                          nsIFrame*       aOldFrame)
{
  NS_ASSERTION(aListID == kColGroupList ||
               NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP !=
                 aOldFrame->StyleDisplay()->mDisplay,
               "Wrong list name; use kColGroupList iff colgroup");
  nsIPresShell* shell = PresContext()->PresShell();
  nsTableFrame* lastParent = nullptr;
  while (aOldFrame) {
    nsIFrame* oldFrameNextContinuation = aOldFrame->GetNextContinuation();
    nsTableFrame* parent = static_cast<nsTableFrame*>(aOldFrame->GetParent());
    if (parent != lastParent) {
      parent->DrainSelfOverflowList();
    }
    parent->DoRemoveFrame(aListID, aOldFrame);
    aOldFrame = oldFrameNextContinuation;
    if (parent != lastParent) {
      
      
      if (parent->IsBorderCollapse()) {
        parent->SetFullBCDamageArea();
      }
      parent->SetGeometryDirty();
      shell->FrameNeedsReflow(parent, nsIPresShell::eTreeChange,
                              NS_FRAME_HAS_DIRTY_CHILDREN);
      lastParent = parent;
    }
  }
#ifdef DEBUG_TABLE_CELLMAP
  printf("=== TableFrame::RemoveFrame\n");
  Dump(true, true, true);
#endif
}

 nsMargin
nsTableFrame::GetUsedBorder() const
{
  if (!IsBorderCollapse())
    return nsContainerFrame::GetUsedBorder();

  return GetIncludedOuterBCBorder();
}

 nsMargin
nsTableFrame::GetUsedPadding() const
{
  if (!IsBorderCollapse())
    return nsContainerFrame::GetUsedPadding();

  return nsMargin(0,0,0,0);
}

 nsMargin
nsTableFrame::GetUsedMargin() const
{
  
  
  return nsMargin(0, 0, 0, 0);
}

NS_DECLARE_FRAME_PROPERTY(TableBCProperty, DeleteValue<BCPropertyData>)

BCPropertyData*
nsTableFrame::GetBCProperty(bool aCreateIfNecessary) const
{
  FrameProperties props = Properties();
  BCPropertyData* value = static_cast<BCPropertyData*>
                          (props.Get(TableBCProperty()));
  if (!value && aCreateIfNecessary) {
    value = new BCPropertyData();
    props.Set(TableBCProperty(), value);
  }

  return value;
}

static void
DivideBCBorderSize(BCPixelSize  aPixelSize,
                   BCPixelSize& aSmallHalf,
                   BCPixelSize& aLargeHalf)
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
  int32_t p2t = nsPresContext::AppUnitsPerCSSPixel();
  BCPropertyData* propData = GetBCProperty();
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
  int32_t p2t = nsPresContext::AppUnitsPerCSSPixel();
  BCPropertyData* propData = GetBCProperty();
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
  
  
  
  const nsStyleBorder* border = aStyleContext.StyleBorder();
  aBorderPadding = border->GetComputedBorder();
  if (aReflowState) {
    aBorderPadding += aReflowState->ComputedPhysicalPadding();
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
  nsMargin* pCollapseBorder = nullptr;
  nsPresContext* presContext = PresContext();
  if (IsBorderCollapse()) {
    nsTableRowGroupFrame* rgFrame =
       static_cast<nsTableRowGroupFrame*>(aReflowState.frame);
    pCollapseBorder = rgFrame->GetBCBorderWidth(collapseBorder);
  }
  aReflowState.Init(presContext, -1, -1, pCollapseBorder, &padding);

  NS_ASSERTION(!mBits.mResizedColumns ||
               !aReflowState.parentReflowState->mFlags.mSpecialHeightReflow,
               "should not resize columns on special height reflow");
  if (mBits.mResizedColumns) {
    aReflowState.SetHResize(true);
  }
}



void nsTableFrame::PlaceChild(nsTableReflowState&  aReflowState,
                              nsIFrame*            aKidFrame,
                              nsPoint              aKidPosition,
                              nsHTMLReflowMetrics& aKidDesiredSize,
                              const nsRect&        aOriginalKidRect,
                              const nsRect&        aOriginalKidVisualOverflow)
{
  bool isFirstReflow =
    (aKidFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;

  
  FinishReflowChild(aKidFrame, PresContext(), aKidDesiredSize, nullptr,
                    aKidPosition.x, aKidPosition.y, 0);

  InvalidateTableFrame(aKidFrame, aOriginalKidRect, aOriginalKidVisualOverflow,
                       isFirstReflow);

  
  aReflowState.y += aKidDesiredSize.Height();

  
  if (NS_UNCONSTRAINEDSIZE != aReflowState.availSize.height) {
    aReflowState.availSize.height -= aKidDesiredSize.Height();
  }
}

void
nsTableFrame::OrderRowGroups(RowGroupArray& aChildren,
                             nsTableRowGroupFrame** aHead,
                             nsTableRowGroupFrame** aFoot) const
{
  aChildren.Clear();
  nsTableRowGroupFrame* head = nullptr;
  nsTableRowGroupFrame* foot = nullptr;

  nsIFrame* kidFrame = mFrames.FirstChild();
  while (kidFrame) {
    const nsStyleDisplay* kidDisplay = kidFrame->StyleDisplay();
    nsTableRowGroupFrame* rowGroup =
      static_cast<nsTableRowGroupFrame*>(kidFrame);

    switch (kidDisplay->mDisplay) {
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
  if (aHead)
    *aHead = head;
  
  if (foot) {
    aChildren.AppendElement(foot);
  }
  if (aFoot)
    *aFoot = foot;
}

nsTableRowGroupFrame*
nsTableFrame::GetTHead() const
{
  nsIFrame* kidFrame = mFrames.FirstChild();
  while (kidFrame) {
    if (kidFrame->StyleDisplay()->mDisplay ==
          NS_STYLE_DISPLAY_TABLE_HEADER_GROUP) {
      return static_cast<nsTableRowGroupFrame*>(kidFrame);
    }

    
    
    while (kidFrame) {
      nsIFrame* nif = kidFrame->GetNextInFlow();
      kidFrame = kidFrame->GetNextSibling();
      if (kidFrame != nif)
        break;
    }
  }

  return nullptr;
}

nsTableRowGroupFrame*
nsTableFrame::GetTFoot() const
{
  nsIFrame* kidFrame = mFrames.FirstChild();
  while (kidFrame) {
    if (kidFrame->StyleDisplay()->mDisplay ==
          NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP) {
      return static_cast<nsTableRowGroupFrame*>(kidFrame);
    }

    
    
    while (kidFrame) {
      nsIFrame* nif = kidFrame->GetNextInFlow();
      kidFrame = kidFrame->GetNextSibling();
      if (kidFrame != nif)
        break;
    }
  }

  return nullptr;
}

static bool
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

  
  WritingMode wm = aFrame->GetWritingMode();
  LogicalSize availSize = aReflowState.reflowState.AvailableSize(wm);
  availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
  nsHTMLReflowState kidReflowState(presContext, aReflowState.reflowState,
                                   aFrame, availSize,
                                   -1, -1, nsHTMLReflowState::CALLER_WILL_INIT);
  InitChildReflowState(kidReflowState);
  kidReflowState.mFlags.mIsTopOfPage = true;
  nsHTMLReflowMetrics desiredSize(aReflowState.reflowState);
  desiredSize.ClearSize();
  nsReflowStatus status;
  ReflowChild(aFrame, presContext, desiredSize, kidReflowState,
              aReflowState.x, aReflowState.y, 0, status);
  

  aFrame->SetRepeatable(IsRepeatable(desiredSize.Height(), pageHeight));
  *aDesiredHeight = desiredSize.Height();
  return NS_OK;
}

void
nsTableFrame::PlaceRepeatedFooter(nsTableReflowState& aReflowState,
                                  nsTableRowGroupFrame *aTfoot,
                                  nscoord aFooterHeight)
{
  nsPresContext* presContext = PresContext();
  WritingMode wm = aTfoot->GetWritingMode();
  LogicalSize kidAvailSize(wm, aReflowState.availSize);
  kidAvailSize.BSize(wm) = aFooterHeight;
  nsHTMLReflowState footerReflowState(presContext,
                                      aReflowState.reflowState,
                                      aTfoot, kidAvailSize,
                                      -1, -1,
                                      nsHTMLReflowState::CALLER_WILL_INIT);
  InitChildReflowState(footerReflowState);
  aReflowState.y += GetRowSpacing(GetRowCount());

  nsRect origTfootRect = aTfoot->GetRect();
  nsRect origTfootVisualOverflow = aTfoot->GetVisualOverflowRect();

  nsReflowStatus footerStatus;
  nsHTMLReflowMetrics desiredSize(aReflowState.reflowState);
  desiredSize.ClearSize();
  ReflowChild(aTfoot, presContext, desiredSize, footerReflowState,
              aReflowState.x, aReflowState.y, 0, footerStatus);
  nsPoint kidPosition(aReflowState.x, aReflowState.y);
  footerReflowState.ApplyRelativePositioning(&kidPosition);

  PlaceChild(aReflowState, aTfoot, kidPosition, desiredSize, origTfootRect,
             origTfootVisualOverflow);
}



void
nsTableFrame::ReflowChildren(nsTableReflowState& aReflowState,
                             nsReflowStatus&     aStatus,
                             nsIFrame*&          aLastChildReflowed,
                             nsOverflowAreas&    aOverflowAreas)
{
  aStatus = NS_FRAME_COMPLETE;
  aLastChildReflowed = nullptr;

  nsIFrame* prevKidFrame = nullptr;

  nsPresContext* presContext = PresContext();
  
  
  
  
  bool isPaginated = presContext->IsPaginated() &&
                       NS_UNCONSTRAINEDSIZE != aReflowState.availSize.height &&
                       aReflowState.reflowState.mFlags.mTableIsSplittable;

  aOverflowAreas.Clear();

  bool reflowAllKids = aReflowState.reflowState.ShouldReflowAllKids() ||
                         mBits.mResizedColumns ||
                         IsGeometryDirty();

  RowGroupArray rowGroups;
  nsTableRowGroupFrame *thead, *tfoot;
  OrderRowGroups(rowGroups, &thead, &tfoot);
  bool pageBreak = false;
  nscoord footerHeight = 0;

  
  
  
  
  
  
  
  
  if (isPaginated) {
    if (thead && !GetPrevInFlow()) {
      nscoord desiredHeight;
      nsresult rv = SetupHeaderFooterChild(aReflowState, thead, &desiredHeight);
      if (NS_FAILED(rv))
        return;
    }
    if (tfoot) {
      nsresult rv = SetupHeaderFooterChild(aReflowState, tfoot, &footerHeight);
      if (NS_FAILED(rv))
        return;
    }
  }
   
  bool allowRepeatedFooter = false;
  for (size_t childX = 0; childX < rowGroups.Length(); childX++) {
    nsIFrame* kidFrame = rowGroups[childX];
    nsTableRowGroupFrame* rowGroupFrame = rowGroups[childX];
    nscoord cellSpacingY = GetRowSpacing(rowGroupFrame->GetStartRowIndex()+
                                         rowGroupFrame->GetRowCount());
    
    
    if (reflowAllKids ||
        NS_SUBTREE_DIRTY(kidFrame) ||
        (aReflowState.reflowState.mFlags.mSpecialHeightReflow &&
         (isPaginated || (kidFrame->GetStateBits() &
                          NS_FRAME_CONTAINS_RELATIVE_HEIGHT)))) {
      if (pageBreak) {
        if (allowRepeatedFooter) {
          PlaceRepeatedFooter(aReflowState, tfoot, footerHeight);
        }
        else if (tfoot && tfoot->IsRepeatable()) {
          tfoot->SetRepeatable(false);
        }
        PushChildren(rowGroups, childX);
        aStatus = NS_FRAME_NOT_COMPLETE;
        break;
      }

      nsSize kidAvailSize(aReflowState.availSize);
      allowRepeatedFooter = false;
      if (isPaginated && (NS_UNCONSTRAINEDSIZE != kidAvailSize.height)) {
        nsTableRowGroupFrame* kidRG =
          static_cast<nsTableRowGroupFrame*>(kidFrame);
        if (kidRG != thead && kidRG != tfoot && tfoot && tfoot->IsRepeatable()) {
          
          NS_ASSERTION(tfoot == rowGroups[rowGroups.Length() - 1], "Missing footer!");
          if (footerHeight + cellSpacingY < kidAvailSize.height) {
            allowRepeatedFooter = true;
            kidAvailSize.height -= footerHeight + cellSpacingY;
          }
        }
      }

      nsRect oldKidRect = kidFrame->GetRect();
      nsRect oldKidVisualOverflow = kidFrame->GetVisualOverflowRect();

      nsHTMLReflowMetrics desiredSize(aReflowState.reflowState);
      desiredSize.ClearSize();

      
      nsHTMLReflowState kidReflowState(presContext, aReflowState.reflowState,
                                       kidFrame,
                                       LogicalSize(kidFrame->GetWritingMode(),
                                                   kidAvailSize),
                                       -1, -1,
                                       nsHTMLReflowState::CALLER_WILL_INIT);
      InitChildReflowState(kidReflowState);

      
      
      
      
      if (childX > ((thead && IsRepeatedFrame(thead)) ? 1u : 0u) &&
          (rowGroups[childX - 1]->GetNormalRect().YMost() > 0)) {
        kidReflowState.mFlags.mIsTopOfPage = false;
      }
      aReflowState.y += cellSpacingY;
      if (NS_UNCONSTRAINEDSIZE != aReflowState.availSize.height) {
        aReflowState.availSize.height -= cellSpacingY;
      }
      
      
      bool reorder = false;
      if (kidFrame->GetNextInFlow())
        reorder = true;

      ReflowChild(kidFrame, presContext, desiredSize, kidReflowState,
                  aReflowState.x, aReflowState.y, 0, aStatus);
      nsPoint kidPosition(aReflowState.x, aReflowState.y);
      kidReflowState.ApplyRelativePositioning(&kidPosition);

      if (reorder) {
        
        OrderRowGroups(rowGroups, &thead, &tfoot);
        childX = rowGroups.IndexOf(kidFrame);
        if (childX == RowGroupArray::NoIndex) {
          
          childX = rowGroups.Length();
        }
      }
      if (isPaginated && !NS_FRAME_IS_FULLY_COMPLETE(aStatus) &&
          ShouldAvoidBreakInside(aReflowState.reflowState)) {
        aStatus = NS_INLINE_LINE_BREAK_BEFORE();
        break;
      }
      
      
      if (isPaginated &&
          (NS_INLINE_IS_BREAK_BEFORE(aStatus) ||
           (NS_FRAME_IS_COMPLETE(aStatus) &&
            (NS_UNCONSTRAINEDSIZE != kidReflowState.AvailableHeight()) &&
            kidReflowState.AvailableHeight() < desiredSize.Height()))) {
        if (ShouldAvoidBreakInside(aReflowState.reflowState)) {
          aStatus = NS_INLINE_LINE_BREAK_BEFORE();
          break;
        }
        
        if (kidReflowState.mFlags.mIsTopOfPage) {
          if (childX+1 < rowGroups.Length()) {
            nsIFrame* nextRowGroupFrame = rowGroups[childX + 1];
            if (nextRowGroupFrame) {
              PlaceChild(aReflowState, kidFrame, kidPosition, desiredSize,
                         oldKidRect, oldKidVisualOverflow);
              if (allowRepeatedFooter) {
                PlaceRepeatedFooter(aReflowState, tfoot, footerHeight);
              }
              else if (tfoot && tfoot->IsRepeatable()) {
                tfoot->SetRepeatable(false);
              }
              aStatus = NS_FRAME_NOT_COMPLETE;
              PushChildren(rowGroups, childX + 1);
              aLastChildReflowed = kidFrame;
              break;
            }
          }
        }
        else { 
          if (prevKidFrame) { 
            if (allowRepeatedFooter) {
              PlaceRepeatedFooter(aReflowState, tfoot, footerHeight);
            }
            else if (tfoot && tfoot->IsRepeatable()) {
              tfoot->SetRepeatable(false);
            }
            aStatus = NS_FRAME_NOT_COMPLETE;
            PushChildren(rowGroups, childX);
            aLastChildReflowed = prevKidFrame;
            break;
          }
          else { 
            PlaceChild(aReflowState, kidFrame, kidPosition, desiredSize,
                       oldKidRect, oldKidVisualOverflow);
            aLastChildReflowed = kidFrame;
            if (allowRepeatedFooter) {
              PlaceRepeatedFooter(aReflowState, tfoot, footerHeight);
              aLastChildReflowed = tfoot;
            }
            break;
          }
        }
      }

      aLastChildReflowed   = kidFrame;

      pageBreak = false;
      
      if (NS_FRAME_IS_COMPLETE(aStatus) && isPaginated &&
          (NS_UNCONSTRAINEDSIZE != kidReflowState.AvailableHeight())) {
        nsIFrame* nextKid =
          (childX + 1 < rowGroups.Length()) ? rowGroups[childX + 1] : nullptr;
        pageBreak = PageBreakAfter(kidFrame, nextKid);
      }

      
      PlaceChild(aReflowState, kidFrame, kidPosition, desiredSize, oldKidRect,
                 oldKidVisualOverflow);

      
      prevKidFrame = kidFrame;

      
      if (NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
        nsIFrame* kidNextInFlow = kidFrame->GetNextInFlow();
        if (!kidNextInFlow) {
          
          
          kidNextInFlow = presContext->PresShell()->FrameConstructor()->
            CreateContinuingFrame(presContext, kidFrame, this);

          
          mFrames.InsertFrame(nullptr, kidFrame, kidNextInFlow);
          
          rowGroups.InsertElementAt(childX + 1,
                      static_cast<nsTableRowGroupFrame*>(kidNextInFlow));
        } else if (kidNextInFlow == kidFrame->GetNextSibling()) {
          
          
          MOZ_ASSERT(!rowGroups.Contains(kidNextInFlow),
                     "OrderRowGroups must not put our NIF in 'rowGroups'");
          rowGroups.InsertElementAt(childX + 1,
                      static_cast<nsTableRowGroupFrame*>(kidNextInFlow));
        }

        
        
        if (allowRepeatedFooter) {
          PlaceRepeatedFooter(aReflowState, tfoot, footerHeight);
        }
        else if (tfoot && tfoot->IsRepeatable()) {
          tfoot->SetRepeatable(false);
        }

        nsIFrame* nextSibling = kidFrame->GetNextSibling();
        if (nextSibling) {
          PushChildren(rowGroups, childX + 1);
        }
        break;
      }
    }
    else { 
      aReflowState.y += cellSpacingY;
      nsRect kidRect = kidFrame->GetNormalRect();
      if (kidRect.y != aReflowState.y) {
        
        kidFrame->InvalidateFrameSubtree();
        
        kidFrame->MovePositionBy(nsPoint(0, aReflowState.y - kidRect.y));
        RePositionViews(kidFrame);
        
        kidFrame->InvalidateFrameSubtree();
      }
      aReflowState.y += kidRect.height;

      
      if (NS_UNCONSTRAINEDSIZE != aReflowState.availSize.height) {
        aReflowState.availSize.height -= cellSpacingY + kidRect.height;
      }
    }
    ConsiderChildOverflow(aOverflowAreas, kidFrame);
  }

  
  
  mBits.mResizedColumns = false;
  ClearGeometryDirty();
}

void
nsTableFrame::ReflowColGroups(nsRenderingContext *aRenderingContext)
{
  if (!GetPrevInFlow() && !HaveReflowedColGroups()) {
    nsHTMLReflowMetrics kidMet(GetWritingMode());
    nsPresContext *presContext = PresContext();
    for (nsIFrame* kidFrame = mColGroups.FirstChild(); kidFrame;
         kidFrame = kidFrame->GetNextSibling()) {
      if (NS_SUBTREE_DIRTY(kidFrame)) {
        
        nsHTMLReflowState
          kidReflowState(presContext, kidFrame, aRenderingContext,
                         LogicalSize(kidFrame->GetWritingMode()));
        nsReflowStatus cgStatus;
        ReflowChild(kidFrame, presContext, kidMet, kidReflowState, 0, 0, 0,
                    cgStatus);
        FinishReflowChild(kidFrame, presContext, kidMet, nullptr, 0, 0, 0);
      }
    }
    SetHaveReflowedColGroups(true);
  }
}

void
nsTableFrame::CalcDesiredHeight(const nsHTMLReflowState& aReflowState, nsHTMLReflowMetrics& aDesiredSize)
{
  nsTableCellMap* cellMap = GetCellMap();
  if (!cellMap) {
    NS_ASSERTION(false, "never ever call me until the cell map is built!");
    aDesiredSize.Height() = 0;
    return;
  }
  nsMargin borderPadding = GetChildAreaOffset(&aReflowState);

  
  RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);
  if (rowGroups.IsEmpty()) {
    
    nscoord tableSpecifiedHeight = CalcBorderBoxHeight(aReflowState);
    if ((NS_UNCONSTRAINEDSIZE != tableSpecifiedHeight) &&
        (tableSpecifiedHeight > 0) &&
        eCompatibility_NavQuirks != PresContext()->CompatibilityMode()) {
          
      aDesiredSize.Height() = tableSpecifiedHeight;
    }
    else
      aDesiredSize.Height() = 0;
    return;
  }
  int32_t rowCount = cellMap->GetRowCount();
  int32_t colCount = cellMap->GetColCount();
  nscoord desiredHeight = borderPadding.top + borderPadding.bottom;
  if (rowCount > 0 && colCount > 0) {
    desiredHeight += GetRowSpacing(-1);
    for (uint32_t rgX = 0; rgX < rowGroups.Length(); rgX++) {
      desiredHeight += rowGroups[rgX]->GetSize().height +
                       GetRowSpacing(rowGroups[rgX]->GetRowCount() +
                                     rowGroups[rgX]->GetStartRowIndex());
    }
  }

  
  if (!GetPrevInFlow()) {
    nscoord tableSpecifiedHeight = CalcBorderBoxHeight(aReflowState);
    if ((tableSpecifiedHeight > 0) &&
        (tableSpecifiedHeight != NS_UNCONSTRAINEDSIZE) &&
        (tableSpecifiedHeight > desiredHeight)) {
      
      
      DistributeHeightToRows(aReflowState, tableSpecifiedHeight - desiredHeight);
      
      for (nsIFrame* kidFrame = mFrames.FirstChild(); kidFrame; kidFrame = kidFrame->GetNextSibling()) {
        ConsiderChildOverflow(aDesiredSize.mOverflowAreas, kidFrame);
      }
      desiredHeight = tableSpecifiedHeight;
    }
  }
  aDesiredSize.Height() = desiredHeight;
}

static
void ResizeCells(nsTableFrame& aTableFrame)
{
  nsTableFrame::RowGroupArray rowGroups;
  aTableFrame.OrderRowGroups(rowGroups);
  WritingMode wm = aTableFrame.GetWritingMode();
  nsHTMLReflowMetrics tableDesiredSize(wm);
  tableDesiredSize.SetSize(wm, aTableFrame.GetLogicalSize(wm));
  tableDesiredSize.SetOverflowAreasToDesiredBounds();

  for (uint32_t rgX = 0; rgX < rowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];

    nsHTMLReflowMetrics groupDesiredSize(wm);
    groupDesiredSize.SetSize(wm, rgFrame->GetLogicalSize(wm));
    groupDesiredSize.SetOverflowAreasToDesiredBounds();

    nsTableRowFrame* rowFrame = rgFrame->GetFirstRow();
    while (rowFrame) {
      rowFrame->DidResize();
      rgFrame->ConsiderChildOverflow(groupDesiredSize.mOverflowAreas, rowFrame);
      rowFrame = rowFrame->GetNextRow();
    }
    rgFrame->FinishAndStoreOverflow(&groupDesiredSize);
    tableDesiredSize.mOverflowAreas.UnionWith(groupDesiredSize.mOverflowAreas +
                                              rgFrame->GetPosition());
  }
  aTableFrame.FinishAndStoreOverflow(&tableDesiredSize);
}

void
nsTableFrame::DistributeHeightToRows(const nsHTMLReflowState& aReflowState,
                                     nscoord                  aAmount)
{
  nsMargin borderPadding = GetChildAreaOffset(&aReflowState);

  RowGroupArray rowGroups;
  OrderRowGroups(rowGroups);

  nscoord amountUsed = 0;
  
  
  
  nscoord pctBasis = aReflowState.ComputedHeight() - GetRowSpacing(-1, GetRowCount());
  nscoord yOriginRG = borderPadding.top + GetRowSpacing(0);
  nscoord yEndRG = yOriginRG;
  uint32_t rgX;
  for (rgX = 0; rgX < rowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
    nscoord amountUsedByRG = 0;
    nscoord yOriginRow = 0;
    nsRect rgNormalRect = rgFrame->GetNormalRect();
    if (!rgFrame->HasStyleHeight()) {
      nsTableRowFrame* rowFrame = rgFrame->GetFirstRow();
      while (rowFrame) {
        nsRect rowNormalRect = rowFrame->GetNormalRect();
        nscoord cellSpacingY = GetRowSpacing(rowFrame->GetRowIndex());
        if ((amountUsed < aAmount) && rowFrame->HasPctHeight()) {
          nscoord pctHeight = rowFrame->GetHeight(pctBasis);
          nscoord amountForRow = std::min(aAmount - amountUsed,
                                          pctHeight - rowNormalRect.height);
          if (amountForRow > 0) {
            
            nsRect origRowRect = rowFrame->GetRect();
            nscoord newRowHeight = rowNormalRect.height + amountForRow;
            rowFrame->SetSize(nsSize(rowNormalRect.width, newRowHeight));
            yOriginRow += newRowHeight + cellSpacingY;
            yEndRG += newRowHeight + cellSpacingY;
            amountUsed += amountForRow;
            amountUsedByRG += amountForRow;
            
            nsTableFrame::RePositionViews(rowFrame);

            rgFrame->InvalidateFrameWithRect(origRowRect);
            rgFrame->InvalidateFrame();
          }
        }
        else {
          if (amountUsed > 0 && yOriginRow != rowNormalRect.y &&
              !(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
            rowFrame->InvalidateFrameSubtree();
            rowFrame->MovePositionBy(nsPoint(0, yOriginRow - rowNormalRect.y));
            nsTableFrame::RePositionViews(rowFrame);
            rowFrame->InvalidateFrameSubtree();
          }
          yOriginRow += rowNormalRect.height + cellSpacingY;
          yEndRG += rowNormalRect.height + cellSpacingY;
        }
        rowFrame = rowFrame->GetNextRow();
      }
      if (amountUsed > 0) {
        if (rgNormalRect.y != yOriginRG) {
          rgFrame->InvalidateFrameSubtree();
        }

        nsRect origRgNormalRect = rgFrame->GetRect();
        nsRect origRgVisualOverflow = rgFrame->GetVisualOverflowRect();

        rgFrame->MovePositionBy(nsPoint(0, yOriginRG - rgNormalRect.y));
        rgFrame->SetSize(nsSize(rgNormalRect.width,
                                rgNormalRect.height + amountUsedByRG));

        nsTableFrame::InvalidateTableFrame(rgFrame, origRgNormalRect,
                                           origRgVisualOverflow, false);
      }
    }
    else if (amountUsed > 0 && yOriginRG != rgNormalRect.y) {
      rgFrame->InvalidateFrameSubtree();
      rgFrame->MovePositionBy(nsPoint(0, yOriginRG - rgNormalRect.y));
      
      nsTableFrame::RePositionViews(rgFrame);
      rgFrame->InvalidateFrameSubtree();
    }
    yOriginRG = yEndRG;
  }

  if (amountUsed >= aAmount) {
    ResizeCells(*this);
    return;
  }

  
  
  nsTableRowGroupFrame* firstUnStyledRG  = nullptr;
  nsTableRowFrame*      firstUnStyledRow = nullptr;
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

  nsTableRowFrame* lastEligibleRow = nullptr;
  
  
  
  
  nscoord divisor = 0;
  int32_t eligibleRows = 0;
  bool expandEmptyRows = false;

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
        expandEmptyRows = true;
      }
      else {
        NS_ERROR("invalid divisor");
        return;
      }
    }
  }
  
  nscoord heightToDistribute = aAmount - amountUsed;
  yOriginRG = borderPadding.top + GetRowSpacing(-1);
  yEndRG = yOriginRG;
  for (rgX = 0; rgX < rowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
    nscoord amountUsedByRG = 0;
    nscoord yOriginRow = 0;
    nsRect rgNormalRect = rgFrame->GetNormalRect();
    nsRect rgVisualOverflow = rgFrame->GetVisualOverflowRect();
    
    if (!firstUnStyledRG || !rgFrame->HasStyleHeight() || !eligibleRows) {
      nsTableRowFrame* rowFrame = rgFrame->GetFirstRow();
      while (rowFrame) {
        nscoord cellSpacingY = GetRowSpacing(rowFrame->GetRowIndex());
        nsRect rowNormalRect = rowFrame->GetNormalRect();
        nsRect rowVisualOverflow = rowFrame->GetVisualOverflowRect();
        
        if (!firstUnStyledRow || !rowFrame->HasStyleHeight() || !eligibleRows) {
          float ratio;
          if (eligibleRows) {
            if (!expandEmptyRows) {
              
              
              ratio = float(rowNormalRect.height) / float(divisor);
            } else {
              
              ratio = 1.0f / float(eligibleRows);
            }
          }
          else {
            
            ratio = 1.0f / float(divisor);
          }
          
          
          nscoord amountForRow = (rowFrame == lastEligibleRow)
                                 ? aAmount - amountUsed : NSToCoordRound(((float)(heightToDistribute)) * ratio);
          amountForRow = std::min(amountForRow, aAmount - amountUsed);

          if (yOriginRow != rowNormalRect.y) {
            rowFrame->InvalidateFrameSubtree();
          }

          
          nsRect origRowRect = rowFrame->GetRect();
          nscoord newRowHeight = rowNormalRect.height + amountForRow;
          rowFrame->MovePositionBy(nsPoint(0, yOriginRow - rowNormalRect.y));
          rowFrame->SetSize(nsSize(rowNormalRect.width, newRowHeight));

          yOriginRow += newRowHeight + cellSpacingY;
          yEndRG += newRowHeight + cellSpacingY;

          amountUsed += amountForRow;
          amountUsedByRG += amountForRow;
          NS_ASSERTION((amountUsed <= aAmount), "invalid row allocation");
          
          nsTableFrame::RePositionViews(rowFrame);

          nsTableFrame::InvalidateTableFrame(rowFrame, origRowRect,
                                             rowVisualOverflow, false);
        }
        else {
          if (amountUsed > 0 && yOriginRow != rowNormalRect.y) {
            rowFrame->InvalidateFrameSubtree();
            rowFrame->MovePositionBy(nsPoint(0, yOriginRow - rowNormalRect.y));
            nsTableFrame::RePositionViews(rowFrame);
            rowFrame->InvalidateFrameSubtree();
          }
          yOriginRow += rowNormalRect.height + cellSpacingY;
          yEndRG += rowNormalRect.height + cellSpacingY;
        }
        rowFrame = rowFrame->GetNextRow();
      }
      if (amountUsed > 0) {
        if (rgNormalRect.y != yOriginRG) {
          rgFrame->InvalidateFrameSubtree();
        }

        nsRect origRgNormalRect = rgFrame->GetRect();
        rgFrame->MovePositionBy(nsPoint(0, yOriginRG - rgNormalRect.y));
        rgFrame->SetSize(nsSize(rgNormalRect.width,
                                rgNormalRect.height + amountUsedByRG));

        nsTableFrame::InvalidateTableFrame(rgFrame, origRgNormalRect,
                                           rgVisualOverflow, false);
      }
      
    }
    else if (amountUsed > 0 && yOriginRG != rgNormalRect.y) {
      rgFrame->InvalidateFrameSubtree();
      rgFrame->MovePositionBy(nsPoint(0, yOriginRG - rgNormalRect.y));
      
      nsTableFrame::RePositionViews(rgFrame);
      rgFrame->InvalidateFrameSubtree();
    }
    yOriginRG = yEndRG;
  }

  ResizeCells(*this);
}

int32_t nsTableFrame::GetColumnWidth(int32_t aColIndex)
{
  nsTableFrame* firstInFlow = static_cast<nsTableFrame*>(FirstInFlow());
  if (this == firstInFlow) {
    nsTableColFrame* colFrame = GetColFrame(aColIndex);
    return colFrame ? colFrame->GetFinalWidth() : 0;
  }
  return firstInFlow->GetColumnWidth(aColIndex);
}

nscoord nsTableFrame::GetColSpacing()
{
  if (IsBorderCollapse())
    return 0;

  return StyleTableBorder()->mBorderSpacingCol;
}


nscoord nsTableFrame::GetColSpacing(int32_t aColIndex)
{
  NS_ASSERTION(aColIndex >= -1 && aColIndex <= GetColCount(),
               "Column index exceeds the bounds of the table");
  
  
  
  return GetColSpacing();
}

nscoord nsTableFrame::GetColSpacing(int32_t aStartColIndex,
                                      int32_t aEndColIndex)
{
  NS_ASSERTION(aStartColIndex >= -1 && aStartColIndex <= GetColCount(),
               "Start column index exceeds the bounds of the table");
  NS_ASSERTION(aEndColIndex >= -1 && aEndColIndex <= GetColCount(),
               "End column index exceeds the bounds of the table");
  NS_ASSERTION(aStartColIndex <= aEndColIndex,
               "End index must not be less than start index");
  
  
  return GetColSpacing() * (aEndColIndex - aStartColIndex);
}

nscoord nsTableFrame::GetRowSpacing()
{
  if (IsBorderCollapse())
    return 0;

  return StyleTableBorder()->mBorderSpacingRow;
}


nscoord nsTableFrame::GetRowSpacing(int32_t aRowIndex)
{
  NS_ASSERTION(aRowIndex >= -1 && aRowIndex <= GetRowCount(),
               "Row index exceeds the bounds of the table");
  
  
  
  return GetRowSpacing();
}

nscoord nsTableFrame::GetRowSpacing(int32_t aStartRowIndex,
                                      int32_t aEndRowIndex)
{
  NS_ASSERTION(aStartRowIndex >= -1 && aStartRowIndex <= GetRowCount(),
               "Start row index exceeds the bounds of the table");
  NS_ASSERTION(aEndRowIndex >= -1 && aEndRowIndex <= GetRowCount(),
               "End row index exceeds the bounds of the table");
  NS_ASSERTION(aStartRowIndex <= aEndRowIndex,
               "End index must not be less than start index");
  
  
  return GetRowSpacing() * (aEndRowIndex - aStartRowIndex);
}

 nscoord
nsTableFrame::GetLogicalBaseline(WritingMode aWritingMode) const
{
  nscoord ascent = 0;
  RowGroupArray orderedRowGroups;
  OrderRowGroups(orderedRowGroups);
  nsTableRowFrame* firstRow = nullptr;
  
  nscoord containerWidth = mRect.width;
  for (uint32_t rgIndex = 0; rgIndex < orderedRowGroups.Length(); rgIndex++) {
    nsTableRowGroupFrame* rgFrame = orderedRowGroups[rgIndex];
    if (rgFrame->GetRowCount()) {
      firstRow = rgFrame->GetFirstRow();

      nscoord rgNormalBStart =
        LogicalRect(aWritingMode, rgFrame->GetNormalRect(), containerWidth)
        .Origin(aWritingMode).B(aWritingMode);
      nscoord firstRowNormalBStart =
        LogicalRect(aWritingMode, firstRow->GetNormalRect(), containerWidth)
        .Origin(aWritingMode).B(aWritingMode);

      ascent = rgNormalBStart + firstRowNormalBStart +
               firstRow->GetRowBaseline(aWritingMode);
      break;
    }
  }
  if (!firstRow)
    ascent = BSize(aWritingMode);
  return ascent;
}


nsTableFrame*
NS_NewTableFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTableFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTableFrame)

nsTableFrame*
nsTableFrame::GetTableFrame(nsIFrame* aFrame)
{
  for (nsIFrame* ancestor = aFrame->GetParent(); ancestor;
       ancestor = ancestor->GetParent()) {
    if (nsGkAtoms::tableFrame == ancestor->GetType()) {
      return static_cast<nsTableFrame*>(ancestor);
    }
  }
  NS_RUNTIMEABORT("unable to find table parent");
  return nullptr;
}

nsTableFrame*
nsTableFrame::GetTableFramePassingThrough(nsIFrame* aMustPassThrough,
                                          nsIFrame* aFrame)
{
  MOZ_ASSERT(aMustPassThrough == aFrame ||
             nsLayoutUtils::IsProperAncestorFrame(aMustPassThrough, aFrame),
             "aMustPassThrough should be an ancestor");

  
  
  bool hitPassThroughFrame = false;
  nsTableFrame* tableFrame = nullptr;
  for (nsIFrame* ancestor = aFrame; ancestor; ancestor = ancestor->GetParent()) {
    if (ancestor == aMustPassThrough) {
      hitPassThroughFrame = true;
    }
    if (nsGkAtoms::tableFrame == ancestor->GetType()) {
      tableFrame = static_cast<nsTableFrame*>(ancestor);
      break;
    }
  }

  MOZ_ASSERT(tableFrame, "Should have a table frame here");
  return hitPassThroughFrame ? tableFrame : nullptr;
}

bool
nsTableFrame::IsAutoHeight()
{
  const nsStyleCoord &height = StylePosition()->mHeight;
  
  return height.GetUnit() == eStyleUnit_Auto ||
         (height.GetUnit() == eStyleUnit_Percent &&
          height.GetPercentValue() <= 0.0f);
}

nscoord
nsTableFrame::CalcBorderBoxHeight(const nsHTMLReflowState& aState)
{
  nscoord height = aState.ComputedHeight();
  if (NS_AUTOHEIGHT != height) {
    nsMargin borderPadding = GetChildAreaOffset(&aState);
    height += borderPadding.top + borderPadding.bottom;
  }
  height = std::max(0, height);

  return height;
}

bool
nsTableFrame::IsAutoLayout()
{
  if (StyleTable()->mLayoutStrategy == NS_STYLE_TABLE_LAYOUT_AUTO)
    return true;
  
  
  
  
  const nsStyleCoord &width = StylePosition()->mWidth;
  return (width.GetUnit() == eStyleUnit_Auto) ||
         (width.GetUnit() == eStyleUnit_Enumerated &&
          width.GetIntValue() == NS_STYLE_WIDTH_MAX_CONTENT);
}

#ifdef DEBUG_FRAME_DUMP
nsresult
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
  nsIFrame* result = nullptr;
  if (!aPriorChildFrame) {
    return result;
  }
  if (aChildType == aPriorChildFrame->GetType()) {
    return aPriorChildFrame;
  }

  
  
  nsIFrame* lastMatchingFrame = nullptr;
  nsIFrame* childFrame = aParentFrame->GetFirstPrincipalChild();
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
  if (!aKidFrame)
    return;

  nsIFrame* cFrame = aKidFrame->GetFirstPrincipalChild();
  while (cFrame) {
    nsTableRowFrame *rowFrame = do_QueryFrame(cFrame);
    if (rowFrame) {
      printf("row(%d)=%p ", rowFrame->GetRowIndex(),
             static_cast<void*>(rowFrame));
      nsIFrame* childFrame = cFrame->GetFirstPrincipalChild();
      while (childFrame) {
        nsTableCellFrame *cellFrame = do_QueryFrame(childFrame);
        if (cellFrame) {
          int32_t colIndex;
          cellFrame->GetColIndex(colIndex);
          printf("cell(%d)=%p ", colIndex, static_cast<void*>(childFrame));
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

void
nsTableFrame::Dump(bool            aDumpRows,
                   bool            aDumpCols,
                   bool            aDumpCellMap)
{
  printf("***START TABLE DUMP*** \n");
  
  printf("mColWidths=");
  int32_t numCols = GetColCount();
  int32_t colX;
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
      printf ("%d=%p ", colX, static_cast<void*>(colFrame));
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
  nsIFrame* firstChild = aSource.GetFirstPrincipalChild();
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
  mCurrentChild   = nullptr;
  mLeftToRight    = true;
  mCount          = -1;

  if (!mFirstChild) {
    return;
  }

  nsTableFrame* table = nsTableFrame::GetTableFrame(mFirstChild);
  mLeftToRight = (NS_STYLE_DIRECTION_LTR ==
                  table->StyleVisibility()->mDirection);

  if (!mLeftToRight) {
    mCount = 0;
    nsIFrame* nextChild = mFirstChild->GetNextSibling();
    while (nullptr != nextChild) {
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
    return nullptr;
  }

  if (mLeftToRight) {
    mCurrentChild = mCurrentChild->GetNextSibling();
    return mCurrentChild;
  }
  else {
    nsIFrame* targetChild = mCurrentChild;
    mCurrentChild = nullptr;
    nsIFrame* child = mFirstListChild;
    while (child && (child != targetChild)) {
      mCurrentChild = child;
      child = child->GetNextSibling();
    }
    return mCurrentChild;
  }
}

bool nsTableIterator::IsLeftToRight()
{
  return mLeftToRight;
}

int32_t nsTableIterator::Count()
{
  if (-1 == mCount) {
    mCount = 0;
    nsIFrame* child = mFirstListChild;
    while (nullptr != child) {
      mCount++;
      child = child->GetNextSibling();
    }
  }
  return mCount;
}

bool
nsTableFrame::ColumnHasCellSpacingBefore(int32_t aColIndex) const
{
  
  
  if (LayoutStrategy()->GetType() == nsITableLayoutStrategy::Fixed)
    return true;
  
  if (aColIndex == 0)
    return true;
  nsTableCellMap* cellMap = GetCellMap();
  if (!cellMap)
    return false;
  return cellMap->GetNumCellsOriginatingInCol(aColIndex) > 0;
}














#ifdef DEBUG
#define VerifyNonNegativeDamageRect(r)                                  \
  NS_ASSERTION((r).x >= 0, "negative col index");                       \
  NS_ASSERTION((r).y >= 0, "negative row index");                       \
  NS_ASSERTION((r).width >= 0, "negative horizontal damage");           \
  NS_ASSERTION((r).height >= 0, "negative vertical damage");
#define VerifyDamageRect(r)                                             \
  VerifyNonNegativeDamageRect(r);                                       \
  NS_ASSERTION((r).XMost() <= GetColCount(),                            \
               "horizontal damage extends outside table");              \
  NS_ASSERTION((r).YMost() <= GetRowCount(),                            \
               "vertical damage extends outside table");
#endif

void
nsTableFrame::AddBCDamageArea(const nsIntRect& aValue)
{
  NS_ASSERTION(IsBorderCollapse(), "invalid AddBCDamageArea call");
#ifdef DEBUG
  VerifyDamageRect(aValue);
#endif

  SetNeedToCalcBCBorders(true);
  
  BCPropertyData* value = GetBCProperty(true);
  if (value) {
#ifdef DEBUG
    VerifyNonNegativeDamageRect(value->mDamageArea);
#endif
    
    int32_t cols = GetColCount();
    if (value->mDamageArea.XMost() > cols) {
      if (value->mDamageArea.x > cols) {
        value->mDamageArea.x = cols;
        value->mDamageArea.width = 0;
      }
      else {
        value->mDamageArea.width = cols - value->mDamageArea.x;
      }
    }
    int32_t rows = GetRowCount();
    if (value->mDamageArea.YMost() > rows) {
      if (value->mDamageArea.y > rows) {
        value->mDamageArea.y = rows;
        value->mDamageArea.height = 0;
      }
      else {
        value->mDamageArea.height = rows - value->mDamageArea.y;
      }
    }

    
    value->mDamageArea.UnionRect(value->mDamageArea, aValue);
  }
}


void
nsTableFrame::SetFullBCDamageArea()
{
  NS_ASSERTION(IsBorderCollapse(), "invalid SetFullBCDamageArea call");

  SetNeedToCalcBCBorders(true);

  BCPropertyData* value = GetBCProperty(true);
  if (value) {
    value->mDamageArea = nsIntRect(0, 0, GetColCount(), GetRowCount());
  }
}














struct BCCellBorder
{
  BCCellBorder() { Reset(0, 1); }
  void Reset(uint32_t aRowIndex, uint32_t aRowSpan);
  nscolor       color;    
  BCPixelSize   width;    
  uint8_t       style;    
                          
  BCBorderOwner owner;    
                          
                          
                          
                          
  int32_t       rowIndex; 
                          
  int32_t       rowSpan;  
                          
};

void
BCCellBorder::Reset(uint32_t aRowIndex,
                    uint32_t aRowSpan)
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
  explicit BCMapCellInfo(nsTableFrame* aTableFrame);
  void ResetCellInfo();
  void SetInfo(nsTableRowFrame*   aNewRow,
               int32_t            aColIndex,
               BCCellData*        aCellData,
               BCMapCellIterator* aIter,
               nsCellMap*         aCellMap = nullptr);
  
  
  
  
  
  
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
  void SetTableLeftBorderWidth(int32_t aRowY, BCPixelSize aWidth);
  void SetTableRightBorderWidth(int32_t aRowY, BCPixelSize aWidth);
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

  
  void SetColumn(int32_t aColX);
  
  void IncrementRow(bool aResetToTopRowOfCell = false);

  
  int32_t GetCellEndRowIndex() const;
  int32_t GetCellEndColIndex() const;

  
  nsTableFrame*         mTableFrame;
  int32_t               mNumTableRows;
  int32_t               mNumTableCols;
  BCPropertyData*       mTableBCData;

  
  
  bool                  mTableIsLTR;
  mozilla::css::Side    mStartSide;
  mozilla::css::Side    mEndSide;

  
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

  int32_t               mRowIndex;
  int32_t               mRowSpan;
  int32_t               mColIndex;
  int32_t               mColSpan;

  
  
  
  bool                  mRgAtTop;
  bool                  mRgAtBottom;
  bool                  mCgAtLeft;
  bool                  mCgAtRight;

};


BCMapCellInfo::BCMapCellInfo(nsTableFrame* aTableFrame)
{
  mTableFrame = aTableFrame;
  mTableIsLTR =
    aTableFrame->StyleVisibility()->mDirection == NS_STYLE_DIRECTION_LTR;
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
  mTableBCData = static_cast<BCPropertyData*>
    (mTableFrame->Properties().Get(TableBCProperty()));

  ResetCellInfo();
}

void BCMapCellInfo::ResetCellInfo()
{
  mCellData  = nullptr;
  mRowGroup  = nullptr;
  mTopRow    = nullptr;
  mBottomRow = nullptr;
  mColGroup  = nullptr;
  mLeftCol   = nullptr;
  mRightCol  = nullptr;
  mCell      = nullptr;
  mRowIndex  = mRowSpan = mColIndex = mColSpan = 0;
  mRgAtTop = mRgAtBottom = mCgAtLeft = mCgAtRight = false;
}

inline int32_t BCMapCellInfo::GetCellEndRowIndex() const
{
  return mRowIndex + mRowSpan - 1;
}

inline int32_t BCMapCellInfo::GetCellEndColIndex() const
{
  return mColIndex + mColSpan - 1;
}


class BCMapCellIterator
{
public:
  BCMapCellIterator(nsTableFrame* aTableFrame,
                    const nsIntRect& aDamageArea);

  void First(BCMapCellInfo& aMapCellInfo);

  void Next(BCMapCellInfo& aMapCellInfo);

  void PeekRight(BCMapCellInfo& aRefInfo,
                 uint32_t     aRowIndex,
                 BCMapCellInfo& aAjaInfo);

  void PeekBottom(BCMapCellInfo& aRefInfo,
                  uint32_t     aColIndex,
                  BCMapCellInfo& aAjaInfo);

  bool IsNewRow() { return mIsNewRow; }

  nsTableRowFrame* GetPrevRow() const { return mPrevRow; }
  nsTableRowFrame* GetCurrentRow() const { return mRow; }
  nsTableRowGroupFrame* GetCurrentRowGroup() const { return mRowGroup;}

  int32_t    mRowGroupStart;
  int32_t    mRowGroupEnd;
  bool       mAtEnd;
  nsCellMap* mCellMap;

private:
  bool SetNewRow(nsTableRowFrame* row = nullptr);
  bool SetNewRowGroup(bool aFindFirstDamagedRow);

  nsTableFrame*         mTableFrame;
  nsTableCellMap*       mTableCellMap;
  nsTableFrame::RowGroupArray mRowGroups;
  nsTableRowGroupFrame* mRowGroup;
  int32_t               mRowGroupIndex;
  uint32_t              mNumTableRows;
  nsTableRowFrame*      mRow;
  nsTableRowFrame*      mPrevRow;
  bool                  mIsNewRow;
  int32_t               mRowIndex;
  uint32_t              mNumTableCols;
  int32_t               mColIndex;
  nsPoint               mAreaStart;
  nsPoint               mAreaEnd;
};

BCMapCellIterator::BCMapCellIterator(nsTableFrame* aTableFrame,
                                     const nsIntRect& aDamageArea)
:mTableFrame(aTableFrame)
{
  mTableCellMap  = aTableFrame->GetCellMap();

  mAreaStart.x   = aDamageArea.x;
  mAreaStart.y   = aDamageArea.y;
  mAreaEnd.y     = aDamageArea.y + aDamageArea.height - 1;
  mAreaEnd.x     = aDamageArea.x + aDamageArea.width - 1;

  mNumTableRows  = mTableFrame->GetRowCount();
  mRow           = nullptr;
  mRowIndex      = 0;
  mNumTableCols  = mTableFrame->GetColCount();
  mColIndex      = 0;
  mRowGroupIndex = -1;

  
  aTableFrame->OrderRowGroups(mRowGroups);

  mAtEnd = true; 
}


void
BCMapCellInfo::SetInfo(nsTableRowFrame*   aNewRow,
                       int32_t            aColIndex,
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

  
  mCell      = nullptr;
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
      for (int32_t spanY = 2; mBottomRow && (spanY < mRowSpan); spanY++) {
        mBottomRow = mBottomRow->GetNextRow();
      }
      NS_ASSERTION(mBottomRow, "spanned row not found");
    }
    else {
      NS_ASSERTION(false, "error in cell map");
      mRowSpan = 1;
      mBottomRow = mTopRow;
    }
  }
  
  
  
  
  uint32_t rgStart  = aIter->mRowGroupStart;
  uint32_t rgEnd    = aIter->mRowGroupEnd;
  mRowGroup = static_cast<nsTableRowGroupFrame*>(mTopRow->GetParent());
  if (mRowGroup != aIter->GetCurrentRowGroup()) {
    rgStart = mRowGroup->GetStartRowIndex();
    rgEnd   = rgStart + mRowGroup->GetRowCount() - 1;
  }
  uint32_t rowIndex = mTopRow->GetRowIndex();
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
  int32_t cgStart = mColGroup->GetStartColumnIndex();
  int32_t cgEnd = std::max(0, cgStart + mColGroup->GetColCount() - 1);
  mCgAtLeft  = (cgStart == aColIndex);
  mCgAtRight = (cgEnd == aColIndex + mColSpan - 1);
}

bool
BCMapCellIterator::SetNewRow(nsTableRowFrame* aRow)
{
  mAtEnd   = true;
  mPrevRow = mRow;
  if (aRow) {
    mRow = aRow;
  }
  else if (mRow) {
    mRow = mRow->GetNextRow();
  }
  if (mRow) {
    mRowIndex = mRow->GetRowIndex();
    
    int32_t rgRowIndex = mRowIndex - mRowGroupStart;
    if (uint32_t(rgRowIndex) >= mCellMap->mRows.Length())
      ABORT1(false);
    const nsCellMap::CellDataArray& row = mCellMap->mRows[rgRowIndex];

    for (mColIndex = mAreaStart.x; mColIndex <= mAreaEnd.x; mColIndex++) {
      CellData* cellData = row.SafeElementAt(mColIndex);
      if (!cellData) { 
        nsIntRect damageArea;
        cellData = mCellMap->AppendCell(*mTableCellMap, nullptr, rgRowIndex,
                                        false, 0, damageArea);
        if (!cellData) ABORT1(false);
      }
      if (cellData && (cellData->IsOrig() || cellData->IsDead())) {
        break;
      }
    }
    mIsNewRow = true;
    mAtEnd    = false;
  }
  else ABORT1(false);

  return !mAtEnd;
}

bool
BCMapCellIterator::SetNewRowGroup(bool aFindFirstDamagedRow)
{
   mAtEnd = true;
  int32_t numRowGroups = mRowGroups.Length();
  mCellMap = nullptr;
  for (mRowGroupIndex++; mRowGroupIndex < numRowGroups; mRowGroupIndex++) {
    mRowGroup = mRowGroups[mRowGroupIndex];
    int32_t rowCount = mRowGroup->GetRowCount();
    mRowGroupStart = mRowGroup->GetStartRowIndex();
    mRowGroupEnd   = mRowGroupStart + rowCount - 1;
    if (rowCount > 0) {
      mCellMap = mTableCellMap->GetMapFor(mRowGroup, mCellMap);
      if (!mCellMap) ABORT1(false);
      nsTableRowFrame* firstRow = mRowGroup->GetFirstRow();
      if (aFindFirstDamagedRow) {
        if ((mAreaStart.y >= mRowGroupStart) && (mAreaStart.y <= mRowGroupEnd)) {
          
          if (aFindFirstDamagedRow) {
            
            int32_t numRows = mAreaStart.y - mRowGroupStart;
            for (int32_t i = 0; i < numRows; i++) {
              firstRow = firstRow->GetNextRow();
              if (!firstRow) ABORT1(false);
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

  SetNewRowGroup(true); 
  while (!mAtEnd) {
    if ((mAreaStart.y >= mRowGroupStart) && (mAreaStart.y <= mRowGroupEnd)) {
      BCCellData* cellData =
        static_cast<BCCellData*>(mCellMap->GetDataAt(mAreaStart.y -
                                                      mRowGroupStart,
                                                      mAreaStart.x));
      if (cellData && (cellData->IsOrig() || cellData->IsDead())) {
        aMapInfo.SetInfo(mRow, mAreaStart.x, cellData, this);
        return;
      }
      else {
        NS_ASSERTION(((0 == mAreaStart.x) && (mRowGroupStart == mAreaStart.y)) ,
                     "damage area expanded incorrectly");
      }
    }
    SetNewRowGroup(true); 
  }
}

void
BCMapCellIterator::Next(BCMapCellInfo& aMapInfo)
{
  if (mAtEnd) ABORT0();
  aMapInfo.ResetCellInfo();

  mIsNewRow = false;
  mColIndex++;
  while ((mRowIndex <= mAreaEnd.y) && !mAtEnd) {
    for (; mColIndex <= mAreaEnd.x; mColIndex++) {
      int32_t rgRowIndex = mRowIndex - mRowGroupStart;
      BCCellData* cellData =
         static_cast<BCCellData*>(mCellMap->GetDataAt(rgRowIndex, mColIndex));
      if (!cellData) { 
        nsIntRect damageArea;
        cellData =
          static_cast<BCCellData*>(mCellMap->AppendCell(*mTableCellMap, nullptr,
                                                         rgRowIndex, false, 0,
                                                         damageArea));
        if (!cellData) ABORT0();
      }
      if (cellData && (cellData->IsOrig() || cellData->IsDead())) {
        aMapInfo.SetInfo(mRow, mColIndex, cellData, this);
        return;
      }
    }
    if (mRowIndex >= mRowGroupEnd) {
      SetNewRowGroup(false); 
    }
    else {
      SetNewRow(); 
    }
  }
  mAtEnd = true;
}

void
BCMapCellIterator::PeekRight(BCMapCellInfo&   aRefInfo,
                             uint32_t         aRowIndex,
                             BCMapCellInfo&   aAjaInfo)
{
  aAjaInfo.ResetCellInfo();
  int32_t colIndex = aRefInfo.mColIndex + aRefInfo.mColSpan;
  uint32_t rgRowIndex = aRowIndex - mRowGroupStart;

  BCCellData* cellData =
    static_cast<BCCellData*>(mCellMap->GetDataAt(rgRowIndex, colIndex));
  if (!cellData) { 
    NS_ASSERTION(colIndex < mTableCellMap->GetColCount(), "program error");
    nsIntRect damageArea;
    cellData =
      static_cast<BCCellData*>(mCellMap->AppendCell(*mTableCellMap, nullptr,
                                                     rgRowIndex, false, 0,
                                                     damageArea));
    if (!cellData) ABORT0();
  }
  nsTableRowFrame* row = nullptr;
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
                              uint32_t         aColIndex,
                              BCMapCellInfo&   aAjaInfo)
{
  aAjaInfo.ResetCellInfo();
  int32_t rowIndex = aRefInfo.mRowIndex + aRefInfo.mRowSpan;
  int32_t rgRowIndex = rowIndex - mRowGroupStart;
  nsTableRowGroupFrame* rg = mRowGroup;
  nsCellMap* cellMap = mCellMap;
  nsTableRowFrame* nextRow = nullptr;
  if (rowIndex > mRowGroupEnd) {
    int32_t nextRgIndex = mRowGroupIndex;
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
    for (int32_t i = 0; i < aRefInfo.mRowSpan; i++) {
      nextRow = nextRow->GetNextRow(); if (!nextRow) ABORT0();
    }
  }

  BCCellData* cellData =
    static_cast<BCCellData*>(cellMap->GetDataAt(rgRowIndex, aColIndex));
  if (!cellData) { 
    NS_ASSERTION(rgRowIndex < cellMap->GetRowCount(), "program error");
    nsIntRect damageArea;
    cellData =
      static_cast<BCCellData*>(cellMap->AppendCell(*mTableCellMap, nullptr,
                                                    rgRowIndex, false, 0,
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



static uint8_t styleToPriority[13] = { 0,  
                                       2,  
                                       4,  
                                       5,  
                                       6,  
                                       7,  
                                       8,  
                                       1,  
                                       3,  
                                       9 };



#define CELL_CORNER true








static void
GetColorAndStyle(const nsIFrame*  aFrame,
                 mozilla::css::Side aSide,
                 uint8_t&         aStyle,
                 nscolor&         aColor,
                 bool             aTableIsLTR)
{
  NS_PRECONDITION(aFrame, "null frame");
  
  aColor = 0;
  const nsStyleBorder* styleData = aFrame->StyleBorder();
  if(!aTableIsLTR) { 
    if (NS_SIDE_RIGHT == aSide) {
      aSide = NS_SIDE_LEFT;
    }
    else if (NS_SIDE_LEFT == aSide) {
      aSide = NS_SIDE_RIGHT;
    }
  }
  aStyle = styleData->GetBorderStyle(aSide);

  if ((NS_STYLE_BORDER_STYLE_NONE == aStyle) ||
      (NS_STYLE_BORDER_STYLE_HIDDEN == aStyle)) {
    return;
  }
  aColor = aFrame->StyleContext()->GetVisitedDependentColor(
             nsCSSProps::SubpropertyEntryFor(eCSSProperty_border_color)[aSide]);
}








static void
GetPaintStyleInfo(const nsIFrame*  aFrame,
                  mozilla::css::Side aSide,
                  uint8_t&         aStyle,
                  nscolor&         aColor,
                  bool             aTableIsLTR)
{
  GetColorAndStyle(aFrame, aSide, aStyle, aColor, aTableIsLTR);
  if (NS_STYLE_BORDER_STYLE_INSET    == aStyle) {
    aStyle = NS_STYLE_BORDER_STYLE_RIDGE;
  }
  else if (NS_STYLE_BORDER_STYLE_OUTSET    == aStyle) {
    aStyle = NS_STYLE_BORDER_STYLE_GROOVE;
  }
}











static void
GetColorAndStyle(const nsIFrame*  aFrame,
                 mozilla::css::Side aSide,
                 uint8_t&         aStyle,
                 nscolor&         aColor,
                 bool             aTableIsLTR,
                 BCPixelSize&     aWidth)
{
  GetColorAndStyle(aFrame, aSide, aStyle, aColor, aTableIsLTR);
  if ((NS_STYLE_BORDER_STYLE_NONE == aStyle) ||
      (NS_STYLE_BORDER_STYLE_HIDDEN == aStyle)) {
    aWidth = 0;
    return;
  }
  const nsStyleBorder* styleData = aFrame->StyleBorder();
  nscoord width;
  if(!aTableIsLTR) { 
    if (NS_SIDE_RIGHT == aSide) {
      aSide = NS_SIDE_LEFT;
    }
    else if (NS_SIDE_LEFT == aSide) {
      aSide = NS_SIDE_RIGHT;
    }
  }
  width = styleData->GetComputedBorderWidth(aSide);
  aWidth = nsPresContext::AppUnitsToIntCSSPixels(width);
}

class nsDelayedCalcBCBorders : public nsRunnable {
public:
  explicit nsDelayedCalcBCBorders(nsIFrame* aFrame) :
    mFrame(aFrame) {}

  NS_IMETHOD Run() override {
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

bool
nsTableFrame::BCRecalcNeeded(nsStyleContext* aOldStyleContext,
                             nsStyleContext* aNewStyleContext)
{
  
  
  

  const nsStyleBorder* oldStyleData = aOldStyleContext->PeekStyleBorder();
  if (!oldStyleData)
    return false;

  const nsStyleBorder* newStyleData = aNewStyleContext->StyleBorder();
  nsChangeHint change = newStyleData->CalcDifference(*oldStyleData);
  if (!change)
    return false;
  if (change & nsChangeHint_NeedReflow)
    return true; 
  if (change & nsChangeHint_RepaintFrame) {
    
    
    
    
    
    
    nsCOMPtr<nsIRunnable> evt = new nsDelayedCalcBCBorders(this);
    NS_DispatchToCurrentThread(evt);
    return true;
  }
  return false;
}





static const BCCellBorder&
CompareBorders(bool                aIsCorner, 
               const BCCellBorder& aBorder1,
               const BCCellBorder& aBorder2,
               bool                aSecondIsHorizontal,
               bool*             aFirstDominates = nullptr)
{
  bool firstDominates = true;

  if (NS_STYLE_BORDER_STYLE_HIDDEN == aBorder1.style) {
    firstDominates = (aIsCorner) ? false : true;
  }
  else if (NS_STYLE_BORDER_STYLE_HIDDEN == aBorder2.style) {
    firstDominates = (aIsCorner) ? true : false;
  }
  else if (aBorder1.width < aBorder2.width) {
    firstDominates = false;
  }
  else if (aBorder1.width == aBorder2.width) {
    if (styleToPriority[aBorder1.style] < styleToPriority[aBorder2.style]) {
      firstDominates = false;
    }
    else if (styleToPriority[aBorder1.style] == styleToPriority[aBorder2.style]) {
      if (aBorder1.owner == aBorder2.owner) {
        firstDominates = !aSecondIsHorizontal;
      }
      else if (aBorder1.owner < aBorder2.owner) {
        firstDominates = false;
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
               bool             aTableIsLTR,
               mozilla::css::Side aSide,
               bool             aAja)
{
  BCCellBorder border, tempBorder;
  bool horizontal = (NS_SIDE_TOP == aSide) || (NS_SIDE_BOTTOM == aSide);

  
  if (aTableFrame) {
    GetColorAndStyle(aTableFrame, aSide, border.style, border.color, aTableIsLTR, border.width);
    border.owner = eTableOwner;
    if (NS_STYLE_BORDER_STYLE_HIDDEN == border.style) {
      return border;
    }
  }
  
  if (aColGroupFrame) {
    GetColorAndStyle(aColGroupFrame, aSide, tempBorder.style, tempBorder.color, aTableIsLTR, tempBorder.width);
    tempBorder.owner = (aAja && !horizontal) ? eAjaColGroupOwner : eColGroupOwner;
    
    border = CompareBorders(!CELL_CORNER, border, tempBorder, false);
    if (NS_STYLE_BORDER_STYLE_HIDDEN == border.style) {
      return border;
    }
  }
  
  if (aColFrame) {
    GetColorAndStyle(aColFrame, aSide, tempBorder.style, tempBorder.color, aTableIsLTR, tempBorder.width);
    tempBorder.owner = (aAja && !horizontal) ? eAjaColOwner : eColOwner;
    border = CompareBorders(!CELL_CORNER, border, tempBorder, false);
    if (NS_STYLE_BORDER_STYLE_HIDDEN == border.style) {
      return border;
    }
  }
  
  if (aRowGroupFrame) {
    GetColorAndStyle(aRowGroupFrame, aSide, tempBorder.style, tempBorder.color, aTableIsLTR, tempBorder.width);
    tempBorder.owner = (aAja && horizontal) ? eAjaRowGroupOwner : eRowGroupOwner;
    border = CompareBorders(!CELL_CORNER, border, tempBorder, false);
    if (NS_STYLE_BORDER_STYLE_HIDDEN == border.style) {
      return border;
    }
  }
  
  if (aRowFrame) {
    GetColorAndStyle(aRowFrame, aSide, tempBorder.style, tempBorder.color, aTableIsLTR, tempBorder.width);
    tempBorder.owner = (aAja && horizontal) ? eAjaRowOwner : eRowOwner;
    border = CompareBorders(!CELL_CORNER, border, tempBorder, false);
    if (NS_STYLE_BORDER_STYLE_HIDDEN == border.style) {
      return border;
    }
  }
  
  if (aCellFrame) {
    GetColorAndStyle(aCellFrame, aSide, tempBorder.style, tempBorder.color, aTableIsLTR, tempBorder.width);
    tempBorder.owner = (aAja) ? eAjaCellOwner : eCellOwner;
    border = CompareBorders(!CELL_CORNER, border, tempBorder, false);
  }
  return border;
}

static bool
Perpendicular(mozilla::css::Side aSide1,
              mozilla::css::Side aSide2)
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
  BCCornerInfo() { ownerColor = 0; ownerWidth = subWidth = ownerElem = subSide =
                   subElem = hasDashDot = numSegs = bevel = 0; ownerSide = NS_SIDE_TOP;
                   ownerStyle = 0xFF; subStyle = NS_STYLE_BORDER_STYLE_SOLID;  }
  void Set(mozilla::css::Side aSide,
           BCCellBorder  border);

  void Update(mozilla::css::Side aSide,
              BCCellBorder  border);

  nscolor   ownerColor;     
  uint16_t  ownerWidth;     
  uint16_t  subWidth;       
                            
  uint32_t  ownerSide:2;    
                            
  uint32_t  ownerElem:3;    
  uint32_t  ownerStyle:8;   
  uint32_t  subSide:2;      
  uint32_t  subElem:3;      
  uint32_t  subStyle:8;     
  uint32_t  hasDashDot:1;   
  uint32_t  numSegs:3;      
  uint32_t  bevel:1;        
  uint32_t  unused:1;
};

void
BCCornerInfo::Set(mozilla::css::Side aSide,
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
BCCornerInfo::Update(mozilla::css::Side aSide,
                     BCCellBorder  aBorder)
{
  bool existingWins = false;
  if (0xFF == ownerStyle) { 
    Set(aSide, aBorder);
  }
  else {
    bool horizontal = (NS_SIDE_LEFT == aSide) || (NS_SIDE_RIGHT == aSide); 
    BCCellBorder oldBorder, tempBorder;
    oldBorder.owner  = (BCBorderOwner) ownerElem;
    oldBorder.style =  ownerStyle;
    oldBorder.width =  ownerWidth;
    oldBorder.color =  ownerColor;

    mozilla::css::Side oldSide  = mozilla::css::Side(ownerSide);

    tempBorder = CompareBorders(CELL_CORNER, oldBorder, aBorder, horizontal, &existingWins);

    ownerElem  = tempBorder.owner;
    ownerStyle = tempBorder.style;
    ownerWidth = tempBorder.width;
    ownerColor = tempBorder.color;
    if (existingWins) { 
      if (::Perpendicular(mozilla::css::Side(ownerSide), aSide)) {
        
        BCCellBorder subBorder;
        subBorder.owner = (BCBorderOwner) subElem;
        subBorder.style =  subStyle;
        subBorder.width =  subWidth;
        subBorder.color = 0; 
        bool firstWins;

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
      if (::Perpendicular(oldSide, mozilla::css::Side(ownerSide))) {
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
  BCCorners(int32_t aNumCorners,
            int32_t aStartIndex);

  ~BCCorners() { delete [] corners; }

  BCCornerInfo& operator [](int32_t i) const
  { NS_ASSERTION((i >= startIndex) && (i <= endIndex), "program error");
    return corners[clamped(i, startIndex, endIndex) - startIndex]; }

  int32_t       startIndex;
  int32_t       endIndex;
  BCCornerInfo* corners;
};

BCCorners::BCCorners(int32_t aNumCorners,
                     int32_t aStartIndex)
{
  NS_ASSERTION((aNumCorners > 0) && (aStartIndex >= 0), "program error");
  startIndex = aStartIndex;
  endIndex   = aStartIndex + aNumCorners - 1;
  corners    = new BCCornerInfo[aNumCorners];
}


struct BCCellBorders
{
  BCCellBorders(int32_t aNumBorders,
                int32_t aStartIndex);

  ~BCCellBorders() { delete [] borders; }

  BCCellBorder& operator [](int32_t i) const
  { NS_ASSERTION((i >= startIndex) && (i <= endIndex), "program error");
    return borders[clamped(i, startIndex, endIndex) - startIndex]; }

  int32_t       startIndex;
  int32_t       endIndex;
  BCCellBorder* borders;
};

BCCellBorders::BCCellBorders(int32_t aNumBorders,
                             int32_t aStartIndex)
{
  NS_ASSERTION((aNumBorders > 0) && (aStartIndex >= 0), "program error");
  startIndex = aStartIndex;
  endIndex   = aStartIndex + aNumBorders - 1;
  borders    = new BCCellBorder[aNumBorders];
}




static bool
SetBorder(const BCCellBorder&   aNewBorder,
          BCCellBorder&         aBorder)
{
  bool changed = (aNewBorder.style != aBorder.style) ||
                   (aNewBorder.width != aBorder.width) ||
                   (aNewBorder.color != aBorder.color);
  aBorder.color        = aNewBorder.color;
  aBorder.width        = aNewBorder.width;
  aBorder.style        = aNewBorder.style;
  aBorder.owner        = aNewBorder.owner;

  return changed;
}




static bool
SetHorBorder(const BCCellBorder& aNewBorder,
             const BCCornerInfo& aCorner,
             BCCellBorder&       aBorder)
{
  bool startSeg = ::SetBorder(aNewBorder, aBorder);
  if (!startSeg) {
    startSeg = ((NS_SIDE_LEFT != aCorner.ownerSide) && (NS_SIDE_RIGHT != aCorner.ownerSide));
  }
  return startSeg;
}





void
nsTableFrame::ExpandBCDamageArea(nsIntRect& aRect) const
{
  int32_t numRows = GetRowCount();
  int32_t numCols = GetColCount();

  int32_t dStartX = aRect.x;
  int32_t dEndX   = aRect.XMost() - 1;
  int32_t dStartY = aRect.y;
  int32_t dEndY   = aRect.YMost() - 1;

  
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
  
  
  
  
  
  bool haveSpanner = false;
  if ((dStartX > 0) || (dEndX < (numCols - 1)) || (dStartY > 0) || (dEndY < (numRows - 1))) {
    nsTableCellMap* tableCellMap = GetCellMap(); if (!tableCellMap) ABORT0();
    
    RowGroupArray rowGroups;
    OrderRowGroups(rowGroups);

    
    nsCellMap* cellMap = nullptr;
    for (uint32_t rgX = 0; rgX < rowGroups.Length(); rgX++) {
      nsTableRowGroupFrame* rgFrame = rowGroups[rgX];
      int32_t rgStartY = rgFrame->GetStartRowIndex();
      int32_t rgEndY   = rgStartY + rgFrame->GetRowCount() - 1;
      if (dEndY < rgStartY)
        break;
      cellMap = tableCellMap->GetMapFor(rgFrame, cellMap);
      if (!cellMap) ABORT0();
      
      if ((dStartY > 0) && (dStartY >= rgStartY) && (dStartY <= rgEndY)) {
        if (uint32_t(dStartY - rgStartY) >= cellMap->mRows.Length())
          ABORT0();
        const nsCellMap::CellDataArray& row =
          cellMap->mRows[dStartY - rgStartY];
        for (int32_t x = dStartX; x <= dEndX; x++) {
          CellData* cellData = row.SafeElementAt(x);
          if (cellData && (cellData->IsRowSpan())) {
             haveSpanner = true;
             break;
          }
        }
        if (dEndY < rgEndY) {
          if (uint32_t(dEndY + 1 - rgStartY) >= cellMap->mRows.Length())
            ABORT0();
          const nsCellMap::CellDataArray& row2 =
            cellMap->mRows[dEndY + 1 - rgStartY];
          for (int32_t x = dStartX; x <= dEndX; x++) {
            CellData* cellData = row2.SafeElementAt(x);
            if (cellData && (cellData->IsRowSpan())) {
              haveSpanner = true;
              break;
            }
          }
        }
      }
      
      int32_t iterStartY = -1;
      int32_t iterEndY   = -1;
      if ((dStartY >= rgStartY) && (dStartY <= rgEndY)) {
        
        iterStartY = dStartY;
        iterEndY   = std::min(dEndY, rgEndY);
      }
      else if ((dEndY >= rgStartY) && (dEndY <= rgEndY)) {
        
        iterStartY = rgStartY;
        iterEndY   = dEndY;
      }
      else if ((rgStartY >= dStartY) && (rgEndY <= dEndY)) {
        
        iterStartY = rgStartY;
        iterEndY   = rgEndY;
      }
      if ((iterStartY >= 0) && (iterEndY >= 0)) {
        for (int32_t y = iterStartY; y <= iterEndY; y++) {
          if (uint32_t(y - rgStartY) >= cellMap->mRows.Length())
            ABORT0();
          const nsCellMap::CellDataArray& row =
            cellMap->mRows[y - rgStartY];
          CellData* cellData = row.SafeElementAt(dStartX);
          if (cellData && (cellData->IsColSpan())) {
            haveSpanner = true;
            break;
          }
          if (dEndX < (numCols - 1)) {
            cellData = row.SafeElementAt(dEndX + 1);
            if (cellData && (cellData->IsColSpan())) {
              haveSpanner = true;
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


#define ADJACENT    true
#define HORIZONTAL  true

void
BCMapCellInfo::SetTableTopLeftContBCBorder()
{
  BCCellBorder currentBorder;
  
  
  if (mTopRow) {
    currentBorder = CompareBorders(mTableFrame, nullptr, nullptr, mRowGroup,
                                   mTopRow, nullptr, mTableIsLTR,
                                   NS_SIDE_TOP, !ADJACENT);
    mTopRow->SetContinuousBCBorderWidth(NS_SIDE_TOP, currentBorder.width);
  }
  if (mCgAtRight && mColGroup) {
    
    currentBorder = CompareBorders(mTableFrame, mColGroup, nullptr, mRowGroup,
                                   mTopRow, nullptr, mTableIsLTR,
                                   NS_SIDE_TOP, !ADJACENT);
    mColGroup->SetContinuousBCBorderWidth(NS_SIDE_TOP, currentBorder.width);
  }
  if (0 == mColIndex) {
    currentBorder = CompareBorders(mTableFrame, mColGroup, mLeftCol, nullptr,
                                   nullptr, nullptr, mTableIsLTR, NS_SIDE_LEFT,
                                   !ADJACENT);
    mTableFrame->SetContinuousLeftBCBorderWidth(currentBorder.width);
  }
}

void
BCMapCellInfo::SetRowGroupLeftContBCBorder()
{
  BCCellBorder currentBorder;
  
  if (mRgAtBottom && mRowGroup) { 
     currentBorder = CompareBorders(mTableFrame, mColGroup, mLeftCol, mRowGroup,
                                    nullptr, nullptr, mTableIsLTR, NS_SIDE_LEFT,
                                    !ADJACENT);
     mRowGroup->SetContinuousBCBorderWidth(mStartSide, currentBorder.width);
  }
}

void
BCMapCellInfo::SetRowGroupRightContBCBorder()
{
  BCCellBorder currentBorder;
  
  if (mRgAtBottom && mRowGroup) { 
    currentBorder = CompareBorders(mTableFrame, mColGroup, mRightCol, mRowGroup,
                                   nullptr, nullptr, mTableIsLTR, NS_SIDE_RIGHT,
                                   ADJACENT);
    mRowGroup->SetContinuousBCBorderWidth(mEndSide, currentBorder.width);
  }
}

void
BCMapCellInfo::SetColumnTopRightContBCBorder()
{
  BCCellBorder currentBorder;
  
  
  currentBorder = CompareBorders(mTableFrame, mCurrentColGroupFrame,
                                 mCurrentColFrame, mRowGroup, mTopRow, nullptr,
                                 mTableIsLTR, NS_SIDE_TOP, !ADJACENT);
  ((nsTableColFrame*) mCurrentColFrame)->SetContinuousBCBorderWidth(NS_SIDE_TOP,
                                                           currentBorder.width);
  if (mNumTableCols == GetCellEndColIndex() + 1) {
    currentBorder = CompareBorders(mTableFrame, mCurrentColGroupFrame,
                                   mCurrentColFrame, nullptr, nullptr, nullptr,
                                   mTableIsLTR, NS_SIDE_RIGHT, !ADJACENT);
  }
  else {
    currentBorder = CompareBorders(nullptr, mCurrentColGroupFrame,
                                   mCurrentColFrame, nullptr,nullptr, nullptr,
                                   mTableIsLTR, NS_SIDE_RIGHT, !ADJACENT);
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
                                 nullptr, mTableIsLTR, NS_SIDE_BOTTOM, ADJACENT);
  mCurrentColFrame->SetContinuousBCBorderWidth(NS_SIDE_BOTTOM,
                                               currentBorder.width);
}

void
BCMapCellInfo::SetColGroupBottomContBCBorder()
{
  BCCellBorder currentBorder;
  if (mColGroup) {
    currentBorder = CompareBorders(mTableFrame, mColGroup, nullptr, mRowGroup,
                                   mBottomRow, nullptr, mTableIsLTR,
                                   NS_SIDE_BOTTOM, ADJACENT);
    mColGroup->SetContinuousBCBorderWidth(NS_SIDE_BOTTOM, currentBorder.width);
  }
}

void
BCMapCellInfo::SetRowGroupBottomContBCBorder()
{
  BCCellBorder currentBorder;
  if (mRowGroup) {
    currentBorder = CompareBorders(mTableFrame, nullptr, nullptr, mRowGroup,
                                   mBottomRow, nullptr, mTableIsLTR,
                                   NS_SIDE_BOTTOM, ADJACENT);
    mRowGroup->SetContinuousBCBorderWidth(NS_SIDE_BOTTOM, currentBorder.width);
  }
}

void
BCMapCellInfo::SetInnerRowGroupBottomContBCBorder(const nsIFrame* aNextRowGroup,
                                                  nsTableRowFrame* aNextRow)
{
  BCCellBorder currentBorder, adjacentBorder;

  const nsIFrame* rowgroup = (mRgAtBottom) ? mRowGroup : nullptr;
  currentBorder = CompareBorders(nullptr, nullptr, nullptr, rowgroup, mBottomRow,
                                 nullptr, mTableIsLTR, NS_SIDE_BOTTOM, ADJACENT);

  adjacentBorder = CompareBorders(nullptr, nullptr, nullptr, aNextRowGroup,
                                  aNextRow, nullptr, mTableIsLTR, NS_SIDE_TOP,
                                  !ADJACENT);
  currentBorder = CompareBorders(false, currentBorder, adjacentBorder,
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
                                   mCurrentRowFrame, nullptr, mTableIsLTR,
                                   NS_SIDE_LEFT, !ADJACENT);
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
                                   mCurrentRowFrame, nullptr, mTableIsLTR,
                                   NS_SIDE_RIGHT, ADJACENT);
    mCurrentRowFrame->SetContinuousBCBorderWidth(mEndSide,
                                                 currentBorder.width);
  }
}
void
BCMapCellInfo::SetTableTopBorderWidth(BCPixelSize aWidth)
{
  mTableBCData->mTopBorderWidth = std::max(mTableBCData->mTopBorderWidth, aWidth);
}

void
BCMapCellInfo::SetTableLeftBorderWidth(int32_t aRowY, BCPixelSize aWidth)
{
  
  if (aRowY == 0) {
    if (mTableIsLTR) {
      mTableBCData->mLeftCellBorderWidth = aWidth;
    }
    else {
      mTableBCData->mRightCellBorderWidth = aWidth;
    }
  }
  mTableBCData->mLeftBorderWidth = std::max(mTableBCData->mLeftBorderWidth,
                                          aWidth);
}

void
BCMapCellInfo::SetTableRightBorderWidth(int32_t aRowY, BCPixelSize aWidth)
{
  
  if (aRowY == 0) {
    if (mTableIsLTR) {
      mTableBCData->mRightCellBorderWidth = aWidth;
    }
    else {
      mTableBCData->mLeftCellBorderWidth = aWidth;
    }
  }
  mTableBCData->mRightBorderWidth = std::max(mTableBCData->mRightBorderWidth,
                                           aWidth);
}

void
BCMapCellInfo::SetRightBorderWidths(BCPixelSize aWidth)
{
   
  if (mCell) {
    mCell->SetBorderWidth(mEndSide, std::max(aWidth,
                          mCell->GetBorderWidth(mEndSide)));
  }
  if (mRightCol) {
    BCPixelSize half = BC_BORDER_LEFT_HALF(aWidth);
    mRightCol->SetRightBorderWidth(std::max(nscoord(half),
                                   mRightCol->GetRightBorderWidth()));
  }
}

void
BCMapCellInfo::SetBottomBorderWidths(BCPixelSize aWidth)
{
  
  if (mCell) {
    mCell->SetBorderWidth(NS_SIDE_BOTTOM, std::max(aWidth,
                          mCell->GetBorderWidth(NS_SIDE_BOTTOM)));
  }
  if (mBottomRow) {
    BCPixelSize half = BC_BORDER_TOP_HALF(aWidth);
    mBottomRow->SetBottomBCBorderWidth(std::max(nscoord(half),
                                       mBottomRow->GetBottomBCBorderWidth()));
  }
}
void
BCMapCellInfo::SetTopBorderWidths(BCPixelSize aWidth)
{
 if (mCell) {
     mCell->SetBorderWidth(NS_SIDE_TOP, std::max(aWidth,
                           mCell->GetBorderWidth(NS_SIDE_TOP)));
  }
  if (mTopRow) {
    BCPixelSize half = BC_BORDER_BOTTOM_HALF(aWidth);
    mTopRow->SetTopBCBorderWidth(std::max(nscoord(half),
                                        mTopRow->GetTopBCBorderWidth()));
  }
}
void
BCMapCellInfo::SetLeftBorderWidths(BCPixelSize aWidth)
{
  if (mCell) {
    mCell->SetBorderWidth(mStartSide, std::max(aWidth,
                          mCell->GetBorderWidth(mStartSide)));
  }
  if (mLeftCol) {
    BCPixelSize half = BC_BORDER_RIGHT_HALF(aWidth);
    mLeftCol->SetLeftBorderWidth(std::max(nscoord(half),
                                        mLeftCol->GetLeftBorderWidth()));
  }
}

void
BCMapCellInfo::SetTableBottomBorderWidth(BCPixelSize aWidth)
{
  mTableBCData->mBottomBorderWidth = std::max(mTableBCData->mBottomBorderWidth,
                                            aWidth);
}

void
BCMapCellInfo::SetColumn(int32_t aColX)
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
BCMapCellInfo::IncrementRow(bool aResetToTopRowOfCell)
{
  mCurrentRowFrame = (aResetToTopRowOfCell) ? mTopRow :
                                                mCurrentRowFrame->GetNextRow();
}

BCCellBorder
BCMapCellInfo::GetTopEdgeBorder()
{
  return CompareBorders(mTableFrame, mCurrentColGroupFrame, mCurrentColFrame,
                        mRowGroup, mTopRow, mCell, mTableIsLTR, NS_SIDE_TOP,
                        !ADJACENT);
}

BCCellBorder
BCMapCellInfo::GetBottomEdgeBorder()
{
  return CompareBorders(mTableFrame, mCurrentColGroupFrame, mCurrentColFrame,
                        mRowGroup, mBottomRow, mCell, mTableIsLTR,
                        NS_SIDE_BOTTOM, ADJACENT);
}
BCCellBorder
BCMapCellInfo::GetLeftEdgeBorder()
{
  return CompareBorders(mTableFrame, mColGroup, mLeftCol, mRowGroup,
                        mCurrentRowFrame, mCell, mTableIsLTR, NS_SIDE_LEFT,
                        !ADJACENT);
}
BCCellBorder
BCMapCellInfo::GetRightEdgeBorder()
{
  return CompareBorders(mTableFrame, mColGroup, mRightCol, mRowGroup,
                        mCurrentRowFrame, mCell, mTableIsLTR, NS_SIDE_RIGHT,
                        ADJACENT);
}
BCCellBorder
BCMapCellInfo::GetRightInternalBorder()
{
  const nsIFrame* cg = (mCgAtRight) ? mColGroup : nullptr;
  return CompareBorders(nullptr, cg, mRightCol, nullptr, nullptr, mCell,
                        mTableIsLTR, NS_SIDE_RIGHT, ADJACENT);
}

BCCellBorder
BCMapCellInfo::GetLeftInternalBorder()
{
  const nsIFrame* cg = (mCgAtLeft) ? mColGroup : nullptr;
  return CompareBorders(nullptr, cg, mLeftCol, nullptr, nullptr, mCell,
                        mTableIsLTR, NS_SIDE_LEFT, !ADJACENT);
}

BCCellBorder
BCMapCellInfo::GetBottomInternalBorder()
{
  const nsIFrame* rg = (mRgAtBottom) ? mRowGroup : nullptr;
  return CompareBorders(nullptr, nullptr, nullptr, rg, mBottomRow, mCell,
                        mTableIsLTR, NS_SIDE_BOTTOM, ADJACENT);
}

BCCellBorder
BCMapCellInfo::GetTopInternalBorder()
{
  const nsIFrame* rg = (mRgAtTop) ? mRowGroup : nullptr;
  return CompareBorders(nullptr, nullptr, nullptr, rg, mTopRow, mCell,
                        mTableIsLTR, NS_SIDE_TOP, !ADJACENT);
}












































void
nsTableFrame::CalcBCBorders()
{
  NS_ASSERTION(IsBorderCollapse(),
               "calling CalcBCBorders on separated-border table");
  nsTableCellMap* tableCellMap = GetCellMap(); if (!tableCellMap) ABORT0();
  int32_t numRows = GetRowCount();
  int32_t numCols = GetColCount();
  if (!numRows || !numCols)
    return; 

  
  BCPropertyData* propData = GetBCProperty();
  if (!propData) ABORT0();



  
  nsIntRect damageArea(propData->mDamageArea);
  ExpandBCDamageArea(damageArea);

  
  
  bool tableBorderReset[4];
  for (uint32_t sideX = NS_SIDE_TOP; sideX <= NS_SIDE_LEFT; sideX++) {
    tableBorderReset[sideX] = false;
  }

  
  BCCellBorders lastVerBorders(damageArea.width + 1, damageArea.x);
  if (!lastVerBorders.borders) ABORT0();
  BCCellBorder  lastTopBorder, lastBottomBorder;
  
  BCCellBorders lastBottomBorders(damageArea.width + 1, damageArea.x);
  if (!lastBottomBorders.borders) ABORT0();
  bool startSeg;
  bool gotRowBorder = false;

  BCMapCellInfo  info(this), ajaInfo(this);

  BCCellBorder currentBorder, adjacentBorder;
  BCCorners topCorners(damageArea.width + 1, damageArea.x);
  if (!topCorners.corners) ABORT0();
  BCCorners bottomCorners(damageArea.width + 1, damageArea.x);
  if (!bottomCorners.corners) ABORT0();

  BCMapCellIterator iter(this, damageArea);
  for (iter.First(info); !iter.mAtEnd; iter.Next(info)) {
    
    if (iter.IsNewRow()) {
      gotRowBorder = false;
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
      }
    }

    
    
    
    if (0 == info.mRowIndex) {
      if (!tableBorderReset[NS_SIDE_TOP]) {
        propData->mTopBorderWidth = 0;
        tableBorderReset[NS_SIDE_TOP] = true;
      }
      for (int32_t colX = info.mColIndex; colX <= info.GetCellEndColIndex();
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
                                          mozilla::css::Side(tlCorner.ownerSide),
                                          tlCorner.subWidth,
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
          mozilla::css::Side cornerSide;
          bool bevel;
          data.GetCorner(cornerSide, bevel);
          if ((NS_SIDE_TOP == cornerSide) || (NS_SIDE_BOTTOM == cornerSide)) {
            data.SetTopStart(true);
          }
        }
      }
    }

    
    
    
    if (0 == info.mColIndex) {
      if (!tableBorderReset[NS_SIDE_LEFT]) {
        propData->mLeftBorderWidth = 0;
        tableBorderReset[NS_SIDE_LEFT] = true;
      }
      info.mCurrentRowFrame = nullptr;
      for (int32_t rowY = info.mRowIndex; rowY <= info.GetCellEndRowIndex();
           rowY++) {
        info.IncrementRow(rowY == info.mRowIndex);
        currentBorder = info.GetLeftEdgeBorder();
        BCCornerInfo& tlCorner = (0 == rowY) ? topCorners[0] : bottomCorners[0];
        tlCorner.Update(NS_SIDE_BOTTOM, currentBorder);
        tableCellMap->SetBCBorderCorner(eTopLeft, *iter.mCellMap,
                                        iter.mRowGroupStart, rowY, 0,
                                        mozilla::css::Side(tlCorner.ownerSide),
                                        tlCorner.subWidth,
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
        tableBorderReset[NS_SIDE_RIGHT] = true;
      }
      info.mCurrentRowFrame = nullptr;
      for (int32_t rowY = info.mRowIndex; rowY <= info.GetCellEndRowIndex();
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
                                        mozilla::css::Side(trCorner.ownerSide),
                                        trCorner.subWidth,
                                        trCorner.bevel);
        BCCornerInfo& brCorner = bottomCorners[info.GetCellEndColIndex() + 1];
        brCorner.Set(NS_SIDE_TOP, currentBorder); 
        tableCellMap->SetBCBorderCorner(eBottomRight, *iter.mCellMap,
                                        iter.mRowGroupStart, rowY,
                                        info.GetCellEndColIndex(),
                                        mozilla::css::Side(brCorner.ownerSide),
                                        brCorner.subWidth,
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
      int32_t segLength = 0;
      BCMapCellInfo priorAjaInfo(this);
      for (int32_t rowY = info.mRowIndex; rowY <= info.GetCellEndRowIndex();
           rowY += segLength) {
        iter.PeekRight(info, rowY, ajaInfo);
        currentBorder  = info.GetRightInternalBorder();
        adjacentBorder = ajaInfo.GetLeftInternalBorder();
        currentBorder = CompareBorders(!CELL_CORNER, currentBorder,
                                        adjacentBorder, !HORIZONTAL);

        segLength = std::max(1, ajaInfo.mRowIndex + ajaInfo.mRowSpan - rowY);
        segLength = std::min(segLength, info.mRowIndex + info.mRowSpan - rowY);

        
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
        
        bool hitsSpanOnRight = (rowY > ajaInfo.mRowIndex) &&
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
                                            mozilla::css::Side(trCorner->ownerSide),
                                            trCorner->subWidth,
                                            trCorner->bevel);
          }
          
          for (int32_t rX = rowY + 1; rX < rowY + segLength; rX++) {
            tableCellMap->SetBCBorderCorner(eBottomRight, *iter.mCellMap,
                                            iter.mRowGroupStart, rX,
                                            info.GetCellEndColIndex(),
                                            mozilla::css::Side(trCorner->ownerSide),
                                            trCorner->subWidth, false);
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
    for (int32_t colX = info.mColIndex + 1; colX <= info.GetCellEndColIndex();
         colX++) {
      lastVerBorders[colX].Reset(0,1);
    }

    
    
    if (info.mNumTableRows == info.GetCellEndRowIndex() + 1) {
      
      if (!tableBorderReset[NS_SIDE_BOTTOM]) {
        propData->mBottomBorderWidth = 0;
        tableBorderReset[NS_SIDE_BOTTOM] = true;
      }
      for (int32_t colX = info.mColIndex; colX <= info.GetCellEndColIndex();
           colX++) {
        info.SetColumn(colX);
        currentBorder = info.GetBottomEdgeBorder();
        
        BCCornerInfo& blCorner = bottomCorners[colX]; 
        blCorner.Update(NS_SIDE_RIGHT, currentBorder);
        tableCellMap->SetBCBorderCorner(eBottomLeft, *iter.mCellMap,
                                        iter.mRowGroupStart,
                                        info.GetCellEndRowIndex(),
                                        colX,
                                        mozilla::css::Side(blCorner.ownerSide),
                                        blCorner.subWidth, blCorner.bevel);
        BCCornerInfo& brCorner = bottomCorners[colX + 1]; 
        brCorner.Update(NS_SIDE_LEFT, currentBorder);
        if (info.mNumTableCols == colX + 1) { 
          tableCellMap->SetBCBorderCorner(eBottomRight, *iter.mCellMap,
                                          iter.mRowGroupStart,
                                          info.GetCellEndRowIndex(),colX,
                                          mozilla::css::Side(brCorner.ownerSide),
                                          brCorner.subWidth,
                                          brCorner.bevel, true);
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
      int32_t segLength = 0;
      for (int32_t colX = info.mColIndex; colX <= info.GetCellEndColIndex();
           colX += segLength) {
        iter.PeekBottom(info, colX, ajaInfo);
        currentBorder  = info.GetBottomInternalBorder();
        adjacentBorder = ajaInfo.GetTopInternalBorder();
        currentBorder = CompareBorders(!CELL_CORNER, currentBorder,
                                        adjacentBorder, HORIZONTAL);
        segLength = std::max(1, ajaInfo.mColIndex + ajaInfo.mColSpan - colX);
        segLength = std::min(segLength, info.mColIndex + info.mColSpan - colX);

        
        BCCornerInfo& blCorner = bottomCorners[colX]; 
        bool hitsSpanBelow = (colX > ajaInfo.mColIndex) &&
                               (colX < ajaInfo.mColIndex + ajaInfo.mColSpan);
        bool update = true;
        if ((colX == info.mColIndex) && (colX > damageArea.x)) {
          int32_t prevRowIndex = lastBottomBorders[colX - 1].rowIndex;
          if (prevRowIndex > info.GetCellEndRowIndex() + 1) {
            
            update = false;
            
          }
          else if (prevRowIndex < info.GetCellEndRowIndex() + 1) {
            
            topCorners[colX] = blCorner;
            blCorner.Set(NS_SIDE_RIGHT, currentBorder);
            update = false;
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
                                            mozilla::css::Side(blCorner.ownerSide),
                                            blCorner.subWidth, blCorner.bevel);
          }
          
          for (int32_t cX = colX + 1; cX < colX + segLength; cX++) {
            BCCornerInfo& corner = bottomCorners[cX];
            corner.Set(NS_SIDE_RIGHT, currentBorder);
            tableCellMap->SetBCBorderCorner(eBottomLeft, *iter.mCellMap,
                                            iter.mRowGroupStart,
                                            info.GetCellEndRowIndex(), cX,
                                            mozilla::css::Side(corner.ownerSide),
                                            corner.subWidth,
                                            false);
          }
        }
        
        startSeg = SetHorBorder(currentBorder, blCorner, lastBottomBorder);
        if (!startSeg) {
           
           
           
           
           startSeg = (lastBottomBorder.rowIndex !=
                       info.GetCellEndRowIndex() + 1);
        }
        lastBottomBorder.rowIndex = info.GetCellEndRowIndex() + 1;
        lastBottomBorder.rowSpan = info.mRowSpan;
        for (int32_t cX = colX; cX < colX + segLength; cX++) {
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
                                                             nullptr;
        info.SetInnerRowGroupBottomContBCBorder(nextRowGroup, ajaInfo.mTopRow);
        gotRowBorder = true;
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
            tableCellMap->ResetTopStart(NS_SIDE_BOTTOM, *iter.mCellMap,
                                        info.GetCellEndRowIndex(),
                                        info.GetCellEndColIndex() + 1);
          }
        }
      }
    }
  } 
  
  SetNeedToCalcBCBorders(false);
  propData->mDamageArea = nsIntRect(0,0,0,0);
#ifdef DEBUG_TABLE_CELLMAP
  mCellMap->Dump();
#endif
}

class BCPaintBorderIterator;

struct BCVerticalSeg
{
  BCVerticalSeg();

  void Start(BCPaintBorderIterator& aIter,
             BCBorderOwner          aBorderOwner,
             BCPixelSize            aVerSegWidth,
             BCPixelSize            aHorSegHeight);

  void Initialize(BCPaintBorderIterator& aIter);
  void GetBottomCorner(BCPaintBorderIterator& aIter,
                       BCPixelSize            aHorSegHeight);


   void Paint(BCPaintBorderIterator& aIter,
              nsRenderingContext&   aRenderingContext,
              BCPixelSize            aHorSegHeight);
  void AdvanceOffsetY();
  void IncludeCurrentBorder(BCPaintBorderIterator& aIter);


  union {
    nsTableColFrame*  mCol;
    int32_t           mColWidth;
  };
  nscoord               mOffsetX;    
  nscoord               mOffsetY;    
  nscoord               mLength;     
  BCPixelSize           mWidth;      

  nsTableCellFrame*     mAjaCell;       
                                        
                                        
  nsTableCellFrame*     mFirstCell;     
  nsTableRowGroupFrame* mFirstRowGroup; 
  nsTableRowFrame*      mFirstRow;      
  nsTableCellFrame*     mLastCell;      
                                        


  uint8_t               mOwner;         
                                        
  mozilla::css::Side    mTopBevelSide;  
  nscoord               mTopBevelOffset; 
  BCPixelSize           mBottomHorSegHeight; 
                                        
  nscoord               mBottomOffset;  
                                        
                                        
                                        
  bool                  mIsBottomBevel; 
};

struct BCHorizontalSeg
{
  BCHorizontalSeg();

  void Start(BCPaintBorderIterator& aIter,
             BCBorderOwner          aBorderOwner,
             BCPixelSize            aBottomVerSegWidth,
             BCPixelSize            aHorSegHeight);
   void GetRightCorner(BCPaintBorderIterator& aIter,
                       BCPixelSize            aLeftSegWidth);
   void AdvanceOffsetX(int32_t aIncrement);
   void IncludeCurrentBorder(BCPaintBorderIterator& aIter);
   void Paint(BCPaintBorderIterator& aIter,
              nsRenderingContext&   aRenderingContext);

  nscoord            mOffsetX;       
  nscoord            mOffsetY;       
  nscoord            mLength;        
  BCPixelSize        mWidth;         
  nscoord            mLeftBevelOffset;   
  mozilla::css::Side mLeftBevelSide;     
  bool               mIsRightBevel;      
  nscoord            mRightBevelOffset;  
  mozilla::css::Side mRightBevelSide;    
  nscoord            mEndOffset;         
                                         
                                         
                                         
  uint8_t            mOwner;             
                                         
  nsTableCellFrame*  mFirstCell;         
  nsTableCellFrame*  mAjaCell;           
                                         
                                         
};




class BCPaintBorderIterator
{
public:


  explicit BCPaintBorderIterator(nsTableFrame* aTable);
  ~BCPaintBorderIterator() { if (mVerInfo) {
                              delete [] mVerInfo;
                           }}
  void Reset();

  





  bool SetDamageArea(const nsRect& aDamageRect);
  void First();
  void Next();
  void AccumulateOrPaintHorizontalSegment(nsRenderingContext& aRenderingContext);
  void AccumulateOrPaintVerticalSegment(nsRenderingContext& aRenderingContext);
  void ResetVerInfo();
  void StoreColumnWidth(int32_t aIndex);
  bool VerticalSegmentOwnsCorner();

  nsTableFrame*         mTable;
  nsTableFrame*         mTableFirstInFlow;
  nsTableCellMap*       mTableCellMap;
  nsCellMap*            mCellMap;
  bool                  mTableIsLTR;
  int32_t               mColInc;            
  const nsStyleBackground* mTableBgColor;
  nsTableFrame::RowGroupArray mRowGroups;

  nsTableRowGroupFrame* mPrevRg;
  nsTableRowGroupFrame* mRg;
  bool                  mIsRepeatedHeader;
  bool                  mIsRepeatedFooter;
  nsTableRowGroupFrame* mStartRg; 
  int32_t               mRgIndex; 
                                        
  int32_t               mFifRgFirstRowIndex; 
                                           
  int32_t               mRgFirstRowIndex; 
                                          
  int32_t               mRgLastRowIndex; 
                                         
  int32_t               mNumTableRows;   
                                         
  int32_t               mNumTableCols;   
  int32_t               mColIndex;       
  int32_t               mRowIndex;       
  int32_t               mRepeatedHeaderRowIndex; 
                                            
                                            
                                            
                                            
                                            
  bool                  mIsNewRow;
  bool                  mAtEnd;             
                                             
  nsTableRowFrame*      mPrevRow;
  nsTableRowFrame*      mRow;
  nsTableRowFrame*      mStartRow;    


  
  nsTableCellFrame*     mPrevCell;
  nsTableCellFrame*     mCell;
  BCCellData*           mPrevCellData;
  BCCellData*           mCellData;
  BCData*               mBCData;

  bool                  IsTableTopMost()    {return (mRowIndex == 0) && !mTable->GetPrevInFlow();}
  bool                  IsTableRightMost()  {return (mColIndex >= mNumTableCols);}
  bool                  IsTableBottomMost() {return (mRowIndex >= mNumTableRows) && !mTable->GetNextInFlow();}
  bool                  IsTableLeftMost()   {return (mColIndex == 0);}
  bool                  IsDamageAreaTopMost()    {return (mRowIndex == mDamageArea.y);}
  bool                  IsDamageAreaRightMost()  {return (mColIndex >= mDamageArea.XMost());}
  bool                  IsDamageAreaBottomMost() {return (mRowIndex >= mDamageArea.YMost());}
  bool                  IsDamageAreaLeftMost()   {return (mColIndex == mDamageArea.x);}
  int32_t               GetRelativeColIndex() {return (mColIndex - mDamageArea.x);}

  nsIntRect             mDamageArea;        
  bool                  IsAfterRepeatedHeader() { return !mIsRepeatedHeader && (mRowIndex == (mRepeatedHeaderRowIndex + 1));}
  bool                  StartRepeatedFooter() {return mIsRepeatedFooter && (mRowIndex == mRgFirstRowIndex) && (mRowIndex != mDamageArea.y);}
  nscoord               mInitialOffsetX;  
                                            
  nscoord               mInitialOffsetY;    
                                            
  nscoord               mNextOffsetY;       
  BCVerticalSeg*        mVerInfo; 
                                  
                                  
                                  
                                  
                                  
                                  
                                  
                                  
                                  
                                  
  BCHorizontalSeg       mHorSeg;            
                                            
  BCPixelSize           mPrevHorSegHeight;  
                                            

private:

  bool SetNewRow(nsTableRowFrame* aRow = nullptr);
  bool SetNewRowGroup();
  void   SetNewData(int32_t aRowIndex, int32_t aColIndex);

};



BCPaintBorderIterator::BCPaintBorderIterator(nsTableFrame* aTable)
{
  mTable      = aTable;
  mVerInfo    = nullptr;
  nsMargin childAreaOffset = mTable->GetChildAreaOffset(nullptr);
  mTableFirstInFlow    = static_cast<nsTableFrame*>(mTable->FirstInFlow());
  mTableCellMap        = mTable->GetCellMap();
  
  mInitialOffsetY = mTable->GetPrevInFlow() ? 0 : childAreaOffset.top;
  mNumTableRows  = mTable->GetRowCount();
  mNumTableCols  = mTable->GetColCount();

  
  mTable->OrderRowGroups(mRowGroups);
  
  mRepeatedHeaderRowIndex = -99;

  mTableIsLTR = mTable->StyleVisibility()->mDirection ==
                   NS_STYLE_DIRECTION_LTR;
  mColInc = (mTableIsLTR) ? 1 : -1;

  nsIFrame* bgFrame =
    nsCSSRendering::FindNonTransparentBackgroundFrame(aTable);
  mTableBgColor = bgFrame->StyleBackground();
}

bool
BCPaintBorderIterator::SetDamageArea(const nsRect& aDirtyRect)
{

  uint32_t startRowIndex, endRowIndex, startColIndex, endColIndex;
  startRowIndex = endRowIndex = startColIndex = endColIndex = 0;
  bool done = false;
  bool haveIntersect = false;
  
  nscoord rowY = mInitialOffsetY;
  for (uint32_t rgX = 0; rgX < mRowGroups.Length() && !done; rgX++) {
    nsTableRowGroupFrame* rgFrame = mRowGroups[rgX];
    for (nsTableRowFrame* rowFrame = rgFrame->GetFirstRow(); rowFrame;
         rowFrame = rowFrame->GetNextRow()) {
      
      nscoord topBorderHalf    = (mTable->GetPrevInFlow()) ? 0 :
       nsPresContext::CSSPixelsToAppUnits(rowFrame->GetTopBCBorderWidth() + 1);
      nscoord bottomBorderHalf = (mTable->GetNextInFlow()) ? 0 :
        nsPresContext::CSSPixelsToAppUnits(rowFrame->GetBottomBCBorderWidth() + 1);
      
      nsSize rowSize = rowFrame->GetSize();
      if (haveIntersect) {
        if (aDirtyRect.YMost() >= (rowY - topBorderHalf)) {
          nsTableRowFrame* fifRow =
            static_cast<nsTableRowFrame*>(rowFrame->FirstInFlow());
          endRowIndex = fifRow->GetRowIndex();
        }
        else done = true;
      }
      else {
        if ((rowY + rowSize.height + bottomBorderHalf) >= aDirtyRect.y) {
          mStartRg  = rgFrame;
          mStartRow = rowFrame;
          nsTableRowFrame* fifRow =
            static_cast<nsTableRowFrame*>(rowFrame->FirstInFlow());
          startRowIndex = endRowIndex = fifRow->GetRowIndex();
          haveIntersect = true;
        }
        else {
          mInitialOffsetY += rowSize.height;
        }
      }
      rowY += rowSize.height;
    }
  }
  mNextOffsetY = mInitialOffsetY;

  
  
  
  
  
  if (!haveIntersect)
    return false;
  
  haveIntersect = false;
  if (0 == mNumTableCols)
    return false;
  int32_t leftCol, rightCol; 

  nsMargin childAreaOffset = mTable->GetChildAreaOffset(nullptr);
  if (mTableIsLTR) {
    mInitialOffsetX = childAreaOffset.left; 
                                            
    leftCol = 0;
    rightCol = mNumTableCols;
  } else {
    
    mInitialOffsetX = mTable->GetRect().width - childAreaOffset.right;
    leftCol = mNumTableCols-1;
    rightCol = -1;
  }
  nscoord x = 0;
  int32_t colX;
  for (colX = leftCol; colX != rightCol; colX += mColInc) {
    nsTableColFrame* colFrame = mTableFirstInFlow->GetColFrame(colX);
    if (!colFrame) ABORT1(false);
    
    nsSize size = colFrame->GetSize();
    if (haveIntersect) {
      
      nscoord leftBorderHalf =
        nsPresContext::CSSPixelsToAppUnits(colFrame->GetLeftBorderWidth() + 1);
      if (aDirtyRect.XMost() >= (x - leftBorderHalf)) {
        endColIndex = colX;
      }
      else break;
    }
    else {
      
      nscoord rightBorderHalf =
        nsPresContext::CSSPixelsToAppUnits(colFrame->GetRightBorderWidth() + 1);
      if ((x + size.width + rightBorderHalf) >= aDirtyRect.x) {
        startColIndex = endColIndex = colX;
        haveIntersect = true;
      }
      else {
        mInitialOffsetX += mColInc * size.width;
      }
    }
    x += size.width;
  }
  if (!mTableIsLTR) {
    uint32_t temp;
    mInitialOffsetX = mTable->GetRect().width - childAreaOffset.right;
    temp = startColIndex; startColIndex = endColIndex; endColIndex = temp;
    for (uint32_t column = 0; column < startColIndex; column++) {
      nsTableColFrame* colFrame = mTableFirstInFlow->GetColFrame(column);
      if (!colFrame) ABORT1(false);
      nsSize size = colFrame->GetSize();
      mInitialOffsetX += mColInc * size.width;
    }
  }
  if (!haveIntersect)
    return false;
  mDamageArea = nsIntRect(startColIndex, startRowIndex,
                          1 + DeprecatedAbs<int32_t>(endColIndex - startColIndex),
                          1 + endRowIndex - startRowIndex);

  Reset();
  mVerInfo = new BCVerticalSeg[mDamageArea.width + 1];
  if (!mVerInfo)
    return false;
  return true;
}

void
BCPaintBorderIterator::Reset()
{
  mAtEnd = true; 
  mRg = mStartRg;
  mPrevRow  = nullptr;
  mRow      = mStartRow;
  mRowIndex      = 0;
  mColIndex      = 0;
  mRgIndex       = -1;
  mPrevCell      = nullptr;
  mCell          = nullptr;
  mPrevCellData  = nullptr;
  mCellData      = nullptr;
  mBCData        = nullptr;
  ResetVerInfo();
}






void
BCPaintBorderIterator::SetNewData(int32_t aY,
                                int32_t aX)
{
  if (!mTableCellMap || !mTableCellMap->mBCInfo) ABORT0();

  mColIndex    = aX;
  mRowIndex    = aY;
  mPrevCellData = mCellData;
  if (IsTableRightMost() && IsTableBottomMost()) {
   mCell = nullptr;
   mBCData = &mTableCellMap->mBCInfo->mLowerRightCorner;
  }
  else if (IsTableRightMost()) {
    mCellData = nullptr;
    mBCData = &mTableCellMap->mBCInfo->mRightBorders.ElementAt(aY);
  }
  else if (IsTableBottomMost()) {
    mCellData = nullptr;
    mBCData = &mTableCellMap->mBCInfo->mBottomBorders.ElementAt(aX);
  }
  else {
    if (uint32_t(mRowIndex - mFifRgFirstRowIndex) < mCellMap->mRows.Length()) {
      mBCData = nullptr;
      mCellData =
        (BCCellData*)mCellMap->mRows[mRowIndex - mFifRgFirstRowIndex].SafeElementAt(mColIndex);
      if (mCellData) {
        mBCData = &mCellData->mData;
        if (!mCellData->IsOrig()) {
          if (mCellData->IsRowSpan()) {
            aY -= mCellData->GetRowSpanOffset();
          }
          if (mCellData->IsColSpan()) {
            aX -= mCellData->GetColSpanOffset();
          }
          if ((aX >= 0) && (aY >= 0)) {
            mCellData = (BCCellData*)mCellMap->mRows[aY - mFifRgFirstRowIndex][aX];
          }
        }
        if (mCellData->IsOrig()) {
          mPrevCell = mCell;
          mCell = mCellData->GetCellFrame();
        }
      }
    }
  }
}






bool
BCPaintBorderIterator::SetNewRow(nsTableRowFrame* aRow)
{
  mPrevRow = mRow;
  mRow     = (aRow) ? aRow : mRow->GetNextRow();
  if (mRow) {
    mIsNewRow = true;
    mRowIndex = mRow->GetRowIndex();
    mColIndex = mDamageArea.x;
    mPrevHorSegHeight = 0;
    if (mIsRepeatedHeader) {
      mRepeatedHeaderRowIndex = mRowIndex;
    }
  }
  else {
    mAtEnd = true;
  }
  return !mAtEnd;
}




bool
BCPaintBorderIterator::SetNewRowGroup()
{

  mRgIndex++;

  mIsRepeatedHeader = false;
  mIsRepeatedFooter = false;

  NS_ASSERTION(mRgIndex >= 0, "mRgIndex out of bounds");
  if (uint32_t(mRgIndex) < mRowGroups.Length()) {
    mPrevRg = mRg;
    mRg = mRowGroups[mRgIndex];
    nsTableRowGroupFrame* fifRg =
      static_cast<nsTableRowGroupFrame*>(mRg->FirstInFlow());
    mFifRgFirstRowIndex = fifRg->GetStartRowIndex();
    mRgFirstRowIndex    = mRg->GetStartRowIndex();
    mRgLastRowIndex     = mRgFirstRowIndex + mRg->GetRowCount() - 1;

    if (SetNewRow(mRg->GetFirstRow())) {
      mCellMap = mTableCellMap->GetMapFor(fifRg, nullptr);
      if (!mCellMap) ABORT1(false);
    }
    if (mRg && mTable->GetPrevInFlow() && !mRg->GetPrevInFlow()) {
      
      
      const nsStyleDisplay* display = mRg->StyleDisplay();
      if (mRowIndex == mDamageArea.y) {
        mIsRepeatedHeader = (NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == display->mDisplay);
      }
      else {
        mIsRepeatedFooter = (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == display->mDisplay);
      }
    }
  }
  else {
    mAtEnd = true;
  }
  return !mAtEnd;
}




void
BCPaintBorderIterator::First()
{
  if (!mTable || (mDamageArea.x >= mNumTableCols) ||
      (mDamageArea.y >= mNumTableRows)) ABORT0();

  mAtEnd = false;

  uint32_t numRowGroups = mRowGroups.Length();
  for (uint32_t rgY = 0; rgY < numRowGroups; rgY++) {
    nsTableRowGroupFrame* rowG = mRowGroups[rgY];
    int32_t start = rowG->GetStartRowIndex();
    int32_t end   = start + rowG->GetRowCount() - 1;
    if ((mDamageArea.y >= start) && (mDamageArea.y <= end)) {
      mRgIndex = rgY - 1; 
      if (SetNewRowGroup()) {
        while ((mRowIndex < mDamageArea.y) && !mAtEnd) {
          SetNewRow();
        }
        if (!mAtEnd) {
          SetNewData(mDamageArea.y, mDamageArea.x);
        }
      }
      return;
    }
  }
  mAtEnd = true;
}




void
BCPaintBorderIterator::Next()
{
  if (mAtEnd) ABORT0();
  mIsNewRow = false;

  mColIndex++;
  if (mColIndex > mDamageArea.XMost()) {
    mRowIndex++;
    if (mRowIndex == mDamageArea.YMost()) {
      mColIndex = mDamageArea.x;
    }
    else if (mRowIndex < mDamageArea.YMost()) {
      if (mRowIndex <= mRgLastRowIndex) {
        SetNewRow();
      }
      else {
        SetNewRowGroup();
      }
    }
    else {
      mAtEnd = true;
    }
  }
  if (!mAtEnd) {
    SetNewData(mRowIndex, mColIndex);
  }
}











static nscoord
CalcVerCornerOffset(mozilla::css::Side aCornerOwnerSide,
                    BCPixelSize aCornerSubWidth,
                    BCPixelSize aHorWidth,
                    bool        aIsStartOfSeg,
                    bool        aIsBevel)
{
  nscoord offset = 0;
  
  BCPixelSize smallHalf, largeHalf;
  if ((NS_SIDE_TOP == aCornerOwnerSide) ||
      (NS_SIDE_BOTTOM == aCornerOwnerSide)) {
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
CalcHorCornerOffset(mozilla::css::Side aCornerOwnerSide,
                    BCPixelSize aCornerSubWidth,
                    BCPixelSize aVerWidth,
                    bool        aIsStartOfSeg,
                    bool        aIsBevel,
                    bool        aTableIsLTR)
{
  nscoord offset = 0;
  
  BCPixelSize smallHalf, largeHalf;
  if ((NS_SIDE_LEFT == aCornerOwnerSide) ||
      (NS_SIDE_RIGHT == aCornerOwnerSide)) {
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

BCVerticalSeg::BCVerticalSeg()
{
  mCol = nullptr;
  mFirstCell = mLastCell = mAjaCell = nullptr;
  mOffsetX = mOffsetY = mLength = mWidth = mTopBevelOffset = 0;
  mTopBevelSide = NS_SIDE_TOP;
  mOwner = eCellOwner;
}









void
BCVerticalSeg::Start(BCPaintBorderIterator& aIter,
                     BCBorderOwner          aBorderOwner,
                     BCPixelSize            aVerSegWidth,
                     BCPixelSize            aHorSegHeight)
{
  mozilla::css::Side ownerSide   = NS_SIDE_TOP;
  bool bevel       = false;


  nscoord cornerSubWidth  = (aIter.mBCData) ?
                               aIter.mBCData->GetCorner(ownerSide, bevel) : 0;

  bool    topBevel        = (aVerSegWidth > 0) ? bevel : false;
  BCPixelSize maxHorSegHeight = std::max(aIter.mPrevHorSegHeight, aHorSegHeight);
  nscoord offset          = CalcVerCornerOffset(ownerSide, cornerSubWidth,
                                                maxHorSegHeight, true,
                                                topBevel);

  mTopBevelOffset = topBevel ?
    nsPresContext::CSSPixelsToAppUnits(maxHorSegHeight): 0;
  
  mTopBevelSide     = (aHorSegHeight > 0) ? NS_SIDE_RIGHT : NS_SIDE_LEFT;
  mOffsetY      += offset;
  mLength        = -offset;
  mWidth         = aVerSegWidth;
  mOwner         = aBorderOwner;
  mFirstCell     = aIter.mCell;
  mFirstRowGroup = aIter.mRg;
  mFirstRow      = aIter.mRow;
  if (aIter.GetRelativeColIndex() > 0) {
    mAjaCell = aIter.mVerInfo[aIter.GetRelativeColIndex() - 1].mLastCell;
  }
}






void
BCVerticalSeg::Initialize(BCPaintBorderIterator& aIter)
{
  int32_t relColIndex = aIter.GetRelativeColIndex();
  mCol = aIter.IsTableRightMost() ? aIter.mVerInfo[relColIndex - 1].mCol :
           aIter.mTableFirstInFlow->GetColFrame(aIter.mColIndex);
  if (!mCol) ABORT0();
  if (0 == relColIndex) {
    mOffsetX = aIter.mInitialOffsetX;
  }
  
  if (!aIter.IsDamageAreaRightMost()) {
    aIter.mVerInfo[relColIndex + 1].mOffsetX = mOffsetX +
                                         aIter.mColInc * mCol->GetSize().width;
  }
  mOffsetY = aIter.mInitialOffsetY;
  mLastCell = aIter.mCell;
}







void
BCVerticalSeg::GetBottomCorner(BCPaintBorderIterator& aIter,
                               BCPixelSize            aHorSegHeight)
{
   mozilla::css::Side ownerSide = NS_SIDE_TOP;
   nscoord cornerSubWidth = 0;
   bool bevel = false;
   if (aIter.mBCData) {
     cornerSubWidth = aIter.mBCData->GetCorner(ownerSide, bevel);
   }
   mIsBottomBevel = (mWidth > 0) ? bevel : false;
   mBottomHorSegHeight = std::max(aIter.mPrevHorSegHeight, aHorSegHeight);
   mBottomOffset = CalcVerCornerOffset(ownerSide, cornerSubWidth,
                                    mBottomHorSegHeight,
                                    false, mIsBottomBevel);
   mLength += mBottomOffset;
}








void
BCVerticalSeg::Paint(BCPaintBorderIterator& aIter,
                     nsRenderingContext&   aRenderingContext,
                     BCPixelSize            aHorSegHeight)
{
  
  mozilla::css::Side side = (aIter.IsDamageAreaRightMost()) ? NS_SIDE_RIGHT :
                                                    NS_SIDE_LEFT;
  int32_t relColIndex = aIter.GetRelativeColIndex();
  nsTableColFrame* col           = mCol; if (!col) ABORT0();
  nsTableCellFrame* cell         = mFirstCell; 
  nsIFrame* owner = nullptr;
  uint8_t style = NS_STYLE_BORDER_STYLE_SOLID;
  nscolor color = 0xFFFFFFFF;

  
  
  int32_t appUnitsPerDevPixel = col->PresContext()->AppUnitsPerDevPixel();

  switch (mOwner) {
    case eTableOwner:
      owner = aIter.mTable;
      break;
    case eAjaColGroupOwner:
      side = NS_SIDE_RIGHT;
      if (!aIter.IsTableRightMost() && (relColIndex > 0)) {
        col = aIter.mVerInfo[relColIndex - 1].mCol;
      } 
    case eColGroupOwner:
      if (col) {
        owner = col->GetParent();
      }
      break;
    case eAjaColOwner:
      side = NS_SIDE_RIGHT;
      if (!aIter.IsTableRightMost() && (relColIndex > 0)) {
        col = aIter.mVerInfo[relColIndex - 1].mCol;
      } 
    case eColOwner:
      owner = col;
      break;
    case eAjaRowGroupOwner:
      NS_ERROR("a neighboring rowgroup can never own a vertical border");
      
    case eRowGroupOwner:
      NS_ASSERTION(aIter.IsTableLeftMost() || aIter.IsTableRightMost(),
                  "row group can own border only at table edge");
      owner = mFirstRowGroup;
      break;
    case eAjaRowOwner:
      NS_ASSERTION(false, "program error"); 
    case eRowOwner:
      NS_ASSERTION(aIter.IsTableLeftMost() || aIter.IsTableRightMost(),
                   "row can own border only at table edge");
      owner = mFirstRow;
      break;
    case eAjaCellOwner:
      side = NS_SIDE_RIGHT;
      cell = mAjaCell; 
    case eCellOwner:
      owner = cell;
      break;
  }
  if (owner) {
    ::GetPaintStyleInfo(owner, side, style, color, aIter.mTableIsLTR);
  }
  BCPixelSize smallHalf, largeHalf;
  DivideBCBorderSize(mWidth, smallHalf, largeHalf);
  nsRect segRect(mOffsetX - nsPresContext::CSSPixelsToAppUnits(largeHalf),
                 mOffsetY,
                 nsPresContext::CSSPixelsToAppUnits(mWidth), mLength);
  nscoord bottomBevelOffset = (mIsBottomBevel) ?
                  nsPresContext::CSSPixelsToAppUnits(mBottomHorSegHeight) : 0;
  mozilla::css::Side bottomBevelSide = ((aHorSegHeight > 0) ^ !aIter.mTableIsLTR) ?
                            NS_SIDE_RIGHT : NS_SIDE_LEFT;
  mozilla::css::Side topBevelSide = ((mTopBevelSide == NS_SIDE_RIGHT) ^ !aIter.mTableIsLTR)?
                         NS_SIDE_RIGHT : NS_SIDE_LEFT;
  nsCSSRendering::DrawTableBorderSegment(aRenderingContext, style, color,
                                         aIter.mTableBgColor, segRect,
                                         appUnitsPerDevPixel,
                                         nsPresContext::AppUnitsPerCSSPixel(),
                                         topBevelSide, mTopBevelOffset,
                                         bottomBevelSide, bottomBevelOffset);
}




void
BCVerticalSeg::AdvanceOffsetY()
{
  mOffsetY +=  mLength - mBottomOffset;
}




void
BCVerticalSeg::IncludeCurrentBorder(BCPaintBorderIterator& aIter)
{
  mLastCell = aIter.mCell;
  mLength  += aIter.mRow->GetRect().height;
}

BCHorizontalSeg::BCHorizontalSeg()
{
  mOffsetX = mOffsetY = mLength = mWidth =  mLeftBevelOffset = 0;
  mLeftBevelSide = NS_SIDE_TOP;
  mFirstCell = mAjaCell = nullptr;
}







void
BCHorizontalSeg::Start(BCPaintBorderIterator& aIter,
                       BCBorderOwner        aBorderOwner,
                       BCPixelSize          aBottomVerSegWidth,
                       BCPixelSize          aHorSegHeight)
{
  mozilla::css::Side cornerOwnerSide = NS_SIDE_TOP;
  bool bevel     = false;

  mOwner = aBorderOwner;
  nscoord cornerSubWidth  = (aIter.mBCData) ?
                             aIter.mBCData->GetCorner(cornerOwnerSide,
                                                       bevel) : 0;

  bool    leftBevel = (aHorSegHeight > 0) ? bevel : false;
  int32_t relColIndex = aIter.GetRelativeColIndex();
  nscoord maxVerSegWidth = std::max(aIter.mVerInfo[relColIndex].mWidth,
                                  aBottomVerSegWidth);
  nscoord offset = CalcHorCornerOffset(cornerOwnerSide, cornerSubWidth,
                                       maxVerSegWidth, true, leftBevel,
                                       aIter.mTableIsLTR);
  mLeftBevelOffset = (leftBevel && (aHorSegHeight > 0)) ? maxVerSegWidth : 0;
  
  mLeftBevelSide   = (aBottomVerSegWidth > 0) ? NS_SIDE_BOTTOM : NS_SIDE_TOP;
  if (aIter.mTableIsLTR) {
    mOffsetX += offset;
  }
  else {
    mOffsetX -= offset;
  }
  mLength          = -offset;
  mWidth           = aHorSegHeight;
  mFirstCell       = aIter.mCell;
  mAjaCell         = (aIter.IsDamageAreaTopMost()) ? nullptr :
                     aIter.mVerInfo[relColIndex].mLastCell;
}







void
BCHorizontalSeg::GetRightCorner(BCPaintBorderIterator& aIter,
                                BCPixelSize            aLeftSegWidth)
{
  mozilla::css::Side ownerSide = NS_SIDE_TOP;
  nscoord cornerSubWidth = 0;
  bool bevel = false;
  if (aIter.mBCData) {
    cornerSubWidth = aIter.mBCData->GetCorner(ownerSide, bevel);
  }

  mIsRightBevel = (mWidth > 0) ? bevel : 0;
  int32_t relColIndex = aIter.GetRelativeColIndex();
  nscoord verWidth = std::max(aIter.mVerInfo[relColIndex].mWidth, aLeftSegWidth);
  mEndOffset = CalcHorCornerOffset(ownerSide, cornerSubWidth, verWidth,
                                   false, mIsRightBevel, aIter.mTableIsLTR);
  mLength += mEndOffset;
  mRightBevelOffset = (mIsRightBevel) ?
                       nsPresContext::CSSPixelsToAppUnits(verWidth) : 0;
  mRightBevelSide = (aLeftSegWidth > 0) ? NS_SIDE_BOTTOM : NS_SIDE_TOP;
}






void
BCHorizontalSeg::Paint(BCPaintBorderIterator& aIter,
                       nsRenderingContext&   aRenderingContext)
{
  
  mozilla::css::Side side = (aIter.IsDamageAreaBottomMost()) ? NS_SIDE_BOTTOM :
                                                     NS_SIDE_TOP;
  nsIFrame* rg   = aIter.mRg;  if (!rg) ABORT0();
  nsIFrame* row  = aIter.mRow; if (!row) ABORT0();
  nsIFrame* cell = mFirstCell;
  nsIFrame* col;
  nsIFrame* owner = nullptr;

  
  
  int32_t appUnitsPerDevPixel = row->PresContext()->AppUnitsPerDevPixel();

  uint8_t style = NS_STYLE_BORDER_STYLE_SOLID;
  nscolor color = 0xFFFFFFFF;


  switch (mOwner) {
    case eTableOwner:
      owner = aIter.mTable;
      break;
    case eAjaColGroupOwner:
      NS_ERROR("neighboring colgroups can never own a horizontal border");
      
    case eColGroupOwner:
      NS_ASSERTION(aIter.IsTableTopMost() || aIter.IsTableBottomMost(),
                   "col group can own border only at the table edge");
      col = aIter.mTableFirstInFlow->GetColFrame(aIter.mColIndex - 1);
      if (!col) ABORT0();
      owner = col->GetParent();
      break;
    case eAjaColOwner:
      NS_ERROR("neighboring column can never own a horizontal border");
      
    case eColOwner:
      NS_ASSERTION(aIter.IsTableTopMost() || aIter.IsTableBottomMost(),
                   "col can own border only at the table edge");
      owner = aIter.mTableFirstInFlow->GetColFrame(aIter.mColIndex - 1);
      break;
    case eAjaRowGroupOwner:
      side = NS_SIDE_BOTTOM;
      rg = (aIter.IsTableBottomMost()) ? aIter.mRg : aIter.mPrevRg;
      
    case eRowGroupOwner:
      owner = rg;
      break;
    case eAjaRowOwner:
      side = NS_SIDE_BOTTOM;
      row = (aIter.IsTableBottomMost()) ? aIter.mRow : aIter.mPrevRow;
      
      case eRowOwner:
      owner = row;
      break;
    case eAjaCellOwner:
      side = NS_SIDE_BOTTOM;
      
      
      cell = mAjaCell;
      
    case eCellOwner:
      owner = cell;
      break;
  }
  if (owner) {
    ::GetPaintStyleInfo(owner, side, style, color, aIter.mTableIsLTR);
  }
  BCPixelSize smallHalf, largeHalf;
  DivideBCBorderSize(mWidth, smallHalf, largeHalf);
  nsRect segRect(mOffsetX,
                 mOffsetY - nsPresContext::CSSPixelsToAppUnits(largeHalf),
                 mLength,
                 nsPresContext::CSSPixelsToAppUnits(mWidth));
  if (aIter.mTableIsLTR) {
    nsCSSRendering::DrawTableBorderSegment(aRenderingContext, style, color,
                                           aIter.mTableBgColor, segRect,
                                           appUnitsPerDevPixel,
                                           nsPresContext::AppUnitsPerCSSPixel(),
                                           mLeftBevelSide,
                                           nsPresContext::CSSPixelsToAppUnits(mLeftBevelOffset),
                                           mRightBevelSide, mRightBevelOffset);
  }
  else {
    segRect.x -= segRect.width;
    nsCSSRendering::DrawTableBorderSegment(aRenderingContext, style, color,
                                           aIter.mTableBgColor, segRect,
                                           appUnitsPerDevPixel,
                                           nsPresContext::AppUnitsPerCSSPixel(),
                                           mRightBevelSide, mRightBevelOffset,
                                           mLeftBevelSide,
                                           nsPresContext::CSSPixelsToAppUnits(mLeftBevelOffset));
  }
}




void
BCHorizontalSeg::AdvanceOffsetX(int32_t aIncrement)
{
  mOffsetX += aIncrement * (mLength - mEndOffset);
}




void
BCHorizontalSeg::IncludeCurrentBorder(BCPaintBorderIterator& aIter)
{
  mLength += aIter.mVerInfo[aIter.GetRelativeColIndex()].mColWidth;
}




void
BCPaintBorderIterator::StoreColumnWidth(int32_t aIndex)
{
  if (IsTableRightMost()) {
      mVerInfo[aIndex].mColWidth = mVerInfo[aIndex - 1].mColWidth;
  }
  else {
    nsTableColFrame* col = mTableFirstInFlow->GetColFrame(mColIndex);
    if (!col) ABORT0();
    mVerInfo[aIndex].mColWidth = col->GetSize().width;
  }
}



bool
BCPaintBorderIterator::VerticalSegmentOwnsCorner()
{
  mozilla::css::Side cornerOwnerSide = NS_SIDE_TOP;
  bool bevel = false;
  if (mBCData) {
    mBCData->GetCorner(cornerOwnerSide, bevel);
  }
  
  return  (NS_SIDE_TOP == cornerOwnerSide) ||
          (NS_SIDE_BOTTOM == cornerOwnerSide);
}





void
BCPaintBorderIterator::AccumulateOrPaintHorizontalSegment(nsRenderingContext& aRenderingContext)
{

  int32_t relColIndex = GetRelativeColIndex();
  
  if (mVerInfo[relColIndex].mColWidth < 0) {
    StoreColumnWidth(relColIndex);
  }

  BCBorderOwner borderOwner = eCellOwner;
  BCBorderOwner ignoreBorderOwner;
  bool isSegStart = true;
  bool ignoreSegStart;

  nscoord leftSegWidth =
    mBCData ? mBCData->GetLeftEdge(ignoreBorderOwner, ignoreSegStart) : 0;
  nscoord topSegHeight =
    mBCData ? mBCData->GetTopEdge(borderOwner, isSegStart) : 0;

  if (mIsNewRow || (IsDamageAreaLeftMost() && IsDamageAreaBottomMost())) {
    
    mHorSeg.mOffsetY = mNextOffsetY;
    mNextOffsetY     = mNextOffsetY + mRow->GetSize().height;
    mHorSeg.mOffsetX = mInitialOffsetX;
    mHorSeg.Start(*this, borderOwner, leftSegWidth, topSegHeight);
  }

  if (!IsDamageAreaLeftMost() && (isSegStart || IsDamageAreaRightMost() ||
                                  VerticalSegmentOwnsCorner())) {
    
    if (mHorSeg.mLength > 0) {
      mHorSeg.GetRightCorner(*this, leftSegWidth);
      if (mHorSeg.mWidth > 0) {
        mHorSeg.Paint(*this, aRenderingContext);
      }
      mHorSeg.AdvanceOffsetX(mColInc);
    }
    mHorSeg.Start(*this, borderOwner, leftSegWidth, topSegHeight);
  }
  mHorSeg.IncludeCurrentBorder(*this);
  mVerInfo[relColIndex].mWidth = leftSegWidth;
  mVerInfo[relColIndex].mLastCell = mCell;
}




void
BCPaintBorderIterator::AccumulateOrPaintVerticalSegment(nsRenderingContext& aRenderingContext)
{
  BCBorderOwner borderOwner = eCellOwner;
  BCBorderOwner ignoreBorderOwner;
  bool isSegStart = true;
  bool ignoreSegStart;

  nscoord verSegWidth  =
    mBCData ? mBCData->GetLeftEdge(borderOwner, isSegStart) : 0;
  nscoord horSegHeight =
    mBCData ? mBCData->GetTopEdge(ignoreBorderOwner, ignoreSegStart) : 0;

  int32_t relColIndex = GetRelativeColIndex();
  BCVerticalSeg& verSeg = mVerInfo[relColIndex];
  if (!verSeg.mCol) { 
                      
    verSeg.Initialize(*this);
    verSeg.Start(*this, borderOwner, verSegWidth, horSegHeight);
  }

  if (!IsDamageAreaTopMost() && (isSegStart || IsDamageAreaBottomMost() ||
                                 IsAfterRepeatedHeader() ||
                                 StartRepeatedFooter())) {
    
    if (verSeg.mLength > 0) {
      verSeg.GetBottomCorner(*this, horSegHeight);
      if (verSeg.mWidth > 0) {
        verSeg.Paint(*this, aRenderingContext, horSegHeight);
      }
      verSeg.AdvanceOffsetY();
    }
    verSeg.Start(*this, borderOwner, verSegWidth, horSegHeight);
  }
  verSeg.IncludeCurrentBorder(*this);
  mPrevHorSegHeight = horSegHeight;
}




void
BCPaintBorderIterator::ResetVerInfo()
{
  if (mVerInfo) {
    memset(mVerInfo, 0, mDamageArea.width * sizeof(BCVerticalSeg));
    
    for (int32_t xIndex = 0; xIndex < mDamageArea.width; xIndex++) {
      mVerInfo[xIndex].mColWidth = -1;
    }
  }
}







void
nsTableFrame::PaintBCBorders(nsRenderingContext& aRenderingContext,
                             const nsRect&        aDirtyRect)
{
  
  
  BCPaintBorderIterator iter(this);
  if (!iter.SetDamageArea(aDirtyRect))
    return;

  
  
  
  
  
  
  
  
  
  for (iter.First(); !iter.mAtEnd; iter.Next()) {
    iter.AccumulateOrPaintVerticalSegment(aRenderingContext);
  }

  
  
  
  iter.Reset();
  for (iter.First(); !iter.mAtEnd; iter.Next()) {
    iter.AccumulateOrPaintHorizontalSegment(aRenderingContext);
  }
}

bool nsTableFrame::RowHasSpanningCells(int32_t aRowIndex, int32_t aNumEffCols)
{
  bool result = false;
  nsTableCellMap* cellMap = GetCellMap();
  NS_PRECONDITION (cellMap, "bad call, cellMap not yet allocated.");
  if (cellMap) {
    result = cellMap->RowHasSpanningCells(aRowIndex, aNumEffCols);
  }
  return result;
}

bool nsTableFrame::RowIsSpannedInto(int32_t aRowIndex, int32_t aNumEffCols)
{
  bool result = false;
  nsTableCellMap* cellMap = GetCellMap();
  NS_PRECONDITION (cellMap, "bad call, cellMap not yet allocated.");
  if (cellMap) {
    result = cellMap->RowIsSpannedInto(aRowIndex, aNumEffCols);
  }
  return result;
}


void
nsTableFrame::InvalidateTableFrame(nsIFrame* aFrame,
                                   const nsRect& aOrigRect,
                                   const nsRect& aOrigVisualOverflow,
                                   bool aIsFirstReflow)
{
  nsIFrame* parent = aFrame->GetParent();
  NS_ASSERTION(parent, "What happened here?");

  if (parent->GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    
    
    return;
  }

  
  
  
  
  
  
  nsRect visualOverflow = aFrame->GetVisualOverflowRect();
  if (aIsFirstReflow ||
      aOrigRect.TopLeft() != aFrame->GetPosition() ||
      aOrigVisualOverflow.TopLeft() != visualOverflow.TopLeft()) {
    
    
    
    
    
    
    aFrame->InvalidateFrame();
    parent->InvalidateFrameWithRect(aOrigVisualOverflow + aOrigRect.TopLeft());
  } else if (aOrigRect.Size() != aFrame->GetSize() ||
             aOrigVisualOverflow.Size() != visualOverflow.Size()){
    aFrame->InvalidateFrameWithRect(aOrigVisualOverflow);
    aFrame->InvalidateFrame();
    parent->InvalidateFrameWithRect(aOrigRect);;
    parent->InvalidateFrame();
  }
}
