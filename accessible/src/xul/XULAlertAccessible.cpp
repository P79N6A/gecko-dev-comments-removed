




#include "XULAlertAccessible.h"

#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;





XULAlertAccessible::
  XULAlertAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(XULAlertAccessible, Accessible)

role
XULAlertAccessible::NativeRole()
{
  return roles::ALERT;
}

PRUint64
XULAlertAccessible::NativeState()
{
  return Accessible::NativeState() | states::ALERT;
}

ENameValueFlag
XULAlertAccessible::Name(nsString& aName)
{
  
  
  aName.Truncate();
  return eNameOK;
}




bool
XULAlertAccessible::IsWidget() const
{
  return true;
}

Accessible*
XULAlertAccessible::ContainerWidget() const
{
  
  if (mParent && mParent->IsMenuButton())
    return mParent;
  return nsnull;
}
