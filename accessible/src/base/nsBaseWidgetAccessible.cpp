






































#include "nsBaseWidgetAccessible.h"

#include "States.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsHyperTextAccessibleWrap.h"

#include "nsGUIEvent.h"
#include "nsILink.h"
#include "nsIFrame.h"
#include "nsINameSpaceManager.h"
#include "nsIURI.h"

using namespace mozilla::a11y;





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
  mIsLink(false),
  mIsOnclick(false)
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


PRUint8
nsLinkableAccessible::ActionCount()
{
  return (mIsOnclick || mIsLink) ? 1 : 0;
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

KeyBinding
nsLinkableAccessible::AccessKey() const
{
  return mActionAcc ?
    mActionAcc->AccessKey() : nsAccessible::AccessKey();
}




void
nsLinkableAccessible::Shutdown()
{
  mIsLink = false;
  mIsOnclick = false;
  mActionAcc = nsnull;
  nsAccessibleWrap::Shutdown();
}




already_AddRefed<nsIURI>
nsLinkableAccessible::AnchorURIAt(PRUint32 aAnchorIndex)
{
  if (mIsLink) {
    NS_ASSERTION(mActionAcc->IsLink(),
                 "nsIAccessibleHyperLink isn't implemented.");

    if (mActionAcc->IsLink())
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
  mIsLink = false;
  mIsOnclick = false;

  if (nsCoreUtils::HasClickListener(mContent)) {
    mIsOnclick = true;
    return;
  }

  
  
  
  nsAccessible* walkUpAcc = this;
  while ((walkUpAcc = walkUpAcc->Parent()) && !walkUpAcc->IsDoc()) {
    if (walkUpAcc->Role() == nsIAccessibleRole::ROLE_LINK &&
        walkUpAcc->State() & states::LINKED) {
      mIsLink = true;
      mActionAcc = walkUpAcc;
      return;
    }

    if (nsCoreUtils::HasClickListener(walkUpAcc->GetContent())) {
      mActionAcc = walkUpAcc;
      mIsOnclick = true;
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
