


#include "stdafx.h"
#include "IEPatcher.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




BEGIN_MESSAGE_MAP(CIEPatcherApp, CWinApp)
	
		
		
	
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()




CIEPatcherApp::CIEPatcherApp()
{
	m_pIEPatcherDlg = NULL;
}




CIEPatcherApp theApp;




BOOL CIEPatcherApp::InitInstance()
{
	
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	
	
	
	

#ifdef _AFXDLL
	Enable3dControls();			
#else
	Enable3dControlsStatic();	
#endif

	
	if (RunEmbedded() || RunAutomated())
	{
		
		
		COleTemplateServer::RegisterAll();
	}
	else
	{
		
		
		COleObjectFactory::UpdateRegistryAll();
	}

	
	m_pIEPatcherDlg = new CIEPatcherDlg;
	m_pMainWnd = m_pIEPatcherDlg;
	int nResponse = m_pIEPatcherDlg->DoModal();
	if (nResponse == IDOK)
	{
		
		
	}
	else if (nResponse == IDCANCEL)
	{
		
		
	}
	delete m_pIEPatcherDlg;

	
	
	return FALSE;
}
