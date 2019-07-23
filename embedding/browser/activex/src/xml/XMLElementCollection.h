

#ifndef __XMLELEMENTCOLLECTION_H_
#define __XMLELEMENTCOLLECTION_H_

#include "resource.h"       



class ATL_NO_VTABLE CXMLElementCollection : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CXMLElementCollection, &CLSID_MozXMLElementCollection>,
	public IDispatchImpl<IXMLElementCollection, &IID_IXMLElementCollection, &LIBID_MozActiveXMLLib>
{
	
	ElementList m_cElements;

public:
	CXMLElementCollection();
	virtual ~CXMLElementCollection();

DECLARE_REGISTRY_RESOURCEID(IDR_XMLELEMENTCOLLECTION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CXMLElementCollection)
	COM_INTERFACE_ENTRY(IXMLElementCollection)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()


	virtual HRESULT STDMETHODCALLTYPE put_length( long v);
	virtual HRESULT STDMETHODCALLTYPE get_length( long __RPC_FAR *p);
	virtual HRESULT STDMETHODCALLTYPE get__newEnum( IUnknown __RPC_FAR *__RPC_FAR *ppUnk);
	virtual HRESULT STDMETHODCALLTYPE item( VARIANT var1,  VARIANT var2,  IDispatch __RPC_FAR *__RPC_FAR *ppDisp);

public:
	HRESULT Add(IXMLElement *pElement);
};

typedef CComObject<CXMLElementCollection> CXMLElementCollectionInstance;

#endif 
