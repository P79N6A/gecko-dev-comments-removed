




#include "nsMenuBarListener.h"
#include "nsMenuBarFrame.h"
#include "nsMenuPopupFrame.h"
#include "nsIDOMEvent.h"
#include "nsGUIEvent.h"


#include "nsIServiceManager.h"
#include "nsWidgetsCID.h"
#include "nsCOMPtr.h"
#include "nsIDOMKeyEvent.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"

#include "nsContentUtils.h"
#include "mozilla/Preferences.h"

using namespace mozilla;





NS_IMPL_ISUPPORTS1(nsMenuBarListener, nsIDOMEventListener)

#define MODIFIER_SHIFT    1
#define MODIFIER_CONTROL  2
#define MODIFIER_ALT      4
#define MODIFIER_META     8
#define MODIFIER_OS       16



PRInt32 nsMenuBarListener::mAccessKey = -1;
PRUint32 nsMenuBarListener::mAccessKeyMask = 0;
bool nsMenuBarListener::mAccessKeyFocuses = false;

nsMenuBarListener::nsMenuBarListener(nsMenuBarFrame* aMenuBar) 
  :mAccessKeyDown(false), mAccessKeyDownCanceled(false)
{
  mMenuBarFrame = aMenuBar;
}


nsMenuBarListener::~nsMenuBarListener() 
{
}

nsresult
nsMenuBarListener::GetMenuAccessKey(PRInt32* aAccessKey)
{
  if (!aAccessKey)
    return NS_ERROR_INVALID_POINTER;
  InitAccessKey();
  *aAccessKey = mAccessKey;
  return NS_OK;
}

void nsMenuBarListener::InitAccessKey()
{
  if (mAccessKey >= 0)
    return;

  
  
#ifdef XP_MACOSX
  mAccessKey = 0;
  mAccessKeyMask = 0;
#else
  mAccessKey = nsIDOMKeyEvent::DOM_VK_ALT;
  mAccessKeyMask = MODIFIER_ALT;
#endif

  
  mAccessKey = Preferences::GetInt("ui.key.menuAccessKey", mAccessKey);
  if (mAccessKey == nsIDOMKeyEvent::DOM_VK_SHIFT)
    mAccessKeyMask = MODIFIER_SHIFT;
  else if (mAccessKey == nsIDOMKeyEvent::DOM_VK_CONTROL)
    mAccessKeyMask = MODIFIER_CONTROL;
  else if (mAccessKey == nsIDOMKeyEvent::DOM_VK_ALT)
    mAccessKeyMask = MODIFIER_ALT;
  else if (mAccessKey == nsIDOMKeyEvent::DOM_VK_META)
    mAccessKeyMask = MODIFIER_META;
  else if (mAccessKey == nsIDOMKeyEvent::DOM_VK_WIN)
    mAccessKeyMask = MODIFIER_OS;

  mAccessKeyFocuses = Preferences::GetBool("ui.key.menuAccessKeyFocuses");
}

void
nsMenuBarListener::ToggleMenuActiveState()
{
  nsMenuFrame* closemenu = mMenuBarFrame->ToggleMenuActiveState();
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && closemenu) {
    nsMenuPopupFrame* popupFrame = closemenu->GetPopup();
    if (popupFrame)
      pm->HidePopup(popupFrame->GetContent(), false, false, true);
  }
}


nsresult
nsMenuBarListener::KeyUp(nsIDOMEvent* aKeyEvent)
{  
  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
  if (!keyEvent) {
    return NS_OK;
  }

  InitAccessKey();

  
  bool trustedEvent = false;
  aKeyEvent->GetIsTrusted(&trustedEvent);

  if (!trustedEvent) {
    return NS_OK;
  }

  if (mAccessKey && mAccessKeyFocuses)
  {
    bool defaultPrevented = false;
    aKeyEvent->GetDefaultPrevented(&defaultPrevented);

    
    
    
    PRUint32 theChar;
    keyEvent->GetKeyCode(&theChar);

    if (!defaultPrevented && mAccessKeyDown && !mAccessKeyDownCanceled &&
        (PRInt32)theChar == mAccessKey)
    {
      
      
      if (!mMenuBarFrame->IsActive()) {
        mMenuBarFrame->SetActiveByKeyboard();
      }
      ToggleMenuActiveState();
    }
    mAccessKeyDown = false;
    mAccessKeyDownCanceled = false;

    bool active = mMenuBarFrame->IsActive();
    if (active) {
      aKeyEvent->StopPropagation();
      aKeyEvent->PreventDefault();
      return NS_OK; 
    }
  }
  
  return NS_OK; 
}


