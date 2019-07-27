











#include "nsBoxLayoutState.h"
#include "nsSprocketLayout.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsIPresShell.h"
#include "nsContainerFrame.h"
#include "nsBoxFrame.h"
#include "StackArena.h"
#include "mozilla/Likely.h"
#include <algorithm>

nsBoxLayout* nsSprocketLayout::gInstance = nullptr;



#define DEBUG_SPRING_SIZE 8
#define DEBUG_BORDER_SIZE 2
#define COIL_SIZE 8


nsresult
NS_NewSprocketLayout( nsIPresShell* aPresShell, nsCOMPtr<nsBoxLayout>& aNewLayout)
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

bool 
nsSprocketLayout::IsHorizontal(nsIFrame* aBox)
{
   return (aBox->GetStateBits() & NS_STATE_IS_HORIZONTAL) != 0;
}

void
nsSprocketLayout::GetFrameState(nsIFrame* aBox, nsFrameState& aState)
{
   aState = aBox->GetStateBits();
}

static uint8_t
GetFrameDirection(nsIFrame* aBox)
{
   return aBox->StyleVisibility()->mDirection;
}

static void
HandleBoxPack(nsIFrame* aBox, const nsFrameState& aFrameState, nscoord& aX, nscoord& aY, 
              const nsRect& aOriginalRect, const nsRect& aClientRect)
{
  
  
  
  
  uint8_t frameDirection = GetFrameDirection(aBox);

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

  
  nsIFrame::Halignment halign = aBox->GetHAlign();
  nsIFrame::Valignment valign = aBox->GetVAlign();

  
  
  
  
  
  
  
  
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
nsSprocketLayout::Layout(nsIFrame* aBox, nsBoxLayoutState& aState)
{
  
  
  if (aBox->IsCollapsed()) {
    nsIFrame* child = nsBox::GetChildBox(aBox);
    while(child) 
    {
      nsBoxFrame::LayoutChildAt(aState, child, nsRect(0,0,0,0));  
      child = nsBox::GetNextBox(child);
    }
    return NS_OK;
  }

  nsBoxLayoutState::AutoReflowDepth depth(aState);
  mozilla::AutoStackArena arena;

  
  const nsSize originalSize = aBox->GetSize();

  
  nsRect clientRect;
  aBox->GetClientRect(clientRect);

  
  
  
  
  
  
  
  nsRect originalClientRect(clientRect);

  
  
  nsFrameState frameState = nsFrameState(0);
  GetFrameState(aBox, frameState);

  
  nsBoxSize*         boxSizes = nullptr;
  nsComputedBoxSize* computedBoxSizes = nullptr;

  nscoord min = 0;
  nscoord max = 0;
  int32_t flexes = 0;
  PopulateBoxSizes(aBox, aState, boxSizes, min, max, flexes);
  
  
  
  
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

  
  bool finished;
  nscoord passes = 0;

  
  
  
  
  nscoord x = 0;
  nscoord y = 0;
  nscoord origX = 0;
  nscoord origY = 0;

  
  
  bool childResized = false;

  
  
  passes = 0;
  do 
  { 
#ifdef DEBUG_REFLOW
    if (passes > 0) {
      AddIndents();
      printf("ChildResized doing pass: %d\n", passes);
    }
#endif 

    
    
    finished = true;

    
    HandleBoxPack(aBox, frameState, x, y, originalClientRect, clientRect);

    
    
    origX = x;
    origY = y;

    
    
    
    nsComputedBoxSize* childComputedBoxSize = computedBoxSizes;
    nsBoxSize* childBoxSize                 = boxSizes;

    nsIFrame* child = nsBox::GetChildBox(aBox);

    int32_t count = 0;
    while (child || (childBoxSize && childBoxSize->bogus))
    { 
      
      
      if (childBoxSize == nullptr) {
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
           width = std::min(prefSize.width, originalClientRect.width);
           height = std::min(prefSize.height, originalClientRect.height);
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

      
      nscoord rectX = x;
      nscoord rectY = y;
      if (!(frameState & NS_STATE_IS_DIRECTION_NORMAL)) {
        if (frameState & NS_STATE_IS_HORIZONTAL)
          rectX -= width;
        else
          rectY -= height;
      }

      
      nsRect childRect(rectX, rectY, width, height);

      
      
      
      
      
      if (childRect.width > clientRect.width)
        clientRect.width = childRect.width;

      if (childRect.height > clientRect.height)
        clientRect.height = childRect.height;
    
      
      
      nscoord nextX = x;
      nscoord nextY = y;

      ComputeChildsNextPosition(aBox, x, y, nextX, nextY, childRect);

      
      
      
      if (frameState & NS_STATE_IS_HORIZONTAL) {
        if (frameState & NS_STATE_IS_DIRECTION_NORMAL)
          nextX += (childBoxSize->right);
        else
          nextX -= (childBoxSize->left);
        childRect.y = originalClientRect.y;
      }
      else {
        if (frameState & NS_STATE_IS_DIRECTION_NORMAL)
          nextY += (childBoxSize->right);
        else 
          nextY -= (childBoxSize->left);
        if (GetFrameDirection(aBox) == NS_STYLE_DIRECTION_LTR) {
          childRect.x = originalClientRect.x;
        } else {
          
          childRect.x = clientRect.x + originalClientRect.width - childRect.width;
        }
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

      bool layout = true;

      
      child->GetMargin(margin);
      childRect.Deflate(margin);
      if (childRect.width < 0)
        childRect.width = 0;
      if (childRect.height < 0)
        childRect.height = 0;

      
      
      if (passes > 0) {
        layout = false;
      } else {
        
        if (!NS_SUBTREE_DIRTY(child))
          layout = false;
      }

      nsRect oldRect(child->GetRect());

      
      
      if (!(frameState & NS_STATE_AUTO_STRETCH)) {
        if (frameState & NS_STATE_IS_HORIZONTAL) {
          childRect.y = oldRect.y;
        } else {
          childRect.x = oldRect.x;
        }
      }

      
      
      

      child->SetBounds(aState, childRect);
      bool sizeChanged = (childRect.width != oldRect.width ||
                            childRect.height != oldRect.height);

      if (sizeChanged) {
        
        
        nsSize minSize = child->GetMinSize(aState);
        nsSize maxSize = child->GetMaxSize(aState);
        maxSize = nsBox::BoundsCheckMinMax(minSize, maxSize);

        
        if (childRect.width > maxSize.width)
          childRect.width = maxSize.width;

        if (childRect.height > maxSize.height)
          childRect.height = maxSize.height;
           
        
        child->SetBounds(aState, childRect);
      }

      
      
      
      if (layout || sizeChanged)
        child->Layout(aState);
      
      
      
      nsRect newChildRect(child->GetRect());

      if (!newChildRect.IsEqualInterior(childRect)) {
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

        
        childResized = true;

        
        
        if (clientRect.width > originalClientRect.width)
          originalClientRect.width = clientRect.width;

        if (clientRect.height > originalClientRect.height)
          originalClientRect.height = clientRect.height;

        if (!(frameState & NS_STATE_IS_DIRECTION_NORMAL)) {
          
          
          
          
          
          
          if (frameState & NS_STATE_IS_HORIZONTAL)
            newChildRect.x = childRect.XMost() - newChildRect.width;
          else
            newChildRect.y = childRect.YMost() - newChildRect.height;
        }

        if (!(frameState & NS_STATE_IS_HORIZONTAL)) {
          if (GetFrameDirection(aBox) != NS_STYLE_DIRECTION_LTR) {
            
            newChildRect.x = childRect.XMost() - newChildRect.width;
          }
        }

        
        ComputeChildsNextPosition(aBox, x, y, nextX, nextY, newChildRect);

        if (newChildRect.width >= margin.left + margin.right && newChildRect.height >= margin.top + margin.bottom) 
          newChildRect.Deflate(margin);

        if (childRect.width >= margin.left + margin.right && childRect.height >= margin.top + margin.bottom) 
          childRect.Deflate(margin);
            
        child->SetBounds(aState, newChildRect);

        
        if (count == 0)
          finished = true;
      }

      
      x = nextX;
      y = nextY;
     
      
      childComputedBoxSize = childComputedBoxSize->next;
      childBoxSize = childBoxSize->next;

      child = nsBox::GetNextBox(child);
      count++;
    }

    
    passes++;
    NS_ASSERTION(passes < 10, "A Box's child is constantly growing!!!!!");
    if (passes > 10)
      break;
  } while (false == finished);

  
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
    nsIFrame* child = nsBox::GetChildBox(aBox);

    
    while (child) 
    {
      nsRect childRect(child->GetRect());
      childRect.x += (x - origX);
      childRect.y += (y - origY);
      child->SetBounds(aState, childRect);
      child = nsBox::GetNextBox(child);
    }
  }

  
  if (!(frameState & NS_STATE_AUTO_STRETCH)) {
    AlignChildren(aBox, aState);
  }
  
  
  
  return NS_OK;
}

void
nsSprocketLayout::PopulateBoxSizes(nsIFrame* aBox, nsBoxLayoutState& aState, nsBoxSize*& aBoxSizes, nscoord& aMinSize, nscoord& aMaxSize, int32_t& aFlexes)
{
  
  nscoord biggestPrefWidth = 0;
  nscoord biggestMinWidth = 0;
  nscoord smallestMaxWidth = NS_INTRINSICSIZE;

  nsFrameState frameState = nsFrameState(0);
  GetFrameState(aBox, frameState);

  
  

  aMinSize = 0;
  aMaxSize = NS_INTRINSICSIZE;

  bool isHorizontal;

  if (IsHorizontal(aBox))
     isHorizontal = true;
  else
     isHorizontal = false;

  
  
  
  
  
  
  
  

  
  nsIFrame* child = nsBox::GetChildBox(aBox);

  aFlexes = 0;
  nsBoxSize* currentBox = nullptr;

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
      currentBox->collapsed = child->IsCollapsed();
    } else {
      flex = start->flex;
      start = start->next;
    }
    
    if (flex > 0) 
       aFlexes++;
   
    child = GetNextBox(child);
  }
#endif

  
  child = nsBox::GetChildBox(aBox);
  currentBox = aBoxSizes;
  nsBoxSize* last = nullptr;

  nscoord maxFlex = 0;
  int32_t childCount = 0;

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
    bool collapsed = child->IsCollapsed();

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

    currentBox->collapsed = collapsed;
    aFlexes += currentBox->flex;

    child = nsBox::GetNextBox(child);

    last = currentBox;
    currentBox = currentBox->next;

  }

  if (childCount > 0) {
    nscoord maxAllowedFlex = nscoord_MAX / childCount;
  
    if (MOZ_UNLIKELY(maxFlex > maxAllowedFlex)) {
      
      currentBox = aBoxSizes;
      while (currentBox) {
        currentBox->flex = std::min(currentBox->flex, maxAllowedFlex);
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
    smallestMaxWidth = std::max(smallestMaxWidth, biggestMinWidth);
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
nsSprocketLayout::ComputeChildsNextPosition(nsIFrame* aBox, 
                                      const nscoord& aCurX, 
                                      const nscoord& aCurY, 
                                      nscoord& aNextX, 
                                      nscoord& aNextY, 
                                      const nsRect& aCurrentChildSize)
{
  
  
  nsFrameState frameState = nsFrameState(0);
  GetFrameState(aBox, frameState);

  if (IsHorizontal(aBox)) {
    
    if (frameState & NS_STATE_IS_DIRECTION_NORMAL)
      aNextX = aCurX + aCurrentChildSize.width;
    else
      aNextX = aCurX - aCurrentChildSize.width;

  } else {
    
    if (frameState & NS_STATE_IS_DIRECTION_NORMAL)
      aNextY = aCurY + aCurrentChildSize.height;
    else
      aNextY = aCurY - aCurrentChildSize.height;
  }
}

void
nsSprocketLayout::AlignChildren(nsIFrame* aBox,
                                nsBoxLayoutState& aState)
{
  nsFrameState frameState = nsFrameState(0);
  GetFrameState(aBox, frameState);
  bool isHorizontal = (frameState & NS_STATE_IS_HORIZONTAL) != 0;
  nsRect clientRect;
  aBox->GetClientRect(clientRect);

  NS_PRECONDITION(!(frameState & NS_STATE_AUTO_STRETCH),
                  "Only AlignChildren() with non-stretch alignment");

  
  nsIFrame::Halignment halign;
  nsIFrame::Valignment valign;
  nscoord maxAscent;
  bool isLTR;

  if (isHorizontal) {
    valign = aBox->GetVAlign();
    if (valign == nsBoxFrame::vAlign_BaseLine) {
      maxAscent = aBox->GetBoxAscent(aState);
    }
  } else {
    isLTR = GetFrameDirection(aBox) == NS_STYLE_DIRECTION_LTR;
    halign = aBox->GetHAlign();
  }

  nsIFrame* child = nsBox::GetChildBox(aBox);
  while (child) {

    nsMargin margin;
    child->GetMargin(margin);
    nsRect childRect = child->GetRect();

    if (isHorizontal) {
      const nscoord startAlign = clientRect.y + margin.top;
      const nscoord endAlign =
        clientRect.YMost() - margin.bottom - childRect.height;

      nscoord y;
      switch (valign) {
        case nsBoxFrame::vAlign_Top:
          y = startAlign;
          break;
        case nsBoxFrame::vAlign_Middle:
          
          
          y = (startAlign + endAlign) / 2;
          break;
        case nsBoxFrame::vAlign_Bottom:
          y = endAlign;
          break;
        case nsBoxFrame::vAlign_BaseLine:
          
          
          y = maxAscent - child->GetBoxAscent(aState);
          y = std::max(startAlign, y);
          y = std::min(y, endAlign);
          break;
      }

      childRect.y = y;

    } else { 
      const nscoord leftAlign = clientRect.x + margin.left;
      const nscoord rightAlign =
        clientRect.XMost() - margin.right - childRect.width;

      nscoord x;
      switch (halign) {
        case nsBoxFrame::hAlign_Left: 
          x = isLTR ? leftAlign : rightAlign;
          break;
        case nsBoxFrame::hAlign_Center:
          x = (leftAlign + rightAlign) / 2;
          break;
        case nsBoxFrame::hAlign_Right: 
          x = isLTR ? rightAlign : leftAlign;
          break;
      }

      childRect.x = x;
    }

    if (childRect.TopLeft() != child->GetPosition()) {
      child->SetBounds(aState, childRect);
    }

    child = nsBox::GetNextBox(child);
  }
}

void
nsSprocketLayout::ChildResized(nsIFrame* aBox,
                         nsBoxLayoutState& aState, 
                         nsIFrame* aChild,
                         nsBoxSize* aChildBoxSize,
                         nsComputedBoxSize* aChildComputedSize,
                         nsBoxSize* aBoxSizes, 
                         nsComputedBoxSize* aComputedBoxSizes, 
                         const nsRect& aChildLayoutRect, 
                         nsRect& aChildActualRect, 
                         nsRect& aContainingRect,
                         int32_t aFlexes, 
                         bool& aFinished)
                         
{
      nsRect childCurrentRect(aChildLayoutRect);

      bool isHorizontal = IsHorizontal(aBox);
      nscoord childLayoutWidth  = GET_WIDTH(aChildLayoutRect,isHorizontal);
      nscoord& childActualWidth  = GET_WIDTH(aChildActualRect,isHorizontal);
      nscoord& containingWidth   = GET_WIDTH(aContainingRect,isHorizontal);   
      
      
      nscoord& childActualHeight = GET_HEIGHT(aChildActualRect,isHorizontal);
      nscoord& containingHeight  = GET_HEIGHT(aContainingRect,isHorizontal);

      bool recompute = false;

      
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

              
              
              
              aFinished = false;

              
              if (aFlexes > 0) {
                
                recompute = true;
                InvalidateComputedSizes(aComputedBoxSizes);
                nsComputedBoxSize* node = aComputedBoxSizes;

                while(node) {
                  node->resized = false;
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
                aChildComputedSize->resized = true;

                while(node) {
                  if (node->resized)
                      node->valid = true;
                
                  node = node->next;
                }

                recompute = true;
                aFinished = false;
              } else {
                containingWidth += aChildComputedSize->size - childLayoutWidth;
              }              
            }
      }

      if (recompute)
            ComputeChildSizes(aBox, aState, containingWidth, aBoxSizes, aComputedBoxSizes);

      if (!childCurrentRect.IsEqualInterior(aChildActualRect)) {
        
        
        
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
      aComputedBoxSizes->valid = false;
      aComputedBoxSizes = aComputedBoxSizes->next;
  }
}

void
nsSprocketLayout::ComputeChildSizes(nsIFrame* aBox,
                           nsBoxLayoutState& aState, 
                           nscoord& aGivenSize, 
                           nsBoxSize* aBoxSizes, 
                           nsComputedBoxSize*& aComputedBoxSizes)
{  

  

  int32_t sizeRemaining            = aGivenSize;
  int32_t spacerConstantsRemaining = 0;

   

  if (!aComputedBoxSizes)
      aComputedBoxSizes = new (aState) nsComputedBoxSize();
  
  nsBoxSize*         boxSizes = aBoxSizes;
  nsComputedBoxSize* computedBoxSizes = aComputedBoxSizes;
  int32_t count = 0;
  int32_t validCount = 0;

  while (boxSizes) 
  {

    NS_ASSERTION((boxSizes->min <= boxSizes->pref && boxSizes->pref <= boxSizes->max),"bad pref, min, max size");

    
     
  
  
    
    
     
  
   
    
      if (computedBoxSizes->valid) { 
        sizeRemaining -= computedBoxSizes->size;
        validCount++;
      } else {
          if (boxSizes->flex == 0)
          {
            computedBoxSizes->valid = true;
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
    
    
    bool limit = true;
    for (int pass=1; true == limit; pass++) 
    {
      limit = false;
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
            int32_t newSize = pref + int32_t(int64_t(sizeRemaining) * flex / spacerConstantsRemaining);

            if (newSize<=min) {
              computedBoxSizes->size = min;
              computedBoxSizes->valid = true;
              spacerConstantsRemaining -= flex;
              sizeRemaining += pref;
              sizeRemaining -= min;
              limit = true;
            } else if (newSize>=max) {
              computedBoxSizes->size = max;
              computedBoxSizes->valid = true;
              spacerConstantsRemaining -= flex;
              sizeRemaining += pref;
              sizeRemaining -= max;
              limit = true;
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
        computedBoxSizes->size = pref + int32_t(int64_t(sizeRemaining) * flex / spacerConstantsRemaining);
        computedBoxSizes->valid = true;
      }

      aGivenSize += (boxSizes->left + boxSizes->right);
      aGivenSize += computedBoxSizes->size;

   

    boxSizes         = boxSizes->next;
    computedBoxSizes = computedBoxSizes->next;
  }
}


nsSize
nsSprocketLayout::GetPrefSize(nsIFrame* aBox, nsBoxLayoutState& aState)
{
   nsSize vpref (0, 0); 
   bool isHorizontal = IsHorizontal(aBox);

   nscoord biggestPref = 0;

   
   

   nsIFrame* child = nsBox::GetChildBox(aBox);
   nsFrameState frameState = nsFrameState(0);
   GetFrameState(aBox, frameState);
   bool isEqual = !!(frameState & NS_STATE_EQUAL_SIZE);
   int32_t count = 0;
   
   while (child) 
   {  
      
      if (!child->IsCollapsed())
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

      child = nsBox::GetNextBox(child);
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
nsSprocketLayout::GetMinSize(nsIFrame* aBox, nsBoxLayoutState& aState)
{
   nsSize minSize (0, 0);
   bool isHorizontal = IsHorizontal(aBox);

   nscoord biggestMin = 0;


   
   

   nsIFrame* child = nsBox::GetChildBox(aBox);
   nsFrameState frameState = nsFrameState(0);
   GetFrameState(aBox, frameState);
   bool isEqual = !!(frameState & NS_STATE_EQUAL_SIZE);
   int32_t count = 0;

   while (child) 
   {  
       
      if (!child->IsCollapsed())
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

      child = nsBox::GetNextBox(child);
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
nsSprocketLayout::GetMaxSize(nsIFrame* aBox, nsBoxLayoutState& aState)
{

  bool isHorizontal = IsHorizontal(aBox);

   nscoord smallestMax = NS_INTRINSICSIZE;
   nsSize maxSize (NS_INTRINSICSIZE, NS_INTRINSICSIZE);

   
   

   nsIFrame* child = nsBox::GetChildBox(aBox);
   nsFrameState frameState = nsFrameState(0);
   GetFrameState(aBox, frameState);
   bool isEqual = !!(frameState & NS_STATE_EQUAL_SIZE);
   int32_t count = 0;

   while (child) 
   {  
      
      if (!child->IsCollapsed())
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

      child = nsBox::GetNextBox(child);
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
nsSprocketLayout::GetAscent(nsIFrame* aBox, nsBoxLayoutState& aState)
{
   nscoord vAscent = 0;

   bool isHorizontal = IsHorizontal(aBox);

   
   
   
   nsIFrame* child = nsBox::GetChildBox(aBox);
   
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
      

      child = nsBox::GetNextBox(child);      
   }

   nsMargin borderPadding;
   aBox->GetBorderAndPadding(borderPadding);

   return vAscent + borderPadding.top;
}

void
nsSprocketLayout::SetLargestSize(nsSize& aSize1, const nsSize& aSize2, bool aIsHorizontal)
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
nsSprocketLayout::SetSmallestSize(nsSize& aSize1, const nsSize& aSize2, bool aIsHorizontal)
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
nsSprocketLayout::AddLargestSize(nsSize& aSize, const nsSize& aSizeToAdd, bool aIsHorizontal)
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
nsSprocketLayout::AddSmallestSize(nsSize& aSize, const nsSize& aSizeToAdd, bool aIsHorizontal)
{
  if (aIsHorizontal)
    AddCoord(aSize.width, aSizeToAdd.width);
  else
    AddCoord(aSize.height, aSizeToAdd.height);
    
  SetSmallestSize(aSize, aSizeToAdd, aIsHorizontal);
}

bool
nsSprocketLayout::GetDefaultFlex(int32_t& aFlex)
{
    aFlex = 0;
    return true;
}

nsComputedBoxSize::nsComputedBoxSize()
{
  resized = false;
  valid = false;
  size = 0;
  next = nullptr;
}

nsBoxSize::nsBoxSize()
{
  pref = 0;
  min = 0;
  max = NS_INTRINSICSIZE;
  collapsed = false;
  left = 0;
  right = 0;
  flex = 0;
  next = nullptr;
  bogus = false;
}


void* 
nsBoxSize::operator new(size_t sz, nsBoxLayoutState& aState) CPP_THROW_NEW
{
  return mozilla::AutoStackArena::Allocate(sz);
}


void 
nsBoxSize::operator delete(void* aPtr, size_t sz)
{
}


void* 
nsComputedBoxSize::operator new(size_t sz, nsBoxLayoutState& aState) CPP_THROW_NEW
{
   return mozilla::AutoStackArena::Allocate(sz);
}

void 
nsComputedBoxSize::operator delete(void* aPtr, size_t sz)
{
}
