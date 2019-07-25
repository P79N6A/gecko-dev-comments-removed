





































#include "nsXPathNamespace.h"
#include "nsIDOMClassInfo.h"

NS_IMPL_ADDREF(nsXPathNamespace)
NS_IMPL_RELEASE(nsXPathNamespace)

DOMCI_DATA(XPathNamespace, nsXPathNamespace)

NS_INTERFACE_MAP_BEGIN(nsXPathNamespace)
  NS_INTERFACE_MAP_ENTRY(nsIDOMXPathNamespace)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMXPathNamespace)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(XPathNamespace)
NS_INTERFACE_MAP_END


NS_IMETHODIMP nsXPathNamespace::GetNodeName(nsAString & aNodeName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::GetNodeValue(nsAString & aNodeValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsXPathNamespace::SetNodeValue(const nsAString & aNodeValue)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::GetNodeType(PRUint16 *aNodeType)
{
    *aNodeType = XPATH_NAMESPACE_NODE;
    return NS_OK;
}


NS_IMETHODIMP nsXPathNamespace::GetParentNode(nsIDOMNode * *aParentNode)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::GetChildNodes(nsIDOMNodeList * *aChildNodes)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::GetFirstChild(nsIDOMNode * *aFirstChild)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::GetLastChild(nsIDOMNode * *aLastChild)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::GetPreviousSibling(nsIDOMNode * *aPreviousSibling)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::GetNextSibling(nsIDOMNode * *aNextSibling)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::GetAttributes(nsIDOMNamedNodeMap * *aAttributes)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::GetOwnerDocument(nsIDOMDocument * *aOwnerDocument)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::InsertBefore(nsIDOMNode *newChild, nsIDOMNode *refChild, nsIDOMNode **aResult)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::ReplaceChild(nsIDOMNode *newChild, nsIDOMNode *oldChild, nsIDOMNode **aResult)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::RemoveChild(nsIDOMNode *oldChild, nsIDOMNode **aResult)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::AppendChild(nsIDOMNode *newChild, nsIDOMNode **aResult)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::HasChildNodes(bool *aResult)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::CloneNode(bool deep, nsIDOMNode **aResult)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::Normalize()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::IsSupported(const nsAString & feature, const nsAString & version, bool *aResult)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::GetNamespaceURI(nsAString & aNamespaceURI)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::GetPrefix(nsAString & aPrefix)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP nsXPathNamespace::SetPrefix(const nsAString & aPrefix)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::GetLocalName(nsAString & aLocalName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::HasAttributes(bool *aResult)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsXPathNamespace::GetOwnerElement(nsIDOMElement * *aOwnerElement)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
