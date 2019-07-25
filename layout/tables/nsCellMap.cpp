




































#include "nsTArray.h"
#include "nsCellMap.h"
#include "nsTableFrame.h"
#include "nsTableCellFrame.h"
#include "nsTableRowGroupFrame.h"


static void
SetDamageArea(PRInt32 aXOrigin,
              PRInt32 aYOrigin,
              PRInt32 aWidth,
              PRInt32 aHeight,
              nsRect& aDamageArea)
{
  NS_ASSERTION(aXOrigin >= 0, "negative col index");
  NS_ASSERTION(aYOrigin >= 0, "negative row index");
  NS_ASSERTION(aWidth >= 0, "negative horizontal damage");
  NS_ASSERTION(aHeight >= 0, "negative vertical damage");
  aDamageArea.x      = aXOrigin;
  aDamageArea.y      = aYOrigin;
  aDamageArea.width  = aWidth;
  aDamageArea.height = aHeight;
}
 

static nsCellMap::CellDataArray * sEmptyRow;



CellData::CellData(nsTableCellFrame* aOrigCell)
{
  MOZ_COUNT_CTOR(CellData);
  mOrigCell = aOrigCell;
}

CellData::~CellData()
{
  MOZ_COUNT_DTOR(CellData);
}

BCCellData::BCCellData(nsTableCellFrame* aOrigCell)
:CellData(aOrigCell)
{
  MOZ_COUNT_CTOR(BCCellData);
}

BCCellData::~BCCellData()
{
  MOZ_COUNT_DTOR(BCCellData);
}



nsTableCellMap::nsTableCellMap(nsTableFrame&   aTableFrame,
                               bool            aBorderCollapse)
:mTableFrame(aTableFrame), mFirstMap(nsnull), mBCInfo(nsnull)
{
  MOZ_COUNT_CTOR(nsTableCellMap);

  nsTableFrame::RowGroupArray orderedRowGroups;
  aTableFrame.OrderRowGroups(orderedRowGroups);

  nsTableRowGroupFrame* prior = nsnull;
  for (PRUint32 rgX = 0; rgX < orderedRowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = orderedRowGroups[rgX];
    InsertGroupCellMap(rgFrame, prior);
    prior = rgFrame;
  }
  if (aBorderCollapse) {
    mBCInfo = new BCInfo();
  }
}

nsTableCellMap::~nsTableCellMap()
{
  MOZ_COUNT_DTOR(nsTableCellMap);

  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    nsCellMap* next = cellMap->GetNextSibling();
    delete cellMap;
    cellMap = next;
  }

  if (mBCInfo) {
    DeleteRightBottomBorders();
    delete mBCInfo;
  }
}


BCData*
nsTableCellMap::GetRightMostBorder(PRInt32 aRowIndex)
{
  if (!mBCInfo) ABORT1(nsnull);

  PRInt32 numRows = mBCInfo->mRightBorders.Length();
  if (aRowIndex < numRows) {
    return &mBCInfo->mRightBorders.ElementAt(aRowIndex);
  }

  if (!mBCInfo->mRightBorders.SetLength(aRowIndex+1))
    ABORT1(nsnull);
  return &mBCInfo->mRightBorders.ElementAt(aRowIndex);
}


BCData*
nsTableCellMap::GetBottomMostBorder(PRInt32 aColIndex)
{
  if (!mBCInfo) ABORT1(nsnull);

  PRInt32 numCols = mBCInfo->mBottomBorders.Length();
  if (aColIndex < numCols) {
    return &mBCInfo->mBottomBorders.ElementAt(aColIndex);
  }

  if (!mBCInfo->mBottomBorders.SetLength(aColIndex+1))
    ABORT1(nsnull);
  return &mBCInfo->mBottomBorders.ElementAt(aColIndex);
}


void
nsTableCellMap::DeleteRightBottomBorders()
{
  if (mBCInfo) {
    mBCInfo->mBottomBorders.Clear();
    mBCInfo->mRightBorders.Clear();
  }
}

void
nsTableCellMap::InsertGroupCellMap(nsCellMap* aPrevMap,
                                   nsCellMap& aNewMap)
{
  nsCellMap* next;
  if (aPrevMap) {
    next = aPrevMap->GetNextSibling();
    aPrevMap->SetNextSibling(&aNewMap);
  }
  else {
    next = mFirstMap;
    mFirstMap = &aNewMap;
  }
  aNewMap.SetNextSibling(next);
}

void nsTableCellMap::InsertGroupCellMap(nsTableRowGroupFrame*  aNewGroup,
                                        nsTableRowGroupFrame*& aPrevGroup)
{
  nsCellMap* newMap = new nsCellMap(aNewGroup, mBCInfo != nsnull);
  nsCellMap* prevMap = nsnull;
  nsCellMap* lastMap = mFirstMap;
  if (aPrevGroup) {
    nsCellMap* map = mFirstMap;
    while (map) {
      lastMap = map;
      if (map->GetRowGroup() == aPrevGroup) {
        prevMap = map;
        break;
      }
      map = map->GetNextSibling();
    }
  }
  if (!prevMap) {
    if (aPrevGroup) {
      prevMap = lastMap;
      aPrevGroup = (prevMap) ? prevMap->GetRowGroup() : nsnull;
    }
    else {
      aPrevGroup = nsnull;
    }
  }
  InsertGroupCellMap(prevMap, *newMap);
}

void nsTableCellMap::RemoveGroupCellMap(nsTableRowGroupFrame* aGroup)
{
  nsCellMap* map = mFirstMap;
  nsCellMap* prior = nsnull;
  while (map) {
    if (map->GetRowGroup() == aGroup) {
      nsCellMap* next = map->GetNextSibling();
      if (mFirstMap == map) {
        mFirstMap = next;
      }
      else {
        prior->SetNextSibling(next);
      }
      delete map;
      break;
    }
    prior = map;
    map = map->GetNextSibling();
  }
}

static nsCellMap*
FindMapFor(const nsTableRowGroupFrame* aRowGroup,
           nsCellMap* aStart,
           const nsCellMap* aEnd)
{
  for (nsCellMap* map = aStart; map != aEnd; map = map->GetNextSibling()) {
    if (aRowGroup == map->GetRowGroup()) {
      return map;
    }
  }

  return nsnull;
}

nsCellMap*
nsTableCellMap::GetMapFor(const nsTableRowGroupFrame* aRowGroup,
                          nsCellMap* aStartHint) const
{
  NS_PRECONDITION(aRowGroup, "Must have a rowgroup");
  NS_ASSERTION(!aRowGroup->GetPrevInFlow(), "GetMapFor called with continuation");
  if (aStartHint) {
    nsCellMap* map = FindMapFor(aRowGroup, aStartHint, nsnull);
    if (map) {
      return map;
    }
  }

  nsCellMap* map = FindMapFor(aRowGroup, mFirstMap, aStartHint);
  if (map) {
    return map;
  }

  
  if (aRowGroup->IsRepeatable()) {
    nsTableFrame* fifTable = static_cast<nsTableFrame*>(mTableFrame.GetFirstInFlow());

    const nsStyleDisplay* display = aRowGroup->GetStyleDisplay();
    nsTableRowGroupFrame* rgOrig =
      (NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == display->mDisplay) ?
      fifTable->GetTHead() : fifTable->GetTFoot();
    
    if (rgOrig && rgOrig != aRowGroup) {
      return GetMapFor(rgOrig, aStartHint);
    }
  }

  return nsnull;
}

void
nsTableCellMap::Synchronize(nsTableFrame* aTableFrame)
{
  nsTableFrame::RowGroupArray orderedRowGroups;
  nsAutoTArray<nsCellMap*, 8> maps;

  aTableFrame->OrderRowGroups(orderedRowGroups);
  if (!orderedRowGroups.Length()) {
    return;
  }

  
  
  

  
  nsCellMap* map = nsnull;
  for (PRUint32 rgX = 0; rgX < orderedRowGroups.Length(); rgX++) {
    nsTableRowGroupFrame* rgFrame = orderedRowGroups[rgX];
    map = GetMapFor((nsTableRowGroupFrame*)rgFrame->GetFirstInFlow(), map);
    if (map) {
      if (!maps.AppendElement(map)) {
        delete map;
        map = nsnull;
        NS_WARNING("Could not AppendElement");
        break;
      }
    }
  }

  PRInt32 mapIndex = maps.Length() - 1;  
  nsCellMap* nextMap = maps.ElementAt(mapIndex);
  nextMap->SetNextSibling(nsnull);
  for (mapIndex-- ; mapIndex >= 0; mapIndex--) {
    nsCellMap* map = maps.ElementAt(mapIndex);
    map->SetNextSibling(nextMap);
    nextMap = map;
  }
  mFirstMap = nextMap;
}

bool
nsTableCellMap::HasMoreThanOneCell(PRInt32 aRowIndex) const
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* map = mFirstMap;
  while (map) {
    if (map->GetRowCount() > rowIndex) {
      return map->HasMoreThanOneCell(rowIndex);
    }
    rowIndex -= map->GetRowCount();
    map = map->GetNextSibling();
  }
  return false;
}

PRInt32
nsTableCellMap::GetNumCellsOriginatingInRow(PRInt32 aRowIndex) const
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* map = mFirstMap;
  while (map) {
    if (map->GetRowCount() > rowIndex) {
      return map->GetNumCellsOriginatingInRow(rowIndex);
    }
    rowIndex -= map->GetRowCount();
    map = map->GetNextSibling();
  }
  return 0;
}
PRInt32
nsTableCellMap::GetEffectiveRowSpan(PRInt32 aRowIndex,
                                    PRInt32 aColIndex) const
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* map = mFirstMap;
  while (map) {
    if (map->GetRowCount() > rowIndex) {
      return map->GetRowSpan(rowIndex, aColIndex, true);
    }
    rowIndex -= map->GetRowCount();
    map = map->GetNextSibling();
  }
  NS_NOTREACHED("Bogus row index?");
  return 0;
}

PRInt32
nsTableCellMap::GetEffectiveColSpan(PRInt32 aRowIndex,
                                    PRInt32 aColIndex) const
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* map = mFirstMap;
  while (map) {
    if (map->GetRowCount() > rowIndex) {
      bool zeroColSpan;
      return map->GetEffectiveColSpan(*this, rowIndex, aColIndex, zeroColSpan);
    }
    rowIndex -= map->GetRowCount();
    map = map->GetNextSibling();
  }
  NS_NOTREACHED("Bogus row index?");
  return 0;
}

nsTableCellFrame*
nsTableCellMap::GetCellFrame(PRInt32   aRowIndex,
                             PRInt32   aColIndex,
                             CellData& aData,
                             bool      aUseRowIfOverlap) const
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* map = mFirstMap;
  while (map) {
    if (map->GetRowCount() > rowIndex) {
      return map->GetCellFrame(rowIndex, aColIndex, aData, aUseRowIfOverlap);
    }
    rowIndex -= map->GetRowCount();
    map = map->GetNextSibling();
  }
  return nsnull;
}

nsColInfo*
nsTableCellMap::GetColInfoAt(PRInt32 aColIndex)
{
  PRInt32 numColsToAdd = aColIndex + 1 - mCols.Length();
  if (numColsToAdd > 0) {
    AddColsAtEnd(numColsToAdd);  
  }
  return &mCols.ElementAt(aColIndex);
}

PRInt32
nsTableCellMap::GetRowCount() const
{
  PRInt32 numRows = 0;
  nsCellMap* map = mFirstMap;
  while (map) {
    numRows += map->GetRowCount();
    map = map->GetNextSibling();
  }
  return numRows;
}

