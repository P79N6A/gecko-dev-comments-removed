





































#include "imgIContainer.h"
#include "imgIRequest.h"

#include "nsHTMLImageAccessible.h"
#include "nsAccessibilityAtoms.h"
#include "nsHTMLAreaAccessible.h"

#include "nsIDOMHTMLCollection.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIImageLoadingContent.h"
#include "nsILink.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMDocument.h"
#include "nsPIDOMWindow.h"



const PRUint32 kDefaultImageCacheSize = 256;




nsHTMLImageAccessible::nsHTMLImageAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsLinkableAccessible(aDOMNode, aShell), mAccessNodeCache(nsnull)
{ 
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(aDOMNode));
  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mWeakShell));
  if (!shell)
    return;

  nsIDocument *doc = shell->GetDocument();
  nsAutoString mapElementName;

  if (doc && element) {
    nsCOMPtr<nsIHTMLDocument> htmlDoc(do_QueryInterface(doc));
    element->GetAttribute(NS_LITERAL_STRING("usemap"),mapElementName);
    if (htmlDoc && !mapElementName.IsEmpty()) {
      if (mapElementName.CharAt(0) == '#')
        mapElementName.Cut(0,1);
      mMapElement = htmlDoc->GetImageMap(mapElementName);
    }
  }

  if (mMapElement) {
    mAccessNodeCache = new nsAccessNodeHashtable();
    mAccessNodeCache->Init(kDefaultImageCacheSize);
  }
}

NS_IMPL_ISUPPORTS_INHERITED1(nsHTMLImageAccessible, nsAccessible,
                             nsIAccessibleImage)




nsresult
nsHTMLImageAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  
  

  nsresult rv = nsLinkableAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  nsCOMPtr<nsIImageLoadingContent> content(do_QueryInterface(mDOMNode));
  nsCOMPtr<imgIRequest> imageRequest;

  if (content)
    content->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                        getter_AddRefs(imageRequest));

  nsCOMPtr<imgIContainer> imgContainer;
  if (imageRequest)
    imageRequest->GetImage(getter_AddRefs(imgContainer));

  if (imgContainer) {
    PRBool animated;
    imgContainer->GetAnimated(&animated);
    if (animated)
      *aState |= nsIAccessibleStates::STATE_ANIMATED;
  }

  return NS_OK;
}

nsresult
nsHTMLImageAccessible::GetNameInternal(nsAString& aName)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  PRBool hasAltAttrib =
    content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::alt, aName);
  if (!aName.IsEmpty())
    return NS_OK;

  nsresult rv = nsAccessible::GetNameInternal(aName);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aName.IsEmpty() && hasAltAttrib) {
    
    
    
    
    return NS_OK_EMPTY_NAME;
  }

  return NS_OK;
}

nsresult
nsHTMLImageAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = mMapElement ? nsIAccessibleRole::ROLE_IMAGE_MAP :
                         nsIAccessibleRole::ROLE_GRAPHIC;
  return NS_OK;
}

void nsHTMLImageAccessible::CacheChildren()
{
  if (!mWeakShell) {
    
    mAccChildCount = eChildCountUninitialized;
    return;
  }

  if (mAccChildCount != eChildCountUninitialized) {
    return;
  }

  mAccChildCount = 0;
  nsCOMPtr<nsIDOMHTMLCollection> mapAreas = GetAreaCollection();
  if (!mapAreas)
    return;

  PRUint32 numMapAreas;
  mapAreas->GetLength(&numMapAreas);
  PRInt32 childCount = 0;
  
  nsCOMPtr<nsIAccessible> areaAccessible;
  nsRefPtr<nsAccessible> prevAcc;
  while (childCount < (PRInt32)numMapAreas && 
         (areaAccessible = GetAreaAccessible(mapAreas, childCount)) != nsnull) {
    if (prevAcc)
      prevAcc->SetNextSibling(areaAccessible);
    else
      SetFirstChild(areaAccessible);

    ++ childCount;

    prevAcc = nsAccUtils::QueryAccessible(areaAccessible);
    prevAcc->SetParent(this);
  }
  mAccChildCount = childCount;
}

NS_IMETHODIMP
nsHTMLImageAccessible::GetNumActions(PRUint8 *aNumActions)
{
  NS_ENSURE_ARG_POINTER(aNumActions);
  *aNumActions = 0;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsresult rv= nsLinkableAccessible::GetNumActions(aNumActions);
  NS_ENSURE_SUCCESS(rv, rv);

  if (HasLongDesc())
    (*aNumActions)++;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLImageAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  aName.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (IsValidLongDescIndex(aIndex)) {
    aName.AssignLiteral("showlongdesc"); 
    return NS_OK;
  }
  return nsLinkableAccessible::GetActionName(aIndex, aName);
}

