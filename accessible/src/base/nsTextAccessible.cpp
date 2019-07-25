







































#include "nsTextAccessible.h"

#include "Role.h"

using namespace mozilla::a11y;





nsTextAccessible::
  nsTextAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsLinkableAccessible(aContent, aDoc)
{
  mFlags |= eTextLeafAccessible;
}

role
nsTextAccessible::NativeRole()
{
  return roles::TEXT_LEAF;
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
