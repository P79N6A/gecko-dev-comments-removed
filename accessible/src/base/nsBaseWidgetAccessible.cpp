






































#include "nsBaseWidgetAccessible.h"
#include "nsAccessibilityAtoms.h"
#include "nsIAccessibilityService.h"
#include "nsIAccessibleDocument.h"
#include "nsAccessibleWrap.h"
#include "nsAccessibilityUtils.h"
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


NS_IMETHODIMP nsLeafAccessible::GetFirstChild(nsIAccessible **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}


NS_IMETHODIMP nsLeafAccessible::GetLastChild(nsIAccessible **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}


NS_IMETHODIMP nsLeafAccessible::GetChildCount(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
}


NS_IMETHODIMP
nsLeafAccessible::GetAllowsAnonChildAccessibles(PRBool *aAllowsAnonChildren)
{
  *aAllowsAnonChildren = PR_FALSE;
  return NS_OK;
}




nsLinkableAccessible::
  nsLinkableAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell) :
  nsHyperTextAccessibleWrap(aNode, aShell),
  mActionContent(nsnull),
  mIsLink(PR_FALSE),
  mIsOnclick(PR_FALSE)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsLinkableAccessible, nsHyperTextAccessibleWrap)




NS_IMETHODIMP
nsLinkableAccessible::TakeFocus()
{
  nsCOMPtr<nsIAccessible> actionAcc = GetActionAccessible();
  if (actionAcc)
    return actionAcc->TakeFocus();

  return NS_OK;
}

NS_IMETHODIMP
nsLinkableAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsHyperTextAccessibleWrap::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mIsLink) {
    *aState |= nsIAccessibleStates::STATE_LINKED;
    nsCOMPtr<nsIAccessible> actionAcc = GetActionAccessible();
    if (actionAcc && (State(actionAcc) & nsIAccessibleStates::STATE_TRAVERSED))
      *aState |= nsIAccessibleStates::STATE_TRAVERSED;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsLinkableAccessible::GetValue(nsAString& aValue)
{
  aValue.Truncate();

  nsHyperTextAccessible::GetValue(aValue);
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
  nsCOMPtr<nsIAccessible> actionAcc = GetActionAccessible();
  if (actionAcc)
    return actionAcc->DoAction(aIndex);

  return NS_ERROR_INVALID_ARG;
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




NS_IMETHODIMP
nsLinkableAccessible::Init()
{
  CacheActionContent();
  return nsHyperTextAccessibleWrap::Init();
}

NS_IMETHODIMP
nsLinkableAccessible::Shutdown()
{
  mActionContent = nsnull;
  return nsHyperTextAccessibleWrap::Shutdown();
}




void
nsLinkableAccessible::CacheActionContent()
{
  nsCOMPtr<nsIContent> walkUpContent(do_QueryInterface(mDOMNode));
  PRBool isOnclick = nsAccUtils::HasListener(walkUpContent,
                                             NS_LITERAL_STRING("click"));

  if (isOnclick) {
    mActionContent = walkUpContent;
    mIsOnclick = PR_TRUE;
    return;
  }

  while ((walkUpContent = walkUpContent->GetParent())) {
    isOnclick = nsAccUtils::HasListener(walkUpContent,
                                        NS_LITERAL_STRING("click"));
  
    nsCOMPtr<nsIDOMNode> walkUpNode(do_QueryInterface(walkUpContent));

    nsCOMPtr<nsIAccessible> walkUpAcc;
    GetAccService()->GetAccessibleInWeakShell(walkUpNode, mWeakShell,
                                              getter_AddRefs(walkUpAcc));

    if (walkUpAcc && Role(walkUpAcc) == nsIAccessibleRole::ROLE_LINK) {
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
