




































#include "nsXULAlertAccessible.h"




NS_IMPL_ISUPPORTS_INHERITED0(nsXULAlertAccessible, nsAccessible)

nsXULAlertAccessible::nsXULAlertAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell) :
  nsAccessibleWrap(aNode, aShell)
{
}

NS_IMETHODIMP nsXULAlertAccessible::GetRole(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_ALERT;
  return NS_OK;
}

NS_IMETHODIMP
nsXULAlertAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  nsresult rv = nsAccessible::GetState(aState, aExtraState);
  NS_ENSURE_SUCCESS(rv, rv);

  *aState &= ~nsIAccessibleStates::STATE_FOCUSABLE;
  *aState |= nsIAccessibleStates::STATE_ALERT_MEDIUM; 
  return NS_OK;
}

