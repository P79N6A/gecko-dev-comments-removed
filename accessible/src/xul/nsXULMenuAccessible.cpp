





































#include "nsXULMenuAccessible.h"

#include "nsAccessibilityService.h"
#include "nsAccUtils.h"
#include "nsDocAccessible.h"
#include "nsXULFormControlAccessible.h"
#include "Role.h"
#include "States.h"

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
#include "nsGUIEvent.h"
#include "nsMenuBarFrame.h"
#include "nsMenuPopupFrame.h"

#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;
using namespace mozilla::a11y;





nsXULMenuitemAccessible::
  nsXULMenuitemAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

PRUint64
nsXULMenuitemAccessible::NativeState()
{
  PRUint64 state = nsAccessible::NativeState();

  
  if (mContent->NodeInfo()->Equals(nsGkAtoms::menu, kNameSpaceID_XUL)) {
    state |= states::HASPOPUP;
    if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::open))
      state |= states::EXPANDED;
    else
      state |= states::COLLAPSED;
  }

  
  static nsIContent::AttrValuesArray strings[] =
    { &nsGkAtoms::radio, &nsGkAtoms::checkbox, nsnull };

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
    nsAccessible* parent = Parent();
    if (parent && parent->State() & states::INVISIBLE)
      isCollapsed = true;

    if (isSelected) {
      state |= states::SELECTED;

      
      if (isCollapsed) {
        
        nsAccessible* grandParent = parent->Parent();
        if (!grandParent)
          return state;
        NS_ASSERTION(grandParent->Role() == roles::COMBOBOX,
                     "grandparent of combobox listitem is not combobox");
        PRUint64 grandParentState = grandParent->State();
        state &= ~(states::OFFSCREEN | states::INVISIBLE);
        state |= (grandParentState & states::OFFSCREEN) |
                 (grandParentState & states::INVISIBLE) |
                 (grandParentState & states::OPAQUE1);
      } 
    } 
  } 

  
  
  if (state & states::UNAVAILABLE) {
    
    PRInt32 skipDisabledMenuItems =
      LookAndFeel::GetInt(LookAndFeel::eIntID_SkipNavigatingDisabledMenuItem);
    
    
    if (skipDisabledMenuItems || isComboboxOption) {
      return state;
    }
  }

  state |= (states::FOCUSABLE | states::SELECTABLE);
  if (FocusMgr()->IsFocused(this))
    state |= states::FOCUSED;

  return state;
}

nsresult
nsXULMenuitemAccessible::GetNameInternal(nsAString& aName)
{
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::label, aName);
  return NS_OK;
}

void
nsXULMenuitemAccessible::Description(nsString& aDescription)
{
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::description,
                    aDescription);
}

KeyBinding
nsXULMenuitemAccessible::AccessKey() const
{
  
  static PRInt32 gMenuAccesskeyModifier = -1;  

  
  
  nsAutoString accesskey;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey,
                    accesskey);
  if (accesskey.IsEmpty())
    return KeyBinding();

  PRUint32 modifierKey = 0;

  nsAccessible* parentAcc = Parent();
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
      }
    }
  }

  return KeyBinding(accesskey[0], modifierKey);
}

KeyBinding
nsXULMenuitemAccessible::KeyboardShortcut() const
{
  nsAutoString keyElmId;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::key, keyElmId);
  if (keyElmId.IsEmpty())
    return KeyBinding();

  nsIContent* keyElm = mContent->OwnerDoc()->GetElementById(keyElmId);
  if (!keyElm)
    return KeyBinding();

  PRUint32 key = 0;

  nsAutoString keyStr;
  keyElm->GetAttr(kNameSpaceID_None, nsGkAtoms::key, keyStr);
  if (keyStr.IsEmpty()) {
    nsAutoString keyCodeStr;
    keyElm->GetAttr(kNameSpaceID_None, nsGkAtoms::keycode, keyCodeStr);
    PRUint32 errorCode;
    key = keyStr.ToInteger(&errorCode, kAutoDetect);
  } else {
    key = keyStr[0];
  }

  nsAutoString modifiersStr;
  keyElm->GetAttr(kNameSpaceID_None, nsGkAtoms::modifiers, modifiersStr);

  PRUint32 modifierMask = 0;
  if (modifiersStr.Find("shift") != -1)
    modifierMask |= KeyBinding::kShift;
  if (modifiersStr.Find("alt") != -1)
    modifierMask |= KeyBinding::kAlt;
  if (modifiersStr.Find("meta") != -1)
    modifierMask |= KeyBinding::kMeta;
  if (modifiersStr.Find("control") != -1)
    modifierMask |= KeyBinding::kControl;
  if (modifiersStr.Find("accel") != -1) {
    
    switch (Preferences::GetInt("ui.key.accelKey", 0)) {
      case nsIDOMKeyEvent::DOM_VK_META:
        modifierMask |= KeyBinding::kMeta;
        break;

      case nsIDOMKeyEvent::DOM_VK_ALT:
        modifierMask |= KeyBinding::kAlt;
        break;

      case nsIDOMKeyEvent::DOM_VK_CONTROL:
        modifierMask |= KeyBinding::kControl;
        break;

      default:
#ifdef XP_MACOSX
        modifierMask |= KeyBinding::kMeta;
#else
        modifierMask |= KeyBinding::kControl;
#endif
    }
  }

  return KeyBinding(key, modifierMask);
}

