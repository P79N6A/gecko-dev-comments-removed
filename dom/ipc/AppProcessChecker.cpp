






#include "AppProcessChecker.h"
#include "ContentParent.h"
#include "mozIApplication.h"
#include "mozilla/hal_sandbox/PHalParent.h"
#include "nsIDOMApplicationRegistry.h"
#include "TabParent.h"

using namespace mozilla::dom;
using namespace mozilla::hal_sandbox;
using namespace mozilla::services;

namespace mozilla {

bool
CheckAppHasPermission(mozIApplication *app, const char* aCapability)
{
  nsresult rv;
  nsCOMPtr<nsIScriptSecurityManager> securityManager =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    return false;
  }

  nsCOMPtr<nsIPrincipal> principal;

  uint32_t appId;
  app->GetLocalId(&appId);
  nsString appOrigin;
  rv = app->GetOrigin(appOrigin);
  if (NS_FAILED(rv)) {
    return false;
  }

  nsCOMPtr<nsIURI> uri;
  NS_NewURI(getter_AddRefs(uri), appOrigin);
  rv = securityManager->GetAppCodebasePrincipal(uri, appId, false, getter_AddRefs(principal));
  if (NS_FAILED(rv)) {
    return false;
  }

  nsCOMPtr<nsIPermissionManager> pm = do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  uint32_t permission = nsIPermissionManager::UNKNOWN_ACTION;
  rv = pm->TestPermissionFromPrincipal(principal, aCapability, &permission);

  return NS_SUCCEEDED(rv) && (permission == nsIPermissionManager::ALLOW_ACTION ||
                              permission == nsIPermissionManager::PROMPT_ACTION);
}

bool
AssertAppProcess(PBrowserParent* aActor,
                 AssertAppProcessType aType,
                 const char* aCapability)
{
  if (!aActor) {
    NS_WARNING("Testing process capability for null actor");
    return false;
  }

  TabParent* tab = static_cast<TabParent*>(aActor);
  nsCOMPtr<mozIApplication> app = tab->GetOwnOrContainingApp();
  bool aValid = false;

  
  
  
  if (app && !tab->IsBrowserElement()) {
    switch (aType) {
      case ASSERT_APP_PROCESS_PERMISSION:
        if (!NS_SUCCEEDED(app->HasPermission(aCapability, &aValid))) {
          aValid = false;
        }
        break;
      case ASSERT_APP_PROCESS_MANIFEST_URL: {
        nsAutoString manifestURL;
        if (NS_SUCCEEDED(app->GetManifestURL(manifestURL)) &&
            manifestURL.EqualsASCII(aCapability)) {
          aValid = true;
        }
        break;
      }
      default:
        break;
    }
  } else {
    aValid = CheckAppHasPermission(app, aCapability);
  }

  if (!aValid) {
    printf_stderr("Security problem: Content process does not have `%s'.  It will be killed.\n", aCapability);
    ContentParent* process = static_cast<ContentParent*>(aActor->Manager());
    process->KillHard();
  }
  return aValid;
}

bool
AssertAppProcess(PContentParent* aActor,
                 AssertAppProcessType aType,
                 const char* aCapability)
{
  const InfallibleTArray<PBrowserParent*>& browsers =
    aActor->ManagedPBrowserParent();
  for (uint32_t i = 0; i < browsers.Length(); ++i) {
    if (AssertAppProcess(browsers[i], aType, aCapability)) {
      return true;
    }
  }
  return false;
}

bool
AssertAppProcess(PHalParent* aActor,
                 AssertAppProcessType aType,
                 const char* aCapability)
{
  return AssertAppProcess(aActor->Manager(), aType, aCapability);
}

} 
