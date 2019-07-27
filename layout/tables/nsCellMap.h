



#ifndef nsCellMap_h__
#define nsCellMap_h__

#include "nscore.h"
#include "celldata.h"
#include "nsTArray.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsAlgorithm.h"
#include "nsAutoPtr.h"
#include "nsRect.h"
#include <algorithm>

#undef DEBUG_TABLE_CELLMAP

class nsTableCellFrame;
class nsTableRowFrame;
class nsTableRowGroupFrame;
class nsTableFrame;
class nsCellMap;
class nsPresContext;
class nsCellMapColumnIterator;

struct nsColInfo
{
  int32_t mNumCellsOrig; 
  int32_t mNumCellsSpan; 

  nsColInfo();
  nsColInfo(int32_t aNumCellsOrig,
            int32_t aNumCellsSpan);
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

  nsTableCellFrame* GetCellFrame(int32_t   aRowIndex,
                                 int32_t   aColIndex,
                                 CellData& aData,
                                 bool      aUseRowIfOverlap) const;

  
  CellData* GetDataAt(int32_t aRowIndex,
                      int32_t aColIndex) const;

  
  nsColInfo* GetColInfoAt(int32_t aColIndex);

  

  CellData* AppendCell(nsTableCellFrame&     aCellFrame,
                       int32_t               aRowIndex,
                       bool                  aRebuildIfNecessary,
                       nsIntRect&            aDamageArea);

  void InsertCells(nsTArray<nsTableCellFrame*>& aCellFrames,
                   int32_t                      aRowIndex,
                   int32_t                      aColIndexBefore,
                   nsIntRect&                   aDamageArea);

  void RemoveCell(nsTableCellFrame* aCellFrame,
                  int32_t           aRowIndex,
                  nsIntRect&        aDamageArea);
  
  void ClearCols();
  void InsertRows(nsTableRowGroupFrame*       aRowGroup,
                  nsTArray<nsTableRowFrame*>& aRows,
                  int32_t                     aFirstRowIndex,
                  bool                        aConsiderSpans,
                  nsIntRect&                  aDamageArea);

  void RemoveRows(int32_t         aFirstRowIndex,
                  int32_t         aNumRowsToRemove,
                  bool            aConsiderSpans,
                  nsIntRect&      aDamageArea);

  int32_t GetNumCellsOriginatingInRow(int32_t aRowIndex) const;
  int32_t GetNumCellsOriginatingInCol(int32_t aColIndex) const;

  


  bool HasMoreThanOneCell(int32_t aRowIndex) const;

  int32_t GetEffectiveRowSpan(int32_t aRowIndex,
                              int32_t aColIndex) const;
  int32_t GetEffectiveColSpan(int32_t aRowIndex,
                              int32_t aColIndex) const;

  
  int32_t GetColCount() const;

  
  int32_t GetRowCount() const;

  nsTableCellFrame* GetCellInfoAt(int32_t  aRowX,
                                  int32_t  aColX,
                                  bool*  aOriginates = nullptr,
                                  int32_t* aColSpan = nullptr) const;

  








  int32_t GetIndexByRowAndColumn(int32_t aRow, int32_t aColumn) const;

  








  void GetRowAndColumnByIndex(int32_t aIndex,
                              int32_t *aRow, int32_t *aColumn) const;

  void AddColsAtEnd(uint32_t aNumCols);
  void RemoveColsAtEnd();

  bool RowIsSpannedInto(int32_t aRowIndex, int32_t aNumEffCols) const;
  bool RowHasSpanningCells(int32_t aRowIndex, int32_t aNumEffCols) const;
  void RebuildConsideringCells(nsCellMap*                   aCellMap,
                               nsTArray<nsTableCellFrame*>* aCellFrames,
                               int32_t                      aRowIndex,
                               int32_t                      aColIndex,
                               bool                         aInsert,
                               nsIntRect&                   aDamageArea);

protected:
  







  void RebuildConsideringRows(nsCellMap*                  aCellMap,
                              int32_t                     aStartRowIndex,
                              nsTArray<nsTableRowFrame*>* aRowsToInsert,
                              int32_t                     aNumRowsToRemove,
                              nsIntRect&                  aDamageArea);

public:
  void ExpandZeroColSpans();

  void ResetTopStart(uint8_t    aSide,
                     nsCellMap& aCellMap,
                     uint32_t   aYPos,
                     uint32_t   aXPos,
                     bool       aIsLowerRight = false);

  void SetBCBorderEdge(mozilla::Side aEdge,
                       nsCellMap&    aCellMap,
                       uint32_t      aCellMapStart,
                       uint32_t      aYPos,
                       uint32_t      aXPos,
                       uint32_t      aLength,
                       BCBorderOwner aOwner,
                       nscoord       aSize,
                       bool          aChanged);

