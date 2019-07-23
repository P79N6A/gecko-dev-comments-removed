




































#include "nscore.h"
#include "nsLayoutDebugCIID.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsRegressionTester.h"
#include "nsLayoutDebuggingTools.h"
#include "nsLayoutDebugCLH.h"
#include "nsIGenericFactory.h"
#include "nsICategoryManager.h"
#include "nsIServiceManager.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsRegressionTester)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLayoutDebuggingTools)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLayoutDebugCLH)

#ifdef MOZ_XUL_APP

static NS_IMETHODIMP
RegisterCommandLineHandlers(nsIComponentManager* compMgr, nsIFile* path,
                            const char *location, const char *type,
                            const nsModuleComponentInfo *info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catMan (do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
  NS_ENSURE_TRUE(catMan, NS_ERROR_FAILURE);

  rv = catMan->AddCategoryEntry("command-line-handler", "m-layoutdebug",
                                "@mozilla.org/commandlinehandler/general-startup;1?type=layoutdebug",
                                PR_TRUE, PR_TRUE, nsnull);
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

static NS_IMETHODIMP
UnregisterCommandLineHandlers(nsIComponentManager* compMgr, nsIFile *path,
                              const char *location,
                              const nsModuleComponentInfo *info)
{
  nsCOMPtr<nsICategoryManager> catMan (do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
  NS_ENSURE_TRUE(catMan, NS_ERROR_FAILURE);

  catMan->DeleteCategoryEntry("command-line-handler", "m-layoutdebug",
                              PR_TRUE);

  return NS_OK;
}

#endif

static const nsModuleComponentInfo components[] =
{
  { "nsRegressionTester",
    NS_REGRESSION_TESTER_CID,
    "@mozilla.org/layout-debug/regressiontester;1",
    nsRegressionTesterConstructor 
  },
  { "nsLayoutDebuggingTools",
    NS_LAYOUT_DEBUGGINGTOOLS_CID,
    NS_LAYOUT_DEBUGGINGTOOLS_CONTRACTID,
    nsLayoutDebuggingToolsConstructor
  },
  { "LayoutDebug Startup Handler",
    NS_LAYOUTDEBUGCLH_CID,
    "@mozilla.org/commandlinehandler/general-startup;1?type=layoutdebug",
    nsLayoutDebugCLHConstructor,
#ifdef MOZ_XUL_APP
    RegisterCommandLineHandlers,
    UnregisterCommandLineHandlers
#else
    nsLayoutDebugCLH::RegisterProc,
    nsLayoutDebugCLH::UnregisterProc
#endif
  }
};

NS_IMPL_NSGETMODULE(nsLayoutDebugModule, components)
