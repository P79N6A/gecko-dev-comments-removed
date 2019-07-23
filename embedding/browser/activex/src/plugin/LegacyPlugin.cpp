




































#include "stdafx.h"

#include "npapi.h"

#include "jsapi.h"
#include "jscntxt.h"

#include "nsISupports.h"

#ifdef MOZ_ACTIVEX_PLUGIN_XPCONNECT
#include "XPConnect.h"
#endif

#include "LegacyPlugin.h"

#ifdef XPC_IDISPATCH_SUPPORT
#include "nsCOMPtr.h"
#include "nsStringAPI.h"
#include "nsIDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIURI.h"
#include "nsIJSContextStack.h"
#include "nsIScriptSecurityManager.h"
#endif




#ifndef MOZ_ACTIVEX_PLUGIN_XPCONNECT


const BOOL kHostSafeControlsOnly = FALSE;


const BOOL kDownloadControlsIfMissing = FALSE;
#endif



const BOOL kDisplayErrorMessages = FALSE;




class CInstallControlProgress :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IBindStatusCallback,
    public IWindowForBindingUI
{
public:
    CInstallControlProgress()
    {
    }

    BOOL mBindingInProgress;
    HRESULT mResult;
    NPP mNPP;

protected:
    virtual ~CInstallControlProgress()
    {
    }
public:
BEGIN_COM_MAP(CInstallControlProgress)
    COM_INTERFACE_ENTRY(IBindStatusCallback)
    COM_INTERFACE_ENTRY(IWindowForBindingUI)
END_COM_MAP()


    HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD dwReserved, 
                                            IBinding __RPC_FAR *pib)
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetPriority(LONG __RPC_FAR *pnPriority)
    {
        return S_OK;
    }
        
    HRESULT STDMETHODCALLTYPE OnLowResource(DWORD reserved)
    {
        return S_OK;
    }
        
    HRESULT STDMETHODCALLTYPE OnProgress(ULONG ulProgress, 
                                         ULONG ulProgressMax, 
                                         ULONG ulStatusCode, 
                                         LPCWSTR szStatusText)
    {
        switch (ulStatusCode)
        {
        case BINDSTATUS_BEGINDOWNLOADDATA:
        case BINDSTATUS_DOWNLOADINGDATA:
        case BINDSTATUS_ENDDOWNLOADDATA:
            {
                char szMsg[100];
                _snprintf(szMsg, sizeof(szMsg) - 1, "Downloading control (%lu of %lu)", ulProgress, ulProgressMax);
                szMsg[sizeof(szMsg) - 1] = '\0';
                NPN_Status(mNPP, szMsg);
            }
            break;
        case BINDSTATUS_FINDINGRESOURCE:
        case BINDSTATUS_CONNECTING:
        case BINDSTATUS_REDIRECTING:
        case BINDSTATUS_BEGINDOWNLOADCOMPONENTS:
        case BINDSTATUS_INSTALLINGCOMPONENTS:
        case BINDSTATUS_ENDDOWNLOADCOMPONENTS:
        case BINDSTATUS_USINGCACHEDCOPY:
        case BINDSTATUS_SENDINGREQUEST:
        case BINDSTATUS_CLASSIDAVAILABLE:
        case BINDSTATUS_MIMETYPEAVAILABLE:
        case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        case BINDSTATUS_BEGINSYNCOPERATION:
        case BINDSTATUS_ENDSYNCOPERATION:
        case BINDSTATUS_BEGINUPLOADDATA:
        case BINDSTATUS_UPLOADINGDATA:
        case BINDSTATUS_ENDUPLOADDATA:
        case BINDSTATUS_PROTOCOLCLASSID:
        case BINDSTATUS_ENCODING:
        case BINDSTATUS_CLASSINSTALLLOCATION:
        case BINDSTATUS_DECODING:
        case BINDSTATUS_LOADINGMIMEHANDLER:
        default:
            
            break;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT hresult, LPCWSTR szError)
    {
        mBindingInProgress = FALSE;
        mResult = hresult;
        NPN_Status(mNPP, "");
        return S_OK;
    }
        
    HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD __RPC_FAR *pgrfBINDF, 
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
        
    HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD grfBSCF, 
                                              DWORD dwSize, 
                                              FORMATETC __RPC_FAR *pformatetc, 
                                              STGMEDIUM __RPC_FAR *pstgmed)
    {
        return E_NOTIMPL;
    }
      
    HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID riid, 
                                                IUnknown __RPC_FAR *punk)
    {
        return S_OK;
    }


    virtual HRESULT STDMETHODCALLTYPE GetWindow(
         REFGUID rguidReason,
         HWND *phwnd)
    {
        HWND hwnd = NULL;
        NPN_GetValue(mNPP, NPNVnetscapeWindow, &hwnd);
        *phwnd = hwnd;
        return S_OK;
    }
};









