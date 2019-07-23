











































#include "nsGridRowLeafLayout.h"
#include "nsGridRowGroupLayout.h"
#include "nsGridRow.h"
#include "nsBoxLayoutState.h"
#include "nsBox.h"
#include "nsIScrollableFrame.h"
#include "nsBoxFrame.h"
#include "nsGridLayout2.h"

nsresult
NS_NewGridRowLeafLayout( nsIPresShell* aPresShell, nsIBoxLayout** aNewLayout)
{
  *aNewLayout = new nsGridRowLeafLayout(aPresShell);
  NS_IF_ADDREF(*aNewLayout);

  return NS_OK;
  
} 

nsGridRowLeafLayout::nsGridRowLeafLayout(nsIPresShell* aPresShell):nsGridRowLayout(aPresShell)
{
}

nsGridRowLeafLayout::~nsGridRowLeafLayout()
{
}

NS_IMETHODIMP
nsGridRowLeafLayout::GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aState, nsSize& aSize)
{
  PRInt32 index = 0;
  nsGrid* grid = GetGrid(aBox, &index);
  PRBool isHorizontal = IsHorizontal(aBox);

  
  
  if (!grid)
    return nsGridRowLayout::GetPrefSize(aBox, aState, aSize); 
  else {
    aSize = grid->GetPrefRowSize(aState, index, isHorizontal);
    
    return NS_OK;
  }
}

NS_IMETHODIMP
nsGridRowLeafLayout::GetMinSize(nsIBox* aBox, nsBoxLayoutState& aState, nsSize& aSize)
{
  PRInt32 index = 0;
  nsGrid* grid = GetGrid(aBox, &index);
  PRBool isHorizontal = IsHorizontal(aBox);

  if (!grid)
    return nsGridRowLayout::GetMinSize(aBox, aState, aSize); 
  else {
    aSize = grid->GetMinRowSize(aState, index, isHorizontal);
    AddBorderAndPadding(aBox, aSize);
    return NS_OK;
  }
}

NS_IMETHODIMP
nsGridRowLeafLayout::GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aState, nsSize& aSize)
{
  PRInt32 index = 0;
  nsGrid* grid = GetGrid(aBox, &index);
  PRBool isHorizontal = IsHorizontal(aBox);

  if (!grid)
    return nsGridRowLayout::GetMaxSize(aBox, aState, aSize); 
  else {
    aSize = grid->GetMaxRowSize(aState, index, isHorizontal);
    AddBorderAndPadding(aBox, aSize);
    return NS_OK;
  }
}



void
nsGridRowLeafLayout::ChildAddedOrRemoved(nsIBox* aBox, nsBoxLayoutState& aState)
{
  PRInt32 index = 0;
  nsGrid* grid = GetGrid(aBox, &index);
  PRBool isHorizontal = IsHorizontal(aBox);

  if (grid)
    grid->CellAddedOrRemoved(aState, index, isHorizontal);
}

void
nsGridRowLeafLayout::PopulateBoxSizes(nsIBox* aBox, nsBoxLayoutState& aState, nsBoxSize*& aBoxSizes, nsComputedBoxSize*& aComputedBoxSizes, nscoord& aMinSize, nscoord& aMaxSize, PRInt32& aFlexes)
{
  PRInt32 index = 0;
  nsGrid* grid = GetGrid(aBox, &index);
  PRBool isHorizontal = IsHorizontal(aBox);

  
  
  
  if (grid) {
   nsGridRow* column;
   PRInt32 count = grid->GetColumnCount(isHorizontal); 
   nsBoxSize* start = nsnull;
   nsBoxSize* last = nsnull;
   nsBoxSize* current = nsnull;
   nsIBox* child = aBox->GetChildBox();
   for (int i=0; i < count; i++)
   {
     column = grid->GetColumnAt(i,isHorizontal); 

     
     
     nscoord pref =
       grid->GetPrefRowHeight(aState, i, !isHorizontal); 
     nscoord min = 
       grid->GetMinRowHeight(aState, i, !isHorizontal);  
     nscoord max = 
       grid->GetMaxRowHeight(aState, i, !isHorizontal);  
     nscoord flex =
       grid->GetRowFlex(aState, i, !isHorizontal);       
     nscoord left  = 0;
     nscoord right  = 0;
     grid->GetRowOffsets(aState, i, left, right, !isHorizontal); 
     nsIBox* box = column->GetBox();
     PRBool collapsed = PR_FALSE;
     nscoord topMargin = column->mTopMargin;
     nscoord bottomMargin = column->mBottomMargin;

     if (box) 
       collapsed = box->IsCollapsed(aState);

     pref = pref - (left + right);
     if (pref < 0)
       pref = 0;

     
     
     
     
     
      PRInt32 firstIndex = 0;
      PRInt32 lastIndex = 0;
      nsGridRow* firstRow = nsnull;
      nsGridRow* lastRow = nsnull;
      grid->GetFirstAndLastRow(aState, firstIndex, lastIndex, firstRow, lastRow, !isHorizontal);

      if (i == firstIndex || i == lastIndex) {
        nsMargin offset = GetTotalMargin(aBox, isHorizontal);

        nsMargin border(0,0,0,0);
        
        aBox->GetBorder(border);
        offset += border;
        aBox->GetPadding(border);
        offset += border;

        
        if (i == firstIndex) 
        {
          if (isHorizontal)
           left -= offset.left;
          else
           left -= offset.top;
        }

        if (i == lastIndex)
        {
          if (isHorizontal)
           right -= offset.right;
          else
           right -= offset.bottom;
        }
      }
    
     
     nsBox::BoundsCheck(min, pref, max);
   
     current = new (aState) nsBoxSize();
     current->pref = pref;
     current->min = min;
     current->max = max;
     current->flex = flex;
     current->bogus = column->mIsBogus;
     current->left = left + topMargin;
     current->right = right + bottomMargin;
     current->collapsed = collapsed;

     if (!start) {
        start = current;
        last = start;
     } else {
        last->next = current;
        last = current;
     }

     if (child && !column->mIsBogus)
       child = child->GetNextBox();

   }
   aBoxSizes = start;
  }

  nsSprocketLayout::PopulateBoxSizes(aBox, aState, aBoxSizes, aComputedBoxSizes, aMinSize, aMaxSize, aFlexes);
}