  void SetBCBorderCorner(::Corner    aCorner,
                         nsCellMap&  aCellMap,
                         uint32_t    aCellMapStart,
                         uint32_t    aYPos,
                         uint32_t    aXPos,
                         mozilla::Side aOwner,
                         nscoord     aSubSize,
                         bool        aBevel,
                         bool        aIsBottomRight = false);

  
#ifdef DEBUG
  void Dump(char* aString = nullptr) const;
#endif

protected:
  BCData* GetRightMostBorder(int32_t aRowIndex);
  BCData* GetBottomMostBorder(int32_t aColIndex);

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

  nsTableCellFrame* GetCellFrame(int32_t   aRowIndex,
                                 int32_t   aColIndex,
                                 CellData& aData,
                                 bool      aUseRowSpanIfOverlap) const;

  




  int32_t GetHighestIndex(int32_t aColCount);

  








  int32_t GetIndexByRowAndColumn(int32_t aColCount,
                                 int32_t aRow, int32_t aColumn) const;

  









  void GetRowAndColumnByIndex(int32_t aColCount, int32_t aIndex,
                              int32_t *aRow, int32_t *aColumn) const;

  

















  CellData* AppendCell(nsTableCellMap&   aMap,
                       nsTableCellFrame* aCellFrame,
                       int32_t           aRowIndex,
                       bool              aRebuildIfNecessary,
                       int32_t           aRgFirstRowIndex,
                       nsIntRect&        aDamageArea,
                       int32_t*          aBeginSearchAtCol = nullptr);

  










  void CollapseZeroColSpan(nsTableCellMap& aMap,
                           CellData*       aOrigData,
                           int32_t         aRowIndex,
                           int32_t         aColIndex);

  void InsertCells(nsTableCellMap&              aMap,
                   nsTArray<nsTableCellFrame*>& aCellFrames,
                   int32_t                      aRowIndex,
                   int32_t                      aColIndexBefore,
                   int32_t                      aRgFirstRowIndex,
                   nsIntRect&                   aDamageArea);

  void RemoveCell(nsTableCellMap&   aMap,
                  nsTableCellFrame* aCellFrame,
                  int32_t           aRowIndex,
                  int32_t           aRgFirstRowIndex,
                  nsIntRect&        aDamageArea);

  void InsertRows(nsTableCellMap&             aMap,
                  nsTArray<nsTableRowFrame*>& aRows,
                  int32_t                     aFirstRowIndex,
                  bool                        aConsiderSpans,
                  int32_t                     aRgFirstRowIndex,
                  nsIntRect&                  aDamageArea);

  void RemoveRows(nsTableCellMap& aMap,
                  int32_t         aFirstRowIndex,
                  int32_t         aNumRowsToRemove,
                  bool            aConsiderSpans,
                  int32_t         aRgFirstRowIndex,
                  nsIntRect&      aDamageArea);

  int32_t GetNumCellsOriginatingInRow(int32_t aRowIndex) const;
  int32_t GetNumCellsOriginatingInCol(int32_t aColIndex) const;

  
  int32_t GetRowCount(bool aConsiderDeadRowSpanRows = false) const;

  nsTableCellFrame* GetCellInfoAt(const nsTableCellMap& aMap,
                                  int32_t          aRowX,
                                  int32_t          aColX,
                                  bool*          aOriginates = nullptr,
                                  int32_t*         aColSpan = nullptr) const;

  bool RowIsSpannedInto(int32_t aRowIndex,
                          int32_t aNumEffCols) const;

  bool RowHasSpanningCells(int32_t aRowIndex,
                             int32_t aNumEffCols) const;

  void ExpandZeroColSpans(nsTableCellMap& aMap);

  


  bool HasMoreThanOneCell(int32_t aRowIndex) const;

  





  int32_t GetRowSpan(int32_t aRowIndex,
                     int32_t aColIndex,
                     bool    aGetEffective) const;

  int32_t GetEffectiveColSpan(const nsTableCellMap& aMap,
                              int32_t     aRowIndex,
                              int32_t     aColIndex,
                              bool&     aIsZeroColSpan) const;

  typedef nsTArray<CellData*> CellDataArray;

  
#ifdef DEBUG
  void Dump(bool aIsBorderCollapse) const;
#endif

protected:
  friend class nsTableCellMap;
  friend class BCMapCellIterator;
  friend class BCPaintBorderIterator;
  friend class nsTableFrame;
  friend class nsCellMapColumnIterator;

  



  bool Grow(nsTableCellMap& aMap,
              int32_t         aNumRows,
              int32_t         aRowIndex = -1);

  void GrowRow(CellDataArray& aRow,
               int32_t        aNumCols);

  
  void SetDataAt(nsTableCellMap& aMap,
                 CellData&       aCellData,
                 int32_t         aMapRowIndex,
                 int32_t         aColIndex);

  CellData* GetDataAt(int32_t         aMapRowIndex,
                      int32_t         aColIndex) const;

  int32_t GetNumCellsIn(int32_t aColIndex) const;

  void ExpandWithRows(nsTableCellMap&             aMap,
                      nsTArray<nsTableRowFrame*>& aRowFrames,
                      int32_t                     aStartRowIndex,
                      int32_t                     aRgFirstRowIndex,
                      nsIntRect&                  aDamageArea);

