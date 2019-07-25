





































#include "nsXFormsWidgetsAccessible.h"

#include "States.h"





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

PRUint64
nsXFormsDropmarkerWidgetAccessible::NativeState()
{
  PRBool isOpen = PR_FALSE;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->IsDropmarkerOpen(DOMNode, &isOpen);
  NS_ENSURE_SUCCESS(rv, 0);

  return isOpen ? states::PRESSED: 0;
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

PRUint64
nsXFormsComboboxPopupWidgetAccessible::NativeState()
{
  PRUint64 state = nsXFormsAccessible::NativeState();

  PRBool isOpen = PR_FALSE;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->IsDropmarkerOpen(DOMNode, &isOpen);
  NS_ENSURE_SUCCESS(rv, state);

  state |= states::FOCUSABLE;

  if (isOpen)
    state = states::FLOATING;
  else
    state = states::INVISIBLE;

  return state;
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

