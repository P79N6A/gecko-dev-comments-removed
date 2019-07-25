



































#ifndef PLUGINHOSTWND_H
#define PLUGINHOSTWND_H

#include "nsPluginWnd.h"

class nsURLDataCallback;

template <class Entry>
class nsSimpleArray
{
    Entry *m_pData;
    unsigned long m_nSize;
    unsigned long m_nMaxSize;
    unsigned long m_nExpandArrayBy;
public:
    nsSimpleArray(unsigned long nExpandArrayBy = 10) :
      m_pData(NULL),
      m_nSize(0),
      m_nMaxSize(0),
      m_nExpandArrayBy(nExpandArrayBy)
    {
    }
    
    virtual ~nsSimpleArray()
    {
        Empty();
    }

    Entry ElementAt(unsigned long aIndex) const
    {
        ATLASSERT(aIndex < m_nSize);
        return m_pData[aIndex];
    }

    Entry operator[](unsigned long aIndex) const
    {
        return ElementAt(aIndex);
    }

    void AppendElement(Entry entry)
    {
        if (!m_pData)
        {
            m_nMaxSize = m_nExpandArrayBy;
            m_pData = (Entry *) malloc(sizeof(Entry) * m_nMaxSize);
        }
        else if (m_nSize == m_nMaxSize)
        {
            m_nMaxSize += m_nExpandArrayBy;
            m_pData = (Entry *) realloc(m_pData, sizeof(Entry) * m_nMaxSize);
        }
        ATLASSERT(m_pData);
        if (m_pData)
        {
            m_pData[m_nSize++] = entry;
        }
    }

    void RemoveElementAt(unsigned long nIndex)
    {
        ATLASSERT(aIndex < m_nSize);
        if (nIndex < m_nSize)
        {
            m_nSize--;
            if (m_nSize > 0)
            {
                m_pData[nIndex] = m_pData[m_nSize - 1];
                m_nSize--;
            }
        }
    }

    void RemoveElement(Entry entry)
    {
        unsigned long i = 0;
        while (i < m_nSize)
        {
            if (m_pData[i] == entry)
            {
                m_nSize--;
                if (m_nSize > 0)
                {
                    m_pData[i] = m_pData[m_nSize - 1];
                    m_nSize--;
                }
                continue;
            }
            i++;
        }
    }

    void Empty()
    {
        if (m_pData)
        {
            free(m_pData);
            m_pData = NULL;
            m_nSize = m_nMaxSize = 0;
        }
    }
    BOOL IsEmpty() const { return m_nSize == 0 ? TRUE : FALSE; }
    unsigned long Count() const { return m_nSize; }
};


#define PLUGINS_FROM_IE       0x1
#define PLUGINS_FROM_NS4X     0x2
#define PLUGINS_FROM_FIREFOX  0x4
#define PLUGINS_FROM_MOZILLA  0x8

class nsPluginHostWnd : public CWindowImpl<nsPluginHostWnd>
{
public:
    nsPluginHostWnd();
    virtual ~nsPluginHostWnd();

DECLARE_WND_CLASS(_T("MozCtrlPluginHostWindow"))

BEGIN_MSG_MAP(nsPluginHostWnd)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)

END_MSG_MAP()


public:
	LRESULT OnCreate(UINT , WPARAM , LPARAM , BOOL& );
	LRESULT OnDestroy(UINT , WPARAM , LPARAM , BOOL& );
	LRESULT OnSize(UINT , WPARAM , LPARAM , BOOL& );
	LRESULT OnPaint(UINT , WPARAM , LPARAM , BOOL& );
	LRESULT OnMouseMove(UINT , WPARAM , LPARAM , BOOL& );
	LRESULT OnLButtonDown(UINT , WPARAM , LPARAM , BOOL& );
	LRESULT OnLButtonUp(UINT , WPARAM , LPARAM , BOOL& );

public:
	CComBSTR m_bstrText;

public:
    CComBSTR m_bstrContentType;
    CComBSTR m_bstrSource;
    CComBSTR m_bstrPluginsPage;

    
    struct PluginInfo
    {
        TCHAR *szPluginPath;
        TCHAR *szPluginName;
        TCHAR *szMIMEType;
    };

    nsSimpleArray<PluginInfo *> m_Plugins;

    
    unsigned long m_nArgs;
    unsigned long m_nArgsMax;
    char **m_pszArgNames;
    char **m_pszArgValues;

    
    struct LoadedPluginInfo
    {
        TCHAR *szFullPath; 
        HINSTANCE hInstance;
        DWORD nRefCount;
        NPPluginFuncs NPPFuncs;
    };

    static nsSimpleArray<LoadedPluginInfo *> m_LoadedPlugins;

    LoadedPluginInfo *m_pLoadedPlugin;

    NPWindow m_NPWindow;

    static NPNetscapeFuncs g_NPNFuncs;
    static HRESULT InitPluginCallbacks();

    HRESULT CreatePluginList(unsigned long ulFlags);
    HRESULT CreatePluginListFrom(const TCHAR *szPluginsDir);
    HRESULT CleanupPluginList();
    HRESULT GetPluginInfo(const TCHAR * pszPluginPath, PluginInfo *pInfo);

public:
    NPP_t m_NPP;
    bool m_bPluginIsAlive;
    bool m_bCreatePluginFromStreamData;
    bool m_bPluginIsWindowless;
    bool m_bPluginIsTransparent;

    nsPluginWnd m_wndPlugin;

    
    NPPluginFuncs m_NPPFuncs;

    virtual HRESULT GetWebBrowserApp(IWebBrowserApp **pBrowser);

    HRESULT GetBaseURL(TCHAR **szBaseURL);

	HRESULT GetPluginSource( BSTR *pVal);
	HRESULT SetPluginSource( BSTR newVal);
	HRESULT GetPluginContentType( BSTR *pVal);
	HRESULT SetPluginContentType( BSTR newVal);
	HRESULT GetPluginsPage( BSTR *pVal);
	HRESULT SetPluginsPage( BSTR newVal);

    HRESULT AddPluginParam(const char *szName, const char *szValue);
    HRESULT LoadPluginByContentType(const TCHAR *pszContentType);
    HRESULT ReadPluginsFromGeckoAppPath(const TCHAR *szAppName);
	HRESULT LoadPlugin(const TCHAR *pszPluginPath);
    HRESULT FindPluginPathByContentType(const TCHAR *pszContentType, TCHAR **ppszPluginPath);
    HRESULT UnloadPlugin();

    HRESULT OpenURLStream(const TCHAR *szURL, void *pNotifyData, const void *pPostData, unsigned long nDataLength);

    HRESULT CreatePluginInstance();
    HRESULT DestroyPluginInstance();
    HRESULT SizeToFitPluginInstance();

    void SetPluginWindowless(bool bWindowless);
    void SetPluginTransparent(bool bTransparent);
};

#endif
