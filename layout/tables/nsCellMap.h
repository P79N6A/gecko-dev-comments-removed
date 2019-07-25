



































#ifndef nsCellMap_h__
#define nsCellMap_h__

#include "nscore.h"
#include "celldata.h"
#include "nsTArray.h"
#include "nsTArray.h"
#include "nsRect.h"
#include "nsCOMPtr.h"
#include "nsAlgorithm.h"
#include "nsAutoPtr.h"

#undef DEBUG_TABLE_CELLMAP

class nsTableColFrame;
class nsTableCellFrame;
class nsTableRowFrame;
class nsTableRowGroupFrame;
class nsTableFrame;
class nsCellMap;
class nsPresContext;
class nsCellMapColumnIterator;

struct nsColInfo
{
  PRInt32 mNumCellsOrig; 
  PRInt32 mNumCellsSpan; 

  nsColInfo();
  nsColInfo(PRInt32 aNumCellsOrig,
            PRInt32 aNumCellsSpan);
};

enum Corner
{
  eTopLeft     = 0,
  eTopRight    = 1,
  eBottomRight = 2,
  eBottomLeft  = 3
};

struct BCInfo
{
  nsTArray<BCData> mRightBorders;
  nsTArray<BCData> mBottomBorders;
  BCData           mLowerRightCorner;
};

class nsTableCellMap
{
public:
  nsTableCellMap(nsTableFrame&   aTableFrame,
                 bool            aBorderCollapse);

  


  ~nsTableCellMap();

  void RemoveGroupCellMap(nsTableRowGroupFrame* aRowGroup);

  void InsertGroupCellMap(nsTableRowGroupFrame*  aNewRowGroup,
                          nsTableRowGroupFrame*& aPrevRowGroup);

  







  nsCellMap* GetMapFor(const nsTableRowGroupFrame* aRowGroup,
                       nsCellMap* aStartHint) const;

  
  void Synchronize(nsTableFrame* aTableFrame);

  nsTableCellFrame* GetCellFrame(PRInt32   aRowIndex,
                                 PRInt32   aColIndex,
                                 CellData& aData,
                                 bool      aUseRowIfOverlap) const;

  
  CellData* GetDataAt(PRInt32 aRowIndex,
                      PRInt32 aColIndex) const;

  
  nsColInfo* GetColInfoAt(PRInt32 aColIndex);

  

  CellData* AppendCell(nsTableCellFrame&     aCellFrame,
                       PRInt32               aRowIndex,
                       bool                  aRebuildIfNecessary,
                       nsRect&               aDamageArea);

  void InsertCells(nsTArray<nsTableCellFrame*>& aCellFrames,
                   PRInt32                      aRowIndex,
                   PRInt32                      aColIndexBefore,
                   nsRect&                      aDamageArea);

  void RemoveCell(nsTableCellFrame* aCellFrame,
                  PRInt32           aRowIndex,
                  nsRect&           aDamageArea);
  
  void ClearCols();
  void InsertRows(nsTableRowGroupFrame*       aRowGroup,
                  nsTArray<nsTableRowFrame*>& aRows,
                  PRInt32                     aFirstRowIndex,
                  bool                        aConsiderSpans,
                  nsRect&                     aDamageArea);

  void RemoveRows(PRInt32         aFirstRowIndex,
                  PRInt32         aNumRowsToRemove,
                  bool            aConsiderSpans,
                  nsRect&               aDamageArea);

  PRInt32 GetNumCellsOriginatingInRow(PRInt32 aRowIndex) const;
  PRInt32 GetNumCellsOriginatingInCol(PRInt32 aColIndex) const;

  


  bool HasMoreThanOneCell(PRInt32 aRowIndex) const;

  PRInt32 GetEffectiveRowSpan(PRInt32 aRowIndex,
                              PRInt32 aColIndex) const;
  PRInt32 GetEffectiveColSpan(PRInt32 aRowIndex,
                              PRInt32 aColIndex) const;

  
  PRInt32 GetColCount() const;

  
  PRInt32 GetRowCount() const;

  nsTableCellFrame* GetCellInfoAt(PRInt32  aRowX,
                                  PRInt32  aColX,
                                  bool*  aOriginates = nsnull,
                                  PRInt32* aColSpan = nsnull) const;

  








  PRInt32 GetIndexByRowAndColumn(PRInt32 aRow, PRInt32 aColumn) const;

  








  void GetRowAndColumnByIndex(PRInt32 aIndex,
                              PRInt32 *aRow, PRInt32 *aColumn) const;

