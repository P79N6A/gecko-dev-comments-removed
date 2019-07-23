










































#include "XPCPrivate.h"

#include "nsIActiveXSecurityPolicy.h"

static PRBool
ClassIsListed(HKEY hkeyRoot, const TCHAR *szKey, const CLSID &clsid, PRBool &listIsEmpty)
{
    

    listIsEmpty = PR_TRUE;

    CRegKey keyList;
    if(keyList.Open(hkeyRoot, szKey, KEY_READ) != ERROR_SUCCESS)
    {
        
        return PR_FALSE;
    }

    
    int i = 0;
    do {
        USES_CONVERSION;
        TCHAR szCLSID[64];
        const DWORD kBufLength = sizeof(szCLSID) / sizeof(szCLSID[0]);
        memset(szCLSID, 0, sizeof(szCLSID));
        if(::RegEnumKey(keyList, i, szCLSID, kBufLength) != ERROR_SUCCESS)
        {
            
            break;
        }
        ++i;
        listIsEmpty = PR_FALSE;
        szCLSID[kBufLength - 1] = TCHAR('\0');
        CLSID clsidToCompare = GUID_NULL;
        if(SUCCEEDED(::CLSIDFromString(T2OLE(szCLSID), &clsidToCompare)) &&
            ::IsEqualCLSID(clsid, clsidToCompare))
        {
            
            return PR_TRUE;
        }
    } while(1);

    
    return PR_FALSE;
}

static PRBool
ClassExists(const CLSID &clsid)
{
    
    
    CRegKey key;
    if(key.Open(HKEY_CLASSES_ROOT, _T("CLSID"), KEY_READ) != ERROR_SUCCESS)
        return PR_FALSE; 
    
    LPOLESTR szCLSID = NULL;
    if(FAILED(StringFromCLSID(clsid, &szCLSID)))
        return PR_FALSE; 

    USES_CONVERSION;
    CRegKey keyCLSID;
    LONG lResult = keyCLSID.Open(key, W2CT(szCLSID), KEY_READ);
    CoTaskMemFree(szCLSID);
    if(lResult != ERROR_SUCCESS)
        return PR_FALSE; 

    return PR_TRUE;
}

static PRBool
ClassImplementsCategory(const CLSID &clsid, const CATID &catid, PRBool &bClassExists)
{
    bClassExists = ClassExists(clsid);
    
    if(!bClassExists)
        return PR_FALSE;

    
    bClassExists = PR_TRUE;
    CComPtr<ICatInformation> catInfo;
    HRESULT hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL,
        CLSCTX_INPROC_SERVER, __uuidof(ICatInformation), (LPVOID*) &catInfo);
    if(catInfo == NULL)
        return PR_FALSE; 
    
    
    CComPtr<IEnumCATID> enumCATID;
    if(FAILED(catInfo->EnumImplCategoriesOfClass(clsid, &enumCATID)))
        return PR_FALSE; 

    
    BOOL bFound = FALSE;
    CATID catidNext = GUID_NULL;
    while(enumCATID->Next(1, &catidNext, NULL) == S_OK)
    {
        if(::IsEqualCATID(catid, catidNext))
            return PR_TRUE; 
    }
    return PR_FALSE;
}

nsDispatchSupport* nsDispatchSupport::mInstance = nsnull;

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDispatchSupport, nsIDispatchSupport)

nsDispatchSupport::nsDispatchSupport()
{
    
}

nsDispatchSupport::~nsDispatchSupport()
{
    
}







NS_IMETHODIMP nsDispatchSupport::COMVariant2JSVal(VARIANT * comvar, jsval *val)
{
    XPCCallContext ccx(NATIVE_CALLER);
    nsresult retval;
    XPCDispConvert::COMToJS(ccx, *comvar, *val, retval);
    return retval;
}







NS_IMETHODIMP nsDispatchSupport::JSVal2COMVariant(jsval val, VARIANT * comvar)
{
    XPCCallContext ccx(NATIVE_CALLER);
    nsresult retval;
    XPCDispConvert::JSToCOM(ccx, val, *comvar, retval);
    return retval;
}