NPError NPP_Initialize(void)
{
    ATLTRACE(_T("NPP_Initialize()\n"));
    _Module.Lock();
    return NPERR_NO_ERROR;
}






void NPP_Shutdown(void)
{
    ATLTRACE(_T("NPP_Shutdown()\n"));
    _Module.Unlock();
}


#define MIME_OLEOBJECT1   "application/x-oleobject"
#define MIME_OLEOBJECT2   "application/oleobject"

enum MozAxPluginErrors
{
    MozAxErrorControlIsNotSafeForScripting,
    MozAxErrorCouldNotCreateControl,
};

static void
ShowError(MozAxPluginErrors errorCode, const CLSID &clsid)
{
    if (!kDisplayErrorMessages)
        return;

    const TCHAR *szMsg = NULL;
    const unsigned long kBufSize = 256;
    TCHAR szBuffer[kBufSize];

    
    switch (errorCode)
    {
    case MozAxErrorControlIsNotSafeForScripting:
        {
            USES_CONVERSION;
            LPOLESTR szClsid;
            StringFromCLSID(clsid, &szClsid);
            _sntprintf(szBuffer, kBufSize - 1,
                _T("Could not create the control %s because it is not marked safe for scripting."), OLE2T(szClsid));
            CoTaskMemFree(szClsid);
            szMsg = szBuffer;
        }
        break;
    case MozAxErrorCouldNotCreateControl:
        {
            USES_CONVERSION;
            LPOLESTR szClsid;
            StringFromCLSID(clsid, &szClsid);
            _sntprintf(szBuffer, kBufSize - 1,
                _T("Could not create the control %s. Check that it has been installed on your computer and that this page correctly references it."), OLE2T(szClsid));
            CoTaskMemFree(szClsid);
            szMsg = szBuffer;
        }
        break;
    }
    szBuffer[kBufSize - 1] = TCHAR('\0');
    if (szMsg)
        MessageBox(NULL, szMsg, _T("ActiveX Error"), MB_OK | MB_ICONWARNING);
}

#if defined(MOZ_ACTIVEX_PLUGIN_XPCONNECT) && defined(XPC_IDISPATCH_SUPPORT)

static nsresult
CreatePrincipal(nsIURI * origin,
                nsIPrincipal ** outPrincipal)
{
    nsresult rv;
    nsCOMPtr<nsIScriptSecurityManager> secMan(
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv));
    if (NS_FAILED(rv)) return rv;

    return secMan->GetCodebasePrincipal(origin, outPrincipal);
}









class MozAxAutoPushJSContext
{
public:
    MozAxAutoPushJSContext(JSContext *cx, nsIURI * aURI);

    ~MozAxAutoPushJSContext();

    nsresult ResultOfPush() { return mPushResult; };

private:
    nsCOMPtr<nsIJSContextStack> mContextStack;
    JSContext*                  mContext;
    JSStackFrame                mFrame;
    nsresult                    mPushResult;
};


