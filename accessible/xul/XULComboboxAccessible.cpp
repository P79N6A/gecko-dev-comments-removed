




#include "XULComboboxAccessible.h"

#include "Accessible-inl.h"
#include "nsAccessibilityService.h"
#include "DocAccessible.h"
#include "nsCoreUtils.h"
#include "Role.h"
#include "States.h"

#include "nsIAutoCompleteInput.h"
#include "nsIDOMXULMenuListElement.h"
#include "nsIDOMXULSelectCntrlItemEl.h"

using namespace mozilla::a11y;





XULComboboxAccessible::
  XULComboboxAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                            nsGkAtoms::autocomplete, eIgnoreCase))
    mGenericTypes |= eAutoComplete;
  else
    mGenericTypes |= eCombobox;
}

role
XULComboboxAccessible::NativeRole()
{
  return IsAutoComplete() ? roles::AUTOCOMPLETE : roles::COMBOBOX;
}

uint64_t
XULComboboxAccessible::NativeState()
{
  
  
  
  
  
  

  
  uint64_t state = Accessible::NativeState();

  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mContent));
  if (menuList) {
    bool isOpen = false;
    menuList->GetOpen(&isOpen);
    if (isOpen)
      state |= states::EXPANDED;
    else
      state |= states::COLLAPSED;
  }

  return state | states::HASPOPUP;
}

void
XULComboboxAccessible::Description(nsString& aDescription)
{
  aDescription.Truncate();
  
  nsCOMPtr<nsIDOMXULMenuListElement> menuListElm(do_QueryInterface(mContent));
  if (!menuListElm)
    return;

  nsCOMPtr<nsIDOMXULSelectControlItemElement> focusedOptionItem;
  menuListElm->GetSelectedItem(getter_AddRefs(focusedOptionItem));
  nsCOMPtr<nsIContent> focusedOptionContent =
    do_QueryInterface(focusedOptionItem);
  if (focusedOptionContent && mDoc) {
    Accessible* focusedOptionAcc = mDoc->GetAccessible(focusedOptionContent);
    if (focusedOptionAcc)
      focusedOptionAcc->Description(aDescription);
  }
}

void
XULComboboxAccessible::Value(nsString& aValue)
{
  aValue.Truncate();

  
  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mContent));
  if (menuList)
    menuList->GetLabel(aValue);
}

bool
XULComboboxAccessible::CanHaveAnonChildren()
{
  if (mContent->NodeInfo()->Equals(nsGkAtoms::textbox, kNameSpaceID_XUL) ||
      mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::editable,
                            nsGkAtoms::_true, eIgnoreCase)) {
    
    
    
    return true;
  }

  
  
  return false;
}

uint8_t
XULComboboxAccessible::ActionCount()
{
  
  return 1;
}

bool
XULComboboxAccessible::DoAction(uint8_t aIndex)
{
  if (aIndex != XULComboboxAccessible::eAction_Click)
    return false;

  
  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mContent));
  if (!menuList)
    return false;

  bool isDroppedDown = false;
  menuList->GetOpen(&isDroppedDown);
  menuList->SetOpen(!isDroppedDown);
  return true;
}

void
XULComboboxAccessible::ActionNameAt(uint8_t aIndex, nsAString& aName)
{
  aName.Truncate();
  if (aIndex != XULComboboxAccessible::eAction_Click)
    return;

  nsCOMPtr<nsIDOMXULMenuListElement> menuList(do_QueryInterface(mContent));
  if (!menuList)
    return;

  bool isDroppedDown = false;
  menuList->GetOpen(&isDroppedDown);
  if (isDroppedDown)
    aName.AssignLiteral("close");
  else
    aName.AssignLiteral("open");
}




bool
XULComboboxAccessible::IsActiveWidget() const
{
  if (IsAutoComplete() ||
     mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::editable,
                           nsGkAtoms::_true, eIgnoreCase)) {
    int32_t childCount = mChildren.Length();
    for (int32_t idx = 0; idx < childCount; idx++) {
      Accessible* child = mChildren[idx];
      if (child->Role() == roles::ENTRY)
        return FocusMgr()->HasDOMFocus(child->GetContent());
    }
    return false;
  }

  return FocusMgr()->HasDOMFocus(mContent);
}

bool
XULComboboxAccessible::AreItemsOperable() const
{
  if (IsAutoComplete()) {
    nsCOMPtr<nsIAutoCompleteInput> autoCompleteInputElm =
      do_QueryInterface(mContent);
    if (autoCompleteInputElm) {
      bool isOpen = false;
      autoCompleteInputElm->GetPopupOpen(&isOpen);
      return isOpen;
    }
    return false;
  }

  nsCOMPtr<nsIDOMXULMenuListElement> menuListElm = do_QueryInterface(mContent);
  if (menuListElm) {
    bool isOpen = false;
    menuListElm->GetOpen(&isOpen);
    return isOpen;
  }

  return false;
}
