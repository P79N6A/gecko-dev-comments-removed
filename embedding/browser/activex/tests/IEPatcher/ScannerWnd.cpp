


#include "stdafx.h"
#include "iepatcher.h"
#include "ScannerWnd.h"
#include "IEPatcherDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




CScannerWnd::CScannerWnd()
{
}

CScannerWnd::~CScannerWnd()
{
}


BEGIN_MESSAGE_MAP(CScannerWnd, CWnd)
	
	ON_WM_TIMER()
	ON_WM_CREATE()
	
END_MESSAGE_MAP()






int CScannerWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetTimer(1, 500, NULL);
	
	return 0;
}


void CScannerWnd::OnTimer(UINT nIDEvent) 
{
	CIEPatcherDlg * pDlg = ((CIEPatcherApp *) AfxGetApp())->m_pIEPatcherDlg;
	if (pDlg)
	{
		CString sFileToProcess;
		if (pDlg->GetPendingFileToScan(sFileToProcess))
		{
			PatchStatus ps = CIEPatcherDlg::ScanFile(sFileToProcess);
			AfxGetMainWnd()->SendMessage(WM_UPDATEFILESTATUS, (WPARAM) ps, (LPARAM) (const TCHAR *) sFileToProcess);
		}
		else if (pDlg->GetPendingFileToPatch(sFileToProcess))
		{
			TCHAR szDrive[_MAX_DRIVE];
			TCHAR szDir[_MAX_DIR];
			TCHAR szFile[_MAX_FNAME];
			TCHAR szExt[_MAX_EXT];

			_tsplitpath(sFileToProcess, szDrive, szDir, szFile, szExt);

			CString sFileOut;
			TCHAR szPath[_MAX_PATH];
			sFileOut.Format(_T("moz_%s"), szFile);
			_tmakepath(szPath, szDrive, szDir, sFileOut, szExt);
			
			PatchStatus ps;
			CIEPatcherDlg::PatchFile(sFileToProcess, szPath, &ps);
			AfxGetMainWnd()->SendMessage(WM_UPDATEFILESTATUS, (WPARAM) ps, (LPARAM) (const TCHAR *) sFileToProcess);
		}
	}
	
	CWnd::OnTimer(nIDEvent);
}
