





































#include "stdafx.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "ActiveScriptSite.h"


CActiveScriptSite::CActiveScriptSite()
{
    m_ssScriptState = SCRIPTSTATE_UNINITIALIZED;
}


CActiveScriptSite::~CActiveScriptSite()
{
    Detach();
}


HRESULT CActiveScriptSite::Attach(CLSID clsidScriptEngine)
{
    
    Detach();

    
    HRESULT hr = m_spIActiveScript.CoCreateInstance(clsidScriptEngine);
    if (FAILED(hr))
    {
        return hr;
    }

    
    m_spIActiveScript->SetScriptSite(this);

    
    CIPtr(IActiveScriptParse) spActiveScriptParse = m_spIActiveScript;
    if (spActiveScriptParse)
    {
        spActiveScriptParse->InitNew();
    }
    else
    {
    }

    return S_OK;
}


HRESULT CActiveScriptSite::Detach()
{
    if (m_spIActiveScript)
    {
        StopScript();
        m_spIActiveScript->Close();
        m_spIActiveScript.Release();
    }

    return S_OK;
}


HRESULT CActiveScriptSite::AttachVBScript()
{
    static const CLSID CLSID_VBScript =
    { 0xB54F3741, 0x5B07, 0x11CF, { 0xA4, 0xB0, 0x00, 0xAA, 0x00, 0x4A, 0x55, 0xE8} };
    
    return Attach(CLSID_VBScript);
}


HRESULT CActiveScriptSite::AttachJScript()
{
    static const CLSID CLSID_JScript =
    { 0xF414C260, 0x6AC0, 0x11CF, { 0xB6, 0xD1, 0x00, 0xAA, 0x00, 0xBB, 0xBB, 0x58} };

    return Attach(CLSID_JScript);
}


HRESULT CActiveScriptSite::AddNamedObject(const TCHAR *szName, IUnknown *pObject, BOOL bGlobalMembers)
{
    if (m_spIActiveScript == NULL)
    {
        return E_UNEXPECTED;
    }

    if (pObject == NULL || szName == NULL)
    {
        return E_INVALIDARG;
    }

    
    CNamedObjectList::iterator i = m_cObjectList.find(szName);
    if (i != m_cObjectList.end())
    {
        return E_FAIL;
    }

    
    m_cObjectList.insert(CNamedObjectList::value_type(szName, pObject));

    
    HRESULT hr;
    USES_CONVERSION;
    DWORD dwFlags = SCRIPTITEM_ISSOURCE | SCRIPTITEM_ISVISIBLE;
    if (bGlobalMembers)
    {
        dwFlags |= SCRIPTITEM_GLOBALMEMBERS;
    }

    hr = m_spIActiveScript->AddNamedItem(T2OLE(szName), dwFlags);

    if (FAILED(hr))
    {
        m_cObjectList.erase(szName);
        return hr;
    }

    return S_OK;
}


HRESULT CActiveScriptSite::ParseScriptFile(const TCHAR *szFile)
{
    USES_CONVERSION;
    const char *pszFileName = T2CA(szFile);
    
    
    struct _stat cStat;
    _stat(pszFileName, &cStat);

    
    size_t nBufSize = cStat.st_size + 1;
    char *pBuffer = (char *) malloc(nBufSize);
    if (pBuffer == NULL)
    {
        return E_OUTOFMEMORY;
    }
    memset(pBuffer, 0, nBufSize);

    
    HRESULT hr = E_FAIL;
    FILE *f = fopen(pszFileName, "rb");
    if (f)
    {
        fread(pBuffer, 1, nBufSize - 1, f);
        hr = ParseScriptText(A2T(pBuffer));
        fclose(f);
    }

    free(pBuffer);

    return hr;
}


HRESULT CActiveScriptSite::ParseScriptText(const TCHAR *szScript)
{
    if (m_spIActiveScript == NULL)
    {
        return E_UNEXPECTED;
    }

    CIPtr(IActiveScriptParse) spIActiveScriptParse = m_spIActiveScript;
    if (spIActiveScriptParse)
    {
        USES_CONVERSION;

        CComVariant vResult;
        DWORD dwCookie = 0; 
        DWORD dwFlags = 0;
        EXCEPINFO cExcepInfo;
        HRESULT hr;

        hr = spIActiveScriptParse->ParseScriptText(
                    T2OLE(szScript),
                    NULL, NULL, NULL, dwCookie, 0, dwFlags,
                    &vResult, &cExcepInfo);

        if (FAILED(hr))
        {
            return E_FAIL;
        }
    }
    else
    {
        CIPtr(IPersistStream) spPersistStream = m_spIActiveScript;
        CIPtr(IStream) spStream;

        
        if (spPersistStream &&
            SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &spStream)))
        {
            USES_CONVERSION;
            LARGE_INTEGER cPos = { 0, 0 };
            LPOLESTR szText = T2OLE(szScript);
            spStream->Write(szText, wcslen(szText) * sizeof(WCHAR), NULL);
            spStream->Seek(cPos, STREAM_SEEK_SET, NULL);
            spPersistStream->Load(spStream);
        }
        else
        {
            return E_UNEXPECTED;
        }
    }

    return S_OK;
}


