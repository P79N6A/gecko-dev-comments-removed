






































#include "nsHTMLImageMapAccessible.h"

#include "nsAccUtils.h"

#include "nsIDOMHTMLCollection.h"
#include "nsIServiceManager.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIFrame.h"
#include "nsIImageFrame.h"
#include "nsIImageMap.h"





nsHTMLImageMapAccessible::
  nsHTMLImageMapAccessible(nsIContent *aContent, nsIWeakReference *aShell,
                           nsIDOMHTMLMapElement *aMapElm) :
  nsHTMLImageAccessibleWrap(aContent, aShell), mMapElement(aMapElm)
{
}




NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLImageMapAccessible, nsHTMLImageAccessible)




NS_IMETHODIMP
nsHTMLImageMapAccessible::GetAnchorCount(PRInt32 *aAnchorCount)
{
  NS_ENSURE_ARG_POINTER(aAnchorCount);

  return GetChildCount(aAnchorCount);
}

NS_IMETHODIMP
nsHTMLImageMapAccessible::GetURI(PRInt32 aIndex, nsIURI **aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  *aURI = nsnull;

  nsAccessible *areaAcc = GetChildAt(aIndex);
  if (!areaAcc)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIDOMNode> areaNode;
  areaAcc->GetDOMNode(getter_AddRefs(areaNode));

  nsCOMPtr<nsIContent> link(do_QueryInterface(areaNode));
  if (link)
    *aURI = link->GetHrefURI().get();

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLImageMapAccessible::GetAnchor(PRInt32 aIndex, nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  nsAccessible *areaAcc = GetChildAt(aIndex);
  if (!areaAcc)
    return NS_ERROR_INVALID_ARG;

  NS_ADDREF(*aAccessible = areaAcc);
  return NS_OK;
}




nsresult
nsHTMLImageMapAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_IMAGE_MAP;
  return NS_OK;
}




void 
nsHTMLImageMapAccessible::CacheChildren()
{
  if (!mMapElement)
    return;

  nsCOMPtr<nsIDOMHTMLCollection> mapAreas;
  mMapElement->GetAreas(getter_AddRefs(mapAreas));
  if (!mapAreas)
    return;

  PRUint32 areaCount = 0;
  mapAreas->GetLength(&areaCount);

  for (PRUint32 areaIdx = 0; areaIdx < areaCount; areaIdx++) {
    nsCOMPtr<nsIDOMNode> areaNode;
    mapAreas->Item(areaIdx, getter_AddRefs(areaNode));
    if (!areaNode)
      return;

    nsCOMPtr<nsIContent> areaContent(do_QueryInterface(areaNode));
    nsRefPtr<nsAccessible> areaAcc =
      new nsHTMLAreaAccessible(areaContent, mWeakShell);
    if (!areaAcc)
      return;

    if (!areaAcc->Init()) {
      areaAcc->Shutdown();
      return;
    }

    
    areaAcc->SetRoleMapEntry(nsAccUtils::GetRoleMapEntry(areaContent));

    if (!AppendChild(areaAcc))
      return;
  }
}






nsHTMLAreaAccessible::
  nsHTMLAreaAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsHTMLLinkAccessible(aContent, aShell)
{
}




nsresult
nsHTMLAreaAccessible::GetNameInternal(nsAString & aName)
{
  nsresult rv = nsAccessible::GetNameInternal(aName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aName.IsEmpty())
    return NS_OK;

  if (!mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::alt,
                         aName)) {
    return GetValue(aName);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAreaAccessible::GetDescription(nsAString& aDescription)
{
  aDescription.Truncate();

  
  nsCOMPtr<nsIDOMHTMLAreaElement> area(do_QueryInterface(mContent));
  if (area) 
    area->GetShape(aDescription);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLAreaAccessible::GetBounds(PRInt32 *aX, PRInt32 *aY,
                                PRInt32 *aWidth, PRInt32 *aHeight)
{
  NS_ENSURE_ARG_POINTER(aX);
  *aX = 0;
  NS_ENSURE_ARG_POINTER(aY);
  *aY = 0;
  NS_ENSURE_ARG_POINTER(aWidth);
  *aWidth = 0;
  NS_ENSURE_ARG_POINTER(aHeight);
  *aHeight = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  
  nsPresContext *presContext = GetPresContext();
  NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

  nsIFrame *frame = GetFrame();
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);
  nsIImageFrame *imageFrame = do_QueryFrame(frame);

  nsCOMPtr<nsIImageMap> map;
  imageFrame->GetImageMap(presContext, getter_AddRefs(map));
  NS_ENSURE_TRUE(map, NS_ERROR_FAILURE);

  nsRect rect;
  nsIntRect orgRectPixels;
  nsresult rv = map->GetBoundsForAreaContent(mContent, rect);
  NS_ENSURE_SUCCESS(rv, rv);

  *aX = presContext->AppUnitsToDevPixels(rect.x);
  *aY = presContext->AppUnitsToDevPixels(rect.y);

  
  
  *aWidth  = presContext->AppUnitsToDevPixels(rect.width - rect.x);
  *aHeight = presContext->AppUnitsToDevPixels(rect.height - rect.y);

  
  orgRectPixels = frame->GetScreenRectExternal();
  *aX += orgRectPixels.x;
  *aY += orgRectPixels.y;

  return NS_OK;
}




nsresult
nsHTMLAreaAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  
  if (mRoleMapEntry &&
      mRoleMapEntry->role != nsIAccessibleRole::ROLE_NOTHING &&
      mRoleMapEntry->role != nsIAccessibleRole::ROLE_LINK) {
    return nsAccessible::GetStateInternal(aState,aExtraState);
  }

  return nsHTMLLinkAccessible::GetStateInternal(aState, aExtraState);
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
  
}
