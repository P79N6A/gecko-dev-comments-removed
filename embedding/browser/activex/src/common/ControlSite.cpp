





































#include "StdAfx.h"

#include <Objsafe.h>

#include "ControlSite.h"
#include "PropertyBag.h"
#include "ControlSiteIPFrame.h"

class CDefaultControlSiteSecurityPolicy : public CControlSiteSecurityPolicy
{
    
    BOOL ClassImplementsCategory(const CLSID & clsid, const CATID &catid, BOOL &bClassExists);
public:
    
    virtual BOOL IsClassSafeToHost(const CLSID & clsid);
    
    virtual BOOL IsClassMarkedSafeForScripting(const CLSID & clsid, BOOL &bClassExists);
    
    virtual BOOL IsObjectSafeForScripting(IUnknown *pObject, const IID &iid);
};

BOOL
CDefaultControlSiteSecurityPolicy::ClassImplementsCategory(const CLSID &clsid, const CATID &catid, BOOL &bClassExists)
{
#ifndef WINCE
    bClassExists = FALSE;

    
    
    

    CRegKey key;
    if (key.Open(HKEY_CLASSES_ROOT, _T("CLSID"), KEY_READ) != ERROR_SUCCESS)
    {
        
        return FALSE;
    }
    LPOLESTR szCLSID = NULL;
    if (FAILED(StringFromCLSID(clsid, &szCLSID)))
    {
        return FALSE;
    }
    USES_CONVERSION;
    CRegKey keyCLSID;
    LONG lResult = keyCLSID.Open(key, W2CT(szCLSID), KEY_READ);
    CoTaskMemFree(szCLSID);
    if (lResult != ERROR_SUCCESS)
    {
        
        return FALSE;
    }
    keyCLSID.Close();

    
    bClassExists = TRUE;
    CComQIPtr<ICatInformation> spCatInfo;
    HRESULT hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatInformation, (LPVOID*) &spCatInfo);
    if (spCatInfo == NULL)
    {
        
        return FALSE;
    }
    
    
    CComQIPtr<IEnumCATID> spEnumCATID;
    if (FAILED(spCatInfo->EnumImplCategoriesOfClass(clsid, &spEnumCATID)))
    {
        
        return FALSE;
    }

    
    BOOL bFound = FALSE;
    CATID catidNext = GUID_NULL;
    while (spEnumCATID->Next(1, &catidNext, NULL) == S_OK)
    {
        if (::IsEqualCATID(catid, catidNext))
        {
            return TRUE;
        }
    }
#endif

    return FALSE;
}


BOOL CDefaultControlSiteSecurityPolicy::IsClassSafeToHost(const CLSID & clsid)
{
    return TRUE;
}


BOOL CDefaultControlSiteSecurityPolicy::IsClassMarkedSafeForScripting(const CLSID & clsid, BOOL &bClassExists)
{
    
    return ClassImplementsCategory(clsid, CATID_SafeForScripting, bClassExists);
}


BOOL CDefaultControlSiteSecurityPolicy::IsObjectSafeForScripting(IUnknown *pObject, const IID &iid)
{
    if (!pObject) {
        return FALSE;
    }
    
    CComQIPtr<IObjectSafety> spObjectSafety = pObject;
    if (!spObjectSafety)
    {
        return FALSE;
    }

    DWORD dwSupported = 0; 
    DWORD dwEnabled = 0; 

    
    if (FAILED(spObjectSafety->GetInterfaceSafetyOptions(
            iid, &dwSupported, &dwEnabled)))
    {
        
        return FALSE;
    }

    
    if (!(dwEnabled & dwSupported) & INTERFACESAFE_FOR_UNTRUSTED_CALLER)
    {
        
        

        if(!(dwSupported & INTERFACESAFE_FOR_UNTRUSTED_CALLER) ||
            FAILED(spObjectSafety->SetInterfaceSafetyOptions(
               iid, INTERFACESAFE_FOR_UNTRUSTED_CALLER, INTERFACESAFE_FOR_UNTRUSTED_CALLER)) ||
            FAILED(spObjectSafety->GetInterfaceSafetyOptions(
                iid, &dwSupported, &dwEnabled)) ||
            !(dwEnabled & dwSupported) & INTERFACESAFE_FOR_UNTRUSTED_CALLER)
        {
            return FALSE;
        }
    }

    return TRUE;
}




