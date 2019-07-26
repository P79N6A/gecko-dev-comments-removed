




#include "XULSliderAccessible.h"

#include "nsAccessibilityService.h"
#include "Role.h"
#include "States.h"

#include "nsIFrame.h"

using namespace mozilla::a11y;





XULSliderAccessible::
  XULSliderAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
}



NS_IMPL_ISUPPORTS_INHERITED1(XULSliderAccessible,
                             AccessibleWrap,
                             nsIAccessibleValue)



role
XULSliderAccessible::NativeRole()
{
  return roles::SLIDER;
}

PRUint64
XULSliderAccessible::NativeInteractiveState() const
 {
  if (NativelyUnavailable())
    return states::UNAVAILABLE;

  nsIContent* sliderElm = GetSliderElement();
  if (sliderElm) {
    nsIFrame* frame = sliderElm->GetPrimaryFrame();
    if (frame && frame->IsFocusable())
      return states::FOCUSABLE;
  }

  return 0;
}

bool
XULSliderAccessible::NativelyUnavailable() const
{
  return mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                               nsGkAtoms::_true, eCaseMatters);
}



void
XULSliderAccessible::Value(nsString& aValue)
{
  GetSliderAttr(nsGkAtoms::curpos, aValue);
}

PRUint8
XULSliderAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
XULSliderAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  aName.Truncate();

  NS_ENSURE_ARG(aIndex == 0);

  aName.AssignLiteral("activate"); 
  return NS_OK;
}

NS_IMETHODIMP
XULSliderAccessible::DoAction(PRUint8 aIndex)
{
  NS_ENSURE_ARG(aIndex == 0);

  nsIContent* sliderElm = GetSliderElement();
  if (sliderElm)
    DoCommand(sliderElm);

  return NS_OK;
}



NS_IMETHODIMP
XULSliderAccessible::GetMaximumValue(double* aValue)
{
  nsresult rv = AccessibleWrap::GetMaximumValue(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return GetSliderAttr(nsGkAtoms::maxpos, aValue);
}

NS_IMETHODIMP
XULSliderAccessible::GetMinimumValue(double* aValue)
{
  nsresult rv = AccessibleWrap::GetMinimumValue(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return GetSliderAttr(nsGkAtoms::minpos, aValue);
}

NS_IMETHODIMP
XULSliderAccessible::GetMinimumIncrement(double* aValue)
{
  nsresult rv = AccessibleWrap::GetMinimumIncrement(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return GetSliderAttr(nsGkAtoms::increment, aValue);
}

NS_IMETHODIMP
XULSliderAccessible::GetCurrentValue(double* aValue)
{
  nsresult rv = AccessibleWrap::GetCurrentValue(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return GetSliderAttr(nsGkAtoms::curpos, aValue);
}

NS_IMETHODIMP
XULSliderAccessible::SetCurrentValue(double aValue)
{
  nsresult rv = AccessibleWrap::SetCurrentValue(aValue);

  
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  return SetSliderAttr(nsGkAtoms::curpos, aValue);
}

bool
XULSliderAccessible::CanHaveAnonChildren()
{
  
  return false;
}



nsIContent*
XULSliderAccessible::GetSliderElement() const
{
  if (!mSliderNode) {
    
    mSliderNode = mContent->OwnerDoc()->
      GetAnonymousElementByAttribute(mContent, nsGkAtoms::anonid,
                                     NS_LITERAL_STRING("slider"));
  }

  return mSliderNode;
}

nsresult
XULSliderAccessible::GetSliderAttr(nsIAtom* aName, nsAString& aValue)
{
  aValue.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsIContent* sliderElm = GetSliderElement();
  if (sliderElm)
    sliderElm->GetAttr(kNameSpaceID_None, aName, aValue);

  return NS_OK;
}

nsresult
XULSliderAccessible::SetSliderAttr(nsIAtom* aName, const nsAString& aValue)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsIContent* sliderElm = GetSliderElement();
  if (sliderElm)
    sliderElm->SetAttr(kNameSpaceID_None, aName, aValue, true);

  return NS_OK;
}

nsresult
XULSliderAccessible::GetSliderAttr(nsIAtom* aName, double* aValue)
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
XULSliderAccessible::SetSliderAttr(nsIAtom* aName, double aValue)
{
  nsAutoString value;
  value.AppendFloat(aValue);

  return SetSliderAttr(aName, value);
}






XULThumbAccessible::
  XULThumbAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
}




role
XULThumbAccessible::NativeRole()
{
  return roles::INDICATOR;
}

