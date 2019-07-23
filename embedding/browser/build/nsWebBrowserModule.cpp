






































#include "nsIModule.h"
#include "nsIServiceManager.h"
#include "nsIGenericFactory.h"
#include "nsICategoryManager.h"
#include "nsXPIDLString.h"

#include "nsEmbedCID.h"

#include "nsWebBrowser.h"
#include "nsCommandHandler.h"
#include "nsWebBrowserContentPolicy.h"



NS_GENERIC_FACTORY_CONSTRUCTOR(nsWebBrowser)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWebBrowserContentPolicy)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCommandHandler)

static NS_METHOD
RegisterContentPolicy(nsIComponentManager *aCompMgr, nsIFile *aPath,
                      const char *registryLocation, const char *componentType,
                      const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    nsXPIDLCString previous;
    return catman->AddCategoryEntry("content-policy",
                                    NS_WEBBROWSERCONTENTPOLICY_CONTRACTID,
                                    NS_WEBBROWSERCONTENTPOLICY_CONTRACTID,
                                    PR_TRUE, PR_TRUE, getter_Copies(previous));
}

static NS_METHOD
UnregisterContentPolicy(nsIComponentManager *aCompMgr, nsIFile *aPath,
                        const char *registryLocation,
                        const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsCOMPtr<nsICategoryManager> catman =
        do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    return catman->DeleteCategoryEntry("content-policy",
                                       NS_WEBBROWSERCONTENTPOLICY_CONTRACTID,
                                       PR_TRUE);
}



static const nsModuleComponentInfo components[] =
{
   { "WebBrowser Component", NS_WEBBROWSER_CID, 
     NS_WEBBROWSER_CONTRACTID, nsWebBrowserConstructor },
   { "CommandHandler Component", NS_COMMANDHANDLER_CID,
     NS_COMMANDHANDLER_CONTRACTID, nsCommandHandlerConstructor },
   { "nsIWebBrowserSetup content policy enforcer", 
     NS_WEBBROWSERCONTENTPOLICY_CID,
     NS_WEBBROWSERCONTENTPOLICY_CONTRACTID,
     nsWebBrowserContentPolicyConstructor,
     RegisterContentPolicy, UnregisterContentPolicy }
};




NS_IMPL_NSGETMODULE(Browser_Embedding_Module, components)



