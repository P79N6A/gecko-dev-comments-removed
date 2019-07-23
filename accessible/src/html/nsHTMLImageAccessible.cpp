





































#include "imgIContainer.h"
#include "imgIRequest.h"

#include "nsHTMLImageAccessible.h"
#include "nsAccessibilityAtoms.h"

#include "nsIDocument.h"
#include "nsIImageLoadingContent.h"
#include "nsILink.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMDocument.h"
#include "nsPIDOMWindow.h"





nsHTMLImageAccessible::
  nsHTMLImageAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell) :
  nsLinkableAccessible(aDOMNode, aShell)
{
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
  *aRole = nsIAccessibleRole::ROLE_GRAPHIC;
  return NS_OK;
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
