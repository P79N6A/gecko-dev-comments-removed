










#include "nsNoDataProtocolContentPolicy.h"
#include "nsIDOMWindow.h"
#include "nsString.h"
#include "nsIProtocolHandler.h"
#include "nsIIOService.h"
#include "nsIExternalProtocolHandler.h"
#include "nsNetUtil.h"

NS_IMPL_ISUPPORTS1(nsNoDataProtocolContentPolicy, nsIContentPolicy)

NS_IMETHODIMP
nsNoDataProtocolContentPolicy::ShouldLoad(PRUint32 aContentType,
                                          nsIURI *aContentLocation,
                                          nsIURI *aRequestingLocation,
                                          nsISupports *aRequestingContext,
                                          const nsACString &aMimeGuess,
                                          nsISupports *aExtra,
                                          nsIPrincipal *aRequestPrincipal,
                                          PRInt16 *aDecision)
{
  *aDecision = nsIContentPolicy::ACCEPT;

  
  
  
  
  if (aContentType != TYPE_DOCUMENT &&
      aContentType != TYPE_SUBDOCUMENT &&
      aContentType != TYPE_OBJECT &&
      aContentType != TYPE_WEBSOCKET) {

    
    
    nsCAutoString scheme;
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
nsNoDataProtocolContentPolicy::ShouldProcess(PRUint32 aContentType,
                                             nsIURI *aContentLocation,
                                             nsIURI *aRequestingLocation,
                                             nsISupports *aRequestingContext,
                                             const nsACString &aMimeGuess,
                                             nsISupports *aExtra,
                                             nsIPrincipal *aRequestPrincipal,
                                             PRInt16 *aDecision)
{
  return ShouldLoad(aContentType, aContentLocation, aRequestingLocation,
                    aRequestingContext, aMimeGuess, aExtra, aRequestPrincipal,
                    aDecision);
}
