





































#include "nsString.h"

#include "nsIController.h"
#include "nsIControllers.h"
#include "nsIObserver.h"

#include "nsIComponentManager.h"

#include "nsServiceManagerUtils.h"
#include "nsIScriptSecurityManager.h"

#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMWindowInternal.h"
#include "nsIFocusController.h"
#include "nsIFocusManager.h"

#include "nsCOMArray.h"

#include "nsCommandManager.h"


nsCommandManager::nsCommandManager()
: mWindow(nsnull)
{
  
}

nsCommandManager::~nsCommandManager()
{
  
}


static PLDHashOperator
TraverseCommandObservers(const char* aKey, nsCOMArray<nsIObserver>* aObservers,
                         void* aClosure)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);

  PRInt32 i, numItems = aObservers->Count();
  for (i = 0; i < numItems; ++i) {
    cb->NoteXPCOMChild(aObservers->ObjectAt(i));
  }

  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsCommandManager)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsCommandManager)
  tmp->mObserversTable.Clear();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsCommandManager)
  tmp->mObserversTable.EnumerateRead(TraverseCommandObservers, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsCommandManager, nsICommandManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsCommandManager, nsICommandManager)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsCommandManager)
   NS_INTERFACE_MAP_ENTRY(nsICommandManager)
   NS_INTERFACE_MAP_ENTRY(nsPICommandUpdater)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICommandManager)
NS_INTERFACE_MAP_END

#if 0
#pragma mark -
#endif


