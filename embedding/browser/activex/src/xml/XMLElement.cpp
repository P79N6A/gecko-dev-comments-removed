
#include "stdafx.h"

#include "XMLElement.h"


CXMLElement::CXMLElement()
{
	m_nType = 0;
	m_pParent = NULL;
}


CXMLElement::~CXMLElement()
{
}


HRESULT CXMLElement::SetParent(IXMLElement *pParent)
{
	
	m_pParent = pParent;
	return S_OK;
}


HRESULT CXMLElement::PutType(long nType)
{
	m_nType = nType;
	return S_OK;
}


HRESULT CXMLElement::ReleaseAll()
{
	
	m_cChildren.clear();
	return S_OK;
}







HRESULT STDMETHODCALLTYPE CXMLElement::get_tagName( BSTR __RPC_FAR *p)
{
	if (p == NULL)
	{
		return E_INVALIDARG;
	}
	USES_CONVERSION;
	*p = SysAllocString(A2OLE(m_szTagName.c_str()));
	return S_OK;
}



HRESULT STDMETHODCALLTYPE CXMLElement::put_tagName( BSTR p)
{
	if (p == NULL)
	{
		return E_INVALIDARG;
	}
	USES_CONVERSION;
	m_szTagName = OLE2A(p);
	return S_OK;
}



HRESULT STDMETHODCALLTYPE CXMLElement::get_parent( IXMLElement __RPC_FAR *__RPC_FAR *ppParent)
{
	if (ppParent == NULL)
	{
		return E_INVALIDARG;
	}

	*ppParent = NULL;
	if (m_pParent)
	{
		return m_pParent->QueryInterface(IID_IXMLElement, (void **) ppParent);
	}

	return S_OK;
}



HRESULT STDMETHODCALLTYPE CXMLElement::setAttribute( BSTR strPropertyName,  VARIANT PropertyValue)
{
	if (strPropertyName == NULL || PropertyValue.vt != VT_BSTR)
	{
		return E_INVALIDARG;
	}

	USES_CONVERSION;
	std::string szPropertyName = OLE2A(strPropertyName);
	std::string szPropertyValue = OLE2A(PropertyValue.bstrVal);
	m_cAttributes[szPropertyName] = szPropertyValue;

	return S_OK;
}



HRESULT STDMETHODCALLTYPE CXMLElement::getAttribute( BSTR strPropertyName,  VARIANT __RPC_FAR *PropertyValue)
{
	if (strPropertyName == NULL || PropertyValue == NULL)
	{
		return E_INVALIDARG;
	}

	USES_CONVERSION;
	std::string szPropertyName = OLE2A(strPropertyName);
	StringMap::iterator i = m_cAttributes.find(szPropertyName);
	if (i == m_cAttributes.end())
	{
		return S_FALSE;
	}

	PropertyValue->vt = VT_BSTR;
	PropertyValue->bstrVal = SysAllocString(A2OLE((*i).second.c_str()));
	return S_OK;
}



HRESULT STDMETHODCALLTYPE CXMLElement::removeAttribute( BSTR strPropertyName)
{
	if (strPropertyName == NULL)
	{
		return E_INVALIDARG;
	}

	USES_CONVERSION;
	std::string szPropertyName = OLE2A(strPropertyName);
	StringMap::iterator i = m_cAttributes.find(szPropertyName);
	if (i == m_cAttributes.end())
	{
		return E_INVALIDARG;
	}
	
	m_cAttributes.erase(i);
	
	return S_OK;
}



HRESULT STDMETHODCALLTYPE CXMLElement::get_children( IXMLElementCollection __RPC_FAR *__RPC_FAR *pp)
{
	CXMLElementCollectionInstance *pCollection = NULL;
	CXMLElementCollectionInstance::CreateInstance(&pCollection);
	if (pCollection == NULL)
	{
		return E_OUTOFMEMORY;
	}

	
	for (ElementList::iterator i = m_cChildren.begin(); i != m_cChildren.end(); i++)
	{
		pCollection->Add(*i);
	}

	pCollection->QueryInterface(IID_IXMLElementCollection, (void **) pp);

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CXMLElement::get_type( long __RPC_FAR *plType)
{
	if (plType == NULL)
	{
		return E_INVALIDARG;
	}
	*plType = m_nType;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CXMLElement::get_text( BSTR __RPC_FAR *p)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLElement::put_text( BSTR p)
{
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CXMLElement::addChild( IXMLElement __RPC_FAR *pChildElem, long lIndex, long lReserved)
{
	if (pChildElem == NULL)
	{
		return E_INVALIDARG;
	}

	
	((CXMLElement *) pChildElem)->SetParent(this);

	if (lIndex < 0 || lIndex >= m_cChildren.size())
	{
		
		m_cChildren.push_back(pChildElem);
	}
	else
	{

		m_cChildren.push_back(pChildElem);
	}

	return S_OK;
}


HRESULT STDMETHODCALLTYPE CXMLElement::removeChild( IXMLElement __RPC_FAR *pChildElem)
{
	
	return E_NOTIMPL;
}

