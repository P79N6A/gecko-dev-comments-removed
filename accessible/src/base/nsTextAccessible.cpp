








































#include "nsTextAccessible.h"





nsTextAccessible::nsTextAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell):
nsLinkableAccessible(aDOMNode, aShell)
{ 
}

NS_IMPL_ISUPPORTS_INHERITED0(nsTextAccessible, nsLinkableAccessible)




NS_IMETHODIMP nsTextAccessible::GetRole(PRUint32 *_retval)
{
  *_retval = nsIAccessibleRole::ROLE_TEXT_LEAF;
  return NS_OK;
}




NS_IMETHODIMP nsTextAccessible::GetFirstChild(nsIAccessible **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}




NS_IMETHODIMP nsTextAccessible::GetLastChild(nsIAccessible **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}




NS_IMETHODIMP nsTextAccessible::GetChildCount(PRInt32 *_retval)
{
  *_retval = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsTextAccessible::GetContentText(nsAString& aText)
{
  nsresult rv = nsLinkableAccessible::GetContentText(aText);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIFrame *frame = GetFrame();
  if (!frame)
    return NS_OK;

  frame->GetContent()->AppendTextTo(aText);
  return NS_OK;
}

