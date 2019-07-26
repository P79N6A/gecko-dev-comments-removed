











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
nsGridRowLayout::ChildrenInserted(nsIFrame* aBox, nsBoxLayoutState& aState,
                                  nsIFrame* aPrevBox,
                                  const nsFrameList::Slice& aNewChildren)
{
  ChildAddedOrRemoved(aBox, aState);
}

void
nsGridRowLayout::ChildrenAppended(nsIFrame* aBox, nsBoxLayoutState& aState,
                                  const nsFrameList::Slice& aNewChildren)
{
  ChildAddedOrRemoved(aBox, aState);
}

void
nsGridRowLayout::ChildrenRemoved(nsIFrame* aBox, nsBoxLayoutState& aState, nsIFrame* aChildList)
{
  ChildAddedOrRemoved(aBox, aState);
}

void
nsGridRowLayout::ChildrenSet(nsIFrame* aBox, nsBoxLayoutState& aState, nsIFrame* aChildList)
{
  ChildAddedOrRemoved(aBox, aState);
}

nsIGridPart*
nsGridRowLayout::GetParentGridPart(nsIFrame* aBox, nsIFrame** aParentBox)
{
  
  
  *aParentBox = nullptr;
  
  
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

  return nullptr;
}


nsGrid*
nsGridRowLayout::GetGrid(nsIFrame* aBox, PRInt32* aIndex, nsGridRowLayout* aRequestor)
{

   if (aRequestor == nullptr)
   {
      nsIFrame* parentBox; 
      nsIGridPart* parent = GetParentGridPart(aBox, &parentBox);
      if (parent)
         return parent->GetGrid(parentBox, aIndex, this);
      return nullptr;
   }

   PRInt32 index = -1;
   nsIFrame* child = aBox->GetChildBox();
   PRInt32 count = 0;
   while(child)
   {
     
     nsIFrame* childBox = nsGrid::GetScrolledBox(child);

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
     return nullptr;
   }

   (*aIndex) += index;

   nsIFrame* parentBox; 
   nsIGridPart* parent = GetParentGridPart(aBox, &parentBox);
   if (parent)
     return parent->GetGrid(parentBox, aIndex, this);

   return nullptr;
}

nsMargin
nsGridRowLayout::GetTotalMargin(nsIFrame* aBox, bool aIsHorizontal)
{
  
  nsMargin margin(0,0,0,0);
  nsIFrame* parent = nullptr;
  nsIGridPart* part = GetParentGridPart(aBox, &parent);
  if (part && parent) {
    

    
    aBox = nsGrid::GetScrollBox(aBox);

    
    nsIFrame* next = aBox->GetNextBox();

    
    nsIFrame* child = parent->GetChildBox();

    margin = part->GetTotalMargin(parent, aIsHorizontal);

    
    if (child == aBox || next == nullptr) {

       
       
       if (child != aBox)
       {
          if (aIsHorizontal)
              margin.top = 0;
          else 
              margin.left = 0;
       }

       
       
       if (next != nullptr)
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
