
#include "stdafx.h"

#include "XMLElementCollection.h"

CXMLElementCollection::CXMLElementCollection()
{
}


CXMLElementCollection::~CXMLElementCollection()
{
}


HRESULT CXMLElementCollection::Add(IXMLElement *pElement)
{
	if (pElement == NULL)
	{
		return E_INVALIDARG;
	}

	m_cElements.push_back( CComQIPtr<IXMLElement, &IID_IXMLElement>(pElement));
	return S_OK;
}





HRESULT STDMETHODCALLTYPE CXMLElementCollection::put_length( long v)
{
	
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CXMLElementCollection::get_length( long __RPC_FAR *p)
{
	if (p == NULL)
	{
		return E_INVALIDARG;
	}
	*p = m_cElements.size();
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CXMLElementCollection::get__newEnum( IUnknown __RPC_FAR *__RPC_FAR *ppUnk)
{
	return E_NOTIMPL;
}



HRESULT STDMETHODCALLTYPE CXMLElementCollection::item( VARIANT var1,  VARIANT var2,  IDispatch __RPC_FAR *__RPC_FAR *ppDisp)
{
	if (ppDisp == NULL)
	{
		return E_INVALIDARG;
	}

	*ppDisp;

	CComVariant vIndex;

	
	
	if (vIndex.ChangeType(VT_I4, &var1) == S_OK)
	{
		long nIndex = vIndex.intVal;
		if (nIndex < 0 || nIndex >= m_cElements.size())
		{
			return E_INVALIDARG;
		}
		
		m_cElements[nIndex]->QueryInterface(IID_IDispatch, (void **) ppDisp);
		return S_OK;
	}

	
	
	
	
	CComVariant vName;
	if (FAILED(vName.ChangeType(VT_BSTR, &var1)))
	{
		return E_INVALIDARG;
	}

	
	ElementList cElements;
	ElementList::iterator i;

	for (i = m_cElements.begin(); i != m_cElements.end(); i++)
	{
		CComQIPtr<IXMLElement, &IID_IXMLElement> spElement;
		BSTR bstrTagName = NULL;
		(*i)->get_tagName(&bstrTagName);
		if (bstrTagName)
		{
			if (wcscmp(bstrTagName, vName.bstrVal) == 0)
			{
				cElements.push_back(*i);
			}
			SysFreeString(bstrTagName);
		}
	}

	
	if (cElements.empty())
	{
		return S_OK;
	}

	
	if (var2.vt == VT_I4)
	{
		long nIndex = var2.vt;
		if (nIndex < 0 || nIndex >= cElements.size())
		{
			return E_INVALIDARG;
		}

		
		cElements[nIndex]->QueryInterface(IID_IDispatch, (void **) ppDisp);
		return S_OK;
	}

	
	if (cElements.size() > 1)
	{
		
		CXMLElementCollectionInstance *pCollection = NULL;
		CXMLElementCollectionInstance::CreateInstance(&pCollection);
		if (pCollection == NULL)
		{
			return E_OUTOFMEMORY;
		}

		if (FAILED(pCollection->QueryInterface(IID_IDispatch, (void **) ppDisp)))
		{
			pCollection->Release();
			return E_FAIL;
		}

		
		for (i = cElements.begin(); i != cElements.end(); i++)
		{
			pCollection->Add(*i);
		}

		return S_OK;
	}

	
	if (FAILED(cElements[0]->QueryInterface(IID_IDispatch, (void **) ppDisp)))
	{
		return E_FAIL;
	}

	return S_OK;
}

