#if !defined(AFX_SCANNERTHREAD_H__2CAAFC04_6C47_11D2_A1F5_000000000000__INCLUDED_)
#define AFX_SCANNERTHREAD_H__2CAAFC04_6C47_11D2_A1F5_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif 



#include "ScannerWnd.h"





class CScannerThread : public CWinThread
{
	DECLARE_DYNCREATE(CScannerThread)
protected:
	CScannerThread();           


public:
	CScannerWnd m_cScannerWnd;


public:


	
	
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	


protected:
	virtual ~CScannerThread();

	
	
		
	

	DECLARE_MESSAGE_MAP()
};






#endif
