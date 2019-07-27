




#include "nsString.h"
#include "nsIControllerCommand.h"
#include "nsControllerCommandTable.h"


nsresult
NS_NewControllerCommandTable(nsIControllerCommandTable** aResult);



#define NUM_COMMANDS_LENGTH       32


nsControllerCommandTable::nsControllerCommandTable()
: mCommandsTable(NUM_COMMANDS_LENGTH)
, mMutable(true)
{
}


nsControllerCommandTable::~nsControllerCommandTable()
{
}

NS_IMPL_ISUPPORTS(nsControllerCommandTable, nsIControllerCommandTable, nsISupportsWeakReference)

NS_IMETHODIMP
nsControllerCommandTable::MakeImmutable(void)
{
  mMutable = false;
  return NS_OK;
}

NS_IMETHODIMP
nsControllerCommandTable::RegisterCommand(const char * aCommandName, nsIControllerCommand *aCommand)
{
  NS_ENSURE_TRUE(mMutable, NS_ERROR_FAILURE);

  mCommandsTable.Put(nsDependentCString(aCommandName), aCommand);

  return NS_OK;
}


NS_IMETHODIMP
nsControllerCommandTable::UnregisterCommand(const char * aCommandName, nsIControllerCommand *aCommand)
{
  NS_ENSURE_TRUE(mMutable, NS_ERROR_FAILURE);

  nsDependentCString commandKey(aCommandName);

  if (!mCommandsTable.Get(commandKey, nullptr)) {
    return NS_ERROR_FAILURE;
  }

  mCommandsTable.Remove(commandKey);
  return NS_OK;
}


NS_IMETHODIMP
nsControllerCommandTable::FindCommandHandler(const char * aCommandName, nsIControllerCommand **outCommand)
{
  NS_ENSURE_ARG_POINTER(outCommand);

  *outCommand = nullptr;

  nsCOMPtr<nsIControllerCommand> foundCommand;
  mCommandsTable.Get(nsDependentCString(aCommandName), getter_AddRefs(foundCommand));
  if (!foundCommand) return NS_ERROR_FAILURE;

  foundCommand.forget(outCommand);
  return NS_OK;
}




NS_IMETHODIMP
nsControllerCommandTable::IsCommandEnabled(const char * aCommandName, nsISupports *aCommandRefCon, bool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  *aResult = false;

  
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
nsControllerCommandTable::SupportsCommand(const char * aCommandName, nsISupports *aCommandRefCon, bool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  

  *aResult = false;

  
  nsCOMPtr<nsIControllerCommand> commandHandler;
  FindCommandHandler(aCommandName, getter_AddRefs(commandHandler));

  *aResult = (commandHandler.get() != nullptr);
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
  FindCommandHandler(aCommandName, getter_AddRefs(commandHandler));
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
  FindCommandHandler(aCommandName, getter_AddRefs(commandHandler));
  if (!commandHandler)
  {
#if DEBUG
    NS_WARNING("Controller command table asked to do a command that it does not handle -- ");
#endif
    return NS_OK;    
  }
  return commandHandler->GetCommandStateParams(aCommandName, aParams, aCommandRefCon);
}

static PLDHashOperator
AddCommand(const nsACString& aKey, nsIControllerCommand* aData, void* aArg)
{
  
  
  
  char*** commands = static_cast<char***>(aArg);
  (**commands) = ToNewCString(aKey);
  (*commands)++;
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsControllerCommandTable::GetSupportedCommands(uint32_t* aCount,
                                               char*** aCommands)
{
  char** commands =
    static_cast<char **>(moz_xmalloc(sizeof(char *) * mCommandsTable.Count()));
  *aCount = mCommandsTable.Count();
  *aCommands = commands;

  mCommandsTable.EnumerateRead(AddCommand, &commands);
  return NS_OK;
}

nsresult
NS_NewControllerCommandTable(nsIControllerCommandTable** aResult)
{
  NS_PRECONDITION(aResult != nullptr, "null ptr");
  if (! aResult)
    return NS_ERROR_NULL_POINTER;

  nsControllerCommandTable* newCommandTable = new nsControllerCommandTable();
  if (! newCommandTable)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(newCommandTable);
  *aResult = newCommandTable;
  return NS_OK;
}
