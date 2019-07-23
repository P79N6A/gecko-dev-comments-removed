













































#include "nsBoxLayoutState.h"
#include "nsSprocketLayout.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsHTMLContainerFrame.h"
#include "nsBoxFrame.h"
#include "nsBoxFrame.h"

nsIBoxLayout* nsSprocketLayout::gInstance = nsnull;



#define DEBUG_SPRING_SIZE 8
#define DEBUG_BORDER_SIZE 2
#define COIL_SIZE 8


nsresult
NS_NewSprocketLayout( nsIPresShell* aPresShell, nsCOMPtr<nsIBoxLayout>& aNewLayout)
{
  if (!nsSprocketLayout::gInstance) {
    nsSprocketLayout::gInstance = new nsSprocketLayout();
    NS_IF_ADDREF(nsSprocketLayout::gInstance);
  }
  
  aNewLayout = nsSprocketLayout::gInstance;
  return NS_OK;
} 

 void
nsSprocketLayout::Shutdown()
{
  NS_IF_RELEASE(gInstance);
}

nsSprocketLayout::nsSprocketLayout()
{
}

PRBool 
nsSprocketLayout::IsHorizontal(nsIBox* aBox)
{
   return (aBox->GetStateBits() & NS_STATE_IS_HORIZONTAL) != 0;
}

void
nsSprocketLayout::GetFrameState(nsIBox* aBox, nsFrameState& aState)
{
   aState = aBox->GetStateBits();
}

static PRUint8
GetFrameDirection(nsIBox* aBox)
{
   return aBox->GetStyleVisibility()->mDirection;
}

static void
HandleBoxPack(nsIBox* aBox, const nsFrameState& aFrameState, nscoord& aX, nscoord& aY, 
              const nsRect& aOriginalRect, const nsRect& aClientRect)
{
  
  
  
  
  PRUint8 frameDirection = GetFrameDirection(aBox);

  if (aFrameState & NS_STATE_IS_HORIZONTAL) {
    if (aFrameState & NS_STATE_IS_DIRECTION_NORMAL) {
      
      aX = aClientRect.x;
    }
    else {
      
      aX = aClientRect.x + aOriginalRect.width;
    }
    
    aY = aClientRect.y;    
  }
  else {
    
    if (frameDirection == NS_STYLE_DIRECTION_LTR) {
      
      aX = aClientRect.x;
    }
    else {
      
      aX = aClientRect.x + aOriginalRect.width;
    }
    if (aFrameState & NS_STATE_IS_DIRECTION_NORMAL) {
      
      aY = aClientRect.y;
    }
    else {
      
      aY = aClientRect.y + aOriginalRect.height;
    }
  }

  
  nsIBox::Halignment halign = aBox->GetHAlign();
  nsIBox::Valignment valign = aBox->GetVAlign();

  
  
  
  
  
  
  
  
  if (aFrameState & NS_STATE_IS_HORIZONTAL) {
    switch(halign) {
      case nsBoxFrame::hAlign_Left:
        break; 

      case nsBoxFrame::hAlign_Center:
        if (aFrameState & NS_STATE_IS_DIRECTION_NORMAL)
          aX += (aOriginalRect.width - aClientRect.width)/2;
        else 
          aX -= (aOriginalRect.width - aClientRect.width)/2;
        break;

      case nsBoxFrame::hAlign_Right:
        if (aFrameState & NS_STATE_IS_DIRECTION_NORMAL)
          aX += (aOriginalRect.width - aClientRect.width);
        else
          aX -= (aOriginalRect.width - aClientRect.width);
        break; 
    }
  } else {
    switch(valign) {
      case nsBoxFrame::vAlign_Top:
      case nsBoxFrame::vAlign_BaseLine: 
        break;  

      case nsBoxFrame::vAlign_Middle:
        if (aFrameState & NS_STATE_IS_DIRECTION_NORMAL)
          aY += (aOriginalRect.height - aClientRect.height)/2;
        else
          aY -= (aOriginalRect.height - aClientRect.height)/2;
        break;

      case nsBoxFrame::vAlign_Bottom:
        if (aFrameState & NS_STATE_IS_DIRECTION_NORMAL)
          aY += (aOriginalRect.height - aClientRect.height);
        else
          aY -= (aOriginalRect.height - aClientRect.height);
        break;
    }
  }
}

