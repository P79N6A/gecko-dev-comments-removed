




































#include "nsCOMPtr.h"
#include "nsIScrollBoxObject.h"
#include "nsBoxObject.h"
#include "nsIPresShell.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMElement.h"
#include "nsPresContext.h"
#include "nsIFrame.h"
#include "nsIScrollableView.h"
#include "nsIScrollableFrame.h"


class nsScrollBoxObject : public nsIScrollBoxObject, public nsBoxObject
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSISCROLLBOXOBJECT

  nsScrollBoxObject();
  virtual ~nsScrollBoxObject();

  virtual nsIScrollableView* GetScrollableView();

  
};



NS_INTERFACE_MAP_BEGIN(nsScrollBoxObject)
  NS_INTERFACE_MAP_ENTRY(nsIScrollBoxObject)
NS_INTERFACE_MAP_END_INHERITING(nsBoxObject)

NS_IMPL_ADDREF_INHERITED(nsScrollBoxObject, nsBoxObject)
NS_IMPL_RELEASE_INHERITED(nsScrollBoxObject, nsBoxObject)

nsScrollBoxObject::nsScrollBoxObject()
{
  
}

nsScrollBoxObject::~nsScrollBoxObject()
{
  
}


NS_IMETHODIMP nsScrollBoxObject::ScrollTo(PRInt32 x, PRInt32 y)
{
  nsIScrollableView* scrollableView = GetScrollableView();
  if (!scrollableView)
    return NS_ERROR_FAILURE;

  return scrollableView->ScrollTo(nsPresContext::CSSPixelsToAppUnits(x),
                                  nsPresContext::CSSPixelsToAppUnits(y),
                                  NS_SCROLL_PROPERTY_ALWAYS_BLIT);
}


NS_IMETHODIMP nsScrollBoxObject::ScrollBy(PRInt32 dx, PRInt32 dy)
{
  PRInt32 x, y;
  nsresult rv = GetPosition(&x, &y);
  if (NS_FAILED(rv))
    return rv;

  return ScrollTo(x + dx, y + dy);
}


NS_IMETHODIMP nsScrollBoxObject::ScrollByLine(PRInt32 dlines)
{
  nsIScrollableView* scrollableView = GetScrollableView();
  if (!scrollableView)
    return NS_ERROR_FAILURE;

  return scrollableView->ScrollByLines(0, dlines);
}







static nsIFrame* GetScrolledBox(nsBoxObject* aScrollBox) {
  nsIFrame* frame = aScrollBox->GetFrame(PR_FALSE);
  if (!frame) 
    return nsnull;
  nsIScrollableFrame* scrollFrame;
  if (NS_FAILED(CallQueryInterface(frame, &scrollFrame))) {
    NS_WARNING("nsIScrollBoxObject attached to something that's not a scroll frame!");
    return nsnull;
  }
  nsIFrame* scrolledFrame = scrollFrame->GetScrolledFrame();
  if (!scrolledFrame)
    return nsnull;
  return scrolledFrame->GetChildBox();
}


NS_IMETHODIMP nsScrollBoxObject::ScrollByIndex(PRInt32 dindexes)
{
    nsIScrollableView* scrollableView = GetScrollableView();
    if (!scrollableView)
       return NS_ERROR_FAILURE;
    nsIFrame* scrolledBox = GetScrolledBox(this);
    if (!scrolledBox)
       return NS_ERROR_FAILURE;

    nsRect rect;

    
    nsIFrame* child = scrolledBox->GetChildBox();

    PRBool horiz = scrolledBox->IsHorizontal();
    nsPoint cp;
    scrollableView->GetScrollPosition(cp.x,cp.y);
    nscoord diff = 0;
    PRInt32 curIndex = 0;
    PRBool isLTR = scrolledBox->IsNormalDirection();

    PRInt32 frameWidth = 0;
    if (!isLTR && horiz) {
      GetWidth(&frameWidth);
      nsCOMPtr<nsIPresShell> shell = GetPresShell(PR_FALSE);
      if (!shell) {
        return NS_ERROR_UNEXPECTED;
      }
      frameWidth = nsPresContext::CSSPixelsToAppUnits(frameWidth);
    }

    
    while(child) {
      rect = child->GetRect();
      if (horiz) {
        
        
        
        
        
        
        diff = rect.x + rect.width/2; 
        if ((isLTR && diff > cp.x) ||
            (!isLTR && diff < cp.x + frameWidth)) {
          break;
        }
      } else {
        diff = rect.y + rect.height/2;
        if (diff > cp.y) {
          break;
        }
      }
      child = child->GetNextBox();
      curIndex++;
    }

    PRInt32 count = 0;

    if (dindexes == 0)
       return NS_OK;

    if (dindexes > 0) {
      while(child) {
        child = child->GetNextBox();
        if (child)
          rect = child->GetRect();
        count++;
        if (count >= dindexes)
          break;
      }

   } else if (dindexes < 0) {
      child = scrolledBox->GetChildBox();
      while(child) {
        rect = child->GetRect();
        if (count >= curIndex + dindexes)
          break;

        count++;
        child = child->GetNextBox();

      }
   }

   if (horiz)
       
       
       
       
       return scrollableView->ScrollTo((isLTR) ? rect.x :
                                       rect.x + rect.width - frameWidth, 
                                       cp.y, NS_SCROLL_PROPERTY_ALWAYS_BLIT);
   else
       return scrollableView->ScrollTo(cp.x, rect.y, NS_SCROLL_PROPERTY_ALWAYS_BLIT);
}


