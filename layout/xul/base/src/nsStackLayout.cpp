












































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

nsSize
nsStackLayout::GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aState)
{
  nsSize rpref (0, 0);

  
  

  nsIBox* child = aBox->GetChildBox();
  while (child) {  
    nsSize pref = child->GetPrefSize(aState);

    AddMargin(child, pref);
    AddOffset(aState, child, pref);
    AddLargestSize(rpref, pref);

    child = child->GetNextBox();
  }

  
  AddBorderAndPadding(aBox, rpref);

  return rpref;
}

nsSize
nsStackLayout::GetMinSize(nsIBox* aBox, nsBoxLayoutState& aState)
{
  nsSize minSize (0, 0);

  

  nsIBox* child = aBox->GetChildBox();
  while (child) {  
    nsSize min = child->GetMinSize(aState);
    AddMargin(child, min);
    AddOffset(aState, child, min);
    AddLargestSize(minSize, min);

    child = child->GetNextBox();
  }

  
  AddBorderAndPadding(aBox, minSize);

  return minSize;
}

nsSize
nsStackLayout::GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aState)
{
  nsSize maxSize (NS_INTRINSICSIZE, NS_INTRINSICSIZE);

  

  nsIBox* child = aBox->GetChildBox();
  while (child) {  
    nsSize min = child->GetMinSize(aState);
    nsSize max = nsBox::BoundsCheckMinMax(min, child->GetMaxSize(aState));

    AddMargin(child, max);
    AddOffset(aState, child, max);
    AddSmallestSize(maxSize, max);

    child = child->GetNextBox();
  }

  
  AddBorderAndPadding(aBox, maxSize);

  return maxSize;
}


nscoord
nsStackLayout::GetAscent(nsIBox* aBox, nsBoxLayoutState& aState)
{
  nscoord vAscent = 0;

  nsIBox* child = aBox->GetChildBox();
  while (child) {  
    nscoord ascent = child->GetBoxAscent(aState);
    nsMargin margin;
    child->GetMargin(margin);
    ascent += margin.top + margin.bottom;
    if (ascent > vAscent)
      vAscent = ascent;

    child = child->GetNextBox();
  }

  return vAscent;
}

PRBool
nsStackLayout::AddOffset(nsBoxLayoutState& aState, nsIBox* aChild, nsSize& aSize)
{
  nsSize offset(0,0);
  
  
  
  
  
  if (aChild->IsBoxFrame() &&
      (aChild->GetStateBits() & NS_STATE_STACK_NOT_POSITIONED))
    return PR_FALSE;
  
  PRBool offsetSpecified = PR_FALSE;
  const nsStylePosition* pos = aChild->GetStylePosition();
  if (eStyleUnit_Coord == pos->mOffset.GetLeftUnit()) {
     offset.width = pos->mOffset.GetLeft().GetCoordValue();
     offsetSpecified = PR_TRUE;
  }

  if (eStyleUnit_Coord == pos->mOffset.GetTopUnit()) {
     offset.height = pos->mOffset.GetTop().GetCoordValue();
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

  if (!offsetSpecified && aChild->IsBoxFrame()) {
    
    
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

      
      if (sizeChanged || NS_SUBTREE_DIRTY(child)) {
          
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

