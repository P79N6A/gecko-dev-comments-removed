




#include "nsXFormsWidgetsAccessible.h"

#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;





nsXFormsDropmarkerWidgetAccessible::
  nsXFormsDropmarkerWidgetAccessible(nsIContent* aContent,
                                     DocAccessible* aDoc) :
  nsLeafAccessible(aContent, aDoc)
{
}

role
nsXFormsDropmarkerWidgetAccessible::NativeRole()
{
  return roles::PUSHBUTTON;
}

PRUint64
nsXFormsDropmarkerWidgetAccessible::NativeState()
{
  bool isOpen = false;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->IsDropmarkerOpen(DOMNode, &isOpen);
  NS_ENSURE_SUCCESS(rv, 0);

  return isOpen ? states::PRESSED: 0;
}

PRUint8
nsXFormsDropmarkerWidgetAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
nsXFormsDropmarkerWidgetAccessible::GetActionName(PRUint8 aIndex,
                                                  nsAString& aName)
{
  if (aIndex != eAction_Click)
    return NS_ERROR_INVALID_ARG;

  bool isOpen = false;
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
  nsXFormsCalendarWidgetAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
}

role
nsXFormsCalendarWidgetAccessible::NativeRole()
{
  return roles::CALENDAR;
}






nsXFormsComboboxPopupWidgetAccessible::
  nsXFormsComboboxPopupWidgetAccessible(nsIContent* aContent,
                                        DocAccessible* aDoc) :
  nsXFormsAccessible(aContent, aDoc)
{
}

role
nsXFormsComboboxPopupWidgetAccessible::NativeRole()
{
  return roles::LIST;
}

PRUint64
nsXFormsComboboxPopupWidgetAccessible::NativeState()
{
  PRUint64 state = nsXFormsAccessible::NativeState();

  bool isOpen = false;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->IsDropmarkerOpen(DOMNode, &isOpen);
  NS_ENSURE_SUCCESS(rv, state);

  if (isOpen)
    state = states::FLOATING;
  else
    state = states::INVISIBLE;

  return state;
}

PRUint64
nsXFormsComboboxPopupWidgetAccessible::NativeInteractiveState() const
{
  return NativelyUnavailable() ? states::UNAVAILABLE : states::FOCUSABLE;
}

nsresult
nsXFormsComboboxPopupWidgetAccessible::GetNameInternal(nsAString& aName)
{
  
  
  return NS_OK;
}

void
nsXFormsComboboxPopupWidgetAccessible::Description(nsString& aDescription)
{
  aDescription.Truncate();
}

void
nsXFormsComboboxPopupWidgetAccessible::Value(nsString& aValue)
{
  aValue.Truncate();
}

void
nsXFormsComboboxPopupWidgetAccessible::CacheChildren()
{
  nsCOMPtr<nsIDOMNode> parent = do_QueryInterface(mContent->GetNodeParent());
  
  CacheSelectChildren(parent);
}

