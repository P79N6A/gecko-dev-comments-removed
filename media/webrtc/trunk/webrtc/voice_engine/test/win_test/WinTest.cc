









#include "stdafx.h"
#include "WinTest.h"
#include "WinTestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif




BEGIN_MESSAGE_MAP(CWinTestApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()




CWinTestApp::CWinTestApp()
{
}




CWinTestApp theApp;




BOOL CWinTestApp::InitInstance()
{
	
	
	
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	
	
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	
	
	
	
	
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CWinTestDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}

	
	
	return FALSE;
}
