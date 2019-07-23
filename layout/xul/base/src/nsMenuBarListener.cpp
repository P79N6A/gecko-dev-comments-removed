







































#include "nsMenuBarListener.h"
#include "nsMenuBarFrame.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMEventReceiver.h"
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

#include "nsISupportsArray.h"
#include "nsContentUtils.h"





NS_IMPL_ADDREF(nsMenuBarListener)
NS_IMPL_RELEASE(nsMenuBarListener)
NS_IMPL_QUERY_INTERFACE3(nsMenuBarListener, nsIDOMKeyListener, nsIDOMFocusListener, nsIDOMMouseListener)

#define MODIFIER_SHIFT    1
#define MODIFIER_CONTROL  2
#define MODIFIER_ALT      4
#define MODIFIER_META     8



PRInt32 nsMenuBarListener::mAccessKey = -1;
PRUint32 nsMenuBarListener::mAccessKeyMask = 0;
PRBool nsMenuBarListener::mAccessKeyFocuses = PR_FALSE;

nsMenuBarListener::nsMenuBarListener(nsMenuBarFrame* aMenuBar) 
  :mAccessKeyDown(PR_FALSE)
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

  
  
#if !(defined(XP_MAC) || defined(XP_MACOSX))
  mAccessKey = nsIDOMKeyEvent::DOM_VK_ALT;
  mAccessKeyMask = MODIFIER_ALT;
#else
  mAccessKey = 0;
  mAccessKeyMask = 0;
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

    if (mAccessKeyDown && (PRInt32)theChar == mAccessKey)
    {
      
      
      mMenuBarFrame->ToggleMenuActiveState();
    }
    mAccessKeyDown = PR_FALSE; 

    PRBool active = mMenuBarFrame->IsActive();
    if (active) {
      aKeyEvent->StopPropagation();
      aKeyEvent->PreventDefault();
      return NS_ERROR_BASE; 
    }
  }
  
  return NS_OK; 
}


nsresult
nsMenuBarListener::KeyPress(nsIDOMEvent* aKeyEvent)
{
  mMenuBarFrame->ClearRecentlyRolledUp();

  
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

      
      if (keyCode != (PRUint32)mAccessKey)
        mAccessKeyDown = PR_FALSE;

      
      
      if (IsAccessKeyPressed(keyEvent) && charCode)
      {
        
        
        
        PRBool active = PR_FALSE;
        mMenuBarFrame->ShortcutNavigation(keyEvent, active);

        if (active) {
          aKeyEvent->StopPropagation();
          aKeyEvent->PreventDefault();

          retVal = NS_ERROR_BASE;       
        }
      }    
#if !defined(XP_MAC) && !defined(XP_MACOSX)
      
      else if (keyCode == NS_VK_F10) {
        if ((GetModifiers(keyEvent) & ~MODIFIER_CONTROL) == 0) {
          
          
          mMenuBarFrame->ToggleMenuActiveState();

          aKeyEvent->StopPropagation();
          aKeyEvent->PreventDefault();
          return NS_ERROR_BASE; 
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

    if (theChar == (PRUint32)mAccessKey && (GetModifiers(keyEvent) & ~mAccessKeyMask) == 0) {
      
      
      
      
      mAccessKeyDown = PR_TRUE;
    }
    else {
      
      
      

      mAccessKeyDown = PR_FALSE;
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
  if (!mMenuBarFrame->IsOpen() && mMenuBarFrame->IsActive()) {
    mMenuBarFrame->ToggleMenuActiveState();
    PRBool handled;
    mMenuBarFrame->Escape(handled);
    mAccessKeyDown = PR_FALSE;
  }
  return NS_OK; 
}
  

nsresult 
nsMenuBarListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  if (!mMenuBarFrame->IsOpen() && mMenuBarFrame->IsActive()) {
    mMenuBarFrame->ToggleMenuActiveState();
    PRBool handled;
    mMenuBarFrame->Escape(handled);
  }

  mAccessKeyDown = PR_FALSE;

  return NS_OK; 
}


nsresult 
nsMenuBarListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
  mMenuBarFrame->ClearRecentlyRolledUp();

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
