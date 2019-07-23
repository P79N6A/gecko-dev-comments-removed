
#include "stdafx.h"

#include "XMLDocument.h"


CXMLDocument::CXMLDocument()
{
	ATLTRACE(_T("CXMLDocument::CXMLDocument()\n"));
	m_nReadyState = READYSTATE_COMPLETE;
}


CXMLDocument::~CXMLDocument()
{
}






STDMETHODIMP CXMLDocument::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IXMLDocument
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}






HRESULT STDMETHODCALLTYPE CXMLDocument::Load( LPSTREAM pStm)
{
	if (pStm == NULL)
	{
		return E_INVALIDARG;
	}

	
	STATSTG statstg;
	pStm->Stat(&statstg, STATFLAG_NONAME);

	ULONG cbBufSize = statstg.cbSize.LowPart;

	char *pBuffer = new char[cbBufSize];
	if (pBuffer == NULL)
	{
		return E_OUTOFMEMORY;
	}

	memset(pBuffer, 0, cbBufSize);
	pStm->Read(pBuffer, cbBufSize, NULL);

	m_spRoot.Release();
	ParseExpat(pBuffer, cbBufSize, (IXMLDocument *) this, &m_spRoot);

	delete []pBuffer;

	m_nReadyState = READYSTATE_LOADED;

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::Save( LPSTREAM pStm,  BOOL fClearDirty)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::GetSizeMax( ULARGE_INTEGER __RPC_FAR *pCbSize)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::InitNew(void)
{
	return S_OK;
}






HRESULT STDMETHODCALLTYPE CXMLDocument::GetClassID( CLSID __RPC_FAR *pClassID)
{
	if (pClassID == NULL)
	{
		return E_INVALIDARG;
	}
	*pClassID = CLSID_MozXMLDocument;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::IsDirty(void)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::Load( BOOL fFullyAvailable,  IMoniker __RPC_FAR *pimkName,  LPBC pibc,  DWORD grfMode)
{
	if (pimkName == NULL)
	{
		return E_INVALIDARG;
	}

	m_nReadyState = READYSTATE_LOADING;

	
	CComQIPtr<IStream, &IID_IStream> spIStream;
	if (FAILED(pimkName->BindToStorage(pibc, NULL, IID_IStream, (void **) &spIStream)))
	{
		return E_FAIL;
	}

	return Load(spIStream);
}


HRESULT STDMETHODCALLTYPE CXMLDocument::Save( IMoniker __RPC_FAR *pimkName,  LPBC pbc,  BOOL fRemember)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::SaveCompleted( IMoniker __RPC_FAR *pimkName,  LPBC pibc)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::GetCurMoniker( IMoniker __RPC_FAR *__RPC_FAR *ppimkName)
{
	return E_NOTIMPL;
}





HRESULT STDMETHODCALLTYPE CXMLDocument::GetErrorInfo(XML_ERROR __RPC_FAR *pErrorReturn)
{
	return E_NOTIMPL;
}





HRESULT STDMETHODCALLTYPE CXMLDocument::get_root( IXMLElement __RPC_FAR *__RPC_FAR *p)
{
	if (p == NULL)
	{
		return E_INVALIDARG;
	}
	*p = NULL;
	if (m_spRoot)
	{
		m_spRoot->QueryInterface(IID_IXMLElement, (void **) p);
	}
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::get_fileSize( BSTR __RPC_FAR *p)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::get_fileModifiedDate( BSTR __RPC_FAR *p)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::get_fileUpdatedDate( BSTR __RPC_FAR *p)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::get_URL( BSTR __RPC_FAR *p)
{
	if (p == NULL)
	{
		return E_INVALIDARG;
	}

	USES_CONVERSION;
	*p = SysAllocString(A2OLE(m_szURL.c_str()));
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::put_URL( BSTR p)
{
	if (p == NULL)
	{
		return E_INVALIDARG;
	}

	USES_CONVERSION;
	m_szURL = OLE2A(p);

	
	CComQIPtr<IMoniker, &IID_IMoniker> spIMoniker;
	if (FAILED(CreateURLMoniker(NULL, A2W(m_szURL.c_str()), &spIMoniker)))
	{
		return E_FAIL;
	}

	CComQIPtr<IBindCtx, &IID_IBindCtx> spIBindCtx;
	if (FAILED(CreateBindCtx(0, &spIBindCtx)))
	{
		return E_FAIL;
	}

	if (FAILED(Load(TRUE, spIMoniker, spIBindCtx, 0)))
	{
		return E_FAIL;
	}

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::get_mimeType( BSTR __RPC_FAR *p)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::get_readyState( long __RPC_FAR *pl)
{
	if (pl == NULL)
	{
		return E_INVALIDARG;
	}
	*pl = m_nReadyState;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::get_charset( BSTR __RPC_FAR *p)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::put_charset( BSTR p)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::get_version( BSTR __RPC_FAR *p)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::get_doctype( BSTR __RPC_FAR *p)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::get_dtdURL( BSTR __RPC_FAR *p)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLDocument::createElement( VARIANT vType,  VARIANT var1,  IXMLElement __RPC_FAR *__RPC_FAR *ppElem)
{
	if (vType.vt != VT_I4)
	{
		return E_INVALIDARG;
	}
	if (ppElem == NULL)
	{
		return E_INVALIDARG;
	}

	CXMLElementInstance *pInstance = NULL;
	CXMLElementInstance::CreateInstance(&pInstance);
	if (pInstance == NULL)
	{
		return E_OUTOFMEMORY;
	}

	IXMLElement *pElement = NULL;
	if (FAILED(pInstance->QueryInterface(IID_IXMLElement, (void **) &pElement)))
	{
		pInstance->Release();
		return E_NOINTERFACE;
	}

	
	long nType = vType.intVal;
	pInstance->PutType(nType);

	
	if (var1.vt == VT_BSTR)
	{
		pInstance->put_tagName(var1.bstrVal);
	}
	
	*ppElem = pElement;
	return S_OK;
}


