




































#include "nsISupportsUtils.h"
#include "nsIMenuBoxObject.h"
#include "nsBoxObject.h"
#include "nsIFrame.h"
#include "nsGUIEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsMenuBarFrame.h"
#include "nsMenuBarListener.h"
#include "nsMenuFrame.h"
#include "nsMenuPopupFrame.h"

class nsMenuBoxObject : public nsIMenuBoxObject, public nsBoxObject
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIMENUBOXOBJECT

  nsMenuBoxObject();
  virtual ~nsMenuBoxObject();
};

nsMenuBoxObject::nsMenuBoxObject()
{
}

nsMenuBoxObject::~nsMenuBoxObject()
{
}

NS_IMPL_ISUPPORTS_INHERITED1(nsMenuBoxObject, nsBoxObject, nsIMenuBoxObject)


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
          nsMenuPopupFrame* popupFrame = (static_cast<nsMenuFrame *>(frame))->GetPopup();
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
    return static_cast<nsMenuFrame *>(frame)->GetActiveChild(aResult);
  return NS_OK;
}

NS_IMETHODIMP nsMenuBoxObject::SetActiveChild(nsIDOMElement* aResult)
{
  nsIFrame* frame = GetFrame(PR_FALSE);
  if (frame && frame->GetType() == nsGkAtoms::menuFrame)
    return static_cast<nsMenuFrame *>(frame)->SetActiveChild(aResult);
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

  nsMenuPopupFrame* popupFrame = static_cast<nsMenuFrame *>(frame)->GetPopup();
  if (!popupFrame)
    return NS_OK;

  PRUint32 keyCode;
  aKeyEvent->GetKeyCode(&keyCode);
  switch (keyCode) {
    case NS_VK_UP:
    case NS_VK_DOWN:
    case NS_VK_HOME:
    case NS_VK_END:
    {
      nsNavigationDirection theDirection;
      theDirection = NS_DIRECTION_FROM_KEY_CODE(popupFrame, keyCode);
      *aHandledFlag =
        pm->HandleKeyboardNavigationInPopup(popupFrame, theDirection);
      return NS_OK;
    }
    default:
      *aHandledFlag = pm->HandleShortcutNavigation(aKeyEvent, popupFrame);
      return NS_OK;
  }
}

NS_IMETHODIMP
nsMenuBoxObject::GetOpenedWithKey(PRBool* aOpenedWithKey)
{
  *aOpenedWithKey = PR_FALSE;

  nsIFrame* frame = GetFrame(PR_FALSE);
  if (!frame || frame->GetType() != nsGkAtoms::menuFrame)
    return NS_OK;

  frame = frame->GetParent();
  while (frame) {
    if (frame->GetType() == nsGkAtoms::menuBarFrame) {
      *aOpenedWithKey = (static_cast<nsMenuBarFrame *>(frame))->IsActiveByKeyboard();
      return NS_OK;
    }
    frame = frame->GetParent();
  }

  return NS_OK;
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

