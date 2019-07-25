




































#include "nsXULAlertAccessible.h"






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

nsresult
nsXULAlertAccessible::GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsAccessible::GetStateInternal(aState, aExtraState);
  NS_ENSURE_A11Y_SUCCESS(rv, rv);

  
  *aState |= nsIAccessibleStates::STATE_ALERT_MEDIUM;
  return NS_OK;
}

NS_IMETHODIMP
nsXULAlertAccessible::GetName(nsAString& aName)
{
  
  
  aName.Truncate();
  return NS_OK;
}
