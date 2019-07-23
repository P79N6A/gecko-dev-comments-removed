










































#include "nsNoDataProtocolContentPolicy.h"
#include "nsIDocument.h"
#include "nsINode.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsString.h"
#include "nsContentUtils.h"
#include "nsIProtocolHandler.h"
#include "nsIIOService.h"
#include "nsIExternalProtocolHandler.h"

NS_IMPL_ISUPPORTS1(nsNoDataProtocolContentPolicy, nsIContentPolicy)

NS_IMETHODIMP
nsNoDataProtocolContentPolicy::ShouldLoad(PRUint32 aContentType,
                                          nsIURI *aContentLocation,
                                          nsIURI *aRequestingLocation,
                                          nsISupports *aRequestingContext,
                                          const nsACString &aMimeGuess,
                                          nsISupports *aExtra,
                                          PRInt16 *aDecision)
{
  *aDecision = nsIContentPolicy::ACCEPT;

  
  
  if (aContentType == TYPE_OTHER ||
      aContentType == TYPE_SCRIPT ||
      aContentType == TYPE_IMAGE ||
      aContentType == TYPE_STYLESHEET) {
    nsCAutoString scheme;
    aContentLocation->GetScheme(scheme);
    
    if (scheme.EqualsLiteral("http") ||
        scheme.EqualsLiteral("https") ||
        scheme.EqualsLiteral("ftp") ||
        scheme.EqualsLiteral("file") ||
        scheme.EqualsLiteral("chrome")) {
      return NS_OK;
    }

    nsIIOService* ios = nsContentUtils::GetIOService();
    if (!ios) {
      
      return NS_OK;
    }
    
    nsCOMPtr<nsIProtocolHandler> handler;
    ios->GetProtocolHandler(scheme.get(), getter_AddRefs(handler));

    nsCOMPtr<nsIExternalProtocolHandler> extHandler =
      do_QueryInterface(handler);

    if (extHandler) {
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
                                             PRInt16 *aDecision)
{
  return ShouldLoad(aContentType, aContentLocation, aRequestingLocation,
                    aRequestingContext, aMimeGuess, aExtra, aDecision);
}
