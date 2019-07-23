






































#include "nsCommandHandler.h"
#include "nsWebBrowser.h"
#include "nsDocShellTreeOwner.h"

#include "nsIAllocator.h"
#include "nsPIDOMWindow.h"

nsCommandHandler::nsCommandHandler() :
    mWindow(nsnull)
{
}

nsCommandHandler::~nsCommandHandler()
{
}

nsresult nsCommandHandler::GetCommandHandler(nsICommandHandler **aCommandHandler)
{
    NS_ENSURE_ARG_POINTER(aCommandHandler);

    *aCommandHandler = nsnull;
    if (mWindow == nsnull)
    {
        return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mWindow));
    if (!window)
    {
        return NS_ERROR_FAILURE;
    }

    

    nsCOMPtr<nsIDocShellTreeItem> docShellAsTreeItem =
      do_QueryInterface(window->GetDocShell());
    nsIDocShellTreeOwner *treeOwner = nsnull;
    docShellAsTreeItem->GetTreeOwner(&treeOwner);

    
    
    

    nsCOMPtr<nsICDocShellTreeOwner> realTreeOwner(do_QueryInterface(treeOwner));
    if (realTreeOwner)
    {
        nsDocShellTreeOwner *tree = static_cast<nsDocShellTreeOwner *>(treeOwner);
        if (tree->mTreeOwner)
        {
            nsresult rv;
            rv = tree->mTreeOwner->QueryInterface(NS_GET_IID(nsICommandHandler), (void **)aCommandHandler);
            NS_RELEASE(treeOwner);
            return rv;
        }
     
        NS_RELEASE(treeOwner);
     }

    *aCommandHandler = nsnull;

    return NS_OK;
}


NS_IMPL_ADDREF(nsCommandHandler)
NS_IMPL_RELEASE(nsCommandHandler)

NS_INTERFACE_MAP_BEGIN(nsCommandHandler)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICommandHandler)
    NS_INTERFACE_MAP_ENTRY(nsICommandHandlerInit)
    NS_INTERFACE_MAP_ENTRY(nsICommandHandler)
NS_INTERFACE_MAP_END





NS_IMETHODIMP nsCommandHandler::GetWindow(nsIDOMWindow * *aWindow)
{
    *aWindow = nsnull;
    return NS_OK;
}

NS_IMETHODIMP nsCommandHandler::SetWindow(nsIDOMWindow * aWindow)
{
    if (aWindow == nsnull)
    {
       return NS_ERROR_FAILURE;
    }
    mWindow = aWindow;
    return NS_OK;
}





NS_IMETHODIMP nsCommandHandler::Exec(const char *aCommand, const char *aStatus, char **aResult)
{
    NS_ENSURE_ARG_POINTER(aCommand);
    NS_ENSURE_ARG_POINTER(aResult);

    nsCOMPtr<nsICommandHandler> commandHandler;
    GetCommandHandler(getter_AddRefs(commandHandler));

    
    if (commandHandler)
    {
        *aResult = nsnull;
        return commandHandler->Exec(aCommand, aStatus, aResult);
    }

    
    const char szEmpty[] = "";
    *aResult = (char *) nsAllocator::Clone(szEmpty, sizeof(szEmpty));

    return NS_OK;
}


NS_IMETHODIMP nsCommandHandler::Query(const char *aCommand, const char *aStatus, char **aResult)
{
    NS_ENSURE_ARG_POINTER(aCommand);
    NS_ENSURE_ARG_POINTER(aResult);

    nsCOMPtr<nsICommandHandler> commandHandler;
    GetCommandHandler(getter_AddRefs(commandHandler));

    
    if (commandHandler)
    {
        *aResult = nsnull;
        return commandHandler->Query(aCommand, aStatus, aResult);
    }

    
    const char szEmpty[] = "";
    *aResult = (char *) nsAllocator::Clone(szEmpty, sizeof(szEmpty));

    return NS_OK;
}
