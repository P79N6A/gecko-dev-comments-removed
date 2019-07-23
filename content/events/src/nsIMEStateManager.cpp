






































#include "nsIMEStateManager.h"
#include "nsCOMPtr.h"
#include "nsIWidget.h"
#include "nsIViewManager.h"
#include "nsIViewObserver.h"
#include "nsIPresShell.h"
#include "nsISupports.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIEditorDocShell.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsIKBStateControl.h"
#include "nsIFocusController.h"
#include "nsIDOMWindow.h"
#include "nsContentUtils.h"





nsIContent*    nsIMEStateManager::sContent      = nsnull;
nsPresContext* nsIMEStateManager::sPresContext  = nsnull;
nsPIDOMWindow* nsIMEStateManager::sActiveWindow = nsnull;
PRBool         nsIMEStateManager::sInstalledMenuKeyboardListener = PR_FALSE;

nsresult
nsIMEStateManager::OnDestroyPresContext(nsPresContext* aPresContext)
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  if (aPresContext != sPresContext)
    return NS_OK;
  sContent = nsnull;
  sPresContext = nsnull;
  return NS_OK;
}

nsresult
nsIMEStateManager::OnRemoveContent(nsPresContext* aPresContext,
                                   nsIContent* aContent)
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  if (!sPresContext || !sContent ||
      aPresContext != sPresContext ||
      aContent != sContent)
    return NS_OK;

  
  nsCOMPtr<nsIKBStateControl> kb = GetKBStateControl(sPresContext);
  if (kb) {
    nsresult rv = kb->CancelIMEComposition();
    if (NS_FAILED(rv))
      kb->ResetInputState();
  }

  sContent = nsnull;
  sPresContext = nsnull;

  return NS_OK;
}

nsresult
nsIMEStateManager::OnChangeFocus(nsPresContext* aPresContext,
                                 nsIContent* aContent)
{
  NS_ENSURE_ARG_POINTER(aPresContext);

  if (!IsActive(aPresContext)) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIKBStateControl> kb = GetKBStateControl(aPresContext);
  if (!kb) {
    
    return NS_OK;
  }

  PRUint32 newState = GetNewIMEState(aPresContext, aContent);
  if (aPresContext == sPresContext && aContent == sContent) {
    
    
    PRUint32 newEnabledState = newState & nsIContent::IME_STATUS_MASK_ENABLED;
    if (newEnabledState == 0) {
      
      return NS_OK;
    }
    PRUint32 enabled;
    if (NS_FAILED(kb->GetIMEEnabled(&enabled))) {
      
      return NS_OK;
    }
    if (enabled ==
        nsContentUtils::GetKBStateControlStatusFromIMEStatus(newEnabledState)) {
      
      return NS_OK;
    }
  }

  
  if (sPresContext) {
    nsCOMPtr<nsIKBStateControl> oldKB;
    if (sPresContext == aPresContext)
      oldKB = kb;
    else
      oldKB = GetKBStateControl(sPresContext);
    if (oldKB)
      oldKB->ResetInputState();
  }

  if (newState != nsIContent::IME_STATUS_NONE) {
    
    SetIMEState(aPresContext, newState, kb);
  }

  sPresContext = aPresContext;
  sContent = aContent;

  return NS_OK;
}

nsresult
nsIMEStateManager::OnActivate(nsPresContext* aPresContext)
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  sActiveWindow = aPresContext->Document()->GetWindow();
  NS_ENSURE_TRUE(sActiveWindow, NS_ERROR_FAILURE);
  sActiveWindow = sActiveWindow->GetPrivateRoot();
  return NS_OK;
}

nsresult
nsIMEStateManager::OnDeactivate(nsPresContext* aPresContext)
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  NS_ENSURE_TRUE(aPresContext->Document()->GetWindow(), NS_ERROR_FAILURE);
  if (sActiveWindow !=
      aPresContext->Document()->GetWindow()->GetPrivateRoot())
    return NS_OK;

  sActiveWindow = nsnull;