NS_IMETHODIMP nsScrollBoxObject::ScrollToLine(PRInt32 line)
{
  nsIScrollableView* scrollableView = GetScrollableView();
  if (!scrollableView)
    return NS_ERROR_FAILURE;
  
  nscoord height = 0;
  scrollableView->GetLineHeight(&height);
  scrollableView->ScrollTo(0,height*line, NS_SCROLL_PROPERTY_ALWAYS_BLIT);

  return NS_OK;
}


NS_IMETHODIMP nsScrollBoxObject::ScrollToElement(nsIDOMElement *child)
{
    NS_ENSURE_ARG_POINTER(child);
    nsIScrollableView* scrollableView = GetScrollableView();
    if (!scrollableView)
       return NS_ERROR_FAILURE;

    nsCOMPtr<nsIPresShell> shell = GetPresShell(PR_FALSE);
    if (!shell) {
      return NS_ERROR_UNEXPECTED;
    }

    nsIFrame* scrolledBox = GetScrolledBox(this);
    if (!scrolledBox)
       return NS_ERROR_FAILURE;

    nsRect rect, crect;
    nsCOMPtr<nsIDOMDocument> doc;
    child->GetOwnerDocument(getter_AddRefs(doc));
    nsCOMPtr<nsIDOMNSDocument> nsDoc(do_QueryInterface(doc));
    if(!nsDoc)
      return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIBoxObject> childBoxObject;
    nsDoc->GetBoxObjectFor(child, getter_AddRefs(childBoxObject));
    if(!childBoxObject)
      return NS_ERROR_UNEXPECTED;

    PRInt32 x,y;
    childBoxObject->GetX(&x);
    childBoxObject->GetY(&y);
    
    rect.x = nsPresContext::CSSPixelsToAppUnits(x);
    rect.y = nsPresContext::CSSPixelsToAppUnits(y);

    

    
    nsPoint cp;
    scrollableView->GetScrollPosition(cp.x,cp.y);

    GetOffsetRect(crect);    
    crect.x = nsPresContext::CSSPixelsToAppUnits(crect.x);
    crect.y = nsPresContext::CSSPixelsToAppUnits(crect.y);
    nscoord newx=cp.x, newy=cp.y;

    
    
    if (scrolledBox->IsHorizontal()) {
        newx = rect.x - crect.x;
    } else {
        newy = rect.y - crect.y;
    }
    
    return scrollableView->ScrollTo(newx, newy, NS_SCROLL_PROPERTY_ALWAYS_BLIT);
}


NS_IMETHODIMP nsScrollBoxObject::ScrollToIndex(PRInt32 index)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsScrollBoxObject::GetPosition(PRInt32 *x, PRInt32 *y)
{
  nsIScrollableView* scrollableView = GetScrollableView();
  if (!scrollableView)
    return NS_ERROR_FAILURE;

  nscoord xc, yc;
  nsresult rv = scrollableView->GetScrollPosition(xc, yc);
  if (NS_FAILED(rv))
    return rv;

  *x = nsPresContext::AppUnitsToIntCSSPixels(xc);
  *y = nsPresContext::AppUnitsToIntCSSPixels(yc);

  return NS_OK;  
}


