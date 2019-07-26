




#include "nsXFormsWidgetsAccessible.h"

#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;





nsXFormsDropmarkerWidgetAccessible::
  nsXFormsDropmarkerWidgetAccessible(nsIContent* aContent,
                                     DocAccessible* aDoc) :
  LeafAccessible(aContent, aDoc)
{
}

role
nsXFormsDropmarkerWidgetAccessible::NativeRole()
{
  return roles::PUSHBUTTON;
}

uint64_t
nsXFormsDropmarkerWidgetAccessible::NativeState()
{
  bool isOpen = false;
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv = sXFormsService->IsDropmarkerOpen(DOMNode, &isOpen);
  NS_ENSURE_SUCCESS(rv, 0);

  return isOpen ? states::PRESSED: 0;
}

uint8_t
nsXFormsDropmarkerWidgetAccessible::ActionCount()
{
  return 1;
}

NS_IMETHODIMP
nsXFormsDropmarkerWidgetAccessible::GetActionName(uint8_t aIndex,
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
nsXFormsDropmarkerWidgetAccessible::DoAction(uint8_t aIndex)
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

uint64_t
nsXFormsComboboxPopupWidgetAccessible::NativeState()
{
  uint64_t state = nsXFormsAccessible::NativeState();

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

uint64_t
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

