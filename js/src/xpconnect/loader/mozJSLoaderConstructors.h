










































#ifdef XPCONNECT_STANDALONE
#define NO_SUBSCRIPT_LOADER
#endif

#include "nsIGenericFactory.h"
#include "nsICategoryManager.h"
#include "mozJSComponentLoader.h"

#ifndef NO_SUBSCRIPT_LOADER
#include "mozJSSubScriptLoader.h"
const char mozJSSubScriptLoadContractID[] = "@mozilla.org/moz/jssubscript-loader;1";
#endif

static NS_METHOD
RegisterJSLoader(nsIComponentManager *aCompMgr, nsIFile *aPath,
                 const char *registryLocation, const char *componentType,
                 const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    nsXPIDLCString previous;
    return catman->AddCategoryEntry("module-loader",
                                    MOZJSCOMPONENTLOADER_TYPE_NAME,
                                    MOZJSCOMPONENTLOADER_CONTRACTID,
                                    PR_TRUE, PR_TRUE, getter_Copies(previous));
}

static NS_METHOD
UnregisterJSLoader(nsIComponentManager *aCompMgr, nsIFile *aPath,
                   const char *registryLocation,
                   const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    nsXPIDLCString jsLoader;
    rv = catman->GetCategoryEntry("module-loader",
                                  MOZJSCOMPONENTLOADER_TYPE_NAME,
                                  getter_Copies(jsLoader));
    if (NS_FAILED(rv)) return rv;

    
    if (!strcmp(jsLoader, MOZJSCOMPONENTLOADER_CONTRACTID)) {
        return catman->DeleteCategoryEntry("module-loader",
                                           MOZJSCOMPONENTLOADER_TYPE_NAME,
                                           PR_TRUE);
    }
    return NS_OK;
}

NS_GENERIC_FACTORY_CONSTRUCTOR(mozJSComponentLoader)

#ifndef NO_SUBSCRIPT_LOADER
NS_GENERIC_FACTORY_CONSTRUCTOR(mozJSSubScriptLoader)
#endif
