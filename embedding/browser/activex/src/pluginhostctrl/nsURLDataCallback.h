



































#ifndef __NSURLDATACALLBACK_H_
#define __NSURLDATACALLBACK_H_

#include "resource.h"       

#include <stdio.h>

#include "npapi.h"

class nsPluginHostWnd;

#define WM_NPP_NEWSTREAM      WM_USER 
#define WM_NPP_DESTROYSTREAM  WM_USER + 1
#define WM_NPP_URLNOTIFY      WM_USER + 2
#define WM_NPP_WRITEREADY     WM_USER + 3
#define WM_NPP_WRITE          WM_USER + 4

#define WM_CLASS_CLEANUP      WM_USER + 10
#define WM_CLASS_CREATEPLUGININSTANCE WM_USER + 11

struct _DestroyStreamData
{
    NPP npp;
    NPStream *stream;
    NPReason reason;
};

struct _UrlNotifyData
{
    NPP npp;
    char *url;
    NPReason reason;
    void *notifydata;
};

struct _NewStreamData
{
    NPP npp;
    char *contenttype;
    NPStream *stream;
    NPBool seekable;
    uint16 *stype;
};

struct _WriteReadyData
{
    NPP npp;
    NPStream *stream;
    int32 result;
};

struct _WriteData
{
    NPP npp;
    NPStream *stream;
    int32 offset;
    int32 len;
    void* buffer;
};



class ATL_NO_VTABLE nsURLDataCallback : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CWindowImpl<nsURLDataCallback, CWindow, CNullTraits>,
    public CComCoClass<nsURLDataCallback, &CLSID_NULL>,
    public IBindStatusCallback,
    public IAuthenticate
{
public:
    nsURLDataCallback();

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(nsURLDataCallback)
    COM_INTERFACE_ENTRY(IBindStatusCallback)
    COM_INTERFACE_ENTRY(IAuthenticate)
END_COM_MAP()

    DECLARE_WND_CLASS(_T("MozStreamWindow"))

BEGIN_MSG_MAP(nsURLDataCallback)
    MESSAGE_HANDLER(WM_NPP_NEWSTREAM, OnNPPNewStream)
    MESSAGE_HANDLER(WM_NPP_DESTROYSTREAM, OnNPPDestroyStream)
    MESSAGE_HANDLER(WM_NPP_URLNOTIFY, OnNPPURLNotify)
    MESSAGE_HANDLER(WM_NPP_WRITEREADY, OnNPPWriteReady)
    MESSAGE_HANDLER(WM_NPP_WRITE, OnNPPWrite)
    MESSAGE_HANDLER(WM_CLASS_CLEANUP, OnClassCleanup)
    MESSAGE_HANDLER(WM_CLASS_CREATEPLUGININSTANCE, OnClassCreatePluginInstance)
END_MSG_MAP()

    LRESULT OnNPPNewStream(UINT , WPARAM , LPARAM , BOOL& );
    LRESULT OnNPPDestroyStream(UINT , WPARAM , LPARAM , BOOL& );
    LRESULT OnNPPURLNotify(UINT , WPARAM , LPARAM , BOOL& );
    LRESULT OnNPPWriteReady(UINT , WPARAM , LPARAM , BOOL& );
    LRESULT OnNPPWrite(UINT , WPARAM , LPARAM , BOOL& );

    LRESULT OnClassCreatePluginInstance(UINT , WPARAM , LPARAM , BOOL& );
    LRESULT OnClassCleanup(UINT , WPARAM , LPARAM , BOOL& );

protected:
    virtual ~nsURLDataCallback();

protected:
    nsPluginHostWnd *m_pOwner;
    void *m_pNotifyData;
    HGLOBAL m_hPostData;

    NPStream m_NPStream;
    unsigned long m_nDataPos;
    unsigned long m_nDataMax;

    char *m_szContentType;
    char *m_szURL;

    BOOL m_bSaveToTempFile;
    BOOL m_bNotifyOnWrite;
    FILE *m_pTempFile;
    char *m_szTempFileName;

    CComPtr<IBinding> m_cpBinding;

    void SetURL(const char *szURL)
    {
        if (m_szURL) { free(m_szURL); m_szURL = NULL; }
        if (szURL) { m_szURL = strdup(szURL); }
    }
    void SetContentType(const char *szContentType)
    {
        if (m_szContentType) { free(m_szContentType); m_szContentType = NULL; }
        if (szContentType) { m_szContentType = strdup(szContentType); }
    }
    void SetPostData(const void *pData, unsigned long nSize);
    void SetOwner(nsPluginHostWnd *pOwner) { m_pOwner = pOwner; }
    void SetNotifyData(void *pNotifyData)   { m_pNotifyData = pNotifyData; }
    
    static void __cdecl StreamThread(void *pThis);

public:
    static HRESULT OpenURL(nsPluginHostWnd *pOwner, const TCHAR *szURL, void *pNotifyData, const void *pData, unsigned long nSize);


public:
    virtual HRESULT STDMETHODCALLTYPE OnStartBinding( 
         DWORD dwReserved,
         IBinding __RPC_FAR *pib);
    
    virtual HRESULT STDMETHODCALLTYPE GetPriority( 
         LONG __RPC_FAR *pnPriority);
    
    virtual HRESULT STDMETHODCALLTYPE OnLowResource( 
         DWORD reserved);
    
    virtual HRESULT STDMETHODCALLTYPE OnProgress( 
         ULONG ulProgress,
         ULONG ulProgressMax,
         ULONG ulStatusCode,
         LPCWSTR szStatusText);
    
    virtual HRESULT STDMETHODCALLTYPE OnStopBinding( 
         HRESULT hresult,
         LPCWSTR szError);
    
    virtual  HRESULT STDMETHODCALLTYPE GetBindInfo( 
         DWORD __RPC_FAR *grfBINDF,
         BINDINFO __RPC_FAR *pbindinfo);
    
    virtual  HRESULT STDMETHODCALLTYPE OnDataAvailable( 
         DWORD grfBSCF,
         DWORD dwSize,
         FORMATETC __RPC_FAR *pformatetc,
         STGMEDIUM __RPC_FAR *pstgmed);
    
    virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable( 
         REFIID riid,
         IUnknown __RPC_FAR *punk);


public:
    virtual HRESULT STDMETHODCALLTYPE Authenticate( 
         HWND __RPC_FAR *phwnd,
         LPWSTR __RPC_FAR *pszUsername,
         LPWSTR __RPC_FAR *pszPassword);
};

#endif 
