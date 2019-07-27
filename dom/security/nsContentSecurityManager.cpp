#include "nsContentSecurityManager.h"
#include "nsIChannel.h"
#include "nsIStreamListener.h"
#include "nsILoadInfo.h"
#include "nsContentUtils.h"
#include "nsCORSListenerProxy.h"
#include "nsIStreamListener.h"

#include "mozilla/dom/Element.h"

nsresult
ValidateSecurityFlags(nsILoadInfo* aLoadInfo)
{
  nsSecurityFlags securityMode = aLoadInfo->GetSecurityMode();

  if (securityMode != nsILoadInfo::SEC_REQUIRE_SAME_ORIGIN_DATA_INHERITS &&
      securityMode != nsILoadInfo::SEC_REQUIRE_SAME_ORIGIN_DATA_IS_BLOCKED &&
      securityMode != nsILoadInfo::SEC_ALLOW_CROSS_ORIGIN_DATA_INHERITS &&
      securityMode != nsILoadInfo::SEC_ALLOW_CROSS_ORIGIN_DATA_IS_NULL &&
      securityMode != nsILoadInfo::SEC_REQUIRE_CORS_DATA_INHERITS) {
    MOZ_ASSERT(false, "need one securityflag from nsILoadInfo to perform security checks");
    return NS_ERROR_FAILURE;
  }

  
  if (aLoadInfo->GetRequireCorsWithCredentials() &&
      securityMode != nsILoadInfo::SEC_REQUIRE_CORS_DATA_INHERITS) {
    MOZ_ASSERT(false, "can not use cors-with-credentials without cors");
    return NS_ERROR_FAILURE;
  }
  
  return NS_OK;
}

nsresult
DoSOPChecks(nsIURI* aURI, nsILoadInfo* aLoadInfo)
{
  nsSecurityFlags securityMode = aLoadInfo->GetSecurityMode();

  
  if ((securityMode != nsILoadInfo::SEC_REQUIRE_SAME_ORIGIN_DATA_INHERITS) &&
      (securityMode != nsILoadInfo::SEC_REQUIRE_SAME_ORIGIN_DATA_IS_BLOCKED)) {
    return NS_OK;
  }

  nsIPrincipal* loadingPrincipal = aLoadInfo->LoadingPrincipal();
  bool sameOriginDataInherits =
    securityMode == nsILoadInfo::SEC_REQUIRE_SAME_ORIGIN_DATA_INHERITS;
  return loadingPrincipal->CheckMayLoad(aURI,
                                        true, 
                                        sameOriginDataInherits);
}

