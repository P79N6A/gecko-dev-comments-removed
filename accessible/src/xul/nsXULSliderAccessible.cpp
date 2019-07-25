





































#include "nsXULSliderAccessible.h"

#include "nsAccessibilityService.h"
#include "Role.h"
#include "States.h"

#include "nsIDOMDocument.h"
#include "nsIDOMDocumentXBL.h"
#include "nsIFrame.h"

using namespace mozilla::a11y;





nsXULSliderAccessible::
  nsXULSliderAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsAccessibleWrap(aContent, aDoc)
{
}



NS_IMPL_ISUPPORTS_INHERITED1(nsXULSliderAccessible,
                             nsAccessibleWrap,
                             nsIAccessibleValue)



role
nsXULSliderAccessible::NativeRole()
{
  return roles::SLIDER;
}

PRUint64
nsXULSliderAccessible::NativeState()
{
  PRUint64 states = nsAccessibleWrap::NativeState();

  nsCOMPtr<nsIContent> sliderContent(GetSliderNode());
  NS_ENSURE_STATE(sliderContent);

  nsIFrame *frame = sliderContent->GetPrimaryFrame();
  if (frame && frame->IsFocusable())
    states |= states::FOCUSABLE;

  if (FocusMgr()->IsFocused(this))
    states |= states::FOCUSED;

  return states;
}



void
nsXULSliderAccessible::Value(nsString& aValue)
{
  GetSliderAttr(nsGkAtoms::curpos, aValue);
}

PRUint8
nsXULSliderAccessible::ActionCount()
{
  return 1;
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

  DoCommand(sliderContent);
  return NS_OK;
}



NS_IMETHODIMP
nsXULSliderAccessible::GetMaximumValue(double *aValue)
{
  nsresult rv = nsAccessibleWrap::GetMaximumValue(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return GetSliderAttr(nsGkAtoms::maxpos, aValue);
}

NS_IMETHODIMP
nsXULSliderAccessible::GetMinimumValue(double *aValue)
{
  nsresult rv = nsAccessibleWrap::GetMinimumValue(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return GetSliderAttr(nsGkAtoms::minpos, aValue);
}

NS_IMETHODIMP
nsXULSliderAccessible::GetMinimumIncrement(double *aValue)
{
  nsresult rv = nsAccessibleWrap::GetMinimumIncrement(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return GetSliderAttr(nsGkAtoms::increment, aValue);
}

NS_IMETHODIMP
nsXULSliderAccessible::GetCurrentValue(double *aValue)
{
  nsresult rv = nsAccessibleWrap::GetCurrentValue(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return GetSliderAttr(nsGkAtoms::curpos, aValue);
}

NS_IMETHODIMP
nsXULSliderAccessible::SetCurrentValue(double aValue)
{
  nsresult rv = nsAccessibleWrap::SetCurrentValue(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return SetSliderAttr(nsGkAtoms::curpos, aValue);
}

bool
nsXULSliderAccessible::CanHaveAnonChildren()
{
  
  return false;
}



already_AddRefed<nsIContent>
nsXULSliderAccessible::GetSliderNode()
{
  if (IsDefunct())
    return nsnull;

  if (!mSliderNode) {
    nsCOMPtr<nsIDOMDocumentXBL> xblDoc(do_QueryInterface(mContent->OwnerDoc()));
    if (!xblDoc)
      return nsnull;

    
    nsCOMPtr<nsIDOMElement> domElm(do_QueryInterface(mContent));
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

  sliderNode->SetAttr(kNameSpaceID_None, aName, aValue, true);
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
  double value = attrValue.ToDouble(&error);
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






nsXULThumbAccessible::
  nsXULThumbAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsAccessibleWrap(aContent, aDoc)
{
}




role
nsXULThumbAccessible::NativeRole()
{
  return roles::INDICATOR;
}

