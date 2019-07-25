




#include "nsBaseWidgetAccessible.h"

#include "Accessible-inl.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsHyperTextAccessibleWrap.h"
#include "Role.h"
#include "States.h"

#include "nsGUIEvent.h"
#include "nsILink.h"
#include "nsIFrame.h"
#include "nsINameSpaceManager.h"
#include "nsIURI.h"

using namespace mozilla::a11y;





nsLeafAccessible::
  nsLeafAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  nsAccessibleWrap(aContent, aDoc)
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
  nsLinkableAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  nsAccessibleWrap(aContent, aDoc),
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
nsLinkableAccessible::NativeLinkState() const
{
  if (mIsLink)
    return states::LINKED | (mActionAcc->LinkState() & states::TRAVERSED);

  return 0;
}

void
nsLinkableAccessible::Value(nsString& aValue)
{
  aValue.Truncate();

  nsAccessible::Value(aValue);
  if (!aValue.IsEmpty())
    return;

  if (aValue.IsEmpty() && mIsLink)
    mActionAcc->Value(aValue);
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
    if (walkUpAcc->LinkState() & states::LINKED) {
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

void
nsLinkableAccessible::UnbindFromParent()
{
  mActionAcc = nsnull;
  mIsLink = false;
  mIsOnclick = false;

  nsAccessibleWrap::UnbindFromParent();
}





nsEnumRoleAccessible::
  nsEnumRoleAccessible(nsIContent* aNode, DocAccessible* aDoc,
                       roles::Role aRole) :
  nsAccessibleWrap(aNode, aDoc), mRole(aRole)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsEnumRoleAccessible, nsAccessible)

role
nsEnumRoleAccessible::NativeRole()
{
  return mRole;
}
