

#ifndef __XMLDOCUMENT_H_
#define __XMLDOCUMENT_H_

#include "resource.h"       



class ATL_NO_VTABLE CXMLDocument : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CXMLDocument, &CLSID_MozXMLDocument>,
	public ISupportErrorInfo,
	public IDispatchImpl<IXMLDocument, &IID_IXMLDocument, &LIBID_MozActiveXMLLib>,
	public IPersistMoniker,
	public IPersistStreamInit
{
public:
	CXMLDocument();
	virtual ~CXMLDocument();


DECLARE_REGISTRY_RESOURCEID(IDR_XMLDOCUMENT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CXMLDocument)
	COM_INTERFACE_ENTRY(IXMLDocument)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IPersistMoniker)
	COM_INTERFACE_ENTRY(IPersistStreamInit)

END_COM_MAP()

	LONG m_nReadyState;
	std::string m_szURL;
	CComQIPtr<IXMLElement, &IID_IXMLElement> m_spRoot;


	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


	
	HRESULT STDMETHODCALLTYPE Load( LPSTREAM pStm);
	HRESULT STDMETHODCALLTYPE Save( LPSTREAM pStm,  BOOL fClearDirty);
	HRESULT STDMETHODCALLTYPE GetSizeMax( ULARGE_INTEGER __RPC_FAR *pCbSize);
	HRESULT STDMETHODCALLTYPE InitNew(void);


	HRESULT STDMETHODCALLTYPE GetClassID( CLSID __RPC_FAR *pClassID);
	HRESULT STDMETHODCALLTYPE IsDirty(void);
	HRESULT STDMETHODCALLTYPE Load( BOOL fFullyAvailable,  IMoniker __RPC_FAR *pimkName,  LPBC pibc,  DWORD grfMode);
	HRESULT STDMETHODCALLTYPE Save( IMoniker __RPC_FAR *pimkName,  LPBC pbc,  BOOL fRemember);
	HRESULT STDMETHODCALLTYPE SaveCompleted( IMoniker __RPC_FAR *pimkName,  LPBC pibc);
	HRESULT STDMETHODCALLTYPE GetCurMoniker( IMoniker __RPC_FAR *__RPC_FAR *ppimkName);


	HRESULT STDMETHODCALLTYPE GetErrorInfo(XML_ERROR __RPC_FAR *pErrorReturn);


	HRESULT STDMETHODCALLTYPE get_root( IXMLElement __RPC_FAR *__RPC_FAR *p);
	HRESULT STDMETHODCALLTYPE get_fileSize( BSTR __RPC_FAR *p);
	HRESULT STDMETHODCALLTYPE get_fileModifiedDate( BSTR __RPC_FAR *p);
	HRESULT STDMETHODCALLTYPE get_fileUpdatedDate( BSTR __RPC_FAR *p);
	HRESULT STDMETHODCALLTYPE get_URL( BSTR __RPC_FAR *p);
	HRESULT STDMETHODCALLTYPE put_URL( BSTR p);
	HRESULT STDMETHODCALLTYPE get_mimeType( BSTR __RPC_FAR *p);
	HRESULT STDMETHODCALLTYPE get_readyState( long __RPC_FAR *pl);
	HRESULT STDMETHODCALLTYPE get_charset( BSTR __RPC_FAR *p);
	HRESULT STDMETHODCALLTYPE put_charset( BSTR p);
	HRESULT STDMETHODCALLTYPE get_version( BSTR __RPC_FAR *p);
	HRESULT STDMETHODCALLTYPE get_doctype( BSTR __RPC_FAR *p);
	HRESULT STDMETHODCALLTYPE get_dtdURL( BSTR __RPC_FAR *p);
	HRESULT STDMETHODCALLTYPE createElement( VARIANT vType,  VARIANT var1,  IXMLElement __RPC_FAR *__RPC_FAR *ppElem);
public:
};

typedef CComObject<CXMLDocument> CXMLDocumentInstance;

#endif
