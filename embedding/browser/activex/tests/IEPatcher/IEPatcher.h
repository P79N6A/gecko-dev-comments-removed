


#if !defined(AFX_IEPATCHER_H__A603167A_3B36_11D2_B44D_00600819607E__INCLUDED_)
#define AFX_IEPATCHER_H__A603167A_3B36_11D2_B44D_00600819607E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif 

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		

#include "IEPatcherDlg.h"






class CIEPatcherApp : public CWinApp
{
public:
	CIEPatcherApp();

	CIEPatcherDlg *m_pIEPatcherDlg;


	
	
	public:
	virtual BOOL InitInstance();
	



	
		
		
	
	DECLARE_MESSAGE_MAP()
};







#endif
