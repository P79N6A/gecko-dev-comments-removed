




#include "txURIUtils.h"
#include "nsNetUtil.h"
#include "nsIDocument.h"
#include "nsIPrincipal.h"
#include "mozilla/LoadInfo.h"

using mozilla::LoadInfo;











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
        
        aNewDoc->Reset(nullptr, nullptr);
        return;
    }

    nsCOMPtr<nsIDocument> sourceDoc = node->OwnerDoc();
    nsIPrincipal* sourcePrincipal = sourceDoc->NodePrincipal();

    
    nsCOMPtr<nsILoadGroup> loadGroup = sourceDoc->GetDocumentLoadGroup();
    nsCOMPtr<nsIChannel> channel = sourceDoc->GetChannel();
    if (!channel) {
        
        nsresult rv = NS_NewChannel(getter_AddRefs(channel),
                                    sourceDoc->GetDocumentURI(),
                                    sourceDoc,
                                    nsILoadInfo::SEC_FORCE_INHERIT_PRINCIPAL,
                                    nsIContentPolicy::TYPE_OTHER,
                                    loadGroup);

        if (NS_FAILED(rv)) {
            return;
        }
    }
    aNewDoc->Reset(channel, loadGroup);
    aNewDoc->SetPrincipal(sourcePrincipal);
    aNewDoc->SetBaseURI(sourceDoc->GetDocBaseURI());

    
    aNewDoc->SetDocumentCharacterSetSource(
          sourceDoc->GetDocumentCharacterSetSource());
    aNewDoc->SetDocumentCharacterSet(sourceDoc->GetDocumentCharacterSet());
}