CellData*
nsTableCellMap::GetDataAt(PRInt32 aRowIndex,
                          PRInt32 aColIndex) const
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* map = mFirstMap;
  while (map) {
    if (map->GetRowCount() > rowIndex) {
      return map->GetDataAt(rowIndex, aColIndex);
    }
    rowIndex -= map->GetRowCount();
    map = map->GetNextSibling();
  }
  return nsnull;
}

void
nsTableCellMap::AddColsAtEnd(PRUint32 aNumCols)
{
  if (!mCols.AppendElements(aNumCols)) {
    NS_WARNING("Could not AppendElement");
  }
  if (mBCInfo) {
    if (!mBCInfo->mBottomBorders.AppendElements(aNumCols)) {
      NS_WARNING("Could not AppendElement");
    }
  }
}

void
nsTableCellMap::RemoveColsAtEnd()
{
  
  
  PRInt32 numCols = GetColCount();
  PRInt32 lastGoodColIndex = mTableFrame.GetIndexOfLastRealCol();
  for (PRInt32 colX = numCols - 1; (colX >= 0) && (colX > lastGoodColIndex); colX--) {
    nsColInfo& colInfo = mCols.ElementAt(colX);
    if ((colInfo.mNumCellsOrig <= 0) && (colInfo.mNumCellsSpan <= 0))  {
      mCols.RemoveElementAt(colX);

      if (mBCInfo) {
        PRInt32 count = mBCInfo->mBottomBorders.Length();
        if (colX < count) {
          mBCInfo->mBottomBorders.RemoveElementAt(colX);
        }
      }
    }
    else break; 
  }
}

void
nsTableCellMap::ClearCols()
{
  mCols.Clear();
  if (mBCInfo)
    mBCInfo->mBottomBorders.Clear();
}
void
nsTableCellMap::InsertRows(nsTableRowGroupFrame*       aParent,
                           nsTArray<nsTableRowFrame*>& aRows,
                           PRInt32                     aFirstRowIndex,
                           bool                        aConsiderSpans,
                           nsRect&                     aDamageArea)
{
  PRInt32 numNewRows = aRows.Length();
  if ((numNewRows <= 0) || (aFirstRowIndex < 0)) ABORT0();

  PRInt32 rowIndex = aFirstRowIndex;
  PRInt32 rgStartRowIndex = 0;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    nsTableRowGroupFrame* rg = cellMap->GetRowGroup();
    if (rg == aParent) {
      cellMap->InsertRows(*this, aRows, rowIndex, aConsiderSpans,
                          rgStartRowIndex, aDamageArea);
#ifdef DEBUG_TABLE_CELLMAP
      Dump("after InsertRows");
#endif
      if (mBCInfo) {
        PRInt32 count = mBCInfo->mRightBorders.Length();
        if (aFirstRowIndex < count) {
          for (PRInt32 rowX = aFirstRowIndex; rowX < aFirstRowIndex + numNewRows; rowX++) {
            if (!mBCInfo->mRightBorders.InsertElementAt(rowX))
              ABORT0();
          }
        }
        else {
          GetRightMostBorder(aFirstRowIndex); 
          for (PRInt32 rowX = aFirstRowIndex + 1; rowX < aFirstRowIndex + numNewRows; rowX++) {
            if (!mBCInfo->mRightBorders.AppendElement())
              ABORT0();
          }
        }
      }
      return;
    }
    PRInt32 rowCount = cellMap->GetRowCount();
    rgStartRowIndex += rowCount;
    rowIndex -= rowCount;
    cellMap = cellMap->GetNextSibling();
  }

  NS_ERROR("Attempt to insert row into wrong map.");
}

void
nsTableCellMap::RemoveRows(PRInt32         aFirstRowIndex,
                           PRInt32         aNumRowsToRemove,
                           bool            aConsiderSpans,
                           nsRect&         aDamageArea)
{
  PRInt32 rowIndex = aFirstRowIndex;
  PRInt32 rgStartRowIndex = 0;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    PRInt32 rowCount = cellMap->GetRowCount();
    if (rowCount > rowIndex) {
      cellMap->RemoveRows(*this, rowIndex, aNumRowsToRemove, aConsiderSpans,
                          rgStartRowIndex, aDamageArea);
      if (mBCInfo) {
        for (PRInt32 rowX = aFirstRowIndex + aNumRowsToRemove - 1; rowX >= aFirstRowIndex; rowX--) {
          if (PRUint32(rowX) < mBCInfo->mRightBorders.Length()) {
            mBCInfo->mRightBorders.RemoveElementAt(rowX);
          }
        }
      }
      break;
    }
    rgStartRowIndex += rowCount;
    rowIndex -= rowCount;
    cellMap = cellMap->GetNextSibling();
  }
#ifdef DEBUG_TABLE_CELLMAP
  Dump("after RemoveRows");
#endif
}



CellData*
nsTableCellMap::AppendCell(nsTableCellFrame& aCellFrame,
                           PRInt32           aRowIndex,
                           bool              aRebuildIfNecessary,
                           nsRect&           aDamageArea)
{
  NS_ASSERTION(&aCellFrame == aCellFrame.GetFirstInFlow(), "invalid call on continuing frame");
  nsIFrame* rgFrame = aCellFrame.GetParent(); 
  if (!rgFrame) return 0;
  rgFrame = rgFrame->GetParent();   
  if (!rgFrame) return 0;

  CellData* result = nsnull;
  PRInt32 rowIndex = aRowIndex;
  PRInt32 rgStartRowIndex = 0;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->GetRowGroup() == rgFrame) {
      result = cellMap->AppendCell(*this, &aCellFrame, rowIndex,
                                   aRebuildIfNecessary, rgStartRowIndex,
                                   aDamageArea);
      break;
    }
    PRInt32 rowCount = cellMap->GetRowCount();
    rgStartRowIndex += rowCount;
    rowIndex -= rowCount;
    cellMap = cellMap->GetNextSibling();
  }
#ifdef DEBUG_TABLE_CELLMAP
  Dump("after AppendCell");
#endif
  return result;
}


void
nsTableCellMap::InsertCells(nsTArray<nsTableCellFrame*>& aCellFrames,
                            PRInt32                      aRowIndex,
                            PRInt32                      aColIndexBefore,
                            nsRect&                      aDamageArea)
{
  PRInt32 rowIndex = aRowIndex;
  PRInt32 rgStartRowIndex = 0;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    PRInt32 rowCount = cellMap->GetRowCount();
    if (rowCount > rowIndex) {
      cellMap->InsertCells(*this, aCellFrames, rowIndex, aColIndexBefore,
                           rgStartRowIndex, aDamageArea);
      break;
    }
    rgStartRowIndex += rowCount;
    rowIndex -= rowCount;
    cellMap = cellMap->GetNextSibling();
  }
#ifdef DEBUG_TABLE_CELLMAP
  Dump("after InsertCells");
#endif
}


void
nsTableCellMap::RemoveCell(nsTableCellFrame* aCellFrame,
                           PRInt32           aRowIndex,
                           nsRect&           aDamageArea)
{
  if (!aCellFrame) ABORT0();
  NS_ASSERTION(aCellFrame == (nsTableCellFrame *)aCellFrame->GetFirstInFlow(),
               "invalid call on continuing frame");
  PRInt32 rowIndex = aRowIndex;
  PRInt32 rgStartRowIndex = 0;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    PRInt32 rowCount = cellMap->GetRowCount();
    if (rowCount > rowIndex) {
      cellMap->RemoveCell(*this, aCellFrame, rowIndex, rgStartRowIndex,
                          aDamageArea);
#ifdef DEBUG_TABLE_CELLMAP
      Dump("after RemoveCell");
#endif
      return;
    }
    rgStartRowIndex += rowCount;
    rowIndex -= rowCount;
    cellMap = cellMap->GetNextSibling();
  }
  
  
  
  
  NS_ERROR("nsTableCellMap::RemoveCell - could not remove cell");
}

void
nsTableCellMap::RebuildConsideringCells(nsCellMap*                   aCellMap,
                                        nsTArray<nsTableCellFrame*>* aCellFrames,
                                        PRInt32                      aRowIndex,
                                        PRInt32                      aColIndex,
                                        bool                         aInsert,
                                        nsRect&                      aDamageArea)
{
  PRInt32 numOrigCols = GetColCount();
  ClearCols();
  nsCellMap* cellMap = mFirstMap;
  PRInt32 rowCount = 0;
  while (cellMap) {
    if (cellMap == aCellMap) {
      cellMap->RebuildConsideringCells(*this, numOrigCols, aCellFrames,
                                       aRowIndex, aColIndex, aInsert);
    }
    else {
      cellMap->RebuildConsideringCells(*this, numOrigCols, nsnull, -1, 0,
                                       false);
    }
    rowCount += cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
  SetDamageArea(0, 0, GetColCount(), rowCount, aDamageArea);
}

void
nsTableCellMap::RebuildConsideringRows(nsCellMap*                  aCellMap,
                                       PRInt32                     aStartRowIndex,
                                       nsTArray<nsTableRowFrame*>* aRowsToInsert,
                                       PRInt32                     aNumRowsToRemove,
                                       nsRect&                     aDamageArea)
{
  NS_PRECONDITION(!aRowsToInsert || aNumRowsToRemove == 0,
                  "Can't handle both removing and inserting rows at once");

  PRInt32 numOrigCols = GetColCount();
  ClearCols();
  nsCellMap* cellMap = mFirstMap;
  PRInt32 rowCount = 0;
  while (cellMap) {
    if (cellMap == aCellMap) {
      cellMap->RebuildConsideringRows(*this, aStartRowIndex, aRowsToInsert,
                                      aNumRowsToRemove);
    }
    else {
      cellMap->RebuildConsideringCells(*this, numOrigCols, nsnull, -1, 0,
                                       false);
    }
    rowCount += cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
  SetDamageArea(0, 0, GetColCount(), rowCount, aDamageArea);
}

PRInt32
nsTableCellMap::GetNumCellsOriginatingInCol(PRInt32 aColIndex) const
{
  PRInt32 colCount = mCols.Length();
  if ((aColIndex >= 0) && (aColIndex < colCount)) {
    return mCols.ElementAt(aColIndex).mNumCellsOrig;
  }
  else {
    NS_ERROR("nsCellMap::GetNumCellsOriginatingInCol - bad col index");
    return 0;
  }
}

#ifdef NS_DEBUG
void
nsTableCellMap::Dump(char* aString) const
{
  if (aString)
    printf("%s \n", aString);
  printf("***** START TABLE CELL MAP DUMP ***** %p\n", (void*)this);
  
  PRInt32 colCount = mCols.Length();
  printf ("cols array orig/span-> %p", (void*)this);
  for (PRInt32 colX = 0; colX < colCount; colX++) {
    const nsColInfo& colInfo = mCols.ElementAt(colX);
    printf ("%d=%d/%d ", colX, colInfo.mNumCellsOrig, colInfo.mNumCellsSpan);
  }
  printf(" cols in cache %d\n", mTableFrame.GetColCache().Length());
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    cellMap->Dump(nsnull != mBCInfo);
    cellMap = cellMap->GetNextSibling();
  }
  if (nsnull != mBCInfo) {
    printf("***** bottom borders *****\n");
    nscoord       size;
    BCBorderOwner owner;
    mozilla::css::Side side;
    bool          segStart;
    bool          bevel;
    PRInt32       colIndex;
    PRInt32 numCols = mBCInfo->mBottomBorders.Length();
    for (PRInt32 i = 0; i <= 2; i++) {

      printf("\n          ");
      for (colIndex = 0; colIndex < numCols; colIndex++) {
        BCData& cd = mBCInfo->mBottomBorders.ElementAt(colIndex);
        if (0 == i) {
          size = cd.GetTopEdge(owner, segStart);
          printf("t=%d%X%d ", size, owner, segStart);
        }
        else if (1 == i) {
          size = cd.GetLeftEdge(owner, segStart);
          printf("l=%d%X%d ", size, owner, segStart);
        }
        else {
          size = cd.GetCorner(side, bevel);
          printf("c=%d%X%d ", size, side, bevel);
        }
      }
      BCData& cd = mBCInfo->mLowerRightCorner;
      if (0 == i) {
         size = cd.GetTopEdge(owner, segStart);
         printf("t=%d%X%d ", size, owner, segStart);
      }
      else if (1 == i) {
        size = cd.GetLeftEdge(owner, segStart);
        printf("l=%d%X%d ", size, owner, segStart);
      }
      else {
        size = cd.GetCorner(side, bevel);
        printf("c=%d%X%d ", size, side, bevel);
      }
    }
    printf("\n");
  }
  printf("***** END TABLE CELL MAP DUMP *****\n");
}
#endif

nsTableCellFrame*
nsTableCellMap::GetCellInfoAt(PRInt32  aRowIndex,
                              PRInt32  aColIndex,
                              bool*  aOriginates,
                              PRInt32* aColSpan) const
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->GetRowCount() > rowIndex) {
      return cellMap->GetCellInfoAt(*this, rowIndex, aColIndex, aOriginates, aColSpan);
    }
    rowIndex -= cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
  return nsnull;
}

