





































#include "mozilla/ModuleUtils.h"

#include "nsEntropyCollector.h"
#include "nsSecureBrowserUIImpl.h"
#include "nsSecurityWarningDialogs.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsEntropyCollector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSecureBrowserUIImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsSecurityWarningDialogs, Init)

NS_DEFINE_NAMED_CID(NS_ENTROPYCOLLECTOR_CID);
NS_DEFINE_NAMED_CID(NS_SECURITYWARNINGDIALOGS_CID);
NS_DEFINE_NAMED_CID(NS_SECURE_BROWSER_UI_CID);

static const mozilla::Module::CIDEntry kBOOTCIDs[] = {
  { &kNS_ENTROPYCOLLECTOR_CID, false, NULL, nsEntropyCollectorConstructor },
  { &kNS_SECURITYWARNINGDIALOGS_CID, false, NULL, nsSecurityWarningDialogsConstructor },
  { &kNS_SECURE_BROWSER_UI_CID, false, NULL, nsSecureBrowserUIImplConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kBOOTContracts[] = {
  { NS_ENTROPYCOLLECTOR_CONTRACTID, &kNS_ENTROPYCOLLECTOR_CID },
  { NS_SECURITYWARNINGDIALOGS_CONTRACTID, &kNS_SECURITYWARNINGDIALOGS_CID },
  { NS_SECURE_BROWSER_UI_CONTRACTID, &kNS_SECURE_BROWSER_UI_CID },
  { NULL }
};

static const mozilla::Module kBootModule = {
  mozilla::Module::kVersion,
  kBOOTCIDs,
  kBOOTContracts
};

NSMODULE_DEFN(BOOT) = &kBootModule;
