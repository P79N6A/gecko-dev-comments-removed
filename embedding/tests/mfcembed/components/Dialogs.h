






























#ifndef _DIALOGS_H_
#define _DIALOGS_H_

#include "resource.h"

class CPromptDialog : public CDialog
{
public:
    CPromptDialog(CWnd* pParent, const TCHAR* pTitle, const TCHAR* pText,
                  const TCHAR* pInitPromptText,
                  BOOL bHasCheck, const TCHAR* pCheckText, int initCheckVal);
	
	
    enum { IDD = IDD_PROMPT_DIALOG };
    CString m_csPromptAnswer;

    CString m_csDialogTitle;
	CString m_csPromptText;
	BOOL m_bHasCheckBox;
	CString m_csCheckBoxText;
	int m_bCheckBoxValue;
    
	
	
	protected:
    virtual void DoDataExchange(CDataExchange* pDX);
	

	
	virtual BOOL OnInitDialog();
	
    DECLARE_MESSAGE_MAP()
};

class CPromptPasswordDialog : public CDialog
{
public:
    CPromptPasswordDialog(CWnd* pParent, const TCHAR* pTitle, const TCHAR* pText,
                          const TCHAR* pInitPasswordText,
                          BOOL bHasCheck, const TCHAR* pCheckText, int initCheckVal);
	
	
    enum { IDD = IDD_PROMPT_PASSWORD_DIALOG };

    CString m_csDialogTitle;
	CString m_csPromptText;
	CString m_csPassword;
	BOOL m_bHasCheckBox;
	CString m_csCheckBoxText;
	int m_bCheckBoxValue;

	
	
	protected:
    virtual void DoDataExchange(CDataExchange* pDX);
	

	
	virtual BOOL OnInitDialog();
	
    DECLARE_MESSAGE_MAP()
};

class CPromptUsernamePasswordDialog : public CDialog
{
public:
    CPromptUsernamePasswordDialog(CWnd* pParent, const TCHAR* pTitle, const TCHAR* pText,
                                  const TCHAR* pInitUsername, const TCHAR* pInitPassword, 
		                          BOOL bHasCheck, const TCHAR* pCheckText, int initCheckVal);
	
	
    enum { IDD = IDD_PROMPT_USERPASS_DIALOG };

    CString m_csDialogTitle;
	CString m_csPromptText;
	CString m_csUserNameLabel;
	CString m_csPasswordLabel;
	CString m_csPassword;
	CString m_csUserName;
	BOOL m_bHasCheckBox;
	CString m_csCheckBoxText;
	int m_bCheckBoxValue;

	
	
	protected:
    virtual void DoDataExchange(CDataExchange* pDX);
	

	
	virtual BOOL OnInitDialog();
	

    DECLARE_MESSAGE_MAP()
};

class CAlertCheckDialog : public CDialog
{
public:
    CAlertCheckDialog(CWnd* pParent, const TCHAR* pTitle, const TCHAR* pText,
                  const TCHAR* pCheckText, int initCheckVal);
	
    
    enum { IDD = IDD_ALERT_CHECK_DIALOG };

    CString m_csDialogTitle;
    CString m_csMsgText;
    CString m_csCheckBoxText;
    int m_bCheckBoxValue;
    
    
    
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    

    
    virtual BOOL OnInitDialog();
    

    DECLARE_MESSAGE_MAP()
};

class CConfirmCheckDialog : public CDialog
{
public:
    CConfirmCheckDialog(CWnd* pParent, const TCHAR* pTitle, const TCHAR* pText,
                  const TCHAR* pCheckText, int initCheckVal,
                  const TCHAR *pBtn1Text, const TCHAR *pBtn2Text, 
                  const TCHAR *pBtn3Text);
	
    
    enum { IDD = IDD_CONFIRM_CHECK_DIALOG };

    CString m_csDialogTitle;
    CString m_csMsgText;
    CString m_csCheckBoxText;
    int m_bCheckBoxValue;
    CString m_csBtn1Text;
    CString m_csBtn2Text;
    CString m_csBtn3Text;

    
    
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    

    
    virtual BOOL OnInitDialog();
    afx_msg void OnBtn1Clicked();
    afx_msg void OnBtn2Clicked();
    afx_msg void OnBtn3Clicked();
    

    DECLARE_MESSAGE_MAP()
};

#endif
