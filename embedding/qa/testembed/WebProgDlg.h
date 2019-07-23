#if !defined(AFX_WEBPROGDLG_H__79FFB433_9E6A_11D6_9BD9_00C04FA02BE6__INCLUDED_)
#define AFX_WEBPROGDLG_H__79FFB433_9E6A_11D6_9BD9_00C04FA02BE6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 






class CWebProgDlg : public CDialog
{

public:
	CWebProgDlg(CWnd* pParent = NULL);   
	unsigned long m_wpFlagValue;

	
	enum { IDD = IDD_WEBPROGDLG };
	CComboBox	m_webProgFlags;
	int			m_wpFlagIndex;
		
	



	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:

	
	
	virtual BOOL OnInitWPDialog();
	afx_msg	void OnSelectWPCombo();
	
	DECLARE_MESSAGE_MAP()
};




#endif
