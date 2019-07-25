





































#include "nsXULColorPickerAccessible.h"

#include "States.h"
#include "nsAccUtils.h"
#include "nsAccTreeWalker.h"
#include "nsCoreUtils.h"
#include "nsDocAccessible.h"

#include "nsIDOMElement.h"
#include "nsMenuPopupFrame.h"

using namespace mozilla::a11y;





nsXULColorPickerTileAccessible::
  nsXULColorPickerTileAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}




NS_IMETHODIMP
nsXULColorPickerTileAccessible::GetValue(nsAString& aValue)
{
  aValue.Truncate();

  if (IsDefunct())
    return NS_ERROR_FAILURE;

  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::color, aValue);
  return NS_OK;
}




PRUint32
nsXULColorPickerTileAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_PUSHBUTTON;
}

PRUint64
nsXULColorPickerTileAccessible::NativeState()
{
  PRUint64 state = nsAccessibleWrap::NativeState();
  if (!(state & states::UNAVAILABLE))
    state |= states::FOCUSABLE | states::SELECTABLE;

  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::selected))
    state |= states::SELECTED;

  return state;
}




nsAccessible*
nsXULColorPickerTileAccessible::ContainerWidget() const
{
  nsAccessible* parent = Parent();
  if (parent) {
    nsAccessible* grandParent = parent->Parent();
    if (grandParent && grandParent->IsMenuButton())
      return grandParent;
  }
  return nsnull;
}





nsXULColorPickerAccessible::
  nsXULColorPickerAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXULColorPickerTileAccessible(aContent, aShell)
{
  mFlags |= eMenuButtonAccessible;
}




PRUint64
nsXULColorPickerAccessible::NativeState()
{
  

  
  PRUint64 states = nsAccessibleWrap::NativeState();

  states |= states::FOCUSABLE | states::HASPOPUP;

  return states;
}

PRUint32
nsXULColorPickerAccessible::NativeRole()
{
  return nsIAccessibleRole::ROLE_BUTTONDROPDOWNGRID;
}




bool
nsXULColorPickerAccessible::IsWidget() const
{
  return true;
}

bool
nsXULColorPickerAccessible::IsActiveWidget() const
{
  return FocusMgr()->HasDOMFocus(mContent);
}

bool
nsXULColorPickerAccessible::AreItemsOperable() const
{
  nsAccessible* menuPopup = mChildren.SafeElementAt(0, nsnull);
  if (menuPopup) {
    nsMenuPopupFrame* menuPopupFrame = do_QueryFrame(menuPopup->GetFrame());
    return menuPopupFrame && menuPopupFrame->IsOpen();
  }
  return false;
}




void
nsXULColorPickerAccessible::CacheChildren()
{
  nsAccTreeWalker walker(mWeakShell, mContent, PR_TRUE);

  nsAccessible* child = nsnull;
  while ((child = walker.NextChild())) {
    PRUint32 role = child->Role();

    
    if (role == nsIAccessibleRole::ROLE_ALERT) {
      AppendChild(child);
      return;
    }

    
    GetDocAccessible()->UnbindFromDocument(child);
  }
}
