








































 
#include "nsApplicationAccessible.h"

#include "nsAccessibilityService.h"

#include "nsIComponentManager.h"
#include "nsServiceManagerUtils.h"

nsApplicationAccessible::nsApplicationAccessible() :
  nsAccessibleWrap(nsnull, nsnull)
{
}




NS_IMPL_ISUPPORTS_INHERITED1(nsApplicationAccessible, nsAccessible,
                             nsIAccessibleApplication)




NS_IMETHODIMP
nsApplicationAccessible::GetName(nsAString& aName)
{
  aName.Truncate();

  nsCOMPtr<nsIStringBundleService> bundleService =
    do_GetService(NS_STRINGBUNDLE_CONTRACTID);

  NS_ASSERTION(bundleService, "String bundle service must be present!");
  NS_ENSURE_STATE(bundleService);

  nsCOMPtr<nsIStringBundle> bundle;
  nsresult rv = bundleService->CreateBundle("chrome://branding/locale/brand.properties",
                                            getter_AddRefs(bundle));
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLString appName;
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(),
                                 getter_Copies(appName));
  if (NS_FAILED(rv) || appName.IsEmpty()) {
    NS_WARNING("brandShortName not found, using default app name");
    appName.AssignLiteral("Gecko based application");
  }

  aName.Assign(appName);
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetDescription(nsAString& aValue)
{
  aValue.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetRole(PRUint32 *aRole)
{
  NS_ENSURE_ARG_POINTER(aRole);

  return GetRoleInternal(aRole);
}

NS_IMETHODIMP
nsApplicationAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  NS_ENSURE_ARG_POINTER(aState);
  *aState = 0;

  if (aExtraState)
    *aExtraState = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetParent(nsIAccessible **aAccessible)
{
  NS_ENSURE_ARG_POINTER(aAccessible);
  *aAccessible = nsnull;

  return IsDefunct() ? NS_ERROR_FAILURE : NS_OK;
}




NS_IMETHODIMP
nsApplicationAccessible::GetAppName(nsAString& aName)
{
  aName.Truncate();

  if (!mAppInfo)
    return NS_ERROR_FAILURE;

  nsCAutoString cname;
  nsresult rv = mAppInfo->GetName(cname);
  NS_ENSURE_SUCCESS(rv, rv);

  AppendUTF8toUTF16(cname, aName);
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetAppVersion(nsAString& aVersion)
{
  aVersion.Truncate();

  if (!mAppInfo)
    return NS_ERROR_FAILURE;

  nsCAutoString cversion;
  nsresult rv = mAppInfo->GetVersion(cversion);
  NS_ENSURE_SUCCESS(rv, rv);

  AppendUTF8toUTF16(cversion, aVersion);
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetPlatformName(nsAString& aName)
{
  aName.AssignLiteral("Gecko");
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetPlatformVersion(nsAString& aVersion)
{
  aVersion.Truncate();

  if (!mAppInfo)
    return NS_ERROR_FAILURE;

  nsCAutoString cversion;
  nsresult rv = mAppInfo->GetPlatformVersion(cversion);
  NS_ENSURE_SUCCESS(rv, rv);

  AppendUTF8toUTF16(cversion, aVersion);
  return NS_OK;
}




PRBool
nsApplicationAccessible::IsDefunct()
{
  return nsAccessibilityService::gIsShutdown;
}

nsresult
nsApplicationAccessible::Init()
{
  mAppInfo = do_GetService("@mozilla.org/xre/app-info;1");
  return NS_OK;
}

nsresult
nsApplicationAccessible::Shutdown()
{
  mAppInfo = nsnull;
  return NS_OK;
}




nsresult
nsApplicationAccessible::GetRoleInternal(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_APP_ROOT;
  return NS_OK;
}

nsresult
nsApplicationAccessible::GetStateInternal(PRUint32 *aState,
                                          PRUint32 *aExtraState)
{
  *aState = 0;
  if (aExtraState)
    *aExtraState = 0;

  return NS_OK;
}

nsAccessible*
nsApplicationAccessible::GetParent()
{
  return nsnull;
}

void
nsApplicationAccessible::InvalidateChildren()
{
  
  
}




void
nsApplicationAccessible::CacheChildren()
{
  
  
}

nsAccessible*
nsApplicationAccessible::GetSiblingAtOffset(PRInt32 aOffset, nsresult* aError)
{
  if (IsDefunct()) {
    if (aError)
      *aError = NS_ERROR_FAILURE;

    return nsnull;
  }

  if (aError)
    *aError = NS_OK; 

  return nsnull;
}




nsresult
nsApplicationAccessible::AddRootAccessible(nsIAccessible *aRootAccessible)
{
  NS_ENSURE_ARG_POINTER(aRootAccessible);

  nsRefPtr<nsAccessible> rootAcc =
    nsAccUtils::QueryObject<nsAccessible>(aRootAccessible);

  if (!mChildren.AppendElement(rootAcc))
    return NS_ERROR_FAILURE;

  rootAcc->SetParent(this);

  return NS_OK;
}

nsresult
nsApplicationAccessible::RemoveRootAccessible(nsIAccessible *aRootAccessible)
{
  NS_ENSURE_ARG_POINTER(aRootAccessible);

  
  
  
  return mChildren.RemoveElement(aRootAccessible) ? NS_OK : NS_ERROR_FAILURE;
}
