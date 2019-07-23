







































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
nsFocusController::GetControllers(nsIControllers** aResult)
{
  
  
  
  nsCOMPtr<nsIDOMElement> focusedElement;
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm) {
    *aResult = nsnull;
    return NS_OK;
  }

  fm->GetFocusedElement(getter_AddRefs(focusedElement));
  if (focusedElement) {
#ifdef MOZ_XUL
    nsCOMPtr<nsIDOMXULElement> xulElement(do_QueryInterface(focusedElement));
    if (xulElement)
      return xulElement->GetControllers(aResult);
#endif

    nsCOMPtr<nsIDOMNSHTMLTextAreaElement> htmlTextArea =
      do_QueryInterface(focusedElement);
    if (htmlTextArea)
      return htmlTextArea->GetControllers(aResult);

    nsCOMPtr<nsIDOMNSHTMLInputElement> htmlInputElement =
      do_QueryInterface(focusedElement);
    if (htmlInputElement)
      return htmlInputElement->GetControllers(aResult);

    nsCOMPtr<nsIContent> content = do_QueryInterface(focusedElement);
    if (content && content->IsEditable()) {
      
      nsCOMPtr<nsIDOMDocument> domDoc;
      focusedElement->GetOwnerDocument(getter_AddRefs(domDoc));
      nsCOMPtr<nsIDOMWindowInternal> domWindow =
        do_QueryInterface(GetWindowFromDocument(domDoc));
      if (domWindow)
        return domWindow->GetControllers(aResult);
    }
  }
  else {
    nsCOMPtr<nsIDOMWindow> focusedWindow;
    fm->GetFocusedWindow(getter_AddRefs(focusedWindow));
    nsCOMPtr<nsIDOMWindowInternal> domWindow = do_QueryInterface(focusedWindow);
    if (domWindow)
      return domWindow->GetControllers(aResult);
  }

  *aResult = nsnull;
  return NS_OK;
}


nsPIDOMWindow *
nsFocusController::GetWindowFromDocument(nsIDOMDocument* aDocument)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDocument);
  if (!doc)
    return nsnull;

  return doc->GetWindow();
}

NS_IMETHODIMP
nsFocusController::GetControllerForCommand(const char * aCommand,
                                           nsIController** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);	
  *_retval = nsnull;

  nsCOMPtr<nsIControllers> controllers;
  nsCOMPtr<nsIController> controller;

  GetControllers(getter_AddRefs(controllers));
  if(controllers) {
    controllers->GetControllerForCommand(aCommand, getter_AddRefs(controller));
    if(controller) {
      controller.swap(*_retval);
      return NS_OK;
    }
  }

  nsCOMPtr<nsIDOMElement> focusedElement;
  nsCOMPtr<nsIDOMWindow> focusedWindow;
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm) {
    fm->GetFocusedElement(getter_AddRefs(focusedElement));
    fm->GetFocusedWindow(getter_AddRefs(focusedWindow));
  }

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
