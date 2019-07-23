












































#include "nsStackLayout.h"
#include "nsCOMPtr.h"
#include "nsBoxLayoutState.h"
#include "nsBox.h"
#include "nsBoxFrame.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsINameSpaceManager.h"

nsIBoxLayout* nsStackLayout::gInstance = nsnull;

nsresult
NS_NewStackLayout( nsIPresShell* aPresShell, nsCOMPtr<nsIBoxLayout>& aNewLayout)
{
  if (!nsStackLayout::gInstance) {
    nsStackLayout::gInstance = new nsStackLayout();
    NS_IF_ADDREF(nsStackLayout::gInstance);
  }
  
  aNewLayout = nsStackLayout::gInstance;
  return NS_OK;
} 

 void
nsStackLayout::Shutdown()
{
  NS_IF_RELEASE(gInstance);
}

nsStackLayout::nsStackLayout()
{
}

NS_IMETHODIMP
nsStackLayout::GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aState, nsSize& aSize)
{
  aSize.width = 0;
  aSize.height = 0;

  
  

  nsIBox* child = aBox->GetChildBox();
  while (child) {  
    nsSize pref = child->GetPrefSize(aState);

    AddMargin(child, pref);
    AddOffset(aState, child, pref);
    AddLargestSize(aSize, pref);

    child = child->GetNextBox();
  }

  
  AddBorderAndPadding(aBox, aSize);

  return NS_OK;
}

NS_IMETHODIMP
nsStackLayout::GetMinSize(nsIBox* aBox, nsBoxLayoutState& aState, nsSize& aSize)
{
  aSize.width = 0;
  aSize.height = 0;
   
  

  nsIBox* child = aBox->GetChildBox();
  while (child) {  
    nsSize min = child->GetMinSize(aState);
    AddMargin(child, min);
    AddOffset(aState, child, min);
    AddLargestSize(aSize, min);

    child = child->GetNextBox();
  }

  
  AddBorderAndPadding(aBox, aSize);

  return NS_OK;
}

NS_IMETHODIMP
nsStackLayout::GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aState, nsSize& aSize)
{
  aSize.width = NS_INTRINSICSIZE;
  aSize.height = NS_INTRINSICSIZE;

  

  nsIBox* child = aBox->GetChildBox();
  while (child) {  
    nsSize max = child->GetMaxSize(aState);
    nsSize min = child->GetMinSize(aState);
    nsBox::BoundsCheckMinMax(min, max);

    AddMargin(child, max);
    AddOffset(aState, child, max);
    AddSmallestSize(aSize, max);

    child = child->GetNextBox();
  }

  
  AddBorderAndPadding(aBox, aSize);

  return NS_OK;
}


NS_IMETHODIMP
nsStackLayout::GetAscent(nsIBox* aBox, nsBoxLayoutState& aState, nscoord& aAscent)
{
  aAscent = 0;

  nsIBox* child = aBox->GetChildBox();
  while (child) {  
    nscoord ascent = child->GetBoxAscent(aState);
    nsMargin margin;
    child->GetMargin(margin);
    ascent += margin.top + margin.bottom;
    if (ascent > aAscent)
      aAscent = ascent;

    child = child->GetNextBox();
  }

  return NS_OK;
}

