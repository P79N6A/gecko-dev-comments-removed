#if !defined(AFX_TABMESSAGES_H__33D0A350_12E1_11D3_9407_000000000000__INCLUDED_)
#define AFX_TABMESSAGES_H__33D0A350_12E1_11D3_9407_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 



class CBrowseDlg;




class CTabMessages : public CPropertyPage
{
	DECLARE_DYNCREATE(CTabMessages)


public:
	CTabMessages();
	~CTabMessages();

	CBrowseDlg *m_pBrowseDlg;


	
	enum { IDD = IDD_TAB_MESSAGES };
	CProgressCtrl	m_pcProgress;
	CListBox	m_lbMessages;
	CString	m_szStatus;
	



	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:
	
	
		
	
	DECLARE_MESSAGE_MAP()

};




#endif
