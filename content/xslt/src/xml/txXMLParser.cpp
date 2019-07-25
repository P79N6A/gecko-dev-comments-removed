





































#include "txXMLParser.h"
#include "txURIUtils.h"
#include "txXPathTreeWalker.h"

#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsSyncLoadService.h"
#include "nsNetUtil.h"
#include "nsIPrincipal.h"

nsresult
txParseDocumentFromURI(const nsAString& aHref, const txXPathNode& aLoader,
                       nsAString& aErrMsg, txXPathNode** aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nsnull;
    nsCOMPtr<nsIURI> documentURI;
    nsresult rv = NS_NewURI(getter_AddRefs(documentURI), aHref);
    NS_ENSURE_SUCCESS(rv, rv);

    nsIDocument* loaderDocument = txXPathNativeNode::getDocument(aLoader);

    nsCOMPtr<nsILoadGroup> loadGroup = loaderDocument->GetDocumentLoadGroup();

    
    

    
    
    nsIDOMDocument* theDocument = nsnull;
    rv = nsSyncLoadService::LoadDocument(documentURI,
                                         loaderDocument->NodePrincipal(),
                                         loadGroup, true, &theDocument);

    if (NS_FAILED(rv)) {
        aErrMsg.Append(NS_LITERAL_STRING("Document load of ") + 
                       aHref + NS_LITERAL_STRING(" failed."));
        return NS_FAILED(rv) ? rv : NS_ERROR_FAILURE;
    }

    *aResult = txXPathNativeNode::createXPathNode(theDocument);
    if (!*aResult) {
        NS_RELEASE(theDocument);
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}