CControlSite::CControlSite()
{
    TRACE_METHOD(CControlSite::CControlSite);

    m_hWndParent = NULL;
    m_CLSID = CLSID_NULL;
    m_bSetClientSiteFirst = FALSE;
    m_bVisibleAtRuntime = TRUE;
    memset(&m_rcObjectPos, 0, sizeof(m_rcObjectPos));

    m_bInPlaceActive = FALSE;
    m_bUIActive = FALSE;
    m_bInPlaceLocked = FALSE;
    m_bWindowless = FALSE;
    m_bSupportWindowlessActivation = TRUE;
    m_bSafeForScriptingObjectsOnly = FALSE;
    m_pSecurityPolicy = GetDefaultControlSecurityPolicy();

    
    m_nAmbientLocale = 0;
    m_clrAmbientForeColor = ::GetSysColor(COLOR_WINDOWTEXT);
    m_clrAmbientBackColor = ::GetSysColor(COLOR_WINDOW);
    m_bAmbientUserMode = true;
    m_bAmbientShowHatching = true;
    m_bAmbientShowGrabHandles = true;
    m_bAmbientAppearance = true; 

    
    m_hDCBuffer = NULL;
    m_hRgnBuffer = NULL;
    m_hBMBufferOld = NULL;
    m_hBMBuffer = NULL;
}



CControlSite::~CControlSite()
{
    TRACE_METHOD(CControlSite::~CControlSite);
    Detach();
}



HRESULT CControlSite::Create(REFCLSID clsid, PropertyList &pl,
    LPCWSTR szCodebase, IBindCtx *pBindContext)
{
    TRACE_METHOD(CControlSite::Create);

    m_CLSID = clsid;
    m_ParameterList = pl;

    
    if (m_pSecurityPolicy && !m_pSecurityPolicy->IsClassSafeToHost(clsid))
    {
        return E_FAIL;
    }

    
    BOOL checkForObjectSafety = FALSE;
    if (m_pSecurityPolicy && m_bSafeForScriptingObjectsOnly)
    {
        BOOL bClassExists = FALSE;
        BOOL bIsSafe = m_pSecurityPolicy->IsClassMarkedSafeForScripting(clsid, bClassExists);
        if (!bClassExists && szCodebase)
        {
            
        }
        else if (!bIsSafe)
        {
            
            
            checkForObjectSafety = TRUE;
        }
    }

    
    CComPtr<IUnknown> spObject;
    HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_ALL, IID_IUnknown, (void **) &spObject);
    if (SUCCEEDED(hr) && checkForObjectSafety)
    {
        
        if (!m_pSecurityPolicy->IsObjectSafeForScripting(spObject, __uuidof(IDispatch)))
        {
            return E_FAIL;
        }
        
    }

    
    if (FAILED(hr) && szCodebase)
    {
        wchar_t *szURL = NULL;

        
        DWORD dwFileVersionMS = 0xffffffff;
        DWORD dwFileVersionLS = 0xffffffff;
        const wchar_t *szHash = wcsrchr(szCodebase, wchar_t('#'));
        if (szHash)
        {
            if (wcsnicmp(szHash, L"#version=", 9) == 0)
            {
                int a, b, c, d;
                if (swscanf(szHash + 9, L"%d,%d,%d,%d", &a, &b, &c, &d) == 4)
                {
                    dwFileVersionMS = MAKELONG(b,a);
                    dwFileVersionLS = MAKELONG(d,c);
                }
            }
            szURL = _wcsdup(szCodebase);
            
            if (szURL)
                szURL[szHash - szCodebase] = wchar_t('\0');
        }
        else
        {
            szURL = _wcsdup(szCodebase);
        }
        if (!szURL)
            return E_OUTOFMEMORY;

        CComPtr<IBindCtx> spBindContext; 
        CComPtr<IBindStatusCallback> spBindStatusCallback;
        CComPtr<IBindStatusCallback> spOldBSC;

        
        BOOL useInternalBSC = FALSE;
        if (!pBindContext)
        {
            useInternalBSC = TRUE;
            hr = CreateBindCtx(0, &spBindContext);
            if (FAILED(hr))
            {
                free(szURL);
                return hr;
            }
            spBindStatusCallback = dynamic_cast<IBindStatusCallback *>(this);
            hr = RegisterBindStatusCallback(spBindContext, spBindStatusCallback, &spOldBSC, 0);
            if (FAILED(hr))
            {
                free(szURL);
                return hr;
            }
        }
        else
        {
            spBindContext = pBindContext;
        }

        hr = CoGetClassObjectFromURL(clsid, szURL, dwFileVersionMS, dwFileVersionLS,
            NULL, spBindContext, CLSCTX_ALL, NULL, IID_IUnknown, (void **) &m_spObject);
        
        free(szURL);
        
        
        
        if (useInternalBSC)
        {
            if (MK_S_ASYNCHRONOUS == hr)
            {
                m_bBindingInProgress = TRUE;
                m_hrBindResult = E_FAIL;

                
                HANDLE hFakeEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
                while (m_bBindingInProgress)
                {
                    MSG msg;
                    
                    while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
                    {
                        if (!::GetMessage(&msg, NULL, 0, 0))
                        {
                            m_bBindingInProgress = FALSE;
                            break;
                        }
                        ::TranslateMessage(&msg);
                        ::DispatchMessage(&msg);
                    }
                    if (!m_bBindingInProgress)
                        break;
                    
                    ::MsgWaitForMultipleObjects(1, &hFakeEvent, FALSE, 500, QS_ALLEVENTS);
                }
                ::CloseHandle(hFakeEvent);

                
                hr = m_hrBindResult;
            }

            
            if (spBindStatusCallback)
            {
                RevokeBindStatusCallback(spBindContext, spBindStatusCallback);
                spBindContext.Release();
            }
        }
    }

    if (spObject)
        m_spObject = spObject;

    return hr;
}



