#if !defined(AFX_SCANFORFILESDLG_H__2CAAFC02_6C47_11D2_A1F5_000000000000__INCLUDED_)
#define AFX_SCANFORFILESDLG_H__2CAAFC02_6C47_11D2_A1F5_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif 






class CScanForFilesDlg : public CDialog
{

public:
	CScanForFilesDlg(CWnd* pParent = NULL);   


	
	enum { IDD = IDD_SCANFORFILES };
	CString	m_sFilePattern;
	



	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:

	
	
	virtual void OnOK();
	afx_msg void OnSelectFile();
	afx_msg void OnSelectFolder();
	
	DECLARE_MESSAGE_MAP()
};




#endif
