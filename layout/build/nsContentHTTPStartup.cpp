





































#include "nsIServiceManager.h"
#include "nsICategoryManager.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"

#include "nsContentHTTPStartup.h"
#include "nsIHttpProtocolHandler.h"
#include "gbdate.h"

#define PRODUCT_NAME "Gecko"

NS_IMPL_ISUPPORTS1(nsContentHTTPStartup,nsIObserver)

nsresult
nsContentHTTPStartup::Observe( nsISupports *aSubject,
                              const char      *aTopic,
                              const PRUnichar *aData)
{
    if (nsCRT::strcmp(aTopic, NS_HTTP_STARTUP_TOPIC) != 0)
        return NS_OK;
    
    nsresult rv = nsnull;

    nsCOMPtr<nsIHttpProtocolHandler> http(do_QueryInterface(aSubject));
    if (NS_FAILED(rv)) return rv;
    
    rv = http->SetProduct(NS_LITERAL_CSTRING(PRODUCT_NAME));
    if (NS_FAILED(rv)) return rv;
    
    rv = http->SetProductSub(NS_LITERAL_CSTRING(PRODUCT_VERSION));
    if (NS_FAILED(rv)) return rv;
    
    return NS_OK;
}

nsresult
nsContentHTTPStartup::RegisterHTTPStartup(nsIComponentManager*         aCompMgr,
                                          nsIFile*                     aPath,
                                          const char*                  aRegistryLocation,
                                          const char*                  aComponentType,
                                          const nsModuleComponentInfo* aInfo)
{
    nsresult rv;
    nsCOMPtr<nsICategoryManager>
        catMan(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  
    if (NS_FAILED(rv)) return rv;

    nsXPIDLCString previousEntry;
    rv = catMan->AddCategoryEntry(NS_HTTP_STARTUP_CATEGORY,
                                  "Content UserAgent Setter",
                                  NS_CONTENTHTTPSTARTUP_CONTRACTID,
                                  PR_TRUE, PR_TRUE,
                                  getter_Copies(previousEntry));
    return rv;
}

nsresult
nsContentHTTPStartup::UnregisterHTTPStartup(nsIComponentManager*         aCompMgr,
                                            nsIFile*                     aPath,
                                            const char*                  aRegistryLocation,
                                            const nsModuleComponentInfo* aInfo)
{
    nsresult rv;
    nsCOMPtr<nsICategoryManager>
        catMan(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  
    if (NS_FAILED(rv)) return rv;


    return NS_OK;
}
