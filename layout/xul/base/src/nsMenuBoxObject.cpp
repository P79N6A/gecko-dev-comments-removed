




































#include "nsISupportsUtils.h"
#include "nsIMenuBoxObject.h"
#include "nsBoxObject.h"
#include "nsIPresShell.h"
#include "nsIFrame.h"
#include "nsGUIEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsMenuBarListener.h"
#include "nsMenuFrame.h"
#include "nsMenuPopupFrame.h"
#include "nsPopupSetFrame.h"

class nsMenuBoxObject : public nsIMenuBoxObject, public nsBoxObject
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMENUBOXOBJECT

  nsMenuBoxObject();
  virtual ~nsMenuBoxObject();
  
protected:
};


NS_IMPL_ADDREF(nsMenuBoxObject)
NS_IMPL_RELEASE(nsMenuBoxObject)

NS_IMETHODIMP 
nsMenuBoxObject::QueryInterface(REFNSIID iid, void** aResult)
{
  if (!aResult)
    return NS_ERROR_NULL_POINTER;
  
  if (iid.Equals(NS_GET_IID(nsIMenuBoxObject))) {
    *aResult = (nsIMenuBoxObject*)this;
    NS_ADDREF(this);
    return NS_OK;
  }

  return nsBoxObject::QueryInterface(iid, aResult);
}
  
nsMenuBoxObject::nsMenuBoxObject()
{
}

nsMenuBoxObject::~nsMenuBoxObject()
{
  
}


NS_IMETHODIMP nsMenuBoxObject::OpenMenu(PRBool aOpenFlag)
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
    nsIFrame* frame = GetFrame(PR_FALSE);
    if (frame) {
      if (aOpenFlag) {
        nsCOMPtr<nsIContent> content = mContent;
        pm->ShowMenu(content, PR_FALSE, PR_FALSE);
      }
      else {
        if (frame->GetType() == nsGkAtoms::menuFrame) {
          nsMenuPopupFrame* popupFrame = (NS_STATIC_CAST(nsMenuFrame *, frame))->GetPopup();
          if (popupFrame)
            pm->HidePopup(popupFrame->GetContent(), PR_FALSE, PR_TRUE, PR_FALSE);
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsMenuBoxObject::GetActiveChild(nsIDOMElement** aResult)
{
  *aResult = nsnull;
  nsIFrame* frame = GetFrame(PR_FALSE);
  if (frame && frame->GetType() == nsGkAtoms::menuFrame)
    return NS_STATIC_CAST(nsMenuFrame *, frame)->GetActiveChild(aResult);
  return NS_OK;
}

NS_IMETHODIMP nsMenuBoxObject::SetActiveChild(nsIDOMElement* aResult)
{
  nsIFrame* frame = GetFrame(PR_FALSE);
  if (frame && frame->GetType() == nsGkAtoms::menuFrame)
    return NS_STATIC_CAST(nsMenuFrame *, frame)->SetActiveChild(aResult);
  return NS_OK;
}


NS_IMETHODIMP nsMenuBoxObject::HandleKeyPress(nsIDOMKeyEvent* aKeyEvent, PRBool* aHandledFlag)
{
  *aHandledFlag = PR_FALSE;
  NS_ENSURE_ARG(aKeyEvent);

  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (!pm)
    return NS_OK;

  
  nsCOMPtr<nsIDOMNSUIEvent> uiEvent(do_QueryInterface(aKeyEvent));
  if (!uiEvent)
    return NS_OK;

  PRBool eventHandled = PR_FALSE;
  uiEvent->GetPreventDefault(&eventHandled);
  if (eventHandled)
    return NS_OK;

  if (nsMenuBarListener::IsAccessKeyPressed(aKeyEvent))
    return NS_OK;

  nsIFrame* frame = GetFrame(PR_FALSE);
  if (!frame || frame->GetType() != nsGkAtoms::menuFrame)
    return NS_OK;

  nsMenuPopupFrame* popupFrame = NS_STATIC_CAST(nsMenuFrame *, frame)->GetPopup();
  if (!popupFrame)
    return NS_OK;

  PRUint32 keyCode;
  aKeyEvent->GetKeyCode(&keyCode);
  switch (keyCode) {
    case NS_VK_UP:
    case NS_VK_DOWN:
    case NS_VK_HOME:
    case NS_VK_END:
      *aHandledFlag = pm->HandleKeyboardNavigation(keyCode);
      return NS_OK;
    default:
      *aHandledFlag = pm->HandleShortcutNavigation(aKeyEvent);
      return NS_OK;
  }
}



nsresult
NS_NewMenuBoxObject(nsIBoxObject** aResult)
{
  *aResult = new nsMenuBoxObject;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}

