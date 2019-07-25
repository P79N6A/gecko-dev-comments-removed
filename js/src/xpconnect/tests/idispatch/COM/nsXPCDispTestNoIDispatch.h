



#if !defined(AFX_NSXPCDISPTESTNOIDISPATCH_H__E4B74F67_BA6B_4654_8674_E60E487129F7__INCLUDED_)
#define AFX_NSXPCDISPTESTNOIDISPATCH_H__E4B74F67_BA6B_4654_8674_E60E487129F7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 

#include "resource.h"       




class nsXPCDispTestNoIDispatch :
    public nsIXPCDispTestNoIDispatch,
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<nsXPCDispTestNoIDispatch,&CLSID_nsXPCDispTestNoIDispatch>
{
public:
    nsXPCDispTestNoIDispatch() {}
BEGIN_COM_MAP(nsXPCDispTestNoIDispatch)
    COM_INTERFACE_ENTRY(nsIXPCDispTestNoIDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()




DECLARE_REGISTRY_RESOURCEID(IDR_nsXPCDispTestNoIDispatch)

    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


public:
};

#endif 
