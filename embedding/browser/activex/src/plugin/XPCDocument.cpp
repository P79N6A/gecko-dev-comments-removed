





































#include "StdAfx.h"

#include <mshtml.h>
#include <hlink.h>

















#ifdef USE_HTIFACE
#include <htiface.h>
#endif
#ifndef __ITargetFrame_INTERFACE_DEFINED__

MIDL_INTERFACE("d5f78c80-5252-11cf-90fa-00AA0042106e")
IMozTargetFrame : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE SetFrameName(
         LPCWSTR pszFrameName) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFrameName(
         LPWSTR *ppszFrameName) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetParentFrame(
         IUnknown **ppunkParent) = 0;
    virtual HRESULT STDMETHODCALLTYPE FindFrame( 
         LPCWSTR pszTargetName,
         IUnknown *ppunkContextFrame,
         DWORD dwFlags,
         IUnknown **ppunkTargetFrame) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFrameSrc( 
         LPCWSTR pszFrameSrc) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFrameSrc( 
         LPWSTR *ppszFrameSrc) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFramesContainer( 
         IOleContainer **ppContainer) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFrameOptions( 
         DWORD dwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFrameOptions( 
         DWORD *pdwFlags) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFrameMargins( 
         DWORD dwWidth,
         DWORD dwHeight) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFrameMargins( 
         DWORD *pdwWidth,
         DWORD *pdwHeight) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoteNavigate( 
         ULONG cLength,
         ULONG *pulData) = 0;
    virtual HRESULT STDMETHODCALLTYPE OnChildFrameActivate( 
         IUnknown *pUnkChildFrame) = 0;
    virtual HRESULT STDMETHODCALLTYPE OnChildFrameDeactivate( 
         IUnknown *pUnkChildFrame) = 0;
};
#define __ITargetFrame_INTERFACE_DEFINED__
#define ITargetFrame IMozTargetFrame
#endif

#include "npapi.h"

#include "nsCOMPtr.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsStringAPI.h"
#include "nsNetUtil.h"

#include "nsIContent.h"
#include "nsIURI.h"
#include "nsIDocument.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMLocation.h"
#include "nsIWebNavigation.h"
#include "nsILinkHandler.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIPrincipal.h"
#include "nsPIDOMWindow.h"

#include "XPConnect.h"
#include "XPCBrowser.h"
#include "LegacyPlugin.h"

#include "IEHtmlElementCollection.h"
#include "IHTMLLocationImpl.h"









class IELocation :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IHTMLLocationImpl<IELocation>
{
public:
BEGIN_COM_MAP(IELocation)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IHTMLLocation)
END_COM_MAP()

    PluginInstanceData *mData;
    nsCOMPtr<nsIDOMLocation> mDOMLocation;

    IELocation() : mData(NULL)
    {
    }

    HRESULT Init(PluginInstanceData *pData)
    {
        NS_PRECONDITION(pData != nsnull, "null ptr");

        mData = pData;

        
        nsCOMPtr<nsIDOMWindow> domWindow;
        NPN_GetValue(mData->pPluginInstance, NPNVDOMWindow, 
                     static_cast<nsIDOMWindow **>(getter_AddRefs(domWindow)));
        if (!domWindow)
        {
            return E_FAIL;
        }
        nsCOMPtr<nsIDOMWindowInternal> windowInternal = do_QueryInterface(domWindow);
        if (windowInternal)
        {
            windowInternal->GetLocation(getter_AddRefs(mDOMLocation));
        }
        if (!mDOMLocation)
            return E_FAIL;

        return S_OK;
    }

    virtual nsresult GetDOMLocation(nsIDOMLocation **aLocation)
    {
        *aLocation = mDOMLocation;
        NS_IF_ADDREF(*aLocation);
        return NS_OK;
    }
};