HRESULT CControlSite::Attach(HWND hwndParent, const RECT &rcPos, IUnknown *pInitStream)
{
    TRACE_METHOD(CControlSite::Attach);

    if (hwndParent == NULL)
    {
        NS_ERROR("No parent hwnd");
        return E_INVALIDARG;
    }

    m_hWndParent = hwndParent;
    m_rcObjectPos = rcPos;

    
    if (m_spObject == NULL)
    {
        return E_UNEXPECTED;
    }

    m_spIViewObject = m_spObject;
    m_spIOleObject = m_spObject;
    
    if (m_spIOleObject == NULL)
    {
        return E_FAIL;
    }
    
    DWORD dwMiscStatus;
    m_spIOleObject->GetMiscStatus(DVASPECT_CONTENT, &dwMiscStatus);

    if (dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)
    {
        m_bSetClientSiteFirst = TRUE;
    }
    if (dwMiscStatus & OLEMISC_INVISIBLEATRUNTIME)
    {
        m_bVisibleAtRuntime = FALSE;
    }

    
    
    if (m_bSetClientSiteFirst)
    {
        m_spIOleObject->SetClientSite(this);
    }

    
    
    CPropertyBagInstance *pPropertyBag = NULL;
    if (pInitStream == NULL && m_ParameterList.GetSize() > 0)
    {
        CPropertyBagInstance::CreateInstance(&pPropertyBag);
        pPropertyBag->AddRef();
        for (unsigned long i = 0; i < m_ParameterList.GetSize(); i++)
        {
            pPropertyBag->Write(m_ParameterList.GetNameOf(i),
                const_cast<VARIANT *>(m_ParameterList.GetValueOf(i)));
        }
        pInitStream = (IPersistPropertyBag *) pPropertyBag;
    }

    
    if (pInitStream)
    {
        CComQIPtr<IPropertyBag, &IID_IPropertyBag> spPropertyBag = pInitStream;
        CComQIPtr<IStream, &IID_IStream> spStream = pInitStream;
        CComQIPtr<IPersistStream, &IID_IPersistStream> spIPersistStream = m_spIOleObject;
        CComQIPtr<IPersistPropertyBag, &IID_IPersistPropertyBag> spIPersistPropertyBag = m_spIOleObject;

        if (spIPersistPropertyBag && spPropertyBag)
        {
            spIPersistPropertyBag->Load(spPropertyBag, NULL);
        }
        else if (spIPersistStream && spStream)
        {
            spIPersistStream->Load(spStream);
        }
    }
    else
    {
        
        CComQIPtr<IPersistStreamInit, &IID_IPersistStreamInit> spIPersistStreamInit = m_spIOleObject;
        if (spIPersistStreamInit)
        {
            spIPersistStreamInit->InitNew();
        }
    }

    m_spIOleInPlaceObject = m_spObject;
    m_spIOleInPlaceObjectWindowless = m_spObject;

    
    if (m_bVisibleAtRuntime)
    {
        DoVerb(OLEIVERB_INPLACEACTIVATE);
    }

    m_spIOleInPlaceObject->SetObjectRects(&m_rcObjectPos, &m_rcObjectPos);

    
    
    if (!m_bSetClientSiteFirst)
    {
        m_spIOleObject->SetClientSite(this);
    }

    return S_OK;
}



