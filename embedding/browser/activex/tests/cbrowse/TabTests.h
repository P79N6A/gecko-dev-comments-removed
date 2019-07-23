#if !defined(AFX_TABTESTS_H__33D0A351_12E1_11D3_9407_000000000000__INCLUDED_)
#define AFX_TABTESTS_H__33D0A351_12E1_11D3_9407_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 



class CBrowseDlg;




class CTabTests : public CPropertyPage
{
	DECLARE_DYNCREATE(CTabTests)


public:
	CTabTests();
	~CTabTests();

	CBrowseDlg *m_pBrowseDlg;


	
	enum { IDD = IDD_TAB_TESTS };
	CButton	m_btnRunTest;
	CTreeCtrl	m_tcTests;
	CString	m_szTestDescription;
	



	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:
	
	
	afx_msg void OnRunTest();
	afx_msg void OnSelchangedTestlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkTestlist(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	
	DECLARE_MESSAGE_MAP()

};




#endif
