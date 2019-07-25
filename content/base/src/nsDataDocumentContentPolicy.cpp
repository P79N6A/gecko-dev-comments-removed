










































#include "nsDataDocumentContentPolicy.h"
#include "nsNetUtil.h"
#include "nsScriptSecurityManager.h"
#include "nsIDocument.h"
#include "nsINode.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"

NS_IMPL_ISUPPORTS1(nsDataDocumentContentPolicy, nsIContentPolicy)




static bool
HasFlags(nsIURI* aURI, PRUint32 aURIFlags)
{
  bool hasFlags;
  nsresult rv = NS_URIChainHasFlags(aURI, aURIFlags, &hasFlags);
  return NS_SUCCEEDED(rv) && hasFlags;
}

NS_IMETHODIMP
nsDataDocumentContentPolicy::ShouldLoad(PRUint32 aContentType,
                                        nsIURI *aContentLocation,
                                        nsIURI *aRequestingLocation,
                                        nsISupports *aRequestingContext,
                                        const nsACString &aMimeGuess,
                                        nsISupports *aExtra,
                                        PRInt16 *aDecision)
{
  *aDecision = nsIContentPolicy::ACCEPT;
  
  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsINode> node = do_QueryInterface(aRequestingContext);
  if (node) {
    doc = node->OwnerDoc();
  } else {
    nsCOMPtr<nsIDOMWindow> window = do_QueryInterface(aRequestingContext);
    if (window) {
      nsCOMPtr<nsIDOMDocument> domDoc;
      window->GetDocument(getter_AddRefs(domDoc));
      doc = do_QueryInterface(domDoc);
    }
  }

  
  if (!doc || aContentType == nsIContentPolicy::TYPE_DTD) {
    return NS_OK;
  }

  
  if (doc->IsLoadedAsData()) {
    *aDecision = nsIContentPolicy::REJECT_TYPE;
    return NS_OK;
  }

  if (doc->IsBeingUsedAsImage()) {
    
    
    
    
    
    
    if (!HasFlags(aContentLocation,
                  nsIProtocolHandler::URI_IS_LOCAL_RESOURCE) ||
        (!HasFlags(aContentLocation,
                   nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT) &&
         !HasFlags(aContentLocation,
                   nsIProtocolHandler::URI_LOADABLE_BY_SUBSUMERS))) {
      *aDecision = nsIContentPolicy::REJECT_TYPE;

      
      if (node) {
        nsIPrincipal* requestingPrincipal = node->NodePrincipal();
        nsRefPtr<nsIURI> principalURI;
        nsresult rv =
          requestingPrincipal->GetURI(getter_AddRefs(principalURI));
        if (NS_SUCCEEDED(rv) && principalURI) {
          nsScriptSecurityManager::ReportError(
            nsnull, NS_LITERAL_STRING("CheckSameOriginError"), principalURI,
            aContentLocation);
        }
      }
    } else if (aContentType == nsIContentPolicy::TYPE_IMAGE &&
               doc->GetDocumentURI()) {
      
      bool isRecursiveLoad;
      nsresult rv = aContentLocation->EqualsExceptRef(doc->GetDocumentURI(),
                                                      &isRecursiveLoad);
      if (NS_FAILED(rv) || isRecursiveLoad) {
        NS_WARNING("Refusing to recursively load image");
        *aDecision = nsIContentPolicy::REJECT_TYPE;
      }
    }
    return NS_OK;
  }

  
  if (!doc->IsResourceDoc()) {
    return NS_OK;
  }

  
  if (aContentType == nsIContentPolicy::TYPE_OBJECT ||
      aContentType == nsIContentPolicy::TYPE_DOCUMENT ||
      aContentType == nsIContentPolicy::TYPE_SUBDOCUMENT ||
      aContentType == nsIContentPolicy::TYPE_SCRIPT) {
    *aDecision = nsIContentPolicy::REJECT_TYPE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDataDocumentContentPolicy::ShouldProcess(PRUint32 aContentType,
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
