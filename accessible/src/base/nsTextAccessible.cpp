







































#include "nsTextAccessible.h"





nsTextAccessible::
  nsTextAccessible(nsIDOMNode* aDOMNode, nsIWeakReference* aShell) :
  nsLinkableAccessible(aDOMNode, aShell)
{
}

nsresult
nsTextAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_TEXT_LEAF;
  return NS_OK;
}

nsresult
nsTextAccessible::AppendTextTo(nsAString& aText, PRUint32 aStartOffset, PRUint32 aLength)
{
  nsIFrame *frame = GetFrame();
  if (!frame) return NS_ERROR_FAILURE;

  return frame->GetRenderedText(&aText, nsnull, nsnull, aStartOffset, aLength);
}

void
nsTextAccessible::CacheChildren()
{
  
}
