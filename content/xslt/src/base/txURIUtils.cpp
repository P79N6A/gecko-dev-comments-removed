






































#include "txURIUtils.h"
#include "nsNetUtil.h"
#include "nsIAttribute.h"
#include "nsIScriptSecurityManager.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIContent.h"
#include "nsIPrincipal.h"
#include "nsINodeInfo.h"











void URIUtils::resolveHref(const nsAString& href, const nsAString& base,
                           nsAString& dest) {
    if (base.IsEmpty()) {
        dest.Append(href);
        return;
    }
    if (href.IsEmpty()) {
        dest.Append(base);
        return;
    }
    nsCOMPtr<nsIURI> pURL;
    nsAutoString resultHref;
    nsresult result = NS_NewURI(getter_AddRefs(pURL), base);
    if (NS_SUCCEEDED(result)) {
        NS_MakeAbsoluteURI(resultHref, href, pURL);
        dest.Append(resultHref);
    }
} 


void
URIUtils::ResetWithSource(nsIDocument *aNewDoc, nsIDOMNode *aSourceNode)
{
    nsCOMPtr<nsINode> node = do_QueryInterface(aSourceNode);
    if (!node) {
        
        aNewDoc->Reset(nsnull, nsnull);
        return;
    }

    nsCOMPtr<nsIDocument> sourceDoc = node->OwnerDoc();
    nsIPrincipal* sourcePrincipal = sourceDoc->NodePrincipal();

    
    nsCOMPtr<nsILoadGroup> loadGroup = sourceDoc->GetDocumentLoadGroup();
    nsCOMPtr<nsIChannel> channel = sourceDoc->GetChannel();
    if (!channel) {
        
        if (NS_FAILED(NS_NewChannel(getter_AddRefs(channel),
                                    sourceDoc->GetDocumentURI(),
                                    nsnull,
                                    loadGroup))) {
            return;
        }
        channel->SetOwner(sourcePrincipal);
    }
    aNewDoc->Reset(channel, loadGroup);
    aNewDoc->SetPrincipal(sourcePrincipal);
    aNewDoc->SetBaseURI(sourceDoc->GetDocBaseURI());

    
    aNewDoc->SetDocumentCharacterSetSource(
          sourceDoc->GetDocumentCharacterSetSource());
    aNewDoc->SetDocumentCharacterSet(sourceDoc->GetDocumentCharacterSet());
}
