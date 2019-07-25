






































#include "nsHTMLLinkAccessible.h"

#include "nsCoreUtils.h"
#include "Role.h"
#include "States.h"

#include "nsDocAccessible.h"
#include "nsEventStates.h"
#include "mozilla/dom/Element.h"

using namespace mozilla::a11y;





nsHTMLLinkAccessible::
  nsHTMLLinkAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsHyperTextAccessibleWrap(aContent, aDoc)
{
}


NS_IMPL_ISUPPORTS_INHERITED1(nsHTMLLinkAccessible, nsHyperTextAccessibleWrap,
                             nsIAccessibleHyperLink)




role
nsHTMLLinkAccessible::NativeRole()
{
  return roles::LINK;
}

PRUint64
nsHTMLLinkAccessible::NativeState()
{
  PRUint64 states = nsHyperTextAccessibleWrap::NativeState();

  states  &= ~states::READONLY;

  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::name)) {
    
    
    
    states |= states::SELECTABLE;
  }

  return states;
}

PRUint64
nsHTMLLinkAccessible::NativeLinkState() const
{
  nsEventStates eventState = mContent->AsElement()->State();
  if (eventState.HasState(NS_EVENT_STATE_UNVISITED))
    return states::LINKED;

  if (eventState.HasState(NS_EVENT_STATE_VISITED))
    return states::LINKED | states::TRAVERSED;

  
  
  
  return nsCoreUtils::HasClickListener(mContent) ? states::LINKED : 0;
}

void
nsHTMLLinkAccessible::Value(nsString& aValue)
{
  aValue.Truncate();

  nsHyperTextAccessible::Value(aValue);
  if (!aValue.IsEmpty())
    return;
  
  nsIPresShell* presShell(mDoc->PresShell());
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  presShell->GetLinkLocation(DOMNode, aValue);
}

PRUint8
nsHTMLLinkAccessible::ActionCount()
{
  return IsLinked() ? 1 : nsHyperTextAccessible::ActionCount();
}

NS_IMETHODIMP
nsHTMLLinkAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  aName.Truncate();

  if (!IsLinked())
    return nsHyperTextAccessible::GetActionName(aIndex, aName);

  
  if (aIndex != eAction_Jump)
    return NS_ERROR_INVALID_ARG;

  aName.AssignLiteral("jump");
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLLinkAccessible::DoAction(PRUint8 aIndex)
{
  if (!IsLinked())
    return nsHyperTextAccessible::DoAction(aIndex);

  
  if (aIndex != eAction_Jump)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  DoCommand();
  return NS_OK;
}




bool
nsHTMLLinkAccessible::IsLink()
{
  
  return true;
}

already_AddRefed<nsIURI>
nsHTMLLinkAccessible::AnchorURIAt(PRUint32 aAnchorIndex)
{
  return aAnchorIndex == 0 ? mContent->GetHrefURI() : nsnull;
}




bool
nsHTMLLinkAccessible::IsLinked()
{
  if (IsDefunct())
    return false;

  nsEventStates state = mContent->AsElement()->State();
  return state.HasAtLeastOneOfStates(NS_EVENT_STATE_VISITED |
                                     NS_EVENT_STATE_UNVISITED);
}
