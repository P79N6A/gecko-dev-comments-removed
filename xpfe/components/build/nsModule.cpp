




































#include "nsIGenericFactory.h"
#include "nsICategoryManager.h"
#include "nsNetUtil.h"
#include "nsXPIDLString.h"
#ifndef MOZ_THUNDERBIRD
#include "nsDirectoryViewer.h"
#ifdef MOZ_RDF
#include "rdf.h"
#include "nsRDFCID.h"
#endif
#include "nsCURILoader.h"
#include "nsXPFEComponentsCID.h"
#endif

#include "nsBrowserStatusFilter.h"

#ifndef MOZ_THUNDERBIRD
#ifdef MOZ_RDF

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsHTTPIndex, Init)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDirectoryViewerFactory)
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsBrowserStatusFilter)

#ifndef MOZ_THUNDERBIRD
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
#endif

static const nsModuleComponentInfo components[] = {
#ifndef MOZ_THUNDERBIRD
   { "Directory Viewer", NS_DIRECTORYVIEWERFACTORY_CID,
      "@mozilla.org/xpfe/http-index-format-factory-constructor",
      nsDirectoryViewerFactoryConstructor, RegisterProc, UnregisterProc  },
#ifdef MOZ_RDF
    { "Directory Viewer", NS_HTTPINDEX_SERVICE_CID, NS_HTTPINDEX_SERVICE_CONTRACTID,
      nsHTTPIndexConstructor },
    { "Directory Viewer", NS_HTTPINDEX_SERVICE_CID, NS_HTTPINDEX_DATASOURCE_CONTRACTID,
      nsHTTPIndexConstructor },
#endif
#endif
    { NS_BROWSERSTATUSFILTER_CLASSNAME,
      NS_BROWSERSTATUSFILTER_CID,
      NS_BROWSERSTATUSFILTER_CONTRACTID,
      nsBrowserStatusFilterConstructor
    },
};

NS_IMPL_NSGETMODULE(application, components)