HRESULT CActiveScriptSite::PlayScript()
{
    if (m_spIActiveScript == NULL)
    {
        return E_UNEXPECTED;
    }

    m_spIActiveScript->SetScriptState(SCRIPTSTATE_CONNECTED);

    return S_OK;
}


HRESULT CActiveScriptSite::StopScript()
{
    if (m_spIActiveScript == NULL)
    {
        return E_UNEXPECTED;
    }

    m_spIActiveScript->SetScriptState(SCRIPTSTATE_DISCONNECTED);

    return S_OK;
}





HRESULT STDMETHODCALLTYPE CActiveScriptSite::GetLCID( LCID __RPC_FAR *plcid)
{
    
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CActiveScriptSite::GetItemInfo( LPCOLESTR pstrName,  DWORD dwReturnMask,  IUnknown __RPC_FAR *__RPC_FAR *ppiunkItem,  ITypeInfo __RPC_FAR *__RPC_FAR *ppti)
{
    if (pstrName == NULL)
    {
        return E_INVALIDARG;
    }
    
    if (ppiunkItem)
    {
        *ppiunkItem = NULL;
    }

    if (ppti)
    {
        *ppti = NULL;
    }

    USES_CONVERSION;

    
    CIUnkPtr spUnkObject;
    CNamedObjectList::iterator i = m_cObjectList.find(OLE2T(pstrName));
    if (i != m_cObjectList.end())
    {
        spUnkObject = (*i).second;
    }

    
    if (spUnkObject == NULL) 
    {
        return TYPE_E_ELEMENTNOTFOUND;
    }
    if (dwReturnMask & SCRIPTINFO_IUNKNOWN) 
    {
        spUnkObject->QueryInterface(IID_IUnknown, (void **) ppiunkItem);
    }
    if (dwReturnMask & SCRIPTINFO_ITYPEINFO) 
    {
        
        CIPtr(IDispatch) spIDispatch = spUnkObject;
        if (spIDispatch)
        {
            HRESULT hr;
            hr = spIDispatch->GetTypeInfo(0, GetSystemDefaultLCID(), ppti);
            if (FAILED(hr))
            {
                *ppti = NULL;
            }
        }
    }

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CActiveScriptSite::GetDocVersionString( BSTR __RPC_FAR *pbstrVersion)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CActiveScriptSite::OnScriptTerminate( const VARIANT __RPC_FAR *pvarResult,  const EXCEPINFO __RPC_FAR *pexcepinfo)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CActiveScriptSite::OnStateChange( SCRIPTSTATE ssScriptState)
{
    m_ssScriptState = ssScriptState;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CActiveScriptSite::OnScriptError( IActiveScriptError __RPC_FAR *pscripterror)
{
    BSTR bstrSourceLineText = NULL;
    DWORD dwSourceContext = 0;
    ULONG pulLineNumber = 0;
    LONG  ichCharPosition = 0;
    EXCEPINFO cExcepInfo;

    memset(&cExcepInfo, 0, sizeof(cExcepInfo));

    
    pscripterror->GetSourcePosition(&dwSourceContext, &pulLineNumber, &ichCharPosition);
    pscripterror->GetSourceLineText(&bstrSourceLineText);
    pscripterror->GetExceptionInfo(&cExcepInfo);

    tstring szDescription(_T("(No description)"));
    if (cExcepInfo.bstrDescription)
    {
        
        USES_CONVERSION;
        szDescription = OLE2T(cExcepInfo.bstrDescription);
    }

    ATLTRACE(_T("Script Error: %s, code=0x%08x\n"), szDescription.c_str(), cExcepInfo.scode);

    SysFreeString(bstrSourceLineText);

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CActiveScriptSite::OnEnterScript(void)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CActiveScriptSite::OnLeaveScript(void)
{
    return S_OK;
}