class IENavigator :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IOmNavigator, &__uuidof(IOmNavigator), &LIBID_MSHTML>
{
public:
BEGIN_COM_MAP(IENavigator)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IOmNavigator)
END_COM_MAP()

    PluginInstanceData *mData;
    CComBSTR mUserAgent;

    HRESULT Init(PluginInstanceData *pData)
    {
        mData = pData;
        USES_CONVERSION;
        const char *userAgent = NPN_UserAgent(mData->pPluginInstance);
        mUserAgent.Attach(::SysAllocString(A2CW(userAgent)));
        return S_OK;
    }


    virtual  HRESULT STDMETHODCALLTYPE get_appCodeName( 
         BSTR __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }

    virtual  HRESULT STDMETHODCALLTYPE get_appName( 
         BSTR __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }

    virtual  HRESULT STDMETHODCALLTYPE get_appVersion( 
         BSTR __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }

    virtual  HRESULT STDMETHODCALLTYPE get_userAgent( 
         BSTR __RPC_FAR *p)
    {
        *p = mUserAgent.Copy();
        return S_OK;
    }

    virtual  HRESULT STDMETHODCALLTYPE javaEnabled( 
         VARIANT_BOOL __RPC_FAR *enabled)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE taintEnabled( 
         VARIANT_BOOL __RPC_FAR *enabled)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE get_mimeTypes( 
         IHTMLMimeTypesCollection __RPC_FAR *__RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE get_plugins( 
         IHTMLPluginsCollection __RPC_FAR *__RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE get_cookieEnabled( 
         VARIANT_BOOL __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE get_opsProfile( 
         IHTMLOpsProfile __RPC_FAR *__RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE toString( 
         BSTR __RPC_FAR *string)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE get_cpuClass( 
         BSTR __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE get_systemLanguage( 
         BSTR __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE get_browserLanguage( 
         BSTR __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE get_userLanguage( 
         BSTR __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE get_platform( 
         BSTR __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE get_appMinorVersion( 
         BSTR __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE get_connectionSpeed( 
         long __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE get_onLine( 
         VARIANT_BOOL __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
        
    virtual  HRESULT STDMETHODCALLTYPE get_userProfile( 
         IHTMLOpsProfile __RPC_FAR *__RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
};



class IEWindow :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IHTMLWindow2, &__uuidof(IHTMLWindow2), &LIBID_MSHTML>
{
public:
    PluginInstanceData *mData;
    CComObject<IENavigator> *mNavigator;
    CComObject<IELocation>  *mLocation;

    IEWindow() : mNavigator(NULL), mLocation(NULL), mData(NULL)
    {
    }

    HRESULT Init(PluginInstanceData *pData)
    {
        mData = pData;

        CComObject<IENavigator>::CreateInstance(&mNavigator);
        if (!mNavigator)
        {
            return E_UNEXPECTED;
        }
        mNavigator->AddRef();
        mNavigator->Init(mData);

        CComObject<IELocation>::CreateInstance(&mLocation);
        if (!mLocation)
        {
            return E_UNEXPECTED;
        }
        mLocation->AddRef();
        mLocation->Init(mData);

        return S_OK;
    }

protected:
    virtual ~IEWindow()
    {
        if (mNavigator)
        {
            mNavigator->Release();
        }
        if (mLocation)
        {
            mLocation->Release();
        }
    }

public:
    
BEGIN_COM_MAP(IEWindow)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IHTMLWindow2)
    COM_INTERFACE_ENTRY(IHTMLFramesCollection2)
    COM_INTERFACE_ENTRY_BREAK(IHlinkFrame)
END_COM_MAP()


    virtual  HRESULT STDMETHODCALLTYPE item( 
         VARIANT __RPC_FAR *pvarIndex,
         VARIANT __RPC_FAR *pvarResult)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_length( 
         long __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }


    virtual  HRESULT STDMETHODCALLTYPE get_frames( 
         IHTMLFramesCollection2 __RPC_FAR *__RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_defaultStatus( 
         BSTR v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_defaultStatus( 
         BSTR __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_status( 
         BSTR v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_status( 
         BSTR __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE setTimeout( 
         BSTR expression,
         long msec,
         VARIANT __RPC_FAR *language,
         long __RPC_FAR *timerID)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE clearTimeout( 
         long timerID)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE alert( 
         BSTR message)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE confirm( 
         BSTR message,
         VARIANT_BOOL __RPC_FAR *confirmed)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE prompt( 
         BSTR message,
         BSTR defstr,
         VARIANT __RPC_FAR *textdata)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_Image( 
         IHTMLImageElementFactory __RPC_FAR *__RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_location( 
         IHTMLLocation __RPC_FAR *__RPC_FAR *p)
    {
        if (mLocation)
            return mLocation->QueryInterface(__uuidof(IHTMLLocation), (void **) p);
        return E_FAIL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_history( 
         IOmHistory __RPC_FAR *__RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE close( void)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_opener( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_opener( 
         VARIANT __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_navigator( 
         IOmNavigator __RPC_FAR *__RPC_FAR *p)
    {
        if (mNavigator)
            return mNavigator->QueryInterface(__uuidof(IOmNavigator), (void **) p);
        return E_FAIL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_name( 
         BSTR v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_name( 
         BSTR __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_parent( 
         IHTMLWindow2 __RPC_FAR *__RPC_FAR *p)
    {
        *p = NULL;
        return S_OK;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE open( 
         BSTR url,
         BSTR name,
         BSTR features,
         VARIANT_BOOL replace,
         IHTMLWindow2 __RPC_FAR *__RPC_FAR *pomWindowResult)
    {
        *pomWindowResult = NULL;
        return E_FAIL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_self( 
         IHTMLWindow2 __RPC_FAR *__RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_top( 
         IHTMLWindow2 __RPC_FAR *__RPC_FAR *p)
    {
        *p = NULL;
        return S_OK;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_window( 
         IHTMLWindow2 __RPC_FAR *__RPC_FAR *p)
    {
        *p = NULL;
        return S_OK;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE navigate( 
         BSTR url)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onfocus( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onfocus( 
         VARIANT __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onblur( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }

    virtual  HRESULT STDMETHODCALLTYPE get_onblur( 
         VARIANT __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onload( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onload( 
         VARIANT __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onbeforeunload( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onbeforeunload( 
         VARIANT __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onunload( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onunload( 
         VARIANT __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onhelp( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onhelp( 
         VARIANT __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onerror( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onerror( 
         VARIANT __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onresize( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onresize( 
         VARIANT __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onscroll( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onscroll( 
         VARIANT __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_document( 
         IHTMLDocument2 __RPC_FAR *__RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_event( 
         IHTMLEventObj __RPC_FAR *__RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get__newEnum( 
         IUnknown __RPC_FAR *__RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE showModalDialog( 
         BSTR dialog,
         VARIANT __RPC_FAR *varArgIn,
         VARIANT __RPC_FAR *varOptions,
         VARIANT __RPC_FAR *varArgOut)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE showHelp( 
         BSTR helpURL,
         VARIANT helpArg,
         BSTR features)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_screen( 
         IHTMLScreen __RPC_FAR *__RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_Option( 
         IHTMLOptionElementFactory __RPC_FAR *__RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE focus( void)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_closed( 
         VARIANT_BOOL __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE blur( void)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE scroll( 
         long x,
         long y)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_clientInformation( 
         IOmNavigator __RPC_FAR *__RPC_FAR *p)
    {
        return get_navigator(p);
    }
    
    virtual  HRESULT STDMETHODCALLTYPE setInterval( 
         BSTR expression,
         long msec,
         VARIANT __RPC_FAR *language,
         long __RPC_FAR *timerID)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE clearInterval( 
         long timerID)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_offscreenBuffering( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_offscreenBuffering( 
         VARIANT __RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE execScript( 
         BSTR code,
         BSTR language,
         VARIANT __RPC_FAR *pvarRet)
    {
        nsresult rv;

        nsCOMPtr<nsIDOMWindow> domWindow;
        NPN_GetValue(mData->pPluginInstance, NPNVDOMWindow, 
                     static_cast<nsIDOMWindow **>(getter_AddRefs(domWindow)));
        if (!domWindow)
        {
            return E_UNEXPECTED;
        }

        
        
        
        nsCOMPtr<nsIDOMDocument> domDocument;
        rv = domWindow->GetDocument(getter_AddRefs(domDocument));
        NS_ASSERTION(domDocument, "No DOMDocument!");
        if (NS_FAILED(rv)) {
            return E_UNEXPECTED;
        }

        nsCOMPtr<nsIScriptGlobalObject> globalObject(do_QueryInterface(domWindow));
        if (!globalObject)
            return E_UNEXPECTED;

        nsCOMPtr<nsIScriptContext> scriptContext = globalObject->GetContext();
        if (!scriptContext)
            return E_UNEXPECTED;

        nsCOMPtr<nsIScriptObjectPrincipal> doc(do_QueryInterface(domDocument));
        if (!doc)
            return E_UNEXPECTED;

        nsIPrincipal *principal = doc->GetPrincipal();
        if (!principal)
            return E_UNEXPECTED;

        
        
        
        
        
        nsAutoString scriptString(code);
        NS_NAMED_LITERAL_CSTRING(url, "javascript:axplugin");
        rv = scriptContext->EvaluateString(scriptString,
                                           nsnull,      
                                           principal,
                                           url.get(),   
                                           1,           
                                           nsnull,
                                           nsnull,
                                           nsnull);

        if (NS_FAILED(rv))
            return NS_ERROR_FAILURE;

        return S_OK;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE toString( 
         BSTR __RPC_FAR *String)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE scrollBy( 
         long x,
         long y)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE scrollTo( 
         long x,
         long y)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE moveTo( 
         long x,
         long y)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE moveBy( 
         long x,
         long y)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE resizeTo( 
         long x,
         long y)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE resizeBy( 
         long x,
         long y)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_external( 
         IDispatch __RPC_FAR *__RPC_FAR *p)
    {
        return E_NOTIMPL;
    }
        
};



class IEDocument :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<IHTMLDocument2, &__uuidof(IHTMLDocument2), &LIBID_MSHTML>,
    public IServiceProvider,
    public IOleContainer,
    public IBindHost,
    public IHlinkFrame,
    public ITargetFrame
{
public:
    PluginInstanceData *mData;

    nsCOMPtr<nsIDOMWindow> mDOMWindow;
    nsCOMPtr<nsIDOMDocument> mDOMDocument;
    nsCOMPtr<nsIDOMElement> mDOMElement;
    CComObject<IEWindow> *mWindow;
    CComObject<IEBrowser> *mBrowser;
    CComBSTR mURL;
    BSTR mUseTarget;

    IEDocument() :
        mWindow(NULL),
        mBrowser(NULL),
        mData(NULL),
        mUseTarget(NULL)
    {
        MozAxPlugin::AddRef();
    }

    HRESULT Init(PluginInstanceData *pData)
    {
        mData = pData;

        
        NPN_GetValue(mData->pPluginInstance, NPNVDOMElement, 
                     static_cast<nsIDOMElement **>(getter_AddRefs(mDOMElement)));
        if (mDOMElement)
        {
            mDOMElement->GetOwnerDocument(getter_AddRefs(mDOMDocument));
        }

        
        NPN_GetValue(mData->pPluginInstance, NPNVDOMWindow, 
                     static_cast<nsIDOMWindow **>(getter_AddRefs(mDOMWindow)));
        if (mDOMWindow)
        {
            nsCOMPtr<nsIDOMWindowInternal> windowInternal = do_QueryInterface(mDOMWindow);
            if (windowInternal)
            {
                nsCOMPtr<nsIDOMLocation> location;
                nsAutoString href;
                windowInternal->GetLocation(getter_AddRefs(location));
                if (location &&
                    NS_SUCCEEDED(location->GetHref(href)))
                {
                    const PRUnichar *s = href.get();
                    mURL.Attach(::SysAllocString(s));
                }
            }
        }

        CComObject<IEWindow>::CreateInstance(&mWindow);
        ATLASSERT(mWindow);
        if (!mWindow)
        {
            return E_OUTOFMEMORY;
        }
        mWindow->AddRef();
        mWindow->Init(mData);

        CComObject<IEBrowser>::CreateInstance(&mBrowser);
        ATLASSERT(mBrowser);
        if (!mBrowser)
        {
            return E_OUTOFMEMORY;
        }
        mBrowser->AddRef();
        mBrowser->Init(mData);
 
        return S_OK;
    }

    virtual ~IEDocument()
    {
        if (mUseTarget)
        {
            SysFreeString(mUseTarget);
        }
        if (mBrowser)
        {
            mBrowser->Release();
        }
        if (mWindow)
        {
            mWindow->Release();
        }
        MozAxPlugin::Release();
    }

BEGIN_COM_MAP(IEDocument)
    COM_INTERFACE_ENTRY(IServiceProvider)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IHTMLDocument)
    COM_INTERFACE_ENTRY(IHTMLDocument2)
    COM_INTERFACE_ENTRY(IParseDisplayName)
    COM_INTERFACE_ENTRY(IOleContainer)
    COM_INTERFACE_ENTRY(IBindHost)
    COM_INTERFACE_ENTRY_BREAK(IHlinkTarget)
    COM_INTERFACE_ENTRY(IHlinkFrame)
    COM_INTERFACE_ENTRY(ITargetFrame)
END_COM_MAP()


    virtual  HRESULT STDMETHODCALLTYPE QueryService( 
         REFGUID guidService,
         REFIID riid,
         void **ppvObject)
    {
#ifdef DEBUG
        ATLTRACE(_T("IEDocument::QueryService\n"));
        if (IsEqualIID(riid, __uuidof(IWebBrowser)) ||
            IsEqualIID(riid, __uuidof(IWebBrowser2)) ||
            IsEqualIID(riid, __uuidof(IWebBrowserApp)))
        {
            ATLTRACE(_T("  IWebBrowserApp\n"));
            if (mBrowser)
            {
                return mBrowser->QueryInterface(riid, ppvObject);
            }
        }
        else if (IsEqualIID(riid, __uuidof(IHTMLWindow2)))
        {
            ATLTRACE(_T("  IHTMLWindow2\n"));
            if (mWindow)
            {
                return mWindow->QueryInterface(riid, ppvObject);
            }
        }
        else if (IsEqualIID(riid, __uuidof(IHTMLDocument2)))
        {
            ATLTRACE(_T("  IHTMLDocument2\n"));
        }
        else if (IsEqualIID(riid, __uuidof(IBindHost)))
        {
            ATLTRACE(_T("  IBindHost\n"));
        }
        else
        {
            USES_CONVERSION;
            LPOLESTR szClsid = NULL;
            StringFromIID(riid, &szClsid);
            ATLTRACE(_T("  Unknown interface %s\n"), OLE2T(szClsid));
            CoTaskMemFree(szClsid);
        }
#endif
        return QueryInterface(riid, ppvObject);
    }


    virtual  HRESULT STDMETHODCALLTYPE get_Script( 
         IDispatch **p)
    {
        *p = NULL;
        return S_OK;
    }


    virtual  HRESULT STDMETHODCALLTYPE get_all( 
         IHTMLElementCollection **p)
    {
        
        if (p == NULL)
        {
            return E_INVALIDARG;
        }

        *p = NULL;

        
        CIEHtmlElementCollectionInstance *pCollection = NULL;
        CIEHtmlElementCollectionInstance::CreateInstance(&pCollection);
        if (pCollection == NULL)
        {
            return E_OUTOFMEMORY;
        }

        
        nsCOMPtr<nsIDOMNode> docNode = do_QueryInterface(mDOMDocument);
        pCollection->PopulateFromDOMNode(docNode, PR_TRUE);
        pCollection->QueryInterface(IID_IHTMLElementCollection, (void **) p);

        return *p ? S_OK : E_UNEXPECTED;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_body( 
         IHTMLElement **p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_activeElement( 
         IHTMLElement **p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_images( 
         IHTMLElementCollection **p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_applets( 
         IHTMLElementCollection **p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_links( 
         IHTMLElementCollection **p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_forms( 
         IHTMLElementCollection **p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_anchors( 
         IHTMLElementCollection **p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_title( 
         BSTR v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_title( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_scripts( 
         IHTMLElementCollection **p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_designMode( 
         BSTR v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_designMode( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_selection( 
         IHTMLSelectionObject **p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_readyState( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_frames( 
         IHTMLFramesCollection2 **p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_embeds( 
         IHTMLElementCollection **p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_plugins( 
         IHTMLElementCollection **p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_alinkColor( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_alinkColor( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_bgColor( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_bgColor( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_fgColor( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_fgColor( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_linkColor( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_linkColor( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_vlinkColor( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_vlinkColor( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_referrer( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_location( 
         IHTMLLocation **p)
    {
        if (mWindow)
            return mWindow->get_location(p);
        return E_FAIL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_lastModified( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_URL( 
         BSTR v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_URL( 
         BSTR *p)
    {
        *p = mURL.Copy();
        return S_OK;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_domain( 
         BSTR v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_domain( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_cookie( 
         BSTR v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_cookie( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_expando( 
         VARIANT_BOOL v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_expando( 
         VARIANT_BOOL *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_charset( 
         BSTR v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_charset( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_defaultCharset( 
         BSTR v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_defaultCharset( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_mimeType( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_fileSize( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_fileCreatedDate( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_fileModifiedDate( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_fileUpdatedDate( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_security( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_protocol( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_nameProp( 
         BSTR *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE write( 
         SAFEARRAY * psarray)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE writeln( 
         SAFEARRAY * psarray)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE open( 
         BSTR url,
         VARIANT name,
         VARIANT features,
         VARIANT replace,
         IDispatch **pomWindowResult)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE close( void)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE clear( void)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE queryCommandSupported( 
         BSTR cmdID,
         VARIANT_BOOL *pfRet)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE queryCommandEnabled( 
         BSTR cmdID,
         VARIANT_BOOL *pfRet)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE queryCommandState( 
         BSTR cmdID,
         VARIANT_BOOL *pfRet)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE queryCommandIndeterm( 
         BSTR cmdID,
         VARIANT_BOOL *pfRet)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE queryCommandText( 
         BSTR cmdID,
         BSTR *pcmdText)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE queryCommandValue( 
         BSTR cmdID,
         VARIANT *pcmdValue)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE execCommand( 
         BSTR cmdID,
         VARIANT_BOOL showUI,
         VARIANT value,
         VARIANT_BOOL *pfRet)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE execCommandShowHelp( 
         BSTR cmdID,
         VARIANT_BOOL *pfRet)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE createElement( 
         BSTR eTag,
         IHTMLElement **newElem)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onhelp( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onhelp( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onclick( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onclick( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_ondblclick( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_ondblclick( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onkeyup( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onkeyup( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onkeydown( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onkeydown( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onkeypress( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onkeypress( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onmouseup( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onmouseup( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onmousedown( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onmousedown( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onmousemove( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onmousemove( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onmouseout( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onmouseout( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onmouseover( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onmouseover( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onreadystatechange( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onreadystatechange( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onafterupdate( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onafterupdate( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onrowexit( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onrowexit( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onrowenter( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onrowenter( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_ondragstart( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_ondragstart( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onselectstart( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onselectstart( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE elementFromPoint( 
         long x,
         long y,
         IHTMLElement **elementHit)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_parentWindow( 
         IHTMLWindow2 **p)
    {
        return mWindow->QueryInterface(_uuidof(IHTMLWindow2), (void **) p);
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_styleSheets( 
         IHTMLStyleSheetsCollection **p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onbeforeupdate( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onbeforeupdate( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE put_onerrorupdate( 
         VARIANT v)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE get_onerrorupdate( 
         VARIANT *p)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE toString( 
         BSTR *String)
    {
        return E_NOTIMPL;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE createStyleSheet( 
         BSTR bstrHref,
         long lIndex,
         IHTMLStyleSheet **ppnewStyleSheet)
    {
        return E_NOTIMPL;
    }


    virtual HRESULT STDMETHODCALLTYPE ParseDisplayName( 
         IBindCtx *pbc,
         LPOLESTR pszDisplayName,
         ULONG *pchEaten,
         IMoniker **ppmkOut)
    {
        return E_NOTIMPL;
    }


    virtual HRESULT STDMETHODCALLTYPE EnumObjects( 
         DWORD grfFlags,
         IEnumUnknown **ppenum)
    {
        return E_NOTIMPL;
    }
    
    virtual HRESULT STDMETHODCALLTYPE LockContainer( 
         BOOL fLock)
    {
        return E_NOTIMPL;
    }



    virtual HRESULT STDMETHODCALLTYPE SetBrowseContext( 
         IHlinkBrowseContext *pihlbc)
    {
        return E_NOTIMPL;
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetBrowseContext( 
         IHlinkBrowseContext **ppihlbc)
    {
        return E_NOTIMPL;
    }
    
    virtual HRESULT STDMETHODCALLTYPE Navigate( 
         DWORD grfHLNF,
         LPBC pbc,
         IBindStatusCallback *pibsc,
         IHlink *pihlNavigate)
    {
        if (!pihlNavigate) return E_INVALIDARG;
        
        LPWSTR szTarget = NULL;
        LPWSTR szLocation = NULL;
        LPWSTR szTargetFrame = NULL;
        HRESULT hr;
        hr = pihlNavigate->GetStringReference(HLINKGETREF_DEFAULT, &szTarget, &szLocation);
        hr = pihlNavigate->GetTargetFrameName(&szTargetFrame);
        if (szTarget && szTarget[0] != WCHAR('\0'))
        {
            NS_ConvertUTF16toUTF8 spec(szTarget);
            nsCOMPtr<nsIURI> uri;
            nsresult rv = NS_NewURI(getter_AddRefs(uri), spec);
            if (NS_SUCCEEDED(rv) && uri)
            {
                nsCOMPtr<nsIWebNavigation> webNav = do_GetInterface(mDOMWindow);
                if (webNav)
                {
                    nsCOMPtr<nsILinkHandler> lh = do_QueryInterface(webNav);
                    if (lh)
                    {
                        nsCOMPtr<nsPIDOMWindow> window =
                            do_GetInterface(mDOMWindow);

                        nsAutoPopupStatePusher popupStatePusher(window,
                                                                openAllowed);

                        nsCOMPtr<nsIContent> content(do_QueryInterface(mDOMElement));

                        lh->OnLinkClick(content, uri,
                                        szTargetFrame ? szTargetFrame : mUseTarget);
                    }
                }
                hr = S_OK;
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_FAIL;
        }
        if (szTarget)
            CoTaskMemFree(szTarget);
        if (szLocation)
            CoTaskMemFree(szLocation);
        if (szTargetFrame)
            CoTaskMemFree(szTargetFrame);
        if (mUseTarget)
        {
            SysFreeString(mUseTarget);
            mUseTarget = NULL;
        }
        return hr;
    }
    
    virtual HRESULT STDMETHODCALLTYPE OnNavigate( 
         DWORD grfHLNF,
         IMoniker *pimkTarget,
         LPCWSTR pwzLocation,
         LPCWSTR pwzFriendlyName,
         DWORD dwreserved)
    {
        return E_NOTIMPL;
    }
    
    virtual HRESULT STDMETHODCALLTYPE UpdateHlink( 
         ULONG uHLID,
         IMoniker *pimkTarget,
         LPCWSTR pwzLocation,
         LPCWSTR pwzFriendlyName)
    {
        return E_NOTIMPL;
    }


    virtual HRESULT STDMETHODCALLTYPE CreateMoniker( 
         LPOLESTR szName,
         IBindCtx *pBC,
         IMoniker **ppmk,
         DWORD dwReserved)
    {
        if (!szName || !ppmk) return E_POINTER;
        if (!*szName) return E_INVALIDARG;

        *ppmk = NULL;

        
        CComPtr<IMoniker> baseURLMoniker;
        nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDOMDocument));
        if (doc)
        {
            nsIURI *baseURI = doc->GetBaseURI();
            nsCAutoString spec;
            if (baseURI &&
                NS_SUCCEEDED(baseURI->GetSpec(spec)))
            {
                USES_CONVERSION;
                if (FAILED(CreateURLMoniker(NULL, T2CW(spec.get()), &baseURLMoniker)))
                    return E_UNEXPECTED;
            }
        }

        
        HRESULT hr = CreateURLMoniker(baseURLMoniker, szName, ppmk);
        if (SUCCEEDED(hr) && !*ppmk)
            hr = E_FAIL;
        return hr;
    }
    
    virtual  HRESULT STDMETHODCALLTYPE MonikerBindToStorage( 
         IMoniker *pMk,
         IBindCtx *pBC,
         IBindStatusCallback *pBSC,
         REFIID riid,
         void **ppvObj)
    {
        if (!pMk || !ppvObj) return E_POINTER;

        *ppvObj = NULL;
        HRESULT hr = S_OK;
        CComPtr<IBindCtx> spBindCtx;
        if (pBC)
        {
            spBindCtx = pBC;
            if (pBSC)
            {
                hr = RegisterBindStatusCallback(spBindCtx, pBSC, NULL, 0);
                if (FAILED(hr))
                    return hr;
            }
        }
        else
        {
            if (pBSC)
                hr = CreateAsyncBindCtx(0, pBSC, NULL, &spBindCtx);
            else
                hr = CreateBindCtx(0, &spBindCtx);
            if (SUCCEEDED(hr) && !spBindCtx)
                hr = E_FAIL;
            if (FAILED(hr))
                return hr;
        }
        return pMk->BindToStorage(spBindCtx, NULL, riid, ppvObj);
    }
    
    virtual  HRESULT STDMETHODCALLTYPE MonikerBindToObject( 
         IMoniker *pMk,
         IBindCtx *pBC,
         IBindStatusCallback *pBSC,
         REFIID riid,
         void **ppvObj)
    {
        return E_NOTIMPL;
    }


    virtual HRESULT STDMETHODCALLTYPE SetFrameName(
         LPCWSTR pszFrameName)
    {
        NS_ASSERTION(FALSE, "ITargetFrame::SetFrameName is not implemented");
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetFrameName(
         LPWSTR *ppszFrameName)
    {
        NS_ASSERTION(FALSE, "ITargetFrame::GetFrameName is not implemented");
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetParentFrame(
         IUnknown **ppunkParent)
    {
        NS_ASSERTION(FALSE, "ITargetFrame::GetParentFrame is not implemented");
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE FindFrame( 
         LPCWSTR pszTargetName,
         IUnknown *ppunkContextFrame,
         DWORD dwFlags,
         IUnknown **ppunkTargetFrame)
    {
        if (dwFlags & 0x1) 
        {
        }

        if (mUseTarget)
        {
            SysFreeString(mUseTarget);
            mUseTarget = NULL;
        }
        if (pszTargetName)
        {
            mUseTarget = SysAllocString(pszTargetName);
        }

        QueryInterface(__uuidof(IUnknown), (void **) ppunkTargetFrame);
        return S_OK;;
    }

    virtual HRESULT STDMETHODCALLTYPE SetFrameSrc( 
         LPCWSTR pszFrameSrc)
    {
        NS_ASSERTION(FALSE, "ITargetFrame::SetFrameSrc is not implemented");
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetFrameSrc( 
         LPWSTR *ppszFrameSrc)
    {
        NS_ASSERTION(FALSE, "ITargetFrame::GetFrameSrc is not implemented");
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetFramesContainer( 
         IOleContainer **ppContainer)
    {
        NS_ASSERTION(FALSE, "ITargetFrame::GetFramesContainer is not implemented");
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE SetFrameOptions( 
         DWORD dwFlags)
    {
        NS_ASSERTION(FALSE, "ITargetFrame::SetFrameOptions is not implemented");
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetFrameOptions( 
         DWORD *pdwFlags)
    {
        NS_ASSERTION(FALSE, "ITargetFrame::GetFrameOptions is not implemented");
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE SetFrameMargins( 
         DWORD dwWidth,
         DWORD dwHeight)
    {
        NS_ASSERTION(FALSE, "ITargetFrame::SetFrameMargins is not implemented");
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetFrameMargins( 
         DWORD *pdwWidth,
         DWORD *pdwHeight)
    {
        NS_ASSERTION(FALSE, "ITargetFrame::GetFrameMargins is not implemented");
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE RemoteNavigate( 
         ULONG cLength,
         ULONG *pulData)
    {
        NS_ASSERTION(FALSE, "ITargetFrame::RemoteNavigate is not implemented");
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OnChildFrameActivate( 
         IUnknown *pUnkChildFrame)
    {
        NS_ASSERTION(FALSE, "ITargetFrame::OnChildFrameActivate is not implemented");
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OnChildFrameDeactivate( 
         IUnknown *pUnkChildFrame)
    {
        NS_ASSERTION(FALSE, "ITargetFrame::OnChildFrameDeactivate is not implemented");
        return E_NOTIMPL;
    }
};

HRESULT MozAxPlugin::GetServiceProvider(PluginInstanceData *pData, IServiceProvider **pSP)
{
    
    CComObject<IEDocument> *pDoc = NULL;
    CComObject<IEDocument>::CreateInstance(&pDoc);
    ATLASSERT(pDoc);
    if (!pDoc)
    {
        return E_OUTOFMEMORY;
    }
    pDoc->Init(pData);
    
    return pDoc->QueryInterface(_uuidof(IServiceProvider), (void **) pSP);
}
