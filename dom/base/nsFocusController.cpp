







































#include "nsIContent.h"
#include "nsIControllers.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMNSHTMLInputElement.h"
#include "nsIDOMNSHTMLTextAreaElement.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDocument.h"
#include "nsPIDOMWindow.h"
#include "nsFocusController.h"
#include "prlog.h"
#include "nsGlobalWindow.h"
#include "nsFocusManager.h"

#ifdef MOZ_XUL
#include "nsIDOMXULDocument.h"
#include "nsIDOMXULElement.h"
#endif



NS_IMPL_CYCLE_COLLECTION_CLASS(nsFocusController)

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsFocusController, nsIFocusController)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsFocusController,
                                           nsIFocusController)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsFocusController)
  NS_INTERFACE_MAP_ENTRY(nsIFocusController)
  NS_INTERFACE_MAP_ENTRY(nsSupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIFocusController)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_0(nsFocusController)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsFocusController)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPopupNode)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

static nsIContent*
GetRootFocusedContentAndWindow(nsPIDOMWindow* aContextWindow,
                               nsPIDOMWindow** aWindow)
{
  *aWindow = nsnull;

  if (aContextWindow) {
    nsCOMPtr<nsPIDOMWindow> rootWindow = aContextWindow->GetPrivateRoot();
    if (rootWindow) {
      return nsFocusManager::GetFocusedDescendant(rootWindow, PR_TRUE, aWindow);
    }
  }

  return nsnull;
}

NS_IMETHODIMP
nsFocusController::Create(nsIFocusController** aResult)
{
  nsFocusController* controller = new nsFocusController();
  if (!controller)
    return NS_ERROR_OUT_OF_MEMORY;

  *aResult = controller;
  NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::GetControllers(nsPIDOMWindow* aContextWindow, nsIControllers** aResult)
{
  *aResult = nsnull;

  
  
  

  nsCOMPtr<nsPIDOMWindow> focusedWindow;
  nsIContent* focusedContent =
    GetRootFocusedContentAndWindow(aContextWindow, getter_AddRefs(focusedWindow));
  if (focusedContent) {
#ifdef MOZ_XUL
    nsCOMPtr<nsIDOMXULElement> xulElement(do_QueryInterface(focusedContent));
    if (xulElement)
      return xulElement->GetControllers(aResult);
#endif

    nsCOMPtr<nsIDOMNSHTMLTextAreaElement> htmlTextArea =
      do_QueryInterface(focusedContent);
    if (htmlTextArea)
      return htmlTextArea->GetControllers(aResult);

    nsCOMPtr<nsIDOMNSHTMLInputElement> htmlInputElement =
      do_QueryInterface(focusedContent);
    if (htmlInputElement)
      return htmlInputElement->GetControllers(aResult);

    if (focusedContent->IsEditable() && focusedWindow)
      return focusedWindow->GetControllers(aResult);
  }
  else {
    nsCOMPtr<nsIDOMWindowInternal> domWindow = do_QueryInterface(focusedWindow);
    if (domWindow)
      return domWindow->GetControllers(aResult);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::GetControllerForCommand(nsPIDOMWindow* aContextWindow,
                                           const char * aCommand,
                                           nsIController** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;

  nsCOMPtr<nsIControllers> controllers;
  nsCOMPtr<nsIController> controller;

  GetControllers(aContextWindow, getter_AddRefs(controllers));
  if(controllers) {
    controllers->GetControllerForCommand(aCommand, getter_AddRefs(controller));
    if(controller) {
      controller.swap(*_retval);
      return NS_OK;
    }
  }

  nsCOMPtr<nsPIDOMWindow> focusedWindow;
  GetRootFocusedContentAndWindow(aContextWindow, getter_AddRefs(focusedWindow));
  while (focusedWindow) {
    nsCOMPtr<nsIDOMWindowInternal> domWindow(do_QueryInterface(focusedWindow));

    nsCOMPtr<nsIControllers> controllers2;
    domWindow->GetControllers(getter_AddRefs(controllers2));
    if(controllers2) {
      controllers2->GetControllerForCommand(aCommand,
                                            getter_AddRefs(controller));
      if(controller) {
        controller.swap(*_retval);
        return NS_OK;
      }
    }

    
    nsCOMPtr<nsPIDOMWindow> piWindow = do_QueryInterface(focusedWindow); 
    nsGlobalWindow *win =
      static_cast<nsGlobalWindow *>
                 (static_cast<nsIDOMWindowInternal *>(piWindow));
    focusedWindow = win->GetPrivateParent();
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::GetPopupNode(nsIDOMNode** aNode)
{
#ifdef DEBUG_dr
  printf("dr :: nsFocusController::GetPopupNode\n");
#endif

  *aNode = mPopupNode;
  NS_IF_ADDREF(*aNode);
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::SetPopupNode(nsIDOMNode* aNode)
{
#ifdef DEBUG_dr
  printf("dr :: nsFocusController::SetPopupNode\n");
#endif

  if (aNode) {
    nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
    NS_ENSURE_ARG(node);
  }
  mPopupNode = aNode;
  return NS_OK;
}
