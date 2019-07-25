





































#include "mozilla/ModuleUtils.h"
#include "nsAutoConfig.h"
#include "nsReadConfig.h"
#include "nsIAppStartupNotifier.h"
#if defined(MOZ_LDAP_XPCOM)
#include "nsLDAPSyncQuery.h"
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsAutoConfig, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsReadConfig, Init)
#if defined(MOZ_LDAP_XPCOM)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLDAPSyncQuery)
#endif

NS_DEFINE_NAMED_CID(NS_AUTOCONFIG_CID);
NS_DEFINE_NAMED_CID(NS_READCONFIG_CID);
#if defined(MOZ_LDAP_XPCOM)
NS_DEFINE_NAMED_CID(NS_LDAPSYNCQUERY_CID);
#endif

static const mozilla::Module::CIDEntry kAutoConfigCIDs[] = {
  { &kNS_AUTOCONFIG_CID, false, NULL, nsAutoConfigConstructor },
  { &kNS_READCONFIG_CID, false, NULL, nsReadConfigConstructor },
#if defined(MOZ_LDAP_XPCOM)
  { &kNS_LDAPSYNCQUERY_CID, false, NULL, nsLDAPSyncQueryConstructor },
#endif
  { NULL }
};

static const mozilla::Module::ContractIDEntry kAutoConfigContracts[] = {
  { NS_AUTOCONFIG_CONTRACTID, &kNS_AUTOCONFIG_CID },
  { NS_READCONFIG_CONTRACTID, &kNS_READCONFIG_CID },
#if defined(MOZ_LDAP_XPCOM)
  { "@mozilla.org/ldapsyncquery;1", &kNS_LDAPSYNCQUERY_CID },
#endif
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
