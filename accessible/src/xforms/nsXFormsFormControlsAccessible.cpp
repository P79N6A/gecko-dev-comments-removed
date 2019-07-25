





































#include "nsXFormsFormControlsAccessible.h"

#include "nsTextEquivUtils.h"
#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;





nsXFormsLabelAccessible::
  nsXFormsLabelAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsAccessible(aContent, aShell)
{
}

role
nsXFormsLabelAccessible::NativeRole()
{
  return roles::LABEL;
}

nsresult
nsXFormsLabelAccessible::GetNameInternal(nsAString& aName)
{
  
  return NS_OK;
}

void
nsXFormsLabelAccessible::Description(nsString& aDescription)
{
  nsTextEquivUtils::
    GetTextEquivFromIDRefs(this, nsGkAtoms::aria_describedby,
                           aDescription);
}






nsXFormsOutputAccessible::
  nsXFormsOutputAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsAccessible(aContent, aShell)
{
}

role
nsXFormsOutputAccessible::NativeRole()
{
  return roles::STATICTEXT;
}






nsXFormsTriggerAccessible::
  nsXFormsTriggerAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsAccessible(aContent, aShell)
{
}

role
nsXFormsTriggerAccessible::NativeRole()
{
  return roles::PUSHBUTTON;
}

NS_IMETHODIMP
nsXFormsTriggerAccessible::GetValue(nsAString& aValue)
{
  aValue.Truncate();
  return NS_OK;
}

PRUint8
nsXFormsTriggerAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
nsXFormsTriggerAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("press");
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsXFormsTriggerAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}






nsXFormsInputAccessible::
  nsXFormsInputAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsEditableAccessible(aContent, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED3(nsXFormsInputAccessible, nsAccessible, nsHyperTextAccessible, nsIAccessibleText, nsIAccessibleEditableText)

role
nsXFormsInputAccessible::NativeRole()
{
  return roles::ENTRY;
}

PRUint8
nsXFormsInputAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
nsXFormsInputAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  aName.AssignLiteral("activate");
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  return sXFormsService->Focus(DOMNode);
}






nsXFormsInputBooleanAccessible::
  nsXFormsInputBooleanAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsAccessible(aContent, aShell)
{
}

role
nsXFormsInputBooleanAccessible::NativeRole()
{
  return roles::CHECKBUTTON;
}

PRUint64
nsXFormsInputBooleanAccessible::NativeState()
{
  PRUint64 state = nsXFormsAccessible::NativeState();

  nsAutoString value;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->GetValue(DOMNode, value);
  NS_ENSURE_SUCCESS(rv, state);

  if (value.EqualsLiteral("true"))
    state |= states::CHECKED;

  return state;
}

PRUint8
nsXFormsInputBooleanAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
nsXFormsInputBooleanAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  nsAutoString value;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->GetValue(DOMNode, value);
  NS_ENSURE_SUCCESS(rv, rv);

  if (value.EqualsLiteral("true"))
    aName.AssignLiteral("uncheck");
  else
    aName.AssignLiteral("check");

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsInputBooleanAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  DoCommand();
  return NS_OK;
}






nsXFormsInputDateAccessible::
  nsXFormsInputDateAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsContainerAccessible(aContent, aShell)
{
}

role
nsXFormsInputDateAccessible::NativeRole()
{
  return roles::DROPLIST;
}






nsXFormsSecretAccessible::
  nsXFormsSecretAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsInputAccessible(aContent, aShell)
{
}

role
nsXFormsSecretAccessible::NativeRole()
{
  return roles::PASSWORD_TEXT;
}

PRUint64
nsXFormsSecretAccessible::NativeState()
{
  return nsXFormsInputAccessible::NativeState() | states::PROTECTED;
}

NS_IMETHODIMP
nsXFormsSecretAccessible::GetValue(nsAString& aValue)
{
  return NS_ERROR_FAILURE;
}






nsXFormsRangeAccessible::
  nsXFormsRangeAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsAccessible(aContent, aShell)
{
}

role
nsXFormsRangeAccessible::NativeRole()
{
  return roles::SLIDER;
}

PRUint64
nsXFormsRangeAccessible::NativeState()
{
  PRUint64 state = nsXFormsAccessible::NativeState();

  PRUint32 isInRange = nsIXFormsUtilityService::STATE_NOT_A_RANGE;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->IsInRange(DOMNode, &isInRange);
  NS_ENSURE_SUCCESS(rv, state);

  if (isInRange == nsIXFormsUtilityService::STATE_OUT_OF_RANGE)
    state |= states::INVALID;

  return state;
}

NS_IMETHODIMP
nsXFormsRangeAccessible::GetMaximumValue(double *aMaximumValue)
{
  NS_ENSURE_ARG_POINTER(aMaximumValue);

  nsAutoString value;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->GetRangeEnd(DOMNode, value);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 error = NS_OK;
  *aMaximumValue = value.ToDouble(&error);
  return error;
}

NS_IMETHODIMP
nsXFormsRangeAccessible::GetMinimumValue(double *aMinimumValue)
{
  NS_ENSURE_ARG_POINTER(aMinimumValue);

  nsAutoString value;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->GetRangeStart(DOMNode, value);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 error = NS_OK;
  *aMinimumValue = value.ToDouble(&error);
  return error;
}