PRInt32
nsTableCellMap::GetIndexByRowAndColumn(PRInt32 aRow, PRInt32 aColumn) const
{
  PRInt32 index = 0;

  PRInt32 colCount = mCols.Length();
  PRInt32 rowIndex = aRow;

  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    PRInt32 rowCount = cellMap->GetRowCount();
    if (rowIndex >= rowCount) {
      
      
      
      rowIndex -= rowCount;

      PRInt32 cellMapIdx = cellMap->GetHighestIndex(colCount);
      if (cellMapIdx != -1)
        index += cellMapIdx + 1;

    } else {
      
      
      PRInt32 cellMapIdx = cellMap->GetIndexByRowAndColumn(colCount, rowIndex,
                                                           aColumn);
      if (cellMapIdx == -1)
        return -1; 

      index += cellMapIdx;
      return index;  
    }

    cellMap = cellMap->GetNextSibling();
  }

  return -1;
}

void
nsTableCellMap::GetRowAndColumnByIndex(PRInt32 aIndex,
                                       PRInt32 *aRow, PRInt32 *aColumn) const
{
  *aRow = -1;
  *aColumn = -1;

  PRInt32 colCount = mCols.Length();

  PRInt32 previousRows = 0;
  PRInt32 index = aIndex;

  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    PRInt32 rowCount = cellMap->GetRowCount();
    
    
    PRInt32 cellMapIdx = cellMap->GetHighestIndex(colCount);
    if (cellMapIdx == -1) {
      
      
      previousRows += rowCount;
    } else {
      if (index > cellMapIdx) {
        
        
        index -= cellMapIdx + 1;
        previousRows += rowCount;
      } else {
        cellMap->GetRowAndColumnByIndex(colCount, index, aRow, aColumn);
        
        *aRow += previousRows;
        return; 
      }
    }

    cellMap = cellMap->GetNextSibling();
  }
}

bool nsTableCellMap::RowIsSpannedInto(PRInt32 aRowIndex,
                                        PRInt32 aNumEffCols) const
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->GetRowCount() > rowIndex) {
      return cellMap->RowIsSpannedInto(rowIndex, aNumEffCols);
    }
    rowIndex -= cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
  return false;
}

bool nsTableCellMap::RowHasSpanningCells(PRInt32 aRowIndex,
                                           PRInt32 aNumEffCols) const
{
  PRInt32 rowIndex = aRowIndex;
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    if (cellMap->GetRowCount() > rowIndex) {
      return cellMap->RowHasSpanningCells(rowIndex, aNumEffCols);
    }
    rowIndex -= cellMap->GetRowCount();
    cellMap = cellMap->GetNextSibling();
  }
  return false;
}

void nsTableCellMap::ExpandZeroColSpans()
{
  mTableFrame.SetNeedColSpanExpansion(false); 
  mTableFrame.SetHasZeroColSpans(false); 
                                            
  nsCellMap* cellMap = mFirstMap;
  while (cellMap) {
    cellMap->ExpandZeroColSpans(*this);
    cellMap = cellMap->GetNextSibling();
  }
}

void
nsTableCellMap::ResetTopStart(PRUint8    aSide,
                              nsCellMap& aCellMap,
                              PRUint32   aRowIndex,
                              PRUint32   aColIndex,
                              bool       aIsLowerRight)
{
  if (!mBCInfo || aIsLowerRight) ABORT0();

  BCCellData* cellData;
  BCData* bcData = nsnull;

  switch(aSide) {
  case NS_SIDE_BOTTOM:
    aRowIndex++;
    
  case NS_SIDE_TOP:
    cellData = (BCCellData*)aCellMap.GetDataAt(aRowIndex, aColIndex);
    if (cellData) {
      bcData = &cellData->mData;
    }
    else {
      NS_ASSERTION(aSide == NS_SIDE_BOTTOM, "program error");
      
      nsCellMap* cellMap = aCellMap.GetNextSibling();
      if (cellMap) {
        cellData = (BCCellData*)cellMap->GetDataAt(0, aColIndex);
        if (cellData) {
          bcData = &cellData->mData;
        }
        else {
          bcData = GetBottomMostBorder(aColIndex);
        }
      }
    }
    break;
  case NS_SIDE_RIGHT:
    aColIndex++;
    
  case NS_SIDE_LEFT:
    cellData = (BCCellData*)aCellMap.GetDataAt(aRowIndex, aColIndex);
    if (cellData) {
      bcData = &cellData->mData;
    }
    else {
      NS_ASSERTION(aSide == NS_SIDE_RIGHT, "program error");
      bcData = GetRightMostBorder(aRowIndex);
    }
    break;
  }
  if (bcData) {
    bcData->SetTopStart(false);
  }
}





void
nsTableCellMap::SetBCBorderEdge(mozilla::css::Side aSide,
                                nsCellMap&    aCellMap,
                                PRUint32      aCellMapStart,
                                PRUint32      aRowIndex,
                                PRUint32      aColIndex,
                                PRUint32      aLength,
                                BCBorderOwner aOwner,
                                nscoord       aSize,
                                bool          aChanged)
{
  if (!mBCInfo) ABORT0();

  BCCellData* cellData;
  PRInt32 lastIndex, xIndex, yIndex;
  PRInt32 xPos = aColIndex;
  PRInt32 yPos = aRowIndex;
  PRInt32 rgYPos = aRowIndex - aCellMapStart;
  bool changed;

  switch(aSide) {
  case NS_SIDE_BOTTOM:
    rgYPos++;
    yPos++;
  case NS_SIDE_TOP:
    lastIndex = xPos + aLength - 1;
    for (xIndex = xPos; xIndex <= lastIndex; xIndex++) {
      changed = aChanged && (xIndex == xPos);
      BCData* bcData = nsnull;
      cellData = (BCCellData*)aCellMap.GetDataAt(rgYPos, xIndex);
      if (!cellData) {
        PRInt32 numRgRows = aCellMap.GetRowCount();
        if (yPos < numRgRows) { 
          nsRect damageArea;
          cellData = (BCCellData*)aCellMap.AppendCell(*this, nsnull, rgYPos,
                                                       false, 0, damageArea);
          if (!cellData) ABORT0();
        }
        else {
          NS_ASSERTION(aSide == NS_SIDE_BOTTOM, "program error");
          
          nsCellMap* cellMap = aCellMap.GetNextSibling();
          while (cellMap && (0 == cellMap->GetRowCount())) {
            cellMap = cellMap->GetNextSibling();
          }
          if (cellMap) {
            cellData = (BCCellData*)cellMap->GetDataAt(0, xIndex);
            if (!cellData) { 
              nsRect damageArea;
              cellData = (BCCellData*)cellMap->AppendCell(*this, nsnull, 0,
                                                           false, 0,
                                                           damageArea);
            }
          }
          else { 
            bcData = GetBottomMostBorder(xIndex);
          }
        }
      }
      if (!bcData && cellData) {
        bcData = &cellData->mData;
      }
      if (bcData) {
        bcData->SetTopEdge(aOwner, aSize, changed);
      }
      else NS_ERROR("Cellmap: Top edge not found");
    }
    break;
  case NS_SIDE_RIGHT:
    xPos++;
  case NS_SIDE_LEFT:
    
    lastIndex = rgYPos + aLength - 1;
    for (yIndex = rgYPos; yIndex <= lastIndex; yIndex++) {
      changed = aChanged && (yIndex == rgYPos);
      cellData = (BCCellData*)aCellMap.GetDataAt(yIndex, xPos);
      if (cellData) {
        cellData->mData.SetLeftEdge(aOwner, aSize, changed);
      }
      else {
        NS_ASSERTION(aSide == NS_SIDE_RIGHT, "program error");
        BCData* bcData = GetRightMostBorder(yIndex + aCellMapStart);
        if (bcData) {
          bcData->SetLeftEdge(aOwner, aSize, changed);
        }
        else NS_ERROR("Cellmap: Left edge not found");
      }
    }
    break;
  }
}




void
nsTableCellMap::SetBCBorderCorner(Corner      aCorner,
                                  nsCellMap&  aCellMap,
                                  PRUint32    aCellMapStart,
                                  PRUint32    aRowIndex,
                                  PRUint32    aColIndex,
                                  mozilla::css::Side aOwner,
                                  nscoord     aSubSize,
                                  bool        aBevel,
                                  bool        aIsBottomRight)
{
  if (!mBCInfo) ABORT0();

  if (aIsBottomRight) {
    mBCInfo->mLowerRightCorner.SetCorner(aSubSize, aOwner, aBevel);
    return;
  }

  PRInt32 xPos = aColIndex;
  PRInt32 yPos = aRowIndex;
  PRInt32 rgYPos = aRowIndex - aCellMapStart;

  if (eTopRight == aCorner) {
    xPos++;
  }
  else if (eBottomRight == aCorner) {
    xPos++;
    rgYPos++;
    yPos++;
  }
  else if (eBottomLeft == aCorner) {
    rgYPos++;
    yPos++;
  }

  BCCellData* cellData = nsnull;
  BCData*     bcData   = nsnull;
  if (GetColCount() <= xPos) {
    NS_ASSERTION(xPos == GetColCount(), "program error");
    
    NS_ASSERTION(!aIsBottomRight, "should be handled before");
    bcData = GetRightMostBorder(yPos);
  }
  else {
    cellData = (BCCellData*)aCellMap.GetDataAt(rgYPos, xPos);
    if (!cellData) {
      PRInt32 numRgRows = aCellMap.GetRowCount();
      if (yPos < numRgRows) { 
        nsRect damageArea;
        cellData = (BCCellData*)aCellMap.AppendCell(*this, nsnull, rgYPos,
                                                     false, 0, damageArea);
      }
      else {
        
        nsCellMap* cellMap = aCellMap.GetNextSibling();
        while (cellMap && (0 == cellMap->GetRowCount())) {
          cellMap = cellMap->GetNextSibling();
        }
        if (cellMap) {
          cellData = (BCCellData*)cellMap->GetDataAt(0, xPos);
          if (!cellData) { 
            nsRect damageArea;
            cellData = (BCCellData*)cellMap->AppendCell(*this, nsnull, 0,
                                                         false, 0, damageArea);
          }
        }
        else { 
          bcData = GetBottomMostBorder(xPos);
        }
      }
    }
  }
  if (!bcData && cellData) {
    bcData = &cellData->mData;
  }
  if (bcData) {
    bcData->SetCorner(aSubSize, aOwner, aBevel);
  }
  else NS_ERROR("program error: Corner not found");
}