NS_IMETHODIMP
nsHTMLImageAccessible::DoAction(PRUint8 aIndex)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  if (IsValidLongDescIndex(aIndex)) {
    
    nsCOMPtr<nsIDOMHTMLImageElement> element(do_QueryInterface(mDOMNode));
    NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);
    nsAutoString longDesc;
    nsresult rv = element->GetLongDesc(longDesc);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDOMDocument> domDocument;
    rv = mDOMNode->GetOwnerDocument(getter_AddRefs(domDocument));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDocument> document(do_QueryInterface(domDocument));
    nsCOMPtr<nsPIDOMWindow> piWindow = document->GetWindow();
    nsCOMPtr<nsIDOMWindowInternal> win(do_QueryInterface(piWindow));
    NS_ENSURE_TRUE(win, NS_ERROR_FAILURE);
    nsCOMPtr<nsIDOMWindow> tmp;
    return win->Open(longDesc, NS_LITERAL_STRING(""), NS_LITERAL_STRING(""),
                     getter_AddRefs(tmp));
  }
  return nsLinkableAccessible::DoAction(aIndex);
}



NS_IMETHODIMP
nsHTMLImageAccessible::GetAnchorCount(PRInt32 *aAnchorCount)
{
  NS_ENSURE_ARG_POINTER(aAnchorCount);

  if (!mMapElement)
    return nsLinkableAccessible::GetAnchorCount(aAnchorCount);

  return GetChildCount(aAnchorCount);
}

NS_IMETHODIMP
nsHTMLImageAccessible::GetURI(PRInt32 aIndex, nsIURI **aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  *aURI = nsnull;

  if (!mMapElement)
    return nsLinkableAccessible::GetURI(aIndex, aURI);

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
nsHTMLImageAccessible::GetAnchor(PRInt32 aIndex, nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  if (!mMapElement)
    return nsLinkableAccessible::GetAnchor(aIndex, aAccessible);

  nsCOMPtr<nsIDOMHTMLCollection> mapAreas = GetAreaCollection();
  if (mapAreas) {
    nsCOMPtr<nsIAccessible> accessible;
    accessible = GetAreaAccessible(mapAreas, aIndex);
    if (!accessible)
      return NS_ERROR_INVALID_ARG;

    NS_ADDREF(*aAccessible = accessible);
  }

  return NS_OK;
}




NS_IMETHODIMP
nsHTMLImageAccessible::GetImagePosition(PRUint32 aCoordType,
                                        PRInt32 *aX, PRInt32 *aY)
{
  PRInt32 width, height;
  nsresult rv = GetBounds(aX, aY, &width, &height);
  if (NS_FAILED(rv))
    return rv;

  return nsAccUtils::ConvertScreenCoordsTo(aX, aY, aCoordType, this);
}

NS_IMETHODIMP
nsHTMLImageAccessible::GetImageSize(PRInt32 *aWidth, PRInt32 *aHeight)
{
  PRInt32 x, y;
  return GetBounds(&x, &y, aWidth, aHeight);
}




nsresult
nsHTMLImageAccessible::Shutdown()
{
  nsLinkableAccessible::Shutdown();

  if (mAccessNodeCache) {
    ClearCache(*mAccessNodeCache);
    delete mAccessNodeCache;
    mAccessNodeCache = nsnull;
  }

  return NS_OK;
}




nsresult
nsHTMLImageAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;
  
  nsresult rv = nsLinkableAccessible::GetAttributesInternal(aAttributes);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  nsAutoString src;
  content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::src, src);
  if (!src.IsEmpty())
    nsAccUtils::SetAccAttr(aAttributes, nsAccessibilityAtoms::src, src);

  return NS_OK;
}

already_AddRefed<nsIDOMHTMLCollection>
nsHTMLImageAccessible::GetAreaCollection()
{
  if (!mMapElement)
    return nsnull;

  nsIDOMHTMLCollection *mapAreas = nsnull;
  nsresult rv = mMapElement->GetAreas(&mapAreas);
  if (NS_FAILED(rv))
    return nsnull;

  return mapAreas;
}

already_AddRefed<nsIAccessible>
nsHTMLImageAccessible::GetAreaAccessible(nsIDOMHTMLCollection *aAreaCollection,
                                         PRInt32 aAreaNum)
{
  if (!aAreaCollection)
    return nsnull;

  nsCOMPtr<nsIDOMNode> domNode;
  aAreaCollection->Item(aAreaNum,getter_AddRefs(domNode));
  if (!domNode)
    return nsnull;
  
  nsCOMPtr<nsIAccessNode> accessNode;
  GetCacheEntry(*mAccessNodeCache, (void*)(aAreaNum),
                getter_AddRefs(accessNode));
  
  if (!accessNode) {
    accessNode = new nsHTMLAreaAccessible(domNode, this, mWeakShell);
    if (!accessNode)
      return nsnull;
    
    nsRefPtr<nsAccessNode> accNode = nsAccUtils::QueryAccessNode(accessNode);
    nsresult rv = accNode->Init();
    if (NS_FAILED(rv))
      return nsnull;
    
    PutCacheEntry(*mAccessNodeCache, (void*)(aAreaNum), accessNode);
  }

  nsIAccessible *accessible = nsnull;
  CallQueryInterface(accessNode, &accessible);

  return accessible;
}




PRBool
nsHTMLImageAccessible::HasLongDesc()
{
  if (IsDefunct())
    return PR_FALSE;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  return (content->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::longDesc));
}

PRBool
nsHTMLImageAccessible::IsValidLongDescIndex(PRUint8 aIndex)
{
  if (!HasLongDesc())
    return PR_FALSE;

  PRUint8 numActions = 0;
  nsresult rv = nsLinkableAccessible::GetNumActions(&numActions);  
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  return (aIndex == numActions);
}
