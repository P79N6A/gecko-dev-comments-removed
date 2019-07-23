




































#include "nsIGenericFactory.h"
#include "nsAppStartup.h"
#include "nsUserInfo.h"
#include "nsXPFEComponentsCID.h"
#include "nsToolkitCompsCID.h"

#if defined(XP_WIN) && !defined(MOZ_DISABLE_PARENTAL_CONTROLS)
#include "nsParentalControlsServiceWin.h"
#endif

#ifdef ALERTS_SERVICE
#include "nsAlertsService.h"
#endif

#ifdef MOZ_RDF
#include "nsDownloadManager.h"
#include "nsDownloadProxy.h"
#endif

#include "nsTypeAheadFind.h"

#ifdef MOZ_URL_CLASSIFIER
#include "nsUrlClassifierDBService.h"
#include "nsUrlClassifierStreamUpdater.h"
#include "nsUrlClassifierUtils.h"
#include "nsUrlClassifierHashCompleter.h"
#include "nsDocShellCID.h"
#endif

#ifdef MOZ_FEEDS
#include "nsScriptableUnescapeHTML.h"
#endif



NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsAppStartup, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUserInfo)

#if defined(XP_WIN) && !defined(MOZ_DISABLE_PARENTAL_CONTROLS)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsParentalControlsServiceWin)
#endif

#ifdef ALERTS_SERVICE
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAlertsService)
#endif

#ifdef MOZ_RDF
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsDownloadManager,
                                         nsDownloadManager::GetSingleton) 
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDownloadProxy)
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsTypeAheadFind)

#ifdef MOZ_URL_CLASSIFIER
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUrlClassifierStreamUpdater)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsUrlClassifierUtils, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsUrlClassifierHashCompleter, Init)

static NS_IMETHODIMP
nsUrlClassifierDBServiceConstructor(nsISupports *aOuter, REFNSIID aIID,
                                    void **aResult)
{
    nsresult rv;
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_NO_AGGREGATION(aOuter);

    nsUrlClassifierDBService *inst = nsUrlClassifierDBService::GetInstance(&rv);
    if (NULL == inst) {
        return rv;
    }
    
    rv = inst->QueryInterface(aIID, aResult);
    NS_RELEASE(inst);

    return rv;
}
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
#if defined(XP_WIN) && !defined(MOZ_DISABLE_PARENTAL_CONTROLS)
  { "Parental Controls Service",
    NS_PARENTALCONTROLSSERVICE_CID,
    NS_PARENTALCONTROLSSERVICE_CONTRACTID,
    nsParentalControlsServiceWinConstructor },
#endif
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
  { "Url Classifier DB Service",
    NS_URLCLASSIFIERDBSERVICE_CID,
    NS_URICLASSIFIERSERVICE_CONTRACTID,
    nsUrlClassifierDBServiceConstructor },
  { "Url Classifier Stream Updater",
    NS_URLCLASSIFIERSTREAMUPDATER_CID,
    NS_URLCLASSIFIERSTREAMUPDATER_CONTRACTID,
    nsUrlClassifierStreamUpdaterConstructor },
  { "Url Classifier Utils",
    NS_URLCLASSIFIERUTILS_CID,
    NS_URLCLASSIFIERUTILS_CONTRACTID,
    nsUrlClassifierUtilsConstructor },
  { "Url Classifier Hash Completer",
    NS_URLCLASSIFIERHASHCOMPLETER_CID,
    NS_URLCLASSIFIERHASHCOMPLETER_CONTRACTID,
    nsUrlClassifierHashCompleterConstructor },
#endif
#ifdef MOZ_FEEDS
  { "Unescape HTML",
    NS_SCRIPTABLEUNESCAPEHTML_CID,
    NS_SCRIPTABLEUNESCAPEHTML_CONTRACTID,
    nsScriptableUnescapeHTMLConstructor },
#endif
};

NS_IMPL_NSGETMODULE(nsToolkitCompsModule, components)
