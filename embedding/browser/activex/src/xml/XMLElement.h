

#ifndef __XMLELEMENT_H_
#define __XMLELEMENT_H_

#include "resource.h"       

#include <vector>
#include <string>
#include <map>

typedef std::map<std::string, std::string> StringMap;
typedef std::vector< CComQIPtr<IXMLElement, &IID_IXMLElement> > ElementList;



class ATL_NO_VTABLE CXMLElement : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CXMLElement, &CLSID_MozXMLElement>,
	public IDispatchImpl<IXMLElement, &IID_IXMLElement, &LIBID_MozActiveXMLLib>
{
	
	IXMLElement *m_pParent;
	
	ElementList m_cChildren;
	
	std::string m_szTagName;
	
	std::string m_szText;
	
	long m_nType;
	
	StringMap m_cAttributes;

public:
	CXMLElement();
	virtual ~CXMLElement();

	virtual HRESULT SetParent(IXMLElement *pParent);
	virtual HRESULT PutType(long nType);
	virtual HRESULT ReleaseAll();

DECLARE_REGISTRY_RESOURCEID(IDR_XMLELEMENT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CXMLElement)
	COM_INTERFACE_ENTRY(IXMLElement)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()


	virtual  HRESULT STDMETHODCALLTYPE get_tagName( BSTR __RPC_FAR *p);
	virtual  HRESULT STDMETHODCALLTYPE put_tagName( BSTR p);
	virtual  HRESULT STDMETHODCALLTYPE get_parent( IXMLElement __RPC_FAR *__RPC_FAR *ppParent);
	virtual  HRESULT STDMETHODCALLTYPE setAttribute( BSTR strPropertyName,  VARIANT PropertyValue);
	virtual  HRESULT STDMETHODCALLTYPE getAttribute( BSTR strPropertyName,  VARIANT __RPC_FAR *PropertyValue);
	virtual  HRESULT STDMETHODCALLTYPE removeAttribute( BSTR strPropertyName);
	virtual  HRESULT STDMETHODCALLTYPE get_children( IXMLElementCollection __RPC_FAR *__RPC_FAR *pp);
	virtual  HRESULT STDMETHODCALLTYPE get_type( long __RPC_FAR *plType);
	virtual  HRESULT STDMETHODCALLTYPE get_text( BSTR __RPC_FAR *p);
	virtual  HRESULT STDMETHODCALLTYPE put_text( BSTR p);
	virtual  HRESULT STDMETHODCALLTYPE addChild( IXMLElement __RPC_FAR *pChildElem, long lIndex, long lReserved);
	virtual  HRESULT STDMETHODCALLTYPE removeChild( IXMLElement __RPC_FAR *pChildElem);

public:
};

typedef CComObject<CXMLElement> CXMLElementInstance;

#endif 
