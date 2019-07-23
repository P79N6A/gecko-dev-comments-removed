





































#ifndef CONTROLEVENTSINK_H
#define CONTROLEVENTSINK_H



class CControlEventSink : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatch
{
public:
    CControlEventSink();

    
    CComPtr<IConnectionPoint> m_spEventCP;
    CComPtr<ITypeInfo> m_spEventSinkTypeInfo;
    DWORD m_dwEventCookie;
    IID m_EventIID;

protected:
    virtual ~CControlEventSink();

    static HRESULT WINAPI SinkQI(void* pv, REFIID riid, LPVOID* ppv, DWORD dw)
    {
        CControlEventSink *pThis = (CControlEventSink *) pv;
        if (!IsEqualIID(pThis->m_EventIID, GUID_NULL) && 
             IsEqualIID(pThis->m_EventIID, riid))
        {
            return pThis->QueryInterface(__uuidof(IDispatch), ppv);
        }
        return E_NOINTERFACE;
    }

public:

BEGIN_COM_MAP(CControlEventSink)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_FUNC_BLIND(0, SinkQI)
END_COM_MAP()

    virtual HRESULT SubscribeToEvents(IUnknown *pControl);
    virtual void UnsubscribeFromEvents();
    virtual BOOL GetEventSinkIID(IUnknown *pControl, IID &iid, ITypeInfo **typeInfo);
    virtual HRESULT InternalInvoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);


    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount( UINT __RPC_FAR *pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo( UINT iTInfo,  LCID lcid,  ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames( REFIID riid,  LPOLESTR __RPC_FAR *rgszNames,  UINT cNames,  LCID lcid,  DISPID __RPC_FAR *rgDispId);
    virtual  HRESULT STDMETHODCALLTYPE Invoke( DISPID dispIdMember,  REFIID riid,  LCID lcid,  WORD wFlags,  DISPPARAMS __RPC_FAR *pDispParams,  VARIANT __RPC_FAR *pVarResult,  EXCEPINFO __RPC_FAR *pExcepInfo,  UINT __RPC_FAR *puArgErr);
};

typedef CComObject<CControlEventSink> CControlEventSinkInstance;

#endif