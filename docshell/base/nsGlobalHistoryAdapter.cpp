




































#include "nsGlobalHistoryAdapter.h"

#include "nsDocShellCID.h"
#include "nsServiceManagerUtils.h"
#include "nsIComponentRegistrar.h"
#include "nsGlobalHistory2Adapter.h"
#include "nsIURI.h"
#include "nsString.h"

nsresult
nsGlobalHistoryAdapter::Create(nsISupports *aOuter,
                               REFNSIID aIID,
                               void **aResult)
{
  nsresult rv;

  if (aOuter) {
    rv = NS_ERROR_NO_AGGREGATION;
    return rv;
  }

  nsGlobalHistoryAdapter* adapter = new nsGlobalHistoryAdapter();
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
nsGlobalHistoryAdapter::RegisterSelf(nsIComponentManager* aCompMgr,
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

  rv = compReg->IsContractIDRegistered(NS_GLOBALHISTORY2_CONTRACTID, &registered);
  if (NS_FAILED(rv)) return rv;

  
  
  if (registered) {
    rv = NS_OK;
    return rv;
  }

  return compReg->RegisterFactoryLocation(GetCID(),
                                          "nsGlobalHistoryAdapter",
                                          NS_GLOBALHISTORY2_CONTRACTID,
                                          aPath, aLoaderStr, aType);
}                                     

nsGlobalHistoryAdapter::nsGlobalHistoryAdapter()
{ }

nsGlobalHistoryAdapter::~nsGlobalHistoryAdapter()
{ }

nsresult
nsGlobalHistoryAdapter::Init()
{
  nsresult rv;

  nsCOMPtr<nsIComponentRegistrar> compReg;
  rv = NS_GetComponentRegistrar(getter_AddRefs(compReg));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCID *cid;
  rv = compReg->ContractIDToCID(NS_GLOBALHISTORY_CONTRACTID, &cid);
  if (NS_FAILED(rv)) {
    rv = NS_ERROR_FACTORY_NOT_REGISTERED;
    return rv;
  }

  if (cid->Equals(nsGlobalHistory2Adapter::GetCID())) {
    rv = NS_ERROR_FACTORY_NOT_REGISTERED;
    return rv;
  }

  NS_WARNING("Using nsIGlobalHistory2->nsIGlobalHistory adapter.");
  mHistory = do_GetService(NS_GLOBALHISTORY_CONTRACTID, &rv);
  return rv;
}

NS_IMPL_ISUPPORTS1(nsGlobalHistoryAdapter, nsIGlobalHistory2)

NS_IMETHODIMP
nsGlobalHistoryAdapter::AddURI(nsIURI* aURI, PRBool aRedirect,
                               PRBool aToplevel, nsIURI* aReferrer)
{
  NS_ENSURE_ARG_POINTER(aURI);
  nsresult rv;

  
  
  
  
  

  PRBool isHTTP = PR_FALSE;
  PRBool isHTTPS = PR_FALSE;

  NS_ENSURE_SUCCESS(rv = aURI->SchemeIs("http", &isHTTP), rv);
  NS_ENSURE_SUCCESS(rv = aURI->SchemeIs("https", &isHTTPS), rv);

  if (!isHTTP && !isHTTPS) {
    PRBool isAbout, isImap, isNews, isMailbox, isViewSource, isChrome, isData;

    rv = aURI->SchemeIs("about", &isAbout);
    rv |= aURI->SchemeIs("imap", &isImap);
    rv |= aURI->SchemeIs("news", &isNews);
    rv |= aURI->SchemeIs("mailbox", &isMailbox);
    rv |= aURI->SchemeIs("view-source", &isViewSource);
    rv |= aURI->SchemeIs("chrome", &isChrome);
    rv |= aURI->SchemeIs("data", &isData);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    if (isAbout || isImap || isNews || isMailbox || isViewSource || isChrome || isData) {
      return NS_OK;
    }
  }

  nsCAutoString spec;
  rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  return mHistory->AddPage(spec.get());
}

NS_IMETHODIMP
nsGlobalHistoryAdapter::IsVisited(nsIURI* aURI, PRBool* aRetval)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsCAutoString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  return mHistory->IsVisited(spec.get(), aRetval);
}

NS_IMETHODIMP
nsGlobalHistoryAdapter::SetPageTitle(nsIURI* aURI, const nsAString& aTitle)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
