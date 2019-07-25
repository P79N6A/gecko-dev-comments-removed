

#ifndef __TESTSCRIPTHELPER_H_
#define __TESTSCRIPTHELPER_H_

#include "resource.h"       



class ATL_NO_VTABLE CTestScriptHelper : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTestScriptHelper, &CLSID_TestScriptHelper>,
	public IDispatchImpl<DITestScriptHelper, &IID_DITestScriptHelper, &LIBID_CbrowseLib>
{
public:
	CTestScriptHelper()
	{
		m_pBrowserInfo = NULL;
	}

	BrowserInfo *m_pBrowserInfo;

DECLARE_REGISTRY_RESOURCEID(IDR_TESTSCRIPTHELPER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTestScriptHelper)
	COM_INTERFACE_ENTRY(DITestScriptHelper)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()


public:
	STDMETHOD(get_TestCGI)( BSTR *pVal);
	STDMETHOD(get_TestURL)( BSTR *pVal);
	STDMETHOD(put_Result)( TestResult newVal);
	STDMETHOD(get_WebBrowser)( LPDISPATCH *pVal);
	STDMETHOD(OutputString)(BSTR bstrMessage);
};

typedef CComObject<CTestScriptHelper> CTestScriptHelperInstance;

#endif 
