




































#include "nsIFactory.h"
#include "nsIComponentManager.h"
#include "nscore.h"
#include "nsIComponentManager.h"
#include "nsIWindowMediator.h"
#include "nsAbout.h"
#include "nsIGenericFactory.h"

#include "nsIAppShellService.h"
#include "nsAppShellService.h"
#include "nsWindowMediator.h"
#include "nsChromeTreeOwner.h"
#include "nsAppShellCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsAppShellService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindowMediator)

static const nsModuleComponentInfo gAppShellModuleInfo[] =
{
  { "AppShell Service",
    NS_APPSHELLSERVICE_CID,
    NS_APPSHELLSERVICE_CONTRACTID,
    nsAppShellServiceConstructor,
  },
  { "Window Mediator",
    NS_WINDOWMEDIATOR_CID,
    NS_WINDOWMEDIATOR_CONTRACTID,
    nsWindowMediatorConstructor,
  },
  { "kAboutModuleCID",
    NS_ABOUT_CID,
    NS_ABOUT_MODULE_CONTRACTID_PREFIX,
    nsAbout::Create,
  }
};

PR_STATIC_CALLBACK(nsresult)
nsAppShellModuleConstructor(nsIModule *aModule)
{
  return nsChromeTreeOwner::InitGlobals();
}

PR_STATIC_CALLBACK(void)
nsAppShellModuleDestructor(nsIModule *aModule)
{
  nsChromeTreeOwner::FreeGlobals();
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(appshell, gAppShellModuleInfo,
                                   nsAppShellModuleConstructor,
                                   nsAppShellModuleDestructor)
