







































#include "nsFormControlAccessible.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMXULControlElement.h"





template class ProgressMeterAccessible<1>;
template class ProgressMeterAccessible<100>;




template<int Max>
NS_IMPL_ADDREF_INHERITED(ProgressMeterAccessible<Max>, nsFormControlAccessible)

template<int Max>
NS_IMPL_RELEASE_INHERITED(ProgressMeterAccessible<Max>, nsFormControlAccessible)

template<int Max>
NS_IMPL_QUERY_INTERFACE_INHERITED1(ProgressMeterAccessible<Max>,
                                   nsFormControlAccessible,
                                   nsIAccessibleValue)




template<int Max>
PRUint32
ProgressMeterAccessible<Max>::NativeRole()
{
  return nsIAccessibleRole::ROLE_PROGRESSBAR;
}




template<int Max>
NS_IMETHODIMP
ProgressMeterAccessible<Max>::GetValue(nsAString& aValue)
{
  nsresult rv = nsFormControlAccessible::GetValue(aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aValue.IsEmpty())
    return NS_OK;

  double maxValue = 0;
  rv = GetMaximumValue(&maxValue);
  NS_ENSURE_SUCCESS(rv, rv);

  double curValue = 0;
  rv = GetCurrentValue(&curValue);
  NS_ENSURE_SUCCESS(rv, rv);

  
  double percentValue = (curValue < maxValue) ?
    (curValue / maxValue) * 100 : 100;

  nsAutoString value;
  value.AppendFloat(percentValue); 
  value.AppendLiteral("%");
  aValue = value;
  return NS_OK;
}

template<int Max>
NS_IMETHODIMP
ProgressMeterAccessible<Max>::GetMaximumValue(double* aMaximumValue)
{
  nsresult rv = nsFormControlAccessible::GetMaximumValue(aMaximumValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  nsAutoString value;
  if (mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::max, value)) {
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
  nsresult rv = nsFormControlAccessible::GetMinimumValue(aMinimumValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  *aMinimumValue = 0;
  return NS_OK;
}

template<int Max>
NS_IMETHODIMP
ProgressMeterAccessible<Max>::GetMinimumIncrement(double* aMinimumIncrement)
{
  nsresult rv = nsFormControlAccessible::GetMinimumIncrement(aMinimumIncrement);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  *aMinimumIncrement = 0;
  return NS_OK;
}

template<int Max>
NS_IMETHODIMP
ProgressMeterAccessible<Max>::GetCurrentValue(double* aCurrentValue)
{
  nsresult rv = nsFormControlAccessible::GetCurrentValue(aCurrentValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  nsAutoString attrValue;
  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value, attrValue);

  
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





nsRadioButtonAccessible::
  nsRadioButtonAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsFormControlAccessible(aContent, aShell)
{
}

NS_IMETHODIMP
nsRadioButtonAccessible::GetNumActions(PRUint8 *aNumActions)
{
  NS_ENSURE_ARG_POINTER(aNumActions);
  *aNumActions = 1;

  return NS_OK;
}

NS_IMETHODIMP nsRadioButtonAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("select"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsRadioButtonAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}

PRUint32
nsRadioButtonAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_RADIOBUTTON;
}

