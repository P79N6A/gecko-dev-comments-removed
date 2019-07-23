








































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
nsTextAccessible::AppendTextTo(nsAString& aText, PRUint32 aStartOffset, PRUint32 aLength)
{
  nsIFrame *frame = GetFrame();
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  return frame->GetRenderedText(&aText, nsnull, nsnull, aStartOffset, aLength);
}

