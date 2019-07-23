





































#include "nsXULSliderAccessible.h"

#include "nsIDOMDocument.h"
#include "nsIDOMDocumentXBL.h"



nsXULSliderAccessible::nsXULSliderAccessible(nsIDOMNode* aNode,
                                             nsIWeakReference* aShell) :
  nsAccessibleWrap(aNode, aShell)
{
}



NS_IMETHODIMP
nsXULSliderAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);

  *aRole = nsIAccessibleRole::ROLE_SLIDER;
  return NS_OK;
}

NS_IMETHODIMP
nsXULSliderAccessible::GetValue(nsAString& aValue)
{
  return GetSliderAttr(nsAccessibilityAtoms::curpos, aValue);
}



NS_IMETHODIMP
nsXULSliderAccessible::GetMaximumValue(double *aValue)
{
  nsresult rv = nsAccessibleWrap::GetMaximumValue(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return GetSliderAttr(nsAccessibilityAtoms::maxpos, aValue);
}

NS_IMETHODIMP
nsXULSliderAccessible::GetMinimumValue(double *aValue)
{
  nsresult rv = nsAccessibleWrap::GetMinimumValue(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return GetSliderAttr(nsAccessibilityAtoms::minpos, aValue);
}

NS_IMETHODIMP
nsXULSliderAccessible::GetMinimumIncrement(double *aValue)
{
  nsresult rv = nsAccessibleWrap::GetMinimumIncrement(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return GetSliderAttr(nsAccessibilityAtoms::increment, aValue);
}

NS_IMETHODIMP
nsXULSliderAccessible::GetCurrentValue(double *aValue)
{
  nsresult rv = nsAccessibleWrap::GetCurrentValue(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return GetSliderAttr(nsAccessibilityAtoms::curpos, aValue);
}

NS_IMETHODIMP
nsXULSliderAccessible::SetCurrentValue(double aValue)
{
  nsresult rv = nsAccessibleWrap::SetCurrentValue(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return SetSliderAttr(nsAccessibilityAtoms::curpos, aValue);
}


NS_IMETHODIMP
nsXULSliderAccessible::GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren)
{
  NS_ENSURE_ARG_POINTER(aAllowsAnonChildren);

  
  *aAllowsAnonChildren = PR_TRUE;
  return NS_OK;
}



already_AddRefed<nsIContent>
nsXULSliderAccessible::GetSliderNode()
{
  if (!mDOMNode)
    return nsnull;

  if (!mSliderNode) {
    nsCOMPtr<nsIDOMDocument> document;
    mDOMNode->GetOwnerDocument(getter_AddRefs(document));
    if (!document)
      return nsnull;

    nsCOMPtr<nsIDOMDocumentXBL> xblDoc(do_QueryInterface(document));
    if (!xblDoc)
      return nsnull;

    
    nsCOMPtr<nsIDOMElement> domElm(do_QueryInterface(mDOMNode));
    if (!domElm)
      return nsnull;

    xblDoc->GetAnonymousElementByAttribute(domElm, NS_LITERAL_STRING("anonid"),
                                           NS_LITERAL_STRING("slider"),
                                           getter_AddRefs(mSliderNode));
  }

  nsIContent *sliderNode = nsnull;
  nsresult rv = CallQueryInterface(mSliderNode, &sliderNode);
  return NS_FAILED(rv) ? nsnull : sliderNode;
}

nsresult
nsXULSliderAccessible::GetSliderAttr(nsIAtom *aName, nsAString& aValue)
{
  aValue.Truncate();

  if (!mDOMNode)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> sliderNode(GetSliderNode());
  NS_ENSURE_STATE(sliderNode);

  sliderNode->GetAttr(kNameSpaceID_None, aName, aValue);
  return NS_OK;
}

nsresult
nsXULSliderAccessible::SetSliderAttr(nsIAtom *aName, const nsAString& aValue)
{
  if (!mDOMNode)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> sliderNode(GetSliderNode());
  NS_ENSURE_STATE(sliderNode);

  sliderNode->SetAttr(kNameSpaceID_None, aName, aValue, PR_TRUE);
  return NS_OK;
}

nsresult
nsXULSliderAccessible::GetSliderAttr(nsIAtom *aName, double *aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  *aValue = 0;

  nsAutoString value;
  nsresult rv = GetSliderAttr(aName, value);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 error = NS_OK;
  *aValue = value.ToFloat(&error);
  return error;
}

nsresult
nsXULSliderAccessible::SetSliderAttr(nsIAtom *aName, double aValue)
{
  nsAutoString value;
  value.AppendFloat(aValue);

  return SetSliderAttr(aName, value);
}




nsXULThumbAccessible::nsXULThumbAccessible(nsIDOMNode* aNode,
                                           nsIWeakReference* aShell) :
  nsAccessibleWrap(aNode, aShell) {}



NS_IMETHODIMP
nsXULThumbAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);

  *aRole = nsIAccessibleRole::ROLE_INDICATOR;
  return NS_OK;
}
