




































#include "nsXULAlertAccessible.h"

#include "States.h"

using namespace mozilla::a11y;





nsXULAlertAccessible::
  nsXULAlertAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULAlertAccessible, nsAccessible)

PRUint32
nsXULAlertAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_ALERT;
}

PRUint64
nsXULAlertAccessible::NativeState()
{
  return nsAccessible::NativeState() | states::ALERT;
}

NS_IMETHODIMP
nsXULAlertAccessible::GetName(nsAString& aName)
{
  
  
  aName.Truncate();
  return NS_OK;
}




bool
nsXULAlertAccessible::IsWidget() const
{
  return true;
}

nsAccessible*
nsXULAlertAccessible::ContainerWidget() const
{
  
  if (mParent && mParent->IsMenuButton())
    return mParent;
  return nsnull;
}
