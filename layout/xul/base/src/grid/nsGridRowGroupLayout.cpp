
















#include "nsGridRowGroupLayout.h"
#include "nsCOMPtr.h"
#include "nsIScrollableFrame.h"
#include "nsBoxLayoutState.h"
#include "nsGridLayout2.h"
#include "nsGridRow.h"
#include "nsHTMLReflowState.h"

already_AddRefed<nsBoxLayout> NS_NewGridRowGroupLayout()
{
  nsBoxLayout* layout = new nsGridRowGroupLayout();
  NS_IF_ADDREF(layout);
  return layout;
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
  PRInt32 index = 0;
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


 






  PRInt32 index = 0;
  nsGrid* grid = GetGrid(aBox, &index);

  if (grid) 
  {
    
    bool isHorizontal = IsHorizontal(aBox);
    PRInt32 extraColumns = grid->GetExtraColumnCount(isHorizontal);
    PRInt32 start = grid->GetColumnCount(isHorizontal) - grid->GetExtraColumnCount(isHorizontal);
    for (PRInt32 i=0; i < extraColumns; i++)
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

  PRInt32 index = 0;
  nsGrid* grid = GetGrid(aBox, &index);

  if (grid) 
  {
    
    bool isHorizontal = IsHorizontal(aBox);
    PRInt32 extraColumns = grid->GetExtraColumnCount(isHorizontal);
    PRInt32 start = grid->GetColumnCount(isHorizontal) - grid->GetExtraColumnCount(isHorizontal);
    for (PRInt32 i=0; i < extraColumns; i++)
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

  PRInt32 index = 0;
  nsGrid* grid = GetGrid(aBox, &index);

  if (grid) 
  {
    
    bool isHorizontal = IsHorizontal(aBox);
    PRInt32 extraColumns = grid->GetExtraColumnCount(isHorizontal);
    PRInt32 start = grid->GetColumnCount(isHorizontal) - grid->GetExtraColumnCount(isHorizontal);
    for (PRInt32 i=0; i < extraColumns; i++)
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
    nsIFrame* child = aBox->GetChildBox();

    while(child) {

      
      nsIFrame* deepChild = nsGrid::GetScrolledBox(child);

      
      nsIGridPart* monument = nsGrid::GetPartFromBox(deepChild);
      if (monument) 
        monument->DirtyRows(deepChild, aState);

      child = child->GetNextBox();
    }
  }
}


void
nsGridRowGroupLayout::CountRowsColumns(nsIFrame* aBox, PRInt32& aRowCount, PRInt32& aComputedColumnCount)
{
  if (aBox) {
    PRInt32 startCount = aRowCount;

    nsIFrame* child = aBox->GetChildBox();

    while(child) {
      
      
      nsIFrame* deepChild = nsGrid::GetScrolledBox(child);

      nsIGridPart* monument = nsGrid::GetPartFromBox(deepChild);
      if (monument) {
        monument->CountRowsColumns(deepChild, aRowCount, aComputedColumnCount);
        child = child->GetNextBox();
        deepChild = child;
        continue;
      }

      child = child->GetNextBox();

      
      aRowCount++;
    }

    mRowCount = aRowCount - startCount;
  }
}





PRInt32 
nsGridRowGroupLayout::BuildRows(nsIFrame* aBox, nsGridRow* aRows)
{ 
  PRInt32 rowCount = 0;

  if (aBox) {
    nsIFrame* child = aBox->GetChildBox();

    while(child) {
      
      
      nsIFrame* deepChild = nsGrid::GetScrolledBox(child);

      nsIGridPart* monument = nsGrid::GetPartFromBox(deepChild);
      if (monument) {
        rowCount += monument->BuildRows(deepChild, &aRows[rowCount]);
        child = child->GetNextBox();
        deepChild = child;
        continue;
      }

      aRows[rowCount].Init(child, true);

      child = child->GetNextBox();

      
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


