







































#include "nsMenuBarListener.h"
#include "nsMenuBarFrame.h"
#include "nsMenuPopupFrame.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsGUIEvent.h"


#include "nsIServiceManager.h"
#include "nsWidgetsCID.h"
#include "nsCOMPtr.h"
#include "nsIDOMKeyEvent.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"

#include "nsIEventStateManager.h"

#include "nsContentUtils.h"





NS_IMPL_ADDREF(nsMenuBarListener)
NS_IMPL_RELEASE(nsMenuBarListener)
NS_INTERFACE_MAP_BEGIN(nsMenuBarListener)
                     NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMFocusListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener,nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
NS_INTERFACE_MAP_END

#define MODIFIER_SHIFT    1
#define MODIFIER_CONTROL  2
#define MODIFIER_ALT      4
#define MODIFIER_META     8



PRInt32 nsMenuBarListener::mAccessKey = -1;
PRUint32 nsMenuBarListener::mAccessKeyMask = 0;
PRBool nsMenuBarListener::mAccessKeyFocuses = PR_FALSE;

nsMenuBarListener::nsMenuBarListener(nsMenuBarFrame* aMenuBar) 
  :mAccessKeyDown(PR_FALSE), mAccessKeyDownCanceled(PR_FALSE)
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

  
  mAccessKey = nsContentUtils::GetIntPref("ui.key.menuAccessKey", mAccessKey);
  if (mAccessKey == nsIDOMKeyEvent::DOM_VK_SHIFT)
    mAccessKeyMask = MODIFIER_SHIFT;
  else if (mAccessKey == nsIDOMKeyEvent::DOM_VK_CONTROL)
    mAccessKeyMask = MODIFIER_CONTROL;
  else if (mAccessKey == nsIDOMKeyEvent::DOM_VK_ALT)
    mAccessKeyMask = MODIFIER_ALT;
  else if (mAccessKey == nsIDOMKeyEvent::DOM_VK_META)
    mAccessKeyMask = MODIFIER_META;

  mAccessKeyFocuses =
    nsContentUtils::GetBoolPref("ui.key.menuAccessKeyFocuses");
}

void
nsMenuBarListener::ToggleMenuActiveState()
{
  nsMenuFrame* closemenu = mMenuBarFrame->ToggleMenuActiveState();
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm && closemenu) {
    nsMenuPopupFrame* popupFrame = closemenu->GetPopup();
    if (popupFrame)
      pm->HidePopup(popupFrame->GetContent(), PR_FALSE, PR_FALSE, PR_TRUE);
  }
}


nsresult
nsMenuBarListener::KeyUp(nsIDOMEvent* aKeyEvent)
{  
  InitAccessKey();

  
  nsCOMPtr<nsIDOMNSEvent> domNSEvent = do_QueryInterface(aKeyEvent);
  PRBool trustedEvent = PR_FALSE;

  if (domNSEvent) {
    domNSEvent->GetIsTrusted(&trustedEvent);
  }

  if (!trustedEvent)
    return NS_OK;

  if (mAccessKey && mAccessKeyFocuses)
  {
    
    
    
    nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
    PRUint32 theChar;
    keyEvent->GetKeyCode(&theChar);

    if (mAccessKeyDown && !mAccessKeyDownCanceled &&
        (PRInt32)theChar == mAccessKey)
    {
      
      
      if (!mMenuBarFrame->IsActive()) {
        mMenuBarFrame->SetActiveByKeyboard();
      }
      ToggleMenuActiveState();
    }
    mAccessKeyDown = PR_FALSE;
    mAccessKeyDownCanceled = PR_FALSE;

    PRBool active = mMenuBarFrame->IsActive();
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
  
  nsCOMPtr<nsIDOMNSUIEvent> uiEvent ( do_QueryInterface(aKeyEvent) );
  if ( uiEvent ) {
    PRBool eventHandled = PR_FALSE;
    uiEvent->GetPreventDefault ( &eventHandled );
    if ( eventHandled )
      return NS_OK;       
  }

  
  nsCOMPtr<nsIDOMNSEvent> domNSEvent = do_QueryInterface(aKeyEvent);
  PRBool trustedEvent = PR_FALSE;
  if (domNSEvent) {
    domNSEvent->GetIsTrusted(&trustedEvent);
  }

  if (!trustedEvent)
    return NS_OK;

  nsresult retVal = NS_OK;  
  
  InitAccessKey();

  if (mAccessKey)
  {
    nsCOMPtr<nsIDOMNSUIEvent> nsUIEvent = do_QueryInterface(aKeyEvent);

    PRBool preventDefault;

    nsUIEvent->GetPreventDefault(&preventDefault);
    if (!preventDefault) {
      nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
      PRUint32 keyCode, charCode;
      keyEvent->GetKeyCode(&keyCode);
      keyEvent->GetCharCode(&charCode);

      PRBool hasAccessKeyCandidates = charCode != 0;
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
        mAccessKeyDownCanceled = PR_TRUE;
      }

      if (IsAccessKeyPressed(keyEvent) && hasAccessKeyCandidates) {
        
        
        
        nsMenuFrame* result = mMenuBarFrame->FindMenuWithShortcut(keyEvent);
        if (result) {
          mMenuBarFrame->SetActiveByKeyboard();
          mMenuBarFrame->SetActive(PR_TRUE);
          result->OpenMenu(PR_TRUE);

          
          
          mAccessKeyDown = mAccessKeyDownCanceled = PR_FALSE;

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

          aKeyEvent->StopPropagation();
          aKeyEvent->PreventDefault();
          return NS_OK; 
        }
      }
#endif 
    } 
  }

  return retVal;
}