NS_IMETHODIMP nsDispatchSupport::IsClassSafeToHost(JSContext * cx,
                                                   const nsCID & cid,
                                                   PRBool ignoreException, 
                                                   PRBool *classExists, 
                                                   PRBool *aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(classExists);

    *aResult = PR_FALSE;

    CLSID clsid = XPCDispnsCID2CLSID(cid);

    
    XPCCallContext ccx(JS_CALLER, cx);
    nsIXPCSecurityManager* sm =
            ccx.GetXPCContext()->GetAppropriateSecurityManager(
                        nsIXPCSecurityManager::HOOK_CREATE_INSTANCE);
    *aResult = !sm ||
        NS_SUCCEEDED(sm->CanCreateInstance(ccx, cid));

    if(!*aResult)
    {
        if (ignoreException)
            JS_ClearPendingException(ccx);
        *classExists = PR_TRUE;
        return NS_OK;
    }
    *classExists = ClassExists(clsid);

    
    const TCHAR kIEControlsBlacklist[] = _T("SOFTWARE\\Microsoft\\Internet Explorer\\ActiveX Compatibility");
    CRegKey keyExplorer;
    if(keyExplorer.Open(HKEY_LOCAL_MACHINE,
        kIEControlsBlacklist, KEY_READ) == ERROR_SUCCESS)
    {
        LPOLESTR szCLSID = NULL;
        ::StringFromCLSID(clsid, &szCLSID);
        if(szCLSID)
        {
            CRegKey keyCLSID;
            USES_CONVERSION;
            if(keyCLSID.Open(keyExplorer, W2T(szCLSID), KEY_READ) == ERROR_SUCCESS)
            {
                DWORD dwType = REG_DWORD;
                DWORD dwFlags = 0;
                DWORD dwBufSize = sizeof(dwFlags);
                if(::RegQueryValueEx(keyCLSID, _T("Compatibility Flags"),
                    NULL, &dwType, (LPBYTE) &dwFlags, &dwBufSize) == ERROR_SUCCESS)
                {
                    
                    const DWORD kKillBit = 0x00000400; 
                    if(dwFlags & kKillBit)
                    {
                        ::CoTaskMemFree(szCLSID);
                        *aResult = PR_FALSE;
                        return NS_OK;
                    }
                }
            }
            ::CoTaskMemFree(szCLSID);
        }
    }

    *aResult = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP nsDispatchSupport::IsClassMarkedSafeForScripting(const nsCID & cid, PRBool *classExists, PRBool *aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    NS_ENSURE_ARG_POINTER(classExists);
    
    CLSID clsid = XPCDispnsCID2CLSID(cid);
    *aResult = ClassImplementsCategory(clsid, CATID_SafeForScripting, *classExists);
    return NS_OK;
}


NS_IMETHODIMP nsDispatchSupport::IsObjectSafeForScripting(void * theObject, const nsIID & id, PRBool *aResult)
{
    NS_ENSURE_ARG_POINTER(theObject);
    NS_ENSURE_ARG_POINTER(aResult);

    
    IUnknown *pObject = (IUnknown *) theObject;
    IID iid = XPCDispIID2IID(id);

    
    CComQIPtr<IObjectSafety> objectSafety = pObject;
    if(!objectSafety)
    {
        *aResult = PR_TRUE;
        return NS_OK;
    }

    DWORD dwSupported = 0; 
    DWORD dwEnabled = 0;   

    
    if(FAILED(objectSafety->GetInterfaceSafetyOptions(
            iid, &dwSupported, &dwEnabled)))
    {
        
        *aResult = PR_FALSE;
        return NS_OK;
    }

    
    if(!(dwEnabled & dwSupported) & INTERFACESAFE_FOR_UNTRUSTED_CALLER)
    {
        
        

        if(!(dwSupported & INTERFACESAFE_FOR_UNTRUSTED_CALLER) ||
            FAILED(objectSafety->SetInterfaceSafetyOptions(
                iid, INTERFACESAFE_FOR_UNTRUSTED_CALLER, INTERFACESAFE_FOR_UNTRUSTED_CALLER)) ||
            FAILED(objectSafety->GetInterfaceSafetyOptions(
                iid, &dwSupported, &dwEnabled)) ||
            !(dwEnabled & dwSupported) & INTERFACESAFE_FOR_UNTRUSTED_CALLER)
        {
            *aResult = PR_FALSE;
            return NS_OK;
        }
    }

    *aResult = PR_TRUE;
    return NS_OK;
}

static const PRUint32 kDefaultHostingFlags =
    nsIActiveXSecurityPolicy::HOSTING_FLAGS_HOST_NOTHING;


NS_IMETHODIMP nsDispatchSupport::GetHostingFlags(const char *aContext, PRUint32 *aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);

    
    nsresult rv;
    nsCOMPtr<nsIActiveXSecurityPolicy> securityPolicy =
        do_GetService(NS_IACTIVEXSECURITYPOLICY_CONTRACTID, &rv);
    if(NS_SUCCEEDED(rv) && securityPolicy)
        return securityPolicy->GetHostingFlags(aContext, aResult);
    
    
    *aResult = kDefaultHostingFlags;
    return NS_OK;
}

nsDispatchSupport* nsDispatchSupport::GetSingleton()
{
    if(!mInstance)
    {
        mInstance = new nsDispatchSupport;
        NS_IF_ADDREF(mInstance);
    }
    NS_IF_ADDREF(mInstance);
    return mInstance;
}
