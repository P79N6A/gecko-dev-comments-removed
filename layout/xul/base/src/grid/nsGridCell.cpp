











































#include "nsGridCell.h"
#include "nsFrame.h"
#include "nsBox.h"
#include "nsStackLayout.h"


nsGridCell::nsGridCell():mBoxInColumn(nsnull),mBoxInRow(nsnull)
{
    MOZ_COUNT_CTOR(nsGridCell);
}                                               
        
nsGridCell::~nsGridCell()
{
    MOZ_COUNT_DTOR(nsGridCell);
}

nsSize
nsGridCell::GetPrefSize(nsBoxLayoutState& aState)
{
  nsSize sum(0,0);

  
  
  

  if (mBoxInColumn) {
    nsSize pref = mBoxInColumn->GetPrefSize(aState);

    nsBox::AddMargin(mBoxInColumn, pref);
    nsStackLayout::AddOffset(aState, mBoxInColumn, pref);

    nsBoxLayout::AddLargestSize(sum, pref);
  }

  if (mBoxInRow) {
    nsSize pref = mBoxInRow->GetPrefSize(aState);

    nsBox::AddMargin(mBoxInRow, pref);
    nsStackLayout::AddOffset(aState, mBoxInRow, pref);

    nsBoxLayout::AddLargestSize(sum, pref);
  }

  return sum;
}

nsSize
nsGridCell::GetMinSize(nsBoxLayoutState& aState)
{
  nsSize sum(0, 0);

  
  
  

  if (mBoxInColumn) {
    nsSize min = mBoxInColumn->GetMinSize(aState);

    nsBox::AddMargin(mBoxInColumn, min);
    nsStackLayout::AddOffset(aState, mBoxInColumn, min);

    nsBoxLayout::AddLargestSize(sum, min);
  }

  if (mBoxInRow) {
    nsSize min = mBoxInRow->GetMinSize(aState);

    nsBox::AddMargin(mBoxInRow, min);
    nsStackLayout::AddOffset(aState, mBoxInRow, min);

    nsBoxLayout::AddLargestSize(sum, min);
  }

  return sum;
}

nsSize
nsGridCell::GetMaxSize(nsBoxLayoutState& aState)
{
  nsSize sum(NS_INTRINSICSIZE, NS_INTRINSICSIZE);

  
  
  

  if (mBoxInColumn) {
    nsSize max = mBoxInColumn->GetMaxSize(aState);
 
    nsBox::AddMargin(mBoxInColumn, max);
    nsStackLayout::AddOffset(aState, mBoxInColumn, max);

    nsBoxLayout::AddSmallestSize(sum, max);
  }

  if (mBoxInRow) {
    nsSize max = mBoxInRow->GetMaxSize(aState);

    nsBox::AddMargin(mBoxInRow, max);
    nsStackLayout::AddOffset(aState, mBoxInRow, max);

    nsBoxLayout::AddSmallestSize(sum, max);
  }

  return sum;
}


PRBool
nsGridCell::IsCollapsed(nsBoxLayoutState& aState)
{
  return ((mBoxInColumn && mBoxInColumn->IsCollapsed(aState)) ||
          (mBoxInRow && mBoxInRow->IsCollapsed(aState)));
}


