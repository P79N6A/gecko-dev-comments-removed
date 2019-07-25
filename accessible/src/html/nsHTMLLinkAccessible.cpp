






































#include "nsHTMLLinkAccessible.h"

#include "nsCoreUtils.h"

#include "nsIEventStateManager.h"





nsHTMLLinkAccessible::
  nsHTMLLinkAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aContent, aShell)
{
}


NS_IMPL_ISUPPORTS_INHERITED1(nsHTMLLinkAccessible, nsHyperTextAccessibleWrap,
                             nsIAccessibleHyperLink)




PRUint32
nsHTMLLinkAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_LINK;
}

nsresult
nsHTMLLinkAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetStateInternal(aState,
                                                            aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState  &= ~nsIAccessibleStates::STATE_READONLY;

  if (mContent->HasAttr(kNameSpaceID_None, nsAccessibilityAtoms::name)) {
    
    
    
    *aState |= nsIAccessibleStates::STATE_SELECTABLE;
  }

  nsEventStates state = mContent->IntrinsicState();
  if (state.HasAtLeastOneOfStates(NS_EVENT_STATE_VISITED |
                                  NS_EVENT_STATE_UNVISITED)) {
    *aState |= nsIAccessibleStates::STATE_LINKED;

    if (state.HasState(NS_EVENT_STATE_VISITED))
      *aState |= nsIAccessibleStates::STATE_TRAVERSED;

    return NS_OK;
  }

  
  
  
  if (nsCoreUtils::HasClickListener(mContent))
    *aState |= nsIAccessibleStates::STATE_LINKED;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLLinkAccessible::GetValue(nsAString& aValue)
{
  aValue.Truncate();

  nsresult rv = nsHyperTextAccessible::GetValue(aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aValue.IsEmpty())
    return NS_OK;
  
  nsCOMPtr<nsIPresShell> presShell(do_QueryReferent(mWeakShell));
  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  return presShell->GetLinkLocation(DOMNode, aValue);
}

NS_IMETHODIMP
nsHTMLLinkAccessible::GetNumActions(PRUint8 *aNumActions)
{
  NS_ENSURE_ARG_POINTER(aNumActions);

  if (!IsLinked())
    return nsHyperTextAccessible::GetNumActions(aNumActions);

  *aNumActions = 1;
  return NS_OK;
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
nsHTMLLinkAccessible::IsHyperLink()
{
  
  return true;
}

already_AddRefed<nsIURI>
nsHTMLLinkAccessible::GetAnchorURI(PRUint32 aAnchorIndex)
{
  return aAnchorIndex == 0 ? mContent->GetHrefURI() : nsnull;
}




PRBool
nsHTMLLinkAccessible::IsLinked()
{
  if (IsDefunct())
    return PR_FALSE;

  nsEventStates state = mContent->IntrinsicState();
  return state.HasAtLeastOneOfStates(NS_EVENT_STATE_VISITED |
                                     NS_EVENT_STATE_UNVISITED);
}
