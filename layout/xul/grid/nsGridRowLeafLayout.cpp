











#include "nsGridRowLeafLayout.h"
#include "nsGridRowGroupLayout.h"
#include "nsGridRow.h"
#include "nsBoxLayoutState.h"
#include "nsBox.h"
#include "nsIScrollableFrame.h"
#include "nsBoxFrame.h"
#include "nsGridLayout2.h"
#include <algorithm>

already_AddRefed<nsBoxLayout> NS_NewGridRowLeafLayout()
{
  nsRefPtr<nsBoxLayout> layout = new nsGridRowLeafLayout();
  return layout.forget();
} 

nsGridRowLeafLayout::nsGridRowLeafLayout():nsGridRowLayout()
{
}

nsGridRowLeafLayout::~nsGridRowLeafLayout()
{
}

nsSize
nsGridRowLeafLayout::GetPrefSize(nsIFrame* aBox, nsBoxLayoutState& aState)
{
  int32_t index = 0;
  nsGrid* grid = GetGrid(aBox, &index);
  bool isHorizontal = IsHorizontal(aBox);

  
  
  if (!grid) {
    return nsGridRowLayout::GetPrefSize(aBox, aState); 
  }
  else {
    return grid->GetPrefRowSize(aState, index, isHorizontal);
    
  }
}

nsSize
nsGridRowLeafLayout::GetMinSize(nsIFrame* aBox, nsBoxLayoutState& aState)
{
  int32_t index = 0;
  nsGrid* grid = GetGrid(aBox, &index);
  bool isHorizontal = IsHorizontal(aBox);

  if (!grid)
    return nsGridRowLayout::GetMinSize(aBox, aState); 
  else {
    nsSize minSize = grid->GetMinRowSize(aState, index, isHorizontal);
    AddBorderAndPadding(aBox, minSize);
    return minSize;
  }
}

nsSize
nsGridRowLeafLayout::GetMaxSize(nsIFrame* aBox, nsBoxLayoutState& aState)
{
  int32_t index = 0;
  nsGrid* grid = GetGrid(aBox, &index);
  bool isHorizontal = IsHorizontal(aBox);

  if (!grid)
    return nsGridRowLayout::GetMaxSize(aBox, aState); 
  else {
    nsSize maxSize;
    maxSize = grid->GetMaxRowSize(aState, index, isHorizontal);
    AddBorderAndPadding(aBox, maxSize);
    return maxSize;
  }
}



void
nsGridRowLeafLayout::ChildAddedOrRemoved(nsIFrame* aBox, nsBoxLayoutState& aState)
{
  int32_t index = 0;
  nsGrid* grid = GetGrid(aBox, &index);
  bool isHorizontal = IsHorizontal(aBox);

  if (grid)
    grid->CellAddedOrRemoved(aState, index, isHorizontal);
}

void
nsGridRowLeafLayout::PopulateBoxSizes(nsIFrame* aBox, nsBoxLayoutState& aState, nsBoxSize*& aBoxSizes, nscoord& aMinSize, nscoord& aMaxSize, int32_t& aFlexes)
{
  int32_t index = 0;
  nsGrid* grid = GetGrid(aBox, &index);
  bool isHorizontal = IsHorizontal(aBox);

  
  
  
  if (grid) {
    nsGridRow* column;
    int32_t count = grid->GetColumnCount(isHorizontal); 
    nsBoxSize* start = nullptr;
    nsBoxSize* last = nullptr;
    nsBoxSize* current = nullptr;
    nsIFrame* child = nsBox::GetChildBox(aBox);
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
      nsIFrame* box = column->GetBox();
      bool collapsed = false;
      nscoord topMargin = column->mTopMargin;
      nscoord bottomMargin = column->mBottomMargin;

      if (box) 
        collapsed = box->IsCollapsed();

      pref = pref - (left + right);
      if (pref < 0)
        pref = 0;

      
      
      
      
      
      int32_t firstIndex = 0;
      int32_t lastIndex = 0;
      nsGridRow* firstRow = nullptr;
      nsGridRow* lastRow = nullptr;
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
    
      
      max = std::max(min, max);
      pref = nsBox::BoundsCheck(min, pref, max);
   
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
        child = nsBox::GetNextBox(child);

    }
    aBoxSizes = start;
  }

  nsSprocketLayout::PopulateBoxSizes(aBox, aState, aBoxSizes, aMinSize, aMaxSize, aFlexes);
}

void
nsGridRowLeafLayout::ComputeChildSizes(nsIFrame* aBox,
                           nsBoxLayoutState& aState, 
                           nscoord& aGivenSize, 
                           nsBoxSize* aBoxSizes, 
                           nsComputedBoxSize*& aComputedBoxSizes)
{ 
  
  
  if (aBox) {
    bool isHorizontal = aBox->IsHorizontal();

    
    nscoord diff = 0;
    nsIFrame* parentBox;
    (void)GetParentGridPart(aBox, &parentBox);
    while (parentBox) {
      nsIFrame* scrollbox = nsGrid::GetScrollBox(parentBox);
      nsIScrollableFrame *scrollable = do_QueryFrame(scrollbox);
      if (scrollable) {
        
        
        
        nsMargin scrollbarSizes = scrollable->GetDesiredScrollbarSizes(&aState);
        uint32_t visible = scrollable->GetScrollbarVisibility();

        if (isHorizontal && (visible & nsIScrollableFrame::VERTICAL)) {
          diff += scrollbarSizes.left + scrollbarSizes.right;
        } else if (!isHorizontal && (visible & nsIScrollableFrame::HORIZONTAL)) {
          diff += scrollbarSizes.top + scrollbarSizes.bottom;
        }
      }

      (void)GetParentGridPart(parentBox, &parentBox);
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

      return;
    }
  }
      
  nsSprocketLayout::ComputeChildSizes(aBox, aState, aGivenSize, aBoxSizes, aComputedBoxSizes);

}

NS_IMETHODIMP
nsGridRowLeafLayout::Layout(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState)
{
  return nsGridRowLayout::Layout(aBox, aBoxLayoutState);
}

void
nsGridRowLeafLayout::DirtyRows(nsIFrame* aBox, nsBoxLayoutState& aState)
{
  if (aBox) {
    
    
    
    aState.PresShell()->FrameNeedsReflow(aBox, nsIPresShell::eTreeChange,
                                         NS_FRAME_IS_DIRTY);
  }
}

void
nsGridRowLeafLayout::CountRowsColumns(nsIFrame* aBox, int32_t& aRowCount, int32_t& aComputedColumnCount)
{
  if (aBox) {
    nsIFrame* child = nsBox::GetChildBox(aBox);

    
    int32_t columnCount = 0;
    while(child) {
      child = nsBox::GetNextBox(child);
      columnCount++;
    }

    
    if (columnCount > aComputedColumnCount) 
      aComputedColumnCount = columnCount;

    aRowCount++;
  }
}

int32_t
nsGridRowLeafLayout::BuildRows(nsIFrame* aBox, nsGridRow* aRows)
{
  if (aBox) {
      aRows[0].Init(aBox, false);
      return 1;
  }

  return 0;
}

