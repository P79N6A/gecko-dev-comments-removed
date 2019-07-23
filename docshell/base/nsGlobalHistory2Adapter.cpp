




































#include "nsGlobalHistory2Adapter.h"

#include "nsDocShellCID.h"
#include "nsServiceManagerUtils.h"
#include "nsIComponentRegistrar.h"
#include "nsGlobalHistoryAdapter.h"
#include "nsIURI.h"
#include "nsString.h"
#include "nsNetUtil.h"




nsresult
nsGlobalHistory2Adapter::Create(nsISupports *aOuter,
                                REFNSIID aIID,
                                void **aResult)
{
  nsresult rv;

  if (aOuter) {
    rv = NS_ERROR_NO_AGGREGATION;
    return rv;
  }

  nsGlobalHistory2Adapter* adapter = new nsGlobalHistory2Adapter();
  if (!adapter) {
    rv = NS_ERROR_OUT_OF_MEMORY;
    return rv;
  }

  NS_ADDREF(adapter);
  rv = adapter->Init();
  if (NS_SUCCEEDED(rv)) {
    rv = adapter->QueryInterface(aIID, aResult);
  }
  NS_RELEASE(adapter);

  return rv;
}

nsresult
nsGlobalHistory2Adapter::RegisterSelf(nsIComponentManager* aCompMgr,
                                      nsIFile* aPath,
                                      const char* aLoaderStr,
                                      const char* aType,
                                      const nsModuleComponentInfo *aInfo)
{
  nsresult rv;
  PRBool registered;
  nsCOMPtr<nsIComponentRegistrar> compReg( do_QueryInterface(aCompMgr) );
  if (!compReg) {
    rv = NS_ERROR_UNEXPECTED;
    return rv;
  }

  rv = compReg->IsContractIDRegistered(NS_GLOBALHISTORY_CONTRACTID, &registered);
  if (NS_FAILED(rv)) return rv;

  
  
  if (registered) {
    rv = NS_OK;
    return rv;
  }

  return compReg->RegisterFactoryLocation(GetCID(),
                                          "nsGlobalHistory2Adapter",
                                          NS_GLOBALHISTORY_CONTRACTID,
                                          aPath, aLoaderStr, aType);
}                                     

nsGlobalHistory2Adapter::nsGlobalHistory2Adapter()
{ }

nsGlobalHistory2Adapter::~nsGlobalHistory2Adapter()
{ }

nsresult
nsGlobalHistory2Adapter::Init()
{
  nsresult rv;

  nsCOMPtr<nsIComponentRegistrar> compReg;
  rv = NS_GetComponentRegistrar(getter_AddRefs(compReg));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCID *cid;
  rv = compReg->ContractIDToCID(NS_GLOBALHISTORY2_CONTRACTID, &cid);
  if (NS_FAILED(rv)) {
    rv = NS_ERROR_FACTORY_NOT_REGISTERED;
    return rv;
  }

  if (cid->Equals(nsGlobalHistoryAdapter::GetCID())) {
    rv = NS_ERROR_FACTORY_NOT_REGISTERED;
    return rv;
  }

  NS_WARNING("Using nsIGlobalHistory->nsIGlobalHistory2 adapter.");
  mHistory = do_GetService(NS_GLOBALHISTORY2_CONTRACTID, &rv);
  return rv;
}

NS_IMPL_ISUPPORTS1(nsGlobalHistory2Adapter, nsIGlobalHistory)

NS_IMETHODIMP
nsGlobalHistory2Adapter::AddPage(const char* aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_TRUE(*aURI, NS_ERROR_ILLEGAL_VALUE);

  nsresult rv;

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), nsDependentCString(aURI));

  if (NS_SUCCEEDED(rv)) {
    rv = mHistory->AddURI(uri, PR_FALSE, PR_FALSE, nsnull);
  }

  return rv;
}

NS_IMETHODIMP
nsGlobalHistory2Adapter::IsVisited(const char* aURI, PRBool* aRetval)
{
  NS_ENSURE_ARG_POINTER(aURI);
  nsresult rv;

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), nsDependentCString(aURI));

  if (NS_SUCCEEDED(rv)) {
    rv = mHistory->IsVisited(uri, aRetval);
  }

  return rv;
}
