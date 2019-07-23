




































#if !defined(AFX_BROWSERTOOLTIP_H__13240069_1816_403C_A79D_306FD710B75A__INCLUDED_)
#define AFX_BROWSERTOOLTIP_H__13240069_1816_403C_A79D_306FD710B75A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 






class CBrowserToolTip : public CWnd
{

public:
	CBrowserToolTip();


public:


public:


	
	
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	

	CString m_szTipText;

public:
	virtual ~CBrowserToolTip();

    BOOL Create(CWnd *pParentWnd);
	void SetTipText(const CString &szTipText);
    void Show(CWnd *pOverWnd, long left, long top);
    void Hide();


	
protected:
	
	afx_msg void OnPaint();
	
	DECLARE_MESSAGE_MAP()
};






#endif