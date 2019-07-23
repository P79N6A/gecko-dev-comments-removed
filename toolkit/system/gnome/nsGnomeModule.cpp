





































#include "nsToolkitCompsCID.h"
#include "nsIGenericFactory.h"

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

static const nsModuleComponentInfo components[] = {
#ifdef MOZ_ENABLE_GCONF
  { "GConf Service",
    NS_GCONFSERVICE_CID,
    NS_GCONFSERVICE_CONTRACTID,
    nsGConfServiceConstructor },
#endif
#ifdef MOZ_ENABLE_GNOMEVFS
  { "GnomeVFS Service",
    NS_GNOMEVFSSERVICE_CID,
    NS_GNOMEVFSSERVICE_CONTRACTID,
    nsGnomeVFSServiceConstructor },
#endif
#ifdef MOZ_ENABLE_GIO
  { "GIO Service",
    NS_GIOSERVICE_CID,
    NS_GIOSERVICE_CONTRACTID,
    nsGIOServiceConstructor },
#endif
#ifdef MOZ_ENABLE_LIBNOTIFY
  { "Gnome Alerts Service",
    NS_SYSTEMALERTSSERVICE_CID,
    NS_SYSTEMALERTSERVICE_CONTRACTID,
    nsAlertsServiceConstructor },
#endif
};

NS_IMPL_NSGETMODULE(mozgnome, components)
