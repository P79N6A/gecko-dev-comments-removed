






































#include "mozilla/ModuleUtils.h"

#include "nsShellService.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsShellService)
NS_DEFINE_NAMED_CID(nsShellService_CID);

static const mozilla::Module::CIDEntry kBrowserCIDs[] = {
  { &knsShellService_CID, false, NULL, nsShellServiceConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kBrowserContracts[] = {
  { nsShellService_ContractID, &knsShellService_CID },
  { NULL }
};

static const mozilla::Module kBrowserModule = {
  mozilla::Module::kVersion,
  kBrowserCIDs,
  kBrowserContracts
};

NSMODULE_DEFN(nsBrowserCompsModule) = &kBrowserModule;
