











































#include "nsScriptEventManager.h"

#include "nsString.h"
#include "nsReadableUtils.h"

#include "nsIDOMNode.h"
#include "nsIDOMHTMLScriptElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMDocument.h"

#include "nsIScriptElement.h"

#include "nsIScriptEventHandler.h"
#include "nsIDocument.h"

nsScriptEventManager::nsScriptEventManager(nsIDOMDocument *aDocument)
{
  nsresult rv;


  rv = aDocument->GetElementsByTagName(NS_LITERAL_STRING("script"),
                                       getter_AddRefs(mScriptElements));

}


nsScriptEventManager::~nsScriptEventManager()
{
}


NS_IMPL_ISUPPORTS1(nsScriptEventManager, nsIScriptEventManager)


NS_IMETHODIMP
nsScriptEventManager::FindEventHandler(const nsAString &aObjectName,
                                       const nsAString &aEventName,
                                       PRUint32 aArgCount,
                                       nsISupports **aScriptHandler)
{
  nsresult rv;

  if (!mScriptElements) {
    return NS_ERROR_FAILURE;
  }

  
  if (!aScriptHandler) {
    return NS_ERROR_NULL_POINTER;
  }
  *aScriptHandler = nsnull;

  
  PRUint32 count = 0;
  rv = mScriptElements->GetLength(&count);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  
  
  
  
  nsCOMPtr<nsIDOMNode> node;
  nsCOMPtr<nsIScriptEventHandler> handler;

  while (count--) {
    rv = mScriptElements->Item(count, getter_AddRefs(node));
    if (NS_FAILED(rv)) break;

    
    handler = do_QueryInterface(node, &rv);
    if (NS_FAILED(rv)) continue;

    PRBool bFound = PR_FALSE;
    rv = handler->IsSameEvent(aObjectName, aEventName, aArgCount, &bFound);

    if (NS_SUCCEEDED(rv) && bFound) {
        *aScriptHandler = handler;
        NS_ADDREF(*aScriptHandler);

        return NS_OK;
    }
  }

  
  return rv;
}

NS_IMETHODIMP
nsScriptEventManager::InvokeEventHandler(nsISupports *aHandler,
                                         nsISupports *aTargetObject,
                                         void * aArgs,
                                         PRUint32 aArgCount)
{
  nsCOMPtr<nsIScriptEventHandler> handler(do_QueryInterface(aHandler));

  if (!handler) {
    return NS_ERROR_FAILURE;
  }

  return handler->Invoke(aTargetObject, aArgs, aArgCount);
}
