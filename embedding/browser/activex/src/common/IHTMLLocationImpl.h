





































#ifndef IHTMLLOCATIONIMPL_H
#define IHTMLLOCATIONIMPL_H

#include "nsIDOMLocation.h"

#define IHTMLLOCATION_GET_IMPL(prop) \
    if (!p) return E_INVALIDARG; \
    nsCOMPtr<nsIDOMLocation> location; \
    if (NS_FAILED(GetDOMLocation(getter_AddRefs(location))) || !location) \
        return E_UNEXPECTED; \
    nsAutoString value; \
    NS_ENSURE_SUCCESS(location->Get ## prop(value), E_UNEXPECTED); \
    *p = ::SysAllocString(value.get()); \
    return (*p) ? S_OK : E_OUTOFMEMORY;

#define IHTMLLOCATION_PUT_IMPL(prop) \
    return E_NOTIMPL; // For now

template<class T>
class IHTMLLocationImpl :
    public IDispatchImpl<IHTMLLocation, &__uuidof(IHTMLLocation), &LIBID_MSHTML>
{
protected:

    virtual nsresult GetDOMLocation(nsIDOMLocation **aLocation) = 0;
public:
    

    virtual  HRESULT STDMETHODCALLTYPE put_href( 
         BSTR v)
    {
        IHTMLLOCATION_PUT_IMPL(Href);
    }
    virtual  HRESULT STDMETHODCALLTYPE get_href( 
         BSTR *p)
    {
        IHTMLLOCATION_GET_IMPL(Href);
    }
    virtual  HRESULT STDMETHODCALLTYPE put_protocol( 
         BSTR v)
    {
        IHTMLLOCATION_PUT_IMPL(Protocol);
    }
    virtual  HRESULT STDMETHODCALLTYPE get_protocol( 
         BSTR *p)
    {
        IHTMLLOCATION_GET_IMPL(Protocol);
    }
    virtual  HRESULT STDMETHODCALLTYPE put_host( 
         BSTR v)
    {
        IHTMLLOCATION_PUT_IMPL(Host);
    }
    virtual  HRESULT STDMETHODCALLTYPE get_host( 
         BSTR *p)
    {
        IHTMLLOCATION_GET_IMPL(Host);
    }
    virtual  HRESULT STDMETHODCALLTYPE put_hostname( 
         BSTR v)
    {
        IHTMLLOCATION_PUT_IMPL(Hostname);
    }
    virtual  HRESULT STDMETHODCALLTYPE get_hostname( 
         BSTR *p)
    {
        IHTMLLOCATION_GET_IMPL(Hostname);
    }
    virtual  HRESULT STDMETHODCALLTYPE put_port( 
         BSTR v)
    {
        IHTMLLOCATION_PUT_IMPL(Port);
    }
    virtual  HRESULT STDMETHODCALLTYPE get_port( 
         BSTR *p)
    {
        IHTMLLOCATION_GET_IMPL(Port);
    }
    virtual  HRESULT STDMETHODCALLTYPE put_pathname( 
         BSTR v)
    {
        IHTMLLOCATION_PUT_IMPL(Pathname);
    }
    virtual  HRESULT STDMETHODCALLTYPE get_pathname( 
         BSTR *p)
    {
        IHTMLLOCATION_GET_IMPL(Pathname);
    }
    virtual  HRESULT STDMETHODCALLTYPE put_search( 
         BSTR v)
    {
        IHTMLLOCATION_PUT_IMPL(Search);
    }
    virtual  HRESULT STDMETHODCALLTYPE get_search( 
         BSTR *p)
    {
        IHTMLLOCATION_GET_IMPL(Search);
    }
    virtual  HRESULT STDMETHODCALLTYPE put_hash( 
         BSTR v)
    {
        IHTMLLOCATION_PUT_IMPL(Hash);
    }
    virtual  HRESULT STDMETHODCALLTYPE get_hash( 
         BSTR *p)
    {
        IHTMLLOCATION_GET_IMPL(Hash);
    }
    virtual  HRESULT STDMETHODCALLTYPE reload( 
         VARIANT_BOOL flag)
    {
        nsCOMPtr<nsIDOMLocation> location;
        if (NS_FAILED(GetDOMLocation(getter_AddRefs(location))) || !location)
            return E_UNEXPECTED;
        return NS_SUCCEEDED(location->Reload(flag)) ? S_OK : E_FAIL;
    }
    virtual  HRESULT STDMETHODCALLTYPE replace( 
         BSTR bstr)
    {
        nsCOMPtr<nsIDOMLocation> location;
        if (NS_FAILED(GetDOMLocation(getter_AddRefs(location))) || !location)
            return E_UNEXPECTED;
        nsAutoString value(bstr);
        return NS_SUCCEEDED(location->Replace(value)) ? S_OK : E_FAIL;
    }
    virtual  HRESULT STDMETHODCALLTYPE assign( 
         BSTR bstr)
    {
        nsCOMPtr<nsIDOMLocation> location;
        if (NS_FAILED(GetDOMLocation(getter_AddRefs(location))) || !location)
            return E_UNEXPECTED;
        nsAutoString value(bstr);
        return NS_SUCCEEDED(location->Assign(value)) ? S_OK : E_FAIL;
    }
    virtual  HRESULT STDMETHODCALLTYPE toString( 
         BSTR *string)
    {
        if (!string) return E_INVALIDARG;
        nsCOMPtr<nsIDOMLocation> location;
        if (NS_FAILED(GetDOMLocation(getter_AddRefs(location))) || !location)
            return E_UNEXPECTED;
        nsAutoString value;
        NS_ENSURE_SUCCESS(location->ToString(value), E_UNEXPECTED);
        *string = ::SysAllocString(value.get());
        return (*string) ? S_OK : E_OUTOFMEMORY;
    }
};

#endif