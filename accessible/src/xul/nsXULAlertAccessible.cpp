




#include "nsXULAlertAccessible.h"

#include "Role.h"
#include "States.h"

using namespace mozilla::a11y;





nsXULAlertAccessible::
  nsXULAlertAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsAccessibleWrap(aContent, aDoc)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULAlertAccessible, nsAccessible)

role
nsXULAlertAccessible::NativeRole()
{
  return roles::ALERT;
}

PRUint64
nsXULAlertAccessible::NativeState()
{
  return nsAccessible::NativeState() | states::ALERT;
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

nsAccessible*
nsXULAlertAccessible::ContainerWidget() const
{
  
  if (mParent && mParent->IsMenuButton())
    return mParent;
  return nsnull;
}
