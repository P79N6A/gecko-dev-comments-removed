







































#include "nsAccessibilityAtoms.h"
#include "nsCoreUtils.h"
#include "nsAccUtils.h"
#include "nsBaseWidgetAccessible.h"
#include "nsIDOMXULDescriptionElement.h"
#include "nsINameSpaceManager.h"
#include "nsString.h"
#include "nsXULTextAccessible.h"
#include "nsNetUtil.h"




nsXULTextAccessible::nsXULTextAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell):
nsHyperTextAccessibleWrap(aDomNode, aShell)
{ 
}

nsresult
nsXULTextAccessible::GetNameInternal(nsAString& aName)
{ 
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));

  
  
  content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value, aName);
  return NS_OK;
}

nsresult
nsXULTextAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_LABEL;
  return NS_OK;
}

nsresult
nsXULTextAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetStateInternal(aState,
                                                            aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  
  
  *aState |= nsIAccessibleStates::STATE_READONLY;
  return NS_OK;
}

NS_IMETHODIMP
nsXULTextAccessible::GetRelationByType(PRUint32 aRelationType,
                                       nsIAccessibleRelation **aRelation)
{
  nsresult rv =
    nsHyperTextAccessibleWrap::GetRelationByType(aRelationType, aRelation);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIContent *content = nsCoreUtils::GetRoleContent(mDOMNode);
  if (!content)
    return NS_OK;

  if (aRelationType == nsIAccessibleRelation::RELATION_LABEL_FOR) {
    
    nsIContent *parent = content->GetParent();
    if (parent && parent->Tag() == nsAccessibilityAtoms::caption) {
      nsCOMPtr<nsIAccessible> parentAccessible;
      GetParent(getter_AddRefs(parentAccessible));
      if (nsAccUtils::Role(parentAccessible) == nsIAccessibleRole::ROLE_GROUPING)
        return nsRelUtils::
          AddTarget(aRelationType, aRelation, parentAccessible);
    }
  }

  return NS_OK;
}




nsXULTooltipAccessible::nsXULTooltipAccessible(nsIDOMNode* aDomNode, nsIWeakReference* aShell):
nsLeafAccessible(aDomNode, aShell)
{ 
}

nsresult
nsXULTooltipAccessible::GetStateInternal(PRUint32 *aState,
                                         PRUint32 *aExtraState)
{
  nsresult rv = nsLeafAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState &= ~nsIAccessibleStates::STATE_FOCUSABLE;
  *aState |= nsIAccessibleStates::STATE_READONLY;
  return NS_OK;
}

nsresult
nsXULTooltipAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_TOOLTIP;
  return NS_OK;
}




nsXULLinkAccessible::
  nsXULLinkAccessible(nsIDOMNode *aDomNode, nsIWeakReference *aShell):
  nsHyperTextAccessibleWrap(aDomNode, aShell)
{
}


NS_IMPL_ISUPPORTS_INHERITED1(nsXULLinkAccessible, nsHyperTextAccessibleWrap,
                             nsIAccessibleHyperLink)




NS_IMETHODIMP
nsXULLinkAccessible::GetValue(nsAString& aValue)
{
  aValue.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::href, aValue);
  return NS_OK;
}

nsresult
nsXULLinkAccessible::GetNameInternal(nsAString& aName)
{
  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value, aName);
  if (!aName.IsEmpty())
    return NS_OK;

  return nsTextEquivUtils::GetNameFromSubtree(this, aName);
}

nsresult
nsXULLinkAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_LINK;
  return NS_OK;
}


nsresult
nsXULLinkAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetStateInternal(aState,
                                                            aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  *aState |= nsIAccessibleStates::STATE_LINKED;
  return NS_OK;
}

NS_IMETHODIMP
nsXULLinkAccessible::GetNumActions(PRUint8 *aNumActions)
{
  NS_ENSURE_ARG_POINTER(aNumActions);
  
  *aNumActions = 1;
  return NS_OK;
}

NS_IMETHODIMP
nsXULLinkAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  aName.Truncate();

  if (aIndex != eAction_Jump)
    return NS_ERROR_INVALID_ARG;
  
  aName.AssignLiteral("jump");
  return NS_OK;
}

NS_IMETHODIMP
nsXULLinkAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Jump)
    return NS_ERROR_INVALID_ARG;

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  return DoCommand(content);
}




NS_IMETHODIMP
nsXULLinkAccessible::GetURI(PRInt32 aIndex, nsIURI **aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  *aURI = nsnull;

  if (aIndex != 0)
    return NS_ERROR_INVALID_ARG;

  nsAutoString href;
  nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMNode));
  content->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::href, href);

  nsCOMPtr<nsIURI> baseURI = content->GetBaseURI();
  nsCOMPtr<nsIDocument> document = content->GetOwnerDoc();
  return NS_NewURI(aURI, href,
                   document ? document->GetDocumentCharacterSet().get() : nsnull,
                   baseURI);
}
