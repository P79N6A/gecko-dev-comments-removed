





































#include "stdafx.h"
#include "IEHtmlNode.h"

#include "plhash.h"

static PLHashTable *g_NodeLookupTable;

static PLHashNumber HashFunction(const void *key)
{
    return (PRUint32) key;
}

PRIntn HashComparator(const void *v1, const void *v2)
{
    if (v1 == v2)
    {
        return 1;
    }
    return 0;
}


CNode::CNode() :
    mParent(NULL),mDOMNode(NULL)
{
}

CNode::~CNode()
{
}


HRESULT CNode::SetParent(CNode *pParent)
{
    mParent = pParent;
    return S_OK;
}


HRESULT CNode::FindFromDOMNode(nsIDOMNode *pIDOMNode, CNode **pNode)
{
    if (pIDOMNode == nsnull)
    {
        return E_FAIL;
    }

    if (g_NodeLookupTable == NULL)
    {
        return E_FAIL;
    }

    nsCOMPtr<nsISupports> nodeAsSupports = do_QueryInterface(pIDOMNode);
    *pNode = (CNode *) PL_HashTableLookup(g_NodeLookupTable, nodeAsSupports);

    return S_OK;
}

HRESULT CNode::SetDOMNode(nsIDOMNode *pIDOMNode)
{
    if (pIDOMNode)
    {
        if (g_NodeLookupTable == NULL)
        {
            g_NodeLookupTable = PL_NewHashTable(123, HashFunction, HashComparator, HashComparator, NULL, NULL);
        }

        mDOMNode = pIDOMNode;
        nsCOMPtr<nsISupports> nodeAsSupports= do_QueryInterface(mDOMNode);
        PL_HashTableAdd(g_NodeLookupTable, nodeAsSupports, this);
    }
    else if (mDOMNode)
    {
        
        nsCOMPtr<nsISupports> nodeAsSupports = do_QueryInterface(mDOMNode);
        PL_HashTableRemove(g_NodeLookupTable, nodeAsSupports);
        mDOMNode = nsnull;

        if (g_NodeLookupTable->nentries == 0)
        {
            PL_HashTableDestroy(g_NodeLookupTable);
            g_NodeLookupTable = NULL;
        }
    }
    return S_OK;
}




#include "nsIDOMHTMLButtonElement.h"
#include "nsIDOMHTMLElement.h"

#include "IEHtmlButtonElement.h"
#include "IEHtmlElement.h"

CIEHtmlDomNode::CIEHtmlDomNode()
{
}

CIEHtmlDomNode::~CIEHtmlDomNode()
{
    SetDOMNode(nsnull);
}

