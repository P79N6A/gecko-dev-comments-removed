



#if !defined(AFX_NSXPCDISPTESTARRAYS_H__5F59BD4C_16A4_4BD6_8281_796DE6A2889C__INCLUDED_)
#define AFX_NSXPCDISPTESTARRAYS_H__5F59BD4C_16A4_4BD6_8281_796DE6A2889C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 

#include "resource.h"       




class nsXPCDispTestArrays : 
    public IDispatchImpl<nsIXPCDispTestArrays, &IID_nsIXPCDispTestArrays, &LIBID_IDispatchTestLib>,
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<nsXPCDispTestArrays,&CLSID_nsXPCDispTestArrays>
{
public:
    nsXPCDispTestArrays() {}
BEGIN_CATEGORY_MAP(nsXPCDispTestArrays)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
END_CATEGORY_MAP()
BEGIN_COM_MAP(nsXPCDispTestArrays)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(nsIXPCDispTestArrays)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(nsXPCDispTestArrays) 

DECLARE_REGISTRY_RESOURCEID(IDR_nsXPCDispTestArrays)

    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


public:

    STDMETHOD(ReturnSafeArray)(LPSAFEARRAY * result);
    STDMETHOD(ReturnSafeArrayBSTR)(LPSAFEARRAY * result);
    STDMETHOD(ReturnSafeArrayIDispatch)(LPSAFEARRAY * result);
    STDMETHOD(TakesSafeArray)(LPSAFEARRAY array);
    STDMETHOD(TakesSafeArrayBSTR)(LPSAFEARRAY array);
    STDMETHOD(TakesSafeArrayIDispatch)(LPSAFEARRAY array);
    STDMETHOD(InOutSafeArray)(LPSAFEARRAY * array);
    STDMETHOD(InOutSafeArrayBSTR)(LPSAFEARRAY * array);
    STDMETHOD(InOutSafeArrayIDispatch)(LPSAFEARRAY * array);
};

#endif 
