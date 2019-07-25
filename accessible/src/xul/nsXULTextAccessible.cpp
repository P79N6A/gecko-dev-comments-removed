







































#include "nsXULTextAccessible.h"

#include "nsAccessibilityAtoms.h"
#include "nsAccUtils.h"
#include "nsBaseWidgetAccessible.h"
#include "nsCoreUtils.h"
#include "nsRelUtils.h"
#include "nsTextEquivUtils.h"
#include "States.h"

#include "nsIDOMXULDescriptionElement.h"
#include "nsINameSpaceManager.h"
#include "nsString.h"
#include "nsNetUtil.h"





nsXULTextAccessible::
  nsXULTextAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aContent, aShell)
{
}

nsresult
nsXULTextAccessible::GetNameInternal(nsAString& aName)
{
  
  
  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value, aName);
  return NS_OK;
}

PRUint32
nsXULTextAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_LABEL;
}

PRUint64
nsXULTextAccessible::NativeState()
{
  
  
  return nsHyperTextAccessibleWrap::NativeState() | states::READONLY;
}

NS_IMETHODIMP
nsXULTextAccessible::GetRelationByType(PRUint32 aRelationType,
                                       nsIAccessibleRelation **aRelation)
{
  nsresult rv =
    nsHyperTextAccessibleWrap::GetRelationByType(aRelationType, aRelation);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aRelationType == nsIAccessibleRelation::RELATION_LABEL_FOR) {
    
    nsIContent *parent = mContent->GetParent();
    if (parent && parent->Tag() == nsAccessibilityAtoms::caption) {
      nsAccessible* parent = GetParent();
      if (parent && parent->Role() == nsIAccessibleRole::ROLE_GROUPING)
        return nsRelUtils::AddTarget(aRelationType, aRelation, parent);
    }
  }

  return NS_OK;
}






nsXULTooltipAccessible::
  nsXULTooltipAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsLeafAccessible(aContent, aShell)
{
}

PRUint64
nsXULTooltipAccessible::NativeState()
{
  PRUint64 states = nsLeafAccessible::NativeState();

  states &= ~states::FOCUSABLE;
  states |= states::READONLY;
  return states;
}

PRUint32
nsXULTooltipAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_TOOLTIP;
}






nsXULLinkAccessible::
  nsXULLinkAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsHyperTextAccessibleWrap(aContent, aShell)
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

  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::href, aValue);
  return NS_OK;
}

nsresult
nsXULLinkAccessible::GetNameInternal(nsAString& aName)
{
  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::value, aName);
  if (!aName.IsEmpty())
    return NS_OK;

  return nsTextEquivUtils::GetNameFromSubtree(this, aName);
}

PRUint32
nsXULLinkAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_LINK;
}


PRUint64
nsXULLinkAccessible::NativeState()
{
  return nsHyperTextAccessible::NativeState() | states::LINKED;
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

  DoCommand();
  return NS_OK;
}




bool
nsXULLinkAccessible::IsHyperLink()
{
  
  return true;
}

PRUint32
nsXULLinkAccessible::StartOffset()
{
  
  
  
  
  
  if (nsAccessible::IsHyperLink())
    return nsAccessible::StartOffset();
  return IndexInParent();
}

PRUint32
nsXULLinkAccessible::EndOffset()
{
  if (nsAccessible::IsHyperLink())
    return nsAccessible::EndOffset();
  return IndexInParent() + 1;
}

already_AddRefed<nsIURI>
nsXULLinkAccessible::AnchorURIAt(PRUint32 aAnchorIndex)
{
  if (aAnchorIndex != 0)
    return nsnull;

  nsAutoString href;
  mContent->GetAttr(kNameSpaceID_None, nsAccessibilityAtoms::href, href);

  nsCOMPtr<nsIURI> baseURI = mContent->GetBaseURI();
  nsIDocument* document = mContent->GetOwnerDoc();

  nsIURI* anchorURI = nsnull;
  NS_NewURI(&anchorURI, href,
            document ? document->GetDocumentCharacterSet().get() : nsnull,
            baseURI);

  return anchorURI;
}
