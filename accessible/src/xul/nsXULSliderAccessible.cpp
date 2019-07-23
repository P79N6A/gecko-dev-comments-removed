





































#include "nsXULSliderAccessible.h"

#include "nsIDOMDocument.h"
#include "nsIDOMDocumentXBL.h"



nsXULSliderAccessible::nsXULSliderAccessible(nsIDOMNode* aNode,
                                             nsIWeakReference* aShell) :
  nsAccessibleWrap(aNode, aShell)
{
}



NS_IMPL_ISUPPORTS_INHERITED1(nsXULSliderAccessible,
                             nsAccessibleWrap,
                             nsIAccessibleValue)



nsresult
nsXULSliderAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_SLIDER;
  return NS_OK;
}

nsresult
nsXULSliderAccessible::GetStateInternal(PRUint32 *aState,
                                        PRUint32 *aExtraState)
{
  nsresult rv = nsAccessibleWrap::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> sliderContent(GetSliderNode());
  NS_ENSURE_STATE(sliderContent);

  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(mWeakShell));
  NS_ENSURE_STATE(shell);

  nsIFrame *frame = shell->GetPrimaryFrameFor(sliderContent);
  if (frame && frame->IsFocusable())
    *aState |= nsIAccessibleStates::STATE_FOCUSABLE;

  if (gLastFocusedNode == mDOMNode)
    *aState |= nsIAccessibleStates::STATE_FOCUSED;

  return NS_OK;
}



NS_IMETHODIMP
nsXULSliderAccessible::GetValue(nsAString& aValue)
{
  return GetSliderAttr(nsAccessibilityAtoms::curpos, aValue);
}

NS_IMETHODIMP
nsXULSliderAccessible::GetNumActions(PRUint8 *aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);

  *aCount = 1;
  return NS_OK;
}

NS_IMETHODIMP
nsXULSliderAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  aName.Truncate();

  NS_ENSURE_ARG(aIndex == 0);

  aName.AssignLiteral("activate"); 
  return NS_OK;
}

NS_IMETHODIMP
nsXULSliderAccessible::DoAction(PRUint8 aIndex)
{
  NS_ENSURE_ARG(aIndex == 0);

  nsCOMPtr<nsIContent> sliderContent(GetSliderNode());
  NS_ENSURE_STATE(sliderContent);

  return DoCommand(sliderContent);
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

PRBool
nsXULSliderAccessible::GetAllowsAnonChildAccessibles()
{
  
  return PR_FALSE;
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

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> sliderNode(GetSliderNode());
  NS_ENSURE_STATE(sliderNode);

  sliderNode->GetAttr(kNameSpaceID_None, aName, aValue);
  return NS_OK;
}

nsresult
nsXULSliderAccessible::SetSliderAttr(nsIAtom *aName, const nsAString& aValue)
{
  if (IsDefunct())
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

  nsAutoString attrValue;
  nsresult rv = GetSliderAttr(aName, attrValue);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (attrValue.IsEmpty())
    return NS_OK;

  PRInt32 error = NS_OK;
  double value = attrValue.ToFloat(&error);
  if (NS_SUCCEEDED(error))
    *aValue = value;

  return NS_OK;
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



nsresult
nsXULThumbAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_INDICATOR;
  return NS_OK;
}

