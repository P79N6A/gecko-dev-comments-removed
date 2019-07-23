





































#include "nsIModule.h"
#include "nsIGenericFactory.h"
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


static const nsModuleComponentInfo components[] = {
    { "PermissionManager",
      NS_PERMISSIONMANAGER_CID,
      NS_PERMISSIONMANAGER_CONTRACTID,
      nsPermissionManagerConstructor
    },
    { "PopupWindowManager",
      NS_POPUPWINDOWMANAGER_CID,
      NS_POPUPWINDOWMANAGER_CONTRACTID,
      nsPopupWindowManagerConstructor
    },
    { "CookiePromptService",
      NS_COOKIEPROMPTSERVICE_CID,
      NS_COOKIEPROMPTSERVICE_CONTRACTID,
      nsCookiePromptServiceConstructor
    },
    { "CookiePermission",
      NS_COOKIEPERMISSION_CID,
      NS_COOKIEPERMISSION_CONTRACTID,
      nsCookiePermissionConstructor
    }
};

NS_IMPL_NSGETMODULE(nsCookieModule, components)
