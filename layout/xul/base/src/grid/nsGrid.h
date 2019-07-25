






































#ifndef nsGrid_h___
#define nsGrid_h___

#include "nsStackLayout.h"
#include "nsIGridPart.h"
#include "nsCOMPtr.h"
#include "nsIFrame.h"

class nsGridRowGroupLayout;
class nsGridRowLayout;
class nsBoxLayoutState;
class nsGridCell;






class nsGrid
{
public:
  nsGrid();
  ~nsGrid();

  nsGridRow* GetColumnAt(PRInt32 aIndex, bool aIsHorizontal = true);
  nsGridRow* GetRowAt(PRInt32 aIndex, bool aIsHorizontal = true);
  nsGridCell* GetCellAt(PRInt32 aX, PRInt32 aY);

  void NeedsRebuild(nsBoxLayoutState& aBoxLayoutState);
  void RebuildIfNeeded();

  
  
  
  
  
  
  
  

  nsSize GetPrefRowSize(nsBoxLayoutState& aBoxLayoutState, PRInt32 aRowIndex, bool aIsHorizontal = true);
  nsSize GetMinRowSize(nsBoxLayoutState& aBoxLayoutState, PRInt32 aRowIndex, bool aIsHorizontal = true);
  nsSize GetMaxRowSize(nsBoxLayoutState& aBoxLayoutState, PRInt32 aRowIndex, bool aIsHorizontal = true);
  nscoord GetRowFlex(nsBoxLayoutState& aBoxLayoutState, PRInt32 aRowIndex, bool aIsHorizontal = true);

  nscoord GetPrefRowHeight(nsBoxLayoutState& aBoxLayoutState, PRInt32 aRowIndex, bool aIsHorizontal = true);
  nscoord GetMinRowHeight(nsBoxLayoutState& aBoxLayoutState, PRInt32 aRowIndex, bool aIsHorizontal = true);
  nscoord GetMaxRowHeight(nsBoxLayoutState& aBoxLayoutState, PRInt32 aRowIndex, bool aIsHorizontal = true);
  void GetRowOffsets(nsBoxLayoutState& aState, PRInt32 aIndex, nscoord& aTop, nscoord& aBottom, bool aIsHorizontal = true);

  void RowAddedOrRemoved(nsBoxLayoutState& aBoxLayoutState, PRInt32 aIndex, bool aIsHorizontal = true);
  void CellAddedOrRemoved(nsBoxLayoutState& aBoxLayoutState, PRInt32 aIndex, bool aIsHorizontal = true);
  void DirtyRows(nsIBox* aRowBox, nsBoxLayoutState& aState);
#ifdef DEBUG_grid
  void PrintCellMap();
#endif
  PRInt32 GetExtraColumnCount(bool aIsHorizontal = true);
  PRInt32 GetExtraRowCount(bool aIsHorizontal = true);


  void SetBox(nsIBox* aBox) { mBox = aBox; }
  nsIBox* GetBox() { return mBox; }
  nsIBox* GetRowsBox() { return mRowsBox; }
  nsIBox* GetColumnsBox() { return mColumnsBox; }
  PRInt32 GetRowCount(PRInt32 aIsHorizontal = PR_TRUE);
  PRInt32 GetColumnCount(PRInt32 aIsHorizontal = PR_TRUE);

  static nsIBox* GetScrolledBox(nsIBox* aChild);
  static nsIBox* GetScrollBox(nsIBox* aChild);
  static nsIGridPart* GetPartFromBox(nsIBox* aBox);
  void GetFirstAndLastRow(nsBoxLayoutState& aState, 
                          PRInt32& aFirstIndex, 
                          PRInt32& aLastIndex, 
                          nsGridRow*& aFirstRow,
                          nsGridRow*& aLastRow,
                          bool aIsHorizontal);

private:

  nsMargin GetBoxTotalMargin(nsIBox* aBox, bool aIsHorizontal = true);

  void FreeMap();
  void FindRowsAndColumns(nsIBox** aRows, nsIBox** aColumns);
  void BuildRows(nsIBox* aBox, PRInt32 aSize, nsGridRow** aColumnsRows, bool aIsHorizontal = true);
  nsGridCell* BuildCellMap(PRInt32 aRows, PRInt32 aColumns);
  void PopulateCellMap(nsGridRow* aRows, nsGridRow* aColumns, PRInt32 aRowCount, PRInt32 aColumnCount, bool aIsHorizontal = true);
  void CountRowsColumns(nsIBox* aBox, PRInt32& aRowCount, PRInt32& aComputedColumnCount);
  void SetLargestSize(nsSize& aSize, nscoord aHeight, bool aIsHorizontal = true);
  void SetSmallestSize(nsSize& aSize, nscoord aHeight, bool aIsHorizontal = true);
  bool IsGrid(nsIBox* aBox);

  
  nsIBox* mBox;

  
  nsGridRow* mRows;

  
  nsGridRow* mColumns;

  
  nsIBox* mRowsBox;

  
  nsIBox* mColumnsBox;

  
  bool mNeedsRebuild;

  
  PRInt32 mRowCount;
  PRInt32 mColumnCount;

  
  
  PRInt32 mExtraRowCount;
  PRInt32 mExtraColumnCount;

  
  nsGridCell* mCellMap;

  
  
  bool mMarkingDirty;
};

#endif