nsCellMap::nsCellMap(nsTableRowGroupFrame* aRowGroup, bool aIsBC)
  : mRows(8), mContentRowCount(0), mRowGroupFrame(aRowGroup),
    mNextSibling(nsnull), mIsBC(aIsBC),
    mPresContext(aRowGroup->PresContext())
{
  MOZ_COUNT_CTOR(nsCellMap);
  NS_ASSERTION(mPresContext, "Must have prescontext");
}

nsCellMap::~nsCellMap()
{
  MOZ_COUNT_DTOR(nsCellMap);

  PRUint32 mapRowCount = mRows.Length();
  for (PRUint32 rowX = 0; rowX < mapRowCount; rowX++) {
    CellDataArray &row = mRows[rowX];
    PRUint32 colCount = row.Length();
    for (PRUint32 colX = 0; colX < colCount; colX++) {
      DestroyCellData(row[colX]);
    }
  }
}


void
nsCellMap::Init()
{
  NS_ABORT_IF_FALSE(!sEmptyRow, "How did that happen?");
  sEmptyRow = new nsCellMap::CellDataArray();
}


void
nsCellMap::Shutdown()
{
  delete sEmptyRow;
  sEmptyRow = nsnull;
}

nsTableCellFrame*
nsCellMap::GetCellFrame(PRInt32   aRowIndexIn,
                        PRInt32   aColIndexIn,
                        CellData& aData,
                        bool      aUseRowIfOverlap) const
{
  PRInt32 rowIndex = aRowIndexIn - aData.GetRowSpanOffset();
  PRInt32 colIndex = aColIndexIn - aData.GetColSpanOffset();
  if (aData.IsOverlap()) {
    if (aUseRowIfOverlap) {
      colIndex = aColIndexIn;
    }
    else {
      rowIndex = aRowIndexIn;
    }
  }

  CellData* data =
    mRows.SafeElementAt(rowIndex, *sEmptyRow).SafeElementAt(colIndex);
  if (data) {
    return data->GetCellFrame();
  }
  return nsnull;
}

PRInt32
nsCellMap::GetHighestIndex(PRInt32 aColCount)
{
  PRInt32 index = -1;
  PRInt32 rowCount = mRows.Length();
  for (PRInt32 rowIdx = 0; rowIdx < rowCount; rowIdx++) {
    const CellDataArray& row = mRows[rowIdx];

    for (PRInt32 colIdx = 0; colIdx < aColCount; colIdx++) {
      CellData* data = row.SafeElementAt(colIdx);
      
      if (!data)
        break;

      if (data->IsOrig())
        index++;
    }
  }

  return index;
}

PRInt32
nsCellMap::GetIndexByRowAndColumn(PRInt32 aColCount,
                                  PRInt32 aRow, PRInt32 aColumn) const
{
  if (PRUint32(aRow) >= mRows.Length())
    return -1;

  PRInt32 index = -1;
  PRInt32 lastColsIdx = aColCount - 1;

  
  const CellDataArray& row = mRows[aRow];
  CellData* data = row.SafeElementAt(aColumn);
  PRInt32 origRow = data ? aRow - data->GetRowSpanOffset() : aRow;

  
  for (PRInt32 rowIdx = 0; rowIdx <= origRow; rowIdx++) {
    const CellDataArray& row = mRows[rowIdx];
    PRInt32 colCount = (rowIdx == origRow) ? aColumn : lastColsIdx;

    for (PRInt32 colIdx = 0; colIdx <= colCount; colIdx++) {
      data = row.SafeElementAt(colIdx);
      
      if (!data)
        break;

      if (data->IsOrig())
        index++;
    }
  }

  
  if (!data)
    return -1;

  return index;
}

void
nsCellMap::GetRowAndColumnByIndex(PRInt32 aColCount, PRInt32 aIndex,
                                  PRInt32 *aRow, PRInt32 *aColumn) const
{
  *aRow = -1;
  *aColumn = -1;

  PRInt32 index = aIndex;
  PRInt32 rowCount = mRows.Length();

  for (PRInt32 rowIdx = 0; rowIdx < rowCount; rowIdx++) {
    const CellDataArray& row = mRows[rowIdx];

    for (PRInt32 colIdx = 0; colIdx < aColCount; colIdx++) {
      CellData* data = row.SafeElementAt(colIdx);

      
      if (!data)
        break;

      if (data->IsOrig())
        index--;

      if (index < 0) {
        *aRow = rowIdx;
        *aColumn = colIdx;
        return;
      }
    }
  }
}

bool nsCellMap::Grow(nsTableCellMap& aMap,
                       PRInt32         aNumRows,
                       PRInt32         aRowIndex)
{
  NS_ASSERTION(aNumRows >= 1, "Why are we calling this?");

  
  PRInt32 numCols = aMap.GetColCount();
  if (numCols == 0) {
    numCols = 4;
  }
  PRUint32 startRowIndex = (aRowIndex >= 0) ? aRowIndex : mRows.Length();
  NS_ASSERTION(startRowIndex <= mRows.Length(), "Missing grow call inbetween");

  return mRows.InsertElementsAt(startRowIndex, aNumRows, numCols) != nsnull;
}

void nsCellMap::GrowRow(CellDataArray& aRow,
                        PRInt32        aNumCols)

{
  
  aRow.InsertElementsAt(aRow.Length(), aNumCols, (CellData*)nsnull);
}

void
nsCellMap::InsertRows(nsTableCellMap&             aMap,
                      nsTArray<nsTableRowFrame*>& aRows,
                      PRInt32                     aFirstRowIndex,
                      bool                        aConsiderSpans,
                      PRInt32                     aRgFirstRowIndex,
                      nsRect&                     aDamageArea)
{
  PRInt32 numCols = aMap.GetColCount();
  NS_ASSERTION(aFirstRowIndex >= 0, "nsCellMap::InsertRows called with negative rowIndex");
  if (PRUint32(aFirstRowIndex) > mRows.Length()) {
    
    PRInt32 numEmptyRows = aFirstRowIndex - mRows.Length();
    if (!Grow(aMap, numEmptyRows)) {
      return;
    }
  }

  if (!aConsiderSpans) {
    
    mContentRowCount = NS_MAX(aFirstRowIndex, mContentRowCount);
    ExpandWithRows(aMap, aRows, aFirstRowIndex, aRgFirstRowIndex, aDamageArea);
    return;
  }

  
  bool spansCauseRebuild = CellsSpanInOrOut(aFirstRowIndex,
                                              aFirstRowIndex, 0, numCols - 1);

  
  mContentRowCount = NS_MAX(aFirstRowIndex, mContentRowCount);

  
  
  if (!spansCauseRebuild && (PRUint32(aFirstRowIndex) < mRows.Length())) {
    spansCauseRebuild = CellsSpanOut(aRows);
  }
  if (spansCauseRebuild) {
    aMap.RebuildConsideringRows(this, aFirstRowIndex, &aRows, 0, aDamageArea);
  }
  else {
    ExpandWithRows(aMap, aRows, aFirstRowIndex, aRgFirstRowIndex, aDamageArea);
  }
}

void
nsCellMap::RemoveRows(nsTableCellMap& aMap,
                      PRInt32         aFirstRowIndex,
                      PRInt32         aNumRowsToRemove,
                      bool            aConsiderSpans,
                      PRInt32         aRgFirstRowIndex,
                      nsRect&         aDamageArea)
{
  PRInt32 numRows = mRows.Length();
  PRInt32 numCols = aMap.GetColCount();

  if (aFirstRowIndex >= numRows) {
    
    
    
    mContentRowCount -= aNumRowsToRemove;
    return;
  }
  if (!aConsiderSpans) {
    ShrinkWithoutRows(aMap, aFirstRowIndex, aNumRowsToRemove, aRgFirstRowIndex,
                      aDamageArea);
    return;
  }
  PRInt32 endRowIndex = aFirstRowIndex + aNumRowsToRemove - 1;
  if (endRowIndex >= numRows) {
    NS_ERROR("nsCellMap::RemoveRows tried to remove too many rows");
    endRowIndex = numRows - 1;
  }
  bool spansCauseRebuild = CellsSpanInOrOut(aFirstRowIndex, endRowIndex,
                                              0, numCols - 1);
  if (spansCauseRebuild) {
    aMap.RebuildConsideringRows(this, aFirstRowIndex, nsnull, aNumRowsToRemove,
                                aDamageArea);
  }
  else {
    ShrinkWithoutRows(aMap, aFirstRowIndex, aNumRowsToRemove, aRgFirstRowIndex,
                      aDamageArea);
  }
}




CellData*
nsCellMap::AppendCell(nsTableCellMap&   aMap,
                      nsTableCellFrame* aCellFrame,
                      PRInt32           aRowIndex,
                      bool              aRebuildIfNecessary,
                      PRInt32           aRgFirstRowIndex,
                      nsRect&           aDamageArea,
                      PRInt32*          aColToBeginSearch)
{
  NS_ASSERTION(!!aMap.mBCInfo == mIsBC, "BC state mismatch");
  PRInt32 origNumMapRows = mRows.Length();
  PRInt32 origNumCols = aMap.GetColCount();
  bool    zeroRowSpan = false;
  PRInt32 rowSpan = (aCellFrame) ? GetRowSpanForNewCell(aCellFrame, aRowIndex,
                                                        zeroRowSpan) : 1;
  
  PRInt32 endRowIndex = aRowIndex + rowSpan - 1;
  if (endRowIndex >= origNumMapRows) {
    
    Grow(aMap, 1 + endRowIndex - origNumMapRows);
  }

  
  CellData* origData = nsnull;
  PRInt32 startColIndex = 0;
  if (aColToBeginSearch)
    startColIndex = *aColToBeginSearch;
  for (; startColIndex < origNumCols; startColIndex++) {
    CellData* data = GetDataAt(aRowIndex, startColIndex);
    if (!data)
      break;
    
    
    if (data->IsDead() && aCellFrame) {
      origData = data;
      break;
    }
    if (data->IsZeroColSpan() ) {
      
      CollapseZeroColSpan(aMap, data, aRowIndex, startColIndex);
      
      origData = GetDataAt(aRowIndex, startColIndex);
      NS_ASSERTION(origData->IsDead(),
                   "The cellposition should have been cleared");
      break;
    }
  }
  
  
  
  if (aColToBeginSearch)
    *aColToBeginSearch =  startColIndex + 1;

  bool    zeroColSpan = false;
  PRInt32 colSpan = (aCellFrame) ?
                    GetColSpanForNewCell(*aCellFrame, zeroColSpan) : 1;
  if (zeroColSpan) {
    aMap.mTableFrame.SetHasZeroColSpans(true);
    aMap.mTableFrame.SetNeedColSpanExpansion(true);
  }

  
  
  if (aRebuildIfNecessary && (aRowIndex < mContentRowCount - 1) && (rowSpan > 1)) {
    nsAutoTArray<nsTableCellFrame*, 1> newCellArray;
    newCellArray.AppendElement(aCellFrame);
    aMap.RebuildConsideringCells(this, &newCellArray, aRowIndex, startColIndex, true, aDamageArea);
    return origData;
  }
  mContentRowCount = NS_MAX(mContentRowCount, aRowIndex + 1);

  
  PRInt32 endColIndex = startColIndex + colSpan - 1;
  if (endColIndex >= origNumCols) {
    NS_ASSERTION(aCellFrame, "dead cells should not require new columns");
    aMap.AddColsAtEnd(1 + endColIndex - origNumCols);
  }

  
  if (origData) {
    NS_ASSERTION(origData->IsDead(), "replacing a non dead cell is a memory leak");
    if (aCellFrame) { 
      origData->Init(aCellFrame);
      
      
      nsColInfo* colInfo = aMap.GetColInfoAt(startColIndex);
      NS_ASSERTION(colInfo, "access to a non existing column");
      if (colInfo) {
        colInfo->mNumCellsOrig++;
      }
    }
  }
  else {
    origData = AllocCellData(aCellFrame);
    if (!origData) ABORT1(origData);
    SetDataAt(aMap, *origData, aRowIndex, startColIndex);
  }

  if (aRebuildIfNecessary) {
    
    
    
    PRUint32 height = zeroRowSpan ? endRowIndex - aRowIndex  :
                                    1 + endRowIndex - aRowIndex;
    SetDamageArea(startColIndex, aRgFirstRowIndex + aRowIndex,
                  1 + endColIndex - startColIndex, height, aDamageArea);
  }

  if (!aCellFrame) {
    return origData;
  }

  
  aCellFrame->SetColIndex(startColIndex);

  
  
  for (PRInt32 rowX = aRowIndex; rowX <= endRowIndex; rowX++) {
    
    mRows[rowX].SetCapacity(endColIndex);
    for (PRInt32 colX = startColIndex; colX <= endColIndex; colX++) {
      if ((rowX != aRowIndex) || (colX != startColIndex)) { 
        CellData* cellData = GetDataAt(rowX, colX);
        if (cellData) {
          if (cellData->IsOrig()) {
            NS_ERROR("cannot overlap originating cell");
            continue;
          }
          if (rowX > aRowIndex) { 
            if (cellData->IsRowSpan()) {
              
              
            }
            else {
              cellData->SetRowSpanOffset(rowX - aRowIndex);
              if (zeroRowSpan) {
                cellData->SetZeroRowSpan(true);
              }
            }
          }
          if (colX > startColIndex) { 
            if (!cellData->IsColSpan()) {
              if (cellData->IsRowSpan()) {
                cellData->SetOverlap(true);
              }
              cellData->SetColSpanOffset(colX - startColIndex);
              if (zeroColSpan) {
                cellData->SetZeroColSpan(true);
              }

              nsColInfo* colInfo = aMap.GetColInfoAt(colX);
              colInfo->mNumCellsSpan++;
            }
          }
        }
        else {
          cellData = AllocCellData(nsnull);
          if (!cellData) return origData;
          if (rowX > aRowIndex) {
            cellData->SetRowSpanOffset(rowX - aRowIndex);
            if (zeroRowSpan) {
              cellData->SetZeroRowSpan(true);
            }
          }
          if (colX > startColIndex) {
            cellData->SetColSpanOffset(colX - startColIndex);
            if (zeroColSpan) {
              cellData->SetZeroColSpan(true);
            }
          }
          SetDataAt(aMap, *cellData, rowX, colX);
        }
      }
    }
  }
#ifdef DEBUG_TABLE_CELLMAP
  printf("appended cell=%p row=%d \n", aCellFrame, aRowIndex);
  aMap.Dump();
#endif
  return origData;
}