  void AddColsAtEnd(PRUint32 aNumCols);
  void RemoveColsAtEnd();

  bool RowIsSpannedInto(PRInt32 aRowIndex, PRInt32 aNumEffCols) const;
  bool RowHasSpanningCells(PRInt32 aRowIndex, PRInt32 aNumEffCols) const;
  void RebuildConsideringCells(nsCellMap*                   aCellMap,
                               nsTArray<nsTableCellFrame*>* aCellFrames,
                               PRInt32                      aRowIndex,
                               PRInt32                      aColIndex,
                               bool                         aInsert,
                               nsRect&                      aDamageArea);

protected:
  







  void RebuildConsideringRows(nsCellMap*                  aCellMap,
                              PRInt32                     aStartRowIndex,
                              nsTArray<nsTableRowFrame*>* aRowsToInsert,
                              PRInt32                     aNumRowsToRemove,
                              nsRect&                     aDamageArea);

public:
  void ExpandZeroColSpans();

  void ResetTopStart(PRUint8    aSide,
                     nsCellMap& aCellMap,
                     PRUint32   aYPos,
                     PRUint32   aXPos,
                     bool       aIsLowerRight = false);

  void SetBCBorderEdge(mozilla::css::Side aEdge,
                       nsCellMap&    aCellMap,
                       PRUint32      aCellMapStart,
                       PRUint32      aYPos,
                       PRUint32      aXPos,
                       PRUint32      aLength,
                       BCBorderOwner aOwner,
                       nscoord       aSize,
                       bool          aChanged);

  void SetBCBorderCorner(Corner      aCorner,
                         nsCellMap&  aCellMap,
                         PRUint32    aCellMapStart,
                         PRUint32    aYPos,
                         PRUint32    aXPos,
                         mozilla::css::Side aOwner,
                         nscoord     aSubSize,
                         bool        aBevel,
                         bool        aIsBottomRight = false);

  
#ifdef NS_DEBUG
  void Dump(char* aString = nsnull) const;
#endif

protected:
  BCData* GetRightMostBorder(PRInt32 aRowIndex);
  BCData* GetBottomMostBorder(PRInt32 aColIndex);

  friend class nsCellMap;
  friend class BCMapCellIterator;
  friend class BCPaintBorderIterator;
  friend class nsCellMapColumnIterator;





  void InsertGroupCellMap(nsCellMap* aPrevMap,
                          nsCellMap& aNewMap);
  void DeleteRightBottomBorders();

  nsTableFrame&               mTableFrame;
  nsAutoTArray<nsColInfo, 8>  mCols;
  nsCellMap*                  mFirstMap;
  
  BCInfo*                     mBCInfo;
};














class nsCellMap
{
public:
  



  nsCellMap(nsTableRowGroupFrame* aRowGroupFrame, bool aIsBC);

  


  ~nsCellMap();

  static void Init();
  static void Shutdown();

  nsCellMap* GetNextSibling() const;
  void SetNextSibling(nsCellMap* aSibling);

  nsTableRowGroupFrame* GetRowGroup() const;

  nsTableCellFrame* GetCellFrame(PRInt32   aRowIndex,
                                 PRInt32   aColIndex,
                                 CellData& aData,
                                 bool      aUseRowSpanIfOverlap) const;

  




  PRInt32 GetHighestIndex(PRInt32 aColCount);

  








  PRInt32 GetIndexByRowAndColumn(PRInt32 aColCount,
                                 PRInt32 aRow, PRInt32 aColumn) const;

  









  void GetRowAndColumnByIndex(PRInt32 aColCount, PRInt32 aIndex,
                              PRInt32 *aRow, PRInt32 *aColumn) const;

  

















  CellData* AppendCell(nsTableCellMap&   aMap,
                       nsTableCellFrame* aCellFrame,
                       PRInt32           aRowIndex,
                       bool              aRebuildIfNecessary,
                       PRInt32           aRgFirstRowIndex,
                       nsRect&           aDamageArea,
                       PRInt32*          aBeginSearchAtCol = nsnull);

  










  void CollapseZeroColSpan(nsTableCellMap& aMap,
                           CellData*       aOrigData,
                           PRInt32         aRowIndex,
                           PRInt32         aColIndex);

  void InsertCells(nsTableCellMap&              aMap,
                   nsTArray<nsTableCellFrame*>& aCellFrames,
                   PRInt32                      aRowIndex,
                   PRInt32                      aColIndexBefore,
                   PRInt32                      aRgFirstRowIndex,
                   nsRect&                      aDamageArea);

