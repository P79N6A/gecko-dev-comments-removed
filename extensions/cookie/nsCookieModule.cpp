





#include "mozilla/ModuleUtils.h"
#include "nsIServiceManager.h"
#include "nsPermissionManager.h"
#include "nsPopupWindowManager.h"
#include "nsICategoryManager.h"
#include "nsCookiePromptService.h"
#include "nsCookiePermission.h"
#include "nsXPIDLString.h"


NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsIPermissionManager,
  nsPermissionManager::GetXPCOMSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPopupWindowManager, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCookiePermission)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCookiePromptService)

NS_DEFINE_NAMED_CID(NS_PERMISSIONMANAGER_CID);
NS_DEFINE_NAMED_CID(NS_POPUPWINDOWMANAGER_CID);
NS_DEFINE_NAMED_CID(NS_COOKIEPROMPTSERVICE_CID);
NS_DEFINE_NAMED_CID(NS_COOKIEPERMISSION_CID);


static const mozilla::Module::CIDEntry kCookieCIDs[] = {
    { &kNS_PERMISSIONMANAGER_CID, false, nullptr, nsIPermissionManagerConstructor },
    { &kNS_POPUPWINDOWMANAGER_CID, false, nullptr, nsPopupWindowManagerConstructor },
    { &kNS_COOKIEPROMPTSERVICE_CID, false, nullptr, nsCookiePromptServiceConstructor },
    { &kNS_COOKIEPERMISSION_CID, false, nullptr, nsCookiePermissionConstructor },
    { nullptr }
};

static const mozilla::Module::ContractIDEntry kCookieContracts[] = {
    { NS_PERMISSIONMANAGER_CONTRACTID, &kNS_PERMISSIONMANAGER_CID },
    { NS_POPUPWINDOWMANAGER_CONTRACTID, &kNS_POPUPWINDOWMANAGER_CID },
    { NS_COOKIEPROMPTSERVICE_CONTRACTID, &kNS_COOKIEPROMPTSERVICE_CID },
    { NS_COOKIEPERMISSION_CONTRACTID, &kNS_COOKIEPERMISSION_CID },
    { nullptr }
};

static const mozilla::Module kCookieModule = {
    mozilla::Module::kVersion,
    kCookieCIDs,
    kCookieContracts
};

NSMODULE_DEFN(nsCookieModule) = &kCookieModule;
