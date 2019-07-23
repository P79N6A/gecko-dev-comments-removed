





































#include "nsGConfService.h"
#include "nsGnomeVFSService.h"
#include "nsToolkitCompsCID.h"
#include "nsIGenericFactory.h"

#ifdef MOZ_ENABLE_LIBNOTIFY
#include "nsAlertsService.h"
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGConfService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGnomeVFSService, Init)

#ifdef MOZ_ENABLE_LIBNOTIFY
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsAlertsService, Init)
#endif

static const nsModuleComponentInfo components[] = {
  { "GConf Service",
    NS_GCONFSERVICE_CID,
    NS_GCONFSERVICE_CONTRACTID,
    nsGConfServiceConstructor },
  { "GnomeVFS Service",
    NS_GNOMEVFSSERVICE_CID,
    NS_GNOMEVFSSERVICE_CONTRACTID,
    nsGnomeVFSServiceConstructor },
#ifdef MOZ_ENABLE_LIBNOTIFY
  { "Gnome Alerts Service",
    NS_SYSTEMALERTSSERVICE_CID,
    NS_SYSTEMALERTSERVICE_CONTRACTID,
    nsAlertsServiceConstructor },
#endif
};

NS_IMPL_NSGETMODULE(mozgnome, components)
