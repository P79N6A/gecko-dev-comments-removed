





































#include "nsMetricsModule.h"
#include "nsMetricsService.h"
#include "nsLoadCollector.h"
#include "nsWindowCollector.h"
#include "nsProfileCollector.h"
#include "nsUICommandCollector.h"
#include "nsAutoCompleteCollector.h"
#include "nsIGenericFactory.h"
#include "nsICategoryManager.h"
#include "nsServiceManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsXPCOMCID.h"
#ifndef MOZILLA_1_8_BRANCH
#include "nsIClassInfoImpl.h"
#endif

NS_DECL_CLASSINFO(nsMetricsService)

#define COLLECTOR_CONTRACTID(type) \
  "@mozilla.org/metrics/collector;1?name=" type ":" NS_METRICS_NAMESPACE

static NS_METHOD
nsMetricsServiceRegisterSelf(nsIComponentManager *compMgr,
                             nsIFile *path,
                             const char *loaderStr,
                             const char *type,
                             const nsModuleComponentInfo *info)
{
  nsCOMPtr<nsICategoryManager> cat =
      do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
  NS_ENSURE_STATE(cat);

  cat->AddCategoryEntry("app-startup",
                        NS_METRICSSERVICE_CLASSNAME,
                        "service," NS_METRICSSERVICE_CONTRACTID,
                        PR_TRUE, PR_TRUE, nsnull);
  return NS_OK;
}

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsLoadCollector, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindowCollector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsProfileCollector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUICommandCollector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAutoCompleteCollector)

static const nsModuleComponentInfo components[] = {
  {
    NS_METRICSSERVICE_CLASSNAME,
    NS_METRICSSERVICE_CID,
    NS_METRICSSERVICE_CONTRACTID,
    nsMetricsService::Create,
    nsMetricsServiceRegisterSelf,
    NULL,
    NULL,
    NS_CI_INTERFACE_GETTER_NAME(nsMetricsService),
    NULL,
    &NS_CLASSINFO_NAME(nsMetricsService),
    nsIClassInfo::MAIN_THREAD_ONLY | nsIClassInfo::SINGLETON
  },
  {
    NS_METRICSSERVICE_CLASSNAME,
    NS_METRICSSERVICE_CID,
    NS_ABOUT_MODULE_CONTRACTID_PREFIX "metrics",
    nsMetricsService::Create
  },
  {
    NS_LOADCOLLECTOR_CLASSNAME,
    NS_LOADCOLLECTOR_CID,
    COLLECTOR_CONTRACTID("document"),
    nsLoadCollectorConstructor
  },
  {
    NS_WINDOWCOLLECTOR_CLASSNAME,
    NS_WINDOWCOLLECTOR_CID,
    COLLECTOR_CONTRACTID("window"),
    nsWindowCollectorConstructor
  },
  {
    NS_PROFILECOLLECTOR_CLASSNAME,
    NS_PROFILECOLLECTOR_CID,
    COLLECTOR_CONTRACTID("profile"),
    nsProfileCollectorConstructor
  },
  {
    NS_UICOMMANDCOLLECTOR_CLASSNAME,
    NS_UICOMMANDCOLLECTOR_CID,
    COLLECTOR_CONTRACTID("uielement"),
    nsUICommandCollectorConstructor
  },
  {
    NS_AUTOCOMPLETECOLLECTOR_CLASSNAME,
    NS_AUTOCOMPLETECOLLECTOR_CID,
    COLLECTOR_CONTRACTID("autocomplete"),
    nsAutoCompleteCollectorConstructor
  }
};

NS_IMPL_NSGETMODULE(metrics, components)
