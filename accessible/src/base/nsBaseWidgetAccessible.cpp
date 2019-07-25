






































#include "nsBaseWidgetAccessible.h"

#include "nsAccessibilityAtoms.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsHyperTextAccessibleWrap.h"

#include "nsIDOMNSHTMLElement.h"
#include "nsGUIEvent.h"
#include "nsILink.h"
#include "nsIFrame.h"
#include "nsINameSpaceManager.h"
#include "nsIURI.h"





nsLeafAccessible::
  nsLeafAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsLeafAccessible, nsAccessible)




nsresult
nsLeafAccessible::GetChildAtPoint(PRInt32 aX, PRInt32 aY,
                                  PRBool aDeepestChild,
                                  nsIAccessible **aChild)
{
  
  NS_ADDREF(*aChild = this);
  return NS_OK;
}




void
nsLeafAccessible::CacheChildren()
{
  
}






nsLinkableAccessible::
  nsLinkableAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell),
  mActionContent(nsnull),
  mIsLink(PR_FALSE),
  mIsOnclick(PR_FALSE)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsLinkableAccessible, nsAccessibleWrap)




NS_IMETHODIMP
nsLinkableAccessible::TakeFocus()
{
  nsAccessible *actionAcc = GetActionAccessible();
  if (actionAcc)
    return actionAcc->TakeFocus();

  return nsAccessibleWrap::TakeFocus();
}

nsresult
nsLinkableAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsAccessibleWrap::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  if (mIsLink) {
    *aState |= nsIAccessibleStates::STATE_LINKED;
    nsAccessible *actionAcc = GetActionAccessible();
    if (nsAccUtils::State(actionAcc) & nsIAccessibleStates::STATE_TRAVERSED)
      *aState |= nsIAccessibleStates::STATE_TRAVERSED;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsLinkableAccessible::GetValue(nsAString& aValue)
{
  aValue.Truncate();

  nsAccessible::GetValue(aValue);
  if (!aValue.IsEmpty())
    return NS_OK;

  if (mIsLink) {
    nsAccessible *actionAcc = GetActionAccessible();
    if (actionAcc)
      return actionAcc->GetValue(aValue);
  }

  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsLinkableAccessible::GetNumActions(PRUint8 *aNumActions)
{
  NS_ENSURE_ARG_POINTER(aNumActions);

  *aNumActions = mActionContent ? 1 : 0;
  return NS_OK;
}

NS_IMETHODIMP
nsLinkableAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  aName.Truncate();

  
  if (aIndex == eAction_Jump) {   
    if (mIsLink) {
      aName.AssignLiteral("jump");
      return NS_OK;
    }
    else if (mIsOnclick) {
      aName.AssignLiteral("click");
      return NS_OK;
    }
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
nsLinkableAccessible::DoAction(PRUint8 aIndex)
{
  if (aIndex != eAction_Jump)
    return NS_ERROR_INVALID_ARG;

  nsAccessible *actionAcc = GetActionAccessible();
  if (actionAcc)
    return actionAcc->DoAction(aIndex);
  
  return nsAccessibleWrap::DoAction(aIndex);
}

NS_IMETHODIMP
nsLinkableAccessible::GetKeyboardShortcut(nsAString& aKeyboardShortcut)
{
  aKeyboardShortcut.Truncate();

  nsAccessible *actionAcc = GetActionAccessible();
  if (actionAcc)
    return actionAcc->GetKeyboardShortcut(aKeyboardShortcut);

  return nsAccessible::GetKeyboardShortcut(aKeyboardShortcut);
}




PRBool
nsLinkableAccessible::Init()
{
  CacheActionContent();
  return nsAccessibleWrap::Init();
}

void
nsLinkableAccessible::Shutdown()
{
  mActionContent = nsnull;
  nsAccessibleWrap::Shutdown();
}




already_AddRefed<nsIURI>
nsLinkableAccessible::GetAnchorURI(PRUint32 aAnchorIndex)
{
  if (mIsLink) {
    nsAccessible* link = GetActionAccessible();
    if (link) {
      NS_ASSERTION(link->IsHyperLink(),
                   "nsIAccessibleHyperLink isn't implemented.");

      if (link->IsHyperLink())
        return link->GetAnchorURI(aAnchorIndex);
    }
  }

  return nsnull;
}




void
nsLinkableAccessible::CacheActionContent()
{
  nsIContent* walkUpContent = mContent;
  PRBool isOnclick = nsCoreUtils::HasClickListener(walkUpContent);

  if (isOnclick) {
    mActionContent = walkUpContent;
    mIsOnclick = PR_TRUE;
    return;
  }

  while ((walkUpContent = walkUpContent->GetParent())) {
    isOnclick = nsCoreUtils::HasClickListener(walkUpContent);
    nsAccessible *walkUpAcc =
      GetAccService()->GetAccessibleInWeakShell(walkUpContent, mWeakShell);

    if (nsAccUtils::Role(walkUpAcc) == nsIAccessibleRole::ROLE_LINK &&
        nsAccUtils::State(walkUpAcc) & nsIAccessibleStates::STATE_LINKED) {
      mIsLink = PR_TRUE;
      mActionContent = walkUpContent;
      return;
    }

    if (isOnclick) {
      mActionContent = walkUpContent;
      mIsOnclick = PR_TRUE;
      return;
    }
  }
}

nsAccessible *
nsLinkableAccessible::GetActionAccessible() const
{
  
  
  
  
  if (!mActionContent || mContent == mActionContent)
    return nsnull;

  return GetAccService()->GetAccessibleInWeakShell(mActionContent, mWeakShell);
}





nsEnumRoleAccessible::
  nsEnumRoleAccessible(nsIContent *aNode, nsIWeakReference *aShell,
                       PRUint32 aRole) :
  nsAccessibleWrap(aNode, aShell), mRole(aRole)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsEnumRoleAccessible, nsAccessible)

nsresult
nsEnumRoleAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = mRole;
  return NS_OK;
}
