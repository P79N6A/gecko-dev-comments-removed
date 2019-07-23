






































#include "nsHTMLLinkAccessible.h"

#include "nsILink.h"




nsHTMLLinkAccessible::nsHTMLLinkAccessible(nsIDOMNode* aDomNode,
                                           nsIWeakReference* aShell):
  nsHyperTextAccessibleWrap(aDomNode, aShell)
{ 
}


NS_IMPL_ISUPPORTS_INHERITED1(nsHTMLLinkAccessible, nsHyperTextAccessibleWrap,
                             nsIAccessibleHyperLink)




NS_IMETHODIMP
nsHTMLLinkAccessible::GetName(nsAString& aName)
{ 
  aName.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  return AppendFlatStringFromSubtree(content, &aName);
}

NS_IMETHODIMP
nsHTMLLinkAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);

  *aRole = nsIAccessibleRole::ROLE_LINK;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLLinkAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!mDOMNode)
    return NS_OK;

  *aState  &= ~nsIAccessibleStates::STATE_READONLY;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  if (content && content->HasAttr(kNameSpaceID_None,
                                  nsAccessibilityAtoms::name)) {
    
    
    
    *aState |= nsIAccessibleStates::STATE_SELECTABLE;
  }

  nsCOMPtr<nsILink> link = do_QueryInterface(mDOMNode);
  NS_ENSURE_STATE(link);

  nsLinkState linkState;
  link->GetLinkState(linkState);
  if (linkState == eLinkState_NotLink) {
    
    return NS_OK;
  }

  *aState |= nsIAccessibleStates::STATE_LINKED;

  if (linkState == eLinkState_Visited)
    *aState |= nsIAccessibleStates::STATE_TRAVERSED;

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
  if (mDOMNode && presShell)
    return presShell->GetLinkLocation(mDOMNode, aValue);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLLinkAccessible::GetNumActions(PRUint8 *aNumActions)
{
  NS_ENSURE_ARG_POINTER(aNumActions);

  *aNumActions = 1;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLLinkAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  
  aName.Truncate();
  if (aIndex != eAction_Jump)
    return NS_ERROR_INVALID_ARG;

  aName.AssignLiteral("jump");
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLLinkAccessible::DoAction(PRUint8 aIndex)
{
  
  if (aIndex != eAction_Jump)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  return DoCommand(content);
}




NS_IMETHODIMP
nsHTMLLinkAccessible::GetURI(PRInt32 aIndex, nsIURI **aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  *aURI = nsnull;

  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsILink> link(do_QueryInterface(mDOMNode));
  NS_ENSURE_STATE(link);

  return link->GetHrefURI(aURI);
}
