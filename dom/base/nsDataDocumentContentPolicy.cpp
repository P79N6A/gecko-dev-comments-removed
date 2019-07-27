











#include "nsContentUtils.h"
#include "nsDataDocumentContentPolicy.h"
#include "nsNetUtil.h"
#include "nsIProtocolHandler.h"
#include "nsScriptSecurityManager.h"
#include "nsIDocument.h"
#include "nsINode.h"
#include "nsIDOMWindow.h"

NS_IMPL_ISUPPORTS(nsDataDocumentContentPolicy, nsIContentPolicy)




static bool
HasFlags(nsIURI* aURI, uint32_t aURIFlags)
{
  bool hasFlags;
  nsresult rv = NS_URIChainHasFlags(aURI, aURIFlags, &hasFlags);
  return NS_SUCCEEDED(rv) && hasFlags;
}




NS_IMETHODIMP
nsDataDocumentContentPolicy::ShouldLoad(uint32_t aContentType,
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
  
  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsINode> node = do_QueryInterface(aRequestingContext);
  if (node) {
    doc = node->OwnerDoc();
  } else {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aRequestingContext);
    if (window) {
      doc = window->GetDoc();
    }
  }

  
  if (!doc || aContentType == nsIContentPolicy::TYPE_DTD) {
    return NS_OK;
  }

  
  if (doc->IsLoadedAsData()) {
    
    if (!doc->IsStaticDocument() || aContentType != nsIContentPolicy::TYPE_FONT) {
      *aDecision = nsIContentPolicy::REJECT_TYPE;
      return NS_OK;
    }
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
            nullptr, NS_LITERAL_STRING("ExternalDataError"), principalURI,
            aContentLocation);
        }
      }
    } else if ((aContentType == nsIContentPolicy::TYPE_IMAGE ||
                aContentType == nsIContentPolicy::TYPE_IMAGESET) &&
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
      aContentType == nsIContentPolicy::TYPE_SCRIPT ||
      aContentType == nsIContentPolicy::TYPE_XSLT ||
      aContentType == nsIContentPolicy::TYPE_FETCH ||
      aContentType == nsIContentPolicy::TYPE_WEB_MANIFEST) {
    *aDecision = nsIContentPolicy::REJECT_TYPE;
  }

  
  
  

  return NS_OK;
}

NS_IMETHODIMP
nsDataDocumentContentPolicy::ShouldProcess(uint32_t aContentType,
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
