




































#include "nsHttpHandler.h"
#include "nsHttpChannel.h"
#include "nsHttpAuthManager.h"
#include "nsReadableUtils.h"
#include "nsNetUtil.h"

NS_IMPL_ISUPPORTS1(nsHttpAuthManager, nsIHttpAuthManager)

nsHttpAuthManager::nsHttpAuthManager()
{
}

nsresult nsHttpAuthManager::Init()
{
  
  

  if (!gHttpHandler) {
    nsresult rv;
    nsCOMPtr<nsIIOService> ios = do_GetIOService(&rv);
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIProtocolHandler> handler;
    rv = ios->GetProtocolHandler("http", getter_AddRefs(handler));
    if (NS_FAILED(rv))
      return rv;

    
    NS_ENSURE_TRUE(gHttpHandler, NS_ERROR_UNEXPECTED);
  }
	
  mAuthCache = gHttpHandler->AuthCache();
  NS_ENSURE_TRUE(mAuthCache, NS_ERROR_FAILURE);
  return NS_OK;
}

nsHttpAuthManager::~nsHttpAuthManager()
{
}

NS_IMETHODIMP
nsHttpAuthManager::GetAuthIdentity(const nsACString & aScheme,
                                   const nsACString & aHost,
                                   PRInt32 aPort,
                                   const nsACString & aAuthType,
                                   const nsACString & aRealm,
                                   const nsACString & aPath,
                                   nsAString & aUserDomain,
                                   nsAString & aUserName,
                                   nsAString & aUserPassword)
{
  nsHttpAuthEntry * entry = nsnull;
  nsresult rv;
  if (!aPath.IsEmpty())
    rv = mAuthCache->GetAuthEntryForPath(PromiseFlatCString(aScheme).get(),
                                         PromiseFlatCString(aHost).get(),
                                         aPort,
                                         PromiseFlatCString(aPath).get(),
                                         &entry);
  else
    rv = mAuthCache->GetAuthEntryForDomain(PromiseFlatCString(aScheme).get(),
                                           PromiseFlatCString(aHost).get(),
                                           aPort,
                                           PromiseFlatCString(aRealm).get(),
                                           &entry);

  if (NS_FAILED(rv))
    return rv;
  if (!entry)
    return NS_ERROR_UNEXPECTED;

  aUserDomain.Assign(entry->Domain());
  aUserName.Assign(entry->User());
  aUserPassword.Assign(entry->Pass());
  return NS_OK;
}

NS_IMETHODIMP
nsHttpAuthManager::SetAuthIdentity(const nsACString & aScheme,
                                   const nsACString & aHost,
                                   PRInt32 aPort,
                                   const nsACString & aAuthType,
                                   const nsACString & aRealm,
                                   const nsACString & aPath,
                                   const nsAString & aUserDomain,
                                   const nsAString & aUserName,
                                   const nsAString & aUserPassword)
{
  nsHttpAuthIdentity ident(PromiseFlatString(aUserDomain).get(),
                           PromiseFlatString(aUserName).get(),
                           PromiseFlatString(aUserPassword).get());

  return mAuthCache->SetAuthEntry(PromiseFlatCString(aScheme).get(),
                                  PromiseFlatCString(aHost).get(),
                                  aPort,
                                  PromiseFlatCString(aPath).get(),
                                  PromiseFlatCString(aRealm).get(),
                                  nsnull,  
                                  nsnull,  
                                  ident,
                                  nsnull); 
}

NS_IMETHODIMP
nsHttpAuthManager::ClearAll()
{
  return mAuthCache->ClearAll();
}