role
nsXULMenuitemAccessible::NativeRole()
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

PRInt32
nsXULMenuitemAccessible::GetLevelInternal()
{
  return nsAccUtils::GetLevelForXULContainerItem(mContent);
}

void
nsXULMenuitemAccessible::GetPositionAndSizeInternal(PRInt32 *aPosInSet,
                                                    PRInt32 *aSetSize)
{
  nsAccUtils::GetPositionAndSizeForXULContainerItem(mContent, aPosInSet,
                                                    aSetSize);
}

bool
nsXULMenuitemAccessible::CanHaveAnonChildren()
{
  
  return false;
}

NS_IMETHODIMP nsXULMenuitemAccessible::DoAction(PRUint8 index)
{
  if (index == eAction_Click) {   
    DoCommand();
    return NS_OK;
  }

  return NS_ERROR_INVALID_ARG;
}


NS_IMETHODIMP nsXULMenuitemAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  if (aIndex == eAction_Click) {
    aName.AssignLiteral("click"); 
    return NS_OK;
  }
  return NS_ERROR_INVALID_ARG;
}

PRUint8
nsXULMenuitemAccessible::ActionCount()
{
  return 1;
}




bool
nsXULMenuitemAccessible::IsActiveWidget() const
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
nsXULMenuitemAccessible::AreItemsOperable() const
{
  
  nsIContent* menuPopupContent = mContent->GetFirstChild();
  if (menuPopupContent) {
    nsMenuPopupFrame* menuPopupFrame =
      do_QueryFrame(menuPopupContent->GetPrimaryFrame());
    return menuPopupFrame && menuPopupFrame->IsOpen();
  }
  return false;
}

nsAccessible*
nsXULMenuitemAccessible::ContainerWidget() const
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
  return nsnull;
}






nsXULMenuSeparatorAccessible::
  nsXULMenuSeparatorAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXULMenuitemAccessible(aContent, aShell)
{
}

PRUint64
nsXULMenuSeparatorAccessible::NativeState()
{
  
  return nsXULMenuitemAccessible::NativeState() &
    (states::OFFSCREEN | states::INVISIBLE);
}

nsresult
nsXULMenuSeparatorAccessible::GetNameInternal(nsAString& aName)
{
  return NS_OK;
}

role
nsXULMenuSeparatorAccessible::NativeRole()
{
  return roles::SEPARATOR;
}

