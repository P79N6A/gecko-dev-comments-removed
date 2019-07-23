#if !defined(AFX_URLDIALOG_H__260C4EE1_2F8E_11D5_99F0_00C04FA02BE6__INCLUDED_)
#define AFX_URLDIALOG_H__260C4EE1_2F8E_11D5_99F0_00C04FA02BE6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 






class CUrlDialog : public CDialog
{

public:
	CUrlDialog(CWnd* pParent = NULL);   

    PRUint32 m_flagvalue;

	
	enum { IDD = IDD_URLDIALOG };
	CButton	m_chkFlags;
	CComboBox	m_urlflags;
	CString	m_urlfield;
	int		m_flagIndex;
	int		m_protocolIndex;
	BOOL	m_chkValue;
	CComboBox m_protocol;
	CString m_protocolvalue;
	



	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:

	
	
	afx_msg void OnChangeUrlfield();
	afx_msg void OnChkurlflag();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeCombo1();
	afx_msg void OnSelchangeCombo2();
	
	DECLARE_MESSAGE_MAP()
};




#endif
