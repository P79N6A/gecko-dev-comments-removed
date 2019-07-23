





































#include "nsIGenericFactory.h"
#include "nsPluginHostImpl.h"
#include "nsPluginsCID.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsPluginHostImpl,
                                         nsPluginHostImpl::GetInst)

static const nsModuleComponentInfo gComponentInfo[] = {
  { "Plugin Host",
    NS_PLUGIN_HOST_CID,
    MOZ_PLUGIN_HOST_CONTRACTID,
    nsPluginHostImplConstructor
  },
};

NS_IMPL_NSGETMODULE(nsPluginModule, gComponentInfo)
