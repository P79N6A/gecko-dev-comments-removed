




































#ifndef ACTIVESCRIPTSITE_H
#define ACTIVESCRIPTSITE_H


class CActiveScriptSite :    public CComObjectRootEx<CComSingleThreadModel>,
                            public IActiveScriptSite
{
    
    CComQIPtr<IActiveScript, &IID_IActiveScript> m_spIActiveScript;

    
    CNamedObjectList m_cObjectList;

    
    SCRIPTSTATE m_ssScriptState;

public:
    CActiveScriptSite();
    virtual ~CActiveScriptSite();

BEGIN_COM_MAP(CActiveScriptSite)
    COM_INTERFACE_ENTRY(IActiveScriptSite)
END_COM_MAP()

    
    virtual HRESULT Attach(CLSID clsidScriptEngine);
    
    virtual HRESULT AttachVBScript();
    
    virtual HRESULT AttachJScript();
    
    virtual HRESULT Detach();
    
    virtual SCRIPTSTATE GetScriptState() const
    {
        return m_ssScriptState;
    }

    
    virtual HRESULT ParseScriptText(const TCHAR *szScript);
    
    virtual HRESULT ParseScriptFile(const TCHAR *szFile);
    
    virtual HRESULT AddNamedObject(const TCHAR *szName, IUnknown *pObject, BOOL bGlobalMembers = FALSE);
    
    virtual HRESULT PlayScript();
    
    virtual HRESULT StopScript();

public:
    
    virtual HRESULT STDMETHODCALLTYPE GetLCID( LCID __RPC_FAR *plcid);
    virtual HRESULT STDMETHODCALLTYPE GetItemInfo( LPCOLESTR pstrName,  DWORD dwReturnMask,  IUnknown __RPC_FAR *__RPC_FAR *ppiunkItem,  ITypeInfo __RPC_FAR *__RPC_FAR *ppti);
    virtual HRESULT STDMETHODCALLTYPE GetDocVersionString( BSTR __RPC_FAR *pbstrVersion);
    virtual HRESULT STDMETHODCALLTYPE OnScriptTerminate( const VARIANT __RPC_FAR *pvarResult,  const EXCEPINFO __RPC_FAR *pexcepinfo);
    virtual HRESULT STDMETHODCALLTYPE OnStateChange( SCRIPTSTATE ssScriptState);
    virtual HRESULT STDMETHODCALLTYPE OnScriptError( IActiveScriptError __RPC_FAR *pscripterror);
    virtual HRESULT STDMETHODCALLTYPE OnEnterScript(void);
    virtual HRESULT STDMETHODCALLTYPE OnLeaveScript(void);
};

typedef CComObject<CActiveScriptSite> CActiveScriptSiteInstance;

#endif