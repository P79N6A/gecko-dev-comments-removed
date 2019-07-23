



































#include "stdafx.h"

#include "Pluginhostctrl.h"
#include "nsPluginHostCtrl.h"




nsPluginHostCtrl::nsPluginHostCtrl()
{
    m_bWindowOnly = TRUE;
}

nsPluginHostCtrl::~nsPluginHostCtrl()
{
}


HRESULT nsPluginHostCtrl::GetWebBrowserApp(IWebBrowserApp **pBrowser)
{
    ATLASSERT(pBrowser);
    if (!pBrowser)
    {
        return E_INVALIDARG;
    }

    
    
    

    CComPtr<IWebBrowserApp> cpWebBrowser;
    CComQIPtr<IServiceProvider, &IID_IServiceProvider> cpServiceProvider = m_spClientSite;

    HRESULT hr;
    if (cpServiceProvider)
    {
        hr = cpServiceProvider->QueryService(IID_IWebBrowserApp, &cpWebBrowser);
    }
    if (!cpWebBrowser)
    {
        return E_FAIL;
    }

    *pBrowser = cpWebBrowser;
    (*pBrowser)->AddRef();

    return S_OK;
}




STDMETHODIMP nsPluginHostCtrl::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    CComQIPtr<IPropertyBag2> cpPropBag2 = pPropBag;
    if (cpPropBag2)
    {
        
        
        ULONG nProperties;
        cpPropBag2->CountProperties(&nProperties);
        if (nProperties > 0)
        {
            PROPBAG2 *pProperties = (PROPBAG2 *) malloc(sizeof(PROPBAG2) * nProperties);
            ULONG nPropertiesGotten = 0;
            cpPropBag2->GetPropertyInfo(0, nProperties, pProperties, &nPropertiesGotten);
            for (ULONG i = 0; i < nPropertiesGotten; i++)
            {
                if (pProperties[i].vt == VT_BSTR)
                {
                    USES_CONVERSION;
                    CComVariant v;
                    HRESULT hrRead;
                    cpPropBag2->Read(1, &pProperties[i], NULL, &v, &hrRead);
                    AddPluginParam(OLE2A(pProperties[i].pstrName), OLE2A(v.bstrVal));
                }
                if (pProperties[i].pstrName)
                {
                    CoTaskMemFree(pProperties[i].pstrName);
                }
            }
            free(pProperties);
        }
    }
    return IPersistPropertyBagImpl<nsPluginHostCtrl>::Load(pPropBag, pErrorLog);
}




STDMETHODIMP nsPluginHostCtrl::get_PluginContentType(BSTR *pVal)
{
    return GetPluginContentType(pVal);
}

STDMETHODIMP nsPluginHostCtrl::put_PluginContentType(BSTR newVal)
{
    return SetPluginContentType(newVal);
}

STDMETHODIMP nsPluginHostCtrl::get_PluginSource(BSTR *pVal)
{
    return GetPluginSource(pVal);
}

STDMETHODIMP nsPluginHostCtrl::put_PluginSource(BSTR newVal)
{
    return SetPluginSource(newVal);
}

STDMETHODIMP nsPluginHostCtrl::get_PluginsPage(BSTR *pVal)
{
    return GetPluginsPage(pVal);
}

STDMETHODIMP nsPluginHostCtrl::put_PluginsPage(BSTR newVal)
{
    return SetPluginsPage(newVal);
}
