




#include "XULMenuAccessible.h"

#include "Accessible-inl.h"
#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "DocAccessible.h"
#include "Role.h"
#include "States.h"
#include "XULFormControlAccessible.h"

#include "nsIDOMElement.h"
#include "nsIDOMXULElement.h"
#include "nsIMutableArray.h"
#include "nsIDOMXULContainerElement.h"
#include "nsIDOMXULSelectCntrlItemEl.h"
#include "nsIDOMXULMultSelectCntrlEl.h"
#include "nsIDOMKeyEvent.h"
#include "nsIServiceManager.h"
#include "nsIPresShell.h"
#include "nsIContent.h"
#include "nsMenuBarFrame.h"
#include "nsMenuPopupFrame.h"

#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;
using namespace mozilla::a11y;





XULMenuitemAccessible::
  XULMenuitemAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
}

uint64_t
XULMenuitemAccessible::NativeState()
{
  uint64_t state = Accessible::NativeState();

  
  if (mContent->NodeInfo()->Equals(nsGkAtoms::menu, kNameSpaceID_XUL)) {
    state |= states::HASPOPUP;
    if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::open))
      state |= states::EXPANDED;
    else
      state |= states::COLLAPSED;
  }

  
  static nsIContent::AttrValuesArray strings[] =
    { &nsGkAtoms::radio, &nsGkAtoms::checkbox, nullptr };

  if (mContent->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::type, strings,
                                eCaseMatters) >= 0) {

    
    state |= states::CHECKABLE;

    
    if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::checked,
                              nsGkAtoms::_true, eCaseMatters))
      state |= states::CHECKED;
  }

  
  bool isComboboxOption = (Role() == roles::COMBOBOX_OPTION);
  if (isComboboxOption) {
    
    bool isSelected = false;
    nsCOMPtr<nsIDOMXULSelectControlItemElement>
      item(do_QueryInterface(mContent));
    NS_ENSURE_TRUE(item, state);
    item->GetSelected(&isSelected);

    
    bool isCollapsed = false;
    Accessible* parent = Parent();
    if (parent && parent->State() & states::INVISIBLE)
      isCollapsed = true;

    if (isSelected) {
      state |= states::SELECTED;

      
      if (isCollapsed) {
        
        Accessible* grandParent = parent->Parent();
        if (!grandParent)
          return state;
        NS_ASSERTION(grandParent->Role() == roles::COMBOBOX,
                     "grandparent of combobox listitem is not combobox");
        uint64_t grandParentState = grandParent->State();
        state &= ~(states::OFFSCREEN | states::INVISIBLE);
        state |= (grandParentState & states::OFFSCREEN) |
                 (grandParentState & states::INVISIBLE) |
                 (grandParentState & states::OPAQUE1);
      } 
    } 
  } 

  return state;
}

uint64_t
XULMenuitemAccessible::NativeInteractiveState() const
{
  if (NativelyUnavailable()) {
    
    bool skipNavigatingDisabledMenuItem = true;
    nsMenuFrame* menuFrame = do_QueryFrame(GetFrame());
    if (!menuFrame || !menuFrame->IsOnMenuBar()) {
      skipNavigatingDisabledMenuItem = LookAndFeel::
        GetInt(LookAndFeel::eIntID_SkipNavigatingDisabledMenuItem, 0) != 0;
    }

    if (skipNavigatingDisabledMenuItem)
      return states::UNAVAILABLE;

    return states::UNAVAILABLE | states::FOCUSABLE | states::SELECTABLE;
  }

  return states::FOCUSABLE | states::SELECTABLE;
}

ENameValueFlag
XULMenuitemAccessible::NativeName(nsString& aName)
{
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::label, aName);
  return eNameOK;
}

void
XULMenuitemAccessible::Description(nsString& aDescription)
{
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::description,
                    aDescription);
}

