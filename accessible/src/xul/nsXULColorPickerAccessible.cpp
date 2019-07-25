





































#include "nsXULColorPickerAccessible.h"

#include "Accessible-inl.h"
#include "nsAccUtils.h"
#include "nsAccTreeWalker.h"
#include "nsCoreUtils.h"
#include "nsDocAccessible.h"
#include "Role.h"
#include "States.h"

#include "nsIDOMElement.h"
#include "nsMenuPopupFrame.h"

using namespace mozilla::a11y;





nsXULColorPickerTileAccessible::
  nsXULColorPickerTileAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsAccessibleWrap(aContent, aDoc)
{
}




void
nsXULColorPickerTileAccessible::Value(nsString& aValue)
{
  aValue.Truncate();

  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::color, aValue);
}




role
nsXULColorPickerTileAccessible::NativeRole()
{
  return roles::PUSHBUTTON;
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
  nsXULColorPickerAccessible(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsXULColorPickerTileAccessible(aContent, aDoc)
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

role
nsXULColorPickerAccessible::NativeRole()
{
  return roles::BUTTONDROPDOWNGRID;
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
  NS_ENSURE_TRUE(mDoc,);

  nsAccTreeWalker walker(mDoc, mContent, true);

  nsAccessible* child = nsnull;
  while ((child = walker.NextChild())) {
    PRUint32 role = child->Role();

    
    if (role == roles::ALERT) {
      AppendChild(child);
      return;
    }

    
    Document()->UnbindFromDocument(child);
  }
}
