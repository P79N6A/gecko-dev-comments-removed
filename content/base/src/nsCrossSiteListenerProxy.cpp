




































#include "nsCrossSiteListenerProxy.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsDOMError.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsNetUtil.h"
#include "nsIParser.h"
#include "nsParserCIID.h"
#include "nsICharsetAlias.h"
#include "nsMimeTypes.h"
#include "nsIStreamConverterService.h"
#include "nsStringStream.h"
#include "nsParserUtils.h"
#include "nsGkAtoms.h"
#include "nsWhitespaceTokenizer.h"
#include "nsIChannelEventSink.h"
#include "nsCommaSeparatedTokenizer.h"
#include "prclist.h"
#include "nsAutoPtr.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "prtime.h"





#define ACCESS_CONTROL_CACHE_SIZE 100

class nsACPreflightCache
{
public:
  static void Shutdown()
  {
    PR_INIT_CLIST(&mList);
    delete mTable;
    mTable = nsnull;
  }

  struct TokenTime
  {
    nsCString token;
    PRTime expirationTime;
  };

  class CacheEntry : public PRCList
  {
  public:
    CacheEntry(nsCString& aKey)
      : mKey(aKey)
    {
      MOZ_COUNT_CTOR(nsACPreflightCache::CacheEntry);
    }
    
    ~CacheEntry()
    {
      MOZ_COUNT_DTOR(nsACPreflightCache::CacheEntry);
    }

    void PurgeExpired(PRTime now);
    PRBool CheckRequest(const nsCString& aMethod,
                        const nsTArray<nsCString>& aCustomHeaders);

    nsCString mKey;
    nsTArray<TokenTime> mMethods;
    nsTArray<TokenTime> mHeaders;
  };


  static CacheEntry* GetEntry(nsIURI* aURI, nsIPrincipal* aPrincipal,
                              PRBool aWithCredentials, PRBool aCreate);

private:
  PR_STATIC_CALLBACK(PLDHashOperator)
    RemoveExpiredEntries(const nsACString& aKey, nsAutoPtr<CacheEntry>& aValue,
                         void* aUserData);

  static PRBool GetCacheKey(nsIURI* aURI, nsIPrincipal* aPrincipal,
                            PRBool aWithCredentials, nsACString& _retval);

  static nsClassHashtable<nsCStringHashKey, CacheEntry>* mTable;
  static PRCList mList;
};





