


#if !defined(AFX_REGMOZCTLDLG_H__C7C0A788_F424_11D2_A27B_000000000000__INCLUDED_)
#define AFX_REGMOZCTLDLG_H__C7C0A788_F424_11D2_A27B_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 

#include "RegTaskManager.h"




class CRegMozCtlDlg : public CDialog
{

public:
	CRegMozCtlDlg(CWnd* pParent = NULL);	


	
	enum { IDD = IDD_REGMOZCTL_DIALOG };
	CListCtrl	m_cTaskList;
	CString	m_szMozillaDir;
	

	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	
	


protected:
	HICON m_hIcon;
    CRegTaskManager m_cTaskMgr;


	
	
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnRegister();
	afx_msg void OnUnregister();
	afx_msg void OnPickDir();
	
	DECLARE_MESSAGE_MAP()

	void RegisterMozillaControl(BOOL bRegister);
	BOOL SetSystemPath(const CString &szNewPath);
	CString GetSystemPath();
};




#endif 
