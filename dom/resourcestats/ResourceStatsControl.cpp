



#include "ResourceStatsControl.h"
#include "mozilla/Preferences.h"
#include "nsIPermissionManager.h"
#include "nsJSUtils.h"
#include "nsPIDOMWindow.h"
#include "nsServiceManagerUtils.h"

using namespace mozilla::dom;

 bool
ResourceStatsControl::HasResourceStatsSupport(JSContext* ,
                                              JSObject* aGlobal)
{
  
  if (!Preferences::GetBool("dom.resource_stats.enabled", false)) {
    return false;
  }

  
  nsCOMPtr<nsPIDOMWindow> win =
    do_QueryInterface((nsISupports*)nsJSUtils::GetStaticScriptGlobal(aGlobal));
  MOZ_ASSERT(!win || win->IsInnerWindow());
  if (!win) {
    return false;
  }

  
  nsCOMPtr<nsIPermissionManager> permMgr =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(permMgr, false);

  uint32_t permission = nsIPermissionManager::DENY_ACTION;
  permMgr->TestPermissionFromWindow(win, "resourcestats-manage", &permission);
  return permission == nsIPermissionManager::ALLOW_ACTION;
}
