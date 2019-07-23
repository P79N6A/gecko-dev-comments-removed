




































#include "StdAfx.h"

#include "PropertyBag.h"


CPropertyBag::CPropertyBag()
{
}


CPropertyBag::~CPropertyBag()
{
}





HRESULT STDMETHODCALLTYPE CPropertyBag::Read( LPCOLESTR pszPropName,  VARIANT __RPC_FAR *pVar,  IErrorLog __RPC_FAR *pErrorLog)
{
    if (pszPropName == NULL)
    {
        return E_INVALIDARG;
    }
    if (pVar == NULL)
    {
        return E_INVALIDARG;
    }

    VARTYPE vt = pVar->vt;
    VariantInit(pVar);

    for (unsigned long i = 0; i < m_PropertyList.GetSize(); i++)
    {
        if (wcsicmp(m_PropertyList.GetNameOf(i), pszPropName) == 0)
        {
            const VARIANT *pvSrc = m_PropertyList.GetValueOf(i);
            if (!pvSrc)
            {
                return E_FAIL;
            }
            CComVariant vNew;
            HRESULT hr = (vt == VT_EMPTY) ?
                vNew.Copy(pvSrc) : vNew.ChangeType(vt, pvSrc);
            if (FAILED(hr))
            {
                return E_FAIL;
            }
            
            vNew.Detach(pVar);
            return S_OK;
        }
    }

    
    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE CPropertyBag::Write( LPCOLESTR pszPropName,  VARIANT __RPC_FAR *pVar)
{
    if (pszPropName == NULL)
    {
        return E_INVALIDARG;
    }
    if (pVar == NULL)
    {
        return E_INVALIDARG;
    }

    CComBSTR bstrName(pszPropName);
    m_PropertyList.AddOrReplaceNamedProperty(bstrName, *pVar);

    return S_OK;
}

