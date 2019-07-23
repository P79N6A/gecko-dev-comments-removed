





































#include "nsXPathNSResolver.h"
#include "nsIDOMClassInfo.h"
#include "nsDOMString.h"
#include "nsContentUtils.h"

NS_IMPL_ADDREF(nsXPathNSResolver)
NS_IMPL_RELEASE(nsXPathNSResolver)
NS_INTERFACE_MAP_BEGIN(nsXPathNSResolver)
  NS_INTERFACE_MAP_ENTRY(nsIDOMXPathNSResolver)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMXPathNSResolver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XPathNSResolver)
NS_INTERFACE_MAP_END

nsXPathNSResolver::nsXPathNSResolver(nsIDOMNode* aNode)
{
    mNode = do_QueryInterface(aNode);
    NS_ASSERTION(mNode, "Need a node to resolve namespaces.");
}

nsXPathNSResolver::~nsXPathNSResolver()
{
}

NS_IMETHODIMP
nsXPathNSResolver::LookupNamespaceURI(const nsAString & aPrefix,
                                      nsAString & aResult)
{
    if (aPrefix.EqualsLiteral("xml")) {
        aResult.AssignLiteral("http://www.w3.org/XML/1998/namespace");

        return NS_OK;
    }

    if (!mNode) {
        SetDOMStringToNull(aResult);

        return NS_OK;
    }

    return mNode->LookupNamespaceURI(aPrefix, aResult);
}
