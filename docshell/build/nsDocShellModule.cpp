






































#include "nsIModule.h"
#include "nsIGenericFactory.h"

#include "nsDocShellCID.h"

#include "nsWebShell.h"
#include "nsDefaultURIFixup.h"
#include "nsWebNavigationInfo.h"

#include "nsAboutRedirector.h"


#include "nsURILoader.h"
#include "nsDocLoader.h"
#include "nsOSHelperAppService.h"
#include "nsExternalProtocolHandler.h"
#include "nsPrefetchService.h"
#include "nsHandlerAppImpl.h"


#include "nsSHEntry.h"
#include "nsSHistory.h"
#include "nsSHTransaction.h"


#include "nsGlobalHistoryAdapter.h"
#include "nsGlobalHistory2Adapter.h"

static PRBool gInitialized = PR_FALSE;



PR_STATIC_CALLBACK(nsresult)
Initialize(nsIModule* aSelf)
{
  NS_PRECONDITION(!gInitialized, "docshell module already initialized");
  if (gInitialized) {
    return NS_OK;
  }
  gInitialized = PR_TRUE;

  nsresult rv = nsSHistory::Startup();
  return rv;
}

PR_STATIC_CALLBACK(void)
Shutdown(nsIModule* aSelf)
{
  gInitialized = PR_FALSE;
}


NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsWebShell, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDefaultURIFixup)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsWebNavigationInfo, Init)


NS_GENERIC_FACTORY_CONSTRUCTOR(nsURILoader)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsDocLoader, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsOSHelperAppService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsExternalProtocolHandler)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBlockedExternalProtocolHandler)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrefetchService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLocalHandlerApp)

#if defined(XP_MAC) || defined(XP_MACOSX)
#include "nsInternetConfigService.h"
NS_GENERIC_FACTORY_CONSTRUCTOR(nsInternetConfigService)
#endif


NS_GENERIC_FACTORY_CONSTRUCTOR(nsSHEntry)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSHTransaction)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSHistory)








static const nsModuleComponentInfo gDocShellModuleInfo[] = {
  
    { "WebShell", 
      NS_WEB_SHELL_CID,
      "@mozilla.org/webshell;1",
      nsWebShellConstructor
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

    
    { "about:config",
      NS_ABOUT_REDIRECTOR_MODULE_CID,
      NS_ABOUT_MODULE_CONTRACTID_PREFIX "config",
      nsAboutRedirector::Create
    },
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
    { "about:about",
      NS_ABOUT_REDIRECTOR_MODULE_CID,
      NS_ABOUT_MODULE_CONTRACTID_PREFIX "about",
      nsAboutRedirector::Create
    },
    { "about:neterror",
      NS_ABOUT_REDIRECTOR_MODULE_CID,
      NS_ABOUT_MODULE_CONTRACTID_PREFIX "neterror",
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
  { "Netscape Default Blocked Protocol Handler", NS_BLOCKEDEXTERNALPROTOCOLHANDLER_CID, NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX"default-blocked", 
     nsBlockedExternalProtocolHandlerConstructor, },
  {  NS_PREFETCHSERVICE_CLASSNAME, NS_PREFETCHSERVICE_CID, NS_PREFETCHSERVICE_CONTRACTID,
     nsPrefetchServiceConstructor, },
  { "Local Application Handler App", NS_LOCALHANDLERAPP_CID, 
    NS_LOCALHANDLERAPP_CONTRACTID, nsLocalHandlerAppConstructor, },
#if defined(XP_MAC) || defined(XP_MACOSX)
  { "Internet Config Service", NS_INTERNETCONFIGSERVICE_CID, NS_INTERNETCONFIGSERVICE_CONTRACTID,
    nsInternetConfigServiceConstructor, },
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
      nsGlobalHistory2Adapter::RegisterSelf }
};



NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(docshell_provider, gDocShellModuleInfo,
                                   Initialize, Shutdown)