void nsCellMap::CollapseZeroColSpan(nsTableCellMap& aMap,
                                    CellData*       aOrigData,
                                    PRInt32         aRowIndex,
                                    PRInt32         aColIndex)
{
  
  
  
  
  

  NS_ASSERTION(aOrigData && aOrigData->IsZeroColSpan(),
               "zero colspan should have been passed");
  
  nsTableCellFrame* cell = GetCellFrame(aRowIndex, aColIndex, *aOrigData, true);
  NS_ASSERTION(cell, "originating cell not found");

  
  PRInt32 startRowIndex = aRowIndex - aOrigData->GetRowSpanOffset();
  bool    zeroSpan;
  PRInt32 rowSpan = GetRowSpanForNewCell(cell, startRowIndex, zeroSpan);
  PRInt32 endRowIndex = startRowIndex + rowSpan;

  PRInt32 origColIndex = aColIndex - aOrigData->GetColSpanOffset();
  PRInt32 endColIndex = origColIndex +
                        GetEffectiveColSpan(aMap, startRowIndex,
                                            origColIndex, zeroSpan);
  for (PRInt32 colX = origColIndex +1; colX < endColIndex; colX++) {
    
    
    
    nsColInfo* colInfo = aMap.GetColInfoAt(colX);
    colInfo->mNumCellsSpan -= rowSpan;

    for (PRInt32 rowX = startRowIndex; rowX < endRowIndex; rowX++)
    {
      CellData* data = mRows[rowX][colX];
      NS_ASSERTION(data->IsZeroColSpan(),
                   "Overwriting previous data - memory leak");
      data->Init(nsnull); 
    }
  }
}

bool nsCellMap::CellsSpanOut(nsTArray<nsTableRowFrame*>& aRows) const
{
  PRInt32 numNewRows = aRows.Length();
  for (PRInt32 rowX = 0; rowX < numNewRows; rowX++) {
    nsIFrame* rowFrame = (nsIFrame *) aRows.ElementAt(rowX);
    nsIFrame* childFrame = rowFrame->GetFirstPrincipalChild();
    while (childFrame) {
      nsTableCellFrame *cellFrame = do_QueryFrame(childFrame);
      if (cellFrame) {
        bool zeroSpan;
        PRInt32 rowSpan = GetRowSpanForNewCell(cellFrame, rowX, zeroSpan);
        if (zeroSpan || rowX + rowSpan > numNewRows) {
          return true;
        }
      }
      childFrame = childFrame->GetNextSibling();
    }
  }
  return false;
}



bool nsCellMap::CellsSpanInOrOut(PRInt32 aStartRowIndex,
                                   PRInt32 aEndRowIndex,
                                   PRInt32 aStartColIndex,
                                   PRInt32 aEndColIndex) const
{
  












  PRInt32 numRows = mRows.Length(); 
                                    
  for (PRInt32 colX = aStartColIndex; colX <= aEndColIndex; colX++) {
    CellData* cellData;
    if (aStartRowIndex > 0) {
      cellData = GetDataAt(aStartRowIndex, colX);
      if (cellData && (cellData->IsRowSpan())) {
        return true; 
      }
      if ((aStartRowIndex >= mContentRowCount) &&  (mContentRowCount > 0)) {
        cellData = GetDataAt(mContentRowCount - 1, colX);
        if (cellData && cellData->IsZeroRowSpan()) {
          return true;  
        }
      }
    }
    if (aEndRowIndex < numRows - 1) { 
      cellData = GetDataAt(aEndRowIndex + 1, colX);
      if ((cellData) && (cellData->IsRowSpan())) {
        return true; 
      }
    }
    else {
      cellData = GetDataAt(aEndRowIndex, colX);
      if ((cellData) && (cellData->IsRowSpan()) && (mContentRowCount < numRows)) {
        return true; 
      }
    }
  }
  if (aStartColIndex > 0) {
    for (PRInt32 rowX = aStartRowIndex; rowX <= aEndRowIndex; rowX++) {
      CellData* cellData = GetDataAt(rowX, aStartColIndex);
      if (cellData && (cellData->IsColSpan())) {
        return true; 
      }
      cellData = GetDataAt(rowX, aEndColIndex + 1);
      if (cellData && (cellData->IsColSpan())) {
        return true; 
      }
    }
  }
  return false;
}

void nsCellMap::InsertCells(nsTableCellMap&              aMap,
                            nsTArray<nsTableCellFrame*>& aCellFrames,
                            PRInt32                      aRowIndex,
                            PRInt32                      aColIndexBefore,
                            PRInt32                      aRgFirstRowIndex,
                            nsRect&                      aDamageArea)
{
  if (aCellFrames.Length() == 0) return;
  NS_ASSERTION(aColIndexBefore >= -1, "index out of range");
  PRInt32 numCols = aMap.GetColCount();
  if (aColIndexBefore >= numCols) {
    NS_ERROR("Inserting instead of appending cells indicates a serious cellmap error");
    aColIndexBefore = numCols - 1;
  }

  
  PRInt32 startColIndex;
  for (startColIndex = aColIndexBefore + 1; startColIndex < numCols; startColIndex++) {
    CellData* data = GetDataAt(aRowIndex, startColIndex);
    if (!data || data->IsOrig() || data->IsDead()) {
      
      break;
    }
    if (data->IsZeroColSpan()) {
      
      CollapseZeroColSpan(aMap, data, aRowIndex, startColIndex);
      break;
    }
  }

  
  
  bool spansCauseRebuild = false;

  
  PRInt32 numNewCells = aCellFrames.Length();
  bool zeroRowSpan = false;
  PRInt32 rowSpan = 0;
  for (PRInt32 cellX = 0; cellX < numNewCells; cellX++) {
    nsTableCellFrame* cell = aCellFrames.ElementAt(cellX);
    PRInt32 rowSpan2 = GetRowSpanForNewCell(cell, aRowIndex, zeroRowSpan);
    if (rowSpan == 0) {
      rowSpan = rowSpan2;
    }
    else if (rowSpan != rowSpan2) {
      spansCauseRebuild = true;
      break;
    }
  }

  
  if (!spansCauseRebuild) {
    if (mRows.Length() < PRUint32(aRowIndex + rowSpan)) {
      spansCauseRebuild = true;
    }
  }

  if (!spansCauseRebuild) {
    spansCauseRebuild = CellsSpanInOrOut(aRowIndex, aRowIndex + rowSpan - 1,
                                         startColIndex, numCols - 1);
  }
  if (spansCauseRebuild) {
    aMap.RebuildConsideringCells(this, &aCellFrames, aRowIndex, startColIndex,
                                 true, aDamageArea);
  }
  else {
    ExpandWithCells(aMap, aCellFrames, aRowIndex, startColIndex, rowSpan,
                    zeroRowSpan, aRgFirstRowIndex, aDamageArea);
  }
}

void
nsCellMap::ExpandWithRows(nsTableCellMap&             aMap,
                          nsTArray<nsTableRowFrame*>& aRowFrames,
                          PRInt32                     aStartRowIndexIn,
                          PRInt32                     aRgFirstRowIndex,
                          nsRect&                     aDamageArea)
{
  PRInt32 startRowIndex = (aStartRowIndexIn >= 0) ? aStartRowIndexIn : 0;
  NS_ASSERTION(PRUint32(startRowIndex) <= mRows.Length(), "caller should have grown cellmap before");

  PRInt32 numNewRows  = aRowFrames.Length();
  mContentRowCount += numNewRows;

  PRInt32 endRowIndex = startRowIndex + numNewRows - 1;

  
  
  if (!Grow(aMap, numNewRows, startRowIndex)) {
    return;
  }


  PRInt32 newRowIndex = 0;
  for (PRInt32 rowX = startRowIndex; rowX <= endRowIndex; rowX++) {
    nsTableRowFrame* rFrame = aRowFrames.ElementAt(newRowIndex);
    
    nsIFrame* cFrame = rFrame->GetFirstPrincipalChild();
    PRInt32 colIndex = 0;
    while (cFrame) {
      nsTableCellFrame *cellFrame = do_QueryFrame(cFrame);
      if (cellFrame) {
        AppendCell(aMap, cellFrame, rowX, false, aRgFirstRowIndex, aDamageArea,
                   &colIndex);
      }
      cFrame = cFrame->GetNextSibling();
    }
    newRowIndex++;
  }
  SetDamageArea(0, aRgFirstRowIndex + startRowIndex, aMap.GetColCount(),
                1 + endRowIndex - startRowIndex, aDamageArea);
}

