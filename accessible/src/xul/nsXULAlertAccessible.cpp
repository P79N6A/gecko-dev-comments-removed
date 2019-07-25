




































#include "nsXULAlertAccessible.h"

#include "States.h"





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