  void RemoveCell(nsTableCellMap&   aMap,
                  nsTableCellFrame* aCellFrame,
                  PRInt32           aRowIndex,
                  PRInt32           aRgFirstRowIndex,
                  nsRect&           aDamageArea);

  void InsertRows(nsTableCellMap&             aMap,
                  nsTArray<nsTableRowFrame*>& aRows,
                  PRInt32                     aFirstRowIndex,
                  bool                        aConsiderSpans,
                  PRInt32                     aRgFirstRowIndex,
                  nsRect&                     aDamageArea);

  void RemoveRows(nsTableCellMap& aMap,
                  PRInt32         aFirstRowIndex,
                  PRInt32         aNumRowsToRemove,
                  bool            aConsiderSpans,
                  PRInt32         aRgFirstRowIndex,
                  nsRect&         aDamageArea);

  PRInt32 GetNumCellsOriginatingInRow(PRInt32 aRowIndex) const;
  PRInt32 GetNumCellsOriginatingInCol(PRInt32 aColIndex) const;

  
  PRInt32 GetRowCount(bool aConsiderDeadRowSpanRows = false) const;

  nsTableCellFrame* GetCellInfoAt(const nsTableCellMap& aMap,
                                  PRInt32          aRowX,
                                  PRInt32          aColX,
                                  bool*          aOriginates = nsnull,
                                  PRInt32*         aColSpan = nsnull) const;

  bool RowIsSpannedInto(PRInt32 aRowIndex,
                          PRInt32 aNumEffCols) const;

  bool RowHasSpanningCells(PRInt32 aRowIndex,
                             PRInt32 aNumEffCols) const;

  void ExpandZeroColSpans(nsTableCellMap& aMap);

  


  bool HasMoreThanOneCell(PRInt32 aRowIndex) const;

  





  PRInt32 GetRowSpan(PRInt32 aRowIndex,
                     PRInt32 aColIndex,
                     bool    aGetEffective) const;

  PRInt32 GetEffectiveColSpan(const nsTableCellMap& aMap,
                              PRInt32     aRowIndex,
                              PRInt32     aColIndex,
                              bool&     aIsZeroColSpan) const;

  typedef nsTArray<CellData*> CellDataArray;

  
#ifdef NS_DEBUG
  void Dump(bool aIsBorderCollapse) const;
#endif

protected:
  friend class nsTableCellMap;
  friend class BCMapCellIterator;
  friend class BCPaintBorderIterator;
  friend class nsTableFrame;
  friend class nsCellMapColumnIterator;

  



  bool Grow(nsTableCellMap& aMap,
              PRInt32         aNumRows,
              PRInt32         aRowIndex = -1);

  void GrowRow(CellDataArray& aRow,
               PRInt32        aNumCols);

  
  void SetDataAt(nsTableCellMap& aMap,
                 CellData&       aCellData,
                 PRInt32         aMapRowIndex,
                 PRInt32         aColIndex);

  CellData* GetDataAt(PRInt32         aMapRowIndex,
                      PRInt32         aColIndex) const;

  PRInt32 GetNumCellsIn(PRInt32 aColIndex) const;

  void ExpandWithRows(nsTableCellMap&             aMap,
                      nsTArray<nsTableRowFrame*>& aRowFrames,
                      PRInt32                     aStartRowIndex,
                      PRInt32                     aRgFirstRowIndex,
                      nsRect&                     aDamageArea);

  void ExpandWithCells(nsTableCellMap&              aMap,
                       nsTArray<nsTableCellFrame*>& aCellFrames,
                       PRInt32                      aRowIndex,
                       PRInt32                      aColIndex,
                       PRInt32                      aRowSpan,
                       bool                         aRowSpanIsZero,
                       PRInt32                      aRgFirstRowIndex,
                       nsRect&                      aDamageArea);

  void ShrinkWithoutRows(nsTableCellMap& aMap,
                         PRInt32         aFirstRowIndex,
                         PRInt32         aNumRowsToRemove,
                         PRInt32         aRgFirstRowIndex,
                         nsRect&         aDamageArea);

  void ShrinkWithoutCell(nsTableCellMap&   aMap,
                         nsTableCellFrame& aCellFrame,
                         PRInt32           aRowIndex,
                         PRInt32           aColIndex,
                         PRInt32           aRgFirstRowIndex,
                         nsRect&           aDamageArea);

  







  void RebuildConsideringRows(nsTableCellMap&             aMap,
                              PRInt32                     aStartRowIndex,
                              nsTArray<nsTableRowFrame*>* aRowsToInsert,
                              PRInt32                     aNumRowsToRemove);

