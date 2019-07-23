





































#include "nsProfile.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsProfile, Init)

static const nsModuleComponentInfo components[] =
{
    { "Profile Manager", 
      NS_PROFILE_CID,
      NS_PROFILE_CONTRACTID, 
      nsProfileConstructor,
    },
};

NS_IMPL_NSGETMODULE(nsProfileModule, components)
