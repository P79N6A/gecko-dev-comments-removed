





































#include "nsXFormsWidgetsAccessible.h"





nsXFormsDropmarkerWidgetAccessible::
  nsXFormsDropmarkerWidgetAccessible(nsIContent *aContent,
                                     nsIWeakReference *aShell) :
  nsLeafAccessible(aContent, aShell)
{
}

PRUint32
nsXFormsDropmarkerWidgetAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PUSHBUTTON;
}

nsresult
nsXFormsDropmarkerWidgetAccessible::GetStateInternal(PRUint32 *aState,
                                                     PRUint32 *aExtraState)
{
  NS_ENSURE_ARG_POINTER(aState);
  *aState = 0;

  if (IsDefunct()) {
    if (aExtraState)
      *aExtraState = nsIAccessibleStates::EXT_STATE_DEFUNCT;

    return NS_OK_DEFUNCT_OBJECT;
  }

  if (aExtraState)
    *aExtraState = 0;

  PRBool isOpen = PR_FALSE;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->IsDropmarkerOpen(DOMNode, &isOpen);
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
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->IsDropmarkerOpen(DOMNode, &isOpen);
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

  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  return sXFormsService->ToggleDropmarkerState(DOMNode);
}






nsXFormsCalendarWidgetAccessible::
nsXFormsCalendarWidgetAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

PRUint32
nsXFormsCalendarWidgetAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_CALENDAR;
}






nsXFormsComboboxPopupWidgetAccessible::
  nsXFormsComboboxPopupWidgetAccessible(nsIContent *aContent,
                                        nsIWeakReference *aShell) :
  nsXFormsAccessible(aContent, aShell)
{
}

PRUint32
nsXFormsComboboxPopupWidgetAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_LIST;
}

nsresult
nsXFormsComboboxPopupWidgetAccessible::GetStateInternal(PRUint32 *aState,
                                                        PRUint32 *aExtraState)
{
  NS_ENSURE_ARG_POINTER(aState);

  nsresult rv = nsXFormsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  PRBool isOpen = PR_FALSE;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  rv = sXFormsService->IsDropmarkerOpen(DOMNode, &isOpen);
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

nsresult
nsXFormsComboboxPopupWidgetAccessible::GetNameInternal(nsAString& aName)
{
  
  
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
  nsCOMPtr<nsIDOMNode> parent = do_QueryInterface(mContent->GetNodeParent());
  
  CacheSelectChildren(parent);
}

