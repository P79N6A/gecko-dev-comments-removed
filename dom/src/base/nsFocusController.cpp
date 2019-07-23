







































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
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIScriptGlobalObject.h"
#include "nsPIDOMWindow.h"
#include "nsFocusController.h"
#include "prlog.h"
#include "nsIDOMEventTarget.h"
#include "nsIEventStateManager.h"
#include "nsIDocShell.h"
#include "nsIBaseWindow.h"
#include "nsIWindowWatcher.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsGlobalWindow.h"

#ifdef MOZ_XUL
#include "nsIDOMXULDocument.h"
#include "nsIDOMXULElement.h"
#endif



nsFocusController::nsFocusController(void)
: mSuppressFocus(0), 
  mSuppressFocusScroll(PR_FALSE), 
  mActive(PR_FALSE),
  mUpdateWindowWatcher(PR_FALSE),
  mNeedUpdateCommands(PR_FALSE)
{
}

nsFocusController::~nsFocusController(void)
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsFocusController)

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsFocusController, nsIFocusController)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsFocusController,
                                           nsIFocusController)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsFocusController)
  NS_INTERFACE_MAP_ENTRY(nsIFocusController)
  NS_INTERFACE_MAP_ENTRY(nsIDOMFocusListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsSupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIFocusController)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_0(nsFocusController)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsFocusController)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCurrentElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCurrentWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPopupNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPopupEvent)
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
nsFocusController::GetFocusedElement(nsIDOMElement** aElement)
{
  *aElement = mCurrentElement;
  NS_IF_ADDREF(*aElement);
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::GetFocusedWindow(nsIDOMWindowInternal** aWindow)
{
  *aWindow = mCurrentWindow;
  NS_IF_ADDREF(*aWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::SetFocusedElement(nsIDOMElement* aElement)
{
  if (mCurrentElement) 
    mPreviousElement = mCurrentElement;
  else if (aElement) 
    mPreviousElement = aElement;

  mNeedUpdateCommands = mNeedUpdateCommands || mCurrentElement != aElement;
  mCurrentElement = aElement;

  if (!mSuppressFocus) {
    
    
    
    UpdateCommands();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::RewindFocusState()
{
  mCurrentElement = mPreviousElement;
  mCurrentWindow = mPreviousWindow;

  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::SetFocusedWindow(nsIDOMWindowInternal* aWindow)
{
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aWindow);

  if (win) {
    win = win->GetOuterWindow();
  }

  NS_ASSERTION(!win || !win->IsInnerWindow(),
               "Uh, inner window can't have focus!");

  if (win && (mCurrentWindow != win)) {
    nsCOMPtr<nsIBaseWindow> basewin = do_QueryInterface(win->GetDocShell());
    if (basewin)
      basewin->SetFocus();
  }

  if (mCurrentWindow) {
    mPreviousWindow = mCurrentWindow;
  } else if (win) {
    mPreviousWindow = win;
  }

  mNeedUpdateCommands = mNeedUpdateCommands || mCurrentWindow != win;
  mCurrentWindow = win;

  if (mUpdateWindowWatcher) {
    NS_ASSERTION(mActive, "This shouldn't happen");
    if (mCurrentWindow)
      UpdateWWActiveWindow();
    mUpdateWindowWatcher = PR_FALSE;
  }

  return NS_OK;
}


void
nsFocusController::UpdateCommands()
{
  if (!mNeedUpdateCommands) {
    return;
  }
  nsCOMPtr<nsIDOMWindowInternal> window;
  nsCOMPtr<nsIDocument> doc;
  if (mCurrentWindow) {
    window = mCurrentWindow;
    nsCOMPtr<nsIDOMWindow> domWin(do_QueryInterface(window));
    nsCOMPtr<nsIDOMDocument> domDoc;
    domWin->GetDocument(getter_AddRefs(domDoc));
    doc = do_QueryInterface(domDoc);
  }
  else if (mCurrentElement) {
    nsCOMPtr<nsIDOMDocument> domDoc;
    mCurrentElement->GetOwnerDocument(getter_AddRefs(domDoc));
    if (domDoc) {
      doc = do_QueryInterface(domDoc);
      window = do_QueryInterface(doc->GetScriptGlobalObject());
    }
  }

  
  if (window && doc && doc->GetPrimaryShell()) {
    
    window->UpdateCommands(NS_LITERAL_STRING("focus"));
    mNeedUpdateCommands = PR_FALSE;
  }
}

  
NS_IMETHODIMP
nsFocusController::GetControllers(nsIControllers** aResult)
{
  
  
  
  if (mCurrentElement) {

#ifdef MOZ_XUL
    nsCOMPtr<nsIDOMXULElement> xulElement(do_QueryInterface(mCurrentElement));
    if (xulElement)
      return xulElement->GetControllers(aResult);
#endif

    nsCOMPtr<nsIDOMNSHTMLTextAreaElement> htmlTextArea =
      do_QueryInterface(mCurrentElement);
    if (htmlTextArea)
      return htmlTextArea->GetControllers(aResult);

    nsCOMPtr<nsIDOMNSHTMLInputElement> htmlInputElement =
      do_QueryInterface(mCurrentElement);
    if (htmlInputElement)
      return htmlInputElement->GetControllers(aResult);

    nsCOMPtr<nsIContent> content = do_QueryInterface(mCurrentElement);
    if (content && content->IsEditable()) {
      
      nsCOMPtr<nsIDOMDocument> domDoc;
      mCurrentElement->GetOwnerDocument(getter_AddRefs(domDoc));
      nsCOMPtr<nsIDOMWindowInternal> domWindow =
        do_QueryInterface(GetWindowFromDocument(domDoc));
      if (domWindow)
        return domWindow->GetControllers(aResult);
    }
  }
  else if (mCurrentWindow) {
    nsCOMPtr<nsIDOMWindowInternal> domWindow =
      do_QueryInterface(mCurrentWindow);
    if (domWindow)
      return domWindow->GetControllers(aResult);
  }

  *aResult = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::MoveFocus(PRBool aForward, nsIDOMElement* aElt)
{
  
  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIContent> content;
  if (aElt) {
    content = do_QueryInterface(aElt);
    doc = content->GetDocument();
  }
  else {
    if (mCurrentElement) {
      content = do_QueryInterface(mCurrentElement);
      doc = content->GetDocument();
      content = nsnull;
    }
    else if (mCurrentWindow) {
      nsCOMPtr<nsIDOMDocument> domDoc;
      mCurrentWindow->GetDocument(getter_AddRefs(domDoc));
      doc = do_QueryInterface(domDoc);
    }
  }

  if (!doc) {
    
    return NS_ERROR_FAILURE;
  }

  nsIPresShell *shell = doc->GetPrimaryShell();
  if (!shell)
    return NS_ERROR_FAILURE;

  
  shell->FlushPendingNotifications(Flush_Frames);

  
  nsCOMPtr<nsPresContext> presContext = shell->GetPresContext();

  
  return presContext->EventStateManager()->ShiftFocus(aForward, content);
}





nsresult 
nsFocusController::Focus(nsIDOMEvent* aEvent)
{
  if (mSuppressFocus)
    return NS_OK;

  nsCOMPtr<nsIDOMEventTarget> t;

  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aEvent));
  if (nsevent) {
    nsevent->GetOriginalTarget(getter_AddRefs(t));
  }

  nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(t);
  if (domElement && (domElement != mCurrentElement)) {
    SetFocusedElement(domElement);

    
    
    
    nsCOMPtr<nsIDOMDocument> ownerDoc;
    domElement->GetOwnerDocument(getter_AddRefs(ownerDoc));
    nsCOMPtr<nsIDOMWindowInternal> domWindow = GetWindowFromDocument(ownerDoc);
    if (domWindow)
      SetFocusedWindow(domWindow);
  }
  else {
    
    
    nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(t);
    if (domDoc) {
      nsCOMPtr<nsIDOMWindowInternal> domWindow = GetWindowFromDocument(domDoc);
      if (domWindow) {
        SetFocusedWindow(domWindow);
        if (mCurrentElement) {
          
          
          nsCOMPtr<nsIDOMDocument> ownerDoc;
          mCurrentElement->GetOwnerDocument(getter_AddRefs(ownerDoc));
          nsCOMPtr<nsIDOMDocument> windowDoc;
          mCurrentWindow->GetDocument(getter_AddRefs(windowDoc));
          if (ownerDoc != windowDoc)
            mCurrentElement = mPreviousElement = nsnull;
        }
        else
          mPreviousElement = nsnull;

        if (!mCurrentElement) {
          UpdateCommands();
        }
      }
    }
  }

  return NS_OK;
}

nsresult 
nsFocusController::Blur(nsIDOMEvent* aEvent)
{
  if (mSuppressFocus)
    return NS_OK;

  nsCOMPtr<nsIDOMEventTarget> t;

  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aEvent));

  if (nsevent) {
    nsevent->GetOriginalTarget(getter_AddRefs(t));
  }

  nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(t);
  if (domElement) {
    SetFocusedElement(nsnull);
  }
  
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(t);
  if (domDoc) {
    nsCOMPtr<nsIDOMWindowInternal> domWindow = GetWindowFromDocument(domDoc);
    if (domWindow)
      SetFocusedWindow(nsnull);
  }

  return NS_OK;
}

nsPIDOMWindow *
nsFocusController::GetWindowFromDocument(nsIDOMDocument* aDocument)
{
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aDocument);
  if (!doc)
    return NS_OK;

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
  
  nsCOMPtr<nsPIDOMWindow> currentWindow;
  if (mCurrentElement) {
    
    nsCOMPtr<nsIDOMDocument> domDoc;
    mCurrentElement->GetOwnerDocument(getter_AddRefs(domDoc));
    currentWindow = do_QueryInterface(GetWindowFromDocument(domDoc));
  }
  else if (mCurrentWindow) {
    nsGlobalWindow *win =
      static_cast<nsGlobalWindow *>
                 (static_cast<nsIDOMWindowInternal *>(mCurrentWindow));
    currentWindow = win->GetPrivateParent();
  }
  else return NS_OK;

  while(currentWindow) {
    nsCOMPtr<nsIDOMWindowInternal> domWindow(do_QueryInterface(currentWindow));

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

    nsGlobalWindow *win =
      static_cast<nsGlobalWindow *>
                 (static_cast<nsIDOMWindowInternal *>(currentWindow));
    currentWindow = win->GetPrivateParent();
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::GetSuppressFocusScroll(PRBool* aSuppressFocusScroll)
{
  *aSuppressFocusScroll = mSuppressFocusScroll;
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::SetSuppressFocusScroll(PRBool aSuppressFocusScroll)
{
  mSuppressFocusScroll = aSuppressFocusScroll;
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::GetSuppressFocus(PRBool* aSuppressFocus)
{
  *aSuppressFocus = (mSuppressFocus > 0);
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::SetSuppressFocus(PRBool aSuppressFocus, const char* aReason)
{
  if(aSuppressFocus) {
    ++mSuppressFocus;
    
    
    
  }
  else if(mSuppressFocus > 0) {
    --mSuppressFocus;
    
    
    
  }
  else 
    NS_ASSERTION(PR_FALSE, "Attempt to decrement focus controller's suppression when no suppression active!\n");

  
  
  
  
  if (!mSuppressFocus) {
    
    
    UpdateCommands();
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::GetActive(PRBool* aActive)
{
  *aActive = mActive;
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::SetActive(PRBool aActive)
{
  mActive = aActive;

  
  
  
  
  
  
  if (mActive) {
    if (mCurrentWindow)
      UpdateWWActiveWindow();
    else
      mUpdateWindowWatcher = PR_TRUE;
  } else
    mUpdateWindowWatcher = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::ResetElementFocus()
{
  mCurrentElement = mPreviousElement = nsnull;
  return NS_OK;
}

void
nsFocusController::UpdateWWActiveWindow()
{
  
  nsCOMPtr<nsIWindowWatcher> wwatch = do_GetService("@mozilla.org/embedcomp/window-watcher;1");
  if (!wwatch) return;

  
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem =
    do_QueryInterface(mCurrentWindow->GetDocShell());
  if (!docShellAsItem) return;

  nsCOMPtr<nsIDocShellTreeItem> rootItem;
  docShellAsItem->GetRootTreeItem(getter_AddRefs(rootItem));
  NS_ASSERTION(rootItem, "Invalid docshell tree - no root!");

  nsCOMPtr<nsIDOMWindow> domWin = do_GetInterface(rootItem);
  wwatch->SetActiveWindow(domWin);
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

  mPopupNode = aNode;
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::GetPopupEvent(nsIDOMEvent** aEvent)
{
  *aEvent = mPopupEvent;
  NS_IF_ADDREF(*aEvent);
  return NS_OK;
}

NS_IMETHODIMP
nsFocusController::SetPopupEvent(nsIDOMEvent* aEvent)
{
  mPopupEvent = aEvent;
  return NS_OK;
}

  
