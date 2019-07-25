#if !defined(AFX_TABDOM_H__5216DE08_12D4_11D3_9407_000000000000__INCLUDED_)
#define AFX_TABDOM_H__5216DE08_12D4_11D3_9407_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 



class CBrowseDlg;




class CTabDOM : public CPropertyPage
{
	DECLARE_DYNCREATE(CTabDOM)


public:
	CTabDOM();
	~CTabDOM();

	CBrowseDlg *m_pBrowseDlg;


	
	enum { IDD = IDD_TAB_DOM };
	CTreeCtrl	m_tcDOM;
	



	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:
	
	
	afx_msg void OnRefreshDOM();
	virtual BOOL OnInitDialog();
	
	DECLARE_MESSAGE_MAP()

};




#endif
