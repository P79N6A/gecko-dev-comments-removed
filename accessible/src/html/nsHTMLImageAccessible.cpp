





































#include "imgIContainer.h"
#include "imgIRequest.h"

#include "nsHTMLImageAccessible.h"
#include "nsAccessibilityAtoms.h"
#include "nsHTMLAreaAccessible.h"

#include "nsIDOMHTMLCollection.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIImageLoadingContent.h"
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

NS_IMPL_ISUPPORTS_INHERITED1(nsHTMLImageAccessible, nsLinkableAccessible, nsIAccessibleImage)

NS_IMETHODIMP
nsHTMLImageAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  
  

  nsresult rv = nsLinkableAccessible::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIImageLoadingContent> content(do_QueryInterface(mDOMNode));
  nsCOMPtr<imgIRequest> imageRequest;

  if (content)
    content->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                        getter_AddRefs(imageRequest));

  nsCOMPtr<imgIContainer> imgContainer;
  if (imageRequest)
    imageRequest->GetImage(getter_AddRefs(imgContainer));

  if (imgContainer) {
    PRUint32 numFrames;
    imgContainer->GetNumFrames(&numFrames);
    if (numFrames > 1)
      *aState |= nsIAccessibleStates::STATE_ANIMATED;
  }

  return NS_OK;
}



NS_IMETHODIMP nsHTMLImageAccessible::GetName(nsAString& aName)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (!content) {
    return NS_ERROR_FAILURE;  
  }

  if (!content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::alt,
                        aName)) {
    if (mRoleMapEntry) {
      
      
      return GetHTMLName(aName, PR_FALSE);
    }
    if (!content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::title,
                          aName)) {
      aName.SetIsVoid(PR_TRUE); 
    }
  }

  return NS_OK;
}


NS_IMETHODIMP nsHTMLImageAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = mMapElement ? nsIAccessibleRole::ROLE_IMAGE_MAP :
                           nsIAccessibleRole::ROLE_GRAPHIC;
  return NS_OK;
}


already_AddRefed<nsIAccessible>
nsHTMLImageAccessible::GetAreaAccessible(PRInt32 aAreaNum)
{
  if (!mMapElement)
    return nsnull;

  nsCOMPtr<nsIDOMHTMLCollection> mapAreas;
  mMapElement->GetAreas(getter_AddRefs(mapAreas));
  if (!mapAreas)
    return nsnull;

  nsCOMPtr<nsIDOMNode> domNode;
  mapAreas->Item(aAreaNum,getter_AddRefs(domNode));
  if (!domNode)
    return nsnull;

  nsCOMPtr<nsIAccessNode> accessNode;
  GetCacheEntry(*mAccessNodeCache, (void*)(aAreaNum),
                getter_AddRefs(accessNode));

  if (!accessNode) {
    accessNode = new nsHTMLAreaAccessible(domNode, this, mWeakShell);
    if (!accessNode)
      return nsnull;

    nsCOMPtr<nsPIAccessNode> privateAccessNode(do_QueryInterface(accessNode));
    NS_ASSERTION(privateAccessNode,
                 "Accessible doesn't implement nsPIAccessNode");

    nsresult rv = privateAccessNode->Init();
    if (NS_FAILED(rv))
      return nsnull;

    PutCacheEntry(*mAccessNodeCache, (void*)(aAreaNum), accessNode);
  }

  nsCOMPtr<nsIAccessible> accessible(do_QueryInterface(accessNode));
  nsIAccessible *accPtr;
  NS_IF_ADDREF(accPtr = accessible);
  return accPtr;
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

  nsCOMPtr<nsIDOMHTMLCollection> mapAreas;
  if (mMapElement) {
    mMapElement->GetAreas(getter_AddRefs(mapAreas));
  }
  if (!mapAreas) {
    mAccChildCount = 0;
    return;
  }

  PRUint32 numMapAreas;
  mapAreas->GetLength(&numMapAreas);
  PRInt32 childCount = 0;
  
  nsCOMPtr<nsIAccessible> areaAccessible;
  nsCOMPtr<nsPIAccessible> privatePrevAccessible;
  while (childCount < (PRInt32)numMapAreas && 
         (areaAccessible = GetAreaAccessible(childCount)) != nsnull) {
    if (privatePrevAccessible) {
      privatePrevAccessible->SetNextSibling(areaAccessible);
    }
    else {
      SetFirstChild(areaAccessible);
    }

    ++ childCount;

    privatePrevAccessible = do_QueryInterface(areaAccessible);
    NS_ASSERTION(privatePrevAccessible, "nsIAccessible impl's should always support nsPIAccessible as well");
    privatePrevAccessible->SetParent(this);
  }
  mAccChildCount = childCount;
}

NS_IMETHODIMP nsHTMLImageAccessible::DoAction(PRUint8 index)
{
  if (index == eAction_ShowLongDescription) {
    
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
  return nsLinkableAccessible::DoAction(index);
}

NS_IMETHODIMP nsHTMLImageAccessible::GetImageBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height)
{
  return GetBounds(x, y, width, height);
}

NS_IMETHODIMP
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

