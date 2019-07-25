





































#include "nsString.h"
#include "nsIControllerCommand.h"
#include "nsControllerCommandTable.h"


nsresult
NS_NewControllerCommandTable(nsIControllerCommandTable** aResult);



#define NUM_COMMANDS_BOUNDS       64


nsControllerCommandTable::nsControllerCommandTable()
: mCommandsTable(NUM_COMMANDS_BOUNDS, PR_FALSE)
, mMutable(PR_TRUE)
{
}


nsControllerCommandTable::~nsControllerCommandTable()
{
}

NS_IMPL_ISUPPORTS2(nsControllerCommandTable, nsIControllerCommandTable, nsISupportsWeakReference)

NS_IMETHODIMP
nsControllerCommandTable::MakeImmutable(void)
{
  mMutable = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsControllerCommandTable::RegisterCommand(const char * aCommandName, nsIControllerCommand *aCommand)
{
  NS_ENSURE_TRUE(mMutable, NS_ERROR_FAILURE);
  
  nsCStringKey commandKey(aCommandName);
  
  if (mCommandsTable.Put(&commandKey, aCommand))
  {
#if DEBUG
    NS_WARNING("Replacing existing command -- ");
#endif
  }  
  return NS_OK;
}


NS_IMETHODIMP
nsControllerCommandTable::UnregisterCommand(const char * aCommandName, nsIControllerCommand *aCommand)
{
  NS_ENSURE_TRUE(mMutable, NS_ERROR_FAILURE);

  nsCStringKey commandKey(aCommandName);

  PRBool wasRemoved = mCommandsTable.Remove(&commandKey);
  return wasRemoved ? NS_OK : NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsControllerCommandTable::FindCommandHandler(const char * aCommandName, nsIControllerCommand **outCommand)
{
  NS_ENSURE_ARG_POINTER(outCommand);
  
  *outCommand = NULL;
  
  nsCStringKey commandKey(aCommandName);
  nsISupports* foundCommand = mCommandsTable.Get(&commandKey);
  if (!foundCommand) return NS_ERROR_FAILURE;
  
  
  *outCommand = reinterpret_cast<nsIControllerCommand*>(foundCommand);
  return NS_OK;
}




NS_IMETHODIMP
nsControllerCommandTable::IsCommandEnabled(const char * aCommandName, nsISupports *aCommandRefCon, PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  *aResult = PR_FALSE;
      
  
  nsCOMPtr<nsIControllerCommand> commandHandler;
  FindCommandHandler(aCommandName, getter_AddRefs(commandHandler));  
  if (!commandHandler) 
  {
#if DEBUG
    NS_WARNING("Controller command table asked about a command that it does not handle -- ");
#endif
    return NS_OK;    
  }
  
  return commandHandler->IsCommandEnabled(aCommandName, aCommandRefCon, aResult);
}


NS_IMETHODIMP
nsControllerCommandTable::UpdateCommandState(const char * aCommandName, nsISupports *aCommandRefCon)
{
  
  nsCOMPtr<nsIControllerCommand> commandHandler;
  FindCommandHandler(aCommandName, getter_AddRefs(commandHandler));  
  if (!commandHandler)
  {
#if DEBUG
    NS_WARNING("Controller command table asked to update the state of a command that it does not handle -- ");
#endif
    return NS_OK;    
  }
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsControllerCommandTable::SupportsCommand(const char * aCommandName, nsISupports *aCommandRefCon, PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  

  *aResult = PR_FALSE;
  
  
  nsCOMPtr<nsIControllerCommand> commandHandler;
  FindCommandHandler(aCommandName, getter_AddRefs(commandHandler));

  *aResult = (commandHandler.get() != nsnull);
  return NS_OK;
}


NS_IMETHODIMP
nsControllerCommandTable::DoCommand(const char * aCommandName, nsISupports *aCommandRefCon)
{
  
  nsCOMPtr<nsIControllerCommand> commandHandler;
  FindCommandHandler(aCommandName, getter_AddRefs(commandHandler));
  if (!commandHandler)
  {
#if DEBUG
    NS_WARNING("Controller command table asked to do a command that it does not handle -- ");
#endif
    return NS_OK;    
  }
  
  return commandHandler->DoCommand(aCommandName, aCommandRefCon);
}

NS_IMETHODIMP
nsControllerCommandTable::DoCommandParams(const char *aCommandName, nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  
  nsCOMPtr<nsIControllerCommand> commandHandler;
  nsresult rv;
  rv = FindCommandHandler(aCommandName, getter_AddRefs(commandHandler));
  if (!commandHandler)
  {
#if DEBUG
    NS_WARNING("Controller command table asked to do a command that it does not handle -- ");
#endif
    return NS_OK;    
  }
  return commandHandler->DoCommandParams(aCommandName, aParams, aCommandRefCon);
}


NS_IMETHODIMP
nsControllerCommandTable::GetCommandState(const char *aCommandName, nsICommandParams *aParams, nsISupports *aCommandRefCon)
{
  
  nsCOMPtr<nsIControllerCommand> commandHandler;
  nsresult rv;
  rv = FindCommandHandler(aCommandName, getter_AddRefs(commandHandler));
  if (!commandHandler)
  {
#if DEBUG
    NS_WARNING("Controller command table asked to do a command that it does not handle -- ");
#endif
    return NS_OK;    
  }
  return commandHandler->GetCommandStateParams(aCommandName, aParams, aCommandRefCon);
}


nsresult
NS_NewControllerCommandTable(nsIControllerCommandTable** aResult)
{
  NS_PRECONDITION(aResult != nsnull, "null ptr");
  if (! aResult)
    return NS_ERROR_NULL_POINTER;

  nsControllerCommandTable* newCommandTable = new nsControllerCommandTable();
  if (! newCommandTable)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(newCommandTable);
  *aResult = newCommandTable;
  return NS_OK;
}



