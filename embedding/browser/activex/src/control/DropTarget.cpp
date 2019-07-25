





































#include "stdafx.h"

#include <intshcut.h>
#include <shlobj.h>

#include "DropTarget.h"

#ifndef CFSTR_SHELLURL
#define CFSTR_SHELLURL _T("UniformResourceLocator")
#endif

#ifndef CFSTR_FILENAME
#define CFSTR_FILENAME _T("FileName")
#endif

#ifndef CFSTR_FILENAMEW
#define CFSTR_FILENAMEW _T("FileNameW")
#endif

static const UINT g_cfURL = RegisterClipboardFormat(CFSTR_SHELLURL);
static const UINT g_cfFileName = RegisterClipboardFormat(CFSTR_FILENAME);
static const UINT g_cfFileNameW = RegisterClipboardFormat(CFSTR_FILENAMEW);

CDropTarget::CDropTarget()
{
    m_pOwner = NULL;
}


CDropTarget::~CDropTarget()
{
}


void CDropTarget::SetOwner(CMozillaBrowser *pOwner)
{
    m_pOwner = pOwner;
}


HRESULT CDropTarget::GetURLFromFile(const TCHAR *pszFile, tstring &szURL)
{
    USES_CONVERSION;
    CIPtr(IUniformResourceLocator) spUrl;

    
    HRESULT hr = CoCreateInstance (CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER, IID_IUniformResourceLocator, (void **) &spUrl);
    if (spUrl == NULL)
    {
        return E_FAIL;
    }

    
    CIPtr(IPersistFile) spFile = spUrl;
    if (spFile == NULL)
    {
        return E_FAIL;
    }

    
    LPSTR lpTemp = NULL;
    if (FAILED(spFile->Load(T2OLE(pszFile), STGM_READ)) ||
        FAILED(spUrl->GetURL(&lpTemp)))
    {
        return E_FAIL;
    }

    
    CIPtr(IMalloc) spMalloc;
    if (FAILED(SHGetMalloc(&spMalloc)))
    {
        return E_FAIL;
    }
    
    
    szURL = A2T(lpTemp);
    spMalloc->Free(lpTemp);

    return S_OK;
}




HRESULT STDMETHODCALLTYPE CDropTarget::DragEnter( IDataObject __RPC_FAR *pDataObj,  DWORD grfKeyState,  POINTL pt,  DWORD __RPC_FAR *pdwEffect)
{
    if (pdwEffect == NULL || pDataObj == NULL)
    {
        NG_ASSERT(0);
        return E_INVALIDARG;
    }

    if (m_spDataObject != NULL)
    {
        NG_ASSERT(0);
        return E_UNEXPECTED;
    }

    
    FORMATETC formatetc;
    memset(&formatetc, 0, sizeof(formatetc));
    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.lindex = -1;
    formatetc.tymed = TYMED_HGLOBAL;

    
    formatetc.cfFormat = g_cfURL;
    if (pDataObj->QueryGetData(&formatetc) == S_OK)
    {
        m_spDataObject = pDataObj;
        *pdwEffect = DROPEFFECT_LINK;
        return S_OK;
    }

    
    formatetc.cfFormat = g_cfFileName;
    if (pDataObj->QueryGetData(&formatetc) == S_OK)
    {
        m_spDataObject = pDataObj;
        *pdwEffect = DROPEFFECT_LINK;
        return S_OK;
    }
    
    
    formatetc.cfFormat = g_cfFileName;
    if (pDataObj->QueryGetData(&formatetc) == S_OK)
    {
        m_spDataObject = pDataObj;
        *pdwEffect = DROPEFFECT_LINK;
        return S_OK;
    }

    
    *pdwEffect = DROPEFFECT_NONE;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CDropTarget::DragOver( DWORD grfKeyState,  POINTL pt,  DWORD __RPC_FAR *pdwEffect)
{
    if (pdwEffect == NULL)
    {
        NG_ASSERT(0);
        return E_INVALIDARG;
    }
    *pdwEffect = m_spDataObject ? DROPEFFECT_LINK : DROPEFFECT_NONE;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CDropTarget::DragLeave(void)
{
    if (m_spDataObject)
    {
        m_spDataObject.Release();
    }
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CDropTarget::Drop( IDataObject __RPC_FAR *pDataObj,  DWORD grfKeyState,  POINTL pt,  DWORD __RPC_FAR *pdwEffect)
{
    if (pdwEffect == NULL)
    {
        NG_ASSERT(0);
        return E_INVALIDARG;
    }
    if (m_spDataObject == NULL)
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    *pdwEffect = DROPEFFECT_LINK;

    
    BSTR bstrURL = NULL;
    FORMATETC formatetc;
    STGMEDIUM stgmedium;
    memset(&formatetc, 0, sizeof(formatetc));
    memset(&stgmedium, 0, sizeof(formatetc));

    formatetc.dwAspect = DVASPECT_CONTENT;
    formatetc.lindex = -1;
    formatetc.tymed = TYMED_HGLOBAL;

    
    formatetc.cfFormat = g_cfURL;
    if (m_spDataObject->GetData(&formatetc, &stgmedium) == S_OK)
    {
        NG_ASSERT(stgmedium.tymed == TYMED_HGLOBAL);
        NG_ASSERT(stgmedium.hGlobal);
        char *pszURL = (char *) GlobalLock(stgmedium.hGlobal);
        NG_TRACE("URL \"%s\" dragged over control\n", pszURL);
        
        if (m_pOwner)
        {
            USES_CONVERSION;
            bstrURL = SysAllocString(A2OLE(pszURL));
        }
        GlobalUnlock(stgmedium.hGlobal);
        goto finish;
    }

    
    formatetc.cfFormat = g_cfFileName;
    if (m_spDataObject->GetData(&formatetc, &stgmedium) == S_OK)
    {
        USES_CONVERSION;
        NG_ASSERT(stgmedium.tymed == TYMED_HGLOBAL);
        NG_ASSERT(stgmedium.hGlobal);
        tstring szURL;
        char *pszURLFile = (char *) GlobalLock(stgmedium.hGlobal);
        NG_TRACE("File \"%s\" dragged over control\n", pszURLFile);
        if (SUCCEEDED(GetURLFromFile(A2T(pszURLFile), szURL)))
        {
            bstrURL = SysAllocString(T2OLE(szURL.c_str()));
        }
        GlobalUnlock(stgmedium.hGlobal);
        goto finish;
    }
    
    
    formatetc.cfFormat = g_cfFileNameW;
    if (m_spDataObject->GetData(&formatetc, &stgmedium) == S_OK)
    {
        USES_CONVERSION;
        NG_ASSERT(stgmedium.tymed == TYMED_HGLOBAL);
        NG_ASSERT(stgmedium.hGlobal);
        tstring szURL;
        WCHAR *pszURLFile = (WCHAR *) GlobalLock(stgmedium.hGlobal);
        NG_TRACE("File \"%s\" dragged over control\n", W2A(pszURLFile));
        if (SUCCEEDED(GetURLFromFile(W2T(pszURLFile), szURL)))
        {
            USES_CONVERSION;
            bstrURL = SysAllocString(T2OLE(szURL.c_str()));
        }
        GlobalUnlock(stgmedium.hGlobal);
        goto finish;
    }

finish:
    
    if (bstrURL)
    {
        m_pOwner->Navigate(bstrURL, NULL, NULL, NULL, NULL);
        SysFreeString(bstrURL);
    }

    ReleaseStgMedium(&stgmedium);
    m_spDataObject.Release();

    return S_OK;
}