NS_IMETHODIMP
nsSprocketLayout::Layout(nsIBox* aBox, nsBoxLayoutState& aState)
{
  
  
  if (aBox->IsCollapsed(aState)) {
    nsIBox* child = aBox->GetChildBox();
    while(child) 
    {
      nsBoxFrame::LayoutChildAt(aState, child, nsRect(0,0,0,0));  
      child = child->GetNextBox();
    }
    return NS_OK;
  }

  aState.PushStackMemory();

  
  nsSize originalSize = aBox->GetSize();

  
  nsRect clientRect;
  aBox->GetClientRect(clientRect);

  
  
  
  
  
  
  
  nsRect originalClientRect(clientRect);

  
  
  nsFrameState frameState = 0;
  GetFrameState(aBox, frameState);

  
  nsBoxSize*         boxSizes = nsnull;
  nsComputedBoxSize* computedBoxSizes = nsnull;

  nscoord maxAscent = aBox->GetBoxAscent(aState);

  nscoord min = 0;
  nscoord max = 0;
  PRInt32 flexes = 0;
  PopulateBoxSizes(aBox, aState, boxSizes, computedBoxSizes, min, max, flexes);
  
  
  
  
  nscoord size = clientRect.width;
  if (!IsHorizontal(aBox))
    size = clientRect.height;
  ComputeChildSizes(aBox, aState, size, boxSizes, computedBoxSizes);

  
  
  
  
  
  
  
  
  
  if (IsHorizontal(aBox)) {
    clientRect.width = size;
    if (clientRect.height < min)
      clientRect.height = min;

    if (frameState & NS_STATE_AUTO_STRETCH) {
      if (clientRect.height > max)
        clientRect.height = max;
    }
  } else {
    clientRect.height = size;
    if (clientRect.width < min)
      clientRect.width = min;

    if (frameState & NS_STATE_AUTO_STRETCH) {
      if (clientRect.width > max)
        clientRect.width = max;
    }
  }

  
  PRBool needsRedraw = PR_FALSE;
  PRBool finished;
  nscoord passes = 0;

  
  
  
  
  nscoord x = 0;
  nscoord y = 0;
  nscoord origX = 0;
  nscoord origY = 0;

  
  
  PRBool childResized = PR_FALSE;

  
  
  passes = 0;
  do 
  { 
#ifdef DEBUG_REFLOW
    if (passes > 0) {
      AddIndents();
      printf("ChildResized doing pass: %d\n", passes);
    }
#endif 

    
    
    finished = PR_TRUE;

    
    HandleBoxPack(aBox, frameState, x, y, originalClientRect, clientRect);

    
    
    origX = x;
    origY = y;

    nscoord nextX = x;
    nscoord nextY = y;

    
    
    
    nsComputedBoxSize* childComputedBoxSize = computedBoxSizes;
    nsBoxSize* childBoxSize                 = boxSizes;

    nsIBox* child = aBox->GetChildBox();

    PRInt32 count = 0;
    while (child || (childBoxSize && childBoxSize->bogus))
    { 
      
      
      if (childBoxSize == nsnull) {
        NS_NOTREACHED("Lists not the same length.");
        break;
      }
        
      nscoord width = clientRect.width;
      nscoord height = clientRect.height;

      if (!childBoxSize->bogus) {
        
        
        
        
        if (!(frameState & NS_STATE_AUTO_STRETCH)) {
           nsSize prefSize = child->GetPrefSize(aState);
           nsSize minSize = child->GetMinSize(aState);
           nsSize maxSize = child->GetMaxSize(aState);
           prefSize = nsBox::BoundsCheck(minSize, prefSize, maxSize);
       
           AddMargin(child, prefSize);
           width = NS_MIN(prefSize.width, originalClientRect.width);
           height = NS_MIN(prefSize.height, originalClientRect.height);
        }
      }

      
      
      if (frameState & NS_STATE_IS_HORIZONTAL)
        width = childComputedBoxSize->size;
      else
        height = childComputedBoxSize->size;
      
      
      if (frameState & NS_STATE_IS_HORIZONTAL) {
        if (frameState & NS_STATE_IS_DIRECTION_NORMAL)
          x += (childBoxSize->left);
        else
          x -= (childBoxSize->right);
      } else {
        if (frameState & NS_STATE_IS_DIRECTION_NORMAL)
          y += (childBoxSize->left);
        else
          y -= (childBoxSize->right);
      }

      nextX = x;
      nextY = y;

      
      nscoord rectX = x;
      nscoord rectY = y;
      if (!(frameState & NS_STATE_IS_DIRECTION_NORMAL)) {
        if (frameState & NS_STATE_IS_HORIZONTAL)
          rectX -= width;
        else
          rectY -= height;
      }

      
      nsRect childRect(rectX, rectY, width, height);

      
      
      
      
      
      if (childRect.width > clientRect.width || childRect.height > clientRect.height) {
        if (childRect.width > clientRect.width)
          clientRect.width = childRect.width;

        if (childRect.height > clientRect.height)
          clientRect.height = childRect.height;
      }
    
      
      
      ComputeChildsNextPosition(aBox, child, 
                                x, 
                                y, 
                                nextX, 
                                nextY, 
                                childRect, 
                                originalClientRect, 
                                childBoxSize->ascent,
                                maxAscent);

      
      
      if (frameState & NS_STATE_IS_HORIZONTAL) {
        if (frameState & NS_STATE_IS_DIRECTION_NORMAL)
          nextX += (childBoxSize->right);
        else
          nextX -= (childBoxSize->left);
        childRect.y = y;
      }
      else {
        if (frameState & NS_STATE_IS_DIRECTION_NORMAL)
          nextY += (childBoxSize->right);
        else 
          nextY -= (childBoxSize->left);
        childRect.x = x;
      }
      
      
      
      if (childBoxSize->bogus) 
      {
        childComputedBoxSize = childComputedBoxSize->next;
        childBoxSize = childBoxSize->next;
        count++;
        x = nextX;
        y = nextY;
        continue;
      }

      nsMargin margin(0,0,0,0);

      PRBool layout = PR_TRUE;

      
      child->GetMargin(margin);
      childRect.Deflate(margin);
      if (childRect.width < 0)
        childRect.width = 0;
      if (childRect.height < 0)
        childRect.height = 0;

      
      
      if (passes > 0) {
        layout = PR_FALSE;
      } else {
        
        if (!NS_SUBTREE_DIRTY(child))
          layout = PR_FALSE;
      }

      
      
      
      nsRect oldRect(child->GetRect());
      PRBool sizeChanged = PR_FALSE;

      child->SetBounds(aState, childRect);
      sizeChanged = (childRect.width != oldRect.width || childRect.height != oldRect.height);

      PRBool possibleRedraw = PR_FALSE;

      if (sizeChanged) {
        
        
        nsSize minSize = child->GetMinSize(aState);
        nsSize maxSize = child->GetMaxSize(aState);
        maxSize = nsBox::BoundsCheckMinMax(minSize, maxSize);

        
        if (childRect.width > maxSize.width)
          childRect.width = maxSize.width;

        if (childRect.height > maxSize.height)
          childRect.height = maxSize.height;
           
        
        child->SetBounds(aState, childRect);

        
        possibleRedraw = PR_TRUE;
      }

      
      if (oldRect.x != childRect.x || oldRect.y != childRect.y)
          possibleRedraw = PR_TRUE;

      
      
      
      if (layout || sizeChanged)
        child->Layout(aState);
      
      
      
      nsRect newChildRect(child->GetRect());

      if (newChildRect != childRect) {
#ifdef DEBUG_GROW
        child->DumpBox(stdout);
        printf(" GREW from (%d,%d) -> (%d,%d)\n", childRect.width, childRect.height, newChildRect.width, newChildRect.height);
#endif
        newChildRect.Inflate(margin);
        childRect.Inflate(margin);

        
        
        ChildResized(aBox,
                     aState, 
                     child,
                     childBoxSize,
                     childComputedBoxSize,
                     boxSizes, 
                     computedBoxSizes, 
                     childRect,
                     newChildRect,
                     clientRect,
                     flexes,
                     finished);

        
        childResized = PR_TRUE;

        
        
        if (clientRect.width > originalClientRect.width || clientRect.height > originalClientRect.height) {
          if (clientRect.width > originalClientRect.width)
            originalClientRect.width = clientRect.width;

          if (clientRect.height > originalClientRect.height)
            originalClientRect.height = clientRect.height;
        }

        if (!(frameState & NS_STATE_IS_DIRECTION_NORMAL)) {
          
          
          
          
          
          
          if (frameState & NS_STATE_IS_HORIZONTAL)
            newChildRect.x = childRect.XMost() - newChildRect.width;
          else
            newChildRect.y = childRect.YMost() - newChildRect.height;
        }

        
        ComputeChildsNextPosition(aBox, child, 
                                  x, 
                                  y, 
                                  nextX, 
                                  nextY, 
                                  newChildRect, 
                                  originalClientRect, 
                                  childBoxSize->ascent,
                                  maxAscent);

        
        
        if (frameState & NS_STATE_IS_HORIZONTAL)
          newChildRect.y = y;
        else
          newChildRect.x = x;
         
        if (newChildRect.width >= margin.left + margin.right && newChildRect.height >= margin.top + margin.bottom) 
          newChildRect.Deflate(margin);

        if (childRect.width >= margin.left + margin.right && childRect.height >= margin.top + margin.bottom) 
          childRect.Deflate(margin);
            
        child->SetBounds(aState, newChildRect);

        
        if (count == 0)
          finished = PR_TRUE;
      }

      
      x = nextX;
      y = nextY;
     
      
      if (possibleRedraw)
        needsRedraw = PR_TRUE;

      
      childComputedBoxSize = childComputedBoxSize->next;
      childBoxSize = childBoxSize->next;

      child = child->GetNextBox();
      count++;
    }

    
    passes++;
    NS_ASSERTION(passes < 10, "A Box's child is constantly growing!!!!!");
    if (passes > 10)
      break;
  } while (PR_FALSE == finished);

  
  while(boxSizes)
  {
    nsBoxSize* toDelete = boxSizes;
    boxSizes = boxSizes->next;
    delete toDelete;
  }

  while(computedBoxSizes)
  {
    nsComputedBoxSize* toDelete = computedBoxSizes;
    computedBoxSizes = computedBoxSizes->next;
    delete toDelete;
  }

  if (childResized) {
    
    nsRect tmpClientRect(originalClientRect);
    nsMargin bp(0,0,0,0);
    aBox->GetBorderAndPadding(bp);
    tmpClientRect.Inflate(bp);

    if (tmpClientRect.width > originalSize.width || tmpClientRect.height > originalSize.height)
    {
      
      nsRect bounds(aBox->GetRect());
      if (tmpClientRect.width > originalSize.width)
        bounds.width = tmpClientRect.width;

      if (tmpClientRect.height > originalSize.height)
        bounds.height = tmpClientRect.height;

      aBox->SetBounds(aState, bounds);
    }
  }

  
  
  HandleBoxPack(aBox, frameState, x, y, originalClientRect, clientRect);

  
  
  
  if (x != origX || y != origY) {
    nsIBox* child = aBox->GetChildBox();

    
    while (child) 
    {
      nsRect childRect(child->GetRect());
      childRect.x += (x - origX);
      childRect.y += (y - origY);
      child->SetBounds(aState, childRect);
      child = child->GetNextBox();
    }
  }

  
  if (needsRedraw)
    aBox->Redraw(aState);

  aState.PopStackMemory();

  
  
  return NS_OK;
}