void
nsGridRowLeafLayout::ComputeChildSizes(nsIBox* aBox,
                           nsBoxLayoutState& aState, 
                           nscoord& aGivenSize, 
                           nsBoxSize* aBoxSizes, 
                           nsComputedBoxSize*& aComputedBoxSizes)
{ 
  
  
  if (aBox) {

     
     aBox = aBox->GetParentBox();
     nsIBox* scrollbox = nsGrid::GetScrollBox(aBox);
       
       nsCOMPtr<nsIScrollableFrame> scrollable = do_QueryInterface(scrollbox);
       if (scrollable) {
          nsMargin scrollbarSizes = scrollable->GetActualScrollbarSizes();

          nsRect ourRect(scrollbox->GetRect());
          nsMargin padding(0,0,0,0);
          scrollbox->GetBorderAndPadding(padding);
          ourRect.Deflate(padding);

          nscoord diff;
          if (aBox->IsHorizontal()) {
            diff = scrollbarSizes.left + scrollbarSizes.right;
          } else {
            diff = scrollbarSizes.top + scrollbarSizes.bottom;
          }

          if (diff > 0) {
            aGivenSize += diff;

            nsSprocketLayout::ComputeChildSizes(aBox, aState, aGivenSize, aBoxSizes, aComputedBoxSizes);

            aGivenSize -= diff;

            nsComputedBoxSize* s    = aComputedBoxSizes;
            nsComputedBoxSize* last = aComputedBoxSizes;
            while(s)
            {
              last = s;
              s = s->next;
            }
  
            if (last) 
                last->size -= diff;                         
          }
       }
  }
      
  nsSprocketLayout::ComputeChildSizes(aBox, aState, aGivenSize, aBoxSizes, aComputedBoxSizes);

}

NS_IMETHODIMP
nsGridRowLeafLayout::Layout(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)
{
  return nsGridRowLayout::Layout(aBox, aBoxLayoutState);
}

void
nsGridRowLeafLayout::DirtyRows(nsIBox* aBox, nsBoxLayoutState& aState)
{
  if (aBox) {
    
    
    
    aState.PresShell()->FrameNeedsReflow(aBox, nsIPresShell::eTreeChange,
                                         NS_FRAME_IS_DIRTY);
  }
}

void
nsGridRowLeafLayout::CountRowsColumns(nsIBox* aBox, PRInt32& aRowCount, PRInt32& aComputedColumnCount)
{
  if (aBox) {
    nsIBox* child = aBox->GetChildBox();

    
    PRInt32 columnCount = 0;
    while(child) {
      child = child->GetNextBox();
      columnCount++;
    }

    
    if (columnCount > aComputedColumnCount) 
      aComputedColumnCount = columnCount;

    aRowCount++;
  }
}

PRInt32
nsGridRowLeafLayout::BuildRows(nsIBox* aBox, nsGridRow* aRows)
{ 
  if (aBox) {
      aRows[0].Init(aBox, PR_FALSE);
      return 1;
  }

  return 0;
}

