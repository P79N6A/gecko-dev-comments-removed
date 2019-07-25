






#include "FormControlAccessible.h"
#include "Role.h"

#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMXULElement.h"
#include "nsIDOMXULControlElement.h"

using namespace mozilla::a11y;





template class mozilla::a11y::ProgressMeterAccessible<1>;
template class mozilla::a11y::ProgressMeterAccessible<100>;




template<int Max>
NS_IMPL_ADDREF_INHERITED(ProgressMeterAccessible<Max>, LeafAccessible)

template<int Max>
NS_IMPL_RELEASE_INHERITED(ProgressMeterAccessible<Max>, LeafAccessible)

template<int Max>
NS_IMPL_QUERY_INTERFACE_INHERITED1(ProgressMeterAccessible<Max>,
                                   LeafAccessible,
                                   nsIAccessibleValue)




template<int Max>
role
ProgressMeterAccessible<Max>::NativeRole()
{
  return roles::PROGRESSBAR;
}

template<int Max>
uint64_t
ProgressMeterAccessible<Max>::NativeState()
{
  uint64_t state = LeafAccessible::NativeState();

  
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
  LeafAccessible::Value(aValue);
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
  nsresult rv = LeafAccessible::GetMaximumValue(aMaximumValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  nsAutoString value;
  if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::max, value)) {
    nsresult result = NS_OK;
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
  nsresult rv = LeafAccessible::GetMinimumValue(aMinimumValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  *aMinimumValue = 0;
  return NS_OK;
}

template<int Max>
NS_IMETHODIMP
ProgressMeterAccessible<Max>::GetMinimumIncrement(double* aMinimumIncrement)
{
  nsresult rv = LeafAccessible::GetMinimumIncrement(aMinimumIncrement);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  *aMinimumIncrement = 0;
  return NS_OK;
}

template<int Max>
NS_IMETHODIMP
ProgressMeterAccessible<Max>::GetCurrentValue(double* aCurrentValue)
{
  nsresult rv = LeafAccessible::GetCurrentValue(aCurrentValue);
  if (rv != NS_OK_NO_ARIA_VALUE)
    return rv;

  nsAutoString attrValue;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, attrValue);

  
  if (attrValue.IsEmpty())
    return NS_OK;

  nsresult error = NS_OK;
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
  RadioButtonAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  LeafAccessible(aContent, aDoc)
{
}

uint8_t
RadioButtonAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
RadioButtonAccessible::GetActionName(uint8_t aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("select"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
RadioButtonAccessible::DoAction(uint8_t aIndex)
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
