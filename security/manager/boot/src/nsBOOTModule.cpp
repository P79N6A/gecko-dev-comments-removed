





































#include "nsIModule.h"
#include "nsIGenericFactory.h"

#include "nsEntropyCollector.h"
#include "nsSecureBrowserUIImpl.h"
#include "nsSecurityWarningDialogs.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsEntropyCollector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSecureBrowserUIImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsSecurityWarningDialogs, Init)

static const nsModuleComponentInfo components[] =
{
  {
    "Entropy Collector",
    NS_ENTROPYCOLLECTOR_CID,
    NS_ENTROPYCOLLECTOR_CONTRACTID,
    nsEntropyCollectorConstructor
  },

  {
    "PSM Security Warnings",
    NS_SECURITYWARNINGDIALOGS_CID,
    NS_SECURITYWARNINGDIALOGS_CONTRACTID,
    nsSecurityWarningDialogsConstructor
  },

  {
    NS_SECURE_BROWSER_UI_CLASSNAME,
    NS_SECURE_BROWSER_UI_CID,
    NS_SECURE_BROWSER_UI_CONTRACTID,
    nsSecureBrowserUIImplConstructor
  }
};

NS_IMPL_NSGETMODULE(BOOT, components)
