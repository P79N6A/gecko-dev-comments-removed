





































#include "StdAfx.h"

#include "ControlEventSink.h"

CControlEventSink::CControlEventSink() :
    m_dwEventCookie(0),
    m_EventIID(GUID_NULL)
{
}

CControlEventSink::~CControlEventSink()
{
    UnsubscribeFromEvents();
}

BOOL
CControlEventSink::GetEventSinkIID(IUnknown *pControl, IID &iid, ITypeInfo **typeInfo)
{
	iid = GUID_NULL;
    if (!pControl)
    {
        return E_INVALIDARG;
    }

	










    
    CComQIPtr<IProvideClassInfo> classInfo = pControl;
    if (!classInfo)
    {
        return FALSE;
    }

    
    

    CComPtr<ITypeInfo> classTypeInfo;
    classInfo->GetClassInfo(&classTypeInfo);
    if (!classTypeInfo)
    {
        return FALSE;
    }
    TYPEATTR *classAttr = NULL;
    if (FAILED(classTypeInfo->GetTypeAttr(&classAttr)))
    {
        return FALSE;
    }
    INT implFlags = 0;
    for (UINT i = 0; i < classAttr->cImplTypes; i++)
    {
        
        if (SUCCEEDED(classTypeInfo->GetImplTypeFlags(i, &implFlags)) &&
            implFlags == (IMPLTYPEFLAG_FDEFAULT | IMPLTYPEFLAG_FSOURCE))
        {
            CComPtr<ITypeInfo> eventSinkTypeInfo;
            HREFTYPE hRefType;
            if (SUCCEEDED(classTypeInfo->GetRefTypeOfImplType(i, &hRefType)) &&
                SUCCEEDED(classTypeInfo->GetRefTypeInfo(hRefType, &eventSinkTypeInfo)))
            {
                TYPEATTR *eventSinkAttr = NULL;
                if (SUCCEEDED(eventSinkTypeInfo->GetTypeAttr(&eventSinkAttr)))
                {
                    iid = eventSinkAttr->guid;
                    if (typeInfo)
                    {
                        *typeInfo = eventSinkTypeInfo.p;
                        (*typeInfo)->AddRef();
                    }
                    eventSinkTypeInfo->ReleaseTypeAttr(eventSinkAttr);
                }
            }
            break;
        }
    }
    classTypeInfo->ReleaseTypeAttr(classAttr);

    return (!::IsEqualIID(iid, GUID_NULL));
}

void CControlEventSink::UnsubscribeFromEvents()
{
    if (m_spEventCP)
    {
        
        m_spEventCP->Unadvise(m_dwEventCookie);
        m_dwEventCookie = 0;
        m_spEventCP.Release();
    }
}

HRESULT CControlEventSink::SubscribeToEvents(IUnknown *pControl)
{
    if (!pControl)
    {
        return E_INVALIDARG;
    }

    
    UnsubscribeFromEvents();

    
    

    IID iidEventSink;
    CComPtr<ITypeInfo> typeInfo;
    if (!GetEventSinkIID(pControl, iidEventSink, &typeInfo))
    {
        return E_FAIL;
    }

    
    CComQIPtr<IConnectionPointContainer> ccp = pControl;
    CComPtr<IConnectionPoint> cp;
    if (!ccp)
    {
        return E_FAIL;
    }

    
    m_EventIID = iidEventSink;
    DWORD dwCookie = 0;
    if (!ccp ||
        FAILED(ccp->FindConnectionPoint(m_EventIID, &cp)) ||
        FAILED(cp->Advise(this, &dwCookie)))
    {
        return E_FAIL;
    }

    m_spEventCP = cp;
    m_dwEventCookie = dwCookie;
    m_spEventSinkTypeInfo = typeInfo;
    return S_OK;
}

HRESULT
CControlEventSink::InternalInvoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    
    return E_NOTIMPL;
}





HRESULT STDMETHODCALLTYPE CControlEventSink::GetTypeInfoCount( UINT __RPC_FAR *pctinfo)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CControlEventSink::GetTypeInfo( UINT iTInfo,  LCID lcid,  ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CControlEventSink::GetIDsOfNames( REFIID riid,  LPOLESTR __RPC_FAR *rgszNames,  UINT cNames,  LCID lcid,  DISPID __RPC_FAR *rgDispId)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CControlEventSink::Invoke( DISPID dispIdMember,  REFIID riid,  LCID lcid,  WORD wFlags,  DISPPARAMS __RPC_FAR *pDispParams,  VARIANT __RPC_FAR *pVarResult,  EXCEPINFO __RPC_FAR *pExcepInfo,  UINT __RPC_FAR *puArgErr)
{
    return InternalInvoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}


