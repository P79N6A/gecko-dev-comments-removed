






































#include "nsHTMLImageMapAccessible.h"

#include "nsIDOMHTMLCollection.h"
#include "nsIServiceManager.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIFrame.h"
#include "nsIImageFrame.h"
#include "nsIImageMap.h"





const PRUint32 kDefaultImageMapCacheSize = 256;

nsHTMLImageMapAccessible::
  nsHTMLImageMapAccessible(nsIDOMNode *aDOMNode, nsIWeakReference *aShell,
                           nsIDOMHTMLMapElement *aMapElm) :
  nsHTMLImageAccessibleWrap(aDOMNode, aShell), mMapElement(aMapElm)
{
  mAreaAccCache.Init(kDefaultImageMapCacheSize);
}




NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLImageMapAccessible)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLImageMapAccessible,
                                                  nsAccessible)
CycleCollectorTraverseCache(tmp->mAreaAccCache, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsHTMLImageMapAccessible,
                                                nsAccessible)
ClearCache(tmp->mAreaAccCache);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsHTMLImageMapAccessible)
NS_INTERFACE_MAP_END_INHERITING(nsHTMLImageAccessible)

NS_IMPL_ADDREF_INHERITED(nsHTMLImageMapAccessible, nsHTMLImageAccessible)
NS_IMPL_RELEASE_INHERITED(nsHTMLImageMapAccessible, nsHTMLImageAccessible)




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

  nsCOMPtr<nsIDOMHTMLCollection> mapAreas = GetAreaCollection();
  if (!mapAreas)
    return NS_OK;

  nsCOMPtr<nsIDOMNode> domNode;
  mapAreas->Item(aIndex, getter_AddRefs(domNode));
  if (!domNode)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIContent> link(do_QueryInterface(domNode));
  if (link)
    *aURI = link->GetHrefURI().get();

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLImageMapAccessible::GetAnchor(PRInt32 aIndex, nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  nsCOMPtr<nsIDOMHTMLCollection> mapAreas = GetAreaCollection();
  if (mapAreas) {
    nsRefPtr<nsIAccessible> accessible = GetAreaAccessible(mapAreas, aIndex);
    if (!accessible)
      return NS_ERROR_INVALID_ARG;

    NS_ADDREF(*aAccessible = accessible);
  }

  return NS_OK;
}




nsresult
nsHTMLImageMapAccessible::Shutdown()
{
  nsLinkableAccessible::Shutdown();

  ClearCache(mAreaAccCache);
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
  nsCOMPtr<nsIDOMHTMLCollection> mapAreas = GetAreaCollection();
  if (!mapAreas)
    return;

  PRUint32 areaCount = 0;
  mapAreas->GetLength(&areaCount);

  nsRefPtr<nsAccessible> areaAcc;
  for (PRUint32 areaIdx = 0; areaIdx < areaCount; areaIdx++) {
    areaAcc = GetAreaAccessible(mapAreas, areaIdx);
    if (!areaAcc)
      return;

    mChildren.AppendElement(areaAcc);
    areaAcc->SetParent(this);
  }
}




already_AddRefed<nsIDOMHTMLCollection>
nsHTMLImageMapAccessible::GetAreaCollection()
{
  if (!mMapElement)
    return nsnull;

  nsIDOMHTMLCollection *mapAreas = nsnull;
  mMapElement->GetAreas(&mapAreas);
  return mapAreas;
}

already_AddRefed<nsAccessible>
nsHTMLImageMapAccessible::GetAreaAccessible(nsIDOMHTMLCollection *aAreaCollection,
                                            PRInt32 aAreaNum)
{
  if (!aAreaCollection)
    return nsnull;

  nsCOMPtr<nsIDOMNode> domNode;
  aAreaCollection->Item(aAreaNum,getter_AddRefs(domNode));
  if (!domNode)
    return nsnull;

  void *key = reinterpret_cast<void*>(aAreaNum);
  nsRefPtr<nsAccessible> accessible = mAreaAccCache.GetWeak(key);

  if (!accessible) {
    accessible = new nsHTMLAreaAccessible(domNode, this, mWeakShell);
    if (!accessible)
      return nsnull;

    nsresult rv = accessible->Init();
    if (NS_FAILED(rv)) {
      accessible->Shutdown();
      return nsnull;
    }

    mAreaAccCache.Put(key, accessible);
  }

  return accessible.forget();
}






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
  rv = map->GetBoundsForAreaContent(ourContent, rect);
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
  
}
