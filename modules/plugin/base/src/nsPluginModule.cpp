





































#include "nsIGenericFactory.h"
#include "nsIPluginManager.h"
#include "nsPluginsCID.h"
#include "nsPluginHostImpl.h"
#include "nsJVMAuthTools.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsPluginHostImpl,
                                         nsPluginHostImpl::GetInst)
NS_GENERIC_AGGREGATED_CONSTRUCTOR(nsJVMAuthTools)

static const nsModuleComponentInfo gComponentInfo[] = {
  { "Plugin Host",
    NS_PLUGIN_HOST_CID,
    "@mozilla.org/plugin/host;1",
    nsPluginHostImplConstructor
  },
  { "Plugin Manager",
    NS_PLUGINMANAGER_CID,
    "@mozilla.org/plugin/manager;1",
    nsPluginHostImplConstructor
  },
  { "JVM Authentication Service", 
    NS_JVMAUTHTOOLS_CID,  
    "@mozilla.org/oji/jvm-auth-tools;1", 
    nsJVMAuthToolsConstructor
  }
};

NS_IMPL_NSGETMODULE(nsPluginModule, gComponentInfo)
