




































#include "nsIGenericFactory.h"

#include "nsTransactionManagerCID.h"
#include "nsTransactionStack.h"
#include "nsTransactionManager.h"






NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransactionManager)






static const nsModuleComponentInfo components[] = {
  { "nsTransactionManager", NS_TRANSACTIONMANAGER_CID, NS_TRANSACTIONMANAGER_CONTRACTID, nsTransactionManagerConstructor },
};





NS_IMPL_NSGETMODULE(nsTransactionManagerModule, components)
