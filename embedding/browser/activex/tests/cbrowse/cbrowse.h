


#if !defined(AFX_CBROWSE_H__5121F5E4_5324_11D2_93E1_000000000000__INCLUDED_)
#define AFX_CBROWSE_H__5121F5E4_5324_11D2_93E1_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif 

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		
#include "Cbrowse_i.h"
#include "CBrowseDlg.h"






class CBrowseApp : public CWinApp
{
public:
	CBrowseApp();


	
	
	public:
	virtual BOOL InitInstance();
		virtual int ExitInstance();
	virtual int Run();
	



	
	
	DECLARE_MESSAGE_MAP()
private:

	CBrowseDlg *m_pDlg;
	BOOL m_bATLInited;
private:
	BOOL InitATL();

};







#endif 
