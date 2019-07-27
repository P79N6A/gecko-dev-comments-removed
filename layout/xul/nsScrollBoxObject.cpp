




#include "nsCOMPtr.h"
#include "nsIScrollBoxObject.h"
#include "nsBoxObject.h"
#include "nsIPresShell.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsPresContext.h"
#include "nsBox.h"
#include "nsIScrollableFrame.h"

using namespace mozilla;

class nsScrollBoxObject MOZ_FINAL : public nsIScrollBoxObject,
                                    public nsBoxObject
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSISCROLLBOXOBJECT

  nsScrollBoxObject();

  virtual nsIScrollableFrame* GetScrollFrame() {
    return do_QueryFrame(GetFrame(false));
  }

protected:
  virtual ~nsScrollBoxObject();
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


NS_IMETHODIMP nsScrollBoxObject::ScrollTo(int32_t x, int32_t y)
{
  nsIScrollableFrame* sf = GetScrollFrame();
  if (!sf)
    return NS_ERROR_FAILURE;
  sf->ScrollToCSSPixels(CSSIntPoint(x, y));
  return NS_OK;
}


NS_IMETHODIMP nsScrollBoxObject::ScrollBy(int32_t dx, int32_t dy)
{
  int32_t x, y;
  nsresult rv = GetPosition(&x, &y);
  if (NS_FAILED(rv))
    return rv;

  return ScrollTo(x + dx, y + dy);
}


NS_IMETHODIMP nsScrollBoxObject::ScrollByLine(int32_t dlines)
{
  nsIScrollableFrame* sf = GetScrollFrame();
  if (!sf)
    return NS_ERROR_FAILURE;

  sf->ScrollBy(nsIntPoint(0, dlines), nsIScrollableFrame::LINES,
               nsIScrollableFrame::SMOOTH);
  return NS_OK;
}







static nsIFrame* GetScrolledBox(nsBoxObject* aScrollBox) {
  nsIFrame* frame = aScrollBox->GetFrame(false);
  if (!frame) 
    return nullptr;
  nsIScrollableFrame* scrollFrame = do_QueryFrame(frame);
  if (!scrollFrame) {
    NS_WARNING("nsIScrollBoxObject attached to something that's not a scroll frame!");
    return nullptr;
  }
  nsIFrame* scrolledFrame = scrollFrame->GetScrolledFrame();
  if (!scrolledFrame)
    return nullptr;
  return nsBox::GetChildBox(scrolledFrame);
}


NS_IMETHODIMP nsScrollBoxObject::ScrollByIndex(int32_t dindexes)
{
    nsIScrollableFrame* sf = GetScrollFrame();
    if (!sf)
       return NS_ERROR_FAILURE;
    nsIFrame* scrolledBox = GetScrolledBox(this);
    if (!scrolledBox)
       return NS_ERROR_FAILURE;

    nsRect rect;

    
    nsIFrame* child = nsBox::GetChildBox(scrolledBox);

    bool horiz = scrolledBox->IsHorizontal();
    nsPoint cp = sf->GetScrollPosition();
    nscoord diff = 0;
    int32_t curIndex = 0;
    bool isLTR = scrolledBox->IsNormalDirection();

    int32_t frameWidth = 0;
    if (!isLTR && horiz) {
      GetWidth(&frameWidth);
      nsCOMPtr<nsIPresShell> shell = GetPresShell(false);
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
      child = nsBox::GetNextBox(child);
      curIndex++;
    }

    int32_t count = 0;

    if (dindexes == 0)
       return NS_OK;

    if (dindexes > 0) {
      while(child) {
        child = nsBox::GetNextBox(child);
        if (child)
          rect = child->GetRect();
        count++;
        if (count >= dindexes)
          break;
      }

   } else if (dindexes < 0) {
      child = nsBox::GetChildBox(scrolledBox);
      while(child) {
        rect = child->GetRect();
        if (count >= curIndex + dindexes)
          break;

        count++;
        child = nsBox::GetNextBox(child);

      }
   }

   nscoord csspixel = nsPresContext::CSSPixelsToAppUnits(1);
   if (horiz) {
       
       
       
       

       nsPoint pt(isLTR ? rect.x : rect.x + rect.width - frameWidth,
                  cp.y);

       
       
       
       nsRect range(pt.x, pt.y, csspixel, 0);
       if (isLTR) {
         range.x -= csspixel;
       }
       sf->ScrollTo(pt, nsIScrollableFrame::INSTANT, &range);
   } else {
       
       nsRect range(cp.x, rect.y - csspixel, 0, csspixel);
       sf->ScrollTo(nsPoint(cp.x, rect.y), nsIScrollableFrame::INSTANT, &range);
   }

   return NS_OK;
}


NS_IMETHODIMP nsScrollBoxObject::ScrollToLine(int32_t line)
{
  nsIScrollableFrame* sf = GetScrollFrame();
  if (!sf)
     return NS_ERROR_FAILURE;
  
  nscoord y = sf->GetLineScrollAmount().height * line;
  nsRect range(0, y - nsPresContext::CSSPixelsToAppUnits(1),
               0, nsPresContext::CSSPixelsToAppUnits(1));
  sf->ScrollTo(nsPoint(0, y), nsIScrollableFrame::INSTANT, &range);
  return NS_OK;
}


NS_IMETHODIMP nsScrollBoxObject::ScrollToElement(nsIDOMElement *child)
{
    NS_ENSURE_ARG_POINTER(child);

    nsCOMPtr<nsIPresShell> shell = GetPresShell(false);
    if (!shell) {
      return NS_ERROR_UNEXPECTED;
    }

    nsCOMPtr<nsIContent> content = do_QueryInterface(child);
    shell->ScrollContentIntoView(content,
                                 nsIPresShell::ScrollAxis(
                                   nsIPresShell::SCROLL_TOP,
                                   nsIPresShell::SCROLL_ALWAYS),
                                 nsIPresShell::ScrollAxis(
                                   nsIPresShell::SCROLL_LEFT,
                                   nsIPresShell::SCROLL_ALWAYS),
                                 nsIPresShell::SCROLL_FIRST_ANCESTOR_ONLY |
                                 nsIPresShell::SCROLL_OVERFLOW_HIDDEN);
    return NS_OK;
}


NS_IMETHODIMP nsScrollBoxObject::ScrollToIndex(int32_t index)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsScrollBoxObject::GetPosition(int32_t *x, int32_t *y)
{
  nsIScrollableFrame* sf = GetScrollFrame();
  if (!sf)
     return NS_ERROR_FAILURE;

  CSSIntPoint pt = sf->GetScrollPositionCSSPixels();
  *x = pt.x;
  *y = pt.y;

  return NS_OK;  
}


NS_IMETHODIMP nsScrollBoxObject::GetScrolledSize(int32_t *width, int32_t *height)
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

    nsCOMPtr<nsIPresShell> shell = GetPresShell(false);
    if (!shell) {
      return NS_ERROR_UNEXPECTED;
    }

    nsCOMPtr<nsIContent> content = do_QueryInterface(child);
    shell->ScrollContentIntoView(content,
                                 nsIPresShell::ScrollAxis(),
                                 nsIPresShell::ScrollAxis(),
                                 nsIPresShell::SCROLL_FIRST_ANCESTOR_ONLY |
                                 nsIPresShell::SCROLL_OVERFLOW_HIDDEN);
    return NS_OK;
}


NS_IMETHODIMP nsScrollBoxObject::EnsureIndexIsVisible(int32_t index)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsScrollBoxObject::EnsureLineIsVisible(int32_t line)
{
    return NS_ERROR_NOT_IMPLEMENTED;
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