void
nsSprocketLayout::PopulateBoxSizes(nsIBox* aBox, nsBoxLayoutState& aState, nsBoxSize*& aBoxSizes, nsComputedBoxSize*& aComputedBoxSizes, nscoord& aMinSize, nscoord& aMaxSize, PRInt32& aFlexes)
{
  
  nscoord biggestPrefWidth = 0;
  nscoord biggestMinWidth = 0;
  nscoord smallestMaxWidth = NS_INTRINSICSIZE;

  nsFrameState frameState = 0;
  GetFrameState(aBox, frameState);

  
  

  aMinSize = 0;
  aMaxSize = NS_INTRINSICSIZE;

  PRBool isHorizontal;

  if (IsHorizontal(aBox))
     isHorizontal = PR_TRUE;
  else
     isHorizontal = PR_FALSE;

  
  
  
  
  
  
  
  

  
  nsIBox* child = aBox->GetChildBox();

  aFlexes = 0;
  nsBoxSize* currentBox = nsnull;

#if 0
  nsBoxSize* start = aBoxSizes;
  
  while(child)
  {
    
    
    
    nscoord flex = 0;    

    if (!start) {
      if (!currentBox) {
        aBoxSizes      = new (aState) nsBoxSize();
        currentBox      = aBoxSizes;
      } else {
        currentBox->next      = new (aState) nsBoxSize();
        currentBox      = currentBox->next;
      }
    

      flex = child->GetFlex(aState);

      currentBox->flex = flex;
      currentBox->collapsed = child->IsCollapsed(aState);
    } else {
      flex = start->flex;
      start = start->next;
    }
    
    if (flex > 0) 
       aFlexes++;
   
    child = child->GetNextBox();
  }
#endif

  
  child = aBox->GetChildBox();
  currentBox = aBoxSizes;
  nsBoxSize* last = nsnull;

  nscoord maxFlex = 0;
  PRInt32 childCount = 0;

  while(child)
  {
    while (currentBox && currentBox->bogus) {
      last = currentBox;
      currentBox = currentBox->next;
    }
    ++childCount;
    nsSize pref(0,0);
    nsSize minSize(0,0);
    nsSize maxSize(NS_INTRINSICSIZE,NS_INTRINSICSIZE);
    nscoord ascent = 0;
    PRBool collapsed = child->IsCollapsed(aState);

    if (!collapsed) {
    
    
    

      pref = child->GetPrefSize(aState);
      minSize = child->GetMinSize(aState);
      maxSize = nsBox::BoundsCheckMinMax(minSize, child->GetMaxSize(aState));
      ascent = child->GetBoxAscent(aState);
      nsMargin margin;
      child->GetMargin(margin);
      ascent += margin.top;
    

      pref = nsBox::BoundsCheck(minSize, pref, maxSize);

      AddMargin(child, pref);
      AddMargin(child, minSize);
      AddMargin(child, maxSize);
    }

    if (!currentBox) {
      
      currentBox = new (aState) nsBoxSize();
      if (!aBoxSizes) {
        aBoxSizes = currentBox;
        last = aBoxSizes;
      } else {
        last->next = currentBox;
        last = currentBox;
      }

      nscoord minWidth;
      nscoord maxWidth;
      nscoord prefWidth;

      
      if (isHorizontal) {
          minWidth  = minSize.width;
          maxWidth  = maxSize.width;
          prefWidth = pref.width;
      } else {
          minWidth = minSize.height;
          maxWidth = maxSize.height;
          prefWidth = pref.height;
      }

      nscoord flex = child->GetFlex(aState);

      
      if (collapsed) {
        currentBox->flex = 0;
      }
      else {
        if (flex > maxFlex) {
          maxFlex = flex;
        }
        currentBox->flex = flex;
      }

      
      if (frameState & NS_STATE_EQUAL_SIZE) {

        if (prefWidth > biggestPrefWidth) 
          biggestPrefWidth = prefWidth;

        if (minWidth > biggestMinWidth) 
          biggestMinWidth = minWidth;

        if (maxWidth < smallestMaxWidth) 
          smallestMaxWidth = maxWidth;
      } else { 
        currentBox->pref    = prefWidth;
        currentBox->min     = minWidth;
        currentBox->max     = maxWidth;
      }

      NS_ASSERTION(minWidth <= prefWidth && prefWidth <= maxWidth,"Bad min, pref, max widths!");

    }

    if (!isHorizontal) {
      if (minSize.width > aMinSize)
        aMinSize = minSize.width;

      if (maxSize.width < aMaxSize)
        aMaxSize = maxSize.width;

    } else {
      if (minSize.height > aMinSize)
        aMinSize = minSize.height;

      if (maxSize.height < aMaxSize)
        aMaxSize = maxSize.height;
    }

    currentBox->ascent  = ascent;
    currentBox->collapsed = collapsed;
    aFlexes += currentBox->flex;

    child = child->GetNextBox();

    last = currentBox;
    currentBox = currentBox->next;

  }

  if (childCount > 0) {
    nscoord maxAllowedFlex = nscoord_MAX / childCount;
  
    if (NS_UNLIKELY(maxFlex > maxAllowedFlex)) {
      
      currentBox = aBoxSizes;
      while (currentBox) {
        currentBox->flex = NS_MIN(currentBox->flex, maxAllowedFlex);
        currentBox = currentBox->next;      
      }
    }
  }
#ifdef DEBUG
  else {
    NS_ASSERTION(maxFlex == 0, "How did that happen?");
  }
#endif

  
  if (frameState & NS_STATE_EQUAL_SIZE) {
    smallestMaxWidth = NS_MAX(smallestMaxWidth, biggestMinWidth);
    biggestPrefWidth = nsBox::BoundsCheck(biggestMinWidth, biggestPrefWidth, smallestMaxWidth);

    currentBox = aBoxSizes;

    while(currentBox)
    {
      if (!currentBox->collapsed) {
        currentBox->pref = biggestPrefWidth;
        currentBox->min = biggestMinWidth;
        currentBox->max = smallestMaxWidth;
      } else {
        currentBox->pref = 0;
        currentBox->min = 0;
        currentBox->max = 0;
      }
      currentBox = currentBox->next;
    }
  }

}

