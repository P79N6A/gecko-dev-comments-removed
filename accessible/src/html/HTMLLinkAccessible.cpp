




#include "HTMLLinkAccessible.h"

#include "nsCoreUtils.h"
#include "DocAccessible.h"
#include "Role.h"
#include "States.h"

#include "nsContentUtils.h"
#include "nsEventStates.h"
#include "mozilla/dom/Element.h"

using namespace mozilla::a11y;





HTMLLinkAccessible::
  HTMLLinkAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
}


NS_IMPL_ISUPPORTS_INHERITED1(HTMLLinkAccessible, HyperTextAccessibleWrap,
                             nsIAccessibleHyperLink)




role
HTMLLinkAccessible::NativeRole()
{
  return roles::LINK;
}

uint64_t
HTMLLinkAccessible::NativeState()
{
  return HyperTextAccessibleWrap::NativeState() & ~states::READONLY;
}

uint64_t
HTMLLinkAccessible::NativeLinkState() const
{
  nsEventStates eventState = mContent->AsElement()->State();
  if (eventState.HasState(NS_EVENT_STATE_UNVISITED))
    return states::LINKED;

  if (eventState.HasState(NS_EVENT_STATE_VISITED))
    return states::LINKED | states::TRAVERSED;

  
  
  
  return nsCoreUtils::HasClickListener(mContent) ? states::LINKED : 0;
}

uint64_t
HTMLLinkAccessible::NativeInteractiveState() const
{
  uint64_t state = HyperTextAccessibleWrap::NativeInteractiveState();

  
  
  
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::name))
    state |= states::SELECTABLE;

  return state;
}

void
HTMLLinkAccessible::Value(nsString& aValue)
{
  aValue.Truncate();

  HyperTextAccessible::Value(aValue);
  if (aValue.IsEmpty())
    nsContentUtils::GetLinkLocation(mContent->AsElement(), aValue);
}

uint8_t
HTMLLinkAccessible::ActionCount()
{
  return IsLinked() ? 1 : HyperTextAccessible::ActionCount();
}

NS_IMETHODIMP
HTMLLinkAccessible::GetActionName(uint8_t aIndex, nsAString& aName)
{
  aName.Truncate();

  if (!IsLinked())
    return HyperTextAccessible::GetActionName(aIndex, aName);

  
  if (aIndex != eAction_Jump)
    return NS_ERROR_INVALID_ARG;

  aName.AssignLiteral("jump");
  return NS_OK;
}

NS_IMETHODIMP
HTMLLinkAccessible::DoAction(uint8_t aIndex)
{
  if (!IsLinked())
    return HyperTextAccessible::DoAction(aIndex);

  
  if (aIndex != eAction_Jump)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  DoCommand();
  return NS_OK;
}




bool
HTMLLinkAccessible::IsLink()
{
  
  return true;
}

already_AddRefed<nsIURI>
HTMLLinkAccessible::AnchorURIAt(uint32_t aAnchorIndex)
{
  return aAnchorIndex == 0 ? mContent->GetHrefURI() : nullptr;
}




bool
HTMLLinkAccessible::IsLinked()
{
  if (IsDefunct())
    return false;

  nsEventStates state = mContent->AsElement()->State();
  return state.HasAtLeastOneOfStates(NS_EVENT_STATE_VISITED |
                                     NS_EVENT_STATE_UNVISITED);
}
