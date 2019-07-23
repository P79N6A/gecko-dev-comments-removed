





































#include "nsHTMLAreaAccessible.h"
#include "nsIServiceManager.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIFrame.h"
#include "nsIImageFrame.h"
#include "nsIImageMap.h"






nsHTMLAreaAccessible::
  nsHTMLAreaAccessible(nsIDOMNode *aDomNode, nsIAccessible *aParent,
                       nsIWeakReference* aShell):
  nsHTMLLinkAccessible(aDomNode, aShell)
{
}




nsresult
nsHTMLAreaAccessible::GetNameInternal(nsAString & aName)
{
  nsresult rv = nsAccessible::GetNameInternal(aName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aName.IsEmpty())
    return NS_OK;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (!content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::alt,
                        aName)) {
    return GetValue(aName);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAreaAccessible::GetDescription(nsAString& aDescription)
{
  aDescription.Truncate();

  
  nsCOMPtr<nsIDOMHTMLAreaElement> area(do_QueryInterface(mDOMNode));
  if (area) 
    area->GetShape(aDescription);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAreaAccessible::GetBounds(PRInt32 *x, PRInt32 *y,
                                PRInt32 *width, PRInt32 *height)
{
  nsresult rv;

  

  *x = *y = *width = *height = 0;

  nsPresContext *presContext = GetPresContext();
  NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

  nsCOMPtr<nsIContent> ourContent(do_QueryInterface(mDOMNode));
  NS_ENSURE_TRUE(ourContent, NS_ERROR_FAILURE);

  nsIFrame *frame = GetFrame();
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);
  nsIImageFrame *imageFrame = do_QueryFrame(frame);

  nsCOMPtr<nsIImageMap> map;
  imageFrame->GetImageMap(presContext, getter_AddRefs(map));
  NS_ENSURE_TRUE(map, NS_ERROR_FAILURE);

  nsRect rect;
  nsIntRect orgRectPixels;
  rv = map->GetBoundsForAreaContent(ourContent, presContext, rect);
  NS_ENSURE_SUCCESS(rv, rv);

  *x      = presContext->AppUnitsToDevPixels(rect.x); 
  *y      = presContext->AppUnitsToDevPixels(rect.y); 

  
  
  *width  = presContext->AppUnitsToDevPixels(rect.width - rect.x);
  *height = presContext->AppUnitsToDevPixels(rect.height - rect.y);

  
  orgRectPixels = frame->GetScreenRectExternal();
  *x += orgRectPixels.x;
  *y += orgRectPixels.y;

  return NS_OK;
}




nsresult
nsHTMLAreaAccessible::GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                      PRBool aDeepestChild,
                                      nsIAccessible **aChild)
{
  
  NS_ADDREF(*aChild = this);
  return NS_OK;
}




void
nsHTMLAreaAccessible::CacheChildren()
{
  
  mAccChildCount = IsDefunct() ? eChildCountUninitialized : 0;
}
