











































#include "nsGridRowLayout.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableFrame.h"
#include "nsBox.h"
#include "nsStackLayout.h"
#include "nsGrid.h"

nsGridRowLayout::nsGridRowLayout():nsSprocketLayout()
{
}

void
nsGridRowLayout::ChildrenInserted(nsIBox* aBox, nsBoxLayoutState& aState,
                                  nsIBox* aPrevBox,
                                  const nsFrameList::Slice& aNewChildren)
{
  ChildAddedOrRemoved(aBox, aState);
}

void
nsGridRowLayout::ChildrenAppended(nsIBox* aBox, nsBoxLayoutState& aState,
                                  const nsFrameList::Slice& aNewChildren)
{
  ChildAddedOrRemoved(aBox, aState);
}

void
nsGridRowLayout::ChildrenRemoved(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)
{
  ChildAddedOrRemoved(aBox, aState);
}

void
nsGridRowLayout::ChildrenSet(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)
{
  ChildAddedOrRemoved(aBox, aState);
}

nsIGridPart*
nsGridRowLayout::GetParentGridPart(nsIBox* aBox, nsIBox** aParentBox)
{
  
  
  *aParentBox = nsnull;
  
  
  aBox = nsGrid::GetScrollBox(aBox);

  
  if (aBox)
    aBox = aBox->GetParentBox();

  if (aBox)
  {
    nsIGridPart* parentGridRow = nsGrid::GetPartFromBox(aBox);
    if (parentGridRow && parentGridRow->CanContain(this)) {
      *aParentBox = aBox;
      return parentGridRow;
    }
  }

  return nsnull;
}


nsGrid*
nsGridRowLayout::GetGrid(nsIBox* aBox, PRInt32* aIndex, nsGridRowLayout* aRequestor)
{

   if (aRequestor == nsnull)
   {
      nsIBox* parentBox; 
      nsIGridPart* parent = GetParentGridPart(aBox, &parentBox);
      if (parent)
         return parent->GetGrid(parentBox, aIndex, this);
      return nsnull;
   }

   PRInt32 index = -1;
   nsIBox* child = aBox->GetChildBox();
   PRInt32 count = 0;
   while(child)
   {
     
     nsIBox* childBox = nsGrid::GetScrolledBox(child);

     nsBoxLayout* layout = childBox->GetLayoutManager();
     nsIGridPart* gridRow = nsGrid::GetPartFromBox(childBox);
     if (gridRow) 
     {
       if (layout == aRequestor) {
          index = count;
          break;
       }
       count += gridRow->GetRowCount();
     } else 
       count++;

     child = child->GetNextBox();
   }

   
   
   
   if (index == -1) {
     *aIndex = -1;
     return nsnull;
   }

   (*aIndex) += index;

   nsIBox* parentBox; 
   nsIGridPart* parent = GetParentGridPart(aBox, &parentBox);
   if (parent)
     return parent->GetGrid(parentBox, aIndex, this);

   return nsnull;
}

nsMargin
nsGridRowLayout::GetTotalMargin(nsIBox* aBox, bool aIsHorizontal)
{
  
  nsMargin margin(0,0,0,0);
  nsIBox* parent = nsnull;
  nsIGridPart* part = GetParentGridPart(aBox, &parent);
  if (part && parent) {
    

    
    aBox = nsGrid::GetScrollBox(aBox);

    
    nsIBox* next = aBox->GetNextBox();

    
    nsIBox* child = parent->GetChildBox();

    margin = part->GetTotalMargin(parent, aIsHorizontal);

    
    if (child == aBox || next == nsnull) {

       
       
       if (child != aBox)
       {
          if (aIsHorizontal)
              margin.top = 0;
          else 
              margin.left = 0;
       }

       
       
       if (next != nsnull)
       {
          if (aIsHorizontal)
              margin.bottom = 0;
          else 
              margin.right = 0;
       }

    }
  }
    
  
  nsMargin ourMargin;
  aBox->GetMargin(ourMargin);
  margin += ourMargin;

  return margin;
}

NS_IMPL_ADDREF_INHERITED(nsGridRowLayout, nsBoxLayout)
NS_IMPL_RELEASE_INHERITED(nsGridRowLayout, nsBoxLayout)

NS_INTERFACE_MAP_BEGIN(nsGridRowLayout)
  NS_INTERFACE_MAP_ENTRY(nsIGridPart)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIGridPart)
NS_INTERFACE_MAP_END_INHERITING(nsBoxLayout)
