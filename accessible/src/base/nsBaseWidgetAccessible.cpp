






































#include "nsBaseWidgetAccessible.h"

#include "States.h"
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




nsAccessible*
nsLeafAccessible::ChildAtPoint(PRInt32 aX, PRInt32 aY,
                               EWhichChildAtPoint aWhichChild)
{
  
  return this;
}




void
nsLeafAccessible::CacheChildren()
{
  
}






nsLinkableAccessible::
  nsLinkableAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell),
  mActionAcc(nsnull),
  mIsLink(PR_FALSE),
  mIsOnclick(PR_FALSE)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsLinkableAccessible, nsAccessibleWrap)




NS_IMETHODIMP
nsLinkableAccessible::TakeFocus()
{
  return mActionAcc ? mActionAcc->TakeFocus() : nsAccessibleWrap::TakeFocus();
}

PRUint64
nsLinkableAccessible::NativeState()
{
  PRUint64 states = nsAccessibleWrap::NativeState();
  if (mIsLink) {
    states |= states::LINKED;
    if (mActionAcc->State() & states::TRAVERSED)
      states |= states::TRAVERSED;
  }

  return states;
}

NS_IMETHODIMP
nsLinkableAccessible::GetValue(nsAString& aValue)
{
  aValue.Truncate();

  nsAccessible::GetValue(aValue);
  if (!aValue.IsEmpty())
    return NS_OK;

  return mIsLink ? mActionAcc->GetValue(aValue) : NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsLinkableAccessible::GetNumActions(PRUint8 *aNumActions)
{
  NS_ENSURE_ARG_POINTER(aNumActions);

  *aNumActions = (mIsOnclick || mIsLink) ? 1 : 0;
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

  return mActionAcc ? mActionAcc->DoAction(aIndex) :
    nsAccessibleWrap::DoAction(aIndex);
}

NS_IMETHODIMP
nsLinkableAccessible::GetKeyboardShortcut(nsAString& aKeyboardShortcut)
{
  aKeyboardShortcut.Truncate();

  return mActionAcc ? mActionAcc->GetKeyboardShortcut(aKeyboardShortcut) :
    nsAccessible::GetKeyboardShortcut(aKeyboardShortcut);
}




void
nsLinkableAccessible::Shutdown()
{
  mIsLink = PR_FALSE;
  mIsOnclick = PR_FALSE;
  mActionAcc = nsnull;
  nsAccessibleWrap::Shutdown();
}




already_AddRefed<nsIURI>
nsLinkableAccessible::AnchorURIAt(PRUint32 aAnchorIndex)
{
  if (mIsLink) {
    NS_ASSERTION(mActionAcc->IsHyperLink(),
                 "nsIAccessibleHyperLink isn't implemented.");

    if (mActionAcc->IsHyperLink())
      return mActionAcc->AnchorURIAt(aAnchorIndex);
  }

  return nsnull;
}




void
nsLinkableAccessible::BindToParent(nsAccessible* aParent,
                                   PRUint32 aIndexInParent)
{
  nsAccessibleWrap::BindToParent(aParent, aIndexInParent);

  
  mActionAcc = nsnull;
  mIsLink = PR_FALSE;
  mIsOnclick = PR_FALSE;

  if (nsCoreUtils::HasClickListener(mContent)) {
    mIsOnclick = PR_TRUE;
    return;
  }

  
  
  
  nsAccessible* walkUpAcc = this;
  while ((walkUpAcc = walkUpAcc->GetParent()) && !walkUpAcc->IsDoc()) {
    if (walkUpAcc && walkUpAcc->Role() == nsIAccessibleRole::ROLE_LINK &&
        walkUpAcc->State() & states::LINKED) {
      mIsLink = PR_TRUE;
      mActionAcc = walkUpAcc;
      return;
    }

    if (nsCoreUtils::HasClickListener(walkUpAcc->GetContent())) {
      mActionAcc = walkUpAcc;
      mIsOnclick = PR_TRUE;
      return;
    }
  }
}





nsEnumRoleAccessible::
  nsEnumRoleAccessible(nsIContent *aNode, nsIWeakReference *aShell,
                       PRUint32 aRole) :
  nsAccessibleWrap(aNode, aShell), mRole(aRole)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsEnumRoleAccessible, nsAccessible)

PRUint32
nsEnumRoleAccessible::NativeRole()
{
  return mRole;
}