void
nsSprocketLayout::ComputeChildsNextPosition(nsIBox* aBox, 
                                      nsIBox* aChild, 
                                      nscoord& aCurX, 
                                      nscoord& aCurY, 
                                      nscoord& aNextX, 
                                      nscoord& aNextY, 
                                      const nsRect& aCurrentChildSize, 
                                      const nsRect& aBoxRect,
                                      nscoord childAscent,
                                      nscoord aMaxAscent)
{
  nsFrameState frameState = 0;
  GetFrameState(aBox, frameState);

  nsIBox::Halignment halign = aBox->GetHAlign();
  nsIBox::Valignment valign = aBox->GetVAlign();

  if (IsHorizontal(aBox)) {
    
    if (frameState & NS_STATE_IS_DIRECTION_NORMAL)
      aNextX = aCurX + aCurrentChildSize.width;
    else aNextX = aCurX - aCurrentChildSize.width;

    if (frameState & NS_STATE_AUTO_STRETCH)
      aCurY = aBoxRect.y;
    else {
      switch (valign) 
      {
         case nsBoxFrame::vAlign_BaseLine:
             aCurY = aBoxRect.y + (aMaxAscent - childAscent);
         break;

         case nsBoxFrame::vAlign_Top:
             aCurY = aBoxRect.y;
             break;
         case nsBoxFrame::vAlign_Middle:
             aCurY = aBoxRect.y + (aBoxRect.height/2 - aCurrentChildSize.height/2);
             break;
         case nsBoxFrame::vAlign_Bottom:
             aCurY = aBoxRect.y + aBoxRect.height - aCurrentChildSize.height;
             break;
      }
    }
  } else {
    
    if (frameState & NS_STATE_IS_DIRECTION_NORMAL)
      aNextY = aCurY + aCurrentChildSize.height;
    else
      aNextY = aCurY - aCurrentChildSize.height;

    if (frameState & NS_STATE_AUTO_STRETCH)
      aCurX = aBoxRect.x;
    else {
      PRUint8 frameDirection = GetFrameDirection(aBox);
      switch (halign) 
      {
         case nsBoxFrame::hAlign_Left:
           if (frameDirection == NS_STYLE_DIRECTION_LTR)
             aCurX = aBoxRect.x;
           else
             aCurX = aBoxRect.x + aBoxRect.width - aCurrentChildSize.width;
           break;
         case nsBoxFrame::hAlign_Center:
             aCurX = aBoxRect.x + (aBoxRect.width/2 - aCurrentChildSize.width/2);
             break;
         case nsBoxFrame::hAlign_Right:
           if (frameDirection == NS_STYLE_DIRECTION_LTR)
             aCurX = aBoxRect.x + aBoxRect.width - aCurrentChildSize.width;
           else
             aCurX = aBoxRect.x;
           break;
      }
    }
  }
}

