




#include "nsMixedContentBlocker.h"
#include "nsContentPolicyUtils.h"

#include "nsINode.h"
#include "nsCOMPtr.h"
#include "nsIDocShell.h"
#include "nsISecurityEventSink.h"
#include "nsIWebProgressListener.h"
#include "nsContentUtils.h"
#include "nsNetUtil.h"
#include "mozilla/Preferences.h"

using namespace mozilla;



bool nsMixedContentBlocker::sBlockMixedScript = false;


bool nsMixedContentBlocker::sBlockMixedDisplay = false;




































nsMixedContentBlocker::nsMixedContentBlocker()
{
  
  Preferences::AddBoolVarCache(&sBlockMixedScript,
                               "security.mixed_content.block_active_content");

  
  Preferences::AddBoolVarCache(&sBlockMixedDisplay,
                               "security.mixed_content.block_display_content");
}

nsMixedContentBlocker::~nsMixedContentBlocker()
{
}

NS_IMPL_ISUPPORTS1(nsMixedContentBlocker, nsIContentPolicy)

NS_IMETHODIMP
nsMixedContentBlocker::ShouldLoad(uint32_t aContentType,
                                  nsIURI* aContentLocation,
                                  nsIURI* aRequestingLocation,
                                  nsISupports* aRequestingContext,
                                  const nsACString& aMimeGuess,
                                  nsISupports* aExtra,
                                  nsIPrincipal* aRequestPrincipal,
                                  int16_t* aDecision)
{
  
  *aDecision = nsIContentPolicy::ACCEPT;

  
  
  if (!sBlockMixedScript && !sBlockMixedDisplay) {
    return NS_OK;
  }

  
  
  if (aContentType == nsIContentPolicy::TYPE_DOCUMENT || aContentType == nsIContentPolicy::TYPE_WEBSOCKET) {
    return NS_OK;
  }

  
  
  if (!aRequestingLocation) {
    nsCOMPtr<nsINode> node = do_QueryInterface(aRequestingContext);
    if (node) {
      nsCOMPtr<nsIURI> principalUri;
      node->NodePrincipal()->GetURI(getter_AddRefs(principalUri));
      aRequestingLocation = principalUri;
    }
    
    
    if (!aRequestingLocation) {
      *aDecision = nsIContentPolicy::REJECT_REQUEST;
      return NS_OK;
    }
  }

  
  
  bool parentIsHttps;
  if (NS_FAILED(aRequestingLocation->SchemeIs("https", &parentIsHttps)) ||
      !parentIsHttps) {
    return NS_OK;
  }

 
















  bool schemeLocal = false;
  bool schemeNoReturnData = false;
  bool schemeInherits = false;
  bool schemeSecure = false;
  if (NS_FAILED(NS_URIChainHasFlags(aContentLocation, nsIProtocolHandler::URI_IS_LOCAL_RESOURCE , &schemeLocal))  ||
      NS_FAILED(NS_URIChainHasFlags(aContentLocation, nsIProtocolHandler::URI_DOES_NOT_RETURN_DATA, &schemeNoReturnData)) ||
      NS_FAILED(NS_URIChainHasFlags(aContentLocation, nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT, &schemeInherits)) ||
      NS_FAILED(NS_URIChainHasFlags(aContentLocation, nsIProtocolHandler::URI_SAFE_TO_LOAD_IN_SECURE_CONTEXT, &schemeSecure))) {
    return NS_ERROR_FAILURE;
  }

  if (schemeLocal || schemeNoReturnData || schemeInherits || schemeSecure) {
     return NS_OK;
  }

  

  
  
  switch (aContentType) {
    case nsIContentPolicy::TYPE_FONT:
    case nsIContentPolicy::TYPE_OBJECT:
    case nsIContentPolicy::TYPE_SCRIPT:
    case nsIContentPolicy::TYPE_STYLESHEET:
    case nsIContentPolicy::TYPE_SUBDOCUMENT:
    case nsIContentPolicy::TYPE_WEBSOCKET:
    case nsIContentPolicy::TYPE_XMLHTTPREQUEST:
      
      
      
      if (sBlockMixedScript) {
        *aDecision = nsIContentPolicy::REJECT_REQUEST;

        
        
        
        



      }
      break;

    case nsIContentPolicy::TYPE_IMAGE:
    case nsIContentPolicy::TYPE_MEDIA:
    case nsIContentPolicy::TYPE_PING:
      
      
      if (sBlockMixedDisplay) {
        *aDecision = nsIContentPolicy::REJECT_REQUEST;

        
        
        
        



      }
      break;

    default:
      
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMixedContentBlocker::ShouldProcess(uint32_t aContentType,
                                     nsIURI* aContentLocation,
                                     nsIURI* aRequestingLocation,
                                     nsISupports* aRequestingContext,
                                     const nsACString& aMimeGuess,
                                     nsISupports* aExtra,
                                     nsIPrincipal* aRequestPrincipal,
                                     int16_t* aDecision)
{
  if (!aContentLocation) {
    
    if (aContentType == TYPE_OBJECT) {
       return NS_OK;
    } else {
       return NS_ERROR_FAILURE;
    }
  }

  return ShouldLoad(aContentType, aContentLocation, aRequestingLocation,
                    aRequestingContext, aMimeGuess, aExtra, aRequestPrincipal,
                    aDecision);
}
