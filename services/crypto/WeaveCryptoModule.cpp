





































#include "nsIGenericFactory.h"
#include "WeaveCrypto.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(WeaveCrypto)

static nsModuleComponentInfo components[] =
{
  {
    WEAVE_CRYPTO_CLASSNAME,
    WEAVE_CRYPTO_CID,
    WEAVE_CRYPTO_CONTRACTID,
    WeaveCryptoConstructor,
  }
};

NS_IMPL_NSGETMODULE("WeaveCryptoModule", components) 