void
nsSprocketLayout::ChildResized(nsIBox* aBox,
                         nsBoxLayoutState& aState, 
                         nsIBox* aChild,
                         nsBoxSize* aChildBoxSize,
                         nsComputedBoxSize* aChildComputedSize,
                         nsBoxSize* aBoxSizes, 
                         nsComputedBoxSize* aComputedBoxSizes, 
                         const nsRect& aChildLayoutRect, 
                         nsRect& aChildActualRect, 
                         nsRect& aContainingRect,
                         PRInt32 aFlexes, 
                         PRBool& aFinished)
                         
{
      nsRect childCurrentRect(aChildLayoutRect);

      PRBool isHorizontal = IsHorizontal(aBox);
      nscoord childLayoutWidth  = GET_WIDTH(aChildLayoutRect,isHorizontal);
      nscoord& childActualWidth  = GET_WIDTH(aChildActualRect,isHorizontal);
      nscoord& containingWidth   = GET_WIDTH(aContainingRect,isHorizontal);   
      
      
      nscoord& childActualHeight = GET_HEIGHT(aChildActualRect,isHorizontal);
      nscoord& containingHeight  = GET_HEIGHT(aContainingRect,isHorizontal);

      PRBool recompute = PR_FALSE;

      
      if ( childActualHeight > containingHeight) {
            

            
            

            nsSize min = aChild->GetMinSize(aState);            
            nsSize max = nsBox::BoundsCheckMinMax(min, aChild->GetMaxSize(aState));
            AddMargin(aChild, max);

            if (isHorizontal)
              childActualHeight = max.height < childActualHeight ? max.height : childActualHeight;
            else
              childActualHeight = max.width < childActualHeight ? max.width : childActualHeight;

            
            if (childActualHeight > containingHeight) {
                 containingHeight = childActualHeight;

              
              
              
              aFinished = PR_FALSE;

              
              if (aFlexes > 0) {
                
                recompute = PR_TRUE;
                InvalidateComputedSizes(aComputedBoxSizes);
                nsComputedBoxSize* node = aComputedBoxSizes;

                while(node) {
                  node->resized = PR_FALSE;
                  node = node->next;
                }

              }              
            }
      } 
      
      if (childActualWidth > childLayoutWidth) {
            nsSize min = aChild->GetMinSize(aState);
            nsSize max = nsBox::BoundsCheckMinMax(min, aChild->GetMaxSize(aState));
            
            AddMargin(aChild, max);

            

            if (isHorizontal)
              childActualWidth = max.width < childActualWidth ? max.width : childActualWidth;
            else
              childActualWidth = max.height < childActualWidth ? max.height : childActualWidth;

            if (childActualWidth > childLayoutWidth) {
               aChildComputedSize->size = childActualWidth;
               aChildBoxSize->min = childActualWidth;
               if (aChildBoxSize->pref < childActualWidth)
                  aChildBoxSize->pref = childActualWidth;
               if (aChildBoxSize->max < childActualWidth)
                  aChildBoxSize->max = childActualWidth;

              
              if (aFlexes > 0) {
                InvalidateComputedSizes(aComputedBoxSizes);

                nsComputedBoxSize* node = aComputedBoxSizes;
                aChildComputedSize->resized = PR_TRUE;

                while(node) {
                  if (node->resized)
                      node->valid = PR_TRUE;
                
                  node = node->next;
                }

                recompute = PR_TRUE;
                aFinished = PR_FALSE;
              } else {
                containingWidth += aChildComputedSize->size - childLayoutWidth;
              }              
            }
      }

      if (recompute)
            ComputeChildSizes(aBox, aState, containingWidth, aBoxSizes, aComputedBoxSizes);

      if (childCurrentRect != aChildActualRect) {
        
        
        
        nsMargin margin(0,0,0,0);
        aChild->GetMargin(margin);
        nsRect rect(aChildActualRect);
        if (rect.width >= margin.left + margin.right && rect.height >= margin.top + margin.bottom) 
          rect.Deflate(margin);

        aChild->SetBounds(aState, rect);
        aChild->Layout(aState);
      }

}

