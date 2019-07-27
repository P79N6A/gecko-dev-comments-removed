











#include "nsNoDataProtocolContentPolicy.h"
#include "nsIDOMWindow.h"
#include "nsString.h"
#include "nsIProtocolHandler.h"
#include "nsIIOService.h"
#include "nsIExternalProtocolHandler.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"

NS_IMPL_ISUPPORTS(nsNoDataProtocolContentPolicy, nsIContentPolicy)

NS_IMETHODIMP
nsNoDataProtocolContentPolicy::ShouldLoad(uint32_t aContentType,
                                          nsIURI *aContentLocation,
                                          nsIURI *aRequestingLocation,
                                          nsISupports *aRequestingContext,
                                          const nsACString &aMimeGuess,
                                          nsISupports *aExtra,
                                          nsIPrincipal *aRequestPrincipal,
                                          int16_t *aDecision)
{
  MOZ_ASSERT(aContentType == nsContentUtils::InternalContentPolicyTypeToExternal(aContentType),
             "We should only see external content policy types here.");

  *aDecision = nsIContentPolicy::ACCEPT;

  
  
  
  
  if (aContentType != TYPE_DOCUMENT &&
      aContentType != TYPE_SUBDOCUMENT &&
      aContentType != TYPE_OBJECT &&
      aContentType != TYPE_WEBSOCKET) {

    
    
    nsAutoCString scheme;
    aContentLocation->GetScheme(scheme);
    if (scheme.EqualsLiteral("http") ||
        scheme.EqualsLiteral("https") ||
        scheme.EqualsLiteral("ftp") ||
        scheme.EqualsLiteral("file") ||
        scheme.EqualsLiteral("chrome")) {
      return NS_OK;
    }

    bool shouldBlock;
    nsresult rv = NS_URIChainHasFlags(aContentLocation,
                                      nsIProtocolHandler::URI_DOES_NOT_RETURN_DATA,
                                      &shouldBlock);
    if (NS_SUCCEEDED(rv) && shouldBlock) {
      *aDecision = nsIContentPolicy::REJECT_REQUEST;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsNoDataProtocolContentPolicy::ShouldProcess(uint32_t aContentType,
                                             nsIURI *aContentLocation,
                                             nsIURI *aRequestingLocation,
                                             nsISupports *aRequestingContext,
                                             const nsACString &aMimeGuess,
                                             nsISupports *aExtra,
                                             nsIPrincipal *aRequestPrincipal,
                                             int16_t *aDecision)
{
  return ShouldLoad(aContentType, aContentLocation, aRequestingLocation,
                    aRequestingContext, aMimeGuess, aExtra, aRequestPrincipal,
                    aDecision);
}