nsresult
nsMenuBarListener::KeyPress(nsIDOMEvent* aKeyEvent)
{
  
  if (aKeyEvent) {
    bool eventHandled = false;
    aKeyEvent->GetPreventDefault(&eventHandled);
    if (eventHandled) {
      return NS_OK;       
    }
  }

  
  bool trustedEvent = false;
  if (aKeyEvent) {
    aKeyEvent->GetIsTrusted(&trustedEvent);
  }

  if (!trustedEvent)
    return NS_OK;

  nsresult retVal = NS_OK;  
  
  InitAccessKey();

  if (mAccessKey)
  {
    bool preventDefault;
    aKeyEvent->GetPreventDefault(&preventDefault);
    if (!preventDefault) {
      nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
      PRUint32 keyCode, charCode;
      keyEvent->GetKeyCode(&keyCode);
      keyEvent->GetCharCode(&charCode);

      bool hasAccessKeyCandidates = charCode != 0;
      if (!hasAccessKeyCandidates) {
        nsEvent* nativeEvent = nsContentUtils::GetNativeEvent(aKeyEvent);
        nsKeyEvent* nativeKeyEvent = static_cast<nsKeyEvent*>(nativeEvent);
        if (nativeKeyEvent) {
          nsAutoTArray<PRUint32, 10> keys;
          nsContentUtils::GetAccessKeyCandidates(nativeKeyEvent, keys);
          hasAccessKeyCandidates = !keys.IsEmpty();
        }
      }

      
      if (keyCode != (PRUint32)mAccessKey) {
        mAccessKeyDownCanceled = true;
      }

      if (IsAccessKeyPressed(keyEvent) && hasAccessKeyCandidates) {
        
        
        
        nsMenuFrame* result = mMenuBarFrame->FindMenuWithShortcut(keyEvent);
        if (result) {
          mMenuBarFrame->SetActiveByKeyboard();
          mMenuBarFrame->SetActive(true);
          result->OpenMenu(true);

          
          
          mAccessKeyDown = mAccessKeyDownCanceled = false;

          aKeyEvent->StopPropagation();
          aKeyEvent->PreventDefault();
          retVal = NS_OK;       
        }
      }    
#ifndef XP_MACOSX
      
      else if (keyCode == NS_VK_F10) {
        if ((GetModifiers(keyEvent) & ~MODIFIER_CONTROL) == 0) {
          
          
          mMenuBarFrame->SetActiveByKeyboard();
          ToggleMenuActiveState();

          if (mMenuBarFrame->IsActive()) {
            aKeyEvent->StopPropagation();
            aKeyEvent->PreventDefault();
            return NS_OK; 
          }
        }
      }
#endif 
    } 
  }

  return retVal;
}

bool
nsMenuBarListener::IsAccessKeyPressed(nsIDOMKeyEvent* aKeyEvent)
{
  InitAccessKey();
  
  PRUint32 modifiers = GetModifiers(aKeyEvent);

  return (mAccessKeyMask != MODIFIER_SHIFT &&
          (modifiers & mAccessKeyMask) &&
          (modifiers & ~(mAccessKeyMask | MODIFIER_SHIFT)) == 0);
}

PRUint32
nsMenuBarListener::GetModifiers(nsIDOMKeyEvent* aKeyEvent)
{
  PRUint32 modifiers = 0;
  nsInputEvent* inputEvent =
    static_cast<nsInputEvent*>(aKeyEvent->GetInternalNSEvent());
  MOZ_ASSERT(inputEvent);

  if (inputEvent->IsShift()) {
    modifiers |= MODIFIER_SHIFT;
  }

  if (inputEvent->IsControl()) {
    modifiers |= MODIFIER_CONTROL;
  }

  if (inputEvent->IsAlt()) {
    modifiers |= MODIFIER_ALT;
  }

  if (inputEvent->IsMeta()) {
    modifiers |= MODIFIER_META;
  }

  if (inputEvent->IsOS()) {
    modifiers |= MODIFIER_OS;
  }

  return modifiers;
}


nsresult
nsMenuBarListener::KeyDown(nsIDOMEvent* aKeyEvent)
{
  InitAccessKey();

  
  bool trustedEvent = false;
  if (aKeyEvent) {
    aKeyEvent->GetIsTrusted(&trustedEvent);
  }

  if (!trustedEvent)
    return NS_OK;

  if (mAccessKey && mAccessKeyFocuses)
  {
    bool defaultPrevented = false;
    aKeyEvent->GetDefaultPrevented(&defaultPrevented);

    nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
    PRUint32 theChar;
    keyEvent->GetKeyCode(&theChar);

    
    
    
    bool isAccessKeyDownEvent =
      ((theChar == (PRUint32)mAccessKey) &&
       (GetModifiers(keyEvent) & ~mAccessKeyMask) == 0);

    if (!mAccessKeyDown) {
      
      
      if (!isAccessKeyDownEvent) {
        return NS_OK;
      }

      
      mAccessKeyDown = true;
      
      mAccessKeyDownCanceled = defaultPrevented;
      return NS_OK;
    }

    
    
    if (mAccessKeyDownCanceled || defaultPrevented) {
      return NS_OK;
    }

    
    
    mAccessKeyDownCanceled = !isAccessKeyDownEvent;
  }

  return NS_OK; 
}



nsresult
nsMenuBarListener::Blur(nsIDOMEvent* aEvent)
{
  if (!mMenuBarFrame->IsMenuOpen() && mMenuBarFrame->IsActive()) {
    ToggleMenuActiveState();
  }
  
  
  mAccessKeyDown = false;
  mAccessKeyDownCanceled = false;
  return NS_OK; 
}
  

nsresult 
nsMenuBarListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  

  
  
  
  if (mAccessKeyDown) {
    mAccessKeyDownCanceled = true;
  }

  PRUint16 phase = 0;
  nsresult rv = aMouseEvent->GetEventPhase(&phase);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (phase == nsIDOMEvent::CAPTURING_PHASE) {
    return NS_OK;
  }

  if (!mMenuBarFrame->IsMenuOpen() && mMenuBarFrame->IsActive())
    ToggleMenuActiveState();

  return NS_OK; 
}


nsresult
nsMenuBarListener::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString eventType;
  aEvent->GetType(eventType);
  
  if (eventType.EqualsLiteral("keyup")) {
    return KeyUp(aEvent);
  }
  if (eventType.EqualsLiteral("keydown")) {
    return KeyDown(aEvent);
  }
  if (eventType.EqualsLiteral("keypress")) {
    return KeyPress(aEvent);
  }
  if (eventType.EqualsLiteral("blur")) {
    return Blur(aEvent);
  }
  if (eventType.EqualsLiteral("mousedown")) {
    return MouseDown(aEvent);
  }

  NS_ABORT();

  return NS_OK;
}
