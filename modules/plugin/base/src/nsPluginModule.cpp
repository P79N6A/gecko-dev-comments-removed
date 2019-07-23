





































#include "nsIGenericFactory.h"
#include "nsIPluginManager.h"
#include "nsPluginsCID.h"
#include "nsPluginHostImpl.h"
#include "ns4xPlugin.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsPluginHostImpl,
                                         nsPluginHostImpl::GetInst)

static const nsModuleComponentInfo gComponentInfo[] = {
  { "Plugin Host",
    NS_PLUGIN_HOST_CID,
    "@mozilla.org/plugin/host;1",
    nsPluginHostImplConstructor },

  { "Plugin Manager",
    NS_PLUGINMANAGER_CID,
    "@mozilla.org/plugin/manager;1",
    nsPluginHostImplConstructor },
};

NS_IMPL_NSGETMODULE(nsPluginModule, gComponentInfo)
