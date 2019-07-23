







































#include "nsMenuListener.h"
#include "nsMenuBarFrame.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMNSUIEvent.h"
#include "nsGUIEvent.h"


#include "nsIServiceManager.h"
#include "nsWidgetsCID.h"
#include "nsCOMPtr.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"

#include "nsIEventStateManager.h"

#include "nsISupportsArray.h"





NS_IMPL_ADDREF(nsMenuListener)
NS_IMPL_RELEASE(nsMenuListener)
NS_IMPL_QUERY_INTERFACE3(nsMenuListener, nsIDOMKeyListener, nsIDOMFocusListener, nsIDOMMouseListener)




nsMenuListener::nsMenuListener(nsIMenuParent* aMenuParent) 
{
  mMenuParent = aMenuParent;
}


nsMenuListener::~nsMenuListener() 
{
}




nsresult
nsMenuListener::KeyUp(nsIDOMEvent* aKeyEvent)
{
  aKeyEvent->StopPropagation();
  aKeyEvent->PreventDefault();

  return NS_ERROR_BASE; 
}


nsresult
nsMenuListener::KeyDown(nsIDOMEvent* aKeyEvent)
{
  PRInt32 menuAccessKey = -1;
  
  
  

  nsMenuBarListener::GetMenuAccessKey(&menuAccessKey);
  if (menuAccessKey) {
    PRUint32 theChar;
    nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
    keyEvent->GetKeyCode(&theChar);

    if (theChar == (PRUint32)menuAccessKey) {
      
      
      
      
      PRBool ctrl = PR_FALSE;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_CONTROL)
        keyEvent->GetCtrlKey(&ctrl);
      PRBool alt=PR_FALSE;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_ALT)
        keyEvent->GetAltKey(&alt);
      PRBool shift=PR_FALSE;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_SHIFT)
        keyEvent->GetShiftKey(&shift);
      PRBool meta=PR_FALSE;
      if (menuAccessKey != nsIDOMKeyEvent::DOM_VK_META)
        keyEvent->GetMetaKey(&meta);
      if (!(ctrl || alt || shift || meta)) {
        
        
        mMenuParent->DismissChain();
      }
    }
  }

  
  
  aKeyEvent->StopPropagation();
  aKeyEvent->PreventDefault();
  return NS_ERROR_BASE; 
}


nsresult
nsMenuListener::KeyPress(nsIDOMEvent* aKeyEvent)
{
  
  
  

  
  nsCOMPtr<nsIDOMNSEvent> domNSEvent = do_QueryInterface(aKeyEvent);
  PRBool trustedEvent = PR_FALSE;

  if (domNSEvent) {
    domNSEvent->GetIsTrusted(&trustedEvent);
  }

  if (!trustedEvent)
    return NS_OK;

  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
  PRUint32 theChar;
	keyEvent->GetKeyCode(&theChar);
  PRBool handled = PR_FALSE;

  if (theChar == NS_VK_LEFT ||
      theChar == NS_VK_RIGHT ||
      theChar == NS_VK_UP ||
      theChar == NS_VK_DOWN ||
      theChar == NS_VK_HOME ||
      theChar == NS_VK_END) {
    
    
	  mMenuParent->KeyboardNavigation(theChar, handled);
  }
  else if (theChar == NS_VK_ESCAPE) {
    
    
    NS_ADDREF_THIS();
	  mMenuParent->Escape(handled);
    NS_RELEASE_THIS();
    if (!handled)
      mMenuParent->DismissChain();
  }
  else if (theChar == NS_VK_TAB)
    mMenuParent->DismissChain();
  else if (theChar == NS_VK_ENTER ||
           theChar == NS_VK_RETURN) {
    
    mMenuParent->Enter();
  }
#if !defined(XP_MAC) && !defined(XP_MACOSX)
  else if (theChar == NS_VK_F10) {
    
    
    mMenuParent->DismissChain();
  }
#endif 
  else {
    PRInt32 menuAccessKey = -1;
    nsMenuBarListener::GetMenuAccessKey(&menuAccessKey);
    if (menuAccessKey) {
      
      
      
      mMenuParent->ShortcutNavigation(keyEvent, handled);
    }
  }

  aKeyEvent->StopPropagation();
  aKeyEvent->PreventDefault();
  return NS_ERROR_BASE; 
}



nsresult
nsMenuListener::Focus(nsIDOMEvent* aEvent)
{
  return NS_OK; 
}


nsresult
nsMenuListener::Blur(nsIDOMEvent* aEvent)
{
  
  return NS_OK; 
}
  

nsresult 
nsMenuListener::MouseDown(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; 
}


nsresult 
nsMenuListener::MouseUp(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; 
}

nsresult 
nsMenuListener::MouseClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; 
}


nsresult 
nsMenuListener::MouseDblClick(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; 
}


nsresult 
nsMenuListener::MouseOver(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; 
}


nsresult 
nsMenuListener::MouseOut(nsIDOMEvent* aMouseEvent)
{
  return NS_OK; 
}


nsresult
nsMenuListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}


