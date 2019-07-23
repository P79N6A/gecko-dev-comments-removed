




































#include "nsErrorService.h"
#include "nsCRT.h"

static void* PR_CALLBACK
CloneCString(nsHashKey *aKey, void *aData, void* closure)
{
  return nsCRT::strdup((const char*)aData);
}

static PRBool PR_CALLBACK
DeleteCString(nsHashKey *aKey, void *aData, void* closure)
{
  nsCRT::free((char*)aData);
  return PR_TRUE;
}

nsInt2StrHashtable::nsInt2StrHashtable()
    : mHashtable(CloneCString, nsnull, DeleteCString, nsnull, 16)
{
}

nsresult
nsInt2StrHashtable::Put(PRUint32 key, const char* aData)
{
  char* value = nsCRT::strdup(aData);
  if (value == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  nsPRUint32Key k(key);
  char* oldValue = (char*)mHashtable.Put(&k, value);
  if (oldValue)
    nsCRT::free(oldValue);
  return NS_OK;
}

char* 
nsInt2StrHashtable::Get(PRUint32 key)
{
  nsPRUint32Key k(key);
  const char* value = (const char*)mHashtable.Get(&k);
  if (value == nsnull)
    return nsnull;
  return nsCRT::strdup(value);
}

nsresult
nsInt2StrHashtable::Remove(PRUint32 key)
{
  nsPRUint32Key k(key);
  char* oldValue = (char*)mHashtable.Remove(&k);
  if (oldValue)
    nsCRT::free(oldValue);
  return NS_OK;
}



NS_IMPL_ISUPPORTS1(nsErrorService, nsIErrorService)

NS_METHOD
nsErrorService::Create(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
    NS_ENSURE_NO_AGGREGATION(outer);
    nsErrorService* serv = new nsErrorService();
    if (serv == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(serv);
    nsresult rv = serv->QueryInterface(aIID, aInstancePtr);
    NS_RELEASE(serv);
    return rv;
}

NS_IMETHODIMP
nsErrorService::RegisterErrorStringBundle(PRInt16 errorModule, const char *stringBundleURL)
{
    return mErrorStringBundleURLMap.Put(errorModule, stringBundleURL);
}

NS_IMETHODIMP
nsErrorService::UnregisterErrorStringBundle(PRInt16 errorModule)
{
    return mErrorStringBundleURLMap.Remove(errorModule);
}

NS_IMETHODIMP
nsErrorService::GetErrorStringBundle(PRInt16 errorModule, char **result)
{
    char* value = mErrorStringBundleURLMap.Get(errorModule);
    if (value == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    *result = value;
    return NS_OK;
}

NS_IMETHODIMP
nsErrorService::RegisterErrorStringBundleKey(nsresult error, const char *stringBundleKey)
{
    return mErrorStringBundleKeyMap.Put(error, stringBundleKey);
}

NS_IMETHODIMP
nsErrorService::UnregisterErrorStringBundleKey(nsresult error)
{
    return mErrorStringBundleKeyMap.Remove(error);
}

NS_IMETHODIMP
nsErrorService::GetErrorStringBundleKey(nsresult error, char **result)
{
    char* value = mErrorStringBundleKeyMap.Get(error);
    if (value == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    *result = value;
    return NS_OK;
}


