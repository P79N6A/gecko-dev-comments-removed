






































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

  
  nsCOMPtr<nsIWidget> widget = GetWidget(sPresContext);
  if (widget) {
    nsresult rv = widget->CancelIMEComposition();
    if (NS_FAILED(rv))
      widget->ResetInputState();
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

  nsCOMPtr<nsIWidget> widget = GetWidget(aPresContext);
  if (!widget) {
    return NS_OK;
  }

  PRUint32 newState = GetNewIMEState(aPresContext, aContent);
  if (aPresContext == sPresContext && aContent == sContent) {
    
    
    PRUint32 newEnabledState = newState & nsIContent::IME_STATUS_MASK_ENABLED;
    if (newEnabledState == 0) {
      
      return NS_OK;
    }
    PRUint32 enabled;
    if (NS_FAILED(widget->GetIMEEnabled(&enabled))) {
      
      return NS_OK;
    }
    if (enabled ==
        nsContentUtils::GetWidgetStatusFromIMEStatus(newEnabledState)) {
      
      return NS_OK;
    }
  }

  
  if (sPresContext) {
    nsCOMPtr<nsIWidget> oldWidget;
    if (sPresContext == aPresContext)
      oldWidget = widget;
    else
      oldWidget = GetWidget(sPresContext);
    if (oldWidget)
      oldWidget->ResetInputState();
  }

  if (newState != nsIContent::IME_STATUS_NONE) {
    
    SetIMEState(aPresContext, newState, widget);
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
  NS_ENSURE_TRUE(aPresContext, PR_FALSE);
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

  if (!aContent) {
    
    
    nsIDocument* doc = aPresContext->Document();
    if (doc && doc->HasFlag(NODE_IS_EDITABLE))
      return nsIContent::IME_STATUS_ENABLE;
    return nsIContent::IME_STATUS_DISABLE;
  }

  return aContent->GetDesiredIMEState();
}

void
nsIMEStateManager::SetIMEState(nsPresContext*     aPresContext,
                               PRUint32           aState,
                               nsIWidget*         aKB)
{
  if (aState & nsIContent::IME_STATUS_MASK_ENABLED) {
    PRUint32 state =
      nsContentUtils::GetWidgetStatusFromIMEStatus(aState);
    aKB->SetIMEEnabled(state);
  }
  if (aState & nsIContent::IME_STATUS_MASK_OPENED) {
    PRBool open = !!(aState & nsIContent::IME_STATUS_OPEN);
    aKB->SetIMEOpenState(open);
  }
}

nsIWidget*
nsIMEStateManager::GetWidget(nsPresContext* aPresContext)
{
  nsIViewManager* vm = aPresContext->GetViewManager();
  if (!vm)
    return nsnull;
  nsCOMPtr<nsIWidget> widget = nsnull;
  nsresult rv = vm->GetWidget(getter_AddRefs(widget));
  NS_ENSURE_SUCCESS(rv, nsnull);
  return widget;
}

