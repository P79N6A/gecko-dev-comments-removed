





































#ifndef IWEBBROWSERIMPL_H
#define IWEBBROWSERIMPL_H

#include <mshtml.h>


typedef long SHANDLE_PTR;

#include "nsIWebNavigation.h"
#include "nsIPrefBranch.h"
#include "nsIPrefLocalizedString.h"
#include "nsIDOMWindow.h"
#include "nsIBaseWindow.h"
#include "nsIWindowWatcher.h"
#include "nsIInputStream.h"
#include "nsIStringStream.h"
#include "nsIURI.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"

#include "PropertyList.h"


#include "CPMozillaControl.h"

#define ENSURE_BROWSER_IS_VALID() \
    if (!BrowserIsValid()) \
    { \
        NS_ERROR("Browser is not valid"); \
        return SetErrorInfo(E_UNEXPECTED, L"Browser is not in a valid state"); \
    }

#define ENSURE_GET_WEBNAV() \
    nsCOMPtr<nsIWebNavigation> webNav; \
    nsresult rv = GetWebNavigation(getter_AddRefs(webNav)); \
    if (NS_FAILED(rv)) \
    { \
        NS_ERROR("Cannot get nsIWebNavigation"); \
        return SetErrorInfo(E_UNEXPECTED, L"Could not obtain nsIWebNavigation interface"); \
    }

