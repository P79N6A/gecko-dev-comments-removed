


#if !defined(AFX_REGMOZCTL_H__C7C0A786_F424_11D2_A27B_000000000000__INCLUDED_)
#define AFX_REGMOZCTL_H__C7C0A786_F424_11D2_A27B_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		






class CRegMozCtlApp : public CWinApp
{
public:
	CRegMozCtlApp();


	
	
	public:
	virtual BOOL InitInstance();
	



	
		
		
	
	DECLARE_MESSAGE_MAP()
};

static const TCHAR *c_szValueBinDirPath = _T("BinDirPath");






#endif