NS_IMETHODIMP nsScrollBoxObject::GetScrolledSize(PRInt32 *width, PRInt32 *height)
{
    nsIFrame* scrolledBox = GetScrolledBox(this);
    if (!scrolledBox)
        return NS_ERROR_FAILURE;
        	
    nsRect scrollRect = scrolledBox->GetRect();

    *width  = nsPresContext::AppUnitsToIntCSSPixels(scrollRect.width);
    *height = nsPresContext::AppUnitsToIntCSSPixels(scrollRect.height);

    return NS_OK;
}


NS_IMETHODIMP nsScrollBoxObject::EnsureElementIsVisible(nsIDOMElement *child)
{
    NS_ENSURE_ARG_POINTER(child);

    
    
    nsCOMPtr<nsIDOMDocument> doc;
    
    child->GetOwnerDocument(getter_AddRefs(doc));
    nsCOMPtr<nsIDOMNSDocument> nsDoc(do_QueryInterface(doc));
    if(!nsDoc)
        return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIBoxObject> childBoxObject;
    nsDoc->GetBoxObjectFor(child, getter_AddRefs(childBoxObject));
    if(!childBoxObject)
      return NS_ERROR_UNEXPECTED;

    PRInt32 x, y, width, height;
    childBoxObject->GetX(&x);
    childBoxObject->GetY(&y);
    childBoxObject->GetWidth(&width);
    childBoxObject->GetHeight(&height);

    nsIScrollableView* scrollableView = GetScrollableView();
    if (!scrollableView)
       return NS_ERROR_FAILURE;

    nsIFrame* scrolledBox = GetScrolledBox(this);
    if (!scrolledBox)
       return NS_ERROR_FAILURE;

    nsRect rect, crect;
    
    rect.x = nsPresContext::CSSPixelsToAppUnits(x);
    rect.y = nsPresContext::CSSPixelsToAppUnits(y);
    rect.width = nsPresContext::CSSPixelsToAppUnits(width);
    rect.height = nsPresContext::CSSPixelsToAppUnits(height);

    

    
    nsPoint cp;
    scrollableView->GetScrollPosition(cp.x,cp.y);
    GetOffsetRect(crect);    
    crect.x = nsPresContext::CSSPixelsToAppUnits(crect.x);
    crect.y = nsPresContext::CSSPixelsToAppUnits(crect.y);
    crect.width = nsPresContext::CSSPixelsToAppUnits(crect.width);
    crect.height = nsPresContext::CSSPixelsToAppUnits(crect.height);

    nscoord newx=cp.x, newy=cp.y;

    
    if (scrolledBox->IsHorizontal()) {
        if ((rect.x - crect.x) + rect.width > cp.x + crect.width) {
            newx = cp.x + (((rect.x - crect.x) + rect.width)-(cp.x + crect.width));
        } else if (rect.x - crect.x < cp.x) {
            newx = rect.x - crect.x;
        }
    } else {
        if ((rect.y - crect.y) + rect.height > cp.y + crect.height) {
            newy = cp.y + (((rect.y - crect.y) + rect.height)-(cp.y + crect.height));
        } else if (rect.y - crect.y < cp.y) {
            newy = rect.y - crect.y;
        }
    }
    
    
    return scrollableView->ScrollTo(newx, newy, NS_SCROLL_PROPERTY_ALWAYS_BLIT);
}


NS_IMETHODIMP nsScrollBoxObject::EnsureIndexIsVisible(PRInt32 index)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsScrollBoxObject::EnsureLineIsVisible(PRInt32 line)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsIScrollableView* 
nsScrollBoxObject::GetScrollableView()
{
  
  nsIFrame* frame = GetFrame(PR_FALSE);
  if (!frame) 
    return nsnull;
  
  nsIScrollableFrame* scrollFrame;
  if (NS_FAILED(CallQueryInterface(frame, &scrollFrame)))
    return nsnull;

  nsIScrollableView* scrollingView = scrollFrame->GetScrollableView();
  if (!scrollingView)
    return nsnull;

  return scrollingView;
}

nsresult
NS_NewScrollBoxObject(nsIBoxObject** aResult)
{
  *aResult = new nsScrollBoxObject;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}

