




#include "nsToolkitCompsCID.h"
#include "mozilla/ModuleUtils.h"

#include <glib-object.h>

#ifdef MOZ_ENABLE_GCONF
#include "nsGConfService.h"
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGConfService, Init)
#endif
#ifdef MOZ_ENABLE_GIO
#include "nsGIOService.h"
#include "nsGSettingsService.h"
#include "nsPackageKitService.h"
NS_GENERIC_FACTORY_CONSTRUCTOR(nsGIOService)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGSettingsService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPackageKitService, Init)
#endif
#include "nsSystemAlertsService.h"
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsSystemAlertsService, Init)

#ifdef MOZ_ENABLE_GCONF
NS_DEFINE_NAMED_CID(NS_GCONFSERVICE_CID);
#endif
#ifdef MOZ_ENABLE_GIO
NS_DEFINE_NAMED_CID(NS_GIOSERVICE_CID);
NS_DEFINE_NAMED_CID(NS_GSETTINGSSERVICE_CID);
NS_DEFINE_NAMED_CID(NS_PACKAGEKITSERVICE_CID);
#endif
NS_DEFINE_NAMED_CID(NS_SYSTEMALERTSSERVICE_CID);

static const mozilla::Module::CIDEntry kGnomeCIDs[] = {
#ifdef MOZ_ENABLE_GCONF
  { &kNS_GCONFSERVICE_CID, false, nullptr, nsGConfServiceConstructor },
#endif
#ifdef MOZ_ENABLE_GIO
  { &kNS_GIOSERVICE_CID, false, nullptr, nsGIOServiceConstructor },
  { &kNS_GSETTINGSSERVICE_CID, false, nullptr, nsGSettingsServiceConstructor },
  { &kNS_PACKAGEKITSERVICE_CID, false, nullptr, nsPackageKitServiceConstructor },
#endif
  { &kNS_SYSTEMALERTSSERVICE_CID, false, nullptr, nsSystemAlertsServiceConstructor },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kGnomeContracts[] = {
#ifdef MOZ_ENABLE_GCONF
  { NS_GCONFSERVICE_CONTRACTID, &kNS_GCONFSERVICE_CID },
#endif
#ifdef MOZ_ENABLE_GIO
  { NS_GIOSERVICE_CONTRACTID, &kNS_GIOSERVICE_CID },
  { NS_GSETTINGSSERVICE_CONTRACTID, &kNS_GSETTINGSSERVICE_CID },
  { NS_PACKAGEKITSERVICE_CONTRACTID, &kNS_PACKAGEKITSERVICE_CID },
#endif
  { NS_SYSTEMALERTSERVICE_CONTRACTID, &kNS_SYSTEMALERTSSERVICE_CID },
  { nullptr }
};

static nsresult
InitGType ()
{
  g_type_init();
  return NS_OK;
}

static const mozilla::Module kGnomeModule = {
  mozilla::Module::kVersion,
  kGnomeCIDs,
  kGnomeContracts,
  nullptr,
  nullptr,
  InitGType
};

NSMODULE_DEFN(mozgnome) = &kGnomeModule;
