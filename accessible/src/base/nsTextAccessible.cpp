







































#include "nsTextAccessible.h"





nsTextAccessible::
  nsTextAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsLinkableAccessible(aContent, aShell)
{
}

PRUint32
nsTextAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_TEXT_LEAF;
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
