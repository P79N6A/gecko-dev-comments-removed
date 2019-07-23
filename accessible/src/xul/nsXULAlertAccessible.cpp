




































#include "nsXULAlertAccessible.h"




NS_IMPL_ISUPPORTS_INHERITED0(nsXULAlertAccessible, nsAccessible)

nsXULAlertAccessible::nsXULAlertAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell) :
  nsAccessibleWrap(aNode, aShell)
{
}

nsresult
nsXULAlertAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_ALERT;
  return NS_OK;
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
