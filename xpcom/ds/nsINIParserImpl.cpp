




































#include "nsINIParserImpl.h"

#include "nsILocalFile.h"

#include "nsINIParser.h"
#include "nsStringEnumerator.h"
#include "nsVoidArray.h"

NS_IMETHODIMP
nsINIParserFactory::CreateINIParser(nsILocalFile* aINIFile,
                                    nsIINIParser* *aResult)
{
  *aResult = nsnull;

  nsCOMPtr<nsINIParserImpl> p(new nsINIParserImpl());
  if (!p)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = p->Init(aINIFile);

  if (NS_SUCCEEDED(rv))
    NS_ADDREF(*aResult = p);

  return rv;
}

NS_IMETHODIMP
nsINIParserFactory::CreateInstance(nsISupports* aOuter,
                                   REFNSIID aIID,
                                   void **aResult)
{
  NS_ENSURE_NO_AGGREGATION(aOuter);

  
  return QueryInterface(aIID, aResult);
}

NS_IMETHODIMP
nsINIParserFactory::LockFactory(PRBool aLock)
{
  return NS_OK;
}

static PRBool
SectionCB(const char* aSection, void *aClosure)
{
  nsCStringArray *strings = static_cast<nsCStringArray*>(aClosure);

  strings->AppendCString(nsDependentCString(aSection));
  return PR_TRUE;
}

NS_IMETHODIMP
nsINIParserImpl::GetSections(nsIUTF8StringEnumerator* *aResult)
{
  nsCStringArray *strings = new nsCStringArray;
  if (!strings)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = mParser.GetSections(SectionCB, strings);
  if (NS_SUCCEEDED(rv))
    rv = NS_NewUTF8StringEnumerator(aResult, strings);

  if (NS_FAILED(rv))
    delete strings;

  return rv;
}

static PRBool
KeyCB(const char* aKey, const char *aValue, void *aClosure)
{
  nsCStringArray *strings = static_cast<nsCStringArray*>(aClosure);

  strings->AppendCString(nsDependentCString(aKey));
  return PR_TRUE;
}

NS_IMETHODIMP
nsINIParserImpl::GetKeys(const nsACString& aSection,
                         nsIUTF8StringEnumerator* *aResult)
{
  nsCStringArray *strings = new nsCStringArray;
  if (!strings)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = mParser.GetStrings(PromiseFlatCString(aSection).get(),
                                   KeyCB, strings);
  if (NS_SUCCEEDED(rv))
    rv = NS_NewUTF8StringEnumerator(aResult, strings);

  if (NS_FAILED(rv))
    delete strings;

  return rv;

}

NS_IMETHODIMP
nsINIParserImpl::GetString(const nsACString& aSection,
                           const nsACString& aKey,
                           nsACString& aResult)
{
  return mParser.GetString(PromiseFlatCString(aSection).get(),
                           PromiseFlatCString(aKey).get(),
                           aResult);
}
