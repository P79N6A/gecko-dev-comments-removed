



#include "mozilla/ModuleUtils.h"
#include "nsIServiceManager.h"
#include "nsContentBlocker.h"
#include "nsXPIDLString.h"


NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsContentBlocker, Init)

NS_DEFINE_NAMED_CID(NS_CONTENTBLOCKER_CID);

static const mozilla::Module::CIDEntry kPermissionsCIDs[] = {
  { &kNS_CONTENTBLOCKER_CID, false, nullptr, nsContentBlockerConstructor },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kPermissionsContracts[] = {
  { NS_CONTENTBLOCKER_CONTRACTID, &kNS_CONTENTBLOCKER_CID },
  { nullptr }
};

static const mozilla::Module::CategoryEntry kPermissionsCategories[] = {
  { "content-policy", NS_CONTENTBLOCKER_CONTRACTID, NS_CONTENTBLOCKER_CONTRACTID },
  { nullptr }
};

static const mozilla::Module kPermissionsModule = {
  mozilla::Module::kVersion,
  kPermissionsCIDs,
  kPermissionsContracts,
  kPermissionsCategories
};

NSMODULE_DEFN(nsPermissionsModule) = &kPermissionsModule;
