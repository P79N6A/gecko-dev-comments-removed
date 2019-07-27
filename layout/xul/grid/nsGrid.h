





#ifndef nsGrid_h___
#define nsGrid_h___

#include "nsStackLayout.h"
#include "nsIGridPart.h"
#include "nsCOMPtr.h"

class nsBoxLayoutState;
class nsGridCell;






class nsGrid
{
public:
  nsGrid();
  ~nsGrid();

  nsGridRow* GetColumnAt(int32_t aIndex, bool aIsHorizontal = true);
  nsGridRow* GetRowAt(int32_t aIndex, bool aIsHorizontal = true);
  nsGridCell* GetCellAt(int32_t aX, int32_t aY);

  void NeedsRebuild(nsBoxLayoutState& aBoxLayoutState);
  void RebuildIfNeeded();

  
  
  
  
  
  
  
  

  nsSize GetPrefRowSize(nsBoxLayoutState& aBoxLayoutState, int32_t aRowIndex, bool aIsHorizontal = true);
  nsSize GetMinRowSize(nsBoxLayoutState& aBoxLayoutState, int32_t aRowIndex, bool aIsHorizontal = true);
  nsSize GetMaxRowSize(nsBoxLayoutState& aBoxLayoutState, int32_t aRowIndex, bool aIsHorizontal = true);
  nscoord GetRowFlex(nsBoxLayoutState& aBoxLayoutState, int32_t aRowIndex, bool aIsHorizontal = true);

  nscoord GetPrefRowHeight(nsBoxLayoutState& aBoxLayoutState, int32_t aRowIndex, bool aIsHorizontal = true);
  nscoord GetMinRowHeight(nsBoxLayoutState& aBoxLayoutState, int32_t aRowIndex, bool aIsHorizontal = true);
  nscoord GetMaxRowHeight(nsBoxLayoutState& aBoxLayoutState, int32_t aRowIndex, bool aIsHorizontal = true);
  void GetRowOffsets(nsBoxLayoutState& aState, int32_t aIndex, nscoord& aTop, nscoord& aBottom, bool aIsHorizontal = true);

  void RowAddedOrRemoved(nsBoxLayoutState& aBoxLayoutState, int32_t aIndex, bool aIsHorizontal = true);
  void CellAddedOrRemoved(nsBoxLayoutState& aBoxLayoutState, int32_t aIndex, bool aIsHorizontal = true);
  void DirtyRows(nsIFrame* aRowBox, nsBoxLayoutState& aState);
#ifdef DEBUG_grid
  void PrintCellMap();
#endif
  int32_t GetExtraColumnCount(bool aIsHorizontal = true);
  int32_t GetExtraRowCount(bool aIsHorizontal = true);


  void SetBox(nsIFrame* aBox) { mBox = aBox; }
  nsIFrame* GetBox() { return mBox; }
  nsIFrame* GetRowsBox() { return mRowsBox; }
  nsIFrame* GetColumnsBox() { return mColumnsBox; }
  int32_t GetRowCount(int32_t aIsHorizontal = true);
  int32_t GetColumnCount(int32_t aIsHorizontal = true);

  static nsIFrame* GetScrolledBox(nsIFrame* aChild);
  static nsIFrame* GetScrollBox(nsIFrame* aChild);
  static nsIGridPart* GetPartFromBox(nsIFrame* aBox);
  void GetFirstAndLastRow(nsBoxLayoutState& aState, 
                          int32_t& aFirstIndex, 
                          int32_t& aLastIndex, 
                          nsGridRow*& aFirstRow,
                          nsGridRow*& aLastRow,
                          bool aIsHorizontal);

private:

  nsMargin GetBoxTotalMargin(nsIFrame* aBox, bool aIsHorizontal = true);

  void FreeMap();
  void FindRowsAndColumns(nsIFrame** aRows, nsIFrame** aColumns);
  void BuildRows(nsIFrame* aBox, int32_t aSize, nsGridRow** aColumnsRows, bool aIsHorizontal = true);
  nsGridCell* BuildCellMap(int32_t aRows, int32_t aColumns);
  void PopulateCellMap(nsGridRow* aRows, nsGridRow* aColumns, int32_t aRowCount, int32_t aColumnCount, bool aIsHorizontal = true);
  void CountRowsColumns(nsIFrame* aBox, int32_t& aRowCount, int32_t& aComputedColumnCount);
  void SetLargestSize(nsSize& aSize, nscoord aHeight, bool aIsHorizontal = true);
  void SetSmallestSize(nsSize& aSize, nscoord aHeight, bool aIsHorizontal = true);
  bool IsGrid(nsIFrame* aBox);

  
  nsIFrame* mBox;

  
  nsGridRow* mRows;

  
  nsGridRow* mColumns;

  
  nsIFrame* mRowsBox;

  
  nsIFrame* mColumnsBox;

  
  bool mNeedsRebuild;

  
  int32_t mRowCount;
  int32_t mColumnCount;

  
  
  int32_t mExtraRowCount;
  int32_t mExtraColumnCount;

  
  nsGridCell* mCellMap;

  
  
  bool mMarkingDirty;
};

#endif

