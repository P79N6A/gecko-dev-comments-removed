











#include "nsGridLayout2.h"
#include "nsGridRowGroupLayout.h"
#include "nsGridRow.h"
#include "nsBox.h"
#include "nsIScrollableFrame.h"
#include "nsSprocketLayout.h"
#include "nsHTMLReflowState.h"

nsresult
NS_NewGridLayout2( nsIPresShell* aPresShell, nsBoxLayout** aNewLayout)
{
  *aNewLayout = new nsGridLayout2(aPresShell);
  NS_IF_ADDREF(*aNewLayout);

  return NS_OK;
  
} 

nsGridLayout2::nsGridLayout2(nsIPresShell* aPresShell):nsStackLayout()
{
}


void
nsGridLayout2::AddOffset(nsBoxLayoutState& aState, nsIFrame* aChild, nsSize& aSize)
{
  nsMargin offset;
  GetOffset(aState, aChild, offset);
  aSize.width += offset.left;
  aSize.height += offset.top;
}

NS_IMETHODIMP
nsGridLayout2::Layout(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState)
{
  
  mGrid.SetBox(aBox);
  NS_ASSERTION(aBox->GetLayoutManager() == this, "setting incorrect box");

  nsresult rv = nsStackLayout::Layout(aBox, aBoxLayoutState);
#ifdef DEBUG_grid
  mGrid.PrintCellMap();
#endif
  return rv;
}

void
nsGridLayout2::IntrinsicWidthsDirty(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState)
{
  nsStackLayout::IntrinsicWidthsDirty(aBox, aBoxLayoutState);
  
  
  
  mGrid.NeedsRebuild(aBoxLayoutState);
}

nsGrid*
nsGridLayout2::GetGrid(nsIFrame* aBox, int32_t* aIndex, nsGridRowLayout* aRequestor)
{
  
  mGrid.SetBox(aBox);
  NS_ASSERTION(aBox->GetLayoutManager() == this, "setting incorrect box");
  return &mGrid;
}

void
nsGridLayout2::AddWidth(nsSize& aSize, nscoord aSize2, bool aIsHorizontal)
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
nsGridLayout2::GetMinSize(nsIFrame* aBox, nsBoxLayoutState& aState)
{
  nsSize minSize = nsStackLayout::GetMinSize(aBox, aState); 

  
  
  nsSize total(0,0);
  nsIFrame* rowsBox = mGrid.GetRowsBox();
  nsIFrame* columnsBox = mGrid.GetColumnsBox();
  if (!rowsBox || !columnsBox) {
    if (!rowsBox) {
      
      int32_t rows = mGrid.GetRowCount();
      for (int32_t i=0; i < rows; i++)
      {
        nscoord height = mGrid.GetMinRowHeight(aState, i, true); 
        AddWidth(total, height, false); 
      }
    }

    if (!columnsBox) {
      
      int32_t columns = mGrid.GetColumnCount();
      for (int32_t i=0; i < columns; i++)
      {
        nscoord width = mGrid.GetMinRowHeight(aState, i, false); 
        AddWidth(total, width, true); 
      }
    }

    AddMargin(aBox, total);
    AddOffset(aState, aBox, total);
    AddLargestSize(minSize, total);
  }
  
  return minSize;
}

nsSize
nsGridLayout2::GetPrefSize(nsIFrame* aBox, nsBoxLayoutState& aState)
{
  nsSize pref = nsStackLayout::GetPrefSize(aBox, aState); 

  
  
  nsSize total(0,0);
  nsIFrame* rowsBox = mGrid.GetRowsBox();
  nsIFrame* columnsBox = mGrid.GetColumnsBox();
  if (!rowsBox || !columnsBox) {
    if (!rowsBox) {
      
      int32_t rows = mGrid.GetRowCount();
      for (int32_t i=0; i < rows; i++)
      {
        nscoord height = mGrid.GetPrefRowHeight(aState, i, true); 
        AddWidth(total, height, false); 
      }
    }

    if (!columnsBox) {
      
      int32_t columns = mGrid.GetColumnCount();
      for (int32_t i=0; i < columns; i++)
      {
        nscoord width = mGrid.GetPrefRowHeight(aState, i, false);
        AddWidth(total, width, true); 
      }
    }

    AddMargin(aBox, total);
    AddOffset(aState, aBox, total);
    AddLargestSize(pref, total);
  }

  return pref;
}

nsSize
nsGridLayout2::GetMaxSize(nsIFrame* aBox, nsBoxLayoutState& aState)
{
  nsSize maxSize = nsStackLayout::GetMaxSize(aBox, aState); 

  
  
  nsSize total(NS_INTRINSICSIZE, NS_INTRINSICSIZE);
  nsIFrame* rowsBox = mGrid.GetRowsBox();
  nsIFrame* columnsBox = mGrid.GetColumnsBox();
  if (!rowsBox || !columnsBox) {
    if (!rowsBox) {
      total.height = 0;
      
      int32_t rows = mGrid.GetRowCount();
      for (int32_t i=0; i < rows; i++)
      {
        nscoord height = mGrid.GetMaxRowHeight(aState, i, true); 
        AddWidth(total, height, false); 
      }
    }

    if (!columnsBox) {
      total.width = 0;
      
      int32_t columns = mGrid.GetColumnCount();
      for (int32_t i=0; i < columns; i++)
      {
        nscoord width = mGrid.GetMaxRowHeight(aState, i, false);
        AddWidth(total, width, true); 
      }
    }

    AddMargin(aBox, total);
    AddOffset(aState, aBox, total);
    AddSmallestSize(maxSize, total);
  }

  return maxSize;
}

int32_t
nsGridLayout2::BuildRows(nsIFrame* aBox, nsGridRow* aRows)
{
  if (aBox) {
    aRows[0].Init(aBox, true);
    return 1;
  }
  return 0;
}

nsMargin
nsGridLayout2::GetTotalMargin(nsIFrame* aBox, bool aIsHorizontal)
{
  nsMargin margin(0,0,0,0);
  return margin;
}

void
nsGridLayout2::ChildrenInserted(nsIFrame* aBox, nsBoxLayoutState& aState,
                                nsIFrame* aPrevBox,
                                const nsFrameList::Slice& aNewChildren)
{
  mGrid.NeedsRebuild(aState);
}

void
nsGridLayout2::ChildrenAppended(nsIFrame* aBox, nsBoxLayoutState& aState,
                                const nsFrameList::Slice& aNewChildren)
{
  mGrid.NeedsRebuild(aState);
}

void
nsGridLayout2::ChildrenRemoved(nsIFrame* aBox, nsBoxLayoutState& aState,
                               nsIFrame* aChildList)
{
  mGrid.NeedsRebuild(aState);
}

void
nsGridLayout2::ChildrenSet(nsIFrame* aBox, nsBoxLayoutState& aState,
                           nsIFrame* aChildList)
{
  mGrid.NeedsRebuild(aState);
}

NS_IMPL_ADDREF_INHERITED(nsGridLayout2, nsStackLayout)
NS_IMPL_RELEASE_INHERITED(nsGridLayout2, nsStackLayout)

NS_INTERFACE_MAP_BEGIN(nsGridLayout2)
  NS_INTERFACE_MAP_ENTRY(nsIGridPart)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIGridPart)
NS_INTERFACE_MAP_END_INHERITING(nsStackLayout)