HRESULT CControlSite::Detach()
{
    TRACE_METHOD(CControlSite::Detach);

    if (m_spIOleInPlaceObjectWindowless)
    {
        m_spIOleInPlaceObjectWindowless.Release();
    }

    if (m_spIOleInPlaceObject)
    {
        m_spIOleInPlaceObject->InPlaceDeactivate();
        m_spIOleInPlaceObject.Release();
    }

    if (m_spIOleObject)
    {
        m_spIOleObject->Close(OLECLOSE_NOSAVE);
        m_spIOleObject->SetClientSite(NULL);
        m_spIOleObject.Release();
    }

    m_spIViewObject.Release();
    m_spObject.Release();
    
    return S_OK;
}



HRESULT CControlSite::GetControlUnknown(IUnknown **ppObject)
{
    *ppObject = NULL;
    if (m_spObject)
    {
        m_spObject->QueryInterface(IID_IUnknown, (void **) ppObject);
    }
    return S_OK;
}



HRESULT CControlSite::Advise(IUnknown *pIUnkSink, const IID &iid, DWORD *pdwCookie)
{
    if (m_spObject == NULL)
    {
        return E_UNEXPECTED;
    }

    if (pIUnkSink == NULL || pdwCookie == NULL)
    {
        return E_INVALIDARG;
    }

    return AtlAdvise(m_spObject, pIUnkSink, iid, pdwCookie);
}



HRESULT CControlSite::Unadvise(const IID &iid, DWORD dwCookie)
{
    if (m_spObject == NULL)
    {
        return E_UNEXPECTED;
    }

    return AtlUnadvise(m_spObject, iid, dwCookie);
}



HRESULT CControlSite::Draw(HDC hdc)
{
    TRACE_METHOD(CControlSite::Draw);

    
    if (m_spIViewObject)
    {
        if (m_bWindowless || !m_bInPlaceActive)
        {
            RECTL *prcBounds = (m_bWindowless) ? NULL : (RECTL *) &m_rcObjectPos;
            m_spIViewObject->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdc, prcBounds, NULL, NULL, 0);
        }
    }
    else
    {
        
        HBRUSH hbr = CreateSolidBrush(RGB(200,200,200));
        FillRect(hdc, &m_rcObjectPos, hbr);
        DeleteObject(hbr);
    }

    return S_OK;
}



HRESULT CControlSite::DoVerb(LONG nVerb, LPMSG lpMsg)
{
    TRACE_METHOD(CControlSite::DoVerb);

    if (m_spIOleObject == NULL)
    {
        return E_FAIL;
    }

    return m_spIOleObject->DoVerb(nVerb, lpMsg, this, 0, m_hWndParent, &m_rcObjectPos);
}



HRESULT CControlSite::SetPosition(const RECT &rcPos)
{
    TRACE_METHOD(CControlSite::SetPosition);
    m_rcObjectPos = rcPos;
    if (m_spIOleInPlaceObject)
    {
        m_spIOleInPlaceObject->SetObjectRects(&m_rcObjectPos, &m_rcObjectPos);
    }
    return S_OK;
}


void CControlSite::FireAmbientPropertyChange(DISPID id)
{
    if (m_spObject)
    {
        CComQIPtr<IOleControl> spControl = m_spObject;
        if (spControl)
        {
            spControl->OnAmbientPropertyChange(id);
        }
    }
}


void CControlSite::SetAmbientUserMode(BOOL bUserMode)
{
    bool bNewMode = bUserMode ? true : false;
    if (m_bAmbientUserMode != bNewMode)
    {
        m_bAmbientUserMode = bNewMode;
        FireAmbientPropertyChange(DISPID_AMBIENT_USERMODE);
    }
}




CControlSiteSecurityPolicy *CControlSite::GetDefaultControlSecurityPolicy()
{
    static CDefaultControlSiteSecurityPolicy defaultControlSecurityPolicy;
    return &defaultControlSecurityPolicy;
}


BOOL CControlSite::IsClassSafeToHost(const CLSID & clsid)
{
    if (m_pSecurityPolicy)
        return m_pSecurityPolicy->IsClassSafeToHost(clsid);
    return TRUE;
}


BOOL CControlSite::IsClassMarkedSafeForScripting(const CLSID & clsid, BOOL &bClassExists)
{
    if (m_pSecurityPolicy)
        return m_pSecurityPolicy->IsClassMarkedSafeForScripting(clsid, bClassExists);
    return TRUE;
}


