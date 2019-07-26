





#include "HttpLog.h"

#include "nsHttpHandler.h"
#include "nsHttpAuthManager.h"
#include "nsNetUtil.h"
#include "nsIPrincipal.h"

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
	
  mAuthCache = gHttpHandler->AuthCache(false);
  mPrivateAuthCache = gHttpHandler->AuthCache(true);
  NS_ENSURE_TRUE(mAuthCache, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(mPrivateAuthCache, NS_ERROR_FAILURE);
  return NS_OK;
}

nsHttpAuthManager::~nsHttpAuthManager()
{
}

NS_IMETHODIMP
nsHttpAuthManager::GetAuthIdentity(const nsACString & aScheme,
                                   const nsACString & aHost,
                                   int32_t aPort,
                                   const nsACString & aAuthType,
                                   const nsACString & aRealm,
                                   const nsACString & aPath,
                                   nsAString & aUserDomain,
                                   nsAString & aUserName,
                                   nsAString & aUserPassword,
                                   bool aIsPrivate,
                                   nsIPrincipal* aPrincipal)
{
  nsHttpAuthCache* auth_cache = aIsPrivate ? mPrivateAuthCache : mAuthCache;
  nsHttpAuthEntry * entry = nullptr;
  nsresult rv;
  uint32_t appId = NECKO_NO_APP_ID;
  bool inBrowserElement = false;
  if (aPrincipal) {
    appId = aPrincipal->GetAppId();
    inBrowserElement = aPrincipal->GetIsInBrowserElement();
  }

  if (!aPath.IsEmpty())
    rv = auth_cache->GetAuthEntryForPath(PromiseFlatCString(aScheme).get(),
                                         PromiseFlatCString(aHost).get(),
                                         aPort,
                                         PromiseFlatCString(aPath).get(),
                                         appId, inBrowserElement,
                                         &entry);
  else
    rv = auth_cache->GetAuthEntryForDomain(PromiseFlatCString(aScheme).get(),
                                           PromiseFlatCString(aHost).get(),
                                           aPort,
                                           PromiseFlatCString(aRealm).get(),
                                           appId, inBrowserElement,
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
                                   int32_t aPort,
                                   const nsACString & aAuthType,
                                   const nsACString & aRealm,
                                   const nsACString & aPath,
                                   const nsAString & aUserDomain,
                                   const nsAString & aUserName,
                                   const nsAString & aUserPassword,
                                   bool aIsPrivate,
                                   nsIPrincipal* aPrincipal)
{
  nsHttpAuthIdentity ident(PromiseFlatString(aUserDomain).get(),
                           PromiseFlatString(aUserName).get(),
                           PromiseFlatString(aUserPassword).get());

  uint32_t appId = NECKO_NO_APP_ID;
  bool inBrowserElement = false;
  if (aPrincipal) {
    appId = aPrincipal->GetAppId();
    inBrowserElement = aPrincipal->GetIsInBrowserElement();
  }

  nsHttpAuthCache* auth_cache = aIsPrivate ? mPrivateAuthCache : mAuthCache;
  return auth_cache->SetAuthEntry(PromiseFlatCString(aScheme).get(),
                                  PromiseFlatCString(aHost).get(),
                                  aPort,
                                  PromiseFlatCString(aPath).get(),
                                  PromiseFlatCString(aRealm).get(),
                                  nullptr,  
                                  nullptr,  
                                  appId, inBrowserElement,
                                  &ident,
                                  nullptr); 
}

NS_IMETHODIMP
nsHttpAuthManager::ClearAll()
{
  nsresult rv = mAuthCache->ClearAll();
  nsresult rv2 = mPrivateAuthCache->ClearAll();
  if (NS_FAILED(rv))
    return rv;
  if (NS_FAILED(rv2))
    return rv2;
  return NS_OK;
}
