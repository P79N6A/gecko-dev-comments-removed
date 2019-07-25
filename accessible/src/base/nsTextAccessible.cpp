







































#include "nsTextAccessible.h"





nsTextAccessible::
  nsTextAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsLinkableAccessible(aContent, aShell)
{
  mFlags |= eTextLeafAccessible;
}

PRUint32
nsTextAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_TEXT_LEAF;
}

void
nsTextAccessible::AppendTextTo(nsAString& aText, PRUint32 aStartOffset,
                               PRUint32 aLength)
{
  aText.Append(Substring(mText, aStartOffset, aLength));
}

void
nsTextAccessible::CacheChildren()
{
  
}