void nsCellMap::ExpandWithCells(nsTableCellMap&              aMap,
                                nsTArray<nsTableCellFrame*>& aCellFrames,
                                PRInt32                      aRowIndex,
                                PRInt32                      aColIndex,
                                PRInt32                      aRowSpan, 
                                bool                         aRowSpanIsZero,
                                PRInt32                      aRgFirstRowIndex,
                                nsRect&                      aDamageArea)
{
  NS_ASSERTION(!!aMap.mBCInfo == mIsBC, "BC state mismatch");
  PRInt32 endRowIndex = aRowIndex + aRowSpan - 1;
  PRInt32 startColIndex = aColIndex;
  PRInt32 endColIndex = aColIndex;
  PRInt32 numCells = aCellFrames.Length();
  PRInt32 totalColSpan = 0;

  
  for (PRInt32 cellX = 0; cellX < numCells; cellX++) {
    nsTableCellFrame* cellFrame = aCellFrames.ElementAt(cellX);
    CellData* origData = AllocCellData(cellFrame); 
    if (!origData) return;

    
    bool zeroColSpan = false;
    PRInt32 colSpan = GetColSpanForNewCell(*cellFrame, zeroColSpan);
    if (zeroColSpan) {
      aMap.mTableFrame.SetHasZeroColSpans(true);
      aMap.mTableFrame.SetNeedColSpanExpansion(true);
    }
    totalColSpan += colSpan;
    if (cellX == 0) {
      endColIndex = aColIndex + colSpan - 1;
    }
    else {
      startColIndex = endColIndex + 1;
      endColIndex   = startColIndex + colSpan - 1;
    }

    
    for (PRInt32 rowX = aRowIndex; rowX <= endRowIndex; rowX++) {
      CellDataArray& row = mRows[rowX];
      
      
      
      PRInt32 insertionIndex = row.Length();
      if (insertionIndex > startColIndex) {
        insertionIndex = startColIndex;
      }
      if (!row.InsertElementsAt(insertionIndex, endColIndex - insertionIndex + 1,
                                (CellData*)nsnull) &&
          rowX == aRowIndex) {
        
        
        
        DestroyCellData(origData);
        return;
      }

      for (PRInt32 colX = startColIndex; colX <= endColIndex; colX++) {
        CellData* data = origData;
        if ((rowX != aRowIndex) || (colX != startColIndex)) {
          data = AllocCellData(nsnull);
          if (!data) return;
          if (rowX > aRowIndex) {
            data->SetRowSpanOffset(rowX - aRowIndex);
            if (aRowSpanIsZero) {
              data->SetZeroRowSpan(true);
            }
          }
          if (colX > startColIndex) {
            data->SetColSpanOffset(colX - startColIndex);
            if (zeroColSpan) {
              data->SetZeroColSpan(true);
            }
          }
        }
        SetDataAt(aMap, *data, rowX, colX);
      }
    }
    cellFrame->SetColIndex(startColIndex);
  }
  PRInt32 damageHeight = NS_MIN(GetRowGroup()->GetRowCount() - aRowIndex,
                                aRowSpan);
  SetDamageArea(aColIndex, aRgFirstRowIndex + aRowIndex,
                1 + endColIndex - aColIndex, damageHeight, aDamageArea);

  PRInt32 rowX;

  
  for (rowX = aRowIndex; rowX <= endRowIndex; rowX++) {
    CellDataArray& row = mRows[rowX];
    PRUint32 numCols = row.Length();
    PRUint32 colX;
    for (colX = aColIndex + totalColSpan; colX < numCols; colX++) {
      CellData* data = row[colX];
      if (data) {
        
        if (data->IsOrig()) {
          
          data->GetCellFrame()->SetColIndex(colX);
          nsColInfo* colInfo = aMap.GetColInfoAt(colX);
          colInfo->mNumCellsOrig++;
        }
        if (data->IsColSpan()) {
          nsColInfo* colInfo = aMap.GetColInfoAt(colX);
          colInfo->mNumCellsSpan++;
        }

        
        PRInt32 colX2 = colX - totalColSpan;
        nsColInfo* colInfo2 = aMap.GetColInfoAt(colX2);
        if (data->IsOrig()) {
          
          colInfo2->mNumCellsOrig--;
        }
        if (data->IsColSpan()) {
          colInfo2->mNumCellsSpan--;
        }
      }
    }
  }
}

void nsCellMap::ShrinkWithoutRows(nsTableCellMap& aMap,
                                  PRInt32         aStartRowIndex,
                                  PRInt32         aNumRowsToRemove,
                                  PRInt32         aRgFirstRowIndex,
                                  nsRect&         aDamageArea)
{
  NS_ASSERTION(!!aMap.mBCInfo == mIsBC, "BC state mismatch");
  PRInt32 endRowIndex = aStartRowIndex + aNumRowsToRemove - 1;
  PRUint32 colCount = aMap.GetColCount();
  for (PRInt32 rowX = endRowIndex; rowX >= aStartRowIndex; --rowX) {
    CellDataArray& row = mRows[rowX];
    PRUint32 colX;
    for (colX = 0; colX < colCount; colX++) {
      CellData* data = row.SafeElementAt(colX);
      if (data) {
        
        if (data->IsOrig()) {
          
          nsColInfo* colInfo = aMap.GetColInfoAt(colX);
          colInfo->mNumCellsOrig--;
        }
        
        else if (data->IsColSpan()) {
          nsColInfo* colInfo = aMap.GetColInfoAt(colX);
          colInfo->mNumCellsSpan--;
        }
      }
    }

    PRUint32 rowLength = row.Length();
    
    for (colX = 0; colX < rowLength; colX++) {
      DestroyCellData(row[colX]);
    }

    mRows.RemoveElementAt(rowX);

    
    mContentRowCount--;
  }
  aMap.RemoveColsAtEnd();
  SetDamageArea(0, aRgFirstRowIndex + aStartRowIndex, aMap.GetColCount(), 0,
                aDamageArea);
}

PRInt32 nsCellMap::GetColSpanForNewCell(nsTableCellFrame& aCellFrameToAdd,
                                        bool&           aIsZeroColSpan) const
{
  aIsZeroColSpan = false;
  PRInt32 colSpan = aCellFrameToAdd.GetColSpan();
  if (0 == colSpan) {
    colSpan = 1; 
    aIsZeroColSpan = true;
  }
  return colSpan;
}

PRInt32 nsCellMap::GetEffectiveColSpan(const nsTableCellMap& aMap,
                                       PRInt32         aRowIndex,
                                       PRInt32         aColIndex,
                                       bool&         aZeroColSpan) const
{
  PRInt32 numColsInTable = aMap.GetColCount();
  aZeroColSpan = false;
  PRInt32 colSpan = 1;
  if (PRUint32(aRowIndex) >= mRows.Length()) {
    return colSpan;
  }

  const CellDataArray& row = mRows[aRowIndex];
  PRInt32 colX;
  CellData* data;
  PRInt32 maxCols = numColsInTable;
  bool hitOverlap = false; 
  for (colX = aColIndex + 1; colX < maxCols; colX++) {
    data = row.SafeElementAt(colX);
    if (data) {
      
      
      
      if (!hitOverlap && data->IsOverlap()) {
        CellData* origData = row.SafeElementAt(aColIndex);
        if (origData && origData->IsOrig()) {
          nsTableCellFrame* cellFrame = origData->GetCellFrame();
          if (cellFrame) {
            
            maxCols = NS_MIN(aColIndex + cellFrame->GetColSpan(), maxCols);
            if (colX >= maxCols)
              break;
          }
        }
      }
      if (data->IsColSpan()) {
        colSpan++;
        if (data->IsZeroColSpan()) {
          aZeroColSpan = true;
        }
      }
      else {
        break;
      }
    }
    else break;
  }
  return colSpan;
}

PRInt32
nsCellMap::GetRowSpanForNewCell(nsTableCellFrame* aCellFrameToAdd,
                                PRInt32           aRowIndex,
                                bool&           aIsZeroRowSpan) const
{
  aIsZeroRowSpan = false;
  PRInt32 rowSpan = aCellFrameToAdd->GetRowSpan();
  if (0 == rowSpan) {
    
    
    rowSpan = NS_MAX(2, mContentRowCount - aRowIndex);
    aIsZeroRowSpan = true;
  }
  return rowSpan;
}

bool nsCellMap::HasMoreThanOneCell(PRInt32 aRowIndex) const
{
  const CellDataArray& row = mRows.SafeElementAt(aRowIndex, *sEmptyRow);
  PRUint32 maxColIndex = row.Length();
  PRUint32 count = 0;
  PRUint32 colIndex;
  for (colIndex = 0; colIndex < maxColIndex; colIndex++) {
    CellData* cellData = row[colIndex];
    if (cellData && (cellData->GetCellFrame() || cellData->IsRowSpan()))
      count++;
    if (count > 1)
      return true;
  }
  return false;
}

PRInt32
nsCellMap::GetNumCellsOriginatingInRow(PRInt32 aRowIndex) const
{
  const CellDataArray& row = mRows.SafeElementAt(aRowIndex, *sEmptyRow);
  PRUint32 count = 0;
  PRUint32 maxColIndex = row.Length();
  PRUint32 colIndex;
  for (colIndex = 0; colIndex < maxColIndex; colIndex++) {
    CellData* cellData = row[colIndex];
    if (cellData && cellData->IsOrig())
      count++;
  }
  return count;
}

PRInt32 nsCellMap::GetRowSpan(PRInt32  aRowIndex,
                              PRInt32  aColIndex,
                              bool     aGetEffective) const
{
  PRInt32 rowSpan = 1;
  PRInt32 rowCount = (aGetEffective) ? mContentRowCount : mRows.Length();
  PRInt32 rowX;
  for (rowX = aRowIndex + 1; rowX < rowCount; rowX++) {
    CellData* data = GetDataAt(rowX, aColIndex);
    if (data) {
      if (data->IsRowSpan()) {
        rowSpan++;
      }
      else {
        break;
      }
    }
    else break;
  }
  return rowSpan;
}

void nsCellMap::ShrinkWithoutCell(nsTableCellMap&   aMap,
                                  nsTableCellFrame& aCellFrame,
                                  PRInt32           aRowIndex,
                                  PRInt32           aColIndex,
                                  PRInt32           aRgFirstRowIndex,
                                  nsRect&           aDamageArea)
{
  NS_ASSERTION(!!aMap.mBCInfo == mIsBC, "BC state mismatch");
  PRUint32 colX, rowX;

  
  bool zeroColSpan;
  PRUint32 numCols = aMap.GetColCount();
  PRInt32 rowSpan = GetRowSpan(aRowIndex, aColIndex, false);
  PRUint32 colSpan = GetEffectiveColSpan(aMap, aRowIndex, aColIndex, zeroColSpan);
  PRUint32 endRowIndex = aRowIndex + rowSpan - 1;
  PRUint32 endColIndex = aColIndex + colSpan - 1;

  if (aMap.mTableFrame.HasZeroColSpans()) {
    aMap.mTableFrame.SetNeedColSpanExpansion(true);
  }

  
  for (colX = aColIndex; colX <= endColIndex; colX++) {
    nsColInfo* colInfo = aMap.GetColInfoAt(colX);
    if (colX == PRUint32(aColIndex)) {
      colInfo->mNumCellsOrig--;
    }
    else  {
      colInfo->mNumCellsSpan--;
    }
  }

  
  for (rowX = aRowIndex; rowX <= endRowIndex; rowX++) {
    CellDataArray& row = mRows[rowX];

    
    
    NS_ASSERTION(endColIndex + 1 <= row.Length(), "span beyond the row size!");
    PRUint32 endIndexForRow = NS_MIN(endColIndex + 1, row.Length());

    
    if (PRUint32(aColIndex) < endIndexForRow) {
      for (colX = endIndexForRow; colX > PRUint32(aColIndex); colX--) {
        DestroyCellData(row[colX-1]);
      }
      row.RemoveElementsAt(aColIndex, endIndexForRow - aColIndex);
    }
  }

  numCols = aMap.GetColCount();

  
  for (rowX = aRowIndex; rowX <= endRowIndex; rowX++) {
    CellDataArray& row = mRows[rowX];
    for (colX = aColIndex; colX < numCols - colSpan; colX++) {
      CellData* data = row.SafeElementAt(colX);
      if (data) {
        if (data->IsOrig()) {
          
          data->GetCellFrame()->SetColIndex(colX);
          nsColInfo* colInfo = aMap.GetColInfoAt(colX);
          colInfo->mNumCellsOrig++;
          
          colInfo = aMap.GetColInfoAt(colX + colSpan);
          if (colInfo) {
            colInfo->mNumCellsOrig--;
          }
        }

        else if (data->IsColSpan()) {
          
          
          nsColInfo* colInfo = aMap.GetColInfoAt(colX);
          colInfo->mNumCellsSpan++;
          
          
          colInfo = aMap.GetColInfoAt(colX + colSpan);
          if (colInfo) {
            colInfo->mNumCellsSpan--;
          }
        }
      }
    }
  }
  aMap.RemoveColsAtEnd();
  SetDamageArea(aColIndex, aRgFirstRowIndex + aRowIndex,
                NS_MAX(0, aMap.GetColCount() - aColIndex - 1),
                1 + endRowIndex - aRowIndex, aDamageArea);
}