PRBool
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
  PRBool modifier;

  aKeyEvent->GetShiftKey(&modifier);
  if (modifier)
    modifiers |= MODIFIER_SHIFT;

  aKeyEvent->GetCtrlKey(&modifier);
  if (modifier)
    modifiers |= MODIFIER_CONTROL;

  aKeyEvent->GetAltKey(&modifier);
  if (modifier)
    modifiers |= MODIFIER_ALT;

  aKeyEvent->GetMetaKey(&modifier);
  if (modifier)
    modifiers |= MODIFIER_META;

  return modifiers;
}


nsresult
nsMenuBarListener::KeyDown(nsIDOMEvent* aKeyEvent)
{
  InitAccessKey();

  
  nsCOMPtr<nsIDOMNSEvent> domNSEvent = do_QueryInterface(aKeyEvent);
  PRBool trustedEvent = PR_FALSE;

  if (domNSEvent) {
    domNSEvent->GetIsTrusted(&trustedEvent);
  }

  if (!trustedEvent)
    return NS_OK;

  if (mAccessKey && mAccessKeyFocuses)
  {
    nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
    PRUint32 theChar;
    keyEvent->GetKeyCode(&theChar);

    if (!mAccessKeyDownCanceled && theChar == (PRUint32)mAccessKey &&
        (GetModifiers(keyEvent) & ~mAccessKeyMask) == 0) {
      
      
      
      
      mAccessKeyDown = PR_TRUE;
    }
    else {
      
      
      

      mAccessKeyDownCanceled = PR_TRUE;
    }
  }

  return NS_OK; 
}



nsresult
nsMenuBarListener::Focus(nsIDOMEvent* aEvent)
{
  return NS_OK; 
}


nsresult
nsMenuBarListener::Blur(nsIDOMEvent* aEvent)
{
  if (!mMenuBarFrame->IsMenuOpen() && mMenuBarFrame->IsActive()) {
    ToggleMenuActiveState();
    mAccessKeyDown = PR_FALSE;
    mAccessKeyDownCanceled = PR_FALSE;
  }
  return NS_OK; 
}
  

nsresult 
nsMenuBarListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  

  
  
  
  if (mAccessKeyDown) {
    mAccessKeyDownCanceled = PR_TRUE;
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
nsMenuBarListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; 
}

nsresult 
nsMenuBarListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; 
}


nsresult 
nsMenuBarListener::MouseDblClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; 
}


nsresult 
nsMenuBarListener::MouseOver(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; 
}


nsresult 
nsMenuBarListener::MouseOut(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; 
}


nsresult
nsMenuBarListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}
