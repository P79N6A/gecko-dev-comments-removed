




#include "HTMLLinkAccessible.h"

#include "nsCoreUtils.h"
#include "DocAccessible.h"
#include "Role.h"
#include "States.h"

#include "nsContentUtils.h"
#include "mozilla/EventStates.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;
using namespace mozilla::a11y;





HTMLLinkAccessible::
  HTMLLinkAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  HyperTextAccessibleWrap(aContent, aDoc)
{
}


NS_IMPL_ISUPPORTS_INHERITED(HTMLLinkAccessible, HyperTextAccessibleWrap,
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
  EventStates eventState = mContent->AsElement()->State();
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

void
HTMLLinkAccessible::ActionNameAt(uint8_t aIndex, nsAString& aName)
{
  aName.Truncate();

  if (!IsLinked()) {
    HyperTextAccessible::ActionNameAt(aIndex, aName);
    return;
  }

  
  if (aIndex == eAction_Jump)
    aName.AssignLiteral("jump");
}

bool
HTMLLinkAccessible::DoAction(uint8_t aIndex)
{
  if (!IsLinked())
    return HyperTextAccessible::DoAction(aIndex);

  
  if (aIndex != eAction_Jump)
    return false;

  DoCommand();
  return true;
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
HTMLLinkAccessible::IsLinked() const
{
  EventStates state = mContent->AsElement()->State();
  return state.HasAtLeastOneOfStates(NS_EVENT_STATE_VISITED |
                                     NS_EVENT_STATE_UNVISITED);
}
