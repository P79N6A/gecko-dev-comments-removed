






























#if !defined(AFX_CMARGINHEADERFOOTER_H__A95A9A17_692E_425B_8B70_419C24DDE6BC__INCLUDED_)
#define AFX_CMARGINHEADERFOOTER_H__A95A9A17_692E_425B_8B70_419C24DDE6BC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 






class CMarginHeaderFooter : public CPropertyPage
{
	DECLARE_DYNCREATE(CMarginHeaderFooter)


public:
	CMarginHeaderFooter();
	~CMarginHeaderFooter();


	
	enum { IDD = IDD_HEADERFOOTER_TAB };
	CString	m_BottomMarginText;
	CString	m_LeftMarginText;
	CString	m_RightMarginText;
	CString	m_TopMarginText;
	

	CString	m_FooterLeftText;
	CString	m_FooterCenterText;
	CString	m_FooterRightText;
	CString	m_HeaderLeftText;
	CString	m_HeaderCenterText;
	CString	m_HeaderRightText;


	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:
  void SetCombobox(int aId, CString& aText);
  void SetComboboxValue(int aId, const TCHAR * aValue);
  void AddCBXItem(int aId, const TCHAR * aItem);

	
	
	virtual BOOL OnInitDialog();
	afx_msg void OnEditchangeFTRLeft();
	afx_msg void OnEditchangeFTRCenter();
	afx_msg void OnEditchangeFTRRight();
	afx_msg void OnEditchangeHDRLeft();
	afx_msg void OnEditchangeHDRCenter();
	afx_msg void OnEditchangeHDRRight();
	
	DECLARE_MESSAGE_MAP()

};




#endif
