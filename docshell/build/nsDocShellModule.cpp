






































#include "nsIModule.h"
#include "nsIGenericFactory.h"

#include "nsDocShellCID.h"

#include "nsDocShell.h"
#include "nsDefaultURIFixup.h"
#include "nsWebNavigationInfo.h"

#include "nsAboutRedirector.h"


#include "nsURILoader.h"
#include "nsDocLoader.h"
#include "nsOSHelperAppService.h"
#include "nsExternalProtocolHandler.h"
#include "nsPrefetchService.h"
#include "nsOfflineCacheUpdate.h"
#include "nsLocalHandlerApp.h"
#ifdef MOZ_ENABLE_DBUS
#include "nsDBusHandlerApp.h"
#endif 


#include "nsSHEntry.h"
#include "nsSHistory.h"
#include "nsSHTransaction.h"


#include "nsGlobalHistoryAdapter.h"
#include "nsGlobalHistory2Adapter.h"


#include "nsDownloadHistory.h"

static PRBool gInitialized = PR_FALSE;



static nsresult
Initialize(nsIModule* aSelf)
{
  NS_PRECONDITION(!gInitialized, "docshell module already initialized");
  if (gInitialized) {
    return NS_OK;
  }
  gInitialized = PR_TRUE;

  nsresult rv = nsSHistory::Startup();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = nsSHEntry::Startup();
  return rv;
}

static void
Shutdown(nsIModule* aSelf)
{
  nsSHEntry::Shutdown();
  gInitialized = PR_FALSE;
}


NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsDocShell, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDefaultURIFixup)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsWebNavigationInfo, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsClassifierCallback)


NS_GENERIC_FACTORY_CONSTRUCTOR(nsURILoader)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsDocLoader, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsOSHelperAppService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsExternalProtocolHandler)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrefetchService, Init)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsOfflineCacheUpdateService,
                                         nsOfflineCacheUpdateService::GetInstance)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsOfflineCacheUpdate)
NS_GENERIC_FACTORY_CONSTRUCTOR(PlatformLocalHandlerApp_t)
#ifdef MOZ_ENABLE_DBUS
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDBusHandlerApp)
#endif 


NS_GENERIC_FACTORY_CONSTRUCTOR(nsSHEntry)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSHTransaction)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSHistory)


NS_GENERIC_FACTORY_CONSTRUCTOR(nsDownloadHistory)

