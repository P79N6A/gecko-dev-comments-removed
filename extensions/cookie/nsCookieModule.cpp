





































#include "mozilla/ModuleUtils.h"
#include "nsIServiceManager.h"
#include "nsPermissionManager.h"
#include "nsPopupWindowManager.h"
#include "nsICategoryManager.h"
#include "nsCookiePromptService.h"
#include "nsCookiePermission.h"
#include "nsXPIDLString.h"


NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPermissionManager, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPopupWindowManager, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsCookiePermission, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCookiePromptService)

NS_DEFINE_NAMED_CID(NS_PERMISSIONMANAGER_CID);
NS_DEFINE_NAMED_CID(NS_POPUPWINDOWMANAGER_CID);
NS_DEFINE_NAMED_CID(NS_COOKIEPROMPTSERVICE_CID);
NS_DEFINE_NAMED_CID(NS_COOKIEPERMISSION_CID);


static const mozilla::Module::CIDEntry kCookieCIDs[] = {
    { &kNS_PERMISSIONMANAGER_CID, false, NULL, nsPermissionManagerConstructor },
    { &kNS_POPUPWINDOWMANAGER_CID, false, NULL, nsPopupWindowManagerConstructor },
    { &kNS_COOKIEPROMPTSERVICE_CID, false, NULL, nsCookiePromptServiceConstructor },
    { &kNS_COOKIEPERMISSION_CID, false, NULL, nsCookiePermissionConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kCookieContracts[] = {
    { NS_PERMISSIONMANAGER_CONTRACTID, &kNS_PERMISSIONMANAGER_CID },
    { NS_POPUPWINDOWMANAGER_CONTRACTID, &kNS_POPUPWINDOWMANAGER_CID },
    { NS_COOKIEPROMPTSERVICE_CONTRACTID, &kNS_COOKIEPROMPTSERVICE_CID },
    { NS_COOKIEPERMISSION_CONTRACTID, &kNS_COOKIEPERMISSION_CID },
    { NULL }
};

static const mozilla::Module kCookieModule = {
    mozilla::Module::kVersion,
    kCookieCIDs,
    kCookieContracts
};

NSMODULE_DEFN(nsCookieModule) = &kCookieModule;