NS_IMETHODIMP
nsXFormsRangeAccessible::GetMinimumIncrement(double *aMinimumIncrement)
{
  NS_ENSURE_ARG_POINTER(aMinimumIncrement);

  nsAutoString value;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->GetRangeStep(DOMNode, value);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 error = NS_OK;
  *aMinimumIncrement = value.ToDouble(&error);
  return error;
}

NS_IMETHODIMP
nsXFormsRangeAccessible::GetCurrentValue(double *aCurrentValue)
{
  NS_ENSURE_ARG_POINTER(aCurrentValue);

  nsAutoString value;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->GetValue(DOMNode, value);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 error = NS_OK;
  *aCurrentValue = value.ToDouble(&error);
  return error;
}






nsXFormsSelectAccessible::
  nsXFormsSelectAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsContainerAccessible(aContent, aShell)
{
}

PRUint64
nsXFormsSelectAccessible::NativeState()
{
  PRUint64 state = nsXFormsContainerAccessible::NativeState();

  PRUint32 isInRange = nsIXFormsUtilityService::STATE_NOT_A_RANGE;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->IsInRange(DOMNode, &isInRange);
  NS_ENSURE_SUCCESS(rv, state);

  if (isInRange == nsIXFormsUtilityService::STATE_OUT_OF_RANGE)
    state |= states::INVALID;

  return state;
}






nsXFormsChoicesAccessible::
  nsXFormsChoicesAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsAccessible(aContent, aShell)
{
}

role
nsXFormsChoicesAccessible::NativeRole()
{
  return roles::GROUPING;
}

NS_IMETHODIMP
nsXFormsChoicesAccessible::GetValue(nsAString& aValue)
{
  aValue.Truncate();
  return NS_OK;
}

void
nsXFormsChoicesAccessible::CacheChildren()
{
  CacheSelectChildren();
}






nsXFormsSelectFullAccessible::
  nsXFormsSelectFullAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsSelectableAccessible(aContent, aShell)
{
}

role
nsXFormsSelectFullAccessible::NativeRole()
{
  return roles::GROUPING;
}

void
nsXFormsSelectFullAccessible::CacheChildren()
{
  CacheSelectChildren();
}






nsXFormsItemCheckgroupAccessible::
  nsXFormsItemCheckgroupAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsSelectableItemAccessible(aContent, aShell)
{
}

role
nsXFormsItemCheckgroupAccessible::NativeRole()
{
  return roles::CHECKBUTTON;
}

PRUint64
nsXFormsItemCheckgroupAccessible::NativeState()
{
  PRUint64 state = nsXFormsSelectableItemAccessible::NativeState();

  if (IsSelected())
    state |= states::CHECKED;

  return state;
}

NS_IMETHODIMP
nsXFormsItemCheckgroupAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  if (IsSelected())
    aName.AssignLiteral("uncheck");
  else
    aName.AssignLiteral("check");

  return NS_OK;
}






nsXFormsItemRadiogroupAccessible::
  nsXFormsItemRadiogroupAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsSelectableItemAccessible(aContent, aShell)
{
}

role
nsXFormsItemRadiogroupAccessible::NativeRole()
{
  return roles::RADIOBUTTON;
}

PRUint64
nsXFormsItemRadiogroupAccessible::NativeState()
{
  PRUint64 state = nsXFormsSelectableItemAccessible::NativeState();

  if (IsSelected())
    state |= states::CHECKED;

  return state;
}

NS_IMETHODIMP
nsXFormsItemRadiogroupAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  aName.AssignLiteral("select");
  return NS_OK;
}






nsXFormsSelectComboboxAccessible::
  nsXFormsSelectComboboxAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsSelectableAccessible(aContent, aShell)
{
}

role
nsXFormsSelectComboboxAccessible::NativeRole()
{
  return roles::COMBOBOX;
}

PRUint64
nsXFormsSelectComboboxAccessible::NativeState()
{
  PRUint64 state = nsXFormsSelectableAccessible::NativeState();

  bool isOpen = false;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->IsDropmarkerOpen(DOMNode, &isOpen);
  NS_ENSURE_SUCCESS(rv, state);

  if (isOpen)
    state |= states::EXPANDED;
  else
    state |= states::COLLAPSED;

  return state | states::HASPOPUP | states::FOCUSABLE;
}

bool
nsXFormsSelectComboboxAccessible::GetAllowsAnonChildAccessibles()
{
  return true;
}






nsXFormsItemComboboxAccessible::
  nsXFormsItemComboboxAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsSelectableItemAccessible(aContent, aShell)
{
}

role
nsXFormsItemComboboxAccessible::NativeRole()
{
  return roles::LISTITEM;
}

PRUint64
nsXFormsItemComboboxAccessible::NativeState()
{
  PRUint64 state = nsXFormsSelectableItemAccessible::NativeState();

  if (state & states::UNAVAILABLE)
    return state;

  state |= states::SELECTABLE;
  if (IsSelected())
    state |= states::SELECTED;

  return state;
}

NS_IMETHODIMP
nsXFormsItemComboboxAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  aName.AssignLiteral("select");
  return NS_OK;
}