void
nsSprocketLayout::InvalidateComputedSizes(nsComputedBoxSize* aComputedBoxSizes)
{
  while(aComputedBoxSizes) {
      aComputedBoxSizes->valid = PR_FALSE;
      aComputedBoxSizes = aComputedBoxSizes->next;
  }
}

void
nsSprocketLayout::ComputeChildSizes(nsIBox* aBox,
                           nsBoxLayoutState& aState, 
                           nscoord& aGivenSize, 
                           nsBoxSize* aBoxSizes, 
                           nsComputedBoxSize*& aComputedBoxSizes)
{  

  

  PRInt32 sizeRemaining            = aGivenSize;
  PRInt32 spacerConstantsRemaining = 0;

   

  if (!aComputedBoxSizes)
      aComputedBoxSizes = new (aState) nsComputedBoxSize();
  
  nsBoxSize*         boxSizes = aBoxSizes;
  nsComputedBoxSize* computedBoxSizes = aComputedBoxSizes;
  PRInt32 count = 0;
  PRInt32 validCount = 0;

  while (boxSizes) 
  {

    NS_ASSERTION((boxSizes->min <= boxSizes->pref && boxSizes->pref <= boxSizes->max),"bad pref, min, max size");

    
     
  
  
    
    
     
  
   
    
      if (computedBoxSizes->valid) { 
        sizeRemaining -= computedBoxSizes->size;
        validCount++;
      } else {
          if (boxSizes->flex == 0)
          {
            computedBoxSizes->valid = PR_TRUE;
            computedBoxSizes->size = boxSizes->pref;
            validCount++;
          }

          spacerConstantsRemaining += boxSizes->flex;
          sizeRemaining -= boxSizes->pref;
      }

      sizeRemaining -= (boxSizes->left + boxSizes->right);

    

    boxSizes = boxSizes->next;

    if (boxSizes && !computedBoxSizes->next) 
      computedBoxSizes->next = new (aState) nsComputedBoxSize();

    computedBoxSizes = computedBoxSizes->next;
    count++;
  }

  
  if (validCount < count)
  {
    
    
    PRBool limit = PR_TRUE;
    for (int pass=1; PR_TRUE == limit; pass++) 
    {
      limit = PR_FALSE;
      boxSizes = aBoxSizes;
      computedBoxSizes = aComputedBoxSizes;

      while (boxSizes) { 

        

   
      
          nscoord pref = 0;
          nscoord max  = NS_INTRINSICSIZE;
          nscoord min  = 0;
          nscoord flex = 0;

          pref = boxSizes->pref;
          min  = boxSizes->min;
          max  = boxSizes->max;
          flex = boxSizes->flex;

          
          if (!computedBoxSizes->valid) {
            PRInt32 newSize = pref + PRInt32(PRInt64(sizeRemaining) * flex / spacerConstantsRemaining);

            if (newSize<=min) {
              computedBoxSizes->size = min;
              computedBoxSizes->valid = PR_TRUE;
              spacerConstantsRemaining -= flex;
              sizeRemaining += pref;
              sizeRemaining -= min;
              limit = PR_TRUE;
            } else if (newSize>=max) {
              computedBoxSizes->size = max;
              computedBoxSizes->valid = PR_TRUE;
              spacerConstantsRemaining -= flex;
              sizeRemaining += pref;
              sizeRemaining -= max;
              limit = PR_TRUE;
            }
          }
       
        boxSizes         = boxSizes->next;
        computedBoxSizes = computedBoxSizes->next;
      }
    }
  }          

  
  
  aGivenSize = 0;
  boxSizes = aBoxSizes;
  computedBoxSizes = aComputedBoxSizes;

  while (boxSizes) { 

    
  
    
      nscoord pref = 0;
      nscoord flex = 0;
      pref = boxSizes->pref;
      flex = boxSizes->flex;

      if (!computedBoxSizes->valid) {
        computedBoxSizes->size = pref + PRInt32(PRInt64(sizeRemaining) * flex / spacerConstantsRemaining);
        computedBoxSizes->valid = PR_TRUE;
      }

      aGivenSize += (boxSizes->left + boxSizes->right);
      aGivenSize += computedBoxSizes->size;

   

    boxSizes         = boxSizes->next;
    computedBoxSizes = computedBoxSizes->next;
  }
}


