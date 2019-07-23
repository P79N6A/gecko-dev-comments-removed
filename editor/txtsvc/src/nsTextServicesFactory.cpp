




































#include "nsIGenericFactory.h"

#include "nsTextServicesDocument.h"
#include "nsTextServicesCID.h"






NS_GENERIC_FACTORY_CONSTRUCTOR(nsTextServicesDocument)






static const nsModuleComponentInfo components[] = {
  { NULL, NS_TEXTSERVICESDOCUMENT_CID, "@mozilla.org/textservices/textservicesdocument;1", nsTextServicesDocumentConstructor },
};





NS_IMPL_NSGETMODULE(nsTextServicesModule, components)
