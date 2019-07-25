




#include "nsXULAlertAccessible.h"

#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;





nsXULAlertAccessible::
  nsXULAlertAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULAlertAccessible, Accessible)

role
nsXULAlertAccessible::NativeRole()
{
  return roles::ALERT;
}

PRUint64
nsXULAlertAccessible::NativeState()
{
  return Accessible::NativeState() | states::ALERT;
}

ENameValueFlag
nsXULAlertAccessible::Name(nsString& aName)
{
  
  
  aName.Truncate();
  return eNameOK;
}




bool
nsXULAlertAccessible::IsWidget() const
{
  return true;
}

Accessible*
nsXULAlertAccessible::ContainerWidget() const
{
  
  if (mParent && mParent->IsMenuButton())
    return mParent;
  return nsnull;
}