nsSize
nsSprocketLayout::GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aState)
{
   nsSize vpref (0, 0); 
   PRBool isHorizontal = IsHorizontal(aBox);

   nscoord biggestPref = 0;

   
   

   nsIBox* child = aBox->GetChildBox();
   nsFrameState frameState = 0;
   GetFrameState(aBox, frameState);
   PRBool isEqual = !!(frameState & NS_STATE_EQUAL_SIZE);
   PRInt32 count = 0;
   
   while (child) 
   {  
      
      if (!child->IsCollapsed(aState))
      {
        nsSize pref = child->GetPrefSize(aState);
        AddMargin(child, pref);

        if (isEqual) {
          if (isHorizontal)
          {
            if (pref.width > biggestPref)
              biggestPref = pref.width;
          } else {
            if (pref.height > biggestPref)
              biggestPref = pref.height;
          }
        }

        AddLargestSize(vpref, pref, isHorizontal);
        count++;
      }

      child = child->GetNextBox();
   }

   if (isEqual) {
      if (isHorizontal)
         vpref.width = biggestPref*count;
      else
         vpref.height = biggestPref*count;
   }
    
   
   AddBorderAndPadding(aBox, vpref);

  return vpref;
}

nsSize
nsSprocketLayout::GetMinSize(nsIBox* aBox, nsBoxLayoutState& aState)
{
   nsSize minSize (0, 0);
   PRBool isHorizontal = IsHorizontal(aBox);

   nscoord biggestMin = 0;


   
   

   nsIBox* child = aBox->GetChildBox();
   nsFrameState frameState = 0;
   GetFrameState(aBox, frameState);
   PRBool isEqual = !!(frameState & NS_STATE_EQUAL_SIZE);
   PRInt32 count = 0;

   while (child) 
   {  
       
      if (!child->IsCollapsed(aState))
      {
        nsSize min = child->GetMinSize(aState);
        nsSize pref(0,0);
        
        
        
        if (child->GetFlex(aState) == 0) {
            pref = child->GetPrefSize(aState);
            if (isHorizontal)
               min.width = pref.width;
            else
               min.height = pref.height;
        }

        if (isEqual) {
          if (isHorizontal)
          {
            if (min.width > biggestMin)
              biggestMin = min.width;
          } else {
            if (min.height > biggestMin)
              biggestMin = min.height;
          }
        }

        AddMargin(child, min);
        AddLargestSize(minSize, min, isHorizontal);
        count++;
      }

      child = child->GetNextBox();
   }

   
   if (isEqual) {
      if (isHorizontal)
         minSize.width = biggestMin*count;
      else
         minSize.height = biggestMin*count;
   }

  
  AddBorderAndPadding(aBox, minSize);

  return minSize;
}

nsSize
nsSprocketLayout::GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aState)
{

  PRBool isHorizontal = IsHorizontal(aBox);

   nscoord smallestMax = NS_INTRINSICSIZE;
   nsSize maxSize (NS_INTRINSICSIZE, NS_INTRINSICSIZE);

   
   

   nsIBox* child = aBox->GetChildBox();
   nsFrameState frameState = 0;
   GetFrameState(aBox, frameState);
   PRBool isEqual = !!(frameState & NS_STATE_EQUAL_SIZE);
   PRInt32 count = 0;

   while (child) 
   {  
      
      if (!child->IsCollapsed(aState))
      {
        
        nsSize min = child->GetMinSize(aState);
        nsSize max = nsBox::BoundsCheckMinMax(min, child->GetMaxSize(aState));

        AddMargin(child, max);
        AddSmallestSize(maxSize, max, isHorizontal);

        if (isEqual) {
          if (isHorizontal)
          {
            if (max.width < smallestMax)
              smallestMax = max.width;
          } else {
            if (max.height < smallestMax)
              smallestMax = max.height;
          }
        }
        count++;
      }

      child = child->GetNextBox();
   }

   if (isEqual) {
     if (isHorizontal) {
         if (smallestMax != NS_INTRINSICSIZE)
            maxSize.width = smallestMax*count;
         else
            maxSize.width = NS_INTRINSICSIZE;
     } else {
         if (smallestMax != NS_INTRINSICSIZE)
            maxSize.height = smallestMax*count;
         else
            maxSize.height = NS_INTRINSICSIZE;
     }
   }

  
  AddBorderAndPadding(aBox, maxSize);

  return maxSize;
}


