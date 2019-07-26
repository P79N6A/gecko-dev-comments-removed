
















#include "nsGridRowGroupLayout.h"
#include "nsCOMPtr.h"
#include "nsIScrollableFrame.h"
#include "nsBox.h"
#include "nsBoxLayoutState.h"
#include "nsGridLayout2.h"
#include "nsGridRow.h"
#include "nsHTMLReflowState.h"

already_AddRefed<nsBoxLayout> NS_NewGridRowGroupLayout()
{
  nsRefPtr<nsBoxLayout> layout = new nsGridRowGroupLayout();
  return layout.forget();
} 

nsGridRowGroupLayout::nsGridRowGroupLayout():nsGridRowLayout(), mRowCount(0)
{
}

nsGridRowGroupLayout::~nsGridRowGroupLayout()
{
}

void
nsGridRowGroupLayout::ChildAddedOrRemoved(nsIFrame* aBox, nsBoxLayoutState& aState)
{
  int32_t index = 0;
  nsGrid* grid = GetGrid(aBox, &index);
  bool isHorizontal = IsHorizontal(aBox);

  if (grid)
    grid->RowAddedOrRemoved(aState, index, isHorizontal);
}

void
nsGridRowGroupLayout::AddWidth(nsSize& aSize, nscoord aSize2, bool aIsHorizontal)
{
  nscoord& size = GET_WIDTH(aSize, aIsHorizontal);

  if (size == NS_INTRINSICSIZE || aSize2 == NS_INTRINSICSIZE)
    size = NS_INTRINSICSIZE;
  else
    size += aSize2;
}

nsSize
nsGridRowGroupLayout::GetPrefSize(nsIFrame* aBox, nsBoxLayoutState& aState)
{ 
  nsSize vpref = nsGridRowLayout::GetPrefSize(aBox, aState); 


 






  int32_t index = 0;
  nsGrid* grid = GetGrid(aBox, &index);

  if (grid) 
  {
    
    bool isHorizontal = IsHorizontal(aBox);
    int32_t extraColumns = grid->GetExtraColumnCount(isHorizontal);
    int32_t start = grid->GetColumnCount(isHorizontal) - grid->GetExtraColumnCount(isHorizontal);
    for (int32_t i=0; i < extraColumns; i++)
    {
      nscoord pref =
        grid->GetPrefRowHeight(aState, i+start, !isHorizontal); 

      AddWidth(vpref, pref, isHorizontal);
    }
  }

  return vpref;
}

nsSize
nsGridRowGroupLayout::GetMaxSize(nsIFrame* aBox, nsBoxLayoutState& aState)
{
 nsSize maxSize = nsGridRowLayout::GetMaxSize(aBox, aState); 

  int32_t index = 0;
  nsGrid* grid = GetGrid(aBox, &index);

  if (grid) 
  {
    
    bool isHorizontal = IsHorizontal(aBox);
    int32_t extraColumns = grid->GetExtraColumnCount(isHorizontal);
    int32_t start = grid->GetColumnCount(isHorizontal) - grid->GetExtraColumnCount(isHorizontal);
    for (int32_t i=0; i < extraColumns; i++)
    {
      nscoord max =
        grid->GetMaxRowHeight(aState, i+start, !isHorizontal); 

      AddWidth(maxSize, max, isHorizontal);
    }
  }

  return maxSize;
}

nsSize
nsGridRowGroupLayout::GetMinSize(nsIFrame* aBox, nsBoxLayoutState& aState)
{
  nsSize minSize = nsGridRowLayout::GetMinSize(aBox, aState); 

  int32_t index = 0;
  nsGrid* grid = GetGrid(aBox, &index);

  if (grid) 
  {
    
    bool isHorizontal = IsHorizontal(aBox);
    int32_t extraColumns = grid->GetExtraColumnCount(isHorizontal);
    int32_t start = grid->GetColumnCount(isHorizontal) - grid->GetExtraColumnCount(isHorizontal);
    for (int32_t i=0; i < extraColumns; i++)
    {
      nscoord min = 
        grid->GetMinRowHeight(aState, i+start, !isHorizontal); 
      AddWidth(minSize, min, isHorizontal);
    }
  }

  return minSize;
}




void
nsGridRowGroupLayout::DirtyRows(nsIFrame* aBox, nsBoxLayoutState& aState)
{
  if (aBox) {
    
    
    
    aState.PresShell()->FrameNeedsReflow(aBox, nsIPresShell::eTreeChange,
                                         NS_FRAME_IS_DIRTY);
    nsIFrame* child = nsBox::GetChildBox(aBox);

    while(child) {

      
      nsIFrame* deepChild = nsGrid::GetScrolledBox(child);

      
      nsIGridPart* monument = nsGrid::GetPartFromBox(deepChild);
      if (monument) 
        monument->DirtyRows(deepChild, aState);

      child = nsBox::GetNextBox(child);
    }
  }
}


void
nsGridRowGroupLayout::CountRowsColumns(nsIFrame* aBox, int32_t& aRowCount, int32_t& aComputedColumnCount)
{
  if (aBox) {
    int32_t startCount = aRowCount;

    nsIFrame* child = nsBox::GetChildBox(aBox);

    while(child) {
      
      
      nsIFrame* deepChild = nsGrid::GetScrolledBox(child);

      nsIGridPart* monument = nsGrid::GetPartFromBox(deepChild);
      if (monument) {
        monument->CountRowsColumns(deepChild, aRowCount, aComputedColumnCount);
        child = nsBox::GetNextBox(child);
        deepChild = child;
        continue;
      }

      child = nsBox::GetNextBox(child);

      
      aRowCount++;
    }

    mRowCount = aRowCount - startCount;
  }
}





int32_t 
nsGridRowGroupLayout::BuildRows(nsIFrame* aBox, nsGridRow* aRows)
{ 
  int32_t rowCount = 0;

  if (aBox) {
    nsIFrame* child = nsBox::GetChildBox(aBox);

    while(child) {
      
      
      nsIFrame* deepChild = nsGrid::GetScrolledBox(child);

      nsIGridPart* monument = nsGrid::GetPartFromBox(deepChild);
      if (monument) {
        rowCount += monument->BuildRows(deepChild, &aRows[rowCount]);
        child = nsBox::GetNextBox(child);
        deepChild = child;
        continue;
      }

      aRows[rowCount].Init(child, true);

      child = nsBox::GetNextBox(child);

      
      rowCount++;
    }
  }

  return rowCount;
}

nsMargin
nsGridRowGroupLayout::GetTotalMargin(nsIFrame* aBox, bool aIsHorizontal)
{
  

  nsMargin margin = nsGridRowLayout::GetTotalMargin(aBox, aIsHorizontal);
  
  
  
  aBox = nsGrid::GetScrollBox(aBox);

  
  nsMargin borderPadding(0,0,0,0);
  aBox->GetBorderAndPadding(borderPadding);
  margin += borderPadding;

  return margin;
}


