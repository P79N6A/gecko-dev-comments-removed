




































#include "mozMySpellDirProvider.h"
#include "nsXULAppAPI.h"
#include "nsString.h"

#include "mozISpellCheckingEngine.h"
#include "nsICategoryManager.h"

NS_IMPL_ISUPPORTS2(mozMySpellDirProvider,
		   nsIDirectoryServiceProvider,
		   nsIDirectoryServiceProvider2)

NS_IMETHODIMP
mozMySpellDirProvider::GetFile(const char *aKey, PRBool *aPersist,
			       nsIFile* *aResult)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
mozMySpellDirProvider::GetFiles(const char *aKey,
				nsISimpleEnumerator* *aResult)
{
  if (strcmp(aKey, DICTIONARY_SEARCH_DIRECTORY_LIST) != 0) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIProperties> dirSvc =
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID);
  if (!dirSvc)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISimpleEnumerator> list;
  nsresult rv = dirSvc->Get(XRE_EXTENSIONS_DIR_LIST,
			    NS_GET_IID(nsISimpleEnumerator),
			    getter_AddRefs(list));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsISimpleEnumerator> e = new AppendingEnumerator(list);
  if (!e)
    return NS_ERROR_OUT_OF_MEMORY;

  *aResult = nsnull;
  e.swap(*aResult);
  return NS_SUCCESS_AGGREGATE_RESULT;
}

NS_IMPL_ISUPPORTS1(mozMySpellDirProvider::AppendingEnumerator,
		   nsISimpleEnumerator)

NS_IMETHODIMP
mozMySpellDirProvider::AppendingEnumerator::HasMoreElements(PRBool *aResult)
{
  *aResult = mNext ? PR_TRUE : PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
mozMySpellDirProvider::AppendingEnumerator::GetNext(nsISupports* *aResult)
{
  if (aResult)
    NS_ADDREF(*aResult = mNext);

  mNext = nsnull;

  nsresult rv;

  

  PRBool more;
  while (NS_SUCCEEDED(mBase->HasMoreElements(&more)) && more) {
    nsCOMPtr<nsISupports> nextbasesupp;
    mBase->GetNext(getter_AddRefs(nextbasesupp));

    nsCOMPtr<nsIFile> nextbase(do_QueryInterface(nextbasesupp));
    if (!nextbase)
      continue;

    nextbase->Clone(getter_AddRefs(mNext));
    if (!mNext)
      continue;

    mNext->AppendNative(NS_LITERAL_CSTRING("dictionaries"));

    PRBool exists;
    rv = mNext->Exists(&exists);
    if (NS_SUCCEEDED(rv) && exists)
      break;

    mNext = nsnull;
  }

  return NS_OK;
}

mozMySpellDirProvider::AppendingEnumerator::AppendingEnumerator
    (nsISimpleEnumerator* aBase) :
  mBase(aBase)
{
  
  GetNext(nsnull);
}

NS_METHOD
mozMySpellDirProvider::Register(nsIComponentManager* aCompMgr,
				nsIFile* aPath, const char *aLoaderStr,
				const char *aType,
				const nsModuleComponentInfo *aInfo)
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catMan =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
  if (!catMan)
    return NS_ERROR_FAILURE;

  rv = catMan->AddCategoryEntry(XPCOM_DIRECTORY_PROVIDER_CATEGORY,
				"spellcheck-directory-provider",
				kContractID, PR_TRUE, PR_TRUE, nsnull);
  return rv;
}

NS_METHOD
mozMySpellDirProvider::Unregister(nsIComponentManager* aCompMgr,
				  nsIFile* aPath,
				  const char *aLoaderStr,
				  const nsModuleComponentInfo *aInfo)
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catMan =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
  if (!catMan)
    return NS_ERROR_FAILURE;

  rv = catMan->DeleteCategoryEntry(XPCOM_DIRECTORY_PROVIDER_CATEGORY,
				   "spellcheck-directory-provider",
				   PR_TRUE);
  return rv;
}

char const *const
mozMySpellDirProvider::kContractID = "@mozilla.org/spellcheck/dir-provider;1";