#define CREATE_FROM_DOMNODE(nsInterface, WrapperType, errorMsg) \
    nsCOMPtr<nsInterface> domNode_##nsInterface = do_QueryInterface(pIDOMNode); \
    if (domNode_##nsInterface) \
    { \
        WrapperType *pWrapper = NULL; \
        WrapperType::CreateInstance(&pWrapper); \
        if (!pWrapper) \
        { \
            NS_ERROR(errorMsg); \
            return E_OUTOFMEMORY; \
        } \
        if (FAILED(pWrapper->QueryInterface(IID_IUnknown, (void**)pNode))) \
            return E_UNEXPECTED; \
        pWrapper->SetDOMNode(pIDOMNode); \
        return S_OK; \
    }


HRESULT CIEHtmlDomNode::CreateFromDOMNode(nsIDOMNode *pIDOMNode, IUnknown **pNode)
{
    CREATE_FROM_DOMNODE(nsIDOMHTMLButtonElement, CIEHtmlButtonElementInstance, "")
    CREATE_FROM_DOMNODE(nsIDOMHTMLElement, CIEHtmlElementInstance, "Could not create element")
    CREATE_FROM_DOMNODE(nsIDOMNode, CIEHtmlDomNodeInstance, "Could not create node")
    return E_FAIL;
}

HRESULT CIEHtmlDomNode::FindFromDOMNode(nsIDOMNode *pIDOMNode, IUnknown **pNode)
{
    if (pIDOMNode == nsnull)
    {
        return E_FAIL;
    }

    if (g_NodeLookupTable == NULL)
    {
        return E_FAIL;
    }

    nsCOMPtr<nsISupports> nodeAsSupports = do_QueryInterface(pIDOMNode);
    *pNode = (IUnknown *) PL_HashTableLookup(g_NodeLookupTable, nodeAsSupports);

    return S_OK;
}

HRESULT CIEHtmlDomNode::FindOrCreateFromDOMNode(nsIDOMNode *pIDOMNode, IUnknown **pNode)
{
    FindFromDOMNode(pIDOMNode,pNode);

    if (*pNode)
    {
        (*pNode)->AddRef();
        return S_OK;
    }

    HRESULT hr = CreateFromDOMNode(pIDOMNode, pNode);
    if SUCCEEDED(hr)
        return S_OK;
    return hr;
}

HRESULT CIEHtmlDomNode::SetDOMNode(nsIDOMNode *pIDOMNode)
{
    if (pIDOMNode)
    {
        if (g_NodeLookupTable == NULL)
        {
            g_NodeLookupTable = PL_NewHashTable(123, HashFunction, HashComparator, HashComparator, NULL, NULL);
        }
        
        mDOMNode = pIDOMNode;
        nsCOMPtr<nsISupports> nodeAsSupports= do_QueryInterface(mDOMNode);
        PL_HashTableAdd(g_NodeLookupTable, nodeAsSupports, (IUnknown *)this );
    }
    else if (mDOMNode)
    {    
        
        nsCOMPtr<nsISupports> nodeAsSupports = do_QueryInterface(mDOMNode);
        PL_HashTableRemove(g_NodeLookupTable, nodeAsSupports);
        mDOMNode = nsnull;

        if (g_NodeLookupTable->nentries == 0)
        {
            PL_HashTableDestroy(g_NodeLookupTable);
            g_NodeLookupTable = NULL;
        }    
    }
    return S_OK;
}




#define SIB_NODE_GET_NUMERIC(function,numtype) \
{ \
  if (!p) return E_INVALIDARG; \
  if (!mDOMNode) return E_UNEXPECTED; \
  numtype nData; \
  HRESULT rc = mDOMNode->function(&nData); \
  *p=nData; \
  return rc; \
}

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::get_nodeType(long __RPC_FAR *p)
  SIB_NODE_GET_NUMERIC(GetNodeType,PRUint16)

#define SIB_NODE_GET_ELEMENT(function,fn_elt_type) \
{ \
    return E_NOTIMPL; \
}

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::get_parentNode(IHTMLDOMNode __RPC_FAR *__RPC_FAR *p)
  SIB_NODE_GET_ELEMENT(GetParentNode,nsIDOMNode)

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::hasChildNodes(VARIANT_BOOL __RPC_FAR *p)
  SIB_NODE_GET_NUMERIC(HasChildNodes,PRBool)

#define SIB_STD_NOTIMPL \
{ \
  return E_NOTIMPL; \
}

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::get_childNodes(IDispatch __RPC_FAR *__RPC_FAR *p)
  SIB_STD_NOTIMPL

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::get_attributes(IDispatch __RPC_FAR *__RPC_FAR *p)
  SIB_STD_NOTIMPL

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::insertBefore(IHTMLDOMNode __RPC_FAR *newChild,
    VARIANT refChild,
    IHTMLDOMNode __RPC_FAR *__RPC_FAR *node)
  SIB_STD_NOTIMPL

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::removeChild(
    IHTMLDOMNode __RPC_FAR *oldChild,
    IHTMLDOMNode __RPC_FAR *__RPC_FAR *node)
  SIB_STD_NOTIMPL

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::replaceChild(
    IHTMLDOMNode __RPC_FAR *newChild,
    IHTMLDOMNode __RPC_FAR *oldChild,
    IHTMLDOMNode __RPC_FAR *__RPC_FAR *node)
  SIB_STD_NOTIMPL

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::cloneNode( 
    VARIANT_BOOL fDeep,
    IHTMLDOMNode __RPC_FAR *__RPC_FAR *clonedNode)
  SIB_STD_NOTIMPL

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::removeNode( 
    VARIANT_BOOL fDeep,
    IHTMLDOMNode __RPC_FAR *__RPC_FAR *removed)
  SIB_STD_NOTIMPL

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::swapNode( 
    IHTMLDOMNode __RPC_FAR *otherNode,
    IHTMLDOMNode __RPC_FAR *__RPC_FAR *swappedNode)
  SIB_STD_NOTIMPL

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::replaceNode( 
    IHTMLDOMNode __RPC_FAR *replacement,
    IHTMLDOMNode __RPC_FAR *__RPC_FAR *replaced)
  SIB_STD_NOTIMPL

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::appendChild(IHTMLDOMNode *newChild, IHTMLDOMNode **node)
{
    if (!newChild || !node)
        return E_INVALIDARG;
    *node = NULL;
    if (!mDOMNode)
        return E_UNEXPECTED;
    nsCOMPtr<nsIDOMNode> domNode;
    nsresult rv = mDOMNode->AppendChild(((CIEHtmlDomNode*)newChild)->mDOMNode, getter_AddRefs(domNode));
    if (NS_FAILED(rv))
        return E_FAIL;
    
    CComPtr<IUnknown> pNode = NULL;
    HRESULT hr = CIEHtmlDomNode::FindOrCreateFromDOMNode(domNode, &pNode);
    if (FAILED(hr))
        return hr;
    if (FAILED(pNode->QueryInterface(__uuidof(IHTMLDOMNode), (void**)node)))
        return E_UNEXPECTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::get_nodeName(BSTR __RPC_FAR *p)
{
  if (!mDOMNode) return E_UNEXPECTED;
  nsString szTagName;
  HRESULT rc = mDOMNode->GetNodeName(szTagName);
  USES_CONVERSION;
  *p = SysAllocString(W2COLE(ToNewUnicode(szTagName)));
  return rc;
}

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::put_nodeValue(VARIANT p)
{
  if (!mDOMNode) return E_UNEXPECTED;
  CComVariant vValue;
  if (FAILED(vValue.ChangeType(VT_BSTR, &p))) {
    return E_INVALIDARG;
  }
  nsString szValue(OLE2W(vValue.bstrVal));
  if (!mDOMNode->SetNodeValue(szValue))
    return E_FAIL;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::get_nodeValue(VARIANT __RPC_FAR *p)
{
  if (p == NULL) {
    return E_INVALIDARG;
  }
  if (!mDOMNode) return E_UNEXPECTED;
  nsString szValue;
  nsresult nr = mDOMNode->GetNodeValue(szValue);
  if (nr == NS_OK) {
    USES_CONVERSION;
    p->vt = VT_BSTR;
    p->bstrVal = SysAllocString(W2COLE(ToNewUnicode(szValue)));
    return S_OK;
  }
  return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::get_firstChild(IHTMLDOMNode __RPC_FAR *__RPC_FAR *p)
  SIB_NODE_GET_ELEMENT(GetFirstChild,nsIDOMNode)

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::get_lastChild(IHTMLDOMNode __RPC_FAR *__RPC_FAR *p)
  SIB_NODE_GET_ELEMENT(GetLastChild,nsIDOMNode)

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::get_previousSibling(IHTMLDOMNode __RPC_FAR *__RPC_FAR *p)
{
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CIEHtmlDomNode::get_nextSibling(IHTMLDOMNode __RPC_FAR *__RPC_FAR *p)
{
  return E_NOTIMPL;
}