nscoord
nsSprocketLayout::GetAscent(nsIBox* aBox, nsBoxLayoutState& aState)
{
   nscoord vAscent = 0;

   PRBool isHorizontal = IsHorizontal(aBox);

   
   
   
   nsIBox* child = aBox->GetChildBox();
   
   while (child) 
   {  
      
      
      
        
        nscoord ascent = child->GetBoxAscent(aState);

        nsMargin margin;
        child->GetMargin(margin);
        ascent += margin.top;

        if (isHorizontal)
        {
          if (ascent > vAscent)
            vAscent = ascent;
        } else {
          if (vAscent == 0)
            vAscent = ascent;
        }
      

      child = child->GetNextBox();      
   }

  return vAscent;
}

void
nsSprocketLayout::SetLargestSize(nsSize& aSize1, const nsSize& aSize2, PRBool aIsHorizontal)
{
  if (aIsHorizontal)
  {
    if (aSize1.height < aSize2.height)
       aSize1.height = aSize2.height;
  } else {
    if (aSize1.width < aSize2.width)
       aSize1.width = aSize2.width;
  }
}

void
nsSprocketLayout::SetSmallestSize(nsSize& aSize1, const nsSize& aSize2, PRBool aIsHorizontal)
{
  if (aIsHorizontal)
  {
    if (aSize1.height > aSize2.height)
       aSize1.height = aSize2.height;
  } else {
    if (aSize1.width > aSize2.width)
       aSize1.width = aSize2.width;

  }
}

void
nsSprocketLayout::AddLargestSize(nsSize& aSize, const nsSize& aSizeToAdd, PRBool aIsHorizontal)
{
  if (aIsHorizontal)
    AddCoord(aSize.width, aSizeToAdd.width);
  else
    AddCoord(aSize.height, aSizeToAdd.height);

  SetLargestSize(aSize, aSizeToAdd, aIsHorizontal);
}

void
nsSprocketLayout::AddCoord(nscoord& aCoord, nscoord aCoordToAdd)
{
  if (aCoord != NS_INTRINSICSIZE) 
  {
    if (aCoordToAdd == NS_INTRINSICSIZE)
      aCoord = aCoordToAdd;
    else
      aCoord += aCoordToAdd;
  }
}
void
nsSprocketLayout::AddSmallestSize(nsSize& aSize, const nsSize& aSizeToAdd, PRBool aIsHorizontal)
{
  if (aIsHorizontal)
    AddCoord(aSize.width, aSizeToAdd.width);
  else
    AddCoord(aSize.height, aSizeToAdd.height);
    
  SetSmallestSize(aSize, aSizeToAdd, aIsHorizontal);
}

PRBool
nsSprocketLayout::GetDefaultFlex(PRInt32& aFlex)
{
    aFlex = 0;
    return PR_TRUE;
}

void
nsBoxSize::Add(const nsSize& minSize, 
               const nsSize& prefSize,
               const nsSize& maxSize,
               nscoord aAscent,
               nscoord aFlex,
               PRBool aIsHorizontal)
{
  nscoord pref2;
  nscoord min2;
  nscoord max2;

  if (aIsHorizontal) {
    pref2 = prefSize.width;
    min2  = minSize.width;
    max2  = maxSize.width;
  } else {
    pref2 = prefSize.height;
    min2  = minSize.height;
    max2  = maxSize.height;
  }

  if (min2 > min)
    min = min2;

  if (pref2 > pref)
    pref = pref2;

  if (max2 < max)
    max = max2;

  flex = aFlex;

  if (!aIsHorizontal) {
    if (aAscent > ascent)
      ascent = aAscent;
    }
}

void
nsBoxSize::Add(const nsMargin& aMargin, PRBool aIsHorizontal)
{
  if (aIsHorizontal) {
    left  += aMargin.left;
    right += aMargin.right;
    pref -= (aMargin.left + aMargin.right);
  } else {
    left  += aMargin.top;
    right += aMargin.bottom;
    pref -= (aMargin.top + aMargin.bottom);
  }

  if (pref < min)
     min = pref;
}

nsComputedBoxSize::nsComputedBoxSize()
{
  Clear();
}

void
nsComputedBoxSize::Clear()
{
  resized = PR_FALSE;
  valid = PR_FALSE;
  size = 0;
  next = nsnull;
}

nsBoxSize::nsBoxSize()
{
  Clear();
}

void
nsBoxSize::Clear()
{
  pref = 0;
  min = 0;
  max = NS_INTRINSICSIZE;
  collapsed = PR_FALSE;
  ascent = 0;
  left = 0;
  right = 0;
  flex = 0;
  next = nsnull;
  bogus = PR_FALSE;
}


void* 
nsBoxSize::operator new(size_t sz, nsBoxLayoutState& aState) CPP_THROW_NEW
{
   return aState.AllocateStackMemory(sz);
}


void 
nsBoxSize::operator delete(void* aPtr, size_t sz)
{
}


void* 
nsComputedBoxSize::operator new(size_t sz, nsBoxLayoutState& aState) CPP_THROW_NEW
{
   return aState.AllocateStackMemory(sz);
}

void 
nsComputedBoxSize::operator delete(void* aPtr, size_t sz)
{
}
