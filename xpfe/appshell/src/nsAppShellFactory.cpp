




































#include "mozilla/ModuleUtils.h"
#include "nscore.h"
#include "nsIWindowMediator.h"
#include "nsAbout.h"

#include "nsIAppShellService.h"
#include "nsAppShellService.h"
#include "nsWindowMediator.h"
#include "nsChromeTreeOwner.h"
#include "nsAppShellCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsAppShellService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsAbout)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsWindowMediator, Init)

NS_DEFINE_NAMED_CID(NS_APPSHELLSERVICE_CID);
NS_DEFINE_NAMED_CID(NS_WINDOWMEDIATOR_CID);
NS_DEFINE_NAMED_CID(NS_ABOUT_CID);

static const mozilla::Module::CIDEntry kAppShellCIDs[] = {
  { &kNS_APPSHELLSERVICE_CID, false, NULL, nsAppShellServiceConstructor },
  { &kNS_WINDOWMEDIATOR_CID, false, NULL, nsWindowMediatorConstructor },
  { &kNS_ABOUT_CID, false, NULL, nsAboutConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kAppShellContracts[] = {
  { NS_APPSHELLSERVICE_CONTRACTID, &kNS_APPSHELLSERVICE_CID },
  { NS_WINDOWMEDIATOR_CONTRACTID, &kNS_WINDOWMEDIATOR_CID },
  { NS_ABOUT_MODULE_CONTRACTID_PREFIX, &kNS_ABOUT_CID },
  { NULL }
};

static nsresult
nsAppShellModuleConstructor()
{
  return nsChromeTreeOwner::InitGlobals();
}

static void
nsAppShellModuleDestructor()
{
  nsChromeTreeOwner::FreeGlobals();
}

static const mozilla::Module kAppShellModule = {
  mozilla::Module::kVersion,
  kAppShellCIDs,
  kAppShellContracts,
  NULL,
  NULL,
  nsAppShellModuleConstructor,
  nsAppShellModuleDestructor
};

NSMODULE_DEFN(appshell) = &kAppShellModule;
