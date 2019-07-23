




































#include "nsIGenericFactory.h"
#include "nsAppStartup.h"
#include "nsUserInfo.h"
#include "nsXPFEComponentsCID.h"
#include "nsToolkitCompsCID.h"
#ifdef ALERTS_SERVICE
#include "nsAlertsService.h"
#endif

#ifndef MOZ_SUITE

#ifdef MOZ_RDF
#include "nsDownloadManager.h"
#include "nsDownloadProxy.h"
#endif
#endif 

#include "nsTypeAheadFind.h"

#ifdef MOZ_URL_CLASSIFIER
#include "nsUrlClassifierDBService.h"
#include "nsUrlClassifierStreamUpdater.h"
#include "nsUrlClassifierUtils.h"
#endif

#ifdef MOZ_FEEDS
#include "nsScriptableUnescapeHTML.h"
#endif



NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsAppStartup, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUserInfo)

#ifdef ALERTS_SERVICE
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAlertsService)
#endif

#ifndef MOZ_SUITE

#ifdef MOZ_RDF
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsDownloadManager, Init) 
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDownloadProxy)
#endif
#endif 

NS_GENERIC_FACTORY_CONSTRUCTOR(nsTypeAheadFind)

#ifdef MOZ_URL_CLASSIFIER
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsUrlClassifierDBService,
                                         nsUrlClassifierDBService::GetInstance)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUrlClassifierStreamUpdater)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsUrlClassifierUtils, Init)
#endif

#ifdef MOZ_FEEDS
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScriptableUnescapeHTML)
#endif



static const nsModuleComponentInfo components[] =
{
  { "App Startup Service",
    NS_TOOLKIT_APPSTARTUP_CID,
    NS_APPSTARTUP_CONTRACTID,
    nsAppStartupConstructor },

  { "User Info Service",
    NS_USERINFO_CID,
    NS_USERINFO_CONTRACTID,
    nsUserInfoConstructor },
#ifdef ALERTS_SERVICE
  { "Alerts Service",
    NS_ALERTSSERVICE_CID, 
    NS_ALERTSERVICE_CONTRACTID,
    nsAlertsServiceConstructor },
#endif
#ifndef MOZ_SUITE

#ifdef MOZ_RDF
  { "Download Manager",
    NS_DOWNLOADMANAGER_CID,
    NS_DOWNLOADMANAGER_CONTRACTID,
    nsDownloadManagerConstructor },
  { "Download",
    NS_DOWNLOAD_CID,
    NS_TRANSFER_CONTRACTID,
    nsDownloadProxyConstructor },
#endif
#endif 
  { "TypeAheadFind Component",
    NS_TYPEAHEADFIND_CID,
    NS_TYPEAHEADFIND_CONTRACTID,
    nsTypeAheadFindConstructor
  },
#ifdef MOZ_URL_CLASSIFIER
  { "Url Classifier DB Service",
    NS_URLCLASSIFIERDBSERVICE_CID,
    NS_URLCLASSIFIERDBSERVICE_CONTRACTID,
    nsUrlClassifierDBServiceConstructor },
  { "Url Classifier Stream Updater",
    NS_URLCLASSIFIERSTREAMUPDATER_CID,
    NS_URLCLASSIFIERSTREAMUPDATER_CONTRACTID,
    nsUrlClassifierStreamUpdaterConstructor },
  { "Url Classifier Utils",
    NS_URLCLASSIFIERUTILS_CID,
    NS_URLCLASSIFIERUTILS_CONTRACTID,
    nsUrlClassifierUtilsConstructor },
#endif
#ifdef MOZ_FEEDS
  { "Unescape HTML",
    NS_SCRIPTABLEUNESCAPEHTML_CID,
    NS_SCRIPTABLEUNESCAPEHTML_CONTRACTID,
    nsScriptableUnescapeHTMLConstructor },
#endif
};

NS_IMPL_NSGETMODULE(nsToolkitCompsModule, components)
