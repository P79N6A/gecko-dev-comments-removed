



#include "nsGrowlAlertsService.h"
#include "nsToolkitCompsCID.h"
#include "mozilla/ModuleUtils.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsServiceManagerUtils.h"
#include "nsICategoryManager.h"
#include "nsMemory.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGrowlAlertsService, Init)
NS_DEFINE_NAMED_CID(NS_SYSTEMALERTSSERVICE_CID);

static const mozilla::Module::CIDEntry kAlertsCIDs[] = {
  { &kNS_SYSTEMALERTSSERVICE_CID, false, NULL, nsGrowlAlertsServiceConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kAlertsContracts[] = {
  { NS_SYSTEMALERTSERVICE_CONTRACTID, &kNS_SYSTEMALERTSSERVICE_CID },
  { NULL }
};

static const mozilla::Module kAlertsModule = {
  mozilla::Module::kVersion,
  kAlertsCIDs,
  kAlertsContracts
};

NSMODULE_DEFN(nsAlertsServiceModule) = &kAlertsModule;
