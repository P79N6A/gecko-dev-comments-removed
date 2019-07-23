





































#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsJVMAuthTools.h"
#include "nsJVMManager.h"
#include "nsJVMConfigManager.h"

#ifdef XP_UNIX
#include "nsJVMConfigManagerUnix.h"
#endif






#ifdef XP_UNIX
NS_GENERIC_FACTORY_CONSTRUCTOR(nsJVMConfigManagerUnix)
#endif
NS_GENERIC_AGGREGATED_CONSTRUCTOR(nsJVMManager)
NS_GENERIC_AGGREGATED_CONSTRUCTOR(nsJVMAuthTools)


static const nsModuleComponentInfo components[] = 
{
    { "JVM Manager Service", 
      NS_JVMMANAGER_CID,  
      "@mozilla.org/oji/jvm-mgr;1", 
      nsJVMManagerConstructor
    },

    { "JVM Authentication Service", 
      NS_JVMAUTHTOOLS_CID,  
      "@mozilla.org/oji/jvm-auth-tools;1", 
      nsJVMAuthToolsConstructor
    },
#ifdef XP_UNIX
    { "JVM Config Manager",
      NS_JVMCONFIGMANAGER_CID,
      "@mozilla.org/oji/jvm-config-mgr;1",
      nsJVMConfigManagerUnixConstructor
    },
#endif
};

NS_IMPL_NSGETMODULE(nsCJVMManagerModule, components)