template<class T, const CLSID *pclsid, const GUID* plibid = &LIBID_MSHTML>
class IWebBrowserImpl :
    public CStockPropImpl<T, IWebBrowser2, &IID_IWebBrowser2, plibid>,
    public CProxyDWebBrowserEvents<T>,
    public CProxyDWebBrowserEvents2<T>
{
public:
    IWebBrowserImpl()
    {
        
        mBrowserReadyState = READYSTATE_UNINITIALIZED;
        
        mBusyFlag = PR_FALSE;
    }

public:

    
    virtual nsresult GetWebNavigation(nsIWebNavigation **aWebNav) = 0;
    
    virtual nsresult GetDOMWindow(nsIDOMWindow **aDOMWindow) = 0;
    
    virtual nsresult GetPrefs(nsIPrefBranch **aPrefBranch) = 0;
    
    virtual PRBool BrowserIsValid() = 0;

public:

    
    CComVariant             mLastPostData;
    
    READYSTATE              mBrowserReadyState;
    
    PRBool                  mBusyFlag;
    
    PropertyList            mPropertyList;



    
    
    
    
    virtual HRESULT SetErrorInfo(HRESULT hr, LPCOLESTR lpszDesc = NULL)
    {
        if (lpszDesc == NULL)
        {
            
            switch (hr)
            {
            case E_UNEXPECTED:
                lpszDesc = L"Method was called while control was uninitialized";
                break;
            case E_INVALIDARG:
                lpszDesc = L"Method was called with an invalid parameter";
                break;
            }
        }
        AtlSetErrorInfo(*pclsid, lpszDesc, 0, NULL, GUID_NULL, hr, NULL);
        return hr;
    }



    virtual HRESULT STDMETHODCALLTYPE GoBack(void)
    {
        ATLTRACE(_T("IWebBrowserImpl::GoBack()\n"));
        ENSURE_BROWSER_IS_VALID();
        ENSURE_GET_WEBNAV();

        PRBool aCanGoBack = PR_FALSE;
        webNav->GetCanGoBack(&aCanGoBack);
        if (aCanGoBack == PR_TRUE)
        {
            webNav->GoBack();
        }
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GoForward(void)
    {
        ATLTRACE(_T("IWebBrowserImpl::GoBack()\n"));
        ENSURE_BROWSER_IS_VALID();
        ENSURE_GET_WEBNAV();

        PRBool aCanGoForward = PR_FALSE;
        webNav->GetCanGoForward(&aCanGoForward);
        if (aCanGoForward == PR_TRUE)
        {
            webNav->GoForward();
        }
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GoHome(void)
    {
        ATLTRACE(_T("IWebBrowserImpl::GoHome()\n"));
        ENSURE_BROWSER_IS_VALID();

        CComBSTR bstrUrl(L"http:

        
        nsCOMPtr<nsIPrefBranch> prefBranch;
        if (NS_SUCCEEDED(GetPrefs(getter_AddRefs(prefBranch))))
        {
            nsCOMPtr<nsIPrefLocalizedString> homePage;
            prefBranch->GetComplexValue("browser.startup.homepage",
                                            NS_GET_IID(nsIPrefLocalizedString),
                                            getter_AddRefs(homePage));

            if (homePage)
            {
                nsString homePageString;
                nsresult rv = homePage->ToString(getter_Copies(homePageString));
                if (NS_SUCCEEDED(rv))
                {
                    bstrUrl = homePageString.get();
                }
            }
        }

        
        Navigate(bstrUrl, NULL, NULL, NULL, NULL);
    
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE GoSearch(void)
    {
        ATLTRACE(_T("IWebBrowserImpl::GoSearch()\n"));
        ENSURE_BROWSER_IS_VALID();

        CComBSTR bstrUrl(L"http:

        
#if 0
        
        nsCOMPtr<nsIPrefBranch> prefBranch;
        if (NS_SUCCEEDED(GetPrefs(getter_AddRefs(prefBranch))))
        {
            
            
        }
#endif

        Navigate(bstrUrl, NULL, NULL, NULL, NULL);
    
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE Navigate(BSTR URL, VARIANT __RPC_FAR *Flags, VARIANT __RPC_FAR *TargetFrameName, VARIANT __RPC_FAR *PostData, VARIANT __RPC_FAR *Headers)
    {
        ATLTRACE(_T("IWebBrowserImpl::Navigate()\n"));
        ENSURE_BROWSER_IS_VALID();

        nsresult rv;

        
        if (URL == NULL)
        {
            NS_ERROR("No URL supplied");
            return SetErrorInfo(E_INVALIDARG);
        }

        PRBool openInNewWindow = PR_FALSE;
        PRUint32 loadFlags = nsIWebNavigation::LOAD_FLAGS_NONE;

        
        LONG lFlags = 0;
        if (Flags &&
            Flags->vt != VT_ERROR &&
            Flags->vt != VT_EMPTY &&
            Flags->vt != VT_NULL)
        {
            CComVariant vFlags;
            if ( vFlags.ChangeType(VT_I4, Flags) != S_OK )
            {
                NS_ERROR("Flags param is invalid");
                return SetErrorInfo(E_INVALIDARG);
            }
            lFlags = vFlags.lVal;
        }
        if (lFlags & navOpenInNewWindow) 
        {
            openInNewWindow = PR_TRUE;
        }
        if (lFlags & navNoHistory)
        {
            
            loadFlags |= nsIWebNavigation::LOAD_FLAGS_BYPASS_HISTORY;
        }
        if (lFlags & navNoReadFromCache)
        {
            
            loadFlags |= nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE;

        }
        if (lFlags & navNoWriteToCache)
        {
            
            loadFlags |= nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE;
        }

        
        nsCOMPtr<nsIWebNavigation> targetNav;
        if (TargetFrameName &&
            TargetFrameName->vt == VT_BSTR &&
            TargetFrameName->bstrVal)
        {
            
            nsCOMPtr<nsIDOMWindow> window;
            GetDOMWindow(getter_AddRefs(window));
            if (window)
            {
                nsCOMPtr<nsIWindowWatcher> windowWatcher = do_GetService(NS_WINDOWWATCHER_CONTRACTID);
                if (windowWatcher)
                {
                    nsCOMPtr<nsIDOMWindow> targetWindow;
                    windowWatcher->GetWindowByName(TargetFrameName->bstrVal, window,
                        getter_AddRefs(targetWindow));
                    if (targetWindow)
                    {
                        targetNav = do_GetInterface(targetWindow);
                    }
                }
            }
            
            if (!targetNav)
                openInNewWindow = PR_TRUE;
        }

        
        if (openInNewWindow)
        {
            CComQIPtr<IDispatch> spDispNew;
            VARIANT_BOOL bCancel = VARIANT_FALSE;
        
            
            Fire_NewWindow2(&spDispNew, &bCancel);

            lFlags &= ~(navOpenInNewWindow);
            if ((bCancel == VARIANT_FALSE) && spDispNew)
            {
                CComQIPtr<IWebBrowser2> spOther = spDispNew;
                if (spOther)
                {
                    CComVariant vURL(URL);
                    CComVariant vFlags(lFlags);
                    return spOther->Navigate2(&vURL, &vFlags, TargetFrameName, PostData, Headers);
                }
            }

            
            
            
            
            

            
            return S_OK;
        }

        
        
        
        
        
        
        

        
        nsCOMPtr<nsIInputStream> postDataStream;
        mLastPostData.Clear();
        if (PostData &&
            PostData->vt == (VT_ARRAY | VT_UI1) &&
            PostData->parray)
        {
            mLastPostData.Copy(PostData);
            
            unsigned long nSizeData = PostData->parray->rgsabound[0].cElements;
            if (nSizeData > 0)
            {
                char szCL[64];
                sprintf(szCL, "Content-Length: %lu\r\n\r\n", nSizeData);
                unsigned long nSizeCL = strlen(szCL);
                unsigned long nSize = nSizeCL + nSizeData;

                char *tmp = (char *) nsMemory::Alloc(nSize + 1); 
                if (tmp)
                {

                    
                    SafeArrayLock(PostData->parray);
                    memcpy(tmp, szCL, nSizeCL);
                    memcpy(tmp + nSizeCL, PostData->parray->pvData, nSizeData);
                    tmp[nSize] = '\0';
                    SafeArrayUnlock(PostData->parray);

                    
                    nsCOMPtr<nsIStringInputStream> stream
                        (do_CreateInstance("@mozilla.org/io/string-input-stream;1"));
                    rv = stream->AdoptData(tmp, nSize);
                    if (NS_FAILED(rv) || !stream)
                    {
                        NS_ERROR("cannot create byte stream");
                        nsMemory::Free(tmp);
                        return SetErrorInfo(E_UNEXPECTED);
                    }

                    postDataStream = stream;
                }
            }
        }

        
        nsCOMPtr<nsIStringInputStream> headersStream;
        if (Headers &&
            Headers->vt == VT_BSTR &&
            Headers->bstrVal)
        {
            
            USES_CONVERSION;
            char *headers = OLE2A(Headers->bstrVal);
            if (headers)
            {
                size_t nSize = SysStringLen(Headers->bstrVal) + 1;
                char *tmp = (char *) nsMemory::Alloc(nSize); 
                if (tmp)
                {
                    
                    WideCharToMultiByte(CP_ACP, 0, Headers->bstrVal, nSize - 1, tmp, nSize, NULL, NULL);
                    tmp[nSize - 1] = '\0';

                    
                    headersStream = do_CreateInstance("@mozilla.org/io/string-input-stream;1");
                    if (headersStream)
                        rv = headersStream->AdoptData(tmp, nSize);

                    if (NS_FAILED(rv) || !headersStream)
                    {
                        NS_ERROR("cannot create byte stream");
                        nsMemory::Free(tmp);
                    }
                }
            }
        }

        
        nsCOMPtr<nsIWebNavigation> webNavToUse;
        if (targetNav)
        {
            webNavToUse = targetNav;
        }
        else
        {
            GetWebNavigation(getter_AddRefs(webNavToUse));
        }
    
        
        rv = NS_ERROR_FAILURE;
        if (webNavToUse)
        {
            rv = webNavToUse->LoadURI(URL,
                    loadFlags, nsnull, postDataStream, headersStream);
        }

        return NS_SUCCEEDED(rv) ? S_OK : E_FAIL;
    }
    
    virtual HRESULT STDMETHODCALLTYPE Refresh(void)
    {
        ATLTRACE(_T("IWebBrowserImpl::Refresh()\n"));
        
        CComVariant vRefreshType(REFRESH_NORMAL);
        return Refresh2(&vRefreshType);
    }

    virtual HRESULT STDMETHODCALLTYPE Refresh2(VARIANT __RPC_FAR *Level)
    {
        ATLTRACE(_T("IWebBrowserImpl::Refresh2()\n"));

        ENSURE_BROWSER_IS_VALID();
        ENSURE_GET_WEBNAV();
        if (Level == NULL)
            return E_INVALIDARG;

        
        OLECMDID_REFRESHFLAG iRefreshLevel = OLECMDIDF_REFRESH_NORMAL;
        CComVariant vLevelAsInt;
        if ( vLevelAsInt.ChangeType(VT_I4, Level) != S_OK )
        {
            NS_ERROR("Cannot change refresh type to int");
            return SetErrorInfo(E_UNEXPECTED);
        }
        iRefreshLevel = (OLECMDID_REFRESHFLAG) vLevelAsInt.iVal;

        
        PRUint32 flags = nsIWebNavigation::LOAD_FLAGS_NONE;
        switch (iRefreshLevel & OLECMDIDF_REFRESH_LEVELMASK)
        {
        case OLECMDIDF_REFRESH_NORMAL:
        case OLECMDIDF_REFRESH_IFEXPIRED:
        case OLECMDIDF_REFRESH_CONTINUE:
        case OLECMDIDF_REFRESH_NO_CACHE:
        case OLECMDIDF_REFRESH_RELOAD:
            flags = nsIWebNavigation::LOAD_FLAGS_NONE;
            break;
        case OLECMDIDF_REFRESH_COMPLETELY:
            flags = nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE | nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY;
            break;
        default:
            
            NS_ERROR("Unknown refresh type");
            return SetErrorInfo(E_UNEXPECTED);
        }

        webNav->Reload(flags);

        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE Stop(void)
    {
        ATLTRACE(_T("IWebBrowserImpl::Stop()\n"));
        ENSURE_BROWSER_IS_VALID();
        ENSURE_GET_WEBNAV();
        webNav->Stop(nsIWebNavigation::STOP_ALL);
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE get_Application(IDispatch __RPC_FAR *__RPC_FAR *ppDisp)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Application()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (!ppDisp)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        
        *ppDisp = (IDispatch *) this;
        (*ppDisp)->AddRef();
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE get_Parent(IDispatch __RPC_FAR *__RPC_FAR *ppDisp)
    {
        
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE get_Container(IDispatch __RPC_FAR *__RPC_FAR *ppDisp)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Container()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (!ppDisp)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        
        *ppDisp = NULL;
        return SetErrorInfo(E_UNEXPECTED);
    }
    virtual HRESULT STDMETHODCALLTYPE get_Document(IDispatch __RPC_FAR *__RPC_FAR *ppDisp)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Document()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (!ppDisp)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        *ppDisp = NULL;
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE get_TopLevelContainer(VARIANT_BOOL __RPC_FAR *pBool)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_TopLevelContainer()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (!pBool)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        
        *pBool = VARIANT_TRUE;
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_Type(BSTR __RPC_FAR *Type)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Type()\n"));
        ENSURE_BROWSER_IS_VALID();

        
        
#if 0
        nsIDOMDocument *pIDOMDocument = nsnull;
        if ( SUCCEEDED(GetDOMDocument(&pIDOMDocument)) )
        {
            nsIDOMDocumentType *pIDOMDocumentType = nsnull;
            if ( SUCCEEDED(pIDOMDocument->GetDoctype(&pIDOMDocumentType)) )
            {
                nsAutoString docName;
                pIDOMDocumentType->GetName(docName);
                
                
            }
        }
#endif
        
        return SetErrorInfo(E_FAIL, L"get_Type: failed");
    }

    virtual HRESULT STDMETHODCALLTYPE get_Left(long __RPC_FAR *pl)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Left()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (pl == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }

        
        *pl = 0;
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE put_Left(long Left)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_Left()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE get_Top(long __RPC_FAR *pl)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Top()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (pl == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        
        *pl = 0;
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE put_Top(long Top)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_Top()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE get_Width(long __RPC_FAR *pl)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Width()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        if (pl == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        *pl = 0;
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE put_Width(long Width)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_Width()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE get_Height(long __RPC_FAR *pl)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Height()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (pl == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }

        
        *pl = 0;
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE put_Height(long Height)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_Height()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE get_LocationName(BSTR __RPC_FAR *LocationName)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_LocationName()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (LocationName == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        
        nsString szLocationName;
        ENSURE_GET_WEBNAV();
        nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(webNav);
        baseWindow->GetTitle(getter_Copies(szLocationName));
        if (!szLocationName.get())
        {
            return SetErrorInfo(E_UNEXPECTED);
        }

        
        USES_CONVERSION;
        LPCOLESTR pszConvertedLocationName = W2COLE(szLocationName.get());
        *LocationName = SysAllocString(pszConvertedLocationName);

        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_LocationURL(BSTR __RPC_FAR *LocationURL)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_LocationURL()\n"));
        ENSURE_BROWSER_IS_VALID();

        if (LocationURL == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        nsCOMPtr<nsIURI> uri;
        
        nsCOMPtr<nsIWebNavigation> webNav;
        GetWebNavigation(getter_AddRefs(webNav));
        if (webNav)
        {
            webNav->GetCurrentURI(getter_AddRefs(uri));
        }
        if (uri)
        {
            USES_CONVERSION;
            nsCAutoString aURI;
            uri->GetAsciiSpec(aURI);
            *LocationURL = SysAllocString(A2OLE(aURI.get()));
        }
        else
        {
            *LocationURL = NULL;
        }

    return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_Busy(VARIANT_BOOL __RPC_FAR *pBool)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Busy()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (!pBool)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        *pBool = (mBusyFlag) ? VARIANT_TRUE : VARIANT_FALSE;
        return S_OK;
    }


    virtual HRESULT STDMETHODCALLTYPE Quit(void)
    {
        ATLTRACE(_T("IWebBrowserImpl::Quit()\n"));
        ENSURE_BROWSER_IS_VALID();

        
        
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE ClientToWindow(int __RPC_FAR *pcx, int __RPC_FAR *pcy)
    {
        ATLTRACE(_T("IWebBrowserImpl::ClientToWindow()\n"));
        ENSURE_BROWSER_IS_VALID();

        
        
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE PutProperty(BSTR Property, VARIANT vtValue)
    {
        ATLTRACE(_T("IWebBrowserImpl::PutProperty()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (Property == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        mPropertyList.AddOrReplaceNamedProperty(Property, vtValue);
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE GetProperty(BSTR Property, VARIANT __RPC_FAR *pvtValue)
    {
        ATLTRACE(_T("IWebBrowserImpl::GetProperty()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (Property == NULL || pvtValue == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        VariantInit(pvtValue);
        for (unsigned long i = 0; i < mPropertyList.GetSize(); i++)
        {
            if (wcsicmp(mPropertyList.GetNameOf(i), Property) == 0)
            {
                
                VariantCopy(pvtValue, const_cast<VARIANT *>(mPropertyList.GetValueOf(i)));
                return S_OK;
            }
        }
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_Name(BSTR __RPC_FAR *Name)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Name()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (Name == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }

        
        *Name = SysAllocString(L"Mozilla Web Browser Control");
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_HWND(SHANDLE_PTR __RPC_FAR *pHWND)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_HWND()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (pHWND == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }

        
        
        *pHWND = NULL;
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_FullName(BSTR __RPC_FAR *FullName)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_FullName()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (FullName == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }

        
        *FullName = SysAllocString(L""); 
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_Path(BSTR __RPC_FAR *Path)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Path()\n"));
        ENSURE_BROWSER_IS_VALID();

        if (Path == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }

        
        *Path = SysAllocString(L"");
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_Visible(VARIANT_BOOL __RPC_FAR *pBool)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Visible()\n"));
        ENSURE_BROWSER_IS_VALID();

        if (pBool == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }

        
        *pBool = VARIANT_TRUE;
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE put_Visible(VARIANT_BOOL Value)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_Visible()\n"));
        ENSURE_BROWSER_IS_VALID();

        
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_StatusBar(VARIANT_BOOL __RPC_FAR *pBool)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_StatusBar()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (pBool == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        
        *pBool = VARIANT_FALSE;
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE put_StatusBar(VARIANT_BOOL Value)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_StatusBar()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_StatusText(BSTR __RPC_FAR *StatusText)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_StatusText()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (StatusText == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }

        
        
        
        *StatusText = SysAllocString(L"");
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE put_StatusText(BSTR StatusText)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_StatusText()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        
        
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_ToolBar(int __RPC_FAR *Value)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_ToolBar()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (Value == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }

        
        *Value = FALSE;
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE put_ToolBar(int Value)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_ToolBar()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_MenuBar(VARIANT_BOOL __RPC_FAR *Value)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_MenuBar()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (Value == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }

        
        *Value = VARIANT_FALSE;
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE put_MenuBar(VARIANT_BOOL Value)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_MenuBar()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_FullScreen(VARIANT_BOOL __RPC_FAR *pbFullScreen)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_FullScreen()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (pbFullScreen == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }

        
        *pbFullScreen = VARIANT_FALSE;
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE put_FullScreen(VARIANT_BOOL bFullScreen)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_FullScreen()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        return S_OK;
    }


    virtual HRESULT STDMETHODCALLTYPE Navigate2(VARIANT __RPC_FAR *URL, VARIANT __RPC_FAR *Flags, VARIANT __RPC_FAR *TargetFrameName, VARIANT __RPC_FAR *PostData, VARIANT __RPC_FAR *Headers)
    {
        ATLTRACE(_T("IWebBrowserImpl::Navigate2()\n"));
        CComVariant vURLAsString;
        if (vURLAsString.ChangeType(VT_BSTR, URL) != S_OK)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        return Navigate(vURLAsString.bstrVal, Flags, TargetFrameName, PostData, Headers);
    }
    
    virtual HRESULT STDMETHODCALLTYPE QueryStatusWB(OLECMDID cmdID, OLECMDF __RPC_FAR *pcmdf)
    {
        ATLTRACE(_T("IWebBrowserImpl::QueryStatusWB()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (pcmdf == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        
        CComQIPtr<IOleCommandTarget> cmdTarget = this;
        if (cmdTarget)
        {
            OLECMD cmd;
            HRESULT hr;
    
            cmd.cmdID = cmdID;
            cmd.cmdf = 0;
            hr = cmdTarget->QueryStatus(NULL, 1, &cmd, NULL);
            if (SUCCEEDED(hr))
            {
                *pcmdf = (OLECMDF) cmd.cmdf;
            }
            return hr;
        }
        return E_NOTIMPL;
    }
    
    virtual HRESULT STDMETHODCALLTYPE ExecWB(OLECMDID cmdID, OLECMDEXECOPT cmdexecopt, VARIANT __RPC_FAR *pvaIn, VARIANT __RPC_FAR *pvaOut)
    {
        ATLTRACE(_T("IWebBrowserImpl::ExecWB()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        CComQIPtr<IOleCommandTarget> cmdTarget = this;
        if (cmdTarget)
        {
            return cmdTarget->Exec(NULL, cmdID, cmdexecopt, pvaIn, pvaOut);
        }
        return E_NOTIMPL;
    }
    
    virtual HRESULT STDMETHODCALLTYPE ShowBrowserBar(VARIANT __RPC_FAR *pvaClsid, VARIANT __RPC_FAR *pvarShow, VARIANT __RPC_FAR *pvarSize)
    {
        ATLTRACE(_T("IWebBrowserImpl::ShowBrowserBar()\n"));
        ENSURE_BROWSER_IS_VALID();
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_ReadyState(READYSTATE __RPC_FAR *plReadyState)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_ReadyState()\n"));
        
        
        if (plReadyState == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        *plReadyState = mBrowserReadyState;
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_Offline(VARIANT_BOOL __RPC_FAR *pbOffline)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Offline()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (pbOffline == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        
        *pbOffline = VARIANT_FALSE;
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE put_Offline(VARIANT_BOOL bOffline)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_Offline()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_Silent(VARIANT_BOOL __RPC_FAR *pbSilent)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Silent()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (pbSilent == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        
        *pbSilent = VARIANT_FALSE;
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE put_Silent(VARIANT_BOOL bSilent)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_Silent()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_RegisterAsBrowser(VARIANT_BOOL __RPC_FAR *pbRegister)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_RegisterAsBrowser()\n"));
        ENSURE_BROWSER_IS_VALID();

        if (pbRegister == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        
        *pbRegister = VARIANT_FALSE;
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE put_RegisterAsBrowser(VARIANT_BOOL bRegister)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_RegisterAsBrowser()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_RegisterAsDropTarget(VARIANT_BOOL __RPC_FAR *pbRegister)
    {
        
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE put_RegisterAsDropTarget(VARIANT_BOOL bRegister)
    {
        
        return E_NOTIMPL;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_TheaterMode(VARIANT_BOOL __RPC_FAR *pbRegister)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_TheaterMode()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (pbRegister == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        
        *pbRegister = VARIANT_FALSE;
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE put_TheaterMode(VARIANT_BOOL bRegister)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_TheaterMode()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE get_AddressBar(VARIANT_BOOL __RPC_FAR *Value)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_AddressBar()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (Value == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        
        *Value = VARIANT_FALSE;
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE put_AddressBar(VARIANT_BOOL Value)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_AddressBar()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        return S_OK;
    }
    
    virtual HRESULT STDMETHODCALLTYPE get_Resizable(VARIANT_BOOL __RPC_FAR *Value)
    {
        ATLTRACE(_T("IWebBrowserImpl::get_Resizable()\n"));
        ENSURE_BROWSER_IS_VALID();
        if (Value == NULL)
        {
            return SetErrorInfo(E_INVALIDARG);
        }
        
        *Value = VARIANT_TRUE;
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE put_Resizable(VARIANT_BOOL Value)
    {
        ATLTRACE(_T("IWebBrowserImpl::put_Resizable()\n"));
        ENSURE_BROWSER_IS_VALID();
        
        return S_OK;
    }
};

#endif
