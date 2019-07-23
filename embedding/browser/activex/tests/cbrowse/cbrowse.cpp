


#include "stdafx.h"
#include "cbrowse.h"
#include "PickerDlg.h"
#include <initguid.h>
#include "Cbrowse_i.c"
#include "TestScriptHelper.h"
#include "ControlEventSink.h"
#include "CBrowserCtlSite.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




BEGIN_MESSAGE_MAP(CBrowseApp, CWinApp)
	
	
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()




CBrowseApp::CBrowseApp()
{
	
	
}




CBrowseApp theApp;




BOOL CBrowseApp::InitInstance()
{
	
	if (!AfxOleInit())
	{
		return FALSE;
	}



	AfxEnableControlContainer();
	_Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
		REGCLS_MULTIPLEUSE);


	
	
	
	

#ifdef _AFXDLL
	Enable3dControls();			
#else
	Enable3dControlsStatic();	
#endif

	int nResponse;

	CPickerDlg pickerDlg;
	nResponse = pickerDlg.DoModal();
	if (nResponse != IDOK)
	{
		return FALSE;
	}

	m_pDlg = new CBrowseDlg;
	m_pDlg->m_clsid = pickerDlg.m_clsid;
	m_pDlg->m_bUseCustomDropTarget = pickerDlg.m_bUseCustom;
	m_pDlg->m_bUseCustomPopupMenu = pickerDlg.m_bUseCustom;
	m_pDlg->Create(IDD_CBROWSE_DIALOG);
	m_pMainWnd = m_pDlg;

	
	
	return TRUE;
}

CBrowseModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_TestScriptHelper, CTestScriptHelper)


END_OBJECT_MAP()

LONG CBrowseModule::Unlock()
{
	AfxOleUnlockApp();
	return 0;
}

LONG CBrowseModule::Lock()
{
	AfxOleLockApp();
	return 1;
}

LPCTSTR CBrowseModule::FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
	while (*p1 != NULL)
	{
		LPCTSTR p = p2;
		while (*p != NULL)
		{
			if (*p1 == *p)
				return CharNext(p1);
			p = CharNext(p);
		}
		p1++;
	}
	return NULL;
}


int CBrowseApp::ExitInstance()
{
	if (m_bATLInited)
	{
		_Module.RevokeClassObjects();
		_Module.Term();
		CoUninitialize();
	}
	return CWinApp::ExitInstance();
}


BOOL CBrowseApp::InitATL()
{
	m_bATLInited = TRUE;

	HRESULT hRes;

#if _WIN32_WINNT >= 0x0400
	
#else
	
#endif
	if (!AfxOleInit())
	{
		return FALSE;
	}


	
	
	
	
	

	_Module.Init(ObjectMap, AfxGetInstanceHandle());
	_Module.dwThreadID = GetCurrentThreadId();

	LPTSTR lpCmdLine = GetCommandLine(); 
	TCHAR szTokens[] = _T("-/");

	BOOL bRun = TRUE;
	LPCTSTR lpszToken = _Module.FindOneOf(lpCmdLine, szTokens);
	while (lpszToken != NULL)
	{
		if (lstrcmpi(lpszToken, _T("UnregServer"))==0)
		{
			_Module.UpdateRegistryFromResource(IDR_CBROWSE, FALSE);
			_Module.UnregisterServer(TRUE); 
			bRun = FALSE;
			break;
		}
		if (lstrcmpi(lpszToken, _T("RegServer"))==0)
		{
			_Module.UpdateRegistryFromResource(IDR_CBROWSE, TRUE);
			_Module.RegisterServer(TRUE);
			bRun = FALSE;
			break;
		}
		lpszToken = _Module.FindOneOf(lpszToken, szTokens);
	}

	if (!bRun)
	{
		m_bATLInited = FALSE;
		_Module.Term();
		CoUninitialize();
		return FALSE;
	}

	hRes = _Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER, 
		REGCLS_MULTIPLEUSE);
	if (FAILED(hRes))
	{
		m_bATLInited = FALSE;
		CoUninitialize();
		return FALSE;
	}	

	return TRUE;
}

int CBrowseApp::Run() 
{
    int rv = 1;
    try {
        rv = CWinApp::Run();
    }
    catch (CException *e)
    {
        ASSERT(0);
    }
    catch (...)
    {
        ASSERT(0);
    }
    return rv;
}
