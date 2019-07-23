#if !defined(AFX_QAFINDDLG_H__D27E3DAF_8479_11D6_9BC7_00C04FA02BE6__INCLUDED_)
#define AFX_QAFINDDLG_H__D27E3DAF_8479_11D6_9BC7_00C04FA02BE6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 






class CQaFindDlg : public CDialog
{

public:
	CQaFindDlg(CWnd* pParent = NULL);   


	
	enum { IDD = IDD_QAFINDDLG };
		
	
	CString	m_textfield;


	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:

	
	
		
	
	DECLARE_MESSAGE_MAP()
};




#endif