#ifdef NS_KBSC_USE_SHARED_CONTEXT
  
  
  sContent = nsnull;
  
  nsCOMPtr<nsIKBStateControl> kb = GetKBStateControl(aPresContext);
  if (kb)
    SetIMEState(aPresContext, nsIContent::IME_STATUS_ENABLE, kb);
#endif 
  return NS_OK;
}

void
nsIMEStateManager::OnInstalledMenuKeyboardListener(PRBool aInstalling)
{
  sInstalledMenuKeyboardListener = aInstalling;
  OnChangeFocus(sPresContext, sContent);
}

PRBool
nsIMEStateManager::IsActive(nsPresContext* aPresContext)
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  nsPIDOMWindow* window = aPresContext->Document()->GetWindow();
  NS_ENSURE_TRUE(window, PR_FALSE);
  if (!sActiveWindow || sActiveWindow != window->GetPrivateRoot()) {
    
    return PR_FALSE;
  }
  NS_ENSURE_TRUE(aPresContext->GetPresShell(), PR_FALSE);
  nsIViewManager* vm = aPresContext->GetViewManager();
  NS_ENSURE_TRUE(vm, PR_FALSE);
  nsCOMPtr<nsIViewObserver> observer;
  vm->GetViewObserver(*getter_AddRefs(observer));
  NS_ENSURE_TRUE(observer, PR_FALSE);
  return observer->IsVisible();
}

nsIFocusController*
nsIMEStateManager::GetFocusController(nsPresContext* aPresContext)
{
  nsCOMPtr<nsISupports> container =
    aPresContext->Document()->GetContainer();
  nsCOMPtr<nsPIDOMWindow> windowPrivate = do_GetInterface(container);

  return windowPrivate ? windowPrivate->GetRootFocusController() : nsnull;
}

PRUint32
nsIMEStateManager::GetNewIMEState(nsPresContext* aPresContext,
                                  nsIContent*    aContent)
{
  
  if (aPresContext->Type() == nsPresContext::eContext_PrintPreview ||
      aPresContext->Type() == nsPresContext::eContext_Print) {
    return nsIContent::IME_STATUS_DISABLE;
  }

  if (sInstalledMenuKeyboardListener)
    return nsIContent::IME_STATUS_DISABLE;

  PRBool isEditable = PR_FALSE;
  nsCOMPtr<nsISupports> container = aPresContext->GetContainer();
  nsCOMPtr<nsIEditorDocShell> editorDocShell(do_QueryInterface(container));
  if (editorDocShell)
    editorDocShell->GetEditable(&isEditable);

  if (isEditable)
    return nsIContent::IME_STATUS_ENABLE;

  if (aContent)
    return aContent->GetDesiredIMEState();

  return nsIContent::IME_STATUS_DISABLE;
}

void
nsIMEStateManager::SetIMEState(nsPresContext*     aPresContext,
                               PRUint32           aState,
                               nsIKBStateControl* aKB)
{
  if (aState & nsIContent::IME_STATUS_MASK_ENABLED) {
    PRUint32 state =
      nsContentUtils::GetKBStateControlStatusFromIMEStatus(aState);
    aKB->SetIMEEnabled(state);
  }
  if (aState & nsIContent::IME_STATUS_MASK_OPENED) {
    PRBool open = (aState & nsIContent::IME_STATUS_OPEN);
    aKB->SetIMEOpenState(open);
  }
}

nsIKBStateControl*
nsIMEStateManager::GetKBStateControl(nsPresContext* aPresContext)
{
  nsIViewManager* vm = aPresContext->GetViewManager();
  if (!vm)
    return nsnull;
  nsCOMPtr<nsIWidget> widget = nsnull;
  nsresult rv = vm->GetWidget(getter_AddRefs(widget));
  NS_ENSURE_SUCCESS(rv, nsnull);
  NS_ENSURE_TRUE(widget, nsnull);
  nsCOMPtr<nsIKBStateControl> kb = do_QueryInterface(widget);
  return kb;
}