void
nsCellMap::RebuildConsideringRows(nsTableCellMap&             aMap,
                                  PRInt32                     aStartRowIndex,
                                  nsTArray<nsTableRowFrame*>* aRowsToInsert,
                                  PRInt32                     aNumRowsToRemove)
{
  NS_ASSERTION(!!aMap.mBCInfo == mIsBC, "BC state mismatch");
  
  PRUint32 numOrigRows = mRows.Length();
  nsTArray<CellDataArray> origRows;
  mRows.SwapElements(origRows);

  PRInt32 rowNumberChange;
  if (aRowsToInsert) {
    rowNumberChange = aRowsToInsert->Length();
  } else {
    rowNumberChange = -aNumRowsToRemove;
  }

  
  
  mContentRowCount += rowNumberChange;
  NS_ASSERTION(mContentRowCount >= 0, "previous mContentRowCount was wrong");
  
  if (mContentRowCount) {
    if (!Grow(aMap, mContentRowCount)) {
      
      return;
    }
  }

  
  
  PRUint32 copyEndRowIndex = NS_MIN(numOrigRows, PRUint32(aStartRowIndex));

  
  
  PRUint32 rowX = 0;
  nsRect damageArea;
  
  
  
  for ( ; rowX < copyEndRowIndex; rowX++) {
    const CellDataArray& row = origRows[rowX];
    PRUint32 numCols = row.Length();
    for (PRUint32 colX = 0; colX < numCols; colX++) {
      
      const CellData* data = row.ElementAt(colX);
      if (data && data->IsOrig()) {
        AppendCell(aMap, data->GetCellFrame(), rowX, false, 0, damageArea);
      }
    }
  }

  
  PRUint32 copyStartRowIndex;
  rowX = aStartRowIndex;
  if (aRowsToInsert) {
    
    PRInt32 numNewRows = aRowsToInsert->Length();
    for (PRInt32 newRowX = 0; newRowX < numNewRows; newRowX++) {
      nsTableRowFrame* rFrame = aRowsToInsert->ElementAt(newRowX);
      nsIFrame* cFrame = rFrame->GetFirstPrincipalChild();
      while (cFrame) {
        nsTableCellFrame *cellFrame = do_QueryFrame(cFrame);
        if (cellFrame) {
          AppendCell(aMap, cellFrame, rowX, false, 0, damageArea);
        }
        cFrame = cFrame->GetNextSibling();
      }
      rowX++;
    }
    copyStartRowIndex = aStartRowIndex;
  }
  else {
    copyStartRowIndex = aStartRowIndex + aNumRowsToRemove;
  }

  
  
  
  for (PRUint32 copyRowX = copyStartRowIndex; copyRowX < numOrigRows;
       copyRowX++) {
    const CellDataArray& row = origRows[copyRowX];
    PRUint32 numCols = row.Length();
    for (PRUint32 colX = 0; colX < numCols; colX++) {
      
      CellData* data = row.ElementAt(colX);
      if (data && data->IsOrig()) {
        AppendCell(aMap, data->GetCellFrame(), rowX, false, 0, damageArea);
      }
    }
    rowX++;
  }

  
  for (rowX = 0; rowX < numOrigRows; rowX++) {
    CellDataArray& row = origRows[rowX];
    PRUint32 len = row.Length();
    for (PRUint32 colX = 0; colX < len; colX++) {
      DestroyCellData(row[colX]);
    }
  }
}

void
nsCellMap::RebuildConsideringCells(nsTableCellMap&              aMap,
                                   PRInt32                      aNumOrigCols,
                                   nsTArray<nsTableCellFrame*>* aCellFrames,
                                   PRInt32                      aRowIndex,
                                   PRInt32                      aColIndex,
                                   bool                         aInsert)
{
  NS_ASSERTION(!!aMap.mBCInfo == mIsBC, "BC state mismatch");
  
  PRInt32 numOrigRows  = mRows.Length();
  nsTArray<CellDataArray> origRows;
  mRows.SwapElements(origRows);

  PRInt32 numNewCells = (aCellFrames) ? aCellFrames->Length() : 0;

  
  NS_ASSERTION(aNumOrigCols >= aColIndex, "Appending cells far beyond cellmap data?!");
  PRInt32 numCols = aInsert ? NS_MAX(aNumOrigCols, aColIndex + 1) : aNumOrigCols;

  
  
  PRInt32 rowX;
  nsRect damageArea;
  for (rowX = 0; rowX < numOrigRows; rowX++) {
    const CellDataArray& row = origRows[rowX];
    for (PRInt32 colX = 0; colX < numCols; colX++) {
      if ((rowX == aRowIndex) && (colX == aColIndex)) {
        if (aInsert) { 
          for (PRInt32 cellX = 0; cellX < numNewCells; cellX++) {
            nsTableCellFrame* cell = aCellFrames->ElementAt(cellX);
            if (cell) {
              AppendCell(aMap, cell, rowX, false, 0, damageArea);
            }
          }
        }
        else {
          continue; 
        }
      }
      
      CellData* data = row.SafeElementAt(colX);
      if (data && data->IsOrig()) {
        AppendCell(aMap, data->GetCellFrame(), rowX, false, 0, damageArea);
      }
    }
  }
  if (aInsert && numOrigRows <= aRowIndex) { 
    NS_ASSERTION (numOrigRows == aRowIndex, "Appending cells far beyond the last row");
    for (PRInt32 cellX = 0; cellX < numNewCells; cellX++) {
      nsTableCellFrame* cell = aCellFrames->ElementAt(cellX);
      if (cell) {
        AppendCell(aMap, cell, aRowIndex, false, 0, damageArea);
      }
    }
  }

  
  for (rowX = 0; rowX < numOrigRows; rowX++) {
    CellDataArray& row = origRows[rowX];
    PRUint32 len = row.Length();
    for (PRUint32 colX = 0; colX < len; colX++) {
      DestroyCellData(row.SafeElementAt(colX));
    }
  }
  
  if (mRows.Length() < PRUint32(mContentRowCount)) {
    Grow(aMap, mContentRowCount - mRows.Length());
  }

}

void nsCellMap::RemoveCell(nsTableCellMap&   aMap,
                           nsTableCellFrame* aCellFrame,
                           PRInt32           aRowIndex,
                           PRInt32           aRgFirstRowIndex,
                           nsRect&           aDamageArea)
{
  PRUint32 numRows = mRows.Length();
  if (PRUint32(aRowIndex) >= numRows) {
    NS_ERROR("bad arg in nsCellMap::RemoveCell");
    return;
  }
  PRInt32 numCols = aMap.GetColCount();

  

  
  PRInt32 startColIndex;
  for (startColIndex = 0; startColIndex < numCols; startColIndex++) {
    CellData* data = mRows[aRowIndex].SafeElementAt(startColIndex);
    if (data && (data->IsOrig()) && (aCellFrame == data->GetCellFrame())) {
      break; 
    }
  }

  PRInt32 rowSpan = GetRowSpan(aRowIndex, startColIndex, false);
  
  
  bool spansCauseRebuild = CellsSpanInOrOut(aRowIndex,
                                              aRowIndex + rowSpan - 1,
                                              startColIndex, numCols - 1);
  
  
  
  if (!aCellFrame->GetRowSpan() || !aCellFrame->GetColSpan())
    spansCauseRebuild = true;

  if (spansCauseRebuild) {
    aMap.RebuildConsideringCells(this, nsnull, aRowIndex, startColIndex, false,
                                 aDamageArea);
  }
  else {
    ShrinkWithoutCell(aMap, *aCellFrame, aRowIndex, startColIndex,
                      aRgFirstRowIndex, aDamageArea);
  }
}

