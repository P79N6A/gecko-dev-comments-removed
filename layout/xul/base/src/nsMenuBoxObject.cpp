




































#include "nsISupportsUtils.h"
#include "nsIMenuBoxObject.h"
#include "nsBoxObject.h"
#include "nsIPresShell.h"
#include "nsIMenuFrame.h"
#include "nsIFrame.h"
#include "nsGUIEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsMenuBarListener.h"
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
  NS_PRECONDITION(aResult, "null out param");

  if (iid.Equals(NS_GET_IID(nsIMenuBoxObject))) {
    *aResult = NS_STATIC_CAST(nsIMenuBoxObject*, this);
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
  nsIFrame* frame = GetFrame(PR_FALSE);
  if (!frame)
    return NS_OK;

  if (!nsPopupSetFrame::MayOpenPopup(frame))
    return NS_OK;

  nsIMenuFrame* menuFrame;
  CallQueryInterface(frame, &menuFrame);
  if (!menuFrame)
    return NS_OK;

  return menuFrame->OpenMenu(aOpenFlag);
}

NS_IMETHODIMP nsMenuBoxObject::GetActiveChild(nsIDOMElement** aResult)
{
  *aResult = nsnull;
  nsIFrame* frame = GetFrame(PR_FALSE);
  if (!frame)
    return NS_OK;

  nsIMenuFrame* menuFrame;
  CallQueryInterface(frame, &menuFrame);
  if (menuFrame)
    menuFrame->GetActiveChild(aResult);
  return NS_OK;
}

NS_IMETHODIMP nsMenuBoxObject::SetActiveChild(nsIDOMElement* aResult)
{
  nsIFrame* frame = GetFrame(PR_FALSE);
  if (!frame)
    return NS_OK;

  nsIMenuFrame* menuFrame;
  CallQueryInterface(frame, &menuFrame);
  if (menuFrame) {
    menuFrame->MarkAsGenerated();
    menuFrame->SetActiveChild(aResult);
  }
  return NS_OK;
}


NS_IMETHODIMP nsMenuBoxObject::HandleKeyPress(nsIDOMKeyEvent* aKeyEvent, PRBool* aHandledFlag)
{
  *aHandledFlag = PR_FALSE;
  NS_ENSURE_ARG(aKeyEvent);

  
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
  if (!frame)
    return NS_OK;

  nsIMenuFrame* menuFrame;
  CallQueryInterface(frame, &menuFrame);
  if (!menuFrame)
    return NS_OK;

  PRUint32 keyCode;
  aKeyEvent->GetKeyCode(&keyCode);
  switch (keyCode) {
    case NS_VK_UP:
    case NS_VK_DOWN:
    case NS_VK_HOME:
    case NS_VK_END:
      return menuFrame->KeyboardNavigation(keyCode, *aHandledFlag);
    default:
      return menuFrame->ShortcutNavigation(aKeyEvent, *aHandledFlag);
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

