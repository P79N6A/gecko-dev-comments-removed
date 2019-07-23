




































#include "nsICategoryManager.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "tmCID.h"
#include "tmTransactionService.h"






NS_GENERIC_FACTORY_CONSTRUCTOR(tmTransactionService)






static const nsModuleComponentInfo components[] = {
  { TRANSACTION_SERVICE_CLASSNAME,
    TRANSACTION_SERVICE_CID,
    TRANSACTION_SERVICE_CONTRACTID,
    tmTransactionServiceConstructor },
    



};





NS_IMPL_NSGETMODULE(transmngr, components)
