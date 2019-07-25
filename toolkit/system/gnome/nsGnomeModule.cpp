





































#include "nsToolkitCompsCID.h"
#include "mozilla/ModuleUtils.h"

#ifdef MOZ_ENABLE_GCONF
#include "nsGConfService.h"
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGConfService, Init)
#endif
#ifdef MOZ_ENABLE_GNOMEVFS
#include "nsGnomeVFSService.h"
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGnomeVFSService, Init)
#endif
#ifdef MOZ_ENABLE_GIO
#include "nsGIOService.h"
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGIOService, Init)
#endif
#ifdef MOZ_ENABLE_LIBNOTIFY
#include "nsAlertsService.h"
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsAlertsService, Init)
#endif

#ifdef MOZ_ENABLE_GCONF
NS_DEFINE_NAMED_CID(NS_GCONFSERVICE_CID);
#endif
#ifdef MOZ_ENABLE_GNOMEVFS
NS_DEFINE_NAMED_CID(NS_GNOMEVFSSERVICE_CID);
#endif
#ifdef MOZ_ENABLE_GIO
NS_DEFINE_NAMED_CID(NS_GIOSERVICE_CID);
#endif
#ifdef MOZ_ENABLE_LIBNOTIFY
NS_DEFINE_NAMED_CID(NS_SYSTEMALERTSSERVICE_CID);
#endif


static const mozilla::Module::CIDEntry kGnomeCIDs[] = {
#ifdef MOZ_ENABLE_GCONF
  { &kNS_GCONFSERVICE_CID, false, NULL, nsGConfServiceConstructor },
#endif
#ifdef MOZ_ENABLE_GNOMEVFS
  { &kNS_GNOMEVFSSERVICE_CID, false, NULL, nsGnomeVFSServiceConstructor },
#endif
#ifdef MOZ_ENABLE_GIO
  { &kNS_GIOSERVICE_CID, false, NULL, nsGIOServiceConstructor },
#endif
#ifdef MOZ_ENABLE_LIBNOTIFY
  { &kNS_SYSTEMALERTSSERVICE_CID, false, NULL, nsAlertsServiceConstructor },
#endif
  { NULL }
};

static const mozilla::Module::ContractIDEntry kGnomeContracts[] = {
#ifdef MOZ_ENABLE_GCONF
  { NS_GCONFSERVICE_CONTRACTID, &kNS_GCONFSERVICE_CID },
#endif
#ifdef MOZ_ENABLE_GNOMEVFS
  { NS_GNOMEVFSSERVICE_CONTRACTID, &kNS_GNOMEVFSSERVICE_CID },
#endif
#ifdef MOZ_ENABLE_GIO
  { NS_GIOSERVICE_CONTRACTID, &kNS_GIOSERVICE_CID },
#endif
#ifdef MOZ_ENABLE_LIBNOTIFY
  { NS_SYSTEMALERTSERVICE_CONTRACTID, &kNS_SYSTEMALERTSSERVICE_CID },
#endif
  { NULL }
};

static const mozilla::Module kGnomeModule = {
  mozilla::Module::kVersion,
  kGnomeCIDs,
  kGnomeContracts
};

NSMODULE_DEFN(mozgnome) = &kGnomeModule;
