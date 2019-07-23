






























#if !defined(AFX_CCUSTOMPROMPTDIALOG_H__6C159F14_88A7_465F_993B_76B03D670DE5__INCLUDED_)
#define AFX_CCUSTOMPROMPTDIALOG_H__6C159F14_88A7_465F_993B_76B03D670DE5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 






class CCustomPromptDialog : public CDialog
{

public:
	CCustomPromptDialog(CWnd* pParent = NULL);   


	
	enum { IDD = IDD_CUSTOM_PROMPT_DIALOG };
	CString	m_CustomText;
	



	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:

	
	
		
	
	DECLARE_MESSAGE_MAP()
};




#endif