MozAxAutoPushJSContext::MozAxAutoPushJSContext(JSContext *cx,
                                               nsIURI * aURI)
                                     : mContext(cx), mPushResult(NS_OK)
{
    nsCOMPtr<nsIJSContextStack> contextStack =
        do_GetService("@mozilla.org/js/xpc/ContextStack;1");

    JSContext* currentCX;
    if(contextStack &&
       
       (NS_FAILED(contextStack->Peek(&currentCX)) ||
        cx != currentCX) )
    {
        if (NS_SUCCEEDED(contextStack->Push(cx)))
        {
            
            
            mContextStack.swap(contextStack);
        }
    }

    memset(&mFrame, 0, sizeof(mFrame));

    
    
    PRBool hasScript = PR_FALSE;
    JSStackFrame* tempFP = cx->fp;
    while (tempFP)
    {
        if (tempFP->script)
        {
            hasScript = PR_TRUE;
            break;
        }
        tempFP = tempFP->down;
    };

    if (!hasScript)
    {
        nsCOMPtr<nsIPrincipal> principal;
        mPushResult = CreatePrincipal(aURI, getter_AddRefs(principal));

        if (NS_SUCCEEDED(mPushResult))
        {
            JSPrincipals* jsprinc;
            principal->GetJSPrincipals(cx, &jsprinc);

            mFrame.script = JS_CompileScriptForPrincipals(cx, JS_GetGlobalObject(cx),
                                                          jsprinc, "", 0, "", 1);
            JSPRINCIPALS_DROP(cx, jsprinc);

            if (mFrame.script)
            {
                mFrame.down = cx->fp;
                cx->fp = &mFrame;
            }
            else
                mPushResult = NS_ERROR_OUT_OF_MEMORY;
        }
    }
}

MozAxAutoPushJSContext::~MozAxAutoPushJSContext()
{
    if (mContextStack)
        mContextStack->Pop(nsnull);

    if (mFrame.script)
        mContext->fp = mFrame.down;

}


static JSContext*
GetPluginsContext(PluginInstanceData *pData)
{
    nsCOMPtr<nsIDOMWindow> window;
    NPN_GetValue(pData->pPluginInstance, NPNVDOMWindow, 
                 static_cast<nsIDOMWindow **>(getter_AddRefs(window)));

    nsCOMPtr<nsIScriptGlobalObject> globalObject(do_QueryInterface(window));
    if (!globalObject)
        return nsnull;

    nsIScriptContext *scriptContext = globalObject->GetContext();

    if (!scriptContext)
        return nsnull;

    return reinterpret_cast<JSContext*>(scriptContext->GetNativeContext());
}

#endif

