
















































#include "nsGridRowGroupLayout.h"
#include "nsCOMPtr.h"
#include "nsIScrollableFrame.h"
#include "nsBoxLayoutState.h"
#include "nsGridLayout2.h"
#include "nsGridRow.h"

nsresult
NS_NewGridRowGroupLayout( nsIPresShell* aPresShell, nsIBoxLayout** aNewLayout)
{
  *aNewLayout = new nsGridRowGroupLayout(aPresShell);
  NS_IF_ADDREF(*aNewLayout);

  return NS_OK;
  
} 

nsGridRowGroupLayout::nsGridRowGroupLayout(nsIPresShell* aPresShell):nsGridRowLayout(aPresShell), mRowCount(0)
{
}

nsGridRowGroupLayout::~nsGridRowGroupLayout()
{
}

void
nsGridRowGroupLayout::ChildAddedOrRemoved(nsIBox* aBox, nsBoxLayoutState& aState)
{
  PRInt32 index = 0;
  nsGrid* grid = GetGrid(aBox, &index);
  PRBool isHorizontal = IsHorizontal(aBox);

  if (grid)
    grid->RowAddedOrRemoved(aState, index, isHorizontal);
}

void
nsGridRowGroupLayout::AddWidth(nsSize& aSize, nscoord aSize2, PRBool aIsHorizontal)
{
  nscoord& size = GET_WIDTH(aSize, aIsHorizontal);

  if (size == NS_INTRINSICSIZE || aSize2 == NS_INTRINSICSIZE)
    size = NS_INTRINSICSIZE;
  else
    size += aSize2;
}

NS_IMETHODIMP
nsGridRowGroupLayout::GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aState, nsSize& aSize)
{ 
  nsresult rv = nsGridRowLayout::GetPrefSize(aBox, aState, aSize); 


 






  PRInt32 index = 0;
  nsGrid* grid = GetGrid(aBox, &index);

  if (grid) 
  {
    
    PRBool isHorizontal = IsHorizontal(aBox);
    PRInt32 extraColumns = grid->GetExtraColumnCount(isHorizontal);
    PRInt32 start = grid->GetColumnCount(isHorizontal) - grid->GetExtraColumnCount(isHorizontal);
    for (PRInt32 i=0; i < extraColumns; i++)
    {
      nscoord pref =
        grid->GetPrefRowHeight(aState, i+start, !isHorizontal); 

      AddWidth(aSize, pref, isHorizontal);
    }
  }

  return rv;
}

NS_IMETHODIMP
nsGridRowGroupLayout::GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aState, nsSize& aSize)
{
 nsresult rv = nsGridRowLayout::GetMaxSize(aBox, aState, aSize); 

  PRInt32 index = 0;
  nsGrid* grid = GetGrid(aBox, &index);

  if (grid) 
  {
    
    PRBool isHorizontal = IsHorizontal(aBox);
    PRInt32 extraColumns = grid->GetExtraColumnCount(isHorizontal);
    PRInt32 start = grid->GetColumnCount(isHorizontal) - grid->GetExtraColumnCount(isHorizontal);
    for (PRInt32 i=0; i < extraColumns; i++)
    {
      nscoord max =
        grid->GetMaxRowHeight(aState, i+start, !isHorizontal); 

      AddWidth(aSize, max, isHorizontal);
    }
  }

  return rv;
}

NS_IMETHODIMP
nsGridRowGroupLayout::GetMinSize(nsIBox* aBox, nsBoxLayoutState& aState, nsSize& aSize)
{
 nsresult rv = nsGridRowLayout::GetMinSize(aBox, aState, aSize); 

  PRInt32 index = 0;
  nsGrid* grid = GetGrid(aBox, &index);

  if (grid) 
  {
    
    PRBool isHorizontal = IsHorizontal(aBox);
    PRInt32 extraColumns = grid->GetExtraColumnCount(isHorizontal);
    PRInt32 start = grid->GetColumnCount(isHorizontal) - grid->GetExtraColumnCount(isHorizontal);
    for (PRInt32 i=0; i < extraColumns; i++)
    {
      nscoord min = 
        grid->GetMinRowHeight(aState, i+start, !isHorizontal); 
      AddWidth(aSize, min, isHorizontal);
    }
  }

  return rv;
}




void
nsGridRowGroupLayout::DirtyRows(nsIBox* aBox, nsBoxLayoutState& aState)
{
  if (aBox) {
    
    aBox->AddStateBits(NS_FRAME_IS_DIRTY);
    
    
    aState.PresShell()->FrameNeedsReflow(aBox, nsIPresShell::eTreeChange);
    nsIBox* child = aBox->GetChildBox();

    while(child) {

      
      nsIBox* deepChild = nsGrid::GetScrolledBox(child);

      
      nsCOMPtr<nsIBoxLayout> layout;
      deepChild->GetLayoutManager(getter_AddRefs(layout));
      if (layout) {
        nsCOMPtr<nsIGridPart> monument( do_QueryInterface(layout) );
        if (monument) 
          monument->DirtyRows(deepChild, aState);
      }

      child = child->GetNextBox();
    }
  }
}


void
nsGridRowGroupLayout::CountRowsColumns(nsIBox* aBox, PRInt32& aRowCount, PRInt32& aComputedColumnCount)
{
  if (aBox) {
    PRInt32 startCount = aRowCount;

    nsIBox* child = aBox->GetChildBox();

    while(child) {
      
      
      nsIBox* deepChild = nsGrid::GetScrolledBox(child);

      nsCOMPtr<nsIBoxLayout> layout;
      deepChild->GetLayoutManager(getter_AddRefs(layout));
      if (layout) {
        nsCOMPtr<nsIGridPart> monument( do_QueryInterface(layout) );
        if (monument) {
          monument->CountRowsColumns(deepChild, aRowCount, aComputedColumnCount);
          child = child->GetNextBox();
          deepChild = child;
          continue;
        }
      }

      child = child->GetNextBox();

      
      aRowCount++;
    }

    mRowCount = aRowCount - startCount;
  }
}





PRInt32 
nsGridRowGroupLayout::BuildRows(nsIBox* aBox, nsGridRow* aRows)
{ 
  PRInt32 rowCount = 0;

  if (aBox) {
    nsIBox* child = aBox->GetChildBox();

    while(child) {
      
      
      nsIBox* deepChild = nsGrid::GetScrolledBox(child);

      nsCOMPtr<nsIBoxLayout> layout;
      deepChild->GetLayoutManager(getter_AddRefs(layout));
      if (layout) {
        nsCOMPtr<nsIGridPart> monument( do_QueryInterface(layout) );
        if (monument) {
          rowCount += monument->BuildRows(deepChild, &aRows[rowCount]);
          child = child->GetNextBox();
          deepChild = child;
          continue;
        }
      }

      aRows[rowCount].Init(child, PR_TRUE);

      child = child->GetNextBox();

      
      rowCount++;
    }
  }

  return rowCount;
}

nsMargin
nsGridRowGroupLayout::GetTotalMargin(nsIBox* aBox, PRBool aIsHorizontal)
{
  

  nsMargin margin = nsGridRowLayout::GetTotalMargin(aBox, aIsHorizontal);
  
  
  
  aBox = nsGrid::GetScrollBox(aBox);

  
  nsMargin borderPadding(0,0,0,0);
  aBox->GetBorderAndPadding(borderPadding);
  margin += borderPadding;

  return margin;
}


