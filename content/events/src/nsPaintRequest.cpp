





































#include "nsPaintRequest.h"

#include "nsDOMClassInfoID.h"
#include "nsClientRect.h"
#include "nsIFrame.h"

NS_INTERFACE_TABLE_HEAD(nsPaintRequest)
  NS_INTERFACE_TABLE1(nsPaintRequest, nsIDOMPaintRequest)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(PaintRequest)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsPaintRequest)
NS_IMPL_RELEASE(nsPaintRequest)

NS_IMETHODIMP
nsPaintRequest::GetClientRect(nsIDOMClientRect** aResult)
{
  nsRefPtr<nsClientRect> clientRect = new nsClientRect();
  if (!clientRect)
    return NS_ERROR_OUT_OF_MEMORY;
  clientRect->SetLayoutRect(mRequest.mRect);
  clientRect.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsPaintRequest::GetReason(nsAString& aResult)
{
  switch (mRequest.mFlags & nsIFrame::INVALIDATE_REASON_MASK) {
  case nsIFrame::INVALIDATE_REASON_SCROLL_BLIT:
    aResult.AssignLiteral("scroll copy");
    break;
  case nsIFrame::INVALIDATE_REASON_SCROLL_REPAINT:
    aResult.AssignLiteral("scroll repaint");
    break;
  default:
    aResult.Truncate();
    break;
  }
  return NS_OK;
}

NS_INTERFACE_TABLE_HEAD(nsPaintRequestList)
  NS_INTERFACE_TABLE1(nsPaintRequestList, nsIDOMPaintRequestList)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(PaintRequestList)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsPaintRequestList)
NS_IMPL_RELEASE(nsPaintRequestList)


NS_IMETHODIMP    
nsPaintRequestList::GetLength(PRUint32* aLength)
{
  *aLength = mArray.Count();
  return NS_OK;
}

NS_IMETHODIMP    
nsPaintRequestList::Item(PRUint32 aIndex, nsIDOMPaintRequest** aReturn)
{
  NS_IF_ADDREF(*aReturn = GetItemAt(aIndex));
  return NS_OK;
}
