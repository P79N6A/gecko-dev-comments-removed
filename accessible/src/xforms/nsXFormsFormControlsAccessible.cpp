





































#include "nsXFormsFormControlsAccessible.h"

#include "nsTextEquivUtils.h"





nsXFormsLabelAccessible::
  nsXFormsLabelAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsAccessible(aContent, aShell)
{
}

nsresult
nsXFormsLabelAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_LABEL;
  return NS_OK;
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

nsresult
nsXFormsOutputAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_STATICTEXT;
  return NS_OK;
}






nsXFormsTriggerAccessible::
  nsXFormsTriggerAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsAccessible(aContent, aShell)
{
}

nsresult
nsXFormsTriggerAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PUSHBUTTON;
  return NS_OK;
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

nsresult
nsXFormsInputAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_ENTRY;
  return NS_OK;
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

nsresult
nsXFormsInputBooleanAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_CHECKBUTTON;
  return NS_OK;
}

nsresult
nsXFormsInputBooleanAccessible::GetStateInternal(PRUint32 *aState,
                                                 PRUint32 *aExtraState)
{
  nsresult rv = nsXFormsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  nsAutoString value;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  rv = sXFormsService->GetValue(DOMNode, value);
  NS_ENSURE_SUCCESS(rv, rv);

  if (value.EqualsLiteral("true"))
    *aState |= nsIAccessibleStates::STATE_CHECKED;

  return NS_OK;
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

nsresult
nsXFormsInputDateAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_DROPLIST;
  return NS_OK;
}






nsXFormsSecretAccessible::
  nsXFormsSecretAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsInputAccessible(aContent, aShell)
{
}

nsresult
nsXFormsSecretAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_PASSWORD_TEXT;
  return NS_OK;
}

nsresult
nsXFormsSecretAccessible::GetStateInternal(PRUint32 *aState,
                                           PRUint32 *aExtraState)
{
  nsresult rv = nsXFormsInputAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_PROTECTED;
  return NS_OK;
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

nsresult
nsXFormsRangeAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_SLIDER;
  return NS_OK;
}

nsresult
nsXFormsRangeAccessible::GetStateInternal(PRUint32 *aState,
                                          PRUint32 *aExtraState)
{
  nsresult rv = nsXFormsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  PRUint32 isInRange = nsIXFormsUtilityService::STATE_NOT_A_RANGE;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  rv = sXFormsService->IsInRange(DOMNode, &isInRange);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isInRange == nsIXFormsUtilityService::STATE_OUT_OF_RANGE)
    *aState |= nsIAccessibleStates::STATE_INVALID;

  return NS_OK;
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
  *aMaximumValue = value.ToFloat(&error);
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
  *aMinimumValue = value.ToFloat(&error);
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
  *aMinimumIncrement = value.ToFloat(&error);
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
  *aCurrentValue = value.ToFloat(&error);
  return error;
}






nsXFormsSelectAccessible::
  nsXFormsSelectAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsContainerAccessible(aContent, aShell)
{
}

nsresult
nsXFormsSelectAccessible::GetStateInternal(PRUint32 *aState,
                                           PRUint32 *aExtraState)
{
  nsresult rv = nsXFormsContainerAccessible::GetStateInternal(aState,
                                                              aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  PRUint32 isInRange = nsIXFormsUtilityService::STATE_NOT_A_RANGE;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  rv = sXFormsService->IsInRange(DOMNode, &isInRange);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isInRange == nsIXFormsUtilityService::STATE_OUT_OF_RANGE)
    *aState |= nsIAccessibleStates::STATE_INVALID;

  return NS_OK;
}






nsXFormsChoicesAccessible::
  nsXFormsChoicesAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXFormsAccessible(aContent, aShell)
{
}

nsresult
nsXFormsChoicesAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_GROUPING;
  return NS_OK;
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

nsresult
nsXFormsSelectFullAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_GROUPING;
  return NS_OK;
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

nsresult
nsXFormsItemCheckgroupAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_CHECKBUTTON;
  return NS_OK;
}

nsresult
nsXFormsItemCheckgroupAccessible::GetStateInternal(PRUint32 *aState,
                                                   PRUint32 *aExtraState)
{
  nsresult rv = nsXFormsSelectableItemAccessible::GetStateInternal(aState,
                                                                   aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  if (IsItemSelected())
    *aState |= nsIAccessibleStates::STATE_CHECKED;

  return NS_OK;
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

nsresult
nsXFormsItemRadiogroupAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_RADIOBUTTON;
  return NS_OK;
}

nsresult
nsXFormsItemRadiogroupAccessible::GetStateInternal(PRUint32 *aState,
                                                   PRUint32 *aExtraState)
{
  nsresult rv = nsXFormsSelectableItemAccessible::GetStateInternal(aState,
                                                                   aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  if (IsItemSelected())
    *aState |= nsIAccessibleStates::STATE_CHECKED;

  return NS_OK;
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

nsresult
nsXFormsSelectComboboxAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_COMBOBOX;
  return NS_OK;
}

nsresult
nsXFormsSelectComboboxAccessible::GetStateInternal(PRUint32 *aState,
                                                   PRUint32 *aExtraState)
{
  nsresult rv = nsXFormsSelectableAccessible::GetStateInternal(aState,
                                                               aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  PRBool isOpen = PR_FALSE;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  rv = sXFormsService->IsDropmarkerOpen(DOMNode, &isOpen);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isOpen)
    *aState = nsIAccessibleStates::STATE_EXPANDED;
  else
    *aState = nsIAccessibleStates::STATE_COLLAPSED;

  *aState |= nsIAccessibleStates::STATE_HASPOPUP |
             nsIAccessibleStates::STATE_FOCUSABLE;
  return NS_OK;
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

nsresult
nsXFormsItemComboboxAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_LISTITEM;
  return NS_OK;
}

nsresult
nsXFormsItemComboboxAccessible::GetStateInternal(PRUint32 *aState,
                                                 PRUint32 *aExtraState)
{
  nsresult rv = nsXFormsSelectableItemAccessible::GetStateInternal(aState,
                                                                   aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  if (*aState & nsIAccessibleStates::STATE_UNAVAILABLE)
    return NS_OK;

  *aState |= nsIAccessibleStates::STATE_SELECTABLE;
  if (IsItemSelected())
    *aState |= nsIAccessibleStates::STATE_SELECTED;

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsItemComboboxAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  aName.AssignLiteral("select");
  return NS_OK;
}

