





































#include "nsGConfService.h"
#include "nsGnomeVFSService.h"
#include "nsIGenericFactory.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGConfService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGnomeVFSService, Init)

static const nsModuleComponentInfo components[] = {
  { "GConf Service",
    NS_GCONFSERVICE_CID,
    NS_GCONFSERVICE_CONTRACTID,
    nsGConfServiceConstructor },
  { "GnomeVFS Service",
    NS_GNOMEVFSSERVICE_CID,
    NS_GNOMEVFSSERVICE_CONTRACTID,
    nsGnomeVFSServiceConstructor }
};

NS_IMPL_NSGETMODULE(mozgnome, components)