NS_IMETHODIMP
nsCommandManager::Init(nsIDOMWindow *aWindow)
{
  NS_ENSURE_ARG_POINTER(aWindow);
  
  NS_ASSERTION(aWindow, "Need non-null window here");
  mWindow = aWindow;      
  NS_ENSURE_TRUE(mObserversTable.Init(), NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}


NS_IMETHODIMP
nsCommandManager::CommandStatusChanged(const char * aCommandName)
{
  nsCOMArray<nsIObserver>* commandObservers;
  mObserversTable.Get(aCommandName, &commandObservers);

  if (commandObservers)
  {
    
    PRInt32 i, numItems = commandObservers->Count();
    for (i = 0; i < numItems;  ++i)
    {
      nsCOMPtr<nsIObserver> observer = commandObservers->ObjectAt(i);
      
      observer->Observe(NS_ISUPPORTS_CAST(nsICommandManager*, this),
                        aCommandName,
                        NS_LITERAL_STRING("command_status_changed").get());
    }
  }

  return NS_OK;
}

#if 0
#pragma mark -
#endif


NS_IMETHODIMP
nsCommandManager::AddCommandObserver(nsIObserver *aCommandObserver, const char *aCommandToObserve)
{
  NS_ENSURE_ARG(aCommandObserver);

  nsresult rv = NS_OK;

  

  
  nsCOMArray<nsIObserver>* commandObservers;
  if (!mObserversTable.Get(aCommandToObserve, &commandObservers))
  {
    nsAutoPtr<nsCOMArray<nsIObserver> > array(new nsCOMArray<nsIObserver>);
    if (!array || !mObserversTable.Put(aCommandToObserve, array))
      return NS_ERROR_OUT_OF_MEMORY;

    commandObservers = array.forget();
  }

  
  PRInt32 existingIndex = commandObservers->IndexOf(aCommandObserver);
  if (existingIndex == -1)
    rv = commandObservers->AppendObject(aCommandObserver);
  else
    NS_WARNING("Registering command observer twice on the same command");
  
  return rv;
}


NS_IMETHODIMP
nsCommandManager::RemoveCommandObserver(nsIObserver *aCommandObserver, const char *aCommandObserved)
{
  NS_ENSURE_ARG(aCommandObserver);

  

  nsCOMArray<nsIObserver>* commandObservers;
  if (!mObserversTable.Get(aCommandObserved, &commandObservers))
    return NS_ERROR_UNEXPECTED;

  return commandObservers->RemoveObject(aCommandObserver) ? NS_OK :
                                                            NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsCommandManager::IsCommandSupported(const char *aCommandName,
                                     nsIDOMWindow *aTargetWindow,
                                     PRBool *outCommandSupported)
{
  NS_ENSURE_ARG_POINTER(outCommandSupported);

  nsCOMPtr<nsIController> controller;
  GetControllerForCommand(aCommandName, aTargetWindow, getter_AddRefs(controller)); 
  *outCommandSupported = (controller.get() != nsnull);
  return NS_OK;
}



NS_IMETHODIMP
nsCommandManager::IsCommandEnabled(const char *aCommandName,
                                   nsIDOMWindow *aTargetWindow,
                                   PRBool *outCommandEnabled)
{
  NS_ENSURE_ARG_POINTER(outCommandEnabled);
  
  PRBool  commandEnabled = PR_FALSE;
  
  nsCOMPtr<nsIController> controller;
  GetControllerForCommand(aCommandName, aTargetWindow, getter_AddRefs(controller)); 
  if (controller)
  {
    controller->IsCommandEnabled(aCommandName, &commandEnabled);
  }
  *outCommandEnabled = commandEnabled;
  return NS_OK;
}




NS_IMETHODIMP
nsCommandManager::GetCommandState(const char *aCommandName,
                                  nsIDOMWindow *aTargetWindow,
                                  nsICommandParams *aCommandParams)
{
  nsCOMPtr<nsIController> controller;
  nsAutoString tValue;
  nsresult rv = GetControllerForCommand(aCommandName, aTargetWindow, getter_AddRefs(controller)); 
  if (!controller)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsICommandController>  commandController = do_QueryInterface(controller);
  if (commandController)
    rv = commandController->GetCommandStateWithParams(aCommandName, aCommandParams);
  else
    rv = NS_ERROR_NOT_IMPLEMENTED;
  return rv;
}




NS_IMETHODIMP
nsCommandManager::DoCommand(const char *aCommandName,
                            nsICommandParams *aCommandParams,
                            nsIDOMWindow *aTargetWindow)
{
  nsCOMPtr<nsIController> controller;
  nsresult rv = GetControllerForCommand(aCommandName, aTargetWindow, getter_AddRefs(controller)); 
  if (!controller)
	  return NS_ERROR_FAILURE;

  nsCOMPtr<nsICommandController>  commandController = do_QueryInterface(controller);
  if (commandController && aCommandParams)
    rv = commandController->DoCommandWithParams(aCommandName, aCommandParams);
  else
    rv = controller->DoCommand(aCommandName);
  return rv;
}

#ifdef XP_MAC
#pragma mark -
#endif

nsresult
nsCommandManager::IsCallerChrome(PRBool *is_caller_chrome)
{
  *is_caller_chrome = PR_FALSE;
  nsresult rv = NS_OK;
  nsCOMPtr<nsIScriptSecurityManager> secMan = 
      do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;
  if (!secMan)
    return NS_ERROR_FAILURE;

  rv = secMan->SubjectPrincipalIsSystem(is_caller_chrome);
  return rv;
}

nsresult
nsCommandManager::GetControllerForCommand(const char *aCommand, 
                                          nsIDOMWindow *aTargetWindow,
                                          nsIController** outController)
{
  nsresult rv = NS_ERROR_FAILURE;
  *outController = nsnull;

  
  
  PRBool isChrome = PR_FALSE;
  rv = IsCallerChrome(&isChrome);
  if (NS_FAILED(rv))
    return rv;

  if (!isChrome) {
    if (!aTargetWindow)
      return rv;

    
    if (aTargetWindow != mWindow)
        return NS_ERROR_FAILURE;
  }

  if (aTargetWindow)
  {
    
    nsCOMPtr<nsIDOMWindowInternal> domWindowInternal = do_QueryInterface(aTargetWindow);
    if (!domWindowInternal)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIControllers> controllers;
    rv = domWindowInternal->GetControllers(getter_AddRefs(controllers));
    if (NS_FAILED(rv))
      return rv;
    if (!controllers)
      return NS_ERROR_FAILURE;

    
    return controllers->GetControllerForCommand(aCommand, outController);
  }


  
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(mWindow);
  if (!window)
    return NS_ERROR_FAILURE;

  nsIFocusController *focusController = window->GetRootFocusController();
  if (!focusController)
    return NS_ERROR_FAILURE;

  
  return focusController->GetControllerForCommand(window, aCommand, outController);
}

