#if !defined(AFX_PICKERDLG_H__648FC652_D34B_11D2_A255_000000000000__INCLUDED_)
#define AFX_PICKERDLG_H__648FC652_D34B_11D2_A255_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif 






class CPickerDlg : public CDialog
{

public:
	CPickerDlg(CWnd* pParent = NULL);   


	
	enum { IDD = IDD_PICKBROWSER };
	CListBox	m_lbPicker;
	CString	m_szTestURL;
	CString	m_szTestCGI;
	BOOL	m_bDebugging;
	CString	m_sDebugFlags;
	BOOL	m_bUseCustom;
	

	CLSID m_clsid;


	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:

	
	
	afx_msg void OnOk();
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkListbrowser();
	
	DECLARE_MESSAGE_MAP()
};




#endif
