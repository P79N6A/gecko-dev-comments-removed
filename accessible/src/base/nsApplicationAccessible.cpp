








































 
#include "nsApplicationAccessible.h"

#include "nsIComponentManager.h"
#include "nsServiceManagerUtils.h"

nsApplicationAccessible::nsApplicationAccessible():
    nsAccessibleWrap(nsnull, nsnull), mChildren(nsnull)
{
}


NS_IMPL_ISUPPORTS_INHERITED0(nsApplicationAccessible,
                             nsAccessible)


NS_IMETHODIMP
nsApplicationAccessible::Init()
{
  nsresult rv;
  mChildren = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
  return rv;
}



NS_IMETHODIMP
nsApplicationAccessible::GetName(nsAString& aName)
{
  nsCOMPtr<nsIStringBundleService> bundleService =
    do_GetService(NS_STRINGBUNDLE_CONTRACTID);

  NS_ASSERTION(bundleService, "String bundle service must be present!");
  NS_ENSURE_STATE(bundleService);

  nsCOMPtr<nsIStringBundle> bundle;
  bundleService->CreateBundle("chrome://branding/locale/brand.properties",
                              getter_AddRefs(bundle));

  nsXPIDLString appName;
  if (bundle) {
    bundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(),
                              getter_Copies(appName));
  } else {
    NS_WARNING("brand.properties not present, using default app name");
    appName.AssignLiteral("Mozilla");
  }

  aName.Assign(appName);
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetDescription(nsAString& aDescription)
{
  GetName(aDescription);
  aDescription.AppendLiteral(" Application Accessible");
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetRole(PRUint32 *aRole)
{
  *aRole = nsIAccessibleRole::ROLE_APP_ROOT;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetFinalRole(PRUint32 *aFinalRole)
{
  return GetRole(aFinalRole);
}

NS_IMETHODIMP
nsApplicationAccessible::GetState(PRUint32 *aState, PRUint32 *aExtraState)
{
  *aState = 0;
  if (aExtraState)
    *aExtraState = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetParent(nsIAccessible **aParent)
{
  *aParent = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetChildAt(PRInt32 aChildNum, nsIAccessible **aChild)
{
  NS_ENSURE_ARG_POINTER(aChild);
  *aChild = nsnull;

  PRUint32 count = 0;
  nsresult rv = NS_OK;

  if (mChildren) {
    rv = mChildren->GetLength(&count);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aChildNum >= static_cast<PRInt32>(count) || count == 0)
    return NS_ERROR_INVALID_ARG;

  if (aChildNum < 0)
    aChildNum = count - 1;

  nsCOMPtr<nsIWeakReference> childWeakRef;
  rv = mChildren->QueryElementAt(aChildNum, NS_GET_IID(nsIWeakReference),
                                 getter_AddRefs(childWeakRef));
  NS_ENSURE_SUCCESS(rv, rv);

  if (childWeakRef) {
    nsCOMPtr<nsIAccessible> childAcc(do_QueryReferent(childWeakRef));
    NS_IF_ADDREF(*aChild = childAcc);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetNextSibling(nsIAccessible **aNextSibling)
{
  NS_ENSURE_ARG_POINTER(aNextSibling);

  *aNextSibling = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsApplicationAccessible::GetPreviousSibling(nsIAccessible **aPreviousSibling)
{
  NS_ENSURE_ARG_POINTER(aPreviousSibling);

  *aPreviousSibling = nsnull;
  return NS_OK;
}

void
nsApplicationAccessible::CacheChildren()
{
  if (!mChildren) {
    mAccChildCount = eChildCountUninitialized;
    return;
  }

  if (mAccChildCount == eChildCountUninitialized) {
    nsCOMPtr<nsISimpleEnumerator> enumerator;
    mChildren->Enumerate(getter_AddRefs(enumerator));

    nsCOMPtr<nsIWeakReference> childWeakRef;
    nsCOMPtr<nsIAccessible> accessible;
    nsCOMPtr<nsPIAccessible> previousAccessible;
    PRBool hasMoreElements;
    while(NS_SUCCEEDED(enumerator->HasMoreElements(&hasMoreElements))
          && hasMoreElements) {
      enumerator->GetNext(getter_AddRefs(childWeakRef));
      accessible = do_QueryReferent(childWeakRef);
      if (accessible) {
        if (previousAccessible)
          previousAccessible->SetNextSibling(accessible);
        else
          SetFirstChild(accessible);

        previousAccessible = do_QueryInterface(accessible);
        previousAccessible->SetParent(this);
      }
    }

    PRUint32 count = 0;
    mChildren->GetLength(&count);
    mAccChildCount = static_cast<PRInt32>(count);
  }
}



nsresult
nsApplicationAccessible::AddRootAccessible(nsIAccessible *aRootAccessible)
{
  NS_ENSURE_ARG_POINTER(aRootAccessible);

  
  nsresult rv = mChildren->AppendElement(aRootAccessible, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  InvalidateChildren();
  return NS_OK;
}

nsresult
nsApplicationAccessible::RemoveRootAccessible(nsIAccessible *aRootAccessible)
{
  NS_ENSURE_ARG_POINTER(aRootAccessible);

  PRUint32 index = 0;

  
  nsCOMPtr<nsIWeakReference> weakPtr = do_GetWeakReference(aRootAccessible);
  nsresult rv = mChildren->IndexOf(0, weakPtr, &index);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mChildren->RemoveElementAt(index);
  NS_ENSURE_SUCCESS(rv, rv);

  InvalidateChildren();
  return NS_OK;
}