nsresult
DoCheckLoadURIChecks(nsIURI* aURI, nsILoadInfo* aLoadInfo)
{
  nsresult rv = NS_OK;
  nsSecurityFlags securityMode = aLoadInfo->GetSecurityMode();
  
  
  
  
  if ((securityMode != nsILoadInfo::SEC_ALLOW_CROSS_ORIGIN_DATA_INHERITS) &&
      (securityMode != nsILoadInfo::SEC_ALLOW_CROSS_ORIGIN_DATA_IS_NULL)) {
    return NS_OK;
  }

  nsCOMPtr<nsIPrincipal> loadingPrincipal = aLoadInfo->LoadingPrincipal();
  
  
  
  rv = nsContentUtils::GetSecurityManager()->
    CheckLoadURIWithPrincipal(loadingPrincipal,
                              aURI,
                              nsIScriptSecurityManager::STANDARD);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIPrincipal> triggeringPrincipal = aLoadInfo->TriggeringPrincipal();
  if (loadingPrincipal != triggeringPrincipal) {
    rv = nsContentUtils::GetSecurityManager()->
           CheckLoadURIWithPrincipal(triggeringPrincipal,
                                     aURI,
                                     nsIScriptSecurityManager::STANDARD);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
DoCORSChecks(nsIChannel* aChannel, nsILoadInfo* aLoadInfo,
             nsCOMPtr<nsIStreamListener>& aInAndOutListener)
{
  if (aLoadInfo->GetSecurityMode() != nsILoadInfo::SEC_REQUIRE_CORS_DATA_INHERITS) {
    return NS_OK;
  }

  nsIPrincipal* loadingPrincipal = aLoadInfo->LoadingPrincipal();
  nsRefPtr<nsCORSListenerProxy> corsListener =
    new nsCORSListenerProxy(aInAndOutListener,
                            loadingPrincipal,
                            aLoadInfo->GetRequireCorsWithCredentials());
  
  
  
  nsresult rv = corsListener->Init(aChannel, DataURIHandling::Allow);
  NS_ENSURE_SUCCESS(rv, rv);
  aInAndOutListener = corsListener;
  return NS_OK;
}

nsresult
DoContentSecurityChecks(nsIURI* aURI, nsILoadInfo* aLoadInfo)
{
  nsContentPolicyType contentPolicyType = aLoadInfo->GetContentPolicyType();
  nsCString mimeTypeGuess;
  nsCOMPtr<nsISupports> requestingContext = nullptr;

  switch(contentPolicyType) {
    case nsIContentPolicy::TYPE_OTHER:
    case nsIContentPolicy::TYPE_SCRIPT:
    case nsIContentPolicy::TYPE_IMAGE:
    case nsIContentPolicy::TYPE_STYLESHEET:
    case nsIContentPolicy::TYPE_OBJECT:
    case nsIContentPolicy::TYPE_DOCUMENT:
    case nsIContentPolicy::TYPE_SUBDOCUMENT:
    case nsIContentPolicy::TYPE_REFRESH:
    case nsIContentPolicy::TYPE_XBL:
    case nsIContentPolicy::TYPE_PING:
    case nsIContentPolicy::TYPE_XMLHTTPREQUEST:
    
    case nsIContentPolicy::TYPE_OBJECT_SUBREQUEST:
    case nsIContentPolicy::TYPE_DTD:
    case nsIContentPolicy::TYPE_FONT:
      MOZ_ASSERT(false, "contentPolicyType not supported yet");
      break;

    case nsIContentPolicy::TYPE_MEDIA:
      mimeTypeGuess = EmptyCString();
      requestingContext = aLoadInfo->LoadingNode();
#ifdef DEBUG
      {
        nsCOMPtr<mozilla::dom::Element> element = do_QueryInterface(requestingContext);
        NS_ASSERTION(element != nullptr,
                     "type_media requires requestingContext of type Element");
      }
#endif
      break;

    case nsIContentPolicy::TYPE_WEBSOCKET:
    case nsIContentPolicy::TYPE_CSP_REPORT:
    case nsIContentPolicy::TYPE_XSLT:
    case nsIContentPolicy::TYPE_BEACON:
    case nsIContentPolicy::TYPE_FETCH:
    case nsIContentPolicy::TYPE_IMAGESET:
      MOZ_ASSERT(false, "contentPolicyType not supported yet");
      break;

    default:
      
      MOZ_ASSERT(false, "can not perform security check without a valid contentType");
  }

  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  nsresult rv = NS_CheckContentLoadPolicy(contentPolicyType,
                                          aURI,
                                          aLoadInfo->LoadingPrincipal(),
                                          requestingContext,
                                          mimeTypeGuess,
                                          nullptr,        
                                          &shouldLoad,
                                          nsContentUtils::GetContentPolicy(),
                                          nsContentUtils::GetSecurityManager());
  NS_ENSURE_SUCCESS(rv, rv);
  if (NS_CP_REJECTED(shouldLoad)) {
    return NS_ERROR_CONTENT_BLOCKED;
  }
  return NS_OK;
}



















nsresult
nsContentSecurityManager::doContentSecurityCheck(nsIChannel* aChannel,
                                                 nsCOMPtr<nsIStreamListener>& aInAndOutListener)
{
  NS_ENSURE_ARG(aChannel);
  nsCOMPtr<nsILoadInfo> loadInfo = aChannel->GetLoadInfo();

  if (!loadInfo) {
    MOZ_ASSERT(false, "channel needs to have loadInfo to perform security checks");
    return NS_ERROR_UNEXPECTED;
  }

  
  
  nsresult rv = ValidateSecurityFlags(loadInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  bool initialSecurityCheckDone = loadInfo->GetInitialSecurityCheckDone();

  
  rv = loadInfo->SetInitialSecurityCheckDone(true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  rv = loadInfo->SetEnforceSecurity(true);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> finalChannelURI;
  rv = NS_GetFinalChannelURI(aChannel, getter_AddRefs(finalChannelURI));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = DoSOPChecks(finalChannelURI, loadInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (initialSecurityCheckDone) {
    return NS_OK;
  }

  rv = DoCheckLoadURIChecks(finalChannelURI, loadInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = DoCORSChecks(aChannel, loadInfo, aInAndOutListener);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = DoContentSecurityChecks(finalChannelURI, loadInfo);
  NS_ENSURE_SUCCESS(rv, rv);

  
  return NS_OK;
}
