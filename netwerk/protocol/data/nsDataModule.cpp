




































#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsDataHandler.h"


static const nsModuleComponentInfo components[] = {
    { "Data Protocol Handler", 
      NS_DATAHANDLER_CID,
      NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "data", 
      nsDataHandler::Create},
};

NS_IMPL_NSGETMODULE(nsDataProtocolModule, components)



