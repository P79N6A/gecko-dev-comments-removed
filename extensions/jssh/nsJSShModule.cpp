





































#include "nsIGenericFactory.h"
#include "nsJSShServer.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(nsJSShServer)

static const nsModuleComponentInfo gComponents[] = {
    { "TCP/IP JavaScript Shell Server", 
      NS_JSSHSERVER_CID,
      NS_JSSHSERVER_CONTRACTID,
      nsJSShServerConstructor
    }
};

NS_IMPL_NSGETMODULE(nsJSShModule, gComponents)