static BOOL
WillHandleCLSID(const CLSID &clsid, PluginInstanceData *pData)
{
#if defined(MOZ_ACTIVEX_PLUGIN_XPCONNECT) && defined(XPC_IDISPATCH_SUPPORT)
    
    nsCOMPtr<nsIDispatchSupport> dispSupport = do_GetService(NS_IDISPATCH_SUPPORT_CONTRACTID);
    if (!dispSupport)
        return FALSE;
    JSContext * cx = GetPluginsContext(pData);
    if (!cx)
        return FALSE;
    nsCID cid;
    memcpy(&cid, &clsid, sizeof(nsCID));
    PRBool isSafe = PR_FALSE;
    PRBool classExists = PR_FALSE;
    nsCOMPtr<nsIURI> uri;
    MozAxPlugin::GetCurrentLocation(pData->pPluginInstance, getter_AddRefs(uri));

    JSAutoRequest req(cx);
    MozAxAutoPushJSContext autoContext(cx, uri);
    dispSupport->IsClassSafeToHost(cx, cid, PR_TRUE, &classExists, &isSafe);
    if (classExists && !isSafe)
        return FALSE;
    return TRUE;
#else
    if (::IsEqualCLSID(clsid, CLSID_NULL))
    {
        return FALSE;
    }

    
    CRegKey keyExplorer;
    if (keyExplorer.Open(HKEY_LOCAL_MACHINE,
        _T("SOFTWARE\\Microsoft\\Internet Explorer\\ActiveX Compatibility"), KEY_READ) == ERROR_SUCCESS)
    {
        LPOLESTR szCLSID = NULL;
        ::StringFromCLSID(clsid, &szCLSID);
        if (szCLSID)
        {
            CRegKey keyCLSID;
            USES_CONVERSION;
            if (keyCLSID.Open(keyExplorer, W2T(szCLSID), KEY_READ) == ERROR_SUCCESS)
            {
                DWORD dwType = REG_DWORD;
                DWORD dwFlags = 0;
                DWORD dwBufSize = sizeof(dwFlags);
                if (::RegQueryValueEx(keyCLSID, _T("Compatibility Flags"),
                    NULL, &dwType, (LPBYTE) &dwFlags, &dwBufSize) == ERROR_SUCCESS)
                {
                    
                    const DWORD kKillBit = 0x00000400;
                    if (dwFlags & kKillBit)
                    {
                        ::CoTaskMemFree(szCLSID);
                        return FALSE;
                    }
                }
            }
            ::CoTaskMemFree(szCLSID);
        }
    }

    
    CRegKey keyDeny;
    if (keyDeny.Open(HKEY_LOCAL_MACHINE, kControlsToDenyKey, KEY_READ) == ERROR_SUCCESS)
    {
        
        int i = 0;
        do {
            USES_CONVERSION;
            TCHAR szCLSID[64];
            const DWORD nLength = sizeof(szCLSID) / sizeof(szCLSID[0]);
            if (::RegEnumKey(keyDeny, i++, szCLSID, nLength) != ERROR_SUCCESS)
            {
                break;
            }
            szCLSID[nLength - 1] = TCHAR('\0');
            CLSID clsidToCompare = GUID_NULL;
            if (SUCCEEDED(::CLSIDFromString(T2OLE(szCLSID), &clsidToCompare)) &&
                ::IsEqualCLSID(clsid, clsidToCompare))
            {
                return FALSE;
            }
        } while (1);
        keyDeny.Close();
    }

    
    CRegKey keyAllow;
    if (keyAllow.Open(HKEY_LOCAL_MACHINE, kControlsToAllowKey, KEY_READ) == ERROR_SUCCESS)
    {
        
        int i = 0;
        do {
            USES_CONVERSION;
            TCHAR szCLSID[64];
            const DWORD nLength = sizeof(szCLSID) / sizeof(szCLSID[0]);
            if (::RegEnumKey(keyAllow, i, szCLSID, nLength) != ERROR_SUCCESS)
            {
                
                return (i == 0) ? TRUE : FALSE;
            }
            ++i;
            szCLSID[nLength - 1] = TCHAR('\0');
            CLSID clsidToCompare = GUID_NULL;
            if (SUCCEEDED(::CLSIDFromString(T2OLE(szCLSID), &clsidToCompare)) &&
                ::IsEqualCLSID(clsid, clsidToCompare))
            {
                return TRUE;
            }
        } while (1);
    }

    return TRUE;
#endif
}