KeyBinding
XULMenuitemAccessible::AccessKey() const
{
  
  static int32_t gMenuAccesskeyModifier = -1;  

  
  
  nsAutoString accesskey;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey,
                    accesskey);
  if (accesskey.IsEmpty())
    return KeyBinding();

  uint32_t modifierKey = 0;

  Accessible* parentAcc = Parent();
  if (parentAcc) {
    if (parentAcc->NativeRole() == roles::MENUBAR) {
      
      
      if (gMenuAccesskeyModifier == -1) {
        
        gMenuAccesskeyModifier = Preferences::GetInt("ui.key.menuAccessKey", 0);
      }

      switch (gMenuAccesskeyModifier) {
        case nsIDOMKeyEvent::DOM_VK_CONTROL:
          modifierKey = KeyBinding::kControl;
          break;
        case nsIDOMKeyEvent::DOM_VK_ALT:
          modifierKey = KeyBinding::kAlt;
          break;
        case nsIDOMKeyEvent::DOM_VK_META:
          modifierKey = KeyBinding::kMeta;
          break;
        case nsIDOMKeyEvent::DOM_VK_WIN:
          modifierKey = KeyBinding::kOS;
          break;
      }
    }
  }

  return KeyBinding(accesskey[0], modifierKey);
}

KeyBinding
XULMenuitemAccessible::KeyboardShortcut() const
{
  nsAutoString keyElmId;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::key, keyElmId);
  if (keyElmId.IsEmpty())
    return KeyBinding();

  nsIContent* keyElm = mContent->OwnerDoc()->GetElementById(keyElmId);
  if (!keyElm)
    return KeyBinding();

  uint32_t key = 0;

  nsAutoString keyStr;
  keyElm->GetAttr(kNameSpaceID_None, nsGkAtoms::key, keyStr);
  if (keyStr.IsEmpty()) {
    nsAutoString keyCodeStr;
    keyElm->GetAttr(kNameSpaceID_None, nsGkAtoms::keycode, keyCodeStr);
    nsresult errorCode;
    key = keyStr.ToInteger(&errorCode, kAutoDetect);
  } else {
    key = keyStr[0];
  }

  nsAutoString modifiersStr;
  keyElm->GetAttr(kNameSpaceID_None, nsGkAtoms::modifiers, modifiersStr);

  uint32_t modifierMask = 0;
  if (modifiersStr.Find("shift") != -1)
    modifierMask |= KeyBinding::kShift;
  if (modifiersStr.Find("alt") != -1)
    modifierMask |= KeyBinding::kAlt;
  if (modifiersStr.Find("meta") != -1)
    modifierMask |= KeyBinding::kMeta;
  if (modifiersStr.Find("os") != -1)
    modifierMask |= KeyBinding::kOS;
  if (modifiersStr.Find("control") != -1)
    modifierMask |= KeyBinding::kControl;
  if (modifiersStr.Find("accel") != -1) {
    modifierMask |= KeyBinding::AccelModifier();
  }

  return KeyBinding(key, modifierMask);
}

role
XULMenuitemAccessible::NativeRole()
{
  nsCOMPtr<nsIDOMXULContainerElement> xulContainer(do_QueryInterface(mContent));
  if (xulContainer)
    return roles::PARENT_MENUITEM;

  if (mParent && mParent->Role() == roles::COMBOBOX_LIST)
    return roles::COMBOBOX_OPTION;

  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                            nsGkAtoms::radio, eCaseMatters)) 
    return roles::RADIO_MENU_ITEM;

  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                            nsGkAtoms::checkbox,
                            eCaseMatters)) 
    return roles::CHECK_MENU_ITEM;

  return roles::MENUITEM;
}

int32_t
XULMenuitemAccessible::GetLevelInternal()
{
  return nsAccUtils::GetLevelForXULContainerItem(mContent);
}

bool
XULMenuitemAccessible::CanHaveAnonChildren()
{
  
  return false;
}

