




































#include "nsIGenericFactory.h"
#include "nsICategoryManager.h"
#include "nsNetUtil.h"
#include "nsXPIDLString.h"
#include "nsDirectoryViewer.h"
#ifdef MOZ_RDF
#include "rdf.h"
#include "nsRDFCID.h"
#endif

#ifdef SUITE_USING_XPFE_DM
#include "nsDownloadManager.h"
#include "nsDownloadProxy.h"
#endif

#if !defined(MOZ_MACBROWSER)
#include "nsBrowserStatusFilter.h"
#include "nsBrowserInstance.h"
#endif
#include "nsCURILoader.h"
#include "nsXPFEComponentsCID.h"

#ifdef MOZ_RDF

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsHTTPIndex, Init)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDirectoryViewerFactory)

#if !defined(MOZ_MACBROWSER)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBrowserStatusFilter)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBrowserInstance)
#endif

#ifdef SUITE_USING_XPFE_DM
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsDownloadManager, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDownloadProxy)
#endif

static NS_METHOD
RegisterProc(nsIComponentManager *aCompMgr,
             nsIFile *aPath,
             const char *registryLocation,
             const char *componentType,
             const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    
    
    
    return catman->AddCategoryEntry("Gecko-Content-Viewers", "application/http-index-format",
                                    "@mozilla.org/xpfe/http-index-format-factory-constructor",
                                    PR_TRUE, PR_TRUE, nsnull);
}

static NS_METHOD
UnregisterProc(nsIComponentManager *aCompMgr,
               nsIFile *aPath,
               const char *registryLocation,
               const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsCOMPtr<nsICategoryManager> catman = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    return catman->DeleteCategoryEntry("Gecko-Content-Viewers",
                                       "application/http-index-format", PR_TRUE);
}

static const nsModuleComponentInfo components[] = {
   { "Directory Viewer", NS_DIRECTORYVIEWERFACTORY_CID,
      "@mozilla.org/xpfe/http-index-format-factory-constructor",
      nsDirectoryViewerFactoryConstructor, RegisterProc, UnregisterProc  },
#ifdef MOZ_RDF
    { "Directory Viewer", NS_HTTPINDEX_SERVICE_CID, NS_HTTPINDEX_SERVICE_CONTRACTID,
      nsHTTPIndexConstructor },
    { "Directory Viewer", NS_HTTPINDEX_SERVICE_CID, NS_HTTPINDEX_DATASOURCE_CONTRACTID,
      nsHTTPIndexConstructor },
#endif

#ifdef SUITE_USING_XPFE_DM
    { "Download Manager", NS_DOWNLOADMANAGER_CID, NS_DOWNLOADMANAGER_CONTRACTID,
      nsDownloadManagerConstructor },
    { "Download", NS_DOWNLOAD_CID, NS_TRANSFER_CONTRACTID,
      nsDownloadProxyConstructor },
#endif

#if !defined(MOZ_MACBROWSER)
    { NS_BROWSERSTATUSFILTER_CLASSNAME,
      NS_BROWSERSTATUSFILTER_CID,
      NS_BROWSERSTATUSFILTER_CONTRACTID,
      nsBrowserStatusFilterConstructor
    },
    { "nsBrowserInstance",
      NS_BROWSERINSTANCE_CID,
      NS_BROWSERINSTANCE_CONTRACTID,
      nsBrowserInstanceConstructor
    },
#endif

};

NS_IMPL_NSGETMODULE(application, components)
