






































#include "nsBaseWidgetAccessible.h"
#include "nsAccessibilityAtoms.h"
#include "nsIAccessibilityService.h"
#include "nsIAccessibleDocument.h"
#include "nsAccessibleWrap.h"
#include "nsCoreUtils.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsGUIEvent.h"
#include "nsHyperTextAccessibleWrap.h"
#include "nsILink.h"
#include "nsIFrame.h"
#include "nsINameSpaceManager.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsIURI.h"





nsLeafAccessible::nsLeafAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell):
nsAccessibleWrap(aNode, aShell)
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
  
  mAccChildCount = IsDefunct() ? eChildCountUninitialized : 0;
}






nsLinkableAccessible::
  nsLinkableAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell) :
  nsAccessibleWrap(aNode, aShell),
  mActionContent(nsnull),
  mIsLink(PR_FALSE),
  mIsOnclick(PR_FALSE)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsLinkableAccessible, nsAccessibleWrap)




NS_IMETHODIMP
nsLinkableAccessible::TakeFocus()
{
  nsCOMPtr<nsIAccessible> actionAcc = GetActionAccessible();
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
    nsCOMPtr<nsIAccessible> actionAcc = GetActionAccessible();
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
    nsCOMPtr<nsIAccessible> actionAcc = GetActionAccessible();
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
  
  nsCOMPtr<nsIAccessible> actionAcc = GetActionAccessible();
  if (actionAcc)
    return actionAcc->DoAction(aIndex);
  
  return nsAccessibleWrap::DoAction(aIndex);
}

NS_IMETHODIMP
nsLinkableAccessible::GetKeyboardShortcut(nsAString& aKeyboardShortcut)
{
  aKeyboardShortcut.Truncate();

  nsCOMPtr<nsIAccessible> actionAcc = GetActionAccessible();
  if (actionAcc)
    return actionAcc->GetKeyboardShortcut(aKeyboardShortcut);

  return nsAccessible::GetKeyboardShortcut(aKeyboardShortcut);
}




NS_IMETHODIMP
nsLinkableAccessible::GetURI(PRInt32 aIndex, nsIURI **aURI)
{
  if (mIsLink) {
    nsCOMPtr<nsIAccessible> actionAcc = GetActionAccessible();
    if (actionAcc) {
      nsCOMPtr<nsIAccessibleHyperLink> hyperLinkAcc =
        do_QueryInterface(actionAcc);
      NS_ASSERTION(hyperLinkAcc,
                   "nsIAccessibleHyperLink isn't implemented.");

      if (hyperLinkAcc)
        return hyperLinkAcc->GetURI(aIndex, aURI);
    }
  }
  
  return NS_ERROR_INVALID_ARG;
}




nsresult
nsLinkableAccessible::Init()
{
  CacheActionContent();
  return nsAccessibleWrap::Init();
}

nsresult
nsLinkableAccessible::Shutdown()
{
  mActionContent = nsnull;
  return nsAccessibleWrap::Shutdown();
}




void
nsLinkableAccessible::CacheActionContent()
{
  nsCOMPtr<nsIContent> walkUpContent(do_QueryInterface(mDOMNode));
  PRBool isOnclick = nsCoreUtils::HasListener(walkUpContent,
                                              NS_LITERAL_STRING("click"));

  if (isOnclick) {
    mActionContent = walkUpContent;
    mIsOnclick = PR_TRUE;
    return;
  }

  while ((walkUpContent = walkUpContent->GetParent())) {
    isOnclick = nsCoreUtils::HasListener(walkUpContent,
                                         NS_LITERAL_STRING("click"));
  
    nsCOMPtr<nsIDOMNode> walkUpNode(do_QueryInterface(walkUpContent));

    nsCOMPtr<nsIAccessible> walkUpAcc;
    GetAccService()->GetAccessibleInWeakShell(walkUpNode, mWeakShell,
                                              getter_AddRefs(walkUpAcc));

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

already_AddRefed<nsIAccessible>
nsLinkableAccessible::GetActionAccessible()
{
  
  
  
  
  nsCOMPtr<nsIDOMNode> actionNode(do_QueryInterface(mActionContent));
  if (!actionNode || mDOMNode == actionNode)
    return nsnull;

  nsIAccessible *accessible = nsnull;
  GetAccService()->GetAccessibleInWeakShell(actionNode, mWeakShell,
                                            &accessible);
  return accessible;
}





nsEnumRoleAccessible::nsEnumRoleAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell, PRUint32 aRole) :
  nsAccessibleWrap(aNode, aShell),
  mRole(aRole)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsEnumRoleAccessible, nsAccessible)

nsresult
nsEnumRoleAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = mRole;
  return NS_OK;
}
