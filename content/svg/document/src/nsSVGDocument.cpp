






































#include "nsSVGDocument.h"
#include "nsContentUtils.h"
#include "nsString.h"
#include "nsLiteralString.h"
#include "nsIDOMSVGSVGElement.h"




nsSVGDocument::nsSVGDocument()
{
}

nsSVGDocument::~nsSVGDocument()
{
}




NS_INTERFACE_TABLE_HEAD(nsSVGDocument)
  NS_INTERFACE_TABLE_INHERITED2(nsSVGDocument,
                                nsIDOMSVGDocument,
                                nsIDOMDocumentEvent)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGDocument)
NS_INTERFACE_MAP_END_INHERITING(nsXMLDocument)

NS_IMPL_ADDREF_INHERITED(nsSVGDocument, nsXMLDocument)
NS_IMPL_RELEASE_INHERITED(nsSVGDocument, nsXMLDocument)





NS_IMETHODIMP
nsSVGDocument::GetTitle(nsAString& aTitle)
{
  return nsXMLDocument::GetTitle(aTitle);
}


NS_IMETHODIMP
nsSVGDocument::GetReferrer(nsAString& aReferrer)
{
  return nsDocument::GetReferrer(aReferrer);
}


NS_IMETHODIMP
nsSVGDocument::GetDomain(nsAString& aDomain)
{
  SetDOMStringToNull(aDomain);

  if (mDocumentURI) {
    nsCAutoString domain;
    nsresult rv = mDocumentURI->GetHost(domain);
    if (domain.IsEmpty() || NS_FAILED(rv))
      return rv;
    CopyUTF8toUTF16(domain, aDomain);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsSVGDocument::GetURL(nsAString& aURL)
{
  SetDOMStringToNull(aURL);

  if (mDocumentURI) {
    nsCAutoString url;
    nsresult rv = mDocumentURI->GetSpec(url);
    if (url.IsEmpty() || NS_FAILED(rv))
      return rv;
    CopyUTF8toUTF16(url, aURL);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsSVGDocument::GetRootElement(nsIDOMSVGSVGElement** aRootElement)
{
  *aRootElement = nsnull;
  nsIContent* root = GetRootContent();

  return root ? CallQueryInterface(root, aRootElement) : NS_OK;
}

nsresult
nsSVGDocument::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  NS_ASSERTION(aNodeInfo->NodeInfoManager() == mNodeInfoManager,
               "Can't import this document into another document!");

  nsRefPtr<nsSVGDocument> clone = new nsSVGDocument();
  NS_ENSURE_TRUE(clone, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = CloneDocHelper(clone.get());
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(clone.get(), aResult);
}




nsresult
NS_NewSVGDocument(nsIDocument** aInstancePtrResult)
{
  *aInstancePtrResult = nsnull;
  nsSVGDocument* doc = new nsSVGDocument();

  if (!doc)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(doc);
  nsresult rv = doc->Init();

  if (NS_FAILED(rv)) {
    NS_RELEASE(doc);
    return rv;
  }

  *aInstancePtrResult = doc;
  return rv;
}
