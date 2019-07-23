






































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

  nsGridRow* GetColumnAt(PRInt32 aIndex, PRBool aIsHorizontal = PR_TRUE);
  nsGridRow* GetRowAt(PRInt32 aIndex, PRBool aIsHorizontal = PR_TRUE);
  nsGridCell* GetCellAt(PRInt32 aX, PRInt32 aY);

  void NeedsRebuild(nsBoxLayoutState& aBoxLayoutState);
  void RebuildIfNeeded();

  nsSize GetPrefRowSize(nsBoxLayoutState& aBoxLayoutState, PRInt32 aRowIndex, PRBool aIsHorizontal = PR_TRUE);
  nsSize GetMinRowSize(nsBoxLayoutState& aBoxLayoutState, PRInt32 aRowIndex, PRBool aIsHorizontal = PR_TRUE);
  nsSize GetMaxRowSize(nsBoxLayoutState& aBoxLayoutState, PRInt32 aRowIndex, PRBool aIsHorizontal = PR_TRUE);
  nscoord GetRowFlex(nsBoxLayoutState& aBoxLayoutState, PRInt32 aRowIndex, PRBool aIsHorizontal = PR_TRUE);

  nscoord GetPrefRowHeight(nsBoxLayoutState& aBoxLayoutState, PRInt32 aRowIndex, PRBool aIsHorizontal = PR_TRUE);
  nscoord GetMinRowHeight(nsBoxLayoutState& aBoxLayoutState, PRInt32 aRowIndex, PRBool aIsHorizontal = PR_TRUE);
  nscoord GetMaxRowHeight(nsBoxLayoutState& aBoxLayoutState, PRInt32 aRowIndex, PRBool aIsHorizontal = PR_TRUE);
  void GetRowOffsets(nsBoxLayoutState& aState, PRInt32 aIndex, nscoord& aTop, nscoord& aBottom, PRBool aIsHorizontal = PR_TRUE);

  void RowAddedOrRemoved(nsBoxLayoutState& aBoxLayoutState, PRInt32 aIndex, PRBool aIsHorizontal = PR_TRUE);
  void CellAddedOrRemoved(nsBoxLayoutState& aBoxLayoutState, PRInt32 aIndex, PRBool aIsHorizontal = PR_TRUE);
  void DirtyRows(nsIBox* aRowBox, nsBoxLayoutState& aState);
#ifdef DEBUG_grid
  void PrintCellMap();
#endif
  PRInt32 GetExtraColumnCount(PRBool aIsHorizontal = PR_TRUE);
  PRInt32 GetExtraRowCount(PRBool aIsHorizontal = PR_TRUE);


  void SetBox(nsIBox* aBox) { mBox = aBox; }
  nsIBox* GetBox() { return mBox; }
  nsIBox* GetRowsBox() { return mRowsBox; }
  nsIBox* GetColumnsBox() { return mColumnsBox; }
  nsGridRow* GetColumns();
  nsGridRow* GetRows();
  PRInt32 GetRowCount(PRInt32 aIsHorizontal = PR_TRUE);
  PRInt32 GetColumnCount(PRInt32 aIsHorizontal = PR_TRUE);

  static nsIBox* GetScrolledBox(nsIBox* aChild);
  static nsIBox* GetScrollBox(nsIBox* aChild);
  void GetFirstAndLastRow(nsBoxLayoutState& aState, 
                          PRInt32& aFirstIndex, 
                          PRInt32& aLastIndex, 
                          nsGridRow*& aFirstRow,
                          nsGridRow*& aLastRow,
                          PRBool aIsHorizontal);

private:
  void GetPartFromBox(nsIBox* aBox, nsIGridPart** aPart);
  nsMargin GetBoxTotalMargin(nsIBox* aBox, PRBool aIsHorizontal = PR_TRUE);

  void FreeMap();
  void FindRowsAndColumns(nsIBox** aRows, nsIBox** aColumns);
  void BuildRows(nsIBox* aBox, PRInt32 aSize, nsGridRow** aColumnsRows, PRBool aIsHorizontal = PR_TRUE);
  nsGridCell* BuildCellMap(PRInt32 aRows, PRInt32 aColumns);
  void PopulateCellMap(nsGridRow* aRows, nsGridRow* aColumns, PRInt32 aRowCount, PRInt32 aColumnCount, PRBool aIsHorizontal = PR_TRUE);
  void CountRowsColumns(nsIBox* aBox, PRInt32& aRowCount, PRInt32& aComputedColumnCount);
  void SetLargestSize(nsSize& aSize, nscoord aHeight, PRBool aIsHorizontal = PR_TRUE);
  void SetSmallestSize(nsSize& aSize, nscoord aHeight, PRBool aIsHorizontal = PR_TRUE);
  PRBool IsGrid(nsIBox* aBox);

  
  nsIBox* mBox;

  
  nsGridRow* mRows;

  
  nsGridRow* mColumns;

  
  nsIBox* mRowsBox;

  
  nsIBox* mColumnsBox;

  
  PRBool mNeedsRebuild;

  
  PRInt32 mRowCount;
  PRInt32 mColumnCount;

  
  
  PRInt32 mExtraRowCount;
  PRInt32 mExtraColumnCount;

  
  nsGridCell* mCellMap;

  
  
  PRBool mMarkingDirty;
};

#endif

