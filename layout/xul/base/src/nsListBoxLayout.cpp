






































#include "nsListBoxLayout.h"

#include "nsListBoxBodyFrame.h"
#include "nsIFrame.h"
#include "nsBox.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableFrame.h"
#include "nsIReflowCallback.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"

nsListBoxLayout::nsListBoxLayout(nsIPresShell* aPresShell)
  : nsGridRowGroupLayout(aPresShell)
{
}



NS_IMETHODIMP
nsListBoxLayout::GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)
{
  nsresult rv = nsGridRowGroupLayout::GetPrefSize(aBox, aBoxLayoutState, aSize);

  nsListBoxBodyFrame* frame = NS_STATIC_CAST(nsListBoxBodyFrame*, aBox);
  if (frame) {
    nscoord rowheight = frame->GetRowHeightAppUnits();
    aSize.height = frame->GetRowCount() * rowheight;
    
    nscoord y = frame->GetAvailableHeight();
    if (aSize.height > y && y > 0 && rowheight > 0) {
      nscoord m = (aSize.height-y)%rowheight;
      nscoord remainder = m == 0 ? 0 : rowheight - m;
      aSize.height += remainder;
    }
    if (nsContentUtils::HasNonEmptyAttr(frame->GetContent(), kNameSpaceID_None,
                                        nsGkAtoms::sizemode)) {
      nscoord width = frame->ComputeIntrinsicWidth(aBoxLayoutState);
      if (width > aSize.width)
        aSize.width = width;
    }
  }
  return rv;
}

NS_IMETHODIMP
nsListBoxLayout::GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)
{
  nsresult rv = nsGridRowGroupLayout::GetMinSize(aBox, aBoxLayoutState, aSize);

  nsListBoxBodyFrame* frame = NS_STATIC_CAST(nsListBoxBodyFrame*, aBox);
  if (frame) {
    nscoord rowheight = frame->GetRowHeightAppUnits();
    aSize.height = frame->GetRowCount() * rowheight;
    
    nscoord y = frame->GetAvailableHeight();
    if (aSize.height > y && y > 0 && rowheight > 0) {
      nscoord m = (aSize.height-y)%rowheight;
      nscoord remainder = m == 0 ? 0 : rowheight - m;
      aSize.height += remainder;
    }
    if (nsContentUtils::HasNonEmptyAttr(frame->GetContent(), kNameSpaceID_None,
                                        nsGkAtoms::sizemode)) {
      nscoord width = frame->ComputeIntrinsicWidth(aBoxLayoutState);
      if (width > aSize.width)
        aSize.width = width;
    }
  }
  return rv;
}

NS_IMETHODIMP
nsListBoxLayout::GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)
{
  nsresult rv = nsGridRowGroupLayout::GetMaxSize(aBox, aBoxLayoutState, aSize);

  nsListBoxBodyFrame* frame = NS_STATIC_CAST(nsListBoxBodyFrame*, aBox);
  if (frame) {
    nscoord rowheight = frame->GetRowHeightAppUnits();
    aSize.height = frame->GetRowCount() * rowheight;
    
    nscoord y = frame->GetAvailableHeight();
    if (aSize.height > y && y > 0 && rowheight > 0) {
      nscoord m = (aSize.height-y)%rowheight;
      nscoord remainder = m == 0 ? 0 : rowheight - m;
      aSize.height += remainder;
    }
  }
  return rv;
}

NS_IMETHODIMP
nsListBoxLayout::Layout(nsIBox* aBox, nsBoxLayoutState& aState)
{
  nsListBoxBodyFrame* frame = NS_STATIC_CAST(nsListBoxBodyFrame*, aBox);

  
  
  
  PRInt32 index;
  frame->GetIndexOfFirstVisibleRow(&index);
  if (index > 0) {
    nscoord pos = frame->GetYPosition();
    PRInt32 rowHeight = frame->GetRowHeightAppUnits();
    if (pos != (rowHeight*index)) {
      frame->VerticalScroll(rowHeight*index);
      frame->Redraw(aState, nsnull, PR_FALSE);
    }
  }

  nsresult rv = LayoutInternal(aBox, aState);
  if (NS_FAILED(rv)) return rv;

  return NS_OK;
}







NS_IMETHODIMP
nsListBoxLayout::LayoutInternal(nsIBox* aBox, nsBoxLayoutState& aState)
{
  PRInt32 redrawStart = -1;

  
  nsListBoxBodyFrame* body = NS_STATIC_CAST(nsListBoxBodyFrame*, aBox);
  if (!body) {
    NS_ERROR("Frame encountered that isn't a listboxbody!\n");
    return NS_ERROR_FAILURE;
  }

  nsMargin margin;

  
  nsRect clientRect;
  aBox->GetClientRect(clientRect);

  
  
  nscoord availableHeight = body->GetAvailableHeight();
  nscoord yOffset = body->GetYPosition();
  
  if (availableHeight <= 0) {
    PRBool fixed = (body->GetFixedRowSize() != -1);
    if (fixed)
      availableHeight = 10;
    else
      return NS_OK;
  }

  
  nsIBox* box = body->GetChildBox();

  
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
      
      
      PRInt32 newPos = yOffset+margin.top;

      
      
      
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
    nsRect bounds(aBox->GetRect());
    nsRect tempRect(0,redrawStart,bounds.width, bounds.height - redrawStart);
    aBox->Redraw(aState, &tempRect);
  }

  return NS_OK;
}



nsresult
NS_NewListBoxLayout( nsIPresShell* aPresShell, nsCOMPtr<nsIBoxLayout>& aNewLayout)
{
  aNewLayout = new nsListBoxLayout(aPresShell);

  return NS_OK;
} 
