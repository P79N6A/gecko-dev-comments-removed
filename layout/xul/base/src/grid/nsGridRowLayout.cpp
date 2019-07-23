











































#include "nsGridRowLayout.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableFrame.h"
#include "nsBox.h"
#include "nsStackLayout.h"
#include "nsGrid.h"

nsGridRowLayout::nsGridRowLayout(nsIPresShell* aPresShell):nsSprocketLayout()
{
}

NS_IMETHODIMP
nsGridRowLayout::ChildrenInserted(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aPrevBox, nsIBox* aChildList)
{
  ChildAddedOrRemoved(aBox, aState);
  return NS_OK;
}

NS_IMETHODIMP
nsGridRowLayout::ChildrenAppended(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)
{
  ChildAddedOrRemoved(aBox, aState);
  return NS_OK;
}

NS_IMETHODIMP
nsGridRowLayout::ChildrenRemoved(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)
{
  ChildAddedOrRemoved(aBox, aState);
  return NS_OK;
}

NS_IMETHODIMP
nsGridRowLayout::ChildrenSet(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)
{
  ChildAddedOrRemoved(aBox, aState);
  return NS_OK;
}

void
nsGridRowLayout::GetParentGridPart(nsIBox* aBox, nsIBox** aParentBox, nsIGridPart** aParentGridPart)
{
  
  
  *aParentGridPart = nsnull;
  *aParentBox = nsnull;
  
  
  aBox = nsGrid::GetScrollBox(aBox);

  
  if (aBox)
    aBox = aBox->GetParentBox();

  if (aBox)
  {
    nsCOMPtr<nsIBoxLayout> layout;
    aBox->GetLayoutManager(getter_AddRefs(layout));
    nsCOMPtr<nsIGridPart> parentGridRow = do_QueryInterface(layout);
    if (parentGridRow && parentGridRow->CanContain(this)) {
      parentGridRow.swap(*aParentGridPart);
      *aParentBox = aBox;
    }
  }
}


nsGrid*
nsGridRowLayout::GetGrid(nsIBox* aBox, PRInt32* aIndex, nsGridRowLayout* aRequestor)
{

   if (aRequestor == nsnull)
   {
      nsCOMPtr<nsIGridPart> parent;
      nsIBox* parentBox; 
      GetParentGridPart(aBox, &parentBox, getter_AddRefs(parent));
      if (parent)
         return parent->GetGrid(parentBox, aIndex, this);
      return nsnull;
   }

   nsresult rv = NS_OK;

   PRInt32 index = -1;
   nsIBox* child = aBox->GetChildBox();
   PRInt32 count = 0;
   while(child)
   {
     
     nsIBox* childBox = nsGrid::GetScrolledBox(child);

     nsCOMPtr<nsIBoxLayout> layout;
     childBox->GetLayoutManager(getter_AddRefs(layout));
     
     
     nsCOMPtr<nsIGridPart> gridRow = do_QueryInterface(layout, &rv);
     if (NS_SUCCEEDED(rv) && gridRow) 
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

   nsCOMPtr<nsIGridPart> parent;
   nsIBox* parentBox; 
   GetParentGridPart(aBox, &parentBox, getter_AddRefs(parent));

   if (parent)
     return parent->GetGrid(parentBox, aIndex, this);

   return nsnull;
}

nsMargin
nsGridRowLayout::GetTotalMargin(nsIBox* aBox, PRBool aIsHorizontal)
{
  
  nsMargin margin(0,0,0,0);
  nsCOMPtr<nsIGridPart> part;
  nsIBox* parent = nsnull;
  GetParentGridPart(aBox, &parent, getter_AddRefs(part));

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