static NPError
CreateControl(const CLSID &clsid, PluginInstanceData *pData, PropertyList &pl, LPCOLESTR szCodebase)
{
    
    if (!WillHandleCLSID(clsid, pData))
    {
        return NPERR_GENERIC_ERROR;
    }

    pData->clsid = clsid;

    
    
    PRBool hostSafeControlsOnly;
    PRBool downloadControlsIfMissing;
#if defined(MOZ_ACTIVEX_PLUGIN_XPCONNECT) && defined(XPC_IDISPATCH_SUPPORT)
#ifdef MOZ_FLASH_ACTIVEX_PATCH
    GUID flashGUID;
    ::CLSIDFromString(_T("{D27CDB6E-AE6D-11CF-96B8-444553540000}"), &flashGUID);

    
    PRUint32 hostingFlags;
    if (clsid == flashGUID)
    {
      hostingFlags = (nsIActiveXSecurityPolicy::HOSTING_FLAGS_HOST_SAFE_OBJECTS |
                      nsIActiveXSecurityPolicy::HOSTING_FLAGS_SCRIPT_SAFE_OBJECTS |
                      nsIActiveXSecurityPolicy::HOSTING_FLAGS_SCRIPT_ALL_OBJECTS |
                      nsIActiveXSecurityPolicy::HOSTING_FLAGS_HOST_ALL_OBJECTS);
    }
    else
    {
      hostingFlags = MozAxPlugin::PrefGetHostingFlags();
    }
#else
    PRUint32 hostingFlags = MozAxPlugin::PrefGetHostingFlags();
#endif
    if (hostingFlags & nsIActiveXSecurityPolicy::HOSTING_FLAGS_HOST_SAFE_OBJECTS &&
        !(hostingFlags & nsIActiveXSecurityPolicy::HOSTING_FLAGS_HOST_ALL_OBJECTS))
    {
        hostSafeControlsOnly = TRUE;
    }
    else if (hostingFlags & nsIActiveXSecurityPolicy::HOSTING_FLAGS_HOST_ALL_OBJECTS)
    {
        hostSafeControlsOnly = FALSE;
    }
    else
    {
        
        
        return NPERR_GENERIC_ERROR;
    }
    if (hostingFlags & nsIActiveXSecurityPolicy::HOSTING_FLAGS_DOWNLOAD_CONTROLS)
    {
        downloadControlsIfMissing = PR_TRUE;
    }
    else
    {
        downloadControlsIfMissing = PR_FALSE;
    }
    
    nsCOMPtr<nsIDispatchSupport> dispSupport = do_GetService(NS_IDISPATCH_SUPPORT_CONTRACTID);
    if (!dispSupport)
    {
        return NPERR_GENERIC_ERROR;
    }
    
    PRBool classIsMarkedSafeForScripting = PR_FALSE;
    if (hostSafeControlsOnly)
    {
        PRBool classExists = PR_FALSE;
        PRBool isClassSafeForScripting = PR_FALSE;
        nsCID cid;
        memcpy(&cid, &clsid, sizeof(cid));
        if (NS_SUCCEEDED(dispSupport->IsClassMarkedSafeForScripting(cid, &classExists, &isClassSafeForScripting)) &&
            classExists && isClassSafeForScripting)
        {
            classIsMarkedSafeForScripting = PR_TRUE;
        }
    }
#else
    hostSafeControlsOnly = kHostSafeControlsOnly;
    downloadControlsIfMissing = kDownloadControlsIfMissing;
#endif

    
    CControlSiteInstance *pSite = NULL;
    CControlSiteInstance::CreateInstance(&pSite);
    if (pSite == NULL)
    {
        return NPERR_GENERIC_ERROR;
    }

    pSite->m_bSupportWindowlessActivation = FALSE;
#if defined(MOZ_ACTIVEX_PLUGIN_XPCONNECT) && defined(XPC_IDISPATCH_SUPPORT)
    
    pSite->SetSecurityPolicy(NULL);
    pSite->m_bSafeForScriptingObjectsOnly = FALSE;
#else
    pSite->m_bSafeForScriptingObjectsOnly = hostSafeControlsOnly;
#endif

    pSite->AddRef();

#ifdef MOZ_ACTIVEX_PLUGIN_XPCONNECT
    
    
    CComPtr<IServiceProvider> sp;
    MozAxPlugin::GetServiceProvider(pData, &sp);
    if (sp)
        pSite->SetServiceProvider(sp);
    CComQIPtr<IOleContainer> container  = sp;
    if (container)
        pSite->SetContainer(container);
#endif

    
    

    
    HRESULT hr;
    if (!downloadControlsIfMissing || !szCodebase)
    {
        hr = pSite->Create(clsid, pl);
    }
    else if (szCodebase)
    {
        CComObject<CInstallControlProgress> *pProgress = NULL;
        CComPtr<IBindCtx> spBindCtx;
        CComPtr<IBindStatusCallback> spOldBSC;
        CComObject<CInstallControlProgress>::CreateInstance(&pProgress);
        pProgress->AddRef();
        CreateBindCtx(0, &spBindCtx);
        RegisterBindStatusCallback(spBindCtx, dynamic_cast<IBindStatusCallback *>(pProgress), &spOldBSC, 0);

        hr = pSite->Create(clsid, pl, szCodebase, spBindCtx);
        if (hr == MK_S_ASYNCHRONOUS)
        {
            pProgress->mNPP = pData->pPluginInstance;
            pProgress->mBindingInProgress = TRUE;
            pProgress->mResult = E_FAIL;

            
            HANDLE hFakeEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
            while (pProgress->mBindingInProgress)
            {
                MSG msg;
                
                while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
                {
                    if (!::GetMessage(&msg, NULL, 0, 0))
                    {
                        pProgress->mBindingInProgress = FALSE;
                        break;
                    }
                    ::TranslateMessage(&msg);
                    ::DispatchMessage(&msg);
                }
                if (!pProgress->mBindingInProgress)
                    break;
                
                ::MsgWaitForMultipleObjects(1, &hFakeEvent, FALSE, 500, QS_ALLEVENTS);
            }
            ::CloseHandle(hFakeEvent);

            hr = pProgress->mResult;
            if (SUCCEEDED(hr))
            {
                hr = pSite->Create(clsid, pl);
            }
        }
        if (pProgress)
        {
            RevokeBindStatusCallback(spBindCtx, dynamic_cast<IBindStatusCallback *>(pProgress));
            pProgress->Release();
        }
    }
    if (FAILED(hr))
    {
        ShowError(MozAxErrorCouldNotCreateControl, clsid);
    }
#if defined(MOZ_ACTIVEX_PLUGIN_XPCONNECT) && defined(XPC_IDISPATCH_SUPPORT)
    if (SUCCEEDED(hr) && hostSafeControlsOnly && !classIsMarkedSafeForScripting)
    {
        CComPtr<IUnknown> cpUnk;
        pSite->GetControlUnknown(&cpUnk);
        nsIID iidDispatch;
        memcpy(&iidDispatch, &__uuidof(IDispatch), sizeof(nsIID));
        PRBool isObjectSafe = PR_FALSE;
        if (!cpUnk ||
            NS_FAILED(dispSupport->IsObjectSafeForScripting(
                reinterpret_cast<void *>(cpUnk.p), iidDispatch, &isObjectSafe)) ||
            !isObjectSafe)
        {
            pSite->Detach();
            hr = E_FAIL;
            ShowError(MozAxErrorControlIsNotSafeForScripting, clsid);
            
        }
    }
#endif

    
    if (FAILED(hr))
    {
        pSite->Release();
        return NPERR_GENERIC_ERROR;
    }
    
#if defined(MOZ_ACTIVEX_PLUGIN_XPCONNECT) && defined(XPC_IDISPATCH_SUPPORT)
    
    nsEventSinkInstance *pSink = NULL;
    nsEventSinkInstance::CreateInstance(&pSink);
    if (pSink)
    {
        pSink->AddRef();
        pSink->mPlugin = pData;
        CComPtr<IUnknown> control;
        pSite->GetControlUnknown(&control);
        pSink->SubscribeToEvents(control);
    }
    pData->pControlEventSink = pSink;
#endif
    pData->nType = itControl;
    pData->pControlSite = pSite;

    return NPERR_NO_ERROR;
}

