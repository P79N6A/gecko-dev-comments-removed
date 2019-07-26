




#include "nsListBoxLayout.h"

#include "nsListBoxBodyFrame.h"
#include "nsBox.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableFrame.h"
#include "nsIReflowCallback.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"

nsListBoxLayout::nsListBoxLayout() : nsGridRowGroupLayout()
{
}



nsSize
nsListBoxLayout::GetPrefSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState)
{
  nsSize pref = nsGridRowGroupLayout::GetPrefSize(aBox, aBoxLayoutState);

  nsListBoxBodyFrame* frame = static_cast<nsListBoxBodyFrame*>(aBox);
  if (frame) {
    nscoord rowheight = frame->GetRowHeightAppUnits();
    pref.height = frame->GetRowCount() * rowheight;
    
    nscoord y = frame->GetAvailableHeight();
    if (pref.height > y && y > 0 && rowheight > 0) {
      nscoord m = (pref.height-y)%rowheight;
      nscoord remainder = m == 0 ? 0 : rowheight - m;
      pref.height += remainder;
    }
    if (nsContentUtils::HasNonEmptyAttr(frame->GetContent(), kNameSpaceID_None,
                                        nsGkAtoms::sizemode)) {
      nscoord width = frame->ComputeIntrinsicWidth(aBoxLayoutState);
      if (width > pref.width)
        pref.width = width;
    }
  }
  return pref;
}

nsSize
nsListBoxLayout::GetMinSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState)
{
  nsSize minSize = nsGridRowGroupLayout::GetMinSize(aBox, aBoxLayoutState);

  nsListBoxBodyFrame* frame = static_cast<nsListBoxBodyFrame*>(aBox);
  if (frame) {
    nscoord rowheight = frame->GetRowHeightAppUnits();
    minSize.height = frame->GetRowCount() * rowheight;
    
    nscoord y = frame->GetAvailableHeight();
    if (minSize.height > y && y > 0 && rowheight > 0) {
      nscoord m = (minSize.height-y)%rowheight;
      nscoord remainder = m == 0 ? 0 : rowheight - m;
      minSize.height += remainder;
    }
    if (nsContentUtils::HasNonEmptyAttr(frame->GetContent(), kNameSpaceID_None,
                                        nsGkAtoms::sizemode)) {
      nscoord width = frame->ComputeIntrinsicWidth(aBoxLayoutState);
      if (width > minSize.width)
        minSize.width = width;
    }
  }
  return minSize;
}

nsSize
nsListBoxLayout::GetMaxSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState)
{
  nsSize maxSize = nsGridRowGroupLayout::GetMaxSize(aBox, aBoxLayoutState);

  nsListBoxBodyFrame* frame = static_cast<nsListBoxBodyFrame*>(aBox);
  if (frame) {
    nscoord rowheight = frame->GetRowHeightAppUnits();
    maxSize.height = frame->GetRowCount() * rowheight;
    
    nscoord y = frame->GetAvailableHeight();
    if (maxSize.height > y && y > 0 && rowheight > 0) {
      nscoord m = (maxSize.height-y)%rowheight;
      nscoord remainder = m == 0 ? 0 : rowheight - m;
      maxSize.height += remainder;
    }
  }
  return maxSize;
}

NS_IMETHODIMP
nsListBoxLayout::Layout(nsIFrame* aBox, nsBoxLayoutState& aState)
{
  return LayoutInternal(aBox, aState);
}







NS_IMETHODIMP
nsListBoxLayout::LayoutInternal(nsIFrame* aBox, nsBoxLayoutState& aState)
{
  int32_t redrawStart = -1;

  
  nsListBoxBodyFrame* body = static_cast<nsListBoxBodyFrame*>(aBox);
  if (!body) {
    NS_ERROR("Frame encountered that isn't a listboxbody!");
    return NS_ERROR_FAILURE;
  }

  nsMargin margin;

  
  nsRect clientRect;
  aBox->GetClientRect(clientRect);

  
  
  nscoord availableHeight = body->GetAvailableHeight();
  nscoord yOffset = body->GetYPosition();
  
  if (availableHeight <= 0) {
    bool fixed = (body->GetFixedRowSize() != -1);
    if (fixed)
      availableHeight = 10;
    else
      return NS_OK;
  }

  
  nsIFrame* box = body->GetChildBox();

  
  nscoord rowHeight = body->GetRowHeightAppUnits();

  while (box) {
    
    
    nsRect childRect(box->GetRect());
    box->GetMargin(margin);
    
    
    
    
    if (NS_SUBTREE_DIRTY(box) || childRect.width < clientRect.width) {
      childRect.x = 0;
      childRect.y = yOffset;
      childRect.width = clientRect.width;
      
      nsSize size = box->GetPrefSize(aState);
      body->SetRowHeight(size.height);
      
      childRect.height = rowHeight;

      childRect.Deflate(margin);
      box->SetBounds(aState, childRect);
      box->Layout(aState);
    } else {
      
      
      int32_t newPos = yOffset+margin.top;

      
      
      
      if (redrawStart == -1 && childRect.y != newPos)
        redrawStart = newPos;

      childRect.y = newPos;
      box->SetBounds(aState, childRect);
    }

    
    
    nscoord size = childRect.height + margin.top + margin.bottom;

    yOffset += size;
    availableHeight -= size;
    
    box = box->GetNextBox();
  }
  
  
  
  
  body->PostReflowCallback();
    
  
  
  
  if (redrawStart > -1) {
    aBox->Redraw(aState);
  }

  return NS_OK;
}



already_AddRefed<nsBoxLayout> NS_NewListBoxLayout()
{
  nsRefPtr<nsBoxLayout> layout = new nsListBoxLayout();
  return layout.forget();
} 
