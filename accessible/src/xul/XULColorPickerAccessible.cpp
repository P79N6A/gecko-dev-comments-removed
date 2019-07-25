




#include "XULColorPickerAccessible.h"

#include "Accessible-inl.h"
#include "nsAccUtils.h"
#include "nsAccTreeWalker.h"
#include "nsCoreUtils.h"
#include "DocAccessible.h"
#include "Role.h"
#include "States.h"

#include "nsIDOMElement.h"
#include "nsMenuPopupFrame.h"

using namespace mozilla::a11y;





XULColorPickerTileAccessible::
  XULColorPickerTileAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
}




void
XULColorPickerTileAccessible::Value(nsString& aValue)
{
  aValue.Truncate();

  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::color, aValue);
}




role
XULColorPickerTileAccessible::NativeRole()
{
  return roles::PUSHBUTTON;
}

uint64_t
XULColorPickerTileAccessible::NativeState()
{
  uint64_t state = AccessibleWrap::NativeState();
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::selected))
    state |= states::SELECTED;

  return state;
}

uint64_t
XULColorPickerTileAccessible::NativeInteractiveState() const
{
  return NativelyUnavailable() ?
    states::UNAVAILABLE : states::FOCUSABLE | states::SELECTABLE;
}




Accessible*
XULColorPickerTileAccessible::ContainerWidget() const
{
  Accessible* parent = Parent();
  if (parent) {
    Accessible* grandParent = parent->Parent();
    if (grandParent && grandParent->IsMenuButton())
      return grandParent;
  }
  return nullptr;
}





XULColorPickerAccessible::
  XULColorPickerAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  XULColorPickerTileAccessible(aContent, aDoc)
{
  mFlags |= eMenuButtonAccessible;
}




uint64_t
XULColorPickerAccessible::NativeState()
{
  uint64_t state = AccessibleWrap::NativeState();
  return state | states::HASPOPUP;
}

role
XULColorPickerAccessible::NativeRole()
{
  return roles::BUTTONDROPDOWNGRID;
}




bool
XULColorPickerAccessible::IsWidget() const
{
  return true;
}

bool
XULColorPickerAccessible::IsActiveWidget() const
{
  return FocusMgr()->HasDOMFocus(mContent);
}

bool
XULColorPickerAccessible::AreItemsOperable() const
{
  Accessible* menuPopup = mChildren.SafeElementAt(0, nullptr);
  if (menuPopup) {
    nsMenuPopupFrame* menuPopupFrame = do_QueryFrame(menuPopup->GetFrame());
    return menuPopupFrame && menuPopupFrame->IsOpen();
  }
  return false;
}




void
XULColorPickerAccessible::CacheChildren()
{
  NS_ENSURE_TRUE(mDoc,);

  nsAccTreeWalker walker(mDoc, mContent, true);

  Accessible* child = nullptr;
  while ((child = walker.NextChild())) {
    uint32_t role = child->Role();

    
    if (role == roles::ALERT) {
      AppendChild(child);
      return;
    }

    
    Document()->UnbindFromDocument(child);
  }
}