static NPError
NewControl(const char *pluginType,
           PluginInstanceData *pData,
           uint16_t mode,
           int16_t argc,
           char *argn[],
           char *argv[])
{
    
    CLSID clsid = CLSID_NULL;
    CComBSTR codebase;
    PropertyList pl;

    if (strcmp(pluginType, MIME_OLEOBJECT1) != 0 &&
        strcmp(pluginType, MIME_OLEOBJECT2) != 0)
    {
        clsid = MozAxPlugin::GetCLSIDForType(pluginType);
    }

    for (int16_t i = 0; i < argc; i++)
    {
        if (stricmp(argn[i], "CLSID") == 0 ||
            stricmp(argn[i], "CLASSID") == 0)
        {
            
            
            
            
            
            
            

            const int kCLSIDLen = 256;
            char szCLSID[kCLSIDLen];
            if (strlen(argv[i]) < sizeof(szCLSID))
            {
                if (_strnicmp(argv[i], "CLSID:", 6) == 0)
                {
                    _snprintf(szCLSID, kCLSIDLen - 1, "{%s}", argv[i]+6);
                }
                else if(argv[i][0] != '{')
                {
                    _snprintf(szCLSID, kCLSIDLen - 1, "{%s}", argv[i]);
                }
                else
                {
                    strncpy(szCLSID, argv[i], kCLSIDLen - 1);
                }
                szCLSID[kCLSIDLen - 1] = '\0';
                USES_CONVERSION;
                CLSIDFromString(A2OLE(szCLSID), &clsid);
            }
        }
        else if (stricmp(argn[i], "CODEBASE") == 0)
        {
            codebase = argv[i];

#ifdef XPC_IDISPATCH_SUPPORT
            
            if (argv[i])
            {
                nsCOMPtr<nsIDOMElement> element;
                NPN_GetValue(pData->pPluginInstance, NPNVDOMElement, 
                             static_cast<nsIDOMElement **>(getter_AddRefs(element)));
                if (element)
                {
                    nsCOMPtr<nsIDOMNode> tagAsNode (do_QueryInterface(element));
                    if (tagAsNode)
                    {
                        nsCOMPtr<nsIDOMDocument> DOMdocument;
                        tagAsNode->GetOwnerDocument(getter_AddRefs(DOMdocument));
                        
                        nsCOMPtr<nsIDocument> doc(do_QueryInterface(DOMdocument));
                        if (doc)
                        {
                            nsIURI *baseURI = doc->GetBaseURI();
                            if (baseURI)
                            {
                                nsCAutoString newURL;
                                if (NS_SUCCEEDED(baseURI->Resolve(nsDependentCString(argv[i]), newURL)))
                                {
                                    codebase = newURL.get();
                                }
                            }
                        }
                    }
                }
            }
#endif

        }
        else 
        {
            CComBSTR paramName;
            if (_strnicmp(argn[i], "PARAM_", 6) == 0)
            {
                paramName = argn[i] + 6;
            }
            else if (stricmp(argn[i], "PARAM") == 0)
            {
                
                
                continue;
            }
            else
            {
                paramName = argn[i];
            }

            
            if (!paramName.m_str || paramName.Length() == 0)
            {
                continue;
            }

            USES_CONVERSION;
            CComBSTR paramValue(A2W(argv[i]));

            
            BOOL bFound = FALSE;
            for (unsigned long j = 0; j < pl.GetSize(); j++)
            {
                if (wcscmp(pl.GetNameOf(j), (BSTR) paramName) == 0)
                {
                    bFound = TRUE;
                    break;
                }
            }
            
            
            if (bFound)
            {
                continue;
            }

            
            CComVariant v(paramValue);
            pl.AddNamedProperty(paramName, v);
        }
    }

    return CreateControl(clsid, pData, pl, codebase.m_str);
}







