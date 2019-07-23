





































#include "nsXFormsFormControlsAccessible.h"



nsXFormsLabelAccessible::
  nsXFormsLabelAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsAccessible(aNode, aShell)
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
  nsXFormsOutputAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsAccessible(aNode, aShell)
{
}

nsresult
nsXFormsOutputAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_STATICTEXT;
  return NS_OK;
}



nsXFormsTriggerAccessible::
  nsXFormsTriggerAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsAccessible(aNode, aShell)
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
  if (aIndex == eAction_Click)
    return DoCommand();

  return NS_ERROR_INVALID_ARG;
}



nsXFormsInputAccessible::
  nsXFormsInputAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsEditableAccessible(aNode, aShell)
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

  return sXFormsService->Focus(mDOMNode);
}



nsXFormsInputBooleanAccessible::
  nsXFormsInputBooleanAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsAccessible(aNode, aShell)
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
  rv = sXFormsService->GetValue(mDOMNode, value);
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
  nsresult rv = sXFormsService->GetValue(mDOMNode, value);
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

  return DoCommand();
}



nsXFormsInputDateAccessible::
  nsXFormsInputDateAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsContainerAccessible(aNode, aShell)
{
}

nsresult
nsXFormsInputDateAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_DROPLIST;
  return NS_OK;
}



nsXFormsSecretAccessible::
  nsXFormsSecretAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsInputAccessible(aNode, aShell)
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
  nsXFormsRangeAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsAccessible(aNode, aShell)
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
  rv = sXFormsService->IsInRange(mDOMNode, &isInRange);
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
  nsresult rv = sXFormsService->GetRangeEnd(mDOMNode, value);
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
  nsresult rv = sXFormsService->GetRangeStart(mDOMNode, value);
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
  nsresult rv = sXFormsService->GetRangeStep(mDOMNode, value);
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
  nsresult rv = sXFormsService->GetValue(mDOMNode, value);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 error = NS_OK;
  *aCurrentValue = value.ToFloat(&error);
  return error;
}




nsXFormsSelectAccessible::
  nsXFormsSelectAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsContainerAccessible(aNode, aShell)
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
  rv = sXFormsService->IsInRange(mDOMNode, &isInRange);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isInRange == nsIXFormsUtilityService::STATE_OUT_OF_RANGE)
    *aState |= nsIAccessibleStates::STATE_INVALID;

  return NS_OK;
}





nsXFormsChoicesAccessible::
  nsXFormsChoicesAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsAccessible(aNode, aShell)
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
  nsXFormsSelectFullAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsSelectableAccessible(aNode, aShell)
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
  nsXFormsItemCheckgroupAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsSelectableItemAccessible(aNode, aShell)
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
  nsXFormsItemRadiogroupAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsSelectableItemAccessible(aNode, aShell)
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
  nsXFormsSelectComboboxAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsSelectableAccessible(aNode, aShell)
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
  rv = sXFormsService->IsDropmarkerOpen(mDOMNode, &isOpen);
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
  nsXFormsItemComboboxAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsSelectableItemAccessible(aNode, aShell)
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