BOOL CControlSite::IsObjectSafeForScripting(IUnknown *pObject, const IID &iid)
{
    if (m_pSecurityPolicy)
        return m_pSecurityPolicy->IsObjectSafeForScripting(pObject, iid);
    return TRUE;
}

BOOL CControlSite::IsObjectSafeForScripting(const IID &iid)
{
    return IsObjectSafeForScripting(m_spObject, iid);
}




HRESULT STDMETHODCALLTYPE CControlSite::QueryService(REFGUID guidService, REFIID riid, void** ppv)
{
    if (m_spServiceProvider)
        return m_spServiceProvider->QueryService(guidService, riid, ppv);
    return E_NOINTERFACE;
}






HRESULT STDMETHODCALLTYPE CControlSite::GetTypeInfoCount( UINT __RPC_FAR *pctinfo)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CControlSite::GetTypeInfo( UINT iTInfo,  LCID lcid,  ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CControlSite::GetIDsOfNames( REFIID riid,  LPOLESTR __RPC_FAR *rgszNames,  UINT cNames,  LCID lcid,  DISPID __RPC_FAR *rgDispId)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CControlSite::Invoke( DISPID dispIdMember,  REFIID riid,  LCID lcid,  WORD wFlags,  DISPPARAMS __RPC_FAR *pDispParams,  VARIANT __RPC_FAR *pVarResult,  EXCEPINFO __RPC_FAR *pExcepInfo,  UINT __RPC_FAR *puArgErr)
{
    if (wFlags & DISPATCH_PROPERTYGET)
    {
        CComVariant vResult;

        switch (dispIdMember)
        {
        case DISPID_AMBIENT_APPEARANCE:
            vResult = CComVariant(m_bAmbientAppearance);
            break;

        case DISPID_AMBIENT_FORECOLOR:
            vResult = CComVariant((long) m_clrAmbientForeColor);
            break;

        case DISPID_AMBIENT_BACKCOLOR:
            vResult = CComVariant((long) m_clrAmbientBackColor);
            break;

        case DISPID_AMBIENT_LOCALEID:
            vResult = CComVariant((long) m_nAmbientLocale);
            break;

        case DISPID_AMBIENT_USERMODE:
            vResult = CComVariant(m_bAmbientUserMode);
            break;
        
        case DISPID_AMBIENT_SHOWGRABHANDLES:
            vResult = CComVariant(m_bAmbientShowGrabHandles);
            break;
        
        case DISPID_AMBIENT_SHOWHATCHING:
            vResult = CComVariant(m_bAmbientShowHatching);
            break;

        default:
            return DISP_E_MEMBERNOTFOUND;
        }

        VariantCopy(pVarResult, &vResult);
        return S_OK;
    }

    return E_FAIL;
}






void STDMETHODCALLTYPE CControlSite::OnDataChange( FORMATETC __RPC_FAR *pFormatetc,  STGMEDIUM __RPC_FAR *pStgmed)
{
}


void STDMETHODCALLTYPE CControlSite::OnViewChange( DWORD dwAspect,  LONG lindex)
{
    
    InvalidateRect(NULL, FALSE);
}


void STDMETHODCALLTYPE CControlSite::OnRename( IMoniker __RPC_FAR *pmk)
{
}


void STDMETHODCALLTYPE CControlSite::OnSave(void)
{
}


void STDMETHODCALLTYPE CControlSite::OnClose(void)
{
}






void STDMETHODCALLTYPE CControlSite::OnLinkSrcChange( IMoniker __RPC_FAR *pmk)
{
}






void STDMETHODCALLTYPE CControlSite::OnViewStatusChange( DWORD dwViewStatus)
{
}





HRESULT STDMETHODCALLTYPE CControlSite::GetWindow( HWND __RPC_FAR *phwnd)
{
    *phwnd = m_hWndParent;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::ContextSensitiveHelp( BOOL fEnterMode)
{
    return S_OK;
}






HRESULT STDMETHODCALLTYPE CControlSite::SaveObject(void)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::GetMoniker( DWORD dwAssign,  DWORD dwWhichMoniker,  IMoniker __RPC_FAR *__RPC_FAR *ppmk)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CControlSite::GetContainer( IOleContainer __RPC_FAR *__RPC_FAR *ppContainer)
{
    if (!ppContainer) return E_INVALIDARG;
    *ppContainer = m_spContainer;
    if (*ppContainer)
    {
        (*ppContainer)->AddRef();
    }
    return (*ppContainer) ? S_OK : E_NOINTERFACE;
}


HRESULT STDMETHODCALLTYPE CControlSite::ShowObject(void)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::OnShowWindow( BOOL fShow)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::RequestNewObjectLayout(void)
{
    return E_NOTIMPL;
}






HRESULT STDMETHODCALLTYPE CControlSite::CanInPlaceActivate(void)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::OnInPlaceActivate(void)
{
    m_bInPlaceActive = TRUE;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::OnUIActivate(void)
{
    m_bUIActive = TRUE;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::GetWindowContext( IOleInPlaceFrame __RPC_FAR *__RPC_FAR *ppFrame,  IOleInPlaceUIWindow __RPC_FAR *__RPC_FAR *ppDoc,  LPRECT lprcPosRect,  LPRECT lprcClipRect,  LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    *lprcPosRect = m_rcObjectPos;
    *lprcClipRect = m_rcObjectPos;

    CControlSiteIPFrameInstance *pIPFrame = NULL;
    CControlSiteIPFrameInstance::CreateInstance(&pIPFrame);
    pIPFrame->AddRef();

    *ppFrame = (IOleInPlaceFrame *) pIPFrame;
    *ppDoc = NULL;

    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = NULL;
    lpFrameInfo->haccel = NULL;
    lpFrameInfo->cAccelEntries = 0;

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::Scroll( SIZE scrollExtant)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::OnUIDeactivate( BOOL fUndoable)
{
    m_bUIActive = FALSE;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::OnInPlaceDeactivate(void)
{
    m_bInPlaceActive = FALSE;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::DiscardUndoState(void)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::DeactivateAndUndo(void)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::OnPosRectChange( LPCRECT lprcPosRect)
{
    if (lprcPosRect == NULL)
    {
        return E_INVALIDARG;
    }
    SetPosition(m_rcObjectPos);
    return S_OK;
}






HRESULT STDMETHODCALLTYPE CControlSite::OnInPlaceActivateEx( BOOL __RPC_FAR *pfNoRedraw,  DWORD dwFlags)
{
    m_bInPlaceActive = TRUE;

    if (pfNoRedraw)
    {
        *pfNoRedraw = FALSE;
    }
    if (dwFlags & ACTIVATE_WINDOWLESS)
    {
        if (!m_bSupportWindowlessActivation)
        {
            return E_INVALIDARG;
        }
        m_bWindowless = TRUE;
    }
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::OnInPlaceDeactivateEx( BOOL fNoRedraw)
{
    m_bInPlaceActive = FALSE;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::RequestUIActivate(void)
{
    return S_FALSE;
}






HRESULT STDMETHODCALLTYPE CControlSite::CanWindowlessActivate(void)
{
    
    return (m_bSupportWindowlessActivation) ? S_OK : S_FALSE;
}


HRESULT STDMETHODCALLTYPE CControlSite::GetCapture(void)
{
    
    return S_FALSE;
}


HRESULT STDMETHODCALLTYPE CControlSite::SetCapture( BOOL fCapture)
{
    
    return S_FALSE;
}


HRESULT STDMETHODCALLTYPE CControlSite::GetFocus(void)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::SetFocus( BOOL fFocus)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::GetDC( LPCRECT pRect,  DWORD grfFlags,  HDC __RPC_FAR *phDC)
{
    if (phDC == NULL)
    {
        return E_INVALIDARG;
    }

    
    if (m_hDCBuffer != NULL)
    {
        return E_UNEXPECTED;
    }

    m_rcBuffer = m_rcObjectPos;
    if (pRect != NULL)
    {
        m_rcBuffer = *pRect;
    }

    m_hBMBuffer = NULL;
    m_dwBufferFlags = grfFlags;

    
    if (m_dwBufferFlags & OLEDC_OFFSCREEN)
    {
        m_hDCBuffer = CreateCompatibleDC(NULL);
        if (m_hDCBuffer == NULL)
        {
            
            return E_OUTOFMEMORY;
        }

        long cx = m_rcBuffer.right - m_rcBuffer.left;
        long cy = m_rcBuffer.bottom - m_rcBuffer.top;

        m_hBMBuffer = CreateCompatibleBitmap(m_hDCBuffer, cx, cy);
        m_hBMBufferOld = (HBITMAP) SelectObject(m_hDCBuffer, m_hBMBuffer);
        SetViewportOrgEx(m_hDCBuffer, m_rcBuffer.left, m_rcBuffer.top, NULL);

        
    }
    else
    {
        

        
        m_hDCBuffer = GetWindowDC(m_hWndParent);
        if (m_hDCBuffer == NULL)
        {
            
            return E_OUTOFMEMORY;
        }

        
        if (!(m_dwBufferFlags & OLEDC_NODRAW))
        {
            m_hRgnBuffer = CreateRectRgnIndirect(&m_rcBuffer);

            

            SelectClipRgn(m_hDCBuffer, m_hRgnBuffer);
        }
    }

    *phDC = m_hDCBuffer;

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::ReleaseDC( HDC hDC)
{
    
    if (hDC == NULL || hDC != m_hDCBuffer)
    {
        return E_INVALIDARG;
    }

    
    if ((m_dwBufferFlags & OLEDC_OFFSCREEN) &&
        !(m_dwBufferFlags & OLEDC_NODRAW))
    {
        
        SetViewportOrgEx(m_hDCBuffer, 0, 0, NULL);
        HDC hdc = GetWindowDC(m_hWndParent);

        long cx = m_rcBuffer.right - m_rcBuffer.left;
        long cy = m_rcBuffer.bottom - m_rcBuffer.top;

        BitBlt(hdc, m_rcBuffer.left, m_rcBuffer.top, cx, cy, m_hDCBuffer, 0, 0, SRCCOPY);
        
        ::ReleaseDC(m_hWndParent, hdc);
    }
    else
    {
        
    }

    
    if (m_hRgnBuffer)
    {
        SelectClipRgn(m_hDCBuffer, NULL);
        DeleteObject(m_hRgnBuffer);
        m_hRgnBuffer = NULL;
    }
    
    SelectObject(m_hDCBuffer, m_hBMBufferOld);
    if (m_hBMBuffer)
    {
        DeleteObject(m_hBMBuffer);
        m_hBMBuffer = NULL;
    }

    
    if (m_dwBufferFlags & OLEDC_OFFSCREEN)
    {
        ::DeleteDC(m_hDCBuffer);
    }
    else
    {
        ::ReleaseDC(m_hWndParent, m_hDCBuffer);
    }
    m_hDCBuffer = NULL;

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::InvalidateRect( LPCRECT pRect,  BOOL fErase)
{
    
    RECT rcI = { 0, 0, 0, 0 };
    if (pRect == NULL)
    {
        rcI = m_rcObjectPos;
    }
    else
    {
        IntersectRect(&rcI, &m_rcObjectPos, pRect);
    }
    ::InvalidateRect(m_hWndParent, &rcI, fErase);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CControlSite::InvalidateRgn( HRGN hRGN,  BOOL fErase)
{
    if (hRGN == NULL)
    {
        ::InvalidateRect(m_hWndParent, &m_rcObjectPos, fErase);
    }
    else
    {
        
        HRGN hrgnClip = CreateRectRgnIndirect(&m_rcObjectPos);
        if (CombineRgn(hrgnClip, hrgnClip, hRGN, RGN_AND) != ERROR)
        {
            ::InvalidateRgn(m_hWndParent, hrgnClip, fErase);
        }
        DeleteObject(hrgnClip);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CControlSite::ScrollRect( INT dx,  INT dy,  LPCRECT pRectScroll,  LPCRECT pRectClip)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CControlSite::AdjustRect( LPRECT prc)
{
    if (prc == NULL)
    {
        return E_INVALIDARG;
    }

    
    RECT rcI = { 0, 0, 0, 0 };
    IntersectRect(&rcI, &m_rcObjectPos, prc);
    *prc = rcI;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CControlSite::OnDefWindowMessage( UINT msg,  WPARAM wParam,  LPARAM lParam,  LRESULT __RPC_FAR *plResult)
{
    if (plResult == NULL)
    {
        return E_INVALIDARG;
    }
    
    
    if (m_bWindowless && m_spIOleInPlaceObjectWindowless)
    {
        return m_spIOleInPlaceObjectWindowless->OnWindowMessage(msg, wParam, lParam, plResult);
    }

    return S_FALSE;
}






HRESULT STDMETHODCALLTYPE CControlSite::OnControlInfoChanged(void)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::LockInPlaceActive( BOOL fLock)
{
    m_bInPlaceLocked = fLock;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::GetExtendedControl( IDispatch __RPC_FAR *__RPC_FAR *ppDisp)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CControlSite::TransformCoords( POINTL __RPC_FAR *pPtlHimetric,  POINTF __RPC_FAR *pPtfContainer,  DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if (pPtlHimetric == NULL)
    {
        return E_INVALIDARG;
    }
    if (pPtfContainer == NULL)
    {
        return E_INVALIDARG;
    }

    HDC hdc = ::GetDC(m_hWndParent);
#ifndef WINCE
    ::SetMapMode(hdc, MM_HIMETRIC);
#endif
    POINT rgptConvert[2];
    rgptConvert[0].x = 0;
    rgptConvert[0].y = 0;

    if (dwFlags & XFORMCOORDS_HIMETRICTOCONTAINER)
    {
        rgptConvert[1].x = pPtlHimetric->x;
        rgptConvert[1].y = pPtlHimetric->y;
#ifndef WINCE
        ::LPtoDP(hdc, rgptConvert, 2);
#endif
        if (dwFlags & XFORMCOORDS_SIZE)
        {
            pPtfContainer->x = (float)(rgptConvert[1].x - rgptConvert[0].x);
            pPtfContainer->y = (float)(rgptConvert[0].y - rgptConvert[1].y);
        }
        else if (dwFlags & XFORMCOORDS_POSITION)
        {
            pPtfContainer->x = (float)rgptConvert[1].x;
            pPtfContainer->y = (float)rgptConvert[1].y;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else if (dwFlags & XFORMCOORDS_CONTAINERTOHIMETRIC)
    {
        rgptConvert[1].x = (int)(pPtfContainer->x);
        rgptConvert[1].y = (int)(pPtfContainer->y);
#ifndef WINCE
        ::DPtoLP(hdc, rgptConvert, 2);
#endif
        if (dwFlags & XFORMCOORDS_SIZE)
        {
            pPtlHimetric->x = rgptConvert[1].x - rgptConvert[0].x;
            pPtlHimetric->y = rgptConvert[0].y - rgptConvert[1].y;
        }
        else if (dwFlags & XFORMCOORDS_POSITION)
        {
            pPtlHimetric->x = rgptConvert[1].x;
            pPtlHimetric->y = rgptConvert[1].y;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    ::ReleaseDC(m_hWndParent, hdc);

    return hr;
}


HRESULT STDMETHODCALLTYPE CControlSite::TranslateAccelerator( MSG __RPC_FAR *pMsg,  DWORD grfModifiers)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CControlSite::OnFocus( BOOL fGotFocus)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::ShowPropertyFrame(void)
{
    return E_NOTIMPL;
}




HRESULT STDMETHODCALLTYPE CControlSite::OnStartBinding(DWORD dwReserved, 
                                                       IBinding __RPC_FAR *pib)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CControlSite::GetPriority(LONG __RPC_FAR *pnPriority)
{
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE CControlSite::OnLowResource(DWORD reserved)
{
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE CControlSite::OnProgress(ULONG ulProgress, 
                                        ULONG ulProgressMax, 
                                        ULONG ulStatusCode, 
                                        LPCWSTR szStatusText)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CControlSite::OnStopBinding(HRESULT hresult, LPCWSTR szError)
{
    m_bBindingInProgress = FALSE;
    m_hrBindResult = hresult;
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE CControlSite::GetBindInfo(DWORD __RPC_FAR *pgrfBINDF, 
                                                    BINDINFO __RPC_FAR *pbindInfo)
{
    *pgrfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE |
                 BINDF_GETNEWESTVERSION | BINDF_NOWRITECACHE;
    pbindInfo->cbSize = sizeof(BINDINFO);
    pbindInfo->szExtraInfo = NULL;
    memset(&pbindInfo->stgmedData, 0, sizeof(STGMEDIUM));
    pbindInfo->grfBindInfoF = 0;
    pbindInfo->dwBindVerb = 0;
    pbindInfo->szCustomVerb = NULL;
    return S_OK;
}
    
HRESULT STDMETHODCALLTYPE CControlSite::OnDataAvailable(DWORD grfBSCF, 
                                                        DWORD dwSize, 
                                                        FORMATETC __RPC_FAR *pformatetc, 
                                                        STGMEDIUM __RPC_FAR *pstgmed)
{
    return E_NOTIMPL;
}
  
HRESULT STDMETHODCALLTYPE CControlSite::OnObjectAvailable(REFIID riid, 
                                                          IUnknown __RPC_FAR *punk)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CControlSite::GetWindow(
     REFGUID rguidReason,
     HWND *phwnd)
{
    *phwnd = NULL;
    return S_OK;
}


