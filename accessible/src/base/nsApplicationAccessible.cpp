








































 
#include "nsApplicationAccessible.h"

#include "nsAccessibilityService.h"

#include "nsIComponentManager.h"
#include "nsServiceManagerUtils.h"

nsApplicationAccessible::nsApplicationAccessible() :
  nsAccessibleWrap(nsnull, nsnull)
{
}




NS_IMPL_ISUPPORTS_INHERITED0(nsApplicationAccessible, nsAccessible)




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




PRBool
nsApplicationAccessible::IsDefunct()
{
  return nsAccessibilityService::gIsShutdown;
}

nsresult
nsApplicationAccessible::Init()
{
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

nsIAccessible*
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

nsIAccessible*
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

  if (!mChildren.AppendObject(aRootAccessible))
    return NS_ERROR_FAILURE;

  nsRefPtr<nsAccessible> rootAcc = nsAccUtils::QueryAccessible(aRootAccessible);
  rootAcc->SetParent(this);

  return NS_OK;
}

nsresult
nsApplicationAccessible::RemoveRootAccessible(nsIAccessible *aRootAccessible)
{
  NS_ENSURE_ARG_POINTER(aRootAccessible);

  
  
  
  return mChildren.RemoveObject(aRootAccessible) ? NS_OK : NS_ERROR_FAILURE;
}
