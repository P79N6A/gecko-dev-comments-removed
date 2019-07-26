




#include "mozilla/ModuleUtils.h"
#include "nsAutoConfig.h"
#include "nsReadConfig.h"
#include "nsIAppStartupNotifier.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsAutoConfig, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsReadConfig, Init)

NS_DEFINE_NAMED_CID(NS_AUTOCONFIG_CID);
NS_DEFINE_NAMED_CID(NS_READCONFIG_CID);

static const mozilla::Module::CIDEntry kAutoConfigCIDs[] = {
  { &kNS_AUTOCONFIG_CID, false, NULL, nsAutoConfigConstructor },
  { &kNS_READCONFIG_CID, false, NULL, nsReadConfigConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kAutoConfigContracts[] = {
  { NS_AUTOCONFIG_CONTRACTID, &kNS_AUTOCONFIG_CID },
  { NS_READCONFIG_CONTRACTID, &kNS_READCONFIG_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kAutoConfigCategories[] = {
  { "pref-config-startup", "ReadConfig Module", NS_READCONFIG_CONTRACTID },
  { NULL }
};

static const mozilla::Module kAutoConfigModule = {
  mozilla::Module::kVersion,
  kAutoConfigCIDs,
  kAutoConfigContracts,
  kAutoConfigCategories
};

NSMODULE_DEFN(nsAutoConfigModule) = &kAutoConfigModule;
