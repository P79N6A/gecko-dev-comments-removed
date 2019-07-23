


#include "stdafx.h"
#include "iepatcher.h"
#include "ScannerThread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




IMPLEMENT_DYNCREATE(CScannerThread, CWinThread)

CScannerThread::CScannerThread()
{
}

CScannerThread::~CScannerThread()
{
}

BOOL CScannerThread::InitInstance()
{
	m_cScannerWnd.Create(AfxRegisterWndClass(0), _T("Scanner"), 0L, CRect(0, 0, 0, 0), AfxGetMainWnd(), 1);
	return TRUE;
}

int CScannerThread::ExitInstance()
{
	
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CScannerThread, CWinThread)
	
		
	
END_MESSAGE_MAP()