bool
XULMenuitemAccessible::DoAction(uint8_t index)
{
  if (index == eAction_Click) {   
    DoCommand();
    return true;
  }

  return false;
}

void
XULMenuitemAccessible::ActionNameAt(uint8_t aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click)
    aName.AssignLiteral("click");
}

uint8_t
XULMenuitemAccessible::ActionCount()
{
  return 1;
}




bool
XULMenuitemAccessible::IsActiveWidget() const
{
  
  nsIContent* menuPopupContent = mContent->GetFirstChild();
  if (menuPopupContent) {
    nsMenuPopupFrame* menuPopupFrame =
      do_QueryFrame(menuPopupContent->GetPrimaryFrame());
    return menuPopupFrame && menuPopupFrame->IsOpen();
  }
  return false;
}

bool
XULMenuitemAccessible::AreItemsOperable() const
{
  
  nsIContent* menuPopupContent = mContent->GetFirstChild();
  if (menuPopupContent) {
    nsMenuPopupFrame* menuPopupFrame =
      do_QueryFrame(menuPopupContent->GetPrimaryFrame());
    return menuPopupFrame && menuPopupFrame->IsOpen();
  }
  return false;
}

Accessible*
XULMenuitemAccessible::ContainerWidget() const
{
  nsMenuFrame* menuFrame = do_QueryFrame(GetFrame());
  if (menuFrame) {
    nsMenuParent* menuParent = menuFrame->GetMenuParent();
    if (menuParent) {
      if (menuParent->IsMenuBar()) 
        return mParent;

      
      if (menuParent->IsMenu())
        return mParent;

      
      
    }
  }
  return nullptr;
}






XULMenuSeparatorAccessible::
  XULMenuSeparatorAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  XULMenuitemAccessible(aContent, aDoc)
{
}

uint64_t
XULMenuSeparatorAccessible::NativeState()
{
  
  return XULMenuitemAccessible::NativeState() &
    (states::OFFSCREEN | states::INVISIBLE);
}

ENameValueFlag
XULMenuSeparatorAccessible::NativeName(nsString& aName)
{
  return eNameOK;
}

role
XULMenuSeparatorAccessible::NativeRole()
{
  return roles::SEPARATOR;
}

bool
XULMenuSeparatorAccessible::DoAction(uint8_t index)
{
  return false;
}

void
XULMenuSeparatorAccessible::ActionNameAt(uint8_t aIndex, nsAString& aName)
{
  aName.Truncate();
}

uint8_t
XULMenuSeparatorAccessible::ActionCount()
{
  return 0;
}





XULMenupopupAccessible::
  XULMenupopupAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  XULSelectControlAccessible(aContent, aDoc)
{
  nsMenuPopupFrame* menuPopupFrame = do_QueryFrame(GetFrame());
  if (menuPopupFrame && menuPopupFrame->IsMenu())
    mType = eMenuPopupType;

  
  mSelectControl = do_QueryInterface(mContent->GetFlattenedTreeParent());
  if (!mSelectControl)
    mGenericTypes &= ~eSelect;
}

uint64_t
XULMenupopupAccessible::NativeState()
{
  uint64_t state = Accessible::NativeState();

#ifdef DEBUG
  
  bool isActive = mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::menuactive);
  if (!isActive) {
    Accessible* parent = Parent();
    if (parent) {
      nsIContent* parentContent = parent->GetContent();
      if (parentContent)
        isActive = parentContent->HasAttr(kNameSpaceID_None, nsGkAtoms::open);
    }
  }

  NS_ASSERTION(isActive || (state & states::INVISIBLE),
               "XULMenupopup doesn't have INVISIBLE when it's inactive");
#endif

  if (state & states::INVISIBLE)
    state |= states::OFFSCREEN | states::COLLAPSED;

  return state;
}

