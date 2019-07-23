





































#include "nsXFormsWidgetsAccessible.h"
 


nsXFormsDropmarkerWidgetAccessible::
nsXFormsDropmarkerWidgetAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
  nsLeafAccessible(aNode, aShell)
{
}

NS_IMETHODIMP
nsXFormsDropmarkerWidgetAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);

  *aRole = nsIAccessibleRole::ROLE_PUSHBUTTON;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsDropmarkerWidgetAccessible::GetState(PRUint32 *aState)
{
  NS_ENSURE_ARG_POINTER(aState);

  *aState = 0;

  PRBool isOpen = PR_FALSE;
  nsresult rv = sXFormsService->IsDropmarkerOpen(mDOMNode, &isOpen);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isOpen)
    *aState = nsIAccessibleStates::STATE_PRESSED;

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsDropmarkerWidgetAccessible::GetNumActions(PRUint8 *aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);

  *aCount = 1;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsDropmarkerWidgetAccessible::GetActionName(PRUint8 aIndex,
                                                  nsAString& aName)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  PRBool isOpen = PR_FALSE;
  nsresult rv = sXFormsService->IsDropmarkerOpen(mDOMNode, &isOpen);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isOpen)
    aName.AssignLiteral("close");
  else
    aName.AssignLiteral("open");

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsDropmarkerWidgetAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  return sXFormsService->ToggleDropmarkerState(mDOMNode);
}




nsXFormsCalendarWidgetAccessible::
nsXFormsCalendarWidgetAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
  nsAccessibleWrap(aNode, aShell)
{
}

NS_IMETHODIMP
nsXFormsCalendarWidgetAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);

  *aRole = nsIAccessibleRole::ROLE_CALENDAR;
  return NS_OK;
}




nsXFormsComboboxPopupWidgetAccessible::
  nsXFormsComboboxPopupWidgetAccessible(nsIDOMNode *aNode, nsIWeakReference *aShell):
  nsXFormsAccessible(aNode, aShell)
{
}

NS_IMETHODIMP
nsXFormsComboboxPopupWidgetAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);

  *aRole = nsIAccessibleRole::ROLE_LIST;
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsComboboxPopupWidgetAccessible::GetState(PRUint32 *aState)
{
  NS_ENSURE_ARG_POINTER(aState);

  nsXFormsAccessible::GetState(aState);

  PRBool isOpen = PR_FALSE;
  nsresult rv = sXFormsService->IsDropmarkerOpen(mDOMNode, &isOpen);
  NS_ENSURE_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_FOCUSABLE;

  if (isOpen)
    *aState = nsIAccessibleStates::STATE_FLOATING;
  else
    *aState = nsIAccessibleStates::STATE_INVISIBLE;

  return NS_OK;
}

NS_IMETHODIMP
nsXFormsComboboxPopupWidgetAccessible::GetValue(nsAString& aValue)
{
  aValue.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsComboboxPopupWidgetAccessible::GetName(nsAString& aName)
{
  aName.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsXFormsComboboxPopupWidgetAccessible::GetDescription(nsAString& aDescription)
{
  aDescription.Truncate();
  return NS_OK;
}

void
nsXFormsComboboxPopupWidgetAccessible::CacheChildren()
{
  nsCOMPtr<nsIDOMNode> parent;
  mDOMNode->GetParentNode(getter_AddRefs(parent));

  
  CacheSelectChildren(parent);
}

