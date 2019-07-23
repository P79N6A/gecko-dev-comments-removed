











































#include "nsGridLayout2.h"
#include "nsGridRowGroupLayout.h"
#include "nsGridRow.h"
#include "nsBox.h"
#include "nsIScrollableFrame.h"
#include "nsSprocketLayout.h"

nsresult
NS_NewGridLayout2( nsIPresShell* aPresShell, nsIBoxLayout** aNewLayout)
{
  *aNewLayout = new nsGridLayout2(aPresShell);
  NS_IF_ADDREF(*aNewLayout);

  return NS_OK;
  
} 

nsGridLayout2::nsGridLayout2(nsIPresShell* aPresShell):nsStackLayout()
{
}


void
nsGridLayout2::AddOffset(nsBoxLayoutState& aState, nsIBox* aChild, nsSize& aSize)
{
  nsMargin offset;
  GetOffset(aState, aChild, offset);
  aSize.width += offset.left;
  aSize.height += offset.top;
}

NS_IMETHODIMP
nsGridLayout2::Layout(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)
{
  
  mGrid.SetBox(aBox);
#ifdef DEBUG
  {
    nsCOMPtr<nsIBoxLayout> lm;
    aBox->GetLayoutManager(getter_AddRefs(lm));
    NS_ASSERTION(lm == this, "setting incorrect box");
  }
#endif

  nsresult rv = nsStackLayout::Layout(aBox, aBoxLayoutState);
#ifdef DEBUG_grid
  mGrid.PrintCellMap();
#endif
  return rv;
}

void
nsGridLayout2::IntrinsicWidthsDirty(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)
{
  nsStackLayout::IntrinsicWidthsDirty(aBox, aBoxLayoutState);
  
  
  
  mGrid.NeedsRebuild(aBoxLayoutState);
}

nsGrid*
nsGridLayout2::GetGrid(nsIBox* aBox, PRInt32* aIndex, nsGridRowLayout* aRequestor)
{
  
  mGrid.SetBox(aBox);
#ifdef DEBUG
  {
    nsCOMPtr<nsIBoxLayout> lm;
    aBox->GetLayoutManager(getter_AddRefs(lm));
    NS_ASSERTION(lm == this, "setting incorrect box");
  }
#endif

  return &mGrid;
}

void
nsGridLayout2::AddWidth(nsSize& aSize, nscoord aSize2, PRBool aIsHorizontal)
{
  nscoord& size = GET_WIDTH(aSize, aIsHorizontal);

  if (size != NS_INTRINSICSIZE) {
    if (aSize2 == NS_INTRINSICSIZE)
      size = NS_INTRINSICSIZE;
    else
      size += aSize2;
  }
}

nsSize
nsGridLayout2::GetMinSize(nsIBox* aBox, nsBoxLayoutState& aState)
{
  nsSize minSize = nsStackLayout::GetMinSize(aBox, aState); 

  
  
  nsSize total(0,0);
  nsIBox* rowsBox = mGrid.GetRowsBox();
  nsIBox* columnsBox = mGrid.GetColumnsBox();
  if (!rowsBox || !columnsBox) {
    if (!rowsBox) {
      
      PRInt32 rows = mGrid.GetRowCount();
      for (PRInt32 i=0; i < rows; i++)
      {
        nscoord height = mGrid.GetMinRowHeight(aState, i, PR_TRUE); 
        AddWidth(total, height, PR_FALSE); 
      }
    }

    if (!columnsBox) {
      
      PRInt32 columns = mGrid.GetColumnCount();
      for (PRInt32 i=0; i < columns; i++)
      {
        nscoord width = mGrid.GetMinRowHeight(aState, i, PR_FALSE); 
        AddWidth(total, width, PR_TRUE); 
      }
    }

    AddMargin(aBox, total);
    AddOffset(aState, aBox, total);
    AddLargestSize(minSize, total);
  }
  
  return minSize;
}