NPError NPP_New(NPMIMEType pluginType,
                NPP instance,
                uint16_t mode,
                int16_t argc,
                char* argn[],
                char* argv[],
                NPSavedData* saved)
{
    ATLTRACE(_T("NPP_New()\n"));

    
    if (instance == NULL)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    PluginInstanceData *pData = new PluginInstanceData;
    if (pData == NULL)
    {
        return NPERR_GENERIC_ERROR;
    }
    pData->pPluginInstance = instance;
    pData->szUrl = NULL;
    pData->szContentType = (pluginType) ? strdup(pluginType) : NULL;
#ifdef MOZ_ACTIVEX_PLUGIN_XPCONNECT
    pData->pScriptingPeer = NULL;
#endif

    
#ifdef MOZ_ACTIVEX_PLUGIN_XPCONNECT
    MozAxPlugin::AddRef();
#endif

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    NPError rv = NPERR_GENERIC_ERROR;
    

    {
        rv = NewControl(pluginType, pData, mode, argc, argn, argv);
    }

    
    if (rv != NPERR_NO_ERROR)
    {
        if (pData->szContentType)
            free(pData->szContentType);
        if (pData->szUrl)
            free(pData->szUrl);
        delete pData;
#ifdef MOZ_ACTIVEX_PLUGIN_XPCONNECT
        MozAxPlugin::Release();
#endif
        CoUninitialize();
        return rv;
    }

    instance->pdata = pData;

    return NPERR_NO_ERROR;
}






