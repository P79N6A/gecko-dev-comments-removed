#if !defined(AFX_PROFILESDLG_H__48358887_EBFA_11D4_9905_00B0D0235410__INCLUDED_)
#define AFX_PROFILESDLG_H__48358887_EBFA_11D4_9905_00B0D0235410__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 






class CNewProfileDlg : public CDialog
{

public:
	CNewProfileDlg(CWnd* pParent = NULL);   


	
	enum { IDD = IDD_PROFILE_NEW };
	int		m_LocaleIndex;
	CString	m_Name;
	



	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:

	
	
		
	
	DECLARE_MESSAGE_MAP()
};




class CRenameProfileDlg : public CDialog
{

public:
	CRenameProfileDlg(CWnd* pParent = NULL);   


	
	enum { IDD = IDD_PROFILE_RENAME };
	CString	m_NewName;
	

    CString     m_CurrentName;


	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:

	
	
		
	
	DECLARE_MESSAGE_MAP()
};




class CProfilesDlg : public CDialog
{

public:
	CProfilesDlg(CWnd* pParent = NULL);   


	
	enum { IDD = IDD_PROFILES };
	CListBox	m_ProfileList;
    BOOL        m_bAtStartUp;
	BOOL	    m_bAskAtStartUp;
	

    nsAutoString m_SelectedProfile;


	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:

	
	
	virtual BOOL OnInitDialog();
	afx_msg void OnNewProfile();
	afx_msg void OnRenameProfile();
	afx_msg void OnDeleteProfile();
	
	DECLARE_MESSAGE_MAP()
};




#endif