  void ExpandWithCells(nsTableCellMap&              aMap,
                       nsTArray<nsTableCellFrame*>& aCellFrames,
                       int32_t                      aRowIndex,
                       int32_t                      aColIndex,
                       int32_t                      aRowSpan,
                       bool                         aRowSpanIsZero,
                       int32_t                      aRgFirstRowIndex,
                       nsIntRect&                   aDamageArea);

  void ShrinkWithoutRows(nsTableCellMap& aMap,
                         int32_t         aFirstRowIndex,
                         int32_t         aNumRowsToRemove,
                         int32_t         aRgFirstRowIndex,
                         nsIntRect&      aDamageArea);

  void ShrinkWithoutCell(nsTableCellMap&   aMap,
                         nsTableCellFrame& aCellFrame,
                         int32_t           aRowIndex,
                         int32_t           aColIndex,
                         int32_t           aRgFirstRowIndex,
                         nsIntRect&        aDamageArea);

  







  void RebuildConsideringRows(nsTableCellMap&             aMap,
                              int32_t                     aStartRowIndex,
                              nsTArray<nsTableRowFrame*>* aRowsToInsert,
                              int32_t                     aNumRowsToRemove);

  void RebuildConsideringCells(nsTableCellMap&              aMap,
                               int32_t                      aNumOrigCols,
                               nsTArray<nsTableCellFrame*>* aCellFrames,
                               int32_t                      aRowIndex,
                               int32_t                      aColIndex,
                               bool                         aInsert);

  bool CellsSpanOut(nsTArray<nsTableRowFrame*>& aNewRows) const;

  










  bool CellsSpanInOrOut(int32_t aStartRowIndex,
                          int32_t aEndRowIndex,
                          int32_t aStartColIndex,
                          int32_t aEndColIndex) const;

  void ExpandForZeroSpan(nsTableCellFrame* aCellFrame,
                         int32_t           aNumColsInTable);

  bool CreateEmptyRow(int32_t aRowIndex,
                        int32_t aNumCols);

  int32_t GetRowSpanForNewCell(nsTableCellFrame* aCellFrameToAdd,
                               int32_t           aRowIndex,
                               bool&           aIsZeroRowSpan) const;

  int32_t GetColSpanForNewCell(nsTableCellFrame& aCellFrameToAdd,
                               bool&           aIsZeroColSpan) const;

  
  
  void DestroyCellData(CellData* aData);
  
  
  
  CellData* AllocCellData(nsTableCellFrame* aOrigCell);

  


  
  nsTArray<CellDataArray> mRows;

  



  int32_t mContentRowCount;

  
  nsTableRowGroupFrame* mRowGroupFrame;

  
  nsCellMap* mNextSibling;

  
  bool mIsBC;

  
  nsRefPtr<nsPresContext> mPresContext;
};





class nsCellMapColumnIterator
{
public:
  nsCellMapColumnIterator(const nsTableCellMap* aMap, int32_t aCol) :
    mMap(aMap), mCurMap(aMap->mFirstMap), mCurMapStart(0),
    mCurMapRow(0), mCol(aCol), mFoundCells(0)
  {
    NS_PRECONDITION(aMap, "Must have map");
    NS_PRECONDITION(mCol < aMap->GetColCount(), "Invalid column");
    mOrigCells = aMap->GetNumCellsOriginatingInCol(mCol);
    if (mCurMap) {
      mCurMapContentRowCount = mCurMap->GetRowCount();
      uint32_t rowArrayLength = mCurMap->mRows.Length();
      mCurMapRelevantRowCount = std::min(mCurMapContentRowCount, rowArrayLength);
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

  nsTableCellFrame* GetNextFrame(int32_t* aRow, int32_t* aColSpan);

private:
  void AdvanceRowGroup();

  
  
  void IncrementRow(int32_t aIncrement);

  const nsTableCellMap* mMap;
  const nsCellMap* mCurMap;

  
  
  
  uint32_t mCurMapStart;

  
  
  
  
  
  uint32_t mCurMapRow;
  const int32_t mCol;
  uint32_t mOrigCells;
  uint32_t mFoundCells;

  
  
  uint32_t mCurMapContentRowCount;

  
  
  
  uint32_t mCurMapRelevantRowCount;
};



inline int32_t nsTableCellMap::GetColCount() const
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

inline int32_t nsCellMap::GetRowCount(bool aConsiderDeadRowSpanRows) const
{
  int32_t rowCount = (aConsiderDeadRowSpanRows) ? mRows.Length() : mContentRowCount;
  return rowCount;
}



inline nsColInfo::nsColInfo()
 :mNumCellsOrig(0), mNumCellsSpan(0)
{}

inline nsColInfo::nsColInfo(int32_t aNumCellsOrig,
                            int32_t aNumCellsSpan)
 :mNumCellsOrig(aNumCellsOrig), mNumCellsSpan(aNumCellsSpan)
{}


#endif
