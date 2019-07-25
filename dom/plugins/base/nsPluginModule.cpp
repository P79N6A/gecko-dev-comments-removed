





































#include "mozilla/ModuleUtils.h"
#include "nsPluginHost.h"
#include "nsPluginsCID.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsPluginHost, nsPluginHost::GetInst)
NS_DEFINE_NAMED_CID(NS_PLUGIN_HOST_CID);

static const mozilla::Module::CIDEntry kPluginCIDs[] = {
  { &kNS_PLUGIN_HOST_CID, false, NULL, nsPluginHostConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kPluginContracts[] = {
  { MOZ_PLUGIN_HOST_CONTRACTID, &kNS_PLUGIN_HOST_CID },
  { NULL }
};

static const mozilla::Module kPluginModule = {
  mozilla::Module::kVersion,
  kPluginCIDs,
  kPluginContracts
};

NSMODULE_DEFN(nsPluginModule) = &kPluginModule;