NPError
NPP_Destroy(NPP instance, NPSavedData** save)
{
    ATLTRACE(_T("NPP_Destroy()\n"));

    PluginInstanceData *pData = (PluginInstanceData *) instance->pdata;
    if (pData == NULL)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    if (pData->nType == itControl)
    {
        
        CControlSiteInstance *pSite = pData->pControlSite;
        if (pSite)
        {
            pSite->Detach();
            pSite->Release();
        }
#if defined(MOZ_ACTIVEX_PLUGIN_XPCONNECT) && defined(XPC_IDISPATCH_SUPPORT)
        if (pData->pControlEventSink)
        {
            pData->pControlEventSink->UnsubscribeFromEvents();
            pData->pControlEventSink->Release();
        }
#endif
#ifdef MOZ_ACTIVEX_PLUGIN_XPCONNECT
        if (pData->pScriptingPeer)
        {
            pData->pScriptingPeer->Release();
        }
#endif
    }
    else if (pData->nType == itScript)
    {
        
    }

    if (pData->szUrl)
        free(pData->szUrl);
    if (pData->szContentType)
        free(pData->szContentType);
    delete pData;
#ifdef MOZ_ACTIVEX_PLUGIN_XPCONNECT
    MozAxPlugin::Release();
#endif

    instance->pdata = 0;

    CoUninitialize();

    return NPERR_NO_ERROR;

}


















NPError
NPP_SetWindow(NPP instance, NPWindow* window)
{
    ATLTRACE(_T("NPP_SetWindow()\n"));

    
    if (!window)
    {
        return NPERR_GENERIC_ERROR;
    }

    PluginInstanceData *pData = (PluginInstanceData *) instance->pdata;
    if (pData == NULL)
    {
        return  NPERR_INVALID_INSTANCE_ERROR;
    }

    if (pData->nType == itControl)
    {
        CControlSiteInstance *pSite = pData->pControlSite;
        if (pSite == NULL)
        {
            return NPERR_GENERIC_ERROR;
        }

        HWND hwndParent = (HWND) window->window;
        if (hwndParent)
        {
            RECT rcPos;
            GetClientRect(hwndParent, &rcPos);

            if (pSite->GetParentWindow() == NULL)
            {
                pSite->Attach(hwndParent, rcPos, NULL);
            }
            else
            {
                pSite->SetPosition(rcPos);
            }

            
            ::SetWindowLong(hwndParent, GWL_STYLE,
                ::GetWindowLong(hwndParent, GWL_STYLE) | WS_CLIPCHILDREN);
        }
    }

    return NPERR_NO_ERROR;
}



















NPError
NPP_NewStream(NPP instance,
              NPMIMEType type,
              NPStream *stream, 
              NPBool seekable,
              uint16_t *stype)
{
    ATLTRACE(_T("NPP_NewStream()\n"));

    if(!instance)
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    
    stream->pdata = instance->pdata;
    *stype = NP_ASFILE;
    return NPERR_NO_ERROR;
}







void
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
    ATLTRACE(_T("NPP_StreamAsFile()\n"));

    if(fname == NULL || fname[0] == NULL)
    {
        return;
    }
}













int32_t STREAMBUFSIZE = 0X0FFFFFFF;   
                                      
                                      
                                






int32_t
NPP_WriteReady(NPP instance, NPStream *stream)
{
    return STREAMBUFSIZE;  
}






int32_t
NPP_Write(NPP instance, NPStream *stream, int32_t offset, int32_t len, void *buffer)
{   
    return len;
}









NPError
NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
    
    
    
    return NPERR_NO_ERROR;
}






void
NPP_Print(NPP instance, NPPrint* printInfo)
{
    if(printInfo == NULL)   
    {
        return;
    }





}




















void
NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{
    PluginInstanceData *pData = (PluginInstanceData *) instance->pdata;
    if (pData)
    {
        if (pData->szUrl)
            free(pData->szUrl);
        pData->szUrl = strdup(url);
    }
}

NPError
NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
    NPError rv = NPERR_GENERIC_ERROR;
#ifdef MOZ_ACTIVEX_PLUGIN_XPCONNECT
    rv = MozAxPlugin::GetValue(instance, variable, value);
#endif
    return rv;
}

NPError
NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
    return NPERR_GENERIC_ERROR;
}