class nsACPreflightListener : public nsIStreamListener,
                              public nsIInterfaceRequestor,
                              public nsIChannelEventSink
{
public:
  nsACPreflightListener(nsIChannel* aOuterChannel,
                        nsIStreamListener* aOuterListener,
                        nsISupports* aOuterContext,
                        nsIPrincipal* aReferrerPrincipal,
                        PRBool aWithCredentials)
   : mOuterChannel(aOuterChannel), mOuterListener(aOuterListener),
     mOuterContext(aOuterContext), mReferrerPrincipal(aReferrerPrincipal),
     mWithCredentials(aWithCredentials)
  { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

private:
  void AddResultToCache(nsIRequest* aRequest);

  nsCOMPtr<nsIChannel> mOuterChannel;
  nsCOMPtr<nsIStreamListener> mOuterListener;
  nsCOMPtr<nsISupports> mOuterContext;
  nsCOMPtr<nsIPrincipal> mReferrerPrincipal;
  PRBool mWithCredentials;
};




NS_IMPL_ISUPPORTS4(nsCrossSiteListenerProxy, nsIStreamListener,
                   nsIRequestObserver, nsIChannelEventSink,
                   nsIInterfaceRequestor)

nsCrossSiteListenerProxy::nsCrossSiteListenerProxy(nsIStreamListener* aOuter,
                                                   nsIPrincipal* aRequestingPrincipal,
                                                   nsIChannel* aChannel,
                                                   PRBool aWithCredentials,
                                                   nsresult* aResult)
  : mOuterListener(aOuter),
    mRequestingPrincipal(aRequestingPrincipal),
    mWithCredentials(aWithCredentials),
    mRequestApproved(PR_FALSE),
    mHasBeenCrossSite(PR_FALSE),
    mIsPreflight(PR_FALSE)
{
  aChannel->GetNotificationCallbacks(getter_AddRefs(mOuterNotificationCallbacks));
  aChannel->SetNotificationCallbacks(this);

  *aResult = UpdateChannel(aChannel);
}


nsCrossSiteListenerProxy::nsCrossSiteListenerProxy(nsIStreamListener* aOuter,
                                                   nsIPrincipal* aRequestingPrincipal,
                                                   nsIChannel* aChannel,
                                                   PRBool aWithCredentials,
                                                   const nsCString& aPreflightMethod,
                                                   const nsTArray<nsCString>& aPreflightHeaders,
                                                   nsresult* aResult)
  : mOuterListener(aOuter),
    mRequestingPrincipal(aRequestingPrincipal),
    mWithCredentials(aWithCredentials),
    mRequestApproved(PR_FALSE),
    mHasBeenCrossSite(PR_FALSE),
    mIsPreflight(PR_TRUE),
    mPreflightMethod(aPreflightMethod),
    mPreflightHeaders(aPreflightHeaders)
{
  aChannel->GetNotificationCallbacks(getter_AddRefs(mOuterNotificationCallbacks));
  aChannel->SetNotificationCallbacks(this);

  *aResult = UpdateChannel(aChannel);
}

nsresult
nsCrossSiteListenerProxy::CheckPreflight(nsIHttpChannel* aRequestChannel,
                                         nsIStreamListener* aRequestListener,
                                         nsISupports* aRequestContext,
                                         nsIPrincipal* aRequestingPrincipal,
                                         PRBool aForcePreflight,
                                         nsTArray<nsCString>& aUnsafeHeaders,
                                         PRBool aWithCredentials,
                                         PRBool* aPreflighted,
                                         nsIChannel** aPreflightChannel,
                                         nsIStreamListener** aPreflightListener)
{
  *aPreflighted = PR_FALSE;

  nsCAutoString method;
  aRequestChannel->GetRequestMethod(method);

  if (!aUnsafeHeaders.IsEmpty() || aForcePreflight) {
    *aPreflighted = PR_TRUE;
  }
  else if (method.LowerCaseEqualsLiteral("post")) {
    nsCAutoString contentTypeHeader;
    aRequestChannel->GetRequestHeader(NS_LITERAL_CSTRING("Content-Type"),
                                      contentTypeHeader);

    nsCAutoString contentType, charset;
    NS_ParseContentType(contentTypeHeader, contentType, charset);
    if (!contentType.LowerCaseEqualsLiteral("text/plain")) {
      *aPreflighted = PR_TRUE;
    }
  }
  else if (!method.LowerCaseEqualsLiteral("get")) {
    *aPreflighted = PR_TRUE;
  }

  if (!*aPreflighted) {
    return NS_OK;
  }

  

  
  
  nsCOMPtr<nsIURI> uri;
  nsresult rv = aRequestChannel->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsACPreflightCache::CacheEntry* entry =
    nsACPreflightCache::GetEntry(uri, aRequestingPrincipal,
                                 aWithCredentials, PR_FALSE);

  if (entry && entry->CheckRequest(method, aUnsafeHeaders)) {
    return NS_OK;
  }

  
  
  nsCOMPtr<nsILoadGroup> loadGroup;
  rv = aRequestChannel->GetLoadGroup(getter_AddRefs(loadGroup));
  NS_ENSURE_SUCCESS(rv, rv);

  nsLoadFlags loadFlags;
  rv = aRequestChannel->GetLoadFlags(&loadFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChannel> preflightChannel;
  rv = NS_NewChannel(getter_AddRefs(preflightChannel), uri, nsnull,
                     loadGroup, nsnull, loadFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> preflightHttp = do_QueryInterface(preflightChannel);
  NS_ASSERTION(preflightHttp, "Failed to QI to nsIHttpChannel!");

  rv = preflightHttp->SetRequestMethod(NS_LITERAL_CSTRING("OPTIONS"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = preflightHttp->
    SetRequestHeader(NS_LITERAL_CSTRING("Access-Control-Request-Method"),
                     method, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aUnsafeHeaders.IsEmpty()) {
    nsCAutoString headers;
    for (PRUint32 i = 0; i < aUnsafeHeaders.Length(); ++i) {
      if (i != 0) {
        headers += ',';
      }
      headers += aUnsafeHeaders[i];
    }
    rv = preflightHttp->
      SetRequestHeader(NS_LITERAL_CSTRING("Access-Control-Request-Headers"),
                       headers, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIStreamListener> preflightListener =
    new nsACPreflightListener(preflightChannel, aRequestListener,
                              aRequestContext, aRequestingPrincipal,
                              aWithCredentials);
  NS_ENSURE_TRUE(preflightListener, NS_ERROR_OUT_OF_MEMORY);

  preflightListener =
    new nsCrossSiteListenerProxy(preflightListener, aRequestingPrincipal,
                                 preflightChannel, aWithCredentials, method,
                                 aUnsafeHeaders, &rv);
  NS_ENSURE_TRUE(preflightListener, NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_SUCCESS(rv, rv);

  preflightChannel.swap(*aPreflightChannel);
  preflightListener.swap(*aPreflightListener);

  return NS_OK;
}

void
nsCrossSiteListenerProxy::ShutdownPreflightCache()
{
  nsACPreflightCache::Shutdown();
}

NS_IMETHODIMP
nsCrossSiteListenerProxy::OnStartRequest(nsIRequest* aRequest,
                                         nsISupports* aContext)
{
  mRequestApproved = NS_SUCCEEDED(CheckRequestApproved(aRequest));
  if (!mRequestApproved) {
    aRequest->Cancel(NS_ERROR_DOM_BAD_URI);
    mOuterListener->OnStartRequest(aRequest, aContext);

    return NS_ERROR_DOM_BAD_URI;
  }

  return mOuterListener->OnStartRequest(aRequest, aContext);
}

PRBool
IsValidHTTPToken(const nsCSubstring& aToken)
{
  if (aToken.IsEmpty()) {
    return PR_FALSE;
  }

  nsCSubstring::const_char_iterator iter, end;

  aToken.BeginReading(iter);
  aToken.EndReading(end);

  while (iter != end) {
    if (*iter <= 32 ||
        *iter >= 127 ||
        *iter == '(' ||
        *iter == ')' ||
        *iter == '<' ||
        *iter == '>' ||
        *iter == '@' ||
        *iter == ',' ||
        *iter == ';' ||
        *iter == ':' ||
        *iter == '\\' ||
        *iter == '\"' ||
        *iter == '/' ||
        *iter == '[' ||
        *iter == ']' ||
        *iter == '?' ||
        *iter == '=' ||
        *iter == '{' ||
        *iter == '}') {
      return PR_FALSE;
    }
    ++iter;
  }

  return PR_TRUE;
}

nsresult
GetOrigin(nsIPrincipal* aPrincipal, nsCString& aOrigin)
{
  aOrigin.AssignLiteral("null");

  nsCString host;
  nsCOMPtr<nsIURI> uri;
  nsresult rv = aPrincipal->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = uri->GetAsciiHost(host);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!host.IsEmpty()) {
    nsCString scheme;
    rv = uri->GetScheme(scheme);
    NS_ENSURE_SUCCESS(rv, rv);

    aOrigin = scheme + NS_LITERAL_CSTRING("://") + host;

    
    PRInt32 port;
    uri->GetPort(&port);
    if (port != -1) {
      PRInt32 defaultPort = NS_GetDefaultPort(scheme.get());
      if (port != defaultPort) {
        aOrigin.Append(":");
        aOrigin.AppendInt(port);
      }
    }
  }
  else {
    aOrigin.AssignLiteral("null");
  }

  return NS_OK;
}

nsresult
nsCrossSiteListenerProxy::CheckRequestApproved(nsIRequest* aRequest)
{
  
  if (!mHasBeenCrossSite) {
    return NS_OK;
  }

  
  nsresult status;
  nsresult rv = aRequest->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(status, status);

  
  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(aRequest);
  NS_ENSURE_TRUE(http, NS_ERROR_DOM_BAD_URI);

  PRBool succeeded;
  rv = http->GetRequestSucceeded(&succeeded);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(succeeded, NS_ERROR_DOM_BAD_URI);

  
  nsCAutoString allowedOriginHeader;
  rv = http->GetResponseHeader(
    NS_LITERAL_CSTRING("Access-Control-Allow-Origin"), allowedOriginHeader);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mWithCredentials || !allowedOriginHeader.EqualsLiteral("*")) {
    nsCAutoString origin;
    rv = GetOrigin(mRequestingPrincipal, origin);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!allowedOriginHeader.Equals(origin)) {
      return NS_ERROR_DOM_BAD_URI;
    }
  }

  
  if (mWithCredentials) {
    nsCAutoString allowCredentialsHeader;
    rv = http->GetResponseHeader(
      NS_LITERAL_CSTRING("Access-Control-Allow-Credentials"), allowCredentialsHeader);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!allowCredentialsHeader.EqualsLiteral("true")) {
      return NS_ERROR_DOM_BAD_URI;
    }
  }

  if (mIsPreflight) {
    nsCAutoString headerVal;
    
    
    http->GetResponseHeader(NS_LITERAL_CSTRING("Access-Control-Allow-Methods"),
                            headerVal);
    PRBool foundMethod = mPreflightMethod.EqualsLiteral("GET") ||
      mPreflightMethod.EqualsLiteral("POST");
    nsCCommaSeparatedTokenizer methodTokens(headerVal);
    while(methodTokens.hasMoreTokens()) {
      const nsDependentCSubstring& method = methodTokens.nextToken();
      if (method.IsEmpty()) {
        continue;
      }
      if (!IsValidHTTPToken(method)) {
        return NS_ERROR_DOM_BAD_URI;
      }
      foundMethod |= mPreflightMethod.Equals(method);
    }
    NS_ENSURE_TRUE(foundMethod, NS_ERROR_DOM_BAD_URI);

    
    
    http->GetResponseHeader(NS_LITERAL_CSTRING("Access-Control-Allow-Headers"),
                            headerVal);
    nsTArray<nsCString> headers;
    nsCCommaSeparatedTokenizer headerTokens(headerVal);
    while(headerTokens.hasMoreTokens()) {
      const nsDependentCSubstring& header = headerTokens.nextToken();
      if (header.IsEmpty()) {
        continue;
      }
      if (!IsValidHTTPToken(header)) {
        return NS_ERROR_DOM_BAD_URI;
      }
      headers.AppendElement(header);
    }
    for (PRUint32 i = 0; i < mPreflightHeaders.Length(); ++i) {
      if (!headers.Contains(mPreflightHeaders[i],
                            nsCaseInsensitiveCStringArrayComparator())) {
        return NS_ERROR_DOM_BAD_URI;
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCrossSiteListenerProxy::OnStopRequest(nsIRequest* aRequest,
                                        nsISupports* aContext,
                                        nsresult aStatusCode)
{
  return mOuterListener->OnStopRequest(aRequest, aContext, aStatusCode);
}

NS_IMETHODIMP
nsCrossSiteListenerProxy::OnDataAvailable(nsIRequest* aRequest,
                                          nsISupports* aContext, 
                                          nsIInputStream* aInputStream,
                                          PRUint32 aOffset,
                                          PRUint32 aCount)
{
  if (!mRequestApproved) {
    return NS_ERROR_DOM_BAD_URI;
  }
  return mOuterListener->OnDataAvailable(aRequest, aContext, aInputStream,
                                         aOffset, aCount);
}

NS_IMETHODIMP
nsCrossSiteListenerProxy::GetInterface(const nsIID & aIID, void **aResult)
{
  if (aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
    *aResult = static_cast<nsIChannelEventSink*>(this);
    NS_ADDREF_THIS();

    return NS_OK;
  }

  return mOuterNotificationCallbacks ?
    mOuterNotificationCallbacks->GetInterface(aIID, aResult) :
    NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP
nsCrossSiteListenerProxy::OnChannelRedirect(nsIChannel *aOldChannel,
                                            nsIChannel *aNewChannel,
                                            PRUint32    aFlags)
{
  nsresult rv = CheckRequestApproved(aOldChannel);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChannelEventSink> outer =
    do_GetInterface(mOuterNotificationCallbacks);
  if (outer) {
    rv = outer->OnChannelRedirect(aOldChannel, aNewChannel, aFlags);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return UpdateChannel(aNewChannel);
}

nsresult
nsCrossSiteListenerProxy::UpdateChannel(nsIChannel* aChannel)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = aChannel->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = nsContentUtils::GetSecurityManager()->
    CheckLoadURIWithPrincipal(mRequestingPrincipal, uri,
                              nsIScriptSecurityManager::STANDARD);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mHasBeenCrossSite &&
      NS_SUCCEEDED(mRequestingPrincipal->CheckMayLoad(uri, PR_FALSE))) {
    return NS_OK;
  }

  
  mHasBeenCrossSite = PR_TRUE;

  nsCString userpass;
  uri->GetUserPass(userpass);
  NS_ENSURE_TRUE(userpass.IsEmpty(), NS_ERROR_DOM_BAD_URI);

  
  nsCAutoString origin;
  rv = GetOrigin(mRequestingPrincipal, origin);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(aChannel);
  NS_ENSURE_TRUE(http, NS_ERROR_FAILURE);

  rv = http->SetRequestHeader(NS_LITERAL_CSTRING("Origin"), origin, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (mIsPreflight || !mWithCredentials) {
    nsLoadFlags flags;
    rv = http->GetLoadFlags(&flags);
    NS_ENSURE_SUCCESS(rv, rv);

    flags |= nsIRequest::LOAD_ANONYMOUS;
    rv = http->SetLoadFlags(flags);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}




NS_IMPL_ISUPPORTS4(nsACPreflightListener, nsIStreamListener, nsIRequestObserver,
                   nsIInterfaceRequestor, nsIChannelEventSink)

void
nsACPreflightListener::AddResultToCache(nsIRequest *aRequest)
{
  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(aRequest);
  NS_ASSERTION(http, "Request was not http");

  
  nsCAutoString headerVal;
  http->GetResponseHeader(NS_LITERAL_CSTRING("Access-Control-Max-Age"),
                          headerVal);
  if (headerVal.IsEmpty()) {
    return;
  }

  
  
  
  PRUint32 age = 0;
  nsCSubstring::const_char_iterator iter, end;
  headerVal.BeginReading(iter);
  headerVal.EndReading(end);
  while (iter != end) {
    if (*iter < '0' || *iter > '9') {
      return;
    }
    age = age * 10 + (*iter - '0');
    
    age = PR_MIN(age, 86400);
    ++iter;
  }

  if (!age) {
    return;
  }


  
  
  

  nsCOMPtr<nsIURI> uri;
  http->GetURI(getter_AddRefs(uri));

  
  PRTime expirationTime = PR_Now() + (PRUint64)age * PR_USEC_PER_SEC;

  nsACPreflightCache::CacheEntry* entry =
    nsACPreflightCache::GetEntry(uri, mReferrerPrincipal, mWithCredentials,
                                 PR_TRUE);
  if (!entry) {
    return;
  }

  
  
  http->GetResponseHeader(NS_LITERAL_CSTRING("Access-Control-Allow-Methods"),
                          headerVal);

  nsCCommaSeparatedTokenizer methods(headerVal);
  while(methods.hasMoreTokens()) {
    const nsDependentCSubstring& method = methods.nextToken();
    if (method.IsEmpty()) {
      continue;
    }
    PRUint32 i;
    for (i = 0; i < entry->mMethods.Length(); ++i) {
      if (entry->mMethods[i].token.Equals(method)) {
        entry->mMethods[i].expirationTime = expirationTime;
        break;
      }
    }
    if (i == entry->mMethods.Length()) {
      nsACPreflightCache::TokenTime* newMethod =
        entry->mMethods.AppendElement();
      if (!newMethod) {
        return;
      }

      newMethod->token = method;
      newMethod->expirationTime = expirationTime;
    }
  }

  
  
  http->GetResponseHeader(NS_LITERAL_CSTRING("Access-Control-Allow-Headers"),
                          headerVal);

  nsCCommaSeparatedTokenizer headers(headerVal);
  while(headers.hasMoreTokens()) {
    const nsDependentCSubstring& header = headers.nextToken();
    if (header.IsEmpty()) {
      continue;
    }
    PRUint32 i;
    for (i = 0; i < entry->mHeaders.Length(); ++i) {
      if (entry->mHeaders[i].token.Equals(header)) {
        entry->mHeaders[i].expirationTime = expirationTime;
        break;
      }
    }
    if (i == entry->mHeaders.Length()) {
      nsACPreflightCache::TokenTime* newHeader =
        entry->mHeaders.AppendElement();
      if (!newHeader) {
        return;
      }

      newHeader->token = header;
      newHeader->expirationTime = expirationTime;
    }
  }
}

NS_IMETHODIMP
nsACPreflightListener::OnStartRequest(nsIRequest *aRequest,
                                      nsISupports *aContext)
{
  nsresult status;
  nsresult rv = aRequest->GetStatus(&status);

  if (NS_SUCCEEDED(rv)) {
    rv = status;
  }

  if (NS_SUCCEEDED(rv)) {
    
    AddResultToCache(aRequest);

    rv = mOuterChannel->AsyncOpen(mOuterListener, mOuterContext);
  }

  if (NS_FAILED(rv)) {
    mOuterChannel->Cancel(rv);
    mOuterListener->OnStartRequest(mOuterChannel, mOuterContext);
    mOuterListener->OnStopRequest(mOuterChannel, mOuterContext, rv);
    
    return rv;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsACPreflightListener::OnStopRequest(nsIRequest *aRequest,
                                     nsISupports *aContext,
                                     nsresult aStatus)
{
  return NS_OK;
}



NS_IMETHODIMP
nsACPreflightListener::OnDataAvailable(nsIRequest *aRequest,
                                       nsISupports *ctxt,
                                       nsIInputStream *inStr,
                                       PRUint32 sourceOffset,
                                       PRUint32 count)
{
  return NS_OK;
}

NS_IMETHODIMP
nsACPreflightListener::OnChannelRedirect(nsIChannel *aOldChannel,
                                         nsIChannel *aNewChannel,
                                         PRUint32 aFlags)
{
  
  return NS_ERROR_DOM_BAD_URI;
}

NS_IMETHODIMP
nsACPreflightListener::GetInterface(const nsIID & aIID, void **aResult)
{
  return QueryInterface(aIID, aResult);
}




nsClassHashtable<nsCStringHashKey, nsACPreflightCache::CacheEntry>*
  nsACPreflightCache::mTable;
PRCList nsACPreflightCache::mList;

void
nsACPreflightCache::CacheEntry::PurgeExpired(PRTime now)
{
  PRUint32 i;
  for (i = 0; i < mMethods.Length(); ++i) {
    if (now >= mMethods[i].expirationTime) {
      mMethods.RemoveElementAt(i--);
    }
  }
  for (i = 0; i < mHeaders.Length(); ++i) {
    if (now >= mHeaders[i].expirationTime) {
      mHeaders.RemoveElementAt(i--);
    }
  }
}

PRBool
nsACPreflightCache::CacheEntry::CheckRequest(const nsCString& aMethod,
                                             const nsTArray<nsCString>& aHeaders)
{
  PurgeExpired(PR_Now());

  if (!aMethod.EqualsLiteral("GET") && !aMethod.EqualsLiteral("POST")) {
    PRUint32 i;
    for (i = 0; i < mMethods.Length(); ++i) {
      if (aMethod.Equals(mMethods[i].token))
        break;
    }
    if (i == mMethods.Length()) {
      return PR_FALSE;
    }
  }

  for (PRUint32 i = 0; i < aHeaders.Length(); ++i) {
    PRUint32 j;
    for (j = 0; j < mHeaders.Length(); ++j) {
      if (aHeaders[i].Equals(mHeaders[j].token,
                             nsCaseInsensitiveCStringComparator())) {
        break;
      }
    }
    if (j == mHeaders.Length()) {
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}

nsACPreflightCache::CacheEntry*
nsACPreflightCache::GetEntry(nsIURI* aURI, nsIPrincipal* aPrincipal,
                             PRBool aWithCredentials, PRBool aCreate)
{
  if (!mTable) {
    if (!aCreate) {
      return nsnull;
    }

    PR_INIT_CLIST(&mList);

    mTable = new nsClassHashtable<nsCStringHashKey, CacheEntry>;
    NS_ENSURE_TRUE(mTable, nsnull);

    if (!mTable->Init()) {
      delete mTable;
      mTable = nsnull;
      return nsnull;
    }
  }

  nsCString key;
  if (!GetCacheKey(aURI, aPrincipal, aWithCredentials, key)) {
    NS_WARNING("Invalid cache key!");
    return nsnull;
  }

  CacheEntry* entry;

  if (mTable->Get(key, &entry)) {
    

    
    PR_REMOVE_LINK(entry);
    PR_INSERT_LINK(entry, &mList);

    return entry;
  }

  if (!aCreate) {
    return nsnull;
  }

  
  
  entry = new CacheEntry(key);
  if (!entry) {
    NS_WARNING("Failed to allocate new cache entry!");
    return nsnull;
  }

  if (!mTable->Put(key, entry)) {
    
    delete entry;

    NS_WARNING("Failed to add entry to the access control cache!");
    return nsnull;
  }

  PR_INSERT_LINK(entry, &mList);

  NS_ASSERTION(mTable->Count() <= ACCESS_CONTROL_CACHE_SIZE + 1,
               "Something is borked, too many entries in the cache!");

  
  if (mTable->Count() > ACCESS_CONTROL_CACHE_SIZE) {
    
    PRTime now = PR_Now();
    mTable->Enumerate(RemoveExpiredEntries, &now);

    
    
    if (mTable->Count() > ACCESS_CONTROL_CACHE_SIZE) {
      CacheEntry* lruEntry = static_cast<CacheEntry*>(PR_LIST_TAIL(&mList));
      PR_REMOVE_LINK(lruEntry);

      
      mTable->Remove(lruEntry->mKey);

      NS_ASSERTION(mTable->Count() >= ACCESS_CONTROL_CACHE_SIZE,
                   "Somehow tried to remove an entry that was never added!");
    }
  }
  
  return entry;
}

 PR_CALLBACK PLDHashOperator
nsACPreflightCache::RemoveExpiredEntries(const nsACString& aKey,
                                         nsAutoPtr<CacheEntry>& aValue,
                                         void* aUserData)
{
  PRTime* now = static_cast<PRTime*>(aUserData);
  
  aValue->PurgeExpired(*now);
  
  if (aValue->mHeaders.IsEmpty() &&
      aValue->mHeaders.IsEmpty()) {
    
    PR_REMOVE_LINK(aValue);
    return PL_DHASH_REMOVE;
  }
  
  return PL_DHASH_NEXT;
}

 PRBool
nsACPreflightCache::GetCacheKey(nsIURI* aURI, nsIPrincipal* aPrincipal,
                                PRBool aWithCredentials, nsACString& _retval)
{
  NS_ASSERTION(aURI, "Null uri!");
  NS_ASSERTION(aPrincipal, "Null principal!");
  
  NS_NAMED_LITERAL_CSTRING(space, " ");

  nsCOMPtr<nsIURI> uri;
  nsresult rv = aPrincipal->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  
  nsCAutoString scheme, host, port;
  if (uri) {
    uri->GetScheme(scheme);
    uri->GetHost(host);
    port.AppendInt(NS_GetRealPort(uri));
  }

  nsCAutoString cred;
  if (aWithCredentials) {
    _retval.AssignLiteral("cred");
  }
  else {
    _retval.AssignLiteral("nocred");
  }

  nsCAutoString spec;
  rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  _retval.Assign(cred + space + scheme + space + host + space + port + space +
                 spec);

  return PR_TRUE;
}
