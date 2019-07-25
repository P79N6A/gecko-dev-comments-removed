




































#ifndef IEHTMLNODECOLLECTION_H
#define IEHTMLNODECOLLECTION_H

#include "nsIDOMHTMLCollection.h"
#include "nsIDOMNodeList.h"

#include "IEHtmlNode.h"

class CIEHtmlElement;

class CIEHtmlElementCollection :
    public CNode,
    public IDispatchImpl<IHTMLElementCollection, &IID_IHTMLElementCollection, &LIBID_MSHTML>
{
private:
    
    nsCOMPtr<nsIDOMNodeList> mDOMNodeList;
    
    IDispatch  **mNodeList;
    PRUint32     mNodeListCount;
    PRUint32     mNodeListCapacity;

public:
    CIEHtmlElementCollection();

protected:
    virtual ~CIEHtmlElementCollection();
    virtual HRESULT FindOrCreateIEElement(nsIDOMNode* domNode, IHTMLElement** pIHtmlElement);

public:
    
    virtual HRESULT AddNode(IDispatch *pNode);

    virtual HRESULT PopulateFromDOMHTMLCollection(nsIDOMHTMLCollection *pNodeList);

    
    virtual HRESULT PopulateFromDOMNode(nsIDOMNode *pIDOMNode, BOOL bRecurseChildren);

    
    static HRESULT CreateFromParentNode(CNode *pParentNode, BOOL bRecurseChildren, CIEHtmlElementCollection **pInstance);

    
    static HRESULT CreateFromDOMHTMLCollection(CNode *pParentNode, nsIDOMHTMLCollection *pNodeList, CIEHtmlElementCollection **pInstance);


BEGIN_COM_MAP(CIEHtmlElementCollection)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IHTMLElementCollection)
END_COM_MAP()

    
    virtual HRESULT STDMETHODCALLTYPE toString(BSTR __RPC_FAR *String);
    virtual HRESULT STDMETHODCALLTYPE put_length(long v);
    virtual HRESULT STDMETHODCALLTYPE get_length(long __RPC_FAR *p);
    virtual HRESULT STDMETHODCALLTYPE get__newEnum(IUnknown __RPC_FAR *__RPC_FAR *p);
    virtual HRESULT STDMETHODCALLTYPE item(VARIANT name, VARIANT index, IDispatch __RPC_FAR *__RPC_FAR *pdisp);
    virtual HRESULT STDMETHODCALLTYPE tags(VARIANT tagName, IDispatch __RPC_FAR *__RPC_FAR *pdisp);
};

typedef CComObject<CIEHtmlElementCollection> CIEHtmlElementCollectionInstance;

#endif