PRBool
nsStackLayout::AddOffset(nsBoxLayoutState& aState, nsIBox* aChild, nsSize& aSize)
{
  nsSize offset(0,0);
  
  
  
  
  
  if (aChild->GetStateBits() & NS_STATE_STACK_NOT_POSITIONED)
    return PR_FALSE;
  
  PRBool offsetSpecified = PR_FALSE;
  const nsStylePosition* pos = aChild->GetStylePosition();
  if (eStyleUnit_Coord == pos->mOffset.GetLeftUnit()) {
     nsStyleCoord left = 0;
     pos->mOffset.GetLeft(left);
     offset.width = left.GetCoordValue();
     offsetSpecified = PR_TRUE;
  }

  if (eStyleUnit_Coord == pos->mOffset.GetTopUnit()) {
     nsStyleCoord top = 0;
     pos->mOffset.GetTop(top);
     offset.height = top.GetCoordValue();
     offsetSpecified = PR_TRUE;
  }

  nsIContent* content = aChild->GetContent();

  if (content) {
    nsAutoString value;
    PRInt32 error;

    content->GetAttr(kNameSpaceID_None, nsGkAtoms::left, value);
    if (!value.IsEmpty()) {
      value.Trim("%");
      offset.width =
        nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
      offsetSpecified = PR_TRUE;
    }

    content->GetAttr(kNameSpaceID_None, nsGkAtoms::top, value);
    if (!value.IsEmpty()) {
      value.Trim("%");
      offset.height =
        nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
      offsetSpecified = PR_TRUE;
    }
  }

  aSize += offset;

  if (!offsetSpecified) {
    
    
    aChild->AddStateBits(NS_STATE_STACK_NOT_POSITIONED);
  }
  
  return offsetSpecified;
}


NS_IMETHODIMP
nsStackLayout::Layout(nsIBox* aBox, nsBoxLayoutState& aState)
{
  nsRect clientRect;
  aBox->GetClientRect(clientRect);

  PRBool grow;

  do {
    nsIBox* child = aBox->GetChildBox();
    grow = PR_FALSE;

    while (child) 
    {  
      nsMargin margin;
      child->GetMargin(margin);
      nsRect childRect(clientRect);
      childRect.Deflate(margin);

      if (childRect.width < 0)
        childRect.width = 0;

      if (childRect.height < 0)
        childRect.height = 0;

      nsRect oldRect(child->GetRect());
      PRBool sizeChanged = (oldRect != childRect);

      
      if (sizeChanged || (child->GetStateBits() & (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN))) {
          
          nsMargin margin;
          child->GetMargin(margin);

          
          nsSize offset(0,0);
          PRBool offsetSpecified = AddOffset(aState, child, offset);

          
          
          childRect.x = clientRect.x + offset.width + margin.left;
          childRect.y = clientRect.y + offset.height + margin.top;
          
          
          
          if (offsetSpecified) {
            nsSize pref = child->GetPrefSize(aState);
            childRect.width = pref.width;
            childRect.height = pref.height;
          }

          
          child->SetBounds(aState, childRect);

          
          child->Layout(aState);

          
          nsRect childRectNoMargin;
          childRectNoMargin = childRect = child->GetRect();
          childRect.Inflate(margin);

          
          if (offset.width + childRect.width > clientRect.width) {
            clientRect.width = childRect.width + offset.width;
            grow = PR_TRUE;
          }

          if (offset.height + childRect.height > clientRect.height) {
            clientRect.height = childRect.height + offset.height;
            grow = PR_TRUE;
          }

          if (childRectNoMargin != oldRect)
          {
            
            
            
            
            
            if (childRectNoMargin.Intersects(oldRect)) {
              nsRect u;
              u.UnionRect(oldRect, childRectNoMargin);
              aBox->Redraw(aState, &u);
            } else {
              aBox->Redraw(aState, &oldRect);
              aBox->Redraw(aState, &childRectNoMargin);
            }
          }
       }

       child = child->GetNextBox();
     }
   } while (grow);
   
   
   
   nsRect bounds(aBox->GetRect());
   nsMargin bp;
   aBox->GetBorderAndPadding(bp);
   clientRect.Inflate(bp);

   if (clientRect.width > bounds.width || clientRect.height > bounds.height)
   {
     if (clientRect.width > bounds.width)
       bounds.width = clientRect.width;
     if (clientRect.height > bounds.height)
       bounds.height = clientRect.height;

     aBox->SetBounds(aState, bounds);
   }

   return NS_OK;
}

