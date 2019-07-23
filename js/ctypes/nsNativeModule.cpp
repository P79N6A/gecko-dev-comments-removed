






































#include "nsIGenericFactory.h"
#include "nsNativeTypes.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsNativeTypes)

static nsModuleComponentInfo components[] =
{
  {
    "jsctypes",
    NATIVETYPES_CID,
    NATIVETYPES_CONTRACTID,
    nsNativeTypesConstructor,
  }
};

NS_IMPL_NSGETMODULE("jsctypes", components)