  void RebuildConsideringCells(nsTableCellMap&              aMap,
                               PRInt32                      aNumOrigCols,
                               nsTArray<nsTableCellFrame*>* aCellFrames,
                               PRInt32                      aRowIndex,
                               PRInt32                      aColIndex,
                               bool                         aInsert);

  bool CellsSpanOut(nsTArray<nsTableRowFrame*>& aNewRows) const;

  










  bool CellsSpanInOrOut(PRInt32 aStartRowIndex,
                          PRInt32 aEndRowIndex,
                          PRInt32 aStartColIndex,
                          PRInt32 aEndColIndex) const;

  void ExpandForZeroSpan(nsTableCellFrame* aCellFrame,
                         PRInt32           aNumColsInTable);

  bool CreateEmptyRow(PRInt32 aRowIndex,
                        PRInt32 aNumCols);

  PRInt32 GetRowSpanForNewCell(nsTableCellFrame* aCellFrameToAdd,
                               PRInt32           aRowIndex,
                               bool&           aIsZeroRowSpan) const;

  PRInt32 GetColSpanForNewCell(nsTableCellFrame& aCellFrameToAdd,
                               bool&           aIsZeroColSpan) const;

  
  
  void DestroyCellData(CellData* aData);
  
  
  
  CellData* AllocCellData(nsTableCellFrame* aOrigCell);

  


  
  nsTArray<CellDataArray> mRows;

  



  PRInt32 mContentRowCount;

  
  nsTableRowGroupFrame* mRowGroupFrame;

  
  nsCellMap* mNextSibling;

  
  bool mIsBC;

  
  nsRefPtr<nsPresContext> mPresContext;
};





class nsCellMapColumnIterator
{
public:
  nsCellMapColumnIterator(const nsTableCellMap* aMap, PRInt32 aCol) :
    mMap(aMap), mCurMap(aMap->mFirstMap), mCurMapStart(0),
    mCurMapRow(0), mCol(aCol), mFoundCells(0)
  {
    NS_PRECONDITION(aMap, "Must have map");
    NS_PRECONDITION(mCol < aMap->GetColCount(), "Invalid column");
    mOrigCells = aMap->GetNumCellsOriginatingInCol(mCol);
    if (mCurMap) {
      mCurMapContentRowCount = mCurMap->GetRowCount();
      PRUint32 rowArrayLength = mCurMap->mRows.Length();
      mCurMapRelevantRowCount = NS_MIN(mCurMapContentRowCount, rowArrayLength);
      if (mCurMapRelevantRowCount == 0 && mOrigCells > 0) {
        
        AdvanceRowGroup();
      }
    }
#ifdef DEBUG
    else {
      NS_ASSERTION(mOrigCells == 0, "Why no rowgroups?");
    }
#endif
  }

  nsTableCellFrame* GetNextFrame(PRInt32* aRow, PRInt32* aColSpan);

private:
  void AdvanceRowGroup();

  
  
  void IncrementRow(PRInt32 aIncrement);

  const nsTableCellMap* mMap;
  const nsCellMap* mCurMap;

  
  
  
  PRUint32 mCurMapStart;

  
  
  
  
  
  PRUint32 mCurMapRow;
  const PRInt32 mCol;
  PRUint32 mOrigCells;
  PRUint32 mFoundCells;

  
  
  PRUint32 mCurMapContentRowCount;

  
  
  
  PRUint32 mCurMapRelevantRowCount;
};



inline PRInt32 nsTableCellMap::GetColCount() const
{
  return mCols.Length();
}

inline nsCellMap* nsCellMap::GetNextSibling() const
{
  return mNextSibling;
}

inline void nsCellMap::SetNextSibling(nsCellMap* aSibling)
{
  mNextSibling = aSibling;
}

inline nsTableRowGroupFrame* nsCellMap::GetRowGroup() const
{
  return mRowGroupFrame;
}

inline PRInt32 nsCellMap::GetRowCount(bool aConsiderDeadRowSpanRows) const
{
  PRInt32 rowCount = (aConsiderDeadRowSpanRows) ? mRows.Length() : mContentRowCount;
  return rowCount;
}



inline nsColInfo::nsColInfo()
 :mNumCellsOrig(0), mNumCellsSpan(0)
{}

inline nsColInfo::nsColInfo(PRInt32 aNumCellsOrig,
                            PRInt32 aNumCellsSpan)
 :mNumCellsOrig(aNumCellsOrig), mNumCellsSpan(aNumCellsSpan)
{}


#endif
