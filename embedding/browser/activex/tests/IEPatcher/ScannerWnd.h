#if !defined(AFX_SCANNERWND_H__2CAAFC05_6C47_11D2_A1F5_000000000000__INCLUDED_)
#define AFX_SCANNERWND_H__2CAAFC05_6C47_11D2_A1F5_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif 






class CScannerWnd : public CWnd
{

public:
	CScannerWnd();


public:


public:


	
	
	


public:
	virtual ~CScannerWnd();

	
protected:
	
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	
	DECLARE_MESSAGE_MAP()
};






#endif