static const nsModuleComponentInfo gDocShellModuleInfo[] = {
  
    { "DocShell", 
      NS_DOCSHELL_CID,
      "@mozilla.org/docshell;1",
      nsDocShellConstructor
    },
    { "Default keyword fixup", 
      NS_DEFAULTURIFIXUP_CID,
      NS_URIFIXUP_CONTRACTID,
      nsDefaultURIFixupConstructor
    },
    { "Webnavigation info service",
      NS_WEBNAVIGATION_INFO_CID,
      NS_WEBNAVIGATION_INFO_CONTRACTID,
      nsWebNavigationInfoConstructor
    },
    {
      "Channel classifier",
      NS_CHANNELCLASSIFIER_CID,
      NS_CHANNELCLASSIFIER_CONTRACTID,
      nsClassifierCallbackConstructor
    },

    
    { "about:config",
      NS_ABOUT_REDIRECTOR_MODULE_CID,
      NS_ABOUT_MODULE_CONTRACTID_PREFIX "config",
      nsAboutRedirector::Create
    },
#ifdef MOZ_CRASHREPORTER
    { "about:crashes",
      NS_ABOUT_REDIRECTOR_MODULE_CID,
      NS_ABOUT_MODULE_CONTRACTID_PREFIX "crashes",
      nsAboutRedirector::Create
    },
#endif
    { "about:credits",
      NS_ABOUT_REDIRECTOR_MODULE_CID,
      NS_ABOUT_MODULE_CONTRACTID_PREFIX "credits",
      nsAboutRedirector::Create
    },
    { "about:plugins",
      NS_ABOUT_REDIRECTOR_MODULE_CID,
      NS_ABOUT_MODULE_CONTRACTID_PREFIX "plugins",
      nsAboutRedirector::Create
    },
    { "about:mozilla",
      NS_ABOUT_REDIRECTOR_MODULE_CID,
      NS_ABOUT_MODULE_CONTRACTID_PREFIX "mozilla",
      nsAboutRedirector::Create
    },
    { "about:logo",
      NS_ABOUT_REDIRECTOR_MODULE_CID,
      NS_ABOUT_MODULE_CONTRACTID_PREFIX "logo",
      nsAboutRedirector::Create
    },
    { "about:buildconfig",
      NS_ABOUT_REDIRECTOR_MODULE_CID,
      NS_ABOUT_MODULE_CONTRACTID_PREFIX "buildconfig",
      nsAboutRedirector::Create
    },
    { "about:license",
      NS_ABOUT_REDIRECTOR_MODULE_CID,
      NS_ABOUT_MODULE_CONTRACTID_PREFIX "license",
      nsAboutRedirector::Create
    },
    { "about:licence",
      NS_ABOUT_REDIRECTOR_MODULE_CID,
      NS_ABOUT_MODULE_CONTRACTID_PREFIX "licence",
      nsAboutRedirector::Create
    },
    { "about:neterror",
      NS_ABOUT_REDIRECTOR_MODULE_CID,
      NS_ABOUT_MODULE_CONTRACTID_PREFIX "neterror",
      nsAboutRedirector::Create
    },
    { "about:memory",
      NS_ABOUT_REDIRECTOR_MODULE_CID,
      NS_ABOUT_MODULE_CONTRACTID_PREFIX "memory",
      nsAboutRedirector::Create
    },

    
  { "Netscape URI Loader Service", NS_URI_LOADER_CID, NS_URI_LOADER_CONTRACTID, nsURILoaderConstructor, },
  { "Netscape Doc Loader Service", NS_DOCUMENTLOADER_SERVICE_CID, NS_DOCUMENTLOADER_SERVICE_CONTRACTID, 
     nsDocLoaderConstructor, },
  { "Netscape External Helper App Service", NS_EXTERNALHELPERAPPSERVICE_CID, NS_EXTERNALHELPERAPPSERVICE_CONTRACTID, 
     nsOSHelperAppServiceConstructor, },
  { "Netscape External Helper App Service", NS_EXTERNALHELPERAPPSERVICE_CID, NS_EXTERNALPROTOCOLSERVICE_CONTRACTID, 
     nsOSHelperAppServiceConstructor, },
  { "Netscape Mime Mapping Service", NS_EXTERNALHELPERAPPSERVICE_CID, NS_MIMESERVICE_CONTRACTID, 
     nsOSHelperAppServiceConstructor, },
  { "Netscape Default Protocol Handler", NS_EXTERNALPROTOCOLHANDLER_CID, NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX"default", 
     nsExternalProtocolHandlerConstructor, },
  {  NS_PREFETCHSERVICE_CLASSNAME, NS_PREFETCHSERVICE_CID, NS_PREFETCHSERVICE_CONTRACTID,
     nsPrefetchServiceConstructor, },
  { NS_OFFLINECACHEUPDATESERVICE_CLASSNAME, NS_OFFLINECACHEUPDATESERVICE_CID, NS_OFFLINECACHEUPDATESERVICE_CONTRACTID,
    nsOfflineCacheUpdateServiceConstructor, },
  { NS_OFFLINECACHEUPDATE_CLASSNAME, NS_OFFLINECACHEUPDATE_CID, NS_OFFLINECACHEUPDATE_CONTRACTID,
    nsOfflineCacheUpdateConstructor, },
  { "Local Application Handler App", NS_LOCALHANDLERAPP_CID, 
    NS_LOCALHANDLERAPP_CONTRACTID, PlatformLocalHandlerApp_tConstructor, },
#ifdef MOZ_ENABLE_DBUS
  { "DBus Handler App", NS_DBUSHANDLERAPP_CID,
      NS_DBUSHANDLERAPP_CONTRACTID, nsDBusHandlerAppConstructor},
#endif
        
    
   { "nsSHEntry", NS_SHENTRY_CID,
      NS_SHENTRY_CONTRACTID, nsSHEntryConstructor },
   { "nsSHEntry", NS_HISTORYENTRY_CID,
      NS_HISTORYENTRY_CONTRACTID, nsSHEntryConstructor },
   { "nsSHTransaction", NS_SHTRANSACTION_CID,
      NS_SHTRANSACTION_CONTRACTID, nsSHTransactionConstructor },
   { "nsSHistory", NS_SHISTORY_CID,
      NS_SHISTORY_CONTRACTID, nsSHistoryConstructor },
   { "nsSHistory", NS_SHISTORY_INTERNAL_CID,
      NS_SHISTORY_INTERNAL_CONTRACTID, nsSHistoryConstructor },

    
    { "nsGlobalHistoryAdapter", NS_GLOBALHISTORYADAPTER_CID,
      nsnull, nsGlobalHistoryAdapter::Create,
      nsGlobalHistoryAdapter::RegisterSelf },
    { "nsGlobalHistory2Adapter", NS_GLOBALHISTORY2ADAPTER_CID,
      nsnull, nsGlobalHistory2Adapter::Create,
      nsGlobalHistory2Adapter::RegisterSelf },
    
    
    { "nsDownloadHistory", NS_DOWNLOADHISTORY_CID,
      nsnull, nsDownloadHistoryConstructor,
      nsDownloadHistory::RegisterSelf }
};



NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(docshell_provider, gDocShellModuleInfo,
                                   Initialize, Shutdown)