NS_IMETHODIMP nsXULMenuSeparatorAccessible::DoAction(PRUint8 index)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsXULMenuSeparatorAccessible::GetActionName(PRUint8 aIndex, nsAString& aName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

PRUint8
nsXULMenuSeparatorAccessible::ActionCount()
{
  return 0;
}





nsXULMenupopupAccessible::
  nsXULMenupopupAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  XULSelectControlAccessible(aContent, aShell)
{
  nsMenuPopupFrame* menuPopupFrame = do_QueryFrame(GetFrame());
  if (menuPopupFrame && menuPopupFrame->IsMenu())
    mFlags |= eMenuPopupAccessible;

  
  mSelectControl = do_QueryInterface(mContent->GetParent());
}

PRUint64
nsXULMenupopupAccessible::NativeState()
{
  PRUint64 state = nsAccessible::NativeState();

#ifdef DEBUG_A11Y
  
  bool isActive = mContent->HasAttr(kNameSpaceID_None,
                                      nsGkAtoms::menuactive);
  if (!isActive) {
    nsAccessible* parent = Parent();
    if (!parent)
      return state;

    nsIContent *parentContent = parnet->GetContent();
    NS_ENSURE_TRUE(parentContent, state);

    isActive = parentContent->HasAttr(kNameSpaceID_None,
                                      nsGkAtoms::open);
  }

  NS_ASSERTION(isActive || states & states::INVISIBLE,
               "XULMenupopup doesn't have INVISIBLE when it's inactive");
#endif

  if (state & states::INVISIBLE)
    state |= states::OFFSCREEN | states::COLLAPSED;

  return state;
}

nsresult
nsXULMenupopupAccessible::GetNameInternal(nsAString& aName)
{
  nsIContent *content = mContent;
  while (content && aName.IsEmpty()) {
    content->GetAttr(kNameSpaceID_None, nsGkAtoms::label, aName);
    content = content->GetParent();
  }

  return NS_OK;
}

role
nsXULMenupopupAccessible::NativeRole()
{
  
  
  if (mParent) {
    roles::Role role = mParent->Role();
    if (role == roles::COMBOBOX || role == roles::AUTOCOMPLETE)
      return roles::COMBOBOX_LIST;

    if (role == roles::PUSHBUTTON) {
      
      nsAccessible* grandParent = mParent->Parent();
      if (grandParent && grandParent->Role() == roles::AUTOCOMPLETE)
        return roles::COMBOBOX_LIST;
    }
  }

  return roles::MENUPOPUP;
}




bool
nsXULMenupopupAccessible::IsWidget() const
{
  return true;
}

bool
nsXULMenupopupAccessible::IsActiveWidget() const
{
  
  nsMenuPopupFrame* menuPopupFrame = do_QueryFrame(GetFrame());
  return menuPopupFrame && menuPopupFrame->IsOpen();
}

bool
nsXULMenupopupAccessible::AreItemsOperable() const
{
  nsMenuPopupFrame* menuPopupFrame = do_QueryFrame(GetFrame());
  return menuPopupFrame && menuPopupFrame->IsOpen();
}

nsAccessible*
nsXULMenupopupAccessible::ContainerWidget() const
{
  nsDocAccessible* document = GetDocAccessible();

  nsMenuPopupFrame* menuPopupFrame = do_QueryFrame(GetFrame());
  while (menuPopupFrame) {
    nsAccessible* menuPopup =
      document->GetAccessible(menuPopupFrame->GetContent());
    if (!menuPopup) 
      return nsnull;

    nsMenuFrame* menuFrame = menuPopupFrame->GetParentMenu();
    if (!menuFrame) 
      return nsnull;

    nsMenuParent* menuParent = menuFrame->GetMenuParent();
    if (!menuParent) 
      return menuPopup->Parent();

    if (menuParent->IsMenuBar()) { 
      nsMenuBarFrame* menuBarFrame = static_cast<nsMenuBarFrame*>(menuParent);
      return document->GetAccessible(menuBarFrame->GetContent());
    }

    
    if (!menuParent->IsMenu())
      return nsnull;

    menuPopupFrame = static_cast<nsMenuPopupFrame*>(menuParent);
  }

  NS_NOTREACHED("Shouldn't be a real case.");
  return nsnull;
}





nsXULMenubarAccessible::
  nsXULMenubarAccessible(nsIContent *aContent, nsIWeakReference *aShell) :
  nsAccessibleWrap(aContent, aShell)
{
}

PRUint64
nsXULMenubarAccessible::NativeState()
{
  PRUint64 state = nsAccessible::NativeState();

  
  state &= ~states::FOCUSABLE;
  return state;
}


nsresult
nsXULMenubarAccessible::GetNameInternal(nsAString& aName)
{
  aName.AssignLiteral("Application");
  return NS_OK;
}

role
nsXULMenubarAccessible::NativeRole()
{
  return roles::MENUBAR;
}




bool
nsXULMenubarAccessible::IsActiveWidget() const
{
  nsMenuBarFrame* menuBarFrame = do_QueryFrame(GetFrame());
  return menuBarFrame && menuBarFrame->IsActive();
}

bool
nsXULMenubarAccessible::AreItemsOperable() const
{
  return true;
}

nsAccessible*
nsXULMenubarAccessible::CurrentItem()
{
  nsMenuBarFrame* menuBarFrame = do_QueryFrame(GetFrame());
  if (menuBarFrame) {
    nsMenuFrame* menuFrame = menuBarFrame->GetCurrentMenuItem();
    if (menuFrame) {
      nsIContent* menuItemNode = menuFrame->GetContent();
      return GetAccService()->GetAccessible(menuItemNode);
    }
  }
  return nsnull;
}

void
nsXULMenubarAccessible::SetCurrentItem(nsAccessible* aItem)
{
  NS_ERROR("nsXULMenubarAccessible::SetCurrentItem not implemented");
}