nsSize
nsGridLayout2::GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aState)
{
  nsSize pref = nsStackLayout::GetPrefSize(aBox, aState); 

  
  
  nsSize total(0,0);
  nsIBox* rowsBox = mGrid.GetRowsBox();
  nsIBox* columnsBox = mGrid.GetColumnsBox();
  if (!rowsBox || !columnsBox) {
    if (!rowsBox) {
      
      PRInt32 rows = mGrid.GetRowCount();
      for (PRInt32 i=0; i < rows; i++)
      {
        nscoord height = mGrid.GetPrefRowHeight(aState, i, PR_TRUE); 
        AddWidth(total, height, PR_FALSE); 
      }
    }

    if (!columnsBox) {
      
      PRInt32 columns = mGrid.GetColumnCount();
      for (PRInt32 i=0; i < columns; i++)
      {
        nscoord width = mGrid.GetPrefRowHeight(aState, i, PR_FALSE);
        AddWidth(total, width, PR_TRUE); 
      }
    }

    AddMargin(aBox, total);
    AddOffset(aState, aBox, total);
    AddLargestSize(pref, total);
  }

  return pref;
}

nsSize
nsGridLayout2::GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aState)
{
  nsSize maxSize = nsStackLayout::GetMaxSize(aBox, aState); 

  
  
  nsSize total(NS_INTRINSICSIZE, NS_INTRINSICSIZE);
  nsIBox* rowsBox = mGrid.GetRowsBox();
  nsIBox* columnsBox = mGrid.GetColumnsBox();
  if (!rowsBox || !columnsBox) {
    if (!rowsBox) {
      total.height = 0;
      
      PRInt32 rows = mGrid.GetRowCount();
      for (PRInt32 i=0; i < rows; i++)
      {
        nscoord height = mGrid.GetMaxRowHeight(aState, i, PR_TRUE); 
        AddWidth(total, height, PR_FALSE); 
      }
    }

    if (!columnsBox) {
      total.width = 0;
      
      PRInt32 columns = mGrid.GetColumnCount();
      for (PRInt32 i=0; i < columns; i++)
      {
        nscoord width = mGrid.GetMaxRowHeight(aState, i, PR_FALSE);
        AddWidth(total, width, PR_TRUE); 
      }
    }

    AddMargin(aBox, total);
    AddOffset(aState, aBox, total);
    AddSmallestSize(maxSize, total);
  }

  return maxSize;
}

PRInt32
nsGridLayout2::BuildRows(nsIBox* aBox, nsGridRow* aRows)
{
  if (aBox) {
    aRows[0].Init(aBox, PR_TRUE);
    return 1;
  }
  return 0;
}

nsMargin
nsGridLayout2::GetTotalMargin(nsIBox* aBox, PRBool aIsHorizontal)
{
  nsMargin margin(0,0,0,0);
  return margin;
}

void
nsGridLayout2::ChildrenInserted(nsIBox* aBox, nsBoxLayoutState& aState,
                                nsIBox* aPrevBox,
                                const nsFrameList::Slice& aNewChildren)
{
  mGrid.NeedsRebuild(aState);
}

void
nsGridLayout2::ChildrenAppended(nsIBox* aBox, nsBoxLayoutState& aState,
                                const nsFrameList::Slice& aNewChildren)
{
  mGrid.NeedsRebuild(aState);
}

void
nsGridLayout2::ChildrenRemoved(nsIBox* aBox, nsBoxLayoutState& aState,
                               nsIBox* aChildList)
{
  mGrid.NeedsRebuild(aState);
}

void
nsGridLayout2::ChildrenSet(nsIBox* aBox, nsBoxLayoutState& aState,
                           nsIBox* aChildList)
{
  mGrid.NeedsRebuild(aState);
}

NS_IMPL_ADDREF_INHERITED(nsGridLayout2, nsStackLayout)
NS_IMPL_RELEASE_INHERITED(nsGridLayout2, nsStackLayout)

NS_INTERFACE_MAP_BEGIN(nsGridLayout2)
  NS_INTERFACE_MAP_ENTRY(nsIGridPart)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIGridPart)
NS_INTERFACE_MAP_END_INHERITING(nsStackLayout)
