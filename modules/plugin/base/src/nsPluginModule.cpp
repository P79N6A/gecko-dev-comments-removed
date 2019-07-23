





































#include "nsIGenericFactory.h"
#include "nsPluginHost.h"
#include "nsPluginsCID.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsPluginHost, nsPluginHost::GetInst)

static const nsModuleComponentInfo gComponentInfo[] = {
  { "Plugin Host",
    NS_PLUGIN_HOST_CID,
    MOZ_PLUGIN_HOST_CONTRACTID,
    nsPluginHostConstructor
  },
};

NS_IMPL_NSGETMODULE(nsPluginModule, gComponentInfo)
