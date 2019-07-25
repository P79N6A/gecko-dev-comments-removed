






#include "FormControlAccessible.h"
#include "Role.h"

#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMXULControlElement.h"

using namespace mozilla::a11y;





template class mozilla::a11y::ProgressMeterAccessible<1>;
template class mozilla::a11y::ProgressMeterAccessible<100>;




template<int Max>
NS_IMPL_ADDREF_INHERITED(ProgressMeterAccessible<Max>, nsLeafAccessible)

template<int Max>
NS_IMPL_RELEASE_INHERITED(ProgressMeterAccessible<Max>, nsLeafAccessible)

template<int Max>
NS_IMPL_QUERY_INTERFACE_INHERITED1(ProgressMeterAccessible<Max>,
                                   nsLeafAccessible,
                                   nsIAccessibleValue)




template<int Max>
role
ProgressMeterAccessible<Max>::NativeRole()
{
  return roles::PROGRESSBAR;
}

template<int Max>
PRUint64
ProgressMeterAccessible<Max>::NativeState()
{
  PRUint64 state = nsLeafAccessible::NativeState();

  
  nsAutoString attrValue;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, attrValue);

  if (attrValue.IsEmpty())
    state |= states::MIXED;

  return state;
}




template<int Max>
bool
ProgressMeterAccessible<Max>::IsWidget() const
{
  return true;
}




template<int Max>
void
ProgressMeterAccessible<Max>::Value(nsString& aValue)
{
  nsLeafAccessible::Value(aValue);
  if (!aValue.IsEmpty())
    return;

  double maxValue = 0;
  nsresult rv = GetMaximumValue(&maxValue);
  if (NS_FAILED(rv) || maxValue == 0)
    return;

  double curValue = 0;
  GetCurrentValue(&curValue);
  if (NS_FAILED(rv))
    return;

  
  double percentValue = (curValue < maxValue) ?
    (curValue / maxValue) * 100 : 100;

  aValue.AppendFloat(percentValue);
  aValue.AppendLiteral("%");
}

template<int Max>
NS_IMETHODIMP
ProgressMeterAccessible<Max>::GetMaximumValue(double* aMaximumValue)
{
  nsresult rv = nsLeafAccessible::GetMaximumValue(aMaximumValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  nsAutoString value;
  if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::max, value)) {
    PRInt32 result = NS_OK;
    *aMaximumValue = value.ToDouble(&result);
    return result;
  }

  *aMaximumValue = Max;
  return NS_OK;
}

template<int Max>
NS_IMETHODIMP
ProgressMeterAccessible<Max>::GetMinimumValue(double* aMinimumValue)
{
  nsresult rv = nsLeafAccessible::GetMinimumValue(aMinimumValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  *aMinimumValue = 0;
  return NS_OK;
}

template<int Max>
NS_IMETHODIMP
ProgressMeterAccessible<Max>::GetMinimumIncrement(double* aMinimumIncrement)
{
  nsresult rv = nsLeafAccessible::GetMinimumIncrement(aMinimumIncrement);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  *aMinimumIncrement = 0;
  return NS_OK;
}

template<int Max>
NS_IMETHODIMP
ProgressMeterAccessible<Max>::GetCurrentValue(double* aCurrentValue)
{
  nsresult rv = nsLeafAccessible::GetCurrentValue(aCurrentValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  nsAutoString attrValue;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, attrValue);

  
  if (attrValue.IsEmpty())
    return NS_OK;

  PRInt32 error = NS_OK;
  double value = attrValue.ToDouble(&error);
  if (NS_FAILED(error))
    return NS_OK; 

  *aCurrentValue = value;
  return NS_OK;
}

template<int Max>
NS_IMETHODIMP
ProgressMeterAccessible<Max>::SetCurrentValue(double aValue)
{
  return NS_ERROR_FAILURE; 
}





RadioButtonAccessible::
  RadioButtonAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsLeafAccessible(aContent, aDoc)
{
}

PRUint8
RadioButtonAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
RadioButtonAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("select"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
RadioButtonAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}

role
RadioButtonAccessible::NativeRole()
{
  return roles::RADIOBUTTON;
}




bool
RadioButtonAccessible::IsWidget() const
{
  return true;
}