ENameValueFlag
XULMenupopupAccessible::NativeName(nsString& aName)
{
  nsIContent* content = mContent;
  while (content && aName.IsEmpty()) {
    content->GetAttr(kNameSpaceID_None, nsGkAtoms::label, aName);
    content = content->GetFlattenedTreeParent();
  }

  return eNameOK;
}

role
XULMenupopupAccessible::NativeRole()
{
  
  
  if (mParent) {
    roles::Role role = mParent->Role();
    if (role == roles::COMBOBOX || role == roles::AUTOCOMPLETE)
      return roles::COMBOBOX_LIST;

    if (role == roles::PUSHBUTTON) {
      
      Accessible* grandParent = mParent->Parent();
      if (grandParent && grandParent->Role() == roles::AUTOCOMPLETE)
        return roles::COMBOBOX_LIST;
    }
  }

  return roles::MENUPOPUP;
}




bool
XULMenupopupAccessible::IsWidget() const
{
  return true;
}

bool
XULMenupopupAccessible::IsActiveWidget() const
{
  
  nsMenuPopupFrame* menuPopupFrame = do_QueryFrame(GetFrame());
  return menuPopupFrame && menuPopupFrame->IsOpen();
}

bool
XULMenupopupAccessible::AreItemsOperable() const
{
  nsMenuPopupFrame* menuPopupFrame = do_QueryFrame(GetFrame());
  return menuPopupFrame && menuPopupFrame->IsOpen();
}

Accessible*
XULMenupopupAccessible::ContainerWidget() const
{
  DocAccessible* document = Document();

  nsMenuPopupFrame* menuPopupFrame = do_QueryFrame(GetFrame());
  while (menuPopupFrame) {
    Accessible* menuPopup =
      document->GetAccessible(menuPopupFrame->GetContent());
    if (!menuPopup) 
      return nullptr;

    nsMenuFrame* menuFrame = do_QueryFrame(menuPopupFrame->GetParent());
    if (!menuFrame) 
      return nullptr;

    nsMenuParent* menuParent = menuFrame->GetMenuParent();
    if (!menuParent) 
      return menuPopup->Parent();

    if (menuParent->IsMenuBar()) { 
      nsMenuBarFrame* menuBarFrame = static_cast<nsMenuBarFrame*>(menuParent);
      return document->GetAccessible(menuBarFrame->GetContent());
    }

    
    if (!menuParent->IsMenu())
      return nullptr;

    menuPopupFrame = static_cast<nsMenuPopupFrame*>(menuParent);
  }

  NS_NOTREACHED("Shouldn't be a real case.");
  return nullptr;
}





XULMenubarAccessible::
  XULMenubarAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  AccessibleWrap(aContent, aDoc)
{
}

ENameValueFlag
XULMenubarAccessible::NativeName(nsString& aName)
{
  aName.AssignLiteral("Application");
  return eNameOK;
}

role
XULMenubarAccessible::NativeRole()
{
  return roles::MENUBAR;
}




bool
XULMenubarAccessible::IsActiveWidget() const
{
  nsMenuBarFrame* menuBarFrame = do_QueryFrame(GetFrame());
  return menuBarFrame && menuBarFrame->IsActive();
}

bool
XULMenubarAccessible::AreItemsOperable() const
{
  return true;
}

Accessible*
XULMenubarAccessible::CurrentItem()
{
  nsMenuBarFrame* menuBarFrame = do_QueryFrame(GetFrame());
  if (menuBarFrame) {
    nsMenuFrame* menuFrame = menuBarFrame->GetCurrentMenuItem();
    if (menuFrame) {
      nsIContent* menuItemNode = menuFrame->GetContent();
      return mDoc->GetAccessible(menuItemNode);
    }
  }
  return nullptr;
}

void
XULMenubarAccessible::SetCurrentItem(Accessible* aItem)
{
  NS_ERROR("XULMenubarAccessible::SetCurrentItem not implemented");
}
