




































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
#include "nsXMLHttpRequest.h"

static PRBool gDisableCORS = PR_FALSE;
static PRBool gDisableCORSPrivateData = PR_FALSE;

class nsChannelCanceller
{
public:
  nsChannelCanceller(nsIChannel* aChannel)
    : mChannel(aChannel)
  {
  }
  ~nsChannelCanceller()
  {
    if (mChannel) {
      mChannel->Cancel(NS_ERROR_DOM_BAD_URI);
    }
  }

  void DontCancel()
  {
    mChannel = nsnull;
  }

private:
  nsIChannel* mChannel;
};

NS_IMPL_ISUPPORTS4(nsCrossSiteListenerProxy, nsIStreamListener,
                   nsIRequestObserver, nsIChannelEventSink,
                   nsIInterfaceRequestor)


void
nsCrossSiteListenerProxy::Startup()
{
  nsContentUtils::AddBoolPrefVarCache("content.cors.disable", &gDisableCORS);
  nsContentUtils::AddBoolPrefVarCache("content.cors.no_private_data", &gDisableCORSPrivateData);
}

nsCrossSiteListenerProxy::nsCrossSiteListenerProxy(nsIStreamListener* aOuter,
                                                   nsIPrincipal* aRequestingPrincipal,
                                                   nsIChannel* aChannel,
                                                   PRBool aWithCredentials,
                                                   nsresult* aResult)
  : mOuterListener(aOuter),
    mRequestingPrincipal(aRequestingPrincipal),
    mWithCredentials(aWithCredentials && !gDisableCORSPrivateData),
    mRequestApproved(PR_FALSE),
    mHasBeenCrossSite(PR_FALSE),
    mIsPreflight(PR_FALSE)
{
  aChannel->GetNotificationCallbacks(getter_AddRefs(mOuterNotificationCallbacks));
  aChannel->SetNotificationCallbacks(this);

  *aResult = UpdateChannel(aChannel);
  if (NS_FAILED(*aResult)) {
    mOuterListener = nsnull;
    mRequestingPrincipal = nsnull;
    mOuterNotificationCallbacks = nsnull;
  }
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
    mWithCredentials(aWithCredentials && !gDisableCORSPrivateData),
    mRequestApproved(PR_FALSE),
    mHasBeenCrossSite(PR_FALSE),
    mIsPreflight(PR_TRUE),
    mPreflightMethod(aPreflightMethod),
    mPreflightHeaders(aPreflightHeaders)
{
  for (PRUint32 i = 0; i < mPreflightHeaders.Length(); ++i) {
    ToLowerCase(mPreflightHeaders[i]);
  }
  mPreflightHeaders.Sort();

  aChannel->GetNotificationCallbacks(getter_AddRefs(mOuterNotificationCallbacks));
  aChannel->SetNotificationCallbacks(this);

  *aResult = UpdateChannel(aChannel);
  if (NS_FAILED(*aResult)) {
    mOuterListener = nsnull;
    mRequestingPrincipal = nsnull;
    mOuterNotificationCallbacks = nsnull;
  }
}

NS_IMETHODIMP
nsCrossSiteListenerProxy::OnStartRequest(nsIRequest* aRequest,
                                         nsISupports* aContext)
{
  mRequestApproved = NS_SUCCEEDED(CheckRequestApproved(aRequest, PR_FALSE));
  if (!mRequestApproved) {
    if (nsXMLHttpRequest::sAccessControlCache) {
      nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
      if (channel) {
      nsCOMPtr<nsIURI> uri;
        channel->GetURI(getter_AddRefs(uri));
        if (uri) {
          nsXMLHttpRequest::sAccessControlCache->
            RemoveEntries(uri, mRequestingPrincipal);
        }
      }
    }

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
nsCrossSiteListenerProxy::CheckRequestApproved(nsIRequest* aRequest,
                                               PRBool aIsRedirect)
{
  
  if (!mHasBeenCrossSite) {
    return NS_OK;
  }

  if (gDisableCORS) {
    return NS_ERROR_DOM_BAD_URI;
  }

  
  nsresult status;
  nsresult rv = aRequest->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(status, status);

  
  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(aRequest);
  NS_ENSURE_TRUE(http, NS_ERROR_DOM_BAD_URI);

  
  
  if (!aIsRedirect) {
    PRBool succeeded;
    rv = http->GetRequestSucceeded(&succeeded);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!succeeded) {
      PRUint32 responseStatus;
      rv = http->GetResponseStatus(&responseStatus);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(mAllowedHTTPErrors.Contains(responseStatus),
                     NS_ERROR_DOM_BAD_URI);
    }
  }

  
  nsCAutoString allowedOriginHeader;
  rv = http->GetResponseHeader(
    NS_LITERAL_CSTRING("Access-Control-Allow-Origin"), allowedOriginHeader);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mWithCredentials || !allowedOriginHeader.EqualsLiteral("*")) {
    nsCAutoString origin;
    rv = nsContentUtils::GetASCIIOrigin(mRequestingPrincipal, origin);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!allowedOriginHeader.Equals(origin) ||
        origin.EqualsLiteral("null")) {
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

    
    
    headerVal.Truncate();
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
  nsChannelCanceller canceller(aOldChannel);
  nsresult rv;
  if (!NS_IsInternalSameURIRedirect(aOldChannel, aNewChannel, aFlags)) {
    rv = CheckRequestApproved(aOldChannel, PR_TRUE);
    if (NS_FAILED(rv)) {
      if (nsXMLHttpRequest::sAccessControlCache) {
        nsCOMPtr<nsIURI> oldURI;
        aOldChannel->GetURI(getter_AddRefs(oldURI));
        if (oldURI) {
          nsXMLHttpRequest::sAccessControlCache->
            RemoveEntries(oldURI, mRequestingPrincipal);
        }
      }
      return NS_ERROR_DOM_BAD_URI;
    }
  }

  nsCOMPtr<nsIChannelEventSink> outer =
    do_GetInterface(mOuterNotificationCallbacks);
  if (outer) {
    rv = outer->OnChannelRedirect(aOldChannel, aNewChannel, aFlags);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = UpdateChannel(aNewChannel);
  NS_ENSURE_SUCCESS(rv, rv);

  canceller.DontCancel();
  
  return NS_OK;
}

nsresult
nsCrossSiteListenerProxy::UpdateChannel(nsIChannel* aChannel)
{
  nsCOMPtr<nsIURI> uri, originalURI;
  nsresult rv = aChannel->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aChannel->GetOriginalURI(getter_AddRefs(originalURI));
  NS_ENSURE_SUCCESS(rv, rv);  

  
  rv = nsContentUtils::GetSecurityManager()->
    CheckLoadURIWithPrincipal(mRequestingPrincipal, uri,
                              nsIScriptSecurityManager::STANDARD);
  NS_ENSURE_SUCCESS(rv, rv);

  if (originalURI != uri) {
    rv = nsContentUtils::GetSecurityManager()->
      CheckLoadURIWithPrincipal(mRequestingPrincipal, originalURI,
                                nsIScriptSecurityManager::STANDARD);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!mHasBeenCrossSite &&
      NS_SUCCEEDED(mRequestingPrincipal->CheckMayLoad(uri, PR_FALSE)) &&
      (originalURI == uri ||
       NS_SUCCEEDED(mRequestingPrincipal->CheckMayLoad(originalURI,
                                                       PR_FALSE)))) {
    return NS_OK;
  }

  
  mHasBeenCrossSite = PR_TRUE;

  nsCString userpass;
  uri->GetUserPass(userpass);
  NS_ENSURE_TRUE(userpass.IsEmpty(), NS_ERROR_DOM_BAD_URI);

  
  nsCAutoString origin;
  rv = nsContentUtils::GetASCIIOrigin(mRequestingPrincipal, origin);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> http = do_QueryInterface(aChannel);
  NS_ENSURE_TRUE(http, NS_ERROR_FAILURE);

  rv = http->SetRequestHeader(NS_LITERAL_CSTRING("Origin"), origin, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (mIsPreflight) {
    rv = http->
      SetRequestHeader(NS_LITERAL_CSTRING("Access-Control-Request-Method"),
                       mPreflightMethod, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mPreflightHeaders.IsEmpty()) {
      nsCAutoString headers;
      for (PRUint32 i = 0; i < mPreflightHeaders.Length(); ++i) {
        if (i != 0) {
          headers += ',';
        }
        headers += mPreflightHeaders[i];
      }
      rv = http->
        SetRequestHeader(NS_LITERAL_CSTRING("Access-Control-Request-Headers"),
                         headers, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
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
