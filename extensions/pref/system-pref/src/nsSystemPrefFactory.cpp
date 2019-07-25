








































#include "nsICategoryManager.h"
#include "mozilla/ModuleUtils.h"
#include "nsSystemPref.h"
#include "nsSystemPrefService.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsSystemPref, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsSystemPrefService, Init)

NS_DEFINE_NAMED_CID(NS_SYSTEMPREF_CID);
NS_DEFINE_NAMED_CID(NS_SYSTEMPREF_SERVICE_CID);

static const mozilla::Module::CIDEntry kSysPrefCIDs[] = {
    { &kNS_SYSTEMPREF_CID, false, NULL, nsSystemPrefConstructor },
    { &kNS_SYSTEMPREF_SERVICE_CID, false, NULL, nsSystemPrefServiceConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kSysPrefContracts[] = {
    { NS_SYSTEMPREF_CONTRACTID, &kNS_SYSTEMPREF_CID },
    { NS_SYSTEMPREF_SERVICE_CONTRACTID, &kNS_SYSTEMPREF_SERVICE_CID },
    { NULL }
};

static const mozilla::Module::CategoryEntry kSysPrefCategories[] = {
    { APPSTARTUP_CATEGORY, "SystemPref Module", NS_SYSTEMPREF_CONTRACTID },
    { NULL }
};

static const mozilla::Module kSysPrefModule = {
    mozilla::Module::kVersion,
    kSysPrefCIDs,
    kSysPrefContracts,
    kSysPrefCategories
};

NSMODULE_DEFN(nsSystemPrefModule) = &kSysPrefModule;