void nsCellMap::ExpandZeroColSpans(nsTableCellMap& aMap)
{
  NS_ASSERTION(!!aMap.mBCInfo == mIsBC, "BC state mismatch");
  PRUint32 numRows = mRows.Length();
  PRUint32 numCols = aMap.GetColCount();
  PRUint32 rowIndex, colIndex;

  for (rowIndex = 0; rowIndex < numRows; rowIndex++) {
    for (colIndex = 0; colIndex < numCols; colIndex++) {
      CellData* data = mRows[rowIndex].SafeElementAt(colIndex);
      if (!data || !data->IsOrig())
        continue;
      nsTableCellFrame* cell = data->GetCellFrame();
      NS_ASSERTION(cell, "There has to be a cell");
      PRInt32 cellRowSpan = cell->GetRowSpan();
      PRInt32 cellColSpan = cell->GetColSpan();
      bool rowZeroSpan = (0 == cell->GetRowSpan());
      bool colZeroSpan = (0 == cell->GetColSpan());
      if (colZeroSpan) {
        aMap.mTableFrame.SetHasZeroColSpans(true);
        
        NS_ASSERTION(numRows > 0, "Bogus numRows");
        NS_ASSERTION(numCols > 0, "Bogus numCols");
        PRUint32 endRowIndex =  rowZeroSpan ? numRows - 1 :
                                              rowIndex + cellRowSpan - 1;
        PRUint32 endColIndex =  colZeroSpan ? numCols - 1 :
                                              colIndex + cellColSpan - 1;
        PRUint32 colX, rowX;
        colX = colIndex + 1;
        while (colX <= endColIndex) {
          
          
          
          
          for (rowX = rowIndex; rowX <= endRowIndex; rowX++) {
            CellData* oldData = GetDataAt(rowX, colX);
            if (oldData) {
              if (oldData->IsOrig()) {
                break; 
              }
              if (oldData->IsRowSpan()) {
                if ((rowX - rowIndex) != oldData->GetRowSpanOffset()) {
                  break;
                }
              }
              if (oldData->IsColSpan()) {
                if ((colX - colIndex) != oldData->GetColSpanOffset()) {
                  break;
                }
              }
            }
          }
          if (endRowIndex >= rowX)
            break;
          for (rowX = rowIndex; rowX <= endRowIndex; rowX++) {
            CellData* newData = AllocCellData(nsnull);
            if (!newData) return;

            newData->SetColSpanOffset(colX - colIndex);
            newData->SetZeroColSpan(true);

            if (rowX > rowIndex) {
              newData->SetRowSpanOffset(rowX - rowIndex);
              if (rowZeroSpan)
                newData->SetZeroRowSpan(true);
            }
            SetDataAt(aMap, *newData, rowX, colX);
          }
          colX++;
        }  
      } 
    }
  }
}
#ifdef NS_DEBUG
void nsCellMap::Dump(bool aIsBorderCollapse) const
{
  printf("\n  ***** START GROUP CELL MAP DUMP ***** %p\n", (void*)this);
  nsTableRowGroupFrame* rg = GetRowGroup();
  const nsStyleDisplay* display = rg->GetStyleDisplay();
  switch (display->mDisplay) {
  case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
    printf("  thead ");
    break;
  case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
    printf("  tfoot ");
    break;
  case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
    printf("  tbody ");
    break;
  default:
    printf("HUH? wrong display type on rowgroup");
  }
  PRUint32 mapRowCount = mRows.Length();
  printf("mapRowCount=%u tableRowCount=%d\n", mapRowCount, mContentRowCount);


  PRUint32 rowIndex, colIndex;
  for (rowIndex = 0; rowIndex < mapRowCount; rowIndex++) {
    const CellDataArray& row = mRows[rowIndex];
    printf("  row %d : ", rowIndex);
    PRUint32 colCount = row.Length();
    for (colIndex = 0; colIndex < colCount; colIndex++) {
      CellData* cd = row[colIndex];
      if (cd) {
        if (cd->IsOrig()) {
          printf("C%d,%d  ", rowIndex, colIndex);
        } else {
          nsTableCellFrame* cell = nsnull;
          if (cd->IsRowSpan()) {
            cell = GetCellFrame(rowIndex, colIndex, *cd, true);
            printf("R ");
          }
          if (cd->IsColSpan()) {
            cell = GetCellFrame(rowIndex, colIndex, *cd, false);
            printf("C ");
          }
          if (!(cd->IsRowSpan() && cd->IsColSpan())) {
            printf("  ");
          }
          printf("  ");
        }
      } else {
        printf("----  ");
      }
    }
    if (aIsBorderCollapse) {
      nscoord       size;
      BCBorderOwner owner;
      mozilla::css::Side side;
      bool          segStart;
      bool          bevel;
      for (PRInt32 i = 0; i <= 2; i++) {
        printf("\n          ");
        for (colIndex = 0; colIndex < colCount; colIndex++) {
          BCCellData* cd = (BCCellData *)row[colIndex];
          if (cd) {
            if (0 == i) {
              size = cd->mData.GetTopEdge(owner, segStart);
              printf("t=%d%d%d ", size, owner, segStart);
            }
            else if (1 == i) {
              size = cd->mData.GetLeftEdge(owner, segStart);
              printf("l=%d%d%d ", size, owner, segStart);
            }
            else {
              size = cd->mData.GetCorner(side, bevel);
              printf("c=%d%d%d ", size, side, bevel);
            }
          }
        }
      }
    }
    printf("\n");
  }

  
  PRUint32 cellCount = 0;
  for (PRUint32 rIndex = 0; rIndex < mapRowCount; rIndex++) {
    const CellDataArray& row = mRows[rIndex];
    PRUint32 colCount = row.Length();
    printf("  ");
    for (colIndex = 0; colIndex < colCount; colIndex++) {
      CellData* cd = row[colIndex];
      if (cd) {
        if (cd->IsOrig()) {
          nsTableCellFrame* cellFrame = cd->GetCellFrame();
          PRInt32 cellFrameColIndex;
          cellFrame->GetColIndex(cellFrameColIndex);
          printf("C%d,%d=%p(%d)  ", rIndex, colIndex, (void*)cellFrame,
                 cellFrameColIndex);
          cellCount++;
        }
      }
    }
    printf("\n");
  }

  printf("  ***** END GROUP CELL MAP DUMP *****\n");
}
#endif

CellData*
nsCellMap::GetDataAt(PRInt32         aMapRowIndex,
                     PRInt32         aColIndex) const
{
  return
    mRows.SafeElementAt(aMapRowIndex, *sEmptyRow).SafeElementAt(aColIndex);
}



void nsCellMap::SetDataAt(nsTableCellMap& aMap,
                          CellData&       aNewCell,
                          PRInt32         aMapRowIndex,
                          PRInt32         aColIndex)
{
  NS_ASSERTION(!!aMap.mBCInfo == mIsBC, "BC state mismatch");
  if (PRUint32(aMapRowIndex) >= mRows.Length()) {
    NS_ERROR("SetDataAt called with row index > num rows");
    return;
  }

  CellDataArray& row = mRows[aMapRowIndex];

  
  PRInt32 numColsToAdd = aColIndex + 1 - aMap.GetColCount();
  if (numColsToAdd > 0) {
    aMap.AddColsAtEnd(numColsToAdd);
  }
  
  numColsToAdd = aColIndex + 1 - row.Length();
  if (numColsToAdd > 0) {
    
    GrowRow(row, numColsToAdd);
  }

  DestroyCellData(row[aColIndex]);

  row.ReplaceElementsAt(aColIndex, 1, &aNewCell);
  
  nsColInfo* colInfo = aMap.GetColInfoAt(aColIndex);
  if (colInfo) {
    if (aNewCell.IsOrig()) {
      colInfo->mNumCellsOrig++;
    }
    else if (aNewCell.IsColSpan()) {
      colInfo->mNumCellsSpan++;
    }
  }
  else NS_ERROR("SetDataAt called with col index > table map num cols");
}

nsTableCellFrame*
nsCellMap::GetCellInfoAt(const nsTableCellMap& aMap,
                         PRInt32               aRowX,
                         PRInt32               aColX,
                         bool*               aOriginates,
                         PRInt32*              aColSpan) const
{
  if (aOriginates) {
    *aOriginates = false;
  }
  CellData* data = GetDataAt(aRowX, aColX);
  nsTableCellFrame* cellFrame = nsnull;
  if (data) {
    if (data->IsOrig()) {
      cellFrame = data->GetCellFrame();
      if (aOriginates)
        *aOriginates = true;
    }
    else {
      cellFrame = GetCellFrame(aRowX, aColX, *data, true);
    }
    if (cellFrame && aColSpan) {
      PRInt32 initialColIndex;
      cellFrame->GetColIndex(initialColIndex);
      bool zeroSpan;
      *aColSpan = GetEffectiveColSpan(aMap, aRowX, initialColIndex, zeroSpan);
    }
  }
  return cellFrame;
}


bool nsCellMap::RowIsSpannedInto(PRInt32         aRowIndex,
                                   PRInt32         aNumEffCols) const
{
  if ((0 > aRowIndex) || (aRowIndex >= mContentRowCount)) {
    return false;
  }
  for (PRInt32 colIndex = 0; colIndex < aNumEffCols; colIndex++) {
    CellData* cd = GetDataAt(aRowIndex, colIndex);
    if (cd) { 
      if (cd->IsSpan()) { 
        if (cd->IsRowSpan() && GetCellFrame(aRowIndex, colIndex, *cd, true)) { 
          return true;
        }
      }
    }
  }
  return false;
}

bool nsCellMap::RowHasSpanningCells(PRInt32 aRowIndex,
                                      PRInt32 aNumEffCols) const
{
  if ((0 > aRowIndex) || (aRowIndex >= mContentRowCount)) {
    return false;
  }
  if (aRowIndex != mContentRowCount - 1) {
    
    for (PRInt32 colIndex = 0; colIndex < aNumEffCols; colIndex++) {
      CellData* cd = GetDataAt(aRowIndex, colIndex);
      if (cd && (cd->IsOrig())) { 
        CellData* cd2 = GetDataAt(aRowIndex + 1, colIndex);
        if (cd2 && cd2->IsRowSpan()) { 
          if (cd->GetCellFrame() == GetCellFrame(aRowIndex + 1, colIndex, *cd2, true)) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

void nsCellMap::DestroyCellData(CellData* aData)
{
  if (!aData) {
    return;
  }

  if (mIsBC) {
    BCCellData* bcData = static_cast<BCCellData*>(aData);
    bcData->~BCCellData();
    mPresContext->FreeToShell(sizeof(BCCellData), bcData);
  } else {
    aData->~CellData();
    mPresContext->FreeToShell(sizeof(CellData), aData);
  }
}

CellData* nsCellMap::AllocCellData(nsTableCellFrame* aOrigCell)
{
  if (mIsBC) {
    BCCellData* data = (BCCellData*)
      mPresContext->AllocateFromShell(sizeof(BCCellData));
    if (data) {
      new (data) BCCellData(aOrigCell);
    }
    return data;
  }

  CellData* data = (CellData*)
    mPresContext->AllocateFromShell(sizeof(CellData));
  if (data) {
    new (data) CellData(aOrigCell);
  }
  return data;
}

void
nsCellMapColumnIterator::AdvanceRowGroup()
{
  do {
    mCurMapStart += mCurMapContentRowCount;
    mCurMap = mCurMap->GetNextSibling();
    if (!mCurMap) {
      
      
      
      
      
      mCurMapContentRowCount = 0;
      mCurMapRelevantRowCount = 0;
      break;
    }

    mCurMapContentRowCount = mCurMap->GetRowCount();
    PRUint32 rowArrayLength = mCurMap->mRows.Length();
    mCurMapRelevantRowCount = NS_MIN(mCurMapContentRowCount, rowArrayLength);
  } while (0 == mCurMapRelevantRowCount);

  NS_ASSERTION(mCurMapRelevantRowCount != 0 || !mCurMap,
               "How did that happen?");

  
  mCurMapRow = 0;
}

void
nsCellMapColumnIterator::IncrementRow(PRInt32 aIncrement)
{
  NS_PRECONDITION(aIncrement >= 0, "Bogus increment");
  NS_PRECONDITION(mCurMap, "Bogus mOrigCells?");
  if (aIncrement == 0) {
    AdvanceRowGroup();
  }
  else {
    mCurMapRow += aIncrement;
    if (mCurMapRow >= mCurMapRelevantRowCount) {
      AdvanceRowGroup();
    }
  }
}

nsTableCellFrame*
nsCellMapColumnIterator::GetNextFrame(PRInt32* aRow, PRInt32* aColSpan)
{
  
  
  if (mFoundCells == mOrigCells) {
    *aRow = 0;
    *aColSpan = 1;
    return nsnull;
  }

  while (1) {
    NS_ASSERTION(mCurMapRow < mCurMapRelevantRowCount, "Bogus mOrigCells?");
    
    
    
    const nsCellMap::CellDataArray& row = mCurMap->mRows[mCurMapRow];
    CellData* cellData = row.SafeElementAt(mCol);
    if (!cellData || cellData->IsDead()) {
      
      
      IncrementRow(1);
      continue;
    }

    if (cellData->IsColSpan()) {
      
      PRInt32 rowspanOffset = cellData->GetRowSpanOffset();
      nsTableCellFrame* cellFrame = mCurMap->GetCellFrame(mCurMapRow, mCol, *cellData, false);
      NS_ASSERTION(cellFrame,"Must have usable originating data here");
      PRInt32 rowSpan = cellFrame->GetRowSpan();
      if (rowSpan == 0) {
        AdvanceRowGroup();
      }
      else {
        IncrementRow(rowSpan - rowspanOffset);
      }
      continue;
    }

    NS_ASSERTION(cellData->IsOrig(),
                 "Must have originating cellData by this point.  "
                 "See comment on mCurMapRow in header.");

    nsTableCellFrame* cellFrame = cellData->GetCellFrame();
    NS_ASSERTION(cellFrame, "Orig data without cellframe?");

    *aRow = mCurMapStart + mCurMapRow;
    bool ignoredZeroSpan;
    *aColSpan = mCurMap->GetEffectiveColSpan(*mMap, mCurMapRow, mCol,
                                             ignoredZeroSpan);

    IncrementRow(cellFrame->GetRowSpan());

    ++mFoundCells;

    NS_ABORT_IF_FALSE(cellData == mMap->GetDataAt(*aRow, mCol),
                      "Giving caller bogus row?");

    return cellFrame;
  }

  NS_NOTREACHED("Can't get here");
  return nsnull;
}
