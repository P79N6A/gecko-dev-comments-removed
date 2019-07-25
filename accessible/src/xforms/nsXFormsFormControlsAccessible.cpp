





































#include "nsXFormsFormControlsAccessible.h"

#include "States.h"
#include "nsTextEquivUtils.h"





nsXFormsLabelAccessible::
  nsXFormsLabelAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsAccessible(aContent, aShell)
{
}

PRUint32
nsXFormsLabelAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_LABEL;
}

nsresult
nsXFormsLabelAccessible::GetNameInternal(nsAString& aName)
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsLabelAccessible::GetDescription(nsAString& aDescription)
{
  nsAutoString description;
  nsresult rv = nsTextEquivUtils::
    GetTextEquivFromIDRefs(this, nsAccessibilityAtoms::aria_describedby,
                           description);
  aDescription = description;
  return rv;
}






nsXFormsOutputAccessible::
  nsXFormsOutputAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsAccessible(aContent, aShell)
{
}

PRUint32
nsXFormsOutputAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_STATICTEXT;
}






nsXFormsTriggerAccessible::
  nsXFormsTriggerAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsAccessible(aContent, aShell)
{
}

PRUint32
nsXFormsTriggerAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PUSHBUTTON;
}

NS_IMETHODIMP
nsXFormsTriggerAccessible::GetValue(nsAString& aValue)
{
  aValue.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsTriggerAccessible::GetNumActions(PRUint8 *aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);

  *aCount = 1;
  return NS_OK;
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

PRUint32
nsXFormsInputAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_ENTRY;
}

NS_IMETHODIMP
nsXFormsInputAccessible::GetNumActions(PRUint8* aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);

  *aCount = 1;
  return NS_OK;
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

PRUint32
nsXFormsInputBooleanAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_CHECKBUTTON;
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

NS_IMETHODIMP
nsXFormsInputBooleanAccessible::GetNumActions(PRUint8 *aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);

  *aCount = 1;
  return NS_OK;
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

PRUint32
nsXFormsInputDateAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_DROPLIST;
}






nsXFormsSecretAccessible::
  nsXFormsSecretAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsInputAccessible(aContent, aShell)
{
}

PRUint32
nsXFormsSecretAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PASSWORD_TEXT;
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

PRUint32
nsXFormsRangeAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_SLIDER;
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

PRUint32
nsXFormsChoicesAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_GROUPING;
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

PRUint32
nsXFormsSelectFullAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_GROUPING;
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

PRUint32
nsXFormsItemCheckgroupAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_CHECKBUTTON;
}

PRUint64
nsXFormsItemCheckgroupAccessible::NativeState()
{
  PRUint64 state = nsXFormsSelectableItemAccessible::NativeState();

  if (IsItemSelected())
    state |= states::CHECKED;

  return state;
}

NS_IMETHODIMP
nsXFormsItemCheckgroupAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  if (IsItemSelected())
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

PRUint32
nsXFormsItemRadiogroupAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_RADIOBUTTON;
}

PRUint64
nsXFormsItemRadiogroupAccessible::NativeState()
{
  PRUint64 state = nsXFormsSelectableItemAccessible::NativeState();

  if (IsItemSelected())
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

PRUint32
nsXFormsSelectComboboxAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_COMBOBOX;
}

PRUint64
nsXFormsSelectComboboxAccessible::NativeState()
{
  PRUint64 state = nsXFormsSelectableAccessible::NativeState();

  PRBool isOpen = PR_FALSE;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->IsDropmarkerOpen(DOMNode, &isOpen);
  NS_ENSURE_SUCCESS(rv, state);

  if (isOpen)
    state |= states::EXPANDED;
  else
    state |= states::COLLAPSED;

  return state | states::HASPOPUP | states::FOCUSABLE;
}

PRBool
nsXFormsSelectComboboxAccessible::GetAllowsAnonChildAccessibles()
{
  return PR_TRUE;
}






nsXFormsItemComboboxAccessible::
  nsXFormsItemComboboxAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsSelectableItemAccessible(aContent, aShell)
{
}

PRUint32
nsXFormsItemComboboxAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_LISTITEM;
}

PRUint64
nsXFormsItemComboboxAccessible::NativeState()
{
  PRUint64 state = nsXFormsSelectableItemAccessible::NativeState();

  if (state & states::UNAVAILABLE)
    return state;

  state |= states::SELECTABLE;
  if (IsItemSelected())
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